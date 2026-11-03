#include "PCH.h"

#include "BrowserPerks.h"

#include <unordered_set>

#include "BrowserIndex.h"
#include "BrowserSources.h"
#include "Holders.h"
#include "ItemTexts.h"
#include "Profile.h"
#include "Trail.h"

namespace BrowserPerks
{
	namespace
	{
		constexpr std::array<std::string_view, kTrees> kTreeKeys{ "oneHandedPerks", "twoHandedPerks", "archeryPerks", "blockPerks", "smithingPerks", "heavyArmorPerks",
			"lightArmorPerks", "pickpocketPerks", "lockpickingPerks", "sneakPerks", "alchemyPerks", "speechPerks", "alterationPerks", "conjurationPerks", "destructionPerks",
			"illusionPerks", "restorationPerks", "enchantingPerks", "vampirePerks", "werewolfPerks", "otherPerks" };

		// broken plugins can loop a tree or a chain of ranks, a walk stops here
		constexpr std::size_t kMostNodes = 20000;
		constexpr std::size_t kMostRanks = 100;
		constexpr std::size_t kNamesShown = 12;
		constexpr std::size_t kNeighboursShown = 8;
		// a list the player opened on the page names this many at most
		constexpr std::size_t kWholeList = 200;
		// a template of a template of a template is as far as the game's own records go
		constexpr int kTemplateDepth = 4;

		struct Trees
		{
			std::vector<Perk> perks;
			std::unordered_map<const RE::TESForm*, std::uint32_t> byRank;
			std::array<std::string, kTrees> names;
		};

		Trees* trees = nullptr;

		// what asks for the perks of the trees, made on the first page of a perk: the recipes, the perks and the
		// living entries, each by the place of the perk in All()
		struct Askers
		{
			std::unordered_map<std::uint32_t, std::vector<const RE::BGSConstructibleObject*>> recipes;
			std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> perks;
			std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> people;
		};

		Askers* askers = nullptr;

		std::uint8_t TreeOf(RE::ActorValue a_value)
		{
			const auto value = std::to_underlying(a_value);
			if (value >= std::to_underlying(RE::ActorValue::kOneHanded) && value <= std::to_underlying(RE::ActorValue::kEnchanting)) {
				return static_cast<std::uint8_t>(value - std::to_underlying(RE::ActorValue::kOneHanded));
			}
			if (a_value == RE::ActorValue::kVampirePerks) {
				return kVampireTree;
			}
			return a_value == RE::ActorValue::kWerewolfPerks ? kWerewolfTree : kOtherTree;
		}

		struct Node
		{
			const RE::BGSSkillPerkTreeNode* node;
			std::uint8_t tree;
		};

		// The actor values the game knows by number, the skills and the trees of the vampire and the werewolf among
		// them, then any record of an actor value a plugin added with a tree of its own. Every node of each, from the
		// root down its children, the ones nearer the root first
		std::vector<Node> ReadNodes(std::array<std::string, kTrees>& a_names)
		{
			std::vector<std::pair<const RE::ActorValueInfo*, std::uint8_t>> roots;
			if (const auto* list = RE::ActorValueList::GetSingleton()) {
				for (std::size_t i = 0; i < std::to_underlying(RE::ActorValue::kTotal); ++i) {
					if (const auto* info = list->actorValues[i]; info && info->perkTree) {
						roots.emplace_back(info, TreeOf(static_cast<RE::ActorValue>(i)));
					}
				}
			}
			for (const auto* info : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::ActorValueInfo>()) {
				if (info && info->perkTree && std::ranges::none_of(roots, [&](const auto& a_had) { return a_had.first == info; })) {
					roots.emplace_back(info, kOtherTree);
				}
			}
			std::vector<Node> nodes;
			std::unordered_set<const RE::BGSSkillPerkTreeNode*> seen;
			for (const auto& [info, tree] : roots) {
				const char* name = info->GetFullName();
				if (tree != kOtherTree && a_names[tree].empty() && name && *name && !BrowserIndex::Unshown(*info, name)) {
					a_names[tree] = name;
				}
				std::vector<const RE::BGSSkillPerkTreeNode*> waiting{ info->perkTree };
				for (std::size_t at = 0; at < waiting.size() && nodes.size() < kMostNodes; ++at) {
					const auto* node = waiting[at];
					if (!node || !seen.insert(node).second) {
						continue;
					}
					nodes.push_back({ node, tree });
					for (const auto* child : node->children) {
						waiting.push_back(child);
					}
				}
			}
			return nodes;
		}

