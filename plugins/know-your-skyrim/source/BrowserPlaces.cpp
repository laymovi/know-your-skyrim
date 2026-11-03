#include "PCH.h"

#include "BrowserPlaces.h"

#include <unordered_set>

#include "BrowserIndex.h"
#include "BrowserSources.h"
#include "Places.h"

namespace BrowserPlaces
{
	namespace
	{
		constexpr std::size_t kNamesShown = 12;
		constexpr std::size_t kGroupsShown = 6;
		// a list the player opened on the page names this many at most
		constexpr std::size_t kWholeList = 200;
		// locations can loop in broken plugins, a walk up stops here
		constexpr int kDeepest = 16;

		// the living entry an actor was made from, a templated one points to the record it was picked from
		const BrowserIndex::Item* EntryOf(RE::Actor& a_actor)
		{
			auto* base = a_actor.GetActorBase();
			const auto* leveled = a_actor.extraList.GetByType<RE::ExtraLeveledCreature>();
			const RE::TESForm* made[] = { base, leveled ? leveled->originalBase : nullptr, leveled ? leveled->templateBase : nullptr, base ? base->faceNPC : nullptr };
			for (const auto* form : made) {
				const auto* entry = form ? BrowserIndex::Find(form->GetFormID()) : nullptr;
				if (entry && entry->unit != BrowserIndex::kNone) {
					return entry;
				}
			}
			return nullptr;
		}

		// Names each once with how many of it there are, in the order found, a name with an entry links to it
		class Crowd
		{
		public:
			void Add(std::string a_name, RE::FormID a_link)
			{
				if (a_name.empty()) {
					return;
				}
				const auto [at, fresh] = known.try_emplace(std::format("{}|{}", a_link, a_name), names.size());
				if (fresh) {
					names.push_back({ std::move(a_name), a_link, 1 });
				} else {
					names[at->second].count++;
				}
			}

			void Add(RE::Actor& a_actor)
			{
				if (const auto* entry = EntryOf(a_actor)) {
					Add(entry->name, entry->form->GetFormID());
				} else if (const char* name = a_actor.GetDisplayFullName(); name && *name) {
					Add(name, 0);
				}
			}

			[[nodiscard]] bool Empty() const { return names.empty(); }

			[[nodiscard]] std::size_t Total() const
			{
				std::size_t total = 0;
				for (const auto& name : names) {
					total += name.count;
				}
				return total;
			}

			RE::GFxValue Put(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const char* a_name, bool a_whole) const
			{
				const auto sent = a_whole ? kWholeList : kNamesShown;
				RE::GFxValue list;
				a_movie.CreateArray(&list);
				for (std::size_t i = 0; i < names.size() && i < sent; ++i) {
					RE::GFxValue one;
					a_movie.CreateObject(&one);
					one.SetMember("name", names[i].name.c_str());
					one.SetMember("id", static_cast<double>(names[i].link));
					one.SetMember("count", static_cast<double>(names[i].count));
					list.PushBack(one);
				}
				a_object.SetMember(a_name, list);
				a_object.SetMember(std::format("{}More", a_name).c_str(), static_cast<double>(names.size() > sent ? names.size() - sent : 0));
				a_object.SetMember(std::format("{}Shown", a_name).c_str(), static_cast<double>(kNamesShown));
				return list;
			}

		private:
			struct Name
			{
				std::string name;
				RE::FormID link;
				std::size_t count;
			};
			std::vector<Name> names;
			std::unordered_map<std::string, std::size_t> known;
		};

		// The place right inside a_place a location lies in, the topmost with an entry on the way up, or a_place
		// itself for a location right there or inside places with no entry. None for a location outside a_place
		const RE::BGSLocation* Within(const RE::BGSLocation* a_location, const RE::BGSLocation& a_place)
		{
			const RE::BGSLocation* named = nullptr;
			for (int depth = 0; a_location && depth < kDeepest; ++depth, a_location = a_location->parentLoc) {
				if (a_location == &a_place) {
					return named ? named : &a_place;
				}
				if (BrowserIndex::Find(a_location->GetFormID())) {
					named = a_location;
				}
			}
			return nullptr;
		}

		// Every actor the game holds that is somewhere: the ones that last the whole game of every living entry, and
		// the ones about the player. Never the player, and neither one switched off nor deleted, the way the person's
		// own page counts them
		std::vector<RE::Actor*> Actors()
		{
			std::vector<RE::Actor*> found;
			std::unordered_set<RE::Actor*> seen;
			const auto take = [&](RE::Actor* a_actor) {
				if (a_actor && !a_actor->IsPlayerRef() && !a_actor->IsDisabled() && !a_actor->IsDeleted() && seen.insert(a_actor).second) {
					found.push_back(a_actor);
				}
			};
			for (const auto& unit : BrowserIndex::Units()) {
				for (const auto* record : unit) {
					for (const auto id : BrowserSources::LastingOf(*record)) {
						take(RE::TESForm::LookupByID<RE::Actor>(id));
					}
				}
			}
			auto* lists = RE::ProcessLists::GetSingleton();
			for (const auto* handles : { &lists->highActorHandles, &lists->middleHighActorHandles, &lists->middleLowActorHandles, &lists->lowActorHandles }) {
				for (const auto& handle : *handles) {
					take(handle.get().get());
				}
			}
			return found;
		}

