#include "PCH.h"

#include "Blockers.h"

#include "BrowserSources.h"
#include "Knowledge.h"
#include "Places.h"

namespace Blockers
{
	namespace
	{
		constexpr float kUnitsPerMeter = 70.0f;
		// a far wing of the same dungeon says nothing to a player who is stuck here
		constexpr float kNear = 10000.0f;
		constexpr std::size_t kShown = 3;
		constexpr std::size_t kMost = 24;
		// a lever can reach a gate through helpers with no name, a broken plugin can loop
		constexpr int kDeepest = 4;

		struct Spot
		{
			float meters = 0.0f;
			int side = 0;
			float up = 0.0f;
		};

		struct Opener
		{
			RE::FormID id = 0;
			std::string name;
			Spot spot;
			// switched off for now, a quest or an event brings it
			bool off = false;
			// in another cell or world, how far means nothing
			bool away = false;
		};

		struct Shut
		{
			std::string thing;
			bool raised = false;
			Spot spot;
			std::vector<Opener> openers;
			std::string key;
			RE::TESKey* keyForm = nullptr;
			bool carried = false;
		};

		struct Read
		{
			std::vector<Shut> shut;
			// shut with no opener in their records, a script or a quest may move them or nothing does
			std::size_t noParent = 0;
			// open tells nothing about the way for these, an arch of Apocrypha that plays "trigger01"
			std::size_t unclear = 0;
		};

		std::optional<Read> known;

		// what "open" of the scripts means for the way. A gate is out of the way when open, a retractable bridge plays
		// "raise" to open and lies across the gap only while closed. Any other animation name says nothing
		enum class Meaning
		{
			kGate,
			kBridge,
			kUnclear
		};

		struct Script
		{
			bool hasState = false;
			bool open = false;
			bool trap = false;
			Meaning meaning = Meaning::kGate;
		};

		// Spots are told from where the player looks, the menu stands the world still, so the words hold while it's
		// open. The game's heading starts at +Y and grows clockwise
		Spot SpotOf(const RE::NiPoint3& a_at)
		{
			const auto* player = RE::PlayerCharacter::GetSingleton();
			const auto from = player->GetPosition();
			const float eastward = a_at.x - from.x;
			const float northward = a_at.y - from.y;
			constexpr float kTurn = std::numbers::pi_v<float> * 2.0f;
			float bearing = std::atan2(eastward, northward) - player->GetAngleZ();
			bearing = std::fmod(std::fmod(bearing, kTurn) + kTurn, kTurn);
			Spot spot;
			spot.meters = std::sqrt(eastward * eastward + northward * northward) / kUnitsPerMeter;
			spot.up = (a_at.z - from.z) / kUnitsPerMeter;
			spot.side = static_cast<int>(std::lround(bearing / (kTurn / 8.0f))) % 8;
			return spot;
		}

		// The state the scripts hold, the class of anything that keeps a bool named isOpen, as default2StateActivator's
		// auto property or as NorPortcullisSCRIPT's own variable. A script named trap anywhere up its parents is a trap,
		// it moves when its lever is pulled and stands in nobody's way. Only values get copied under the lock of the
		// scripts, the same rule as BrowserQuests
		Script ScriptOf(const RE::TESObjectREFR& a_ref)
		{
			static const RE::BSFixedString kAutoOpen{ "::isOpen_var" };
			static const RE::BSFixedString kOwnOpen{ "isOpen" };
			static const RE::BSFixedString kOpenAnim{ "::openAnim_var" };
			Script found;
			auto* machine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = machine ? machine->GetObjectHandlePolicy1() : nullptr;
			if (!policy) {
				return found;
			}
			const auto handle = policy->GetHandleForObject(static_cast<RE::VMTypeID>(RE::FormType::Reference), &a_ref);
			const RE::BSSpinLockGuard locker{ machine->attachedScriptsLock };
			const auto scripts = machine->attachedScripts.find(handle);
			if (scripts == machine->attachedScripts.end()) {
				return found;
			}
			for (std::uint32_t s = 0; s < scripts->second.size(); ++s) {
				const auto* script = scripts->second[s].get();
				if (!script || !script->constructed || !script->initialized || !script->valid) {
					continue;
				}
				for (const auto* type = script->GetTypeInfo(); type; type = type->GetParent()) {
					if (const char* name = type->GetName(); name && _strnicmp(name, "trap", 4) == 0) {
						found.trap = true;
					}
				}
				const auto* open = script->GetVariable(kAutoOpen);
				open = open ? open : script->GetVariable(kOwnOpen);
				if (open && open->IsBool()) {
					found.hasState = true;
					found.open = open->GetBool();
					if (const auto* animation = script->GetVariable(kOpenAnim); animation && animation->IsString()) {
						const std::string played{ animation->GetString() };
						found.meaning = _stricmp(played.c_str(), "open") == 0 ? Meaning::kGate : (_stricmp(played.c_str(), "raise") == 0 ? Meaning::kBridge : Meaning::kUnclear);
					}
				}
			}
			return found;
		}