		// the ranks from a first one on, a rank already of another perk ends the chain
		std::vector<RE::BGSPerk*> ChainOf(RE::BGSPerk* a_first, const Trees& a_trees)
		{
			std::vector<RE::BGSPerk*> ranks;
			for (auto* rank = a_first; rank && ranks.size() < kMostRanks; rank = rank->nextPerk) {
				if (std::ranges::find(ranks, rank) != ranks.end() || a_trees.byRank.contains(rank)) {
					break;
				}
				ranks.push_back(rank);
			}
			return ranks;
		}

		void Add(Trees& a_trees, RE::BGSPerk* a_first, std::uint8_t a_tree)
		{
			auto ranks = ChainOf(a_first, a_trees);
			if (ranks.empty()) {
				return;
			}
			const auto place = static_cast<std::uint32_t>(a_trees.perks.size());
			for (const auto* rank : ranks) {
				a_trees.byRank.emplace(rank, place);
			}
			a_trees.perks.push_back({ std::move(ranks), a_tree, {}, {} });
		}

		void Link(Trees& a_trees, const RE::BGSSkillPerkTreeNode* a_from, const RE::BGSSkillPerkTreeNode* a_to)
		{
			const auto from = a_from && a_from->perk ? a_trees.byRank.find(a_from->perk) : a_trees.byRank.end();
			const auto to = a_to && a_to->perk ? a_trees.byRank.find(a_to->perk) : a_trees.byRank.end();
			if (from == a_trees.byRank.end() || to == a_trees.byRank.end() || from->second == to->second) {
				return;
			}
			auto& children = a_trees.perks[from->second].children;
			auto& parents = a_trees.perks[to->second].parents;
			if (std::ranges::find(children, to->second) == children.end()) {
				children.push_back(to->second);
			}
			if (std::ranges::find(parents, from->second) == parents.end()) {
				parents.push_back(from->second);
			}
		}

		// A perk another perk of the trees leads to by nextPerk is a later rank of that one, the rest of the nodes'
		// perks begin chains. A chain that loops back leaves its nodes uncounted, they get perks of their own
		// after. The tree's lines run from a node to its children, the parents the game keeps are read too
		Trees& Read()
		{
			if (trees) {
				return *trees;
			}
			static const auto work = Profile::Work("browser perks");
			const Profile::Timer timer{ work };
			const auto start = std::chrono::steady_clock::now();
			Trail::Step("perks, the trees of the actor values");
			trees = new Trees();
			const auto nodes = ReadNodes(trees->names);
			std::unordered_set<const RE::BGSPerk*> later;
			for (const auto& one : nodes) {
				std::unordered_set<const RE::BGSPerk*> chain{ one.node->perk };
				for (auto* rank = one.node->perk ? one.node->perk->nextPerk : nullptr; rank && chain.insert(rank).second && chain.size() < kMostRanks; rank = rank->nextPerk) {
					later.insert(rank);
				}
			}
			for (const auto& one : nodes) {
				if (one.node->perk && !later.contains(one.node->perk) && !trees->byRank.contains(one.node->perk)) {
					Add(*trees, one.node->perk, one.tree);
				}
			}
			for (const auto& one : nodes) {
				if (one.node->perk && !trees->byRank.contains(one.node->perk)) {
					Add(*trees, one.node->perk, one.tree);
				}
			}
			for (const auto& one : nodes) {
				for (const auto* child : one.node->children) {
					Link(*trees, one.node, child);
				}
				for (const auto* parent : one.node->parents) {
					Link(*trees, parent, one.node);
				}
			}
			const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			logger::info("perks: {} perks of {} ranks in {} nodes of the trees, read in {:.1f} ms", trees->perks.size(), trees->byRank.size(), nodes.size(), took);
			return *trees;
		}

