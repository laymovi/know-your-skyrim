#include "PCH.h"

#include "Belongings.h"
#include "Browser.h"
#include "BrowserIndex.h"
#include "BrowserPad.h"
#include "BrowserQuestLinks.h"
#include "BrowserQuests.h"
#include "BrowserTextures.h"
#include "Blockers.h"
#include "Cheats.h"
#include "Functions.h"
#include "Inject.h"
#include "Knowledge.h"
#include "Options.h"
#include "PointerFirst.h"
#include "People.h"
#include "Profile.h"
#include "Tile.h"
#include "Trail.h"
#include "Translations.h"

// The browser's movie reaches these as skse.plugins.KnowYourSkyrim.<name>
namespace
{
	// Words() gives the browser the words of the translation file of the game's language, as the file has them
	struct GetWords
	{
		static void Run(Functions::Params& a_params)
		{
			if (a_params.retVal && a_params.movie) {
				Translations::Fill(*a_params.movie, *a_params.retVal);
			}
		}
	};

	// ScreenSize() is the size of the menu's picture in pixels. A menu made on SkyUI 5 has a stage 2560 wide that the
	// game stretches over the screen, the browser squeezes its own drawing back by the difference (the panel does the same)
	struct GetScreenSize
	{
		static void Run(Functions::Params& a_params)
		{
			if (!a_params.retVal || !a_params.movie) {
				return;
			}
			const auto* view = static_cast<RE::GFxMovieView*>(a_params.movie);
			RE::GViewport port;
			view->GetViewport(&port);
			a_params.movie->CreateObject(a_params.retVal);
			a_params.retVal->SetMember("width", RE::GFxValue(port.width));
			a_params.retVal->SetMember("height", RE::GFxValue(port.height));
		}
	};

	bool AddMenuFunctions(RE::GFxMovieView* a_view, RE::GFxValue* a_root)
	{
		if (!a_view || !a_root) {
			return true;
		}
		const auto* definition = a_view->GetMovieDef();
		const char* url = definition ? definition->GetFileURL() : nullptr;
		Trail::Step(std::format("functions for the menu {}", url ? url : "with no file"));
		Functions::Add<GetWords>(*a_view, *a_root, "Words");
		Functions::Add<GetScreenSize>(*a_view, *a_root, "ScreenSize");
		Browser::AddFunctions(*a_view, *a_root);
		Tile::AddFunctions(*a_view, *a_root);
		Cheats::AddFunctions(*a_view, *a_root);
		Belongings::AddFunctions(*a_view, *a_root);
		BrowserPad::AddFunctions(*a_view, *a_root);
		Inject::IntoMenu(*a_view);
		return true;
	}

	// The inventory draws its picture and then its 3D, every frame. The browser follows what the game loaded meanwhile
	struct MenuDisplay
	{
		static void thunk(RE::IMenu* a_menu)
		{
			static const auto work = Profile::Work("menu frame");
			Profile::MenuFrame();
			{
				const Profile::Timer timer{ work };
				Tile::MenuFrame();
				BrowserTextures::Frame();
				BrowserPad::Frame();
			}
			original(a_menu);
		}

		static void Install()
		{
			constexpr std::size_t kPostDisplay = 6;
			REL::Relocation<std::uintptr_t> table{ RE::VTABLE_InventoryMenu[0] };
			original = table.write_vfunc(kPostDisplay, thunk);
		}

		static inline REL::Relocation<decltype(thunk)> original;
	};

	class MenuWatch : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
		{
			if (!a_event || a_event->menuName != RE::InventoryMenu::MENU_NAME) {
				return RE::BSEventNotifyControl::kContinue;
			}
			if (a_event->opening) {
				Trail::Step("the inventory opens");
				Profile::MenuOpened(a_event->menuName.c_str());
				// a switch changed in the ini while the game runs holds from this opening on
				Options::Load();
				Knowledge::MenuOpened();
				Blockers::MenuOpened();
				return RE::BSEventNotifyControl::kContinue;
			}
			Trail::Step("the inventory closes");
			Tile::MenuClosed();
			PointerFirst::MenuClosed();
			BrowserPad::Closed();
			Profile::Report("menu closed");
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	void OnMessage(SKSE::MessagingInterface::Message* a_message)
	{
		Trail::Step(std::format("SKSE message {}", a_message->type));
		switch (a_message->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			{
				static MenuWatch watch;
				Trail::Step("watching the inventory open and close");
				RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(&watch);
				PointerFirst::Install();
				Trail::Step("words of the browser");
				// the menu of the mod is written in English, nothing of the browser needs the game's translator
				Translations::Load("KnowYourSkyrim", "$KYSK_", false);
				Trail::Step("settings");
				Options::Load();
				Trail::Step("the console's word for the cheats");
				Cheats::AddConsoleWord();
				Trail::Step("the gamepad's stick");
				BrowserPad::Listen();
				Trail::Step("what the character meets");
				Knowledge::Listen();
				Trail::Step("ready for the main menu");
				break;
			}
		case SKSE::MessagingInterface::kPostLoadGame:
			Knowledge::Loaded();
			break;
		// what the quests' scripts hold lives in the save, it's read again on the first ask after a load
		case SKSE::MessagingInterface::kPreLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			BrowserQuests::Forget();
			BrowserQuestLinks::Forget();
			break;
		default:
			break;
		}
	}

	// the mod's menu in SkyUI reads and sets one setting by its id, "bEnabled:Cheats"
	float GetSetting(RE::StaticFunctionTag*, RE::BSFixedString a_id)
	{
		Trail::Step(std::format("the menu script asks for {}", a_id.c_str()));
		return Options::Value(a_id.c_str());
	}

	void SetSetting(RE::StaticFunctionTag*, RE::BSFixedString a_id, float a_value)
	{
		Trail::Step(std::format("the menu script sets {} to {}", a_id.c_str(), a_value));
		Options::Set(a_id.c_str(), a_value);
	}

	bool BindMenuScript(RE::BSScript::IVirtualMachine* a_machine)
	{
		Trail::Step("functions of the menu script");
		a_machine->RegisterFunction("GetSetting", "KnowYourSkyrim", GetSetting);
		a_machine->RegisterFunction("SetSetting", "KnowYourSkyrim", SetSetting);
		return true;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	Trail::Step(std::format("the plugin loads into game {}", REL::Module::get().version().string()));
	Trail::Step("hook on the inventory's drawing");
	MenuDisplay::Install();
	Trail::Step("the item zoom key");
	BrowserPad::InstallZoomKey();
	People::SetLookalikes(BrowserIndex::SameName);
	Trail::Step("menu functions, menu script and SKSE messages asked for");
	SKSE::GetScaleformInterface()->Register(AddMenuFunctions, "KnowYourSkyrim");
	SKSE::GetPapyrusInterface()->Register(BindMenuScript);
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	Knowledge::Register();
	Trail::Step("the plugin is loaded");
	return true;
}