		// the list of parents walks only through a pointer that isn't const
		RE::ExtraActivateRef* ParentsOf(const RE::TESObjectREFR& a_ref)
		{
			return const_cast<RE::ExtraActivateRef*>(a_ref.extraList.GetByType<RE::ExtraActivateRef>());
		}

		bool InPlayersSpace(const RE::TESObjectREFR& a_ref)
		{
			const auto* player = RE::PlayerCharacter::GetSingleton();
			const auto* world = player->GetWorldspace();
			return a_ref.GetParentCell() == player->GetParentCell() || (world && a_ref.GetWorldspace() == world);
		}

		// What opens a reference is the first ones with a name going up its parents. An activator with no name and no
		// parents of its own is a trigger, told with no name. A marker is only something a script activates
		void OpenersOf(const RE::TESObjectREFR& a_ref, std::vector<Opener>& a_openers, int a_depth = 0)
		{
			auto* parents = ParentsOf(a_ref);
			if (!parents || a_depth >= kDeepest) {
				return;
			}
			for (auto* parent : parents->parents) {
				const auto reference = parent ? parent->activateRef.get() : RE::NiPointer<RE::TESObjectREFR>{};
				const auto* base = reference ? reference->GetBaseObject() : nullptr;
				if (!base) {
					continue;
				}
				auto name = BrowserSources::NameOf(*base);
				if (name.empty() && ParentsOf(*reference)) {
					OpenersOf(*reference, a_openers, a_depth + 1);
					continue;
				}
				if (name.empty() && !base->Is(RE::FormType::Activator)) {
					continue;
				}
				a_openers.push_back({ reference->GetFormID(), std::move(name), SpotOf(reference->GetPosition()), reference->IsDisabled(), !InPlayersSpace(*reference) });
			}
		}

		// A door of somebody's home or shop stands in nobody's way, the owner's key opens it for the owner. A dungeon's
		// door has no owner
		bool Owned(const RE::TESObjectREFR& a_ref)
		{
			auto* cell = a_ref.GetParentCell();
			return a_ref.GetOwner() || (cell && cell->GetOwner());
		}

		// The player's cell, outdoors also the eight around it, a gate 20 m away can stand over a cell border
		std::vector<RE::TESObjectCELL*> CellsAround(RE::TESObjectCELL& a_cell)
		{
			std::vector<RE::TESObjectCELL*> cells{ &a_cell };
			const auto* grid = RE::TES::GetSingleton()->gridCells;
			const auto* middle = a_cell.IsExteriorCell() ? a_cell.GetCoordinates() : nullptr;
			if (!grid || !middle) {
				return cells;
			}
			for (std::uint32_t x = 0; x < grid->length; ++x) {
				for (std::uint32_t y = 0; y < grid->length; ++y) {
					auto* cell = grid->GetCell(x, y);
					const auto* at = cell && cell != &a_cell && cell->IsAttached() ? cell->GetCoordinates() : nullptr;
					if (at && std::abs(at->cellX - middle->cellX) <= 1 && std::abs(at->cellY - middle->cellY) <= 1) {
						cells.push_back(cell);
					}
				}
			}
			return cells;
		}