		std::uint32_t PlaceOf(const Perk& a_perk)
		{
			return static_cast<std::uint32_t>(&a_perk - Read().perks.data());
		}

		// a condition that asks the player to have a perk, the perk it names
		const RE::TESForm* AskedPerk(const RE::TESConditionItem& a_condition)
		{
			using Function = RE::FUNCTION_DATA::FunctionID;
			using Compare = RE::CONDITION_ITEM_DATA::OpCode;
			const auto& data = a_condition.data;
			const auto compare = data.flags.opCode;
			const auto value = data.comparisonValue.f;
			const bool yes = (compare == Compare::kEqualTo && value == 1.0F) || (compare == Compare::kNotEqualTo && value == 0.0F);
			if (data.flags.global || !yes || data.functionData.function.get() != Function::kHasPerk) {
				return nullptr;
			}
			return static_cast<const RE::TESForm*>(data.functionData.params[0]);
		}

		template <class F>
		void ForEachAsked(const RE::TESCondition& a_conditions, F&& a_take)
		{
			for (const auto* condition = a_conditions.head; condition; condition = condition->next) {
				if (const auto* form = AskedPerk(*condition); form && Of(*form)) {
					a_take(PlaceOf(*Of(*form)));
				}
			}
		}

		template <class T>
		void AddOnce(std::vector<T>& a_list, T a_one)
		{
			if (std::ranges::find(a_list, a_one) == a_list.end()) {
				a_list.push_back(a_one);
			}
		}

		// the record a person takes its perks from, a template's when the record takes its spell list from one
		const RE::TESNPC& PerksHolder(const RE::TESNPC& a_person)
		{
			using Use = RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG;
			const auto* person = &a_person;
			for (int depth = 0; depth < kTemplateDepth && person->baseTemplateForm && person->actorData.templateUseFlags.any(Use::kSpells); ++depth) {
				const auto* next = person->baseTemplateForm->As<RE::TESNPC>();
				if (!next) {
					break;
				}
				person = next;
			}
			return *person;
		}

		Askers& ReadAskers()
		{
			if (askers) {
				return *askers;
			}
			static const auto work = Profile::Work("browser perk askers");
			const Profile::Timer timer{ work };
			const auto start = std::chrono::steady_clock::now();
			Trail::Step("perks, what asks for them");
			askers = new Askers();
			for (const auto* recipe : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSConstructibleObject>()) {
				Trail::Reading("perks, recipes", recipe);
				if (recipe && Holders::Named(recipe->createdItem)) {
					ForEachAsked(recipe->conditions, [&](std::uint32_t a_perk) { AddOnce(askers->recipes[a_perk], recipe); });
				}
			}
			const auto& all = Read().perks;
			for (std::uint32_t asking = 0; asking < all.size(); ++asking) {
				for (const auto* rank : all[asking].ranks) {
					ForEachAsked(rank->perkConditions, [&](std::uint32_t a_perk) {
						if (a_perk != asking) {
							AddOnce(askers->perks[a_perk], asking);
						}
					});
				}
			}
			const auto& items = BrowserIndex::Items();
			for (std::uint32_t entry = 0; entry < items.size(); ++entry) {
				if (items[entry].unit == BrowserIndex::kNone) {
					continue;
				}
				for (const auto* person : BrowserIndex::Units()[items[entry].unit]) {
					const auto& holder = PerksHolder(*person);
					for (std::uint32_t i = 0; holder.perks && i < holder.perkCount; ++i) {
						if (const auto* perk = holder.perks[i].perk; perk && Of(*perk)) {
							AddOnce(askers->people[PlaceOf(*Of(*perk))], entry);
						}
					}
				}
			}
			// the people a player looks for first, an entry whose records are all unique, then the records of everyone
			// (bandit chiefs, the afflicted), each by name as the index has them
			const auto unique = [&](std::uint32_t a_entry) {
				return std::ranges::all_of(BrowserIndex::Units()[items[a_entry].unit], [](const RE::TESNPC* a_person) { return a_person->IsUnique(); });
			};
			for (auto& [perk, people] : askers->people) {
				std::ranges::stable_sort(people, std::greater{}, unique);
			}
			const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			logger::info("perks: {} perks unlock recipes, {} are asked for by other perks, {} are had by people, read in {:.1f} ms", askers->recipes.size(), askers->perks.size(),
				askers->people.size(), took);
			return *askers;
		}

