#include "PCH.h"

#include "BrowserActors.h"

#include "BrowserSources.h"

namespace BrowserActors
{
	namespace
	{
		constexpr RE::FormID kPlayerBase = 0x7;

		// An actor in the world is one of the records when it was made from one. A templated one may be a copy the game
		// made, it points to the record it was picked from and to the one it has its face from
		bool OneOf(RE::Actor& a_actor, const std::vector<const RE::TESNPC*>& a_records)
		{
			auto* base = a_actor.GetActorBase();
			const auto* leveled = a_actor.extraList.GetByType<RE::ExtraLeveledCreature>();
			const RE::TESForm* made[] = { base, leveled ? leveled->originalBase : nullptr, leveled ? leveled->templateBase : nullptr,
				base ? base->faceNPC : nullptr };
			return std::ranges::any_of(made, [&](const RE::TESForm* a_form) {
				const auto* npc = a_form ? a_form->As<RE::TESNPC>() : nullptr;
				return npc && std::ranges::find(a_records, npc) != a_records.end();
			});
		}
	}

	// Every actor of a living entry the game holds now: the ones that last the whole game, and the ones about the
	// player. Only of an entry whose records are all unique, a person or creature there's one of. Of guards, bandits
	// or wolves "the nearest" would be anyone, the player wouldn't even see which, and one quest skeever among all
	// the skeevers doesn't make them one. The player is never one of them, and neither is one switched off or
	// deleted, a quest may bring a switched off one in later and expect it alive
	std::vector<RE::Actor*> Of(const BrowserIndex::Item& a_entry)
	{
		std::vector<RE::Actor*> found;
		if (a_entry.unit == BrowserIndex::kNone) {
			return found;
		}
		const auto& unit = BrowserIndex::Units()[a_entry.unit];
		if (unit.empty() || !std::ranges::all_of(unit, [](const RE::TESNPC* a_record) { return a_record->IsUnique(); })) {
			return found;
		}
		std::vector<const RE::TESNPC*> records;
		for (const auto* record : unit) {
			if (record->GetFormID() != kPlayerBase) {
				records.push_back(record);
			}
		}
		const auto take = [&](RE::Actor* a_actor) {
			if (a_actor && !a_actor->IsPlayerRef() && !a_actor->IsDisabled() && !a_actor->IsDeleted() &&
				std::ranges::find(found, a_actor) == found.end()) {
				found.push_back(a_actor);
			}
		};
		for (const auto* record : records) {
			for (const auto id : BrowserSources::LastingOf(*record)) {
				take(RE::TESForm::LookupByID<RE::Actor>(id));
			}
		}
		auto* lists = RE::ProcessLists::GetSingleton();
		for (const auto* handles : { &lists->highActorHandles, &lists->middleHighActorHandles, &lists->middleLowActorHandles, &lists->lowActorHandles }) {
			for (const auto& handle : *handles) {
				const auto actor = handle.get();
				if (actor && OneOf(*actor, records)) {
					take(actor.get());
				}
			}
		}
		return found;
	}

	// the nearest of them in the player's world or cell, one somewhere else after those
	RE::Actor* Nearest(const std::vector<RE::Actor*>& a_actors, bool a_dead)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		const auto away = [&](RE::Actor* a_actor) {
			const auto* world = a_actor->GetWorldspace();
			const bool here = world ? world == player->GetWorldspace() : a_actor->GetParentCell() == player->GetParentCell();
			return here ? a_actor->GetPosition().GetDistance(player->GetPosition()) : std::numeric_limits<float>::max();
		};
		RE::Actor* nearest = nullptr;
		for (auto* actor : a_actors) {
			if (actor->IsDead() == a_dead && (!nearest || away(actor) < away(nearest))) {
				nearest = actor;
			}
		}
		return nearest;
	}
}
