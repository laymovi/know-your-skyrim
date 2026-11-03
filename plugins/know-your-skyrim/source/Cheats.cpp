#include "PCH.h"

#include "Cheats.h"

#include "BrowserActors.h"
#include "BrowserIndex.h"
#include "BrowserPerks.h"
#include "BrowserQuestLinks.h"
#include "BrowserSources.h"
#include "Functions.h"
#include "Options.h"
#include "Places.h"

namespace Cheats
{
	namespace
	{
		using Functions::Flag;
		using Functions::Params;

		const BrowserIndex::Item* EntryOf(const Params& a_params)
		{
			if (!Options::Cheats() || a_params.argCount < 1 || !a_params.args[0].IsNumber()) {
				return nullptr;
			}
			return BrowserIndex::Find(static_cast<RE::FormID>(a_params.args[0].GetNumber()));
		}

		using ActorFlag = RE::Actor::BOOL_FLAGS;
		using BaseFlag = RE::ACTOR_BASE_DATA::Flag;
		using AliasFlag = RE::BGSBaseAlias::FLAGS;

		// What an actor had of its own to keep it alive, given back with RESURRECT
		struct Own
		{
			RE::FormID actor = 0;
			bool       essential = false;
			bool       guarded = false;
		};
		std::vector<Own> kept;

		// The flags that keep an actor alive, off for as long as this lives. Essential lets no one kill it, protected
		// only the player. They sit on the actor, on the records it was made from, and on the aliases of quests it
		// fills. Balgruuf and Eorlund are kept by aliases, their records aren't essential, and the game's kill only
		// brings them to their knees then. The records and the aliases are shared with the quests, they get their flags
		// back the moment this ends, the actor's own come back with RESURRECT
		class Unguarded
		{
		public:
			explicit Unguarded(RE::Actor& a_actor)
			{
				auto& flags = a_actor.GetActorRuntimeData().boolFlags;
				own = { a_actor.GetFormID(), flags.all(ActorFlag::kEssential), flags.all(ActorFlag::kProtected) };
				flags.reset(ActorFlag::kEssential, ActorFlag::kProtected);
				auto* leveled = a_actor.extraList.GetByType<RE::ExtraLeveledCreature>();
				RE::TESActorBase* bases[] = { a_actor.GetActorBase(), leveled ? leveled->originalBase : nullptr, leveled ? leveled->templateBase : nullptr };
				for (auto* base : bases) {
					if (base && std::ranges::find(records, base, &Record::first) == records.end()) {
						records.emplace_back(base, base->actorData.actorBaseFlags);
						base->actorData.actorBaseFlags.reset(BaseFlag::kEssential, BaseFlag::kProtected);
					}
				}
				if (auto* extra = a_actor.extraList.GetByType<RE::ExtraAliasInstanceArray>()) {
					RE::BSReadLockGuard locker(extra->lock);
					for (const auto* one : extra->aliases) {
						if (auto* alias = one ? const_cast<RE::BGSBaseAlias*>(one->alias) : nullptr) {
							aliases.emplace_back(alias, alias->flags);
							alias->flags.reset(AliasFlag::kEssential, AliasFlag::kProtected);
						}
					}
				}
			}

			Unguarded(const Unguarded&) = delete;
			Unguarded& operator=(const Unguarded&) = delete;

			~Unguarded()
			{
				for (auto& [base, flags] : records) {
					base->actorData.actorBaseFlags = flags;
				}
				for (auto& [alias, flags] : aliases) {
					alias->flags = flags;
				}
			}

			// whether anything kept it alive
			[[nodiscard]] bool Guarded() const
			{
				const auto held = [](const auto& a_pair, auto a_essential, auto a_protected) { return a_pair.second.any(a_essential, a_protected); };
				return own.essential || own.guarded ||
				       std::ranges::any_of(records, [&](const Record& a_one) { return held(a_one, BaseFlag::kEssential, BaseFlag::kProtected); }) ||
				       std::ranges::any_of(aliases, [&](const Alias& a_one) { return held(a_one, AliasFlag::kEssential, AliasFlag::kProtected); });
			}

			Own own;