		// Same openers, same gate to the player, so poles of one portcullis stand as one line
		void Add(Read& a_read, Shut a_shut)
		{
			std::ranges::sort(a_shut.openers, {}, &Opener::id);
			if (a_shut.key.empty()) {
				for (auto& other : a_read.shut) {
					if (other.key.empty() && std::ranges::equal(other.openers, a_shut.openers, {}, &Opener::id, &Opener::id)) {
						if (a_shut.spot.meters < other.spot.meters) {
							other.spot = a_shut.spot;
						}
						return;
					}
				}
			}
			a_read.shut.push_back(std::move(a_shut));
		}

		Read Scan()
		{
			const auto started = std::chrono::steady_clock::now();
			Read read;
			const auto* player = RE::PlayerCharacter::GetSingleton();
			auto* cell = player ? player->GetParentCell() : nullptr;
			if (!cell) {
				return read;
			}
			std::vector<RE::NiPointer<RE::TESObjectREFR>> refs;
			const auto cells = CellsAround(*cell);
			for (const auto* around : cells) {
				around->ForEachReference([&](RE::TESObjectREFR* a_ref) {
					refs.emplace_back(a_ref);
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}

			std::size_t linked = 0;
			std::size_t locks = 0;
			std::size_t states = 0;
			const auto from = player->GetPosition();
			for (const auto& ref : refs) {
				const auto* base = ref->GetBaseObject();
				if (!base || ref->IsDeleted() || ref->IsDisabled() || (!base->Is(RE::FormType::Activator) && !base->Is(RE::FormType::Door))) {
					continue;
				}
				const bool door = base->Is(RE::FormType::Door);
				const auto* parents = ParentsOf(*ref);
				linked += parents ? 1 : 0;
				const auto close = ref->GetPosition().GetDistance(from) <= kNear;

				if (door) {
					const auto* lock = ref->GetLock();
					if (lock && lock->IsLocked() && lock->key && !Owned(*ref)) {
						locks++;
						if (close) {
							Shut shut;
							shut.thing = BrowserSources::NameOf(*base);
							shut.spot = SpotOf(ref->GetPosition());
							shut.key = BrowserSources::NameOf(*lock->key);
							shut.keyForm = lock->key;
							read.shut.push_back(std::move(shut));
						}
						continue;
					}
				}

				const auto script = ScriptOf(*ref);
				if (script.trap || (!parents && !script.hasState)) {
					continue;
				}
				bool clear = false;
				if (script.hasState) {
					states++;
					if (script.meaning == Meaning::kUnclear) {
						read.unclear++;
						continue;
					}
					clear = script.meaning == Meaning::kBridge ? !script.open : script.open;
				} else if (door && parents) {
					const auto state = RE::BGSOpenCloseForm::GetOpenState(ref.get());
					clear = state == RE::BGSOpenCloseForm::OPEN_STATE::kOpen || state == RE::BGSOpenCloseForm::OPEN_STATE::kOpening;
				} else {
					continue;
				}
				if (clear || !close) {
					continue;
				}
				if (!parents) {
					read.noParent++;
					continue;
				}
				Shut shut;
				shut.thing = BrowserSources::NameOf(*base);
				shut.raised = script.hasState && script.meaning == Meaning::kBridge;
				shut.spot = SpotOf(ref->GetPosition());
				OpenersOf(*ref, shut.openers);
				if (shut.openers.empty()) {
					read.noParent++;
					continue;
				}
				Add(read, std::move(shut));
			}

			if (locks > 0) {
				const auto carried = RE::PlayerCharacter::GetSingleton()->GetInventoryCounts([](RE::TESBoundObject& a_one) { return a_one.Is(RE::FormType::KeyMaster); });
				for (auto& shut : read.shut) {
					const auto held = shut.keyForm ? carried.find(shut.keyForm) : carried.end();
					shut.carried = held != carried.end() && held->second > 0;
				}
			}
			std::ranges::sort(read.shut, {}, [](const Shut& a_shut) { return a_shut.spot.meters; });
			const auto taken = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
			logger::info("shut around the player in {} cells: {} references, {} with activate parents, {} keyed locks, {} with a state of scripts, {} lines, {} with no opener, {} unclear, read in {} us", cells.size(), refs.size(), linked, locks, states,
				read.shut.size(), read.noParent, read.unclear, taken);
			return read;
		}

		void PutSpot(RE::GFxValue& a_one, const Spot& a_spot)
		{
			a_one.SetMember("far", static_cast<double>(a_spot.meters));
			a_one.SetMember("side", static_cast<double>(a_spot.side));
			a_one.SetMember("up", static_cast<double>(a_spot.up));
		}
	}

	void MenuOpened()
	{
		known.reset();
	}

	void Put(RE::GFxMovie& a_movie, RE::GFxValue& a_object)
	{
		if (!known) {
			known = Scan();
		}
		RE::GFxValue list;
		a_movie.CreateArray(&list);
		for (std::size_t i = 0; i < known->shut.size() && i < kMost; ++i) {
			const auto& shut = known->shut[i];
			RE::GFxValue one;
			a_movie.CreateObject(&one);
			one.SetMember("thing", shut.thing.c_str());
			one.SetMember("raised", shut.raised);
			PutSpot(one, shut.spot);
			if (!shut.key.empty()) {
				one.SetMember("key", shut.key.c_str());
				one.SetMember("carried", shut.carried);
			}
			RE::GFxValue openers;
			a_movie.CreateArray(&openers);
			for (const auto& opener : shut.openers) {
				RE::GFxValue told;
				a_movie.CreateObject(&told);
				told.SetMember("name", opener.name.c_str());
				told.SetMember("off", opener.off);
				told.SetMember("away", opener.away);
				PutSpot(told, opener.spot);
				openers.PushBack(told);
			}
			one.SetMember("openers", openers);
			list.PushBack(one);
		}
		a_object.SetMember("blockers", list);
		a_object.SetMember("blockersMore", static_cast<double>(known->shut.size() > kMost ? known->shut.size() - kMost : 0));
		a_object.SetMember("blockersShown", static_cast<double>(kShown));
		a_object.SetMember("blockersNoParent", static_cast<double>(known->noParent));
		// the progression mode keeps what opens it out of the page until the player asks
		a_object.SetMember("blockersHidden", Knowledge::On());
	}

	bool TargetsHere(const RE::TESQuest& a_quest)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		const auto* cell = player ? player->GetParentCell() : nullptr;
		if (!cell) {
			return false;
		}
		// GetTargetRef of the game takes the lock of the aliases, so the aliases are copied and read one lock at a time
		std::vector<std::uint32_t> aliases;
		{
			const RE::BSSpinLockGuard guard(player->GetQuestTargetsLock());
			const auto found = player->GetQuestTargets().find(const_cast<RE::TESQuest*>(&a_quest));
			if (found == player->GetQuestTargets().end() || !found->second) {
				return false;
			}
			for (const auto* target : *found->second) {
				aliases.push_back(target->alias);
			}
		}
		std::vector<RE::ObjectRefHandle> handles;
		{
			const RE::BSReadLockGuard locker{ a_quest.aliasAccessLock };
			for (const auto alias : aliases) {
				if (const auto held = a_quest.refAliasMap.find(alias); held != a_quest.refAliasMap.end()) {
					handles.push_back(held->second);
				}
			}
		}
		// A quest's marker stands on the door out of the cell for every target anywhere in the world, it says
		// nothing. The target itself does, here or in another cell of the same dungeon
		const auto* here = Places::LocationOf(*player);
		return std::ranges::any_of(handles, [&](const RE::ObjectRefHandle& a_handle) {
			const auto reference = a_handle.get();
			return reference && (reference->GetParentCell() == cell || (here && Places::LocationOf(*reference) == here));
		});
	}
}
