#include "PCH.h"

#include "BrowserPad.h"

#include "Functions.h"
#include "Inspect.h"
#include "ZoomKey.h"

namespace BrowserPad
{
	namespace
	{
		constexpr float kHold = 0.5F;
		constexpr float kDeadZone = 0.2F;
		constexpr float kItemDegrees = 180.0F;
		// steps of the page a second at full tilt, a step is a few lines
		constexpr float kPageSteps = 8.0F;
		// a frame longer than this is a hitch or the first one after a while, it doesn't count more
		constexpr float kLongestFrame = 0.1F;
		constexpr auto kStickScript = "_global.KnowYourSkyrim.padStick";
		constexpr auto kScrollScript = "_global.KnowYourSkyrim.padScroll";
		constexpr auto kZoomScript = "_global.KnowYourSkyrim.zoomKey";
		// the panel of Inventory Reforged takes the zoom key itself when it's there
		constexpr auto kPanelPlugin = "Data/SKSE/Plugins/InventoryReforged.dll";

		RE::NiPoint2 stick{};
		Turns turns = Turns::kNothing;
		bool holdTold = false;
		float steps = 0.0F;
		std::optional<std::chrono::steady_clock::time_point> lastFrame;

		// A book read from the inventory or a message box takes the menu input over it, the stick isn't the browser's
		// then. Any open menu that stops the game, reads menu input and stands above the inventory counts
		bool InventoryOnTop(RE::UI& a_ui, const RE::IMenu& a_inventory)
		{
			for (const auto& menu : a_ui.menuStack) {
				if (menu && menu.get() != &a_inventory && menu->PausesGame() && menu->UsesMenuContext() && menu->depthPriority > a_inventory.depthPriority) {
					return false;
				}
			}
			return true;
		}

		void Tell(const char* a_script, const RE::GFxValue& a_value)
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
				return;
			}
			if (const auto menu = ui->GetMenu(RE::InventoryMenu::MENU_NAME); menu && menu->uiMovie && InventoryOnTop(*ui, *menu)) {
				menu->uiMovie->Invoke(a_script, nullptr, &a_value, 1);
			}
		}

		// the zoom control of a gamepad. The event may come named by another context of the menu, its button tells it then
		bool IsZoom(const RE::ButtonEvent& a_button)
		{
			return a_button.GetDevice() == RE::INPUT_DEVICE::kGamepad &&
			       (a_button.QUserEvent() == RE::UserEvents::GetSingleton()->itemZoom || a_button.GetIDCode() == RE::BSWin32GamepadDevice::Keys::kRightThumb);
		}

		// A button event comes on the press, every frame it's held and on the let go. A let go before the hold time
		// is a click, the hold is told once when it's reached and its let go says nothing. A let go that comes late
		// with no held event before it is a hold all the same
		class InputWatch : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events, RE::BSTEventSource<RE::InputEvent*>*) override
			{
				for (auto* event = a_events ? *a_events : nullptr; event; event = event->next) {
					if (const auto* thumb = event->AsThumbstickEvent(); thumb && thumb->IsRight()) {
						stick = { thumb->xValue, thumb->yValue };
						continue;
					}
					const auto* button = event->AsButtonEvent();
					if (!button || !IsZoom(*button)) {
						continue;
					}
					if (button->IsDown()) {
						holdTold = false;
					} else if (button->IsHeld() && !holdTold && button->HeldDuration() >= kHold) {
						holdTold = true;
						Tell(kStickScript, RE::GFxValue(true));
					} else if (button->IsUp() && !holdTold) {
						Tell(kStickScript, RE::GFxValue(button->HeldDuration() >= kHold));
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		float Tilt(float a_value)
		{
			return std::abs(a_value) > kDeadZone ? a_value : 0.0F;
		}

		struct PadTurns
		{
			static void Run(Functions::Params& a_params)
			{
				if (a_params.argCount == 1 && a_params.args[0].IsNumber()) {
					SetTurns(static_cast<Turns>(std::clamp(static_cast<int>(a_params.args[0].GetNumber()), 0, 2)));
				}
			}
		};
	}

	void InstallZoomKey()
	{
		std::error_code failed;
		if (std::filesystem::exists(kPanelPlugin, failed)) {
			logger::info("Inventory Reforged takes the item zoom key, the browser's script hears of the stick from its own watch");
			return;
		}
		SKSE::AllocTrampoline(14);
		ZoomKey::Install(kZoomScript, nullptr, [] { return true; });
	}

	void Listen()
	{
		static InputWatch inputs;
		RE::BSInputDeviceManager::GetSingleton()->AddEventSink(&inputs);
	}

	void SetTurns(Turns a_what)
	{
		turns = a_what;
		steps = 0.0F;
	}

	// The stick is kept as its last event left it and counts by the time of a frame, so the pace holds whether the pad
	// sends a value every frame or only when it changes
	void Frame()
	{
		const auto now = std::chrono::steady_clock::now();
		const float seconds = lastFrame ? std::min(kLongestFrame, std::chrono::duration<float>(now - *lastFrame).count()) : 0.0F;
		lastFrame = now;
		const float x = Tilt(stick.x);
		const float y = Tilt(stick.y);
		if (seconds <= 0.0F || (x == 0.0F && y == 0.0F)) {
			return;
		}
		if (turns == Turns::kItem && Inspect::Active()) {
			// pushed up the stick turns the item the way a drag of the mouse upward does
			Inspect::Turn(x * kItemDegrees * seconds, -y * kItemDegrees * seconds);
		} else if (turns == Turns::kPage) {
			// pushed up the page goes back up
			steps -= y * kPageSteps * seconds;
			const auto whole = std::trunc(steps);
			if (whole != 0.0F) {
				steps -= whole;
				Tell(kScrollScript, RE::GFxValue(static_cast<double>(whole)));
			}
		}
	}

	void Closed()
	{
		stick = {};
		turns = Turns::kNothing;
		holdTold = false;
		steps = 0.0F;
		lastFrame.reset();
	}

	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root)
	{
		Functions::Add<PadTurns>(a_view, a_root, "BrowserPadTurns");
	}
}