		private:
			using Record = std::pair<RE::TESActorBase*, REX::EnumSet<BaseFlag, std::uint32_t>>;
			using Alias = std::pair<RE::BGSBaseAlias*, REX::EnumSet<AliasFlag, std::uint32_t>>;
			std::vector<Record> records;
			std::vector<Alias>  aliases;
		};

		void Tell(RE::GFxMovie& a_movie, RE::GFxValue& a_told, RE::Actor& a_actor)
		{
			a_movie.CreateObject(&a_told);
			const char* name = a_actor.GetDisplayFullName();
			a_told.SetMember("name", name ? name : "");
			a_told.SetMember("place", Places::Of(a_actor).name.c_str());
		}

		// Cheats() tells whether the player switched the cheats on
		struct Enabled
		{
			static void Run(Params& a_params)
			{
				if (a_params.retVal) {
					a_params.retVal->SetBoolean(Options::Cheats());
				}
			}
		};

		// CheatTargets(formId) is {alive, dead}, how many of a person or creature the game holds now living and dead. A
		// child is never one to kill, the game itself lets no child die
		struct Targets
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie) {
					return;
				}
				const auto* entry = EntryOf(a_params);
				std::size_t alive = 0;
				std::size_t dead = 0;
				for (auto* actor : entry ? BrowserActors::Of(*entry) : std::vector<RE::Actor*>{}) {
					if (actor->IsDead()) {
						dead++;
					} else if (!actor->IsChild()) {
						alive++;
					}
				}
				a_params.movie->CreateObject(a_params.retVal);
				a_params.retVal->SetMember("alive", static_cast<double>(alive));
				a_params.retVal->SetMember("dead", static_cast<double>(dead));
			}
		};

		void PutGuardBack(RE::Actor& a_actor)
		{
			const auto at = std::ranges::find(kept, a_actor.GetFormID(), &Own::actor);
			if (at == kept.end()) {
				return;
			}
			auto& flags = a_actor.GetActorRuntimeData().boolFlags;
			flags.set(at->essential, ActorFlag::kEssential);
			flags.set(at->guarded, ActorFlag::kProtected);
			kept.erase(at);
		}

		// CheatKill(formId) kills the nearest living one and tells {name, place} of it, an essential one too because
		// that's what the cheat is for. The game's own thread kills
		struct Kill
		{
			static void Run(Params& a_params)
			{
				const auto* entry = EntryOf(a_params);
				auto actors = entry ? BrowserActors::Of(*entry) : std::vector<RE::Actor*>{};
				std::erase_if(actors, [](RE::Actor* a_actor) { return a_actor->IsChild(); });
				auto* actor = BrowserActors::Nearest(actors, false);
				if (!actor || !a_params.retVal || !a_params.movie) {
					return;
				}
				Tell(*a_params.movie, *a_params.retVal, *actor);
				const auto handle = actor->CreateRefHandle();
				SKSE::GetTaskInterface()->AddTask([handle] {
					const auto reference = handle.get();
					auto* one = reference ? reference->As<RE::Actor>() : nullptr;
					if (!one || one->IsDead() || one->IsChild()) {
						return;
					}
					bool guarded = false;
					{
						Unguarded open(*one);
						guarded = open.Guarded();
						one->KillImpl(nullptr, 0.0F, true, false);
						if (open.own.essential || open.own.guarded) {
							std::erase_if(kept, [&](const Own& a_known) { return a_known.actor == open.own.actor; });
							kept.push_back(open.own);
						}
					}
					logger::info("cheat: {:08X} {}{}", one->GetFormID(), one->IsDead() ? "killed" : "still alive after the kill",
						guarded ? ", it was kept alive for the quests" : "");
					Done();
				});
			}
		};

		// CheatRaise(formId) brings the nearest dead one back with what it carried and tells {name, place} of it
		struct Raise
		{
			static void Run(Params& a_params)
			{
				const auto* entry = EntryOf(a_params);
				auto* actor = entry ? BrowserActors::Nearest(BrowserActors::Of(*entry), true) : nullptr;
				if (!actor || !a_params.retVal || !a_params.movie) {
					return;
				}
				Tell(*a_params.movie, *a_params.retVal, *actor);
				const auto handle = actor->CreateRefHandle();
				SKSE::GetTaskInterface()->AddTask([handle] {
					const auto reference = handle.get();
					// The game's own resurrect, the console's, keeping what they carried. A scripted call reached only
					// people with a script of their own, Adril Arano has none
					if (auto* one = reference ? reference->As<RE::Actor>() : nullptr; one && one->IsDead()) {
						PutGuardBack(*one);
						one->Resurrect(false, true);
						logger::info("cheat: {:08X} {}", one->GetFormID(), one->IsDead() ? "still dead after the resurrect" : "brought back");
						Done();
					}
				});
			}
		};

		// a command run the way the console runs it, on the game's thread, because the game's own quest commands reach
		// every quest, with a script of its own or none
		void RunCommand(std::string a_command)
		{
			SKSE::GetTaskInterface()->AddTask([command = std::move(a_command)] {
				auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
				auto* script = factory ? factory->Create() : nullptr;
				if (!script) {
					return;
				}
				script->SetCommand(command);
				script->CompileAndRun(nullptr);
				delete script;
				logger::info("cheat: {}", command);
				Done();
			});
		}

		// CheatQuest(formId, finish) starts a quest or finishes it. To finish, the stage whose journal entry finishes
		// it is set the way its own scripts set it, so its fragment hands out the reward and starts what comes next. A
		// quest no entry finishes is marked done the way the console's CompleteQuest marks it. A quest that already
		// runs and waits for the player (a notice board's, one a talk begins) starts by its first stage with a journal
		// entry. Tells {stage} of the stage set
		struct Quest
		{
			static void Run(Params& a_params)
			{
				const auto* entry = EntryOf(a_params);
				auto* quest = entry ? entry->form->As<RE::TESQuest>() : nullptr;
				if (!quest || !a_params.retVal || !a_params.movie) {
					return;
				}
				const bool finish = a_params.argCount > 1 && Flag(a_params.args[1]);
				const auto id = quest->formEditorID.empty() ? std::format("{:08X}", quest->GetFormID()) : std::string(quest->formEditorID.c_str());
				a_params.movie->CreateObject(a_params.retVal);
				const auto beginning = !finish && quest->IsRunning() ? BrowserQuestLinks::BeginningStage(*quest) : std::nullopt;
				if (beginning) {
					a_params.retVal->SetMember("stage", static_cast<double>(*beginning));
					RunCommand(std::format("setstage {} {}", id, *beginning));
				} else if (!finish) {
					RunCommand(std::format("startquest {}", id));
				} else if (const auto stage = BrowserQuestLinks::FinishingStage(*quest)) {
					a_params.retVal->SetMember("stage", static_cast<double>(*stage));
					RunCommand(std::format("setstage {} {}", id, *stage));
				} else {
					RunCommand(std::format("completequest {}", id));
				}
			}
		};

		// CheatPerk(formId, add) gives the player the first rank of a perk they lack, or takes away the highest one they
		// have, the way the console's AddPerk and RemovePerk do, no perk point spent or given back. Tells {rank, ranks} of
		// the rank given or taken, nothing when there is none to give or take
		struct Perk
		{
			static void Run(Params& a_params)
			{
				const auto* entry = EntryOf(a_params);
				const auto* perk = entry && entry->family == BrowserIndex::Family::kPerks ? BrowserPerks::Of(*entry->form) : nullptr;
				if (!perk || !a_params.retVal || !a_params.movie) {
					return;
				}
				const bool add = a_params.argCount > 1 && Flag(a_params.args[1]);
				const auto held = BrowserPerks::Held(*perk);
				std::optional<std::size_t> at;
				for (std::size_t i = 0; i < held.size(); ++i) {
					if (add ? !held[i] && !at : held[i]) {
						at = i;
					}
				}
				if (!at) {
					return;
				}
				a_params.movie->CreateObject(a_params.retVal);
				a_params.retVal->SetMember("rank", static_cast<double>(*at + 1));
				a_params.retVal->SetMember("ranks", static_cast<double>(perk->ranks.size()));
				// The player's own AddPerk gives a rank at once, the console's took seconds while the menu was
				// open. The player's own RemovePerk left the rank on the player, the console's takes it away a few
				// frames later and the page asks until it went
				const auto id = perk->ranks[*at]->GetFormID();
				if (!add) {
					RunCommand(std::format("player.removeperk {:08X}", id));
					return;
				}
				SKSE::GetTaskInterface()->AddTask([id] {
					auto* player = RE::PlayerCharacter::GetSingleton();
					if (auto* rank = RE::TESForm::LookupByID<RE::BGSPerk>(id); player && rank) {
						player->AddPerk(rank, 0);
						logger::info("cheat: perk {:08X} added, the player {} it now", id, player->HasPerk(rank) ? "has" : "lacks");
					}
					Done();
				});
			}
		};

		constexpr std::int32_t kMostTaken = 9999;

		// CheatTake(formId, count) puts that many of the item into the player's inventory, one when the count is left
		// out. False for an entry that is no item to carry
		struct Take
		{
			static void Run(Params& a_params)
			{
				const auto* entry = EntryOf(a_params);
				const bool item = entry && entry->unit == BrowserIndex::kNone && entry->family != BrowserIndex::Family::kQuests &&
				                  entry->family != BrowserIndex::Family::kPseudo && entry->form->As<RE::TESBoundObject>();
				const double asked = a_params.argCount > 1 && a_params.args[1].IsNumber() ? a_params.args[1].GetNumber() : 1.0;
				const auto count = std::clamp(static_cast<std::int32_t>(asked), 1, kMostTaken);
				if (item) {
					const auto id = entry->form->GetFormID();
					SKSE::GetTaskInterface()->AddTask([id, count] {
						auto* player = RE::PlayerCharacter::GetSingleton();
						if (auto* thing = RE::TESForm::LookupByID<RE::TESBoundObject>(id)) {
							player->AddObjectToContainer(thing, nullptr, count, nullptr);
							logger::info("cheat: {} of {:08X} taken", count, id);
							Done();
						}
					});
				}
				if (a_params.retVal) {
					a_params.retVal->SetBoolean(item);
				}
			}
		};
	}

	void Done()
	{
		SKSE::GetTaskInterface()->AddTask([] {
			RE::SendUIMessage::SendInventoryUpdateMessage(RE::PlayerCharacter::GetSingleton(), nullptr);
		});
		SKSE::GetTaskInterface()->AddUITask([] {
			const auto menu = RE::UI::GetSingleton()->GetMenu(RE::InventoryMenu::MENU_NAME);
			if (menu && menu->uiMovie) {
				menu->uiMovie->Invoke("_global.KnowYourSkyrim.deedDone", nullptr, nullptr, 0);
			}
		});
	}

	void AddConsoleWord()
	{
		// developer commands of the game that do nothing in a released game, another mod may have renamed one already
		constexpr std::array kFree{ "ToggleHeapTracking", "TestLocalMap", "BetaComment" };
		RE::SCRIPT_FUNCTION* command = nullptr;
		for (const auto name : kFree) {
			if ((command = RE::SCRIPT_FUNCTION::LocateConsoleCommand(name))) {
				logger::info("HESOYAM takes the place of the console's {}", name);
				break;
			}
		}
		if (!command) {
			logger::warn("no free line in the console's commands, HESOYAM isn't there, the mod's menu still switches the cheats");
			return;
		}
		command->functionName = "HESOYAM";
		command->shortName = "";
		command->helpString = "Know your Skyrim: switches the cheat buttons of the browser on or off";
		command->referenceFunction = false;
		command->numParams = 0;
		command->params = nullptr;
		command->conditionFunction = nullptr;
		command->executeFunction = [](const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&) {
			const bool on = !Options::Cheats();
			Options::Set("bEnabled:Cheats", on ? 1.0F : 0.0F);
			if (const auto log = RE::ConsoleLog::GetSingleton()) {
				log->Print(on ? "Know your Skyrim: cheats are on" : "Know your Skyrim: cheats are off");
			}
			return true;
		};
	}

	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root)
	{
		Functions::Add<Enabled>(a_view, a_root, "Cheats");
		Functions::Add<Targets>(a_view, a_root, "CheatTargets");
		Functions::Add<Kill>(a_view, a_root, "CheatKill");
		Functions::Add<Raise>(a_view, a_root, "CheatRaise");
		Functions::Add<Take>(a_view, a_root, "CheatTake");
		Functions::Add<Quest>(a_view, a_root, "CheatQuest");
		Functions::Add<Perk>(a_view, a_root, "CheatPerk");
	}
}