		struct Named
		{
			std::string name;
			RE::FormID id;
		};

		void PutNames(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const char* a_key, const std::vector<Named>& a_names, std::size_t a_shown, bool a_whole)
		{
			const auto sent = a_whole ? kWholeList : a_shown;
			RE::GFxValue list;
			a_movie.CreateArray(&list);
			for (std::size_t i = 0; i < a_names.size() && i < sent; ++i) {
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				one.SetMember("name", a_names[i].name.c_str());
				one.SetMember("id", static_cast<double>(a_names[i].id));
				list.PushBack(one);
			}
			a_object.SetMember(a_key, list);
			a_object.SetMember(std::format("{}More", a_key).c_str(), static_cast<double>(a_names.size() > sent ? a_names.size() - sent : 0));
			a_object.SetMember(std::format("{}Shown", a_key).c_str(), static_cast<double>(a_shown));
		}

		std::vector<Named> PerkNames(const std::vector<std::uint32_t>& a_perks)
		{
			std::vector<Named> names;
			for (const auto place : a_perks) {
				const auto* first = Read().perks[place].ranks.front();
				if (auto name = BrowserSources::NameOf(*first); !name.empty()) {
					names.push_back({ std::move(name), BrowserSources::LinkOf(first) });
				}
			}
			return names;
		}

		// Each rank with its description and what it asks. An earlier rank of its own chain is asked anyway, it says
		// nothing. Tells the perks the words of any rank name
		std::unordered_set<std::uint32_t> PutRanks(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const Perk& a_perk)
		{
			const auto held = Held(a_perk);
			std::unordered_set<std::uint32_t> named;
			RE::GFxValue ranks;
			a_movie.CreateArray(&ranks);
			for (std::size_t i = 0; i < a_perk.ranks.size(); ++i) {
				const auto* rank = a_perk.ranks[i];
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				one.SetMember("rank", static_cast<double>(i + 1));
				one.SetMember("has", held[i]);
				one.SetMember("description", std::string(ItemTexts::Of(rank->GetFormID())).c_str());
				const std::vector<const RE::BGSPerk*> earlier(a_perk.ranks.begin(), a_perk.ranks.begin() + static_cast<std::ptrdiff_t>(i));
				for (const auto* asked : BrowserSources::PutNeeds(a_movie, one, rank->perkConditions, nullptr, earlier)) {
					if (const auto* perk = asked ? Of(*asked) : nullptr) {
						named.insert(PlaceOf(*perk));
					}
				}
				ranks.PushBack(one);
			}
			a_object.SetMember("ranks", ranks);
			return named;
		}