		// the names of the groups of one list, the place itself first, then the places with the most in them
		struct Group
		{
			const RE::BGSLocation* place;
			Crowd crowd;
		};

		void PutGroups(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const char* a_name, std::vector<Group>& a_groups, const RE::BGSLocation& a_place, bool a_whole)
		{
			std::ranges::stable_sort(a_groups, [&](const Group& a_one, const Group& a_other) {
				if ((a_one.place == &a_place) != (a_other.place == &a_place)) {
					return a_one.place == &a_place;
				}
				if (a_one.crowd.Total() != a_other.crowd.Total()) {
					return a_one.crowd.Total() > a_other.crowd.Total();
				}
				const auto* one = BrowserIndex::Find(a_one.place->GetFormID());
				const auto* other = BrowserIndex::Find(a_other.place->GetFormID());
				return BrowserIndex::PlaceOf(*one) < BrowserIndex::PlaceOf(*other);
			});
			const auto sent = a_whole ? kWholeList : kGroupsShown;
			RE::GFxValue list;
			a_movie.CreateArray(&list);
			for (std::size_t i = 0; i < a_groups.size() && i < sent; ++i) {
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				if (a_groups[i].place != &a_place) {
					const auto* entry = BrowserIndex::Find(a_groups[i].place->GetFormID());
					one.SetMember("place", entry->name.c_str());
					one.SetMember("id", static_cast<double>(entry->form->GetFormID()));
				}
				a_groups[i].crowd.Put(a_movie, one, "people", a_whole);
				list.PushBack(one);
			}
			a_object.SetMember(a_name, list);
			a_object.SetMember(std::format("{}More", a_name).c_str(), static_cast<double>(a_groups.size() > sent ? a_groups.size() - sent : 0));
			a_object.SetMember(std::format("{}Shown", a_name).c_str(), static_cast<double>(kGroupsShown));
		}

		Crowd& GroupOf(std::vector<Group>& a_groups, const RE::BGSLocation* a_place)
		{
			const auto found = std::ranges::find(a_groups, a_place, &Group::place);
			return found != a_groups.end() ? found->crowd : a_groups.emplace_back(Group{ a_place, {} }).crowd;
		}
	}

	void Fill(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::BGSLocation& a_place, bool a_whole)
	{
		std::vector<Group> alive;
		std::vector<Group> dead;
		for (auto* actor : Actors()) {
			if (const auto* within = Within(Places::LocationOf(*actor), a_place)) {
				GroupOf(actor->IsDead() ? dead : alive, within).Add(*actor);
			}
		}
		PutGroups(a_movie, a_object, "here", alive, a_place, a_whole);
		PutGroups(a_movie, a_object, "deadHere", dead, a_place, a_whole);

		// the people the records put in the place and the places inside it, each once, because a place around lists the
		// people of the places inside it again
		Crowd lives;
		std::unordered_set<RE::FormID> met;
		std::unordered_set<const RE::BGSLocation*> inside{ &a_place };
		std::vector<const RE::BGSLocation*> waiting{ &a_place };
		while (!waiting.empty()) {
			const auto* place = waiting.back();
			waiting.pop_back();
			for (const auto& unique : place->uniqueNPCs) {
				if (auto* actor = RE::TESForm::LookupByID<RE::Actor>(unique.refID); actor && !actor->IsPlayerRef() && met.insert(unique.refID).second) {
					lives.Add(*actor);
				}
			}
			for (const auto row : BrowserIndex::PlacesIn(*place)) {
				const auto* child = BrowserIndex::Items()[row].form->As<RE::BGSLocation>();
				if (child && inside.insert(child).second) {
					waiting.push_back(child);
				}
			}
		}
		lives.Put(a_movie, a_object, "lives", a_whole);

		Crowd places;
		for (const auto row : BrowserIndex::PlacesIn(a_place)) {
			const auto& entry = BrowserIndex::Items()[row];
			places.Add(entry.name, entry.form->GetFormID());
		}
		places.Put(a_movie, a_object, "inside", a_whole);
	}

	bool PlayerIn(const RE::BGSLocation& a_place)
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		return player && Within(Places::LocationOf(*player), a_place) != nullptr;
	}

	bool PlayerRightIn(const RE::BGSLocation& a_place)
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		return player && Within(Places::LocationOf(*player), a_place) == &a_place;
	}

	RE::FormID PlayerPlace()
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		const auto* place = player ? Places::LocationOf(*player) : nullptr;
		for (int depth = 0; place && depth < kDeepest; ++depth, place = place->parentLoc) {
			if (const auto* entry = BrowserIndex::Find(place->GetFormID())) {
				return entry->form->GetFormID();
			}
		}
		return 0;
	}
}
