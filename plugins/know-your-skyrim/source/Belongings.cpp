#include "PCH.h"

#include "Belongings.h"

#include "BrowserActors.h"
#include "BrowserIndex.h"
#include "Cheats.h"
#include "Functions.h"
#include "Options.h"

namespace Belongings
{
	namespace
	{
		using Functions::Flag;
		using Functions::Params;

		// the one of a living entry the list shows is the nearest living one, or the nearest dead one when none lives
		RE::Actor* ActorOf(const RE::GFxValue& a_id)
		{
			const auto* entry = a_id.IsNumber() ? BrowserIndex::Find(static_cast<RE::FormID>(a_id.GetNumber())) : nullptr;
			if (!entry || entry->unit == BrowserIndex::kNone) {
				return nullptr;
			}
			const auto actors = BrowserActors::Of(*entry);
			auto* actor = BrowserActors::Nearest(actors, false);
			return actor ? actor : BrowserActors::Nearest(actors, true);
		}

		// a thing with a name, never a list of loot the game hasn't made into things yet
		bool Shown(RE::TESBoundObject& a_object)
		{
			const char* name = a_object.GetName();
			return !a_object.Is(RE::FormType::LeveledItem) && name && *name;
		}

		struct Row
		{
			RE::TESBoundObject* object;
			std::int32_t count;
			bool worn;
		};

		// what is worn first, then by name
		std::vector<Row> RowsOf(RE::TESObjectREFR& a_holder)
		{
			std::vector<Row> rows;
			for (auto& [object, held] : a_holder.GetInventory(Shown)) {
				if (held.first > 0) {
					rows.push_back({ object, held.first, held.second && held.second->IsWorn() });
				}
			}
			std::ranges::sort(rows, [](const Row& a_one, const Row& a_other) {
				return a_one.worn != a_other.worn ? a_one.worn : _stricmp(a_one.object->GetName(), a_other.object->GetName()) < 0;
			});
			return rows;
		}

		std::int32_t CountOf(RE::TESObjectREFR& a_holder, RE::TESBoundObject& a_object)
		{
			const auto counts = a_holder.GetInventoryCounts([&](RE::TESBoundObject& a_one) { return &a_one == &a_object; });
			const auto found = counts.find(&a_object);
			return found != counts.end() ? found->second : 0;
		}

		// The game's own move from one inventory into another, the way a trade puts things in a container. A scripted
		// RemoveItem reached only people with a script of their own, Adril Arano has none and kept his things
		void Move(RE::Actor& a_from, RE::Actor& a_to, RE::TESBoundObject& a_object, std::int32_t a_count)
		{
			a_from.RemoveItem(&a_object, a_count, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, &a_to);
		}

		// Belongings(entryId, player) is {name, dead, rows:[{id, name, count, icon, worn, entry}]} of the one of a living
		// entry the list shows, or of the player when player is true. Entry is the id of the thing's own entry, 0 for a
		// thing the browser has none of. Nothing when none of them is about
		struct List
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie || a_params.argCount < 1) {
					return;
				}
				auto* actor = ActorOf(a_params.args[0]);
				if (!actor) {
					return;
				}
				const bool player = a_params.argCount > 1 && Flag(a_params.args[1]);
				RE::Actor* holder = player ? RE::PlayerCharacter::GetSingleton() : actor;
				auto& movie = *a_params.movie;
				auto& told = *a_params.retVal;
				movie.CreateObject(&told);
				const char* name = actor->GetDisplayFullName();
				told.SetMember("name", name ? name : "");
				told.SetMember("dead", actor->IsDead());
				RE::GFxValue rows;
				movie.CreateArray(&rows);
				for (const auto& row : RowsOf(*holder)) {
					const auto* entry = BrowserIndex::Find(row.object->GetFormID());
					RE::GFxValue one;
					movie.CreateObject(&one);
					one.SetMember("id", static_cast<double>(row.object->GetFormID()));
					one.SetMember("name", entry ? entry->name.c_str() : row.object->GetName());
					one.SetMember("count", static_cast<double>(row.count));
					one.SetMember("icon", entry ? entry->icon : "sack");
					one.SetMember("worn", row.worn);
					one.SetMember("entry", static_cast<double>(entry ? entry->form->GetFormID() : 0));
					rows.PushBack(one);
				}
				told.SetMember("rows", rows);
			}
		};

		// BelongingsMove(entryId, thingId, all, give) with the cheats on takes one or all of a thing from the one the
		// list shows, or gives one or all of the player's to them. True when the move is on its way, the game's own
		// thread sends it
		struct MoveThing
		{
			static void Run(Params& a_params)
			{
				bool sent = false;
				if (Options::Cheats() && a_params.argCount == 4 && a_params.args[1].IsNumber()) {
					auto* actor = ActorOf(a_params.args[0]);
					auto* object = RE::TESForm::LookupByID<RE::TESBoundObject>(static_cast<RE::FormID>(a_params.args[1].GetNumber()));
					auto* player = RE::PlayerCharacter::GetSingleton();
					const bool give = Flag(a_params.args[3]);
					RE::Actor* from = give ? player : actor;
					const auto rows = from ? RowsOf(*from) : std::vector<Row>{};
					const auto row = std::ranges::find(rows, object, &Row::object);
					if (actor && object && row != rows.end()) {
						const auto count = Flag(a_params.args[2]) ? row->count : 1;
						const auto giver = from->CreateRefHandle();
						const auto taker = (give ? static_cast<RE::Actor*>(actor) : player)->CreateRefHandle();
						const auto id = object->GetFormID();
						SKSE::GetTaskInterface()->AddTask([giver, taker, id, count] {
							const auto one = giver.get();
							const auto other = taker.get();
							auto* thing = RE::TESForm::LookupByID<RE::TESBoundObject>(id);
							if (one && other && thing && one->As<RE::Actor>() && other->As<RE::Actor>()) {
								const auto had = CountOf(*one, *thing);
								Move(*one->As<RE::Actor>(), *other->As<RE::Actor>(), *thing, count);
								logger::info("cheat: {} of {:08X} from {:08X} to {:08X}, the giver had {} and has {}", count, id, one->GetFormID(), other->GetFormID(), had,
									CountOf(*one, *thing));
								Cheats::Done();
							}
						});
						sent = true;
					}
				}
				if (a_params.retVal) {
					a_params.retVal->SetBoolean(sent);
				}
			}
		};
	}

	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root)
	{
		Functions::Add<List>(a_view, a_root, "Belongings");
		Functions::Add<MoveThing>(a_view, a_root, "BelongingsMove");
	}
}