		// the things recipes that ask for the perk make, each once with its bench, the improving apart
		void PutRecipes(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const std::vector<const RE::BGSConstructibleObject*>& a_recipes, bool a_whole)
		{
			std::vector<Named> made;
			std::vector<Named> improved;
			std::unordered_set<std::string> seen;
			for (const auto* recipe : a_recipes) {
				const auto* created = recipe->createdItem;
				const auto bench = BrowserSources::BenchOf(*recipe);
				auto name = bench.empty() ? BrowserSources::NameOf(*created) : std::format("{} ({})", BrowserSources::NameOf(*created), bench);
				const bool improves = Holders::Improves(*recipe);
				if (!BrowserIndex::Unshown(*created, BrowserSources::NameOf(*created)) && seen.insert(std::format("{}|{}", improves, name)).second) {
					(improves ? improved : made).push_back({ std::move(name), BrowserSources::LinkOf(created) });
				}
			}
			PutNames(a_movie, a_object, "recipes", made, kNamesShown, a_whole);
			PutNames(a_movie, a_object, "improving", improved, kNamesShown, a_whole);
		}
	}

	const std::vector<Perk>& All()
	{
		return Read().perks;
	}

	const Perk* Of(const RE::TESForm& a_form)
	{
		const auto& read = Read();
		const auto found = read.byRank.find(&a_form);
		return found != read.byRank.end() ? &read.perks[found->second] : nullptr;
	}

	std::string_view TreeKey(std::uint8_t a_tree)
	{
		return a_tree < kTreeKeys.size() ? kTreeKeys[a_tree] : std::string_view();
	}

	std::optional<std::uint8_t> TreeNamed(std::string_view a_key)
	{
		const auto found = std::ranges::find(kTreeKeys, a_key);
		return found != kTreeKeys.end() ? std::optional<std::uint8_t>(static_cast<std::uint8_t>(found - kTreeKeys.begin())) : std::nullopt;
	}

	std::string_view TreeName(std::uint8_t a_tree)
	{
		return a_tree < kTrees ? std::string_view(Read().names[a_tree]) : std::string_view();
	}

	std::vector<bool> Held(const Perk& a_perk)
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		std::vector<bool> held;
		for (auto* rank : a_perk.ranks) {
			held.push_back(player && player->HasPerk(rank));
		}
		return held;
	}

	std::uint32_t RankOf(const Perk& a_perk)
	{
		const auto held = Held(a_perk);
		std::uint32_t rank = 0;
		for (std::size_t i = 0; i < held.size(); ++i) {
			rank = held[i] ? static_cast<std::uint32_t>(i + 1) : rank;
		}
		return rank;
	}

	void Fill(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const Perk& a_perk, bool a_whole)
	{
		auto& asked = ReadAskers();
		const auto place = PlaceOf(a_perk);
		// a perk right before it that its requirements name already is left to them, their words carry the "or" of
		// a perk with two parents
		const auto named = PutRanks(a_movie, a_object, a_perk);
		std::vector<std::uint32_t> parents;
		std::ranges::copy_if(a_perk.parents, std::back_inserter(parents), [&](std::uint32_t a_parent) { return !named.contains(a_parent); });
		PutNames(a_movie, a_object, "parents", PerkNames(parents), kNeighboursShown, a_whole);
		PutNames(a_movie, a_object, "children", PerkNames(a_perk.children), kNeighboursShown, a_whole);
		const auto recipes = asked.recipes.find(place);
		PutRecipes(a_movie, a_object, recipes != asked.recipes.end() ? recipes->second : std::vector<const RE::BGSConstructibleObject*>{}, a_whole);
		const auto perks = asked.perks.find(place);
		PutNames(a_movie, a_object, "neededBy", PerkNames(perks != asked.perks.end() ? perks->second : std::vector<std::uint32_t>{}), kNamesShown, a_whole);
		std::vector<Named> people;
		if (const auto had = asked.people.find(place); had != asked.people.end()) {
			for (const auto entry : had->second) {
				const auto& item = BrowserIndex::Items()[entry];
				people.push_back({ item.name, item.form->GetFormID() });
			}
		}
		PutNames(a_movie, a_object, "people", people, kNamesShown, a_whole);
	}
}
