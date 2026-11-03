#include "PCH.h"

#include "BrowserMap.h"

namespace BrowserMap
{
	namespace
	{
		constexpr std::array<std::string_view, 4> kItemMenus{ RE::InventoryMenu::MENU_NAME, RE::ContainerMenu::MENU_NAME, RE::BarterMenu::MENU_NAME, RE::GiftMenu::MENU_NAME };

		// the marker the map opens at once the item menu has closed
		RE::RefHandle pending = 0;

		const RE::TESWorldSpace* Top(const RE::TESWorldSpace* a_world)
		{
			for (int depth = 0; a_world && a_world->parentWorld && depth < 8; ++depth) {
				a_world = a_world->parentWorld;
			}
			return a_world;
		}

		// the world the map shows is the one the player stands in, or the last one outside while the player is inside
		const RE::TESWorldSpace* PlayerWorld()
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (const auto* world = player->GetWorldspace()) {
				return Top(world);
			}
			return Top(player->GetPlayerRuntimeData().cachedWorldSpace);
		}

		// The journal opens the map the same way, with a message holding the reference the map looks at first. When the
		// tween menu is still under the item menu it goes with the camera of the menus, as the journal does it
		void OpenMap()
		{
			auto* data = RE::UIMessageDataFactory::Create<RE::RefHandleUIData>();
			if (!data) {
				return;
			}
			data->data = pending;
			pending = 0;
			auto* ui = RE::UI::GetSingleton();
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::MapMenu::MENU_NAME, ui->IsMenuOpen(RE::MapMenu::MENU_NAME) ? RE::UI_MESSAGE_TYPE::kUpdate : RE::UI_MESSAGE_TYPE::kShow, data);
			if (ui->IsMenuOpen(RE::TweenMenu::MENU_NAME)) {
				if (REL::Module::IsAE()) {
					REL::Relocation<void()> leaveTween{ REL::ID(52713) };
					leaveTween();
				} else {
					RE::TweenMenu::CloseTweenMenu();
				}
			}
		}

		class Closing : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				if (a_event && !a_event->opening && pending != 0 && std::ranges::find(kItemMenus, std::string_view(a_event->menuName.c_str())) != kItemMenus.end()) {
					OpenMap();
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	std::string OtherWorld(RE::FormID a_marker)
	{
		const auto* marker = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_marker);
		const auto* world = marker ? Top(marker->GetWorldspace()) : nullptr;
		if (!world || world == PlayerWorld()) {
			return {};
		}
		const char* name = world->GetName();
		const char* id = world->GetFormEditorID();
		return name && *name ? name : (id && *id ? id : "another world");
	}

	bool Show(RE::FormID a_marker)
	{
		static Closing closing;
		static bool listening = false;
		// A marker of the map stands in the cell of its world that lasts, the cell isn't loaded and the reference has
		// no cell of its own then. The game's own question gives its world, a reference inside has none
		auto* marker = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_marker);
		const auto* world = marker ? Top(marker->GetWorldspace()) : nullptr;
		if (!world) {
			return false;
		}
		if (world != PlayerWorld()) {
			logger::info("the map shows {}, the marker {:08X} lies in {}", PlayerWorld() ? PlayerWorld()->GetName() : "no world", a_marker, world ? world->GetName() : "no world");
			return false;
		}
		auto* ui = RE::UI::GetSingleton();
		if (!listening) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&closing);
			listening = true;
		}
		pending = marker->CreateRefHandle().native_handle();
		for (const auto menu : kItemMenus) {
			if (ui->IsMenuOpen(menu)) {
				RE::UIMessageQueue::GetSingleton()->AddMessage(menu, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			}
		}
		logger::info("the map opens at the marker {:08X} of {}", a_marker, world->GetName());
		return true;
	}
}
