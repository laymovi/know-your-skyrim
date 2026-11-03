#include "PCH.h"

#include "BrowserSources.h"

#include <map>
#include <set>
#include <unordered_set>

#include "Blockers.h"
#include "BrowserActors.h"
#include "BrowserFiles.h"
#include "BrowserHandouts.h"
#include "BrowserIndex.h"
#include "BrowserMap.h"
#include "BrowserPerks.h"
#include "BrowserPlaces.h"
#include "BrowserQuestLinks.h"
#include "BrowserQuests.h"
#include "Holders.h"
#include "ItemTexts.h"
#include "Places.h"
#include "Profile.h"
#include "Trail.h"
#include "Translations.h"

namespace BrowserSources
{
	namespace
	{
		using Holders::Known;
		using Holders::Lasting;
		using Holders::Named;
		using Holders::Parent;
		using Holders::Way;
		using Places::Where;
		using BrowserHandouts::Tag;

		// a loot list of at most this many things is one thing at many levels, a bigger one is common loot
		constexpr std::size_t kSmallList = 8;
		// lists of lists can loop in broken plugins, the walk up stops here
		constexpr std::size_t kMostLists = 4000;
		constexpr std::size_t kNamesShown = 6;
		constexpr std::size_t kPlacesShown = 4;
		constexpr std::size_t kUsesShown = 8;
		// what a person carries can be a list of hundreds of things, the page names this many
		constexpr std::size_t kThingsShown = 12;
		// a list the player opened on the page names this many at most
		constexpr std::size_t kWholeList = 200;
		constexpr std::size_t kMostLeaves = 400;

		// a common thing placed in thousands of spots names its places from this many
		constexpr std::size_t kMostSpots = 3000;
		constexpr RE::FormID kPlayerBase = 0x7;

		// What only the browser asks: the plants things grow on, the references of things and chests that last the
		// whole game, how many things each loot list gives
		struct Spread
		{
			std::unordered_map<const RE::TESForm*, std::vector<const RE::TESForm*>> grows;
			std::unordered_map<const RE::TESForm*, std::vector<RE::FormID>> placed;
			std::unordered_map<const RE::TESForm*, std::size_t> leaves;
		};

		Spread spread;

		void ReadPlants(Spread& a_spread, RE::TESDataHandler& a_data)
		{
			for (const auto* flora : a_data.GetFormArray<RE::TESFlora>()) {
				if (Lasting(flora) && Lasting(flora->produceItem)) {
					a_spread.grows[flora->produceItem].push_back(flora);
				}
			}
			for (const auto* tree : a_data.GetFormArray<RE::TESObjectTREE>()) {
				if (Lasting(tree) && Lasting(tree->produceItem)) {
					a_spread.grows[tree->produceItem].push_back(tree);
				}
			}
		}

		// Things, chests and people are kept, the walls and trees are not looked at. A reference is kept by its form id
		// and looked up when it's shown, one of the plugins can still be deleted by a script
		void ReadPlaces(Spread& a_spread)
		{
			Places::WalkLasting([&](RE::FormID a_id, const RE::TESObjectREFR& a_reference) {
				const auto* base = a_reference.GetBaseObject();
				if (Lasting(base) && (base->GetFormType() == RE::FormType::Container || BrowserIndex::Find(base->GetFormID()))) {
					a_spread.placed[base].push_back(a_id);
				}
			});
		}

		// The plugin files are read for the things the browser knows, the people of its living things and every chest
		void StartFiles()
		{
			if (BrowserFiles::Started()) {
				return;
			}
			std::unordered_set<RE::FormID> wanted;
			for (const auto& item : BrowserIndex::Items()) {
				wanted.insert(item.form->GetFormID());
			}
			for (const auto& people : BrowserIndex::Units()) {
				for (const auto* person : people) {
					wanted.insert(person->GetFormID());
				}
			}
			for (const auto* chest : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectCONT>()) {
				if (Lasting(chest)) {
					wanted.insert(chest->GetFormID());
				}
			}
			BrowserFiles::Start(std::move(wanted));
			BrowserHandouts::Start();
			BrowserQuestLinks::Start();
			ItemTexts::Start();
		}

		// the plugin files, the files of the frameworks that hand things out and the quests' records are still being read
		bool Reading()
		{
			return (BrowserFiles::Started() && !BrowserFiles::Ready()) || (BrowserHandouts::Started() && !BrowserHandouts::Ready()) ||
			       (BrowserQuestLinks::Started() && !BrowserQuestLinks::Ready()) || (ItemTexts::Started() && !ItemTexts::Ready());
		}

		// Who holds, deals out and makes what comes from Holders, the item window of Inventory Reforged asks that much
		// for its quest lines. The browser needs the plants and the places too and reads them on its first ask
		bool placesRead = false;

		Holders::Known& Read()
		{
			if (!placesRead) {
				Trail::Step("sources, who holds, sells and makes what");
			}
			auto& read = Holders::Read();
			if (placesRead) {
				return read;
			}
			static const auto work = Profile::Work("browser sources");
			const Profile::Timer timer{ work };
			const auto start = std::chrono::steady_clock::now();
			Trail::Step("sources, plants");
			ReadPlants(spread, *RE::TESDataHandler::GetSingleton());
			Trail::Step("sources, places");
			ReadPlaces(spread);
			Trail::Step("sources, plugin files");
			StartFiles();
			placesRead = true;
			const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			logger::info("the item browser read the plants and places in {:.1f} ms: {} lists hold something, {} holders, {} recipes, {} lasting places", took,
				read.parents.size(), read.holders.size(), read.makes.size(), spread.placed.size());
			return read;
		}

		// how many things a list gives at most, counted up to one past a small list
		std::size_t Leaves(const RE::TESLevItem& a_list, std::size_t a_depth = 0)
		{
			if (const auto counted = spread.leaves.find(&a_list); counted != spread.leaves.end()) {
				return counted->second;
			}
			std::size_t count = 0;
			for (const auto& entry : a_list.entries) {
				const auto* inner = entry.form ? entry.form->As<RE::TESLevItem>() : nullptr;
				count += inner ? (a_depth < 16 ? Leaves(*inner, a_depth + 1) : kSmallList + 1) : 1;
				if (count > kSmallList) {
					break;
				}
			}
			// a count cut short by the depth of the walk holds for this walk only
			if (a_depth == 0) {
				spread.leaves.emplace(&a_list, count);
			}
			return count;
		}

		// The item and every list that can give it, each with the lowest level of the player it can come at, the
		// lowest first so the names shown first are the ones a player meets first, the same ones in every game
		std::vector<std::pair<const RE::TESForm*, std::uint16_t>> Givers(const std::unordered_map<const RE::TESForm*, std::vector<Parent>>& a_parents, const RE::TESForm& a_item)
		{
			std::unordered_map<const RE::TESForm*, std::uint16_t> level{ { &a_item, std::uint16_t{ 1 } } };
			std::vector<const RE::TESForm*> waiting{ &a_item };
			while (!waiting.empty() && level.size() < kMostLists) {
				const auto* form = waiting.back();
				waiting.pop_back();
				const auto found = a_parents.find(form);
				if (found == a_parents.end()) {
					continue;
				}
				for (const auto& parent : found->second) {
					const auto needs = std::max(level[form], parent.level);
					const auto had = level.find(parent.list);
					if (had == level.end() || needs < had->second) {
						level[parent.list] = needs;
						waiting.push_back(parent.list);
					}
				}
			}
			std::vector<std::pair<const RE::TESForm*, std::uint16_t>> givers(level.begin(), level.end());
			std::ranges::sort(givers, [](const auto& a_one, const auto& a_other) {
				return a_one.second != a_other.second ? a_one.second < a_other.second : a_one.first->GetFormID() < a_other.first->GetFormID();
			});
			return givers;
		}

		// a list made for this one thing, or the item itself
		bool Own(const RE::TESForm& a_form, const RE::TESForm& a_item)
		{
			const auto* list = a_form.As<RE::TESLevItem>();
			return &a_form == &a_item || (list && Leaves(*list) <= kSmallList);
		}

		// The places of a form, where the plugin files put it once they are read and its references that last the whole
		// game until then. Tells whether it stands anywhere
		template <class F>
		bool ForEachPlace(const RE::TESForm& a_base, F&& a_take)
		{
			if (BrowserFiles::Ready()) {
				const auto spots = BrowserFiles::SpotsOf(a_base.GetFormID());
				for (std::size_t i = 0; i < spots.size() && i < kMostSpots; ++i) {
					a_take(Places::At(spots[i].cell, spots[i].world, spots[i].position));
				}
				return !spots.empty();
			}
			const auto found = spread.placed.find(&a_base);
			if (found == spread.placed.end()) {
				return false;
			}
			for (const auto reference : found->second) {
				a_take(Places::OfReference(reference));
			}
			return true;
		}

		// the framework that hands a thing out and its chance in percents, a sure way says nothing
		void PutTag(RE::GFxValue& a_one, const Tag& a_tag)
		{
			if (!a_tag.by) {
				return;
			}
			a_one.SetMember("by", a_tag.by);
			if (a_tag.chance < 100.0f) {
				a_one.SetMember("chance", std::round(a_tag.chance * 10.0) / 10.0);
			}
			if (a_tag.somewhere) {
				a_one.SetMember("some", true);
			}
		}

		// Names each once with the lowest level each one gives the item at, shown from the lowest level, in the
		// order found within one level. Gold or torches are held by thousands, each name is found by a hash. A name
		// the browser has an entry for carries its id, the page makes a link of it. A name a mod hands out while the
		// game runs carries the mod's framework and its chance, a way the records give for sure wins over it
		class Names
		{
		public:
			void Add(const RE::TESForm* a_form, std::uint16_t a_level, Tag a_tag = {})
			{
				if (Named(a_form)) {
					auto name = NameOf(*a_form);
					if (!BrowserIndex::Unshown(*a_form, name)) {
						Add(std::move(name), a_level, LinkOf(a_form), a_tag);
					}
				}
			}

			void Add(std::string a_name, std::uint16_t a_level, RE::FormID a_link = 0, Tag a_tag = {})
			{
				Add(Where{ std::move(a_name) }, a_level, a_link, a_tag);
			}

			void Add(Where a_where, std::uint16_t a_level, RE::FormID a_link = 0, Tag a_tag = {})
			{
				if (a_where.name.empty()) {
					return;
				}
				// the place's own entry, the progression mode masks a place the character hasn't found by it
				const auto place = LinkOf(a_where.location);
				const auto [at, fresh] = places.try_emplace(std::format("{}|{}{}", a_where.thing, a_where.nearby ? "~" : "", a_where.name), names.size());
				if (fresh) {
					names.push_back({ std::move(a_where.name), a_level, a_link, a_where.marker, a_where.nearby, std::move(a_where.thing), a_tag, place });
					return;
				}
				auto& had = names[at->second];
				had.level = std::min(had.level, a_level);
				had.link = had.link != 0 ? had.link : a_link;
				had.marker = had.marker != 0 ? had.marker : a_where.marker;
				had.place = had.place != 0 ? had.place : place;
				if (had.tag.by && (!a_tag.by || a_tag.chance > had.tag.chance)) {
					had.tag = a_tag;
				}
			}

			[[nodiscard]] std::size_t Size() const { return names.size(); }

			// The first few, or the whole list once the player opened it on the page. The page is told how many are
			// left out and how many it shows of a list that's shut
			void Put(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const char* a_name, std::size_t a_shown, bool a_whole) const
			{
				auto shown = names;
				std::ranges::stable_sort(shown, {}, &Name::level);
				const auto sent = a_whole ? kWholeList : a_shown;
				RE::GFxValue list;
				a_movie.CreateArray(&list);
				for (std::size_t i = 0; i < shown.size() && i < sent; ++i) {
					RE::GFxValue one;
					a_movie.CreateObject(&one);
					one.SetMember("name", shown[i].name.c_str());
					one.SetMember("level", static_cast<double>(shown[i].level));
					one.SetMember("id", static_cast<double>(shown[i].link));
					// the map shows the player's world only, a place of another world says which instead
					if (const auto world = shown[i].marker != 0 ? BrowserMap::OtherWorld(shown[i].marker) : std::string(); !world.empty()) {
						one.SetMember("world", world.c_str());
					} else if (shown[i].marker != 0) {
						one.SetMember("marker", static_cast<double>(shown[i].marker));
					}
					if (shown[i].nearby) {
						one.SetMember("near", true);
					}
					if (!shown[i].thing.empty()) {
						one.SetMember("thing", shown[i].thing.c_str());
					}
					if (shown[i].place != 0) {
						one.SetMember("place", static_cast<double>(shown[i].place));
					}
					PutTag(one, shown[i].tag);
					list.PushBack(one);
				}
				a_object.SetMember(a_name, list);
				a_object.SetMember(std::format("{}More", a_name).c_str(), static_cast<double>(shown.size() > sent ? shown.size() - sent : 0));
				a_object.SetMember(std::format("{}Shown", a_name).c_str(), static_cast<double>(a_shown));
			}

		private:
			struct Name
			{
				std::string name;
				std::uint16_t level;
				RE::FormID link;
				RE::FormID marker;
				bool nearby;
				std::string thing;
				Tag tag;
				RE::FormID place;
			};
			std::vector<Name> names;
			std::unordered_map<std::string, std::size_t> places;
		};

		// The words of a name the editor gave: a mod's short prefix before "_" left out, "CampfireCookingShared" split
		// where a word starts or at "_", the words every bench's keyword has dropped
		std::string EditorWords(std::string_view a_id)
		{
			static constexpr std::array<std::string_view, 3> kPlain{ "Crafting", "Shared", "Keyword" };
			constexpr std::size_t kPrefix = 8;
			if (const auto cut = a_id.find('_'); cut != std::string_view::npos && cut <= kPrefix &&
												  std::ranges::any_of(a_id.substr(cut + 1), [](unsigned char a_letter) { return std::isalpha(a_letter) != 0; })) {
				a_id.remove_prefix(cut + 1);
			}
			const auto upper = [&](std::size_t a_at) { return std::isupper(static_cast<unsigned char>(a_id[a_at])) != 0; };
			const auto lower = [&](std::size_t a_at) { return std::islower(static_cast<unsigned char>(a_id[a_at])) != 0; };
			std::string words;
			std::size_t from = 0;
			for (std::size_t i = 1; i <= a_id.size(); ++i) {
				const bool starts = i < a_id.size() && (a_id[i] == '_' || (upper(i) && (lower(i - 1) || (i + 1 < a_id.size() && lower(i + 1) && upper(i - 1)))));
				if (i < a_id.size() && !starts) {
					continue;
				}
				auto word = a_id.substr(from, i - from);
				if (word.starts_with('_')) {
					word.remove_prefix(1);
				}
				if (!word.empty() && std::ranges::find(kPlain, word) == kPlain.end()) {
					words += (words.empty() ? "" : " ") + std::string(word);
				}
				from = i;
			}
			return words;
		}

		// A bench is named after the first piece of furniture of the world with its keyword, one no named furniture has,
		// the campfire of a camping mod, by its keyword: "Camping_CampfireCookingShared" is Campfire Cooking
		std::string BenchOf(const Known& a_known, const RE::BGSConstructibleObject& a_recipe)
		{
			const auto* keyword = Holders::Bench(a_recipe);
			if (!keyword) {
				return "";
			}
			const auto named = a_known.benches.find(keyword);
			return named != a_known.benches.end() ? named->second : EditorWords(keyword->formEditorID.c_str());
		}

		// benches that improve a thing one has, any other makes one
		bool Improves(const RE::BGSConstructibleObject& a_recipe)
		{
			return Holders::Improves(a_recipe);
		}

		// What a condition of a recipe or a perk asks in words the page can tell: a perk, a thing carried or worn, a spell
		// known, a stage of a quest or the quest done, a skill, the improved thing not enchanted or enchanted. One the page
		// doesn't tell is none. A thing to carry that the recipe takes anyway holds whenever the recipe can be made, and so
		// does an earlier rank of a perk's own chain, a_holds
		struct Need
		{
			const char* kind = "";
			const RE::TESForm* form = nullptr;
			std::string name;
			double number = 0.0;
		};

		// the things a recipe takes, none for a perk
		std::span<RE::ContainerObject* const> PartsOf(const RE::BGSConstructibleObject* a_recipe)
		{
			return a_recipe ? std::span<RE::ContainerObject* const>(a_recipe->requiredItems.containerObjects, a_recipe->requiredItems.numContainerObjects) :
			                  std::span<RE::ContainerObject* const>();
		}

		std::optional<Need> NeedOf(const RE::TESConditionItem& a_condition, const RE::BGSConstructibleObject* a_recipe, std::span<const RE::BGSPerk* const> a_given, bool& a_holds)
		{
			using Function = RE::FUNCTION_DATA::FunctionID;
			using Compare = RE::CONDITION_ITEM_DATA::OpCode;
			const auto& data = a_condition.data;
			a_holds = false;
			// asked of the one who makes it, or of the player's reference by name (Cloaks of Skyrim asks PlayerRef)
			const auto object = data.object.get();
			const bool player = object == RE::CONDITIONITEMOBJECT::kSelf ||
			                    (object == RE::CONDITIONITEMOBJECT::kRef && data.runOnRef.get().get() == RE::PlayerCharacter::GetSingleton());
			if (data.flags.global || !player) {
				return std::nullopt;
			}
			const auto compare = data.flags.opCode;
			const auto value = data.comparisonValue.f;
			const bool yes = (compare == Compare::kEqualTo && value == 1.0F) || (compare == Compare::kNotEqualTo && value == 0.0F);
			const bool no = (compare == Compare::kEqualTo && value == 0.0F) || (compare == Compare::kNotEqualTo && value == 1.0F);
			const bool atLeast = compare == Compare::kGreaterThanOrEqualTo || compare == Compare::kGreaterThan;
			const auto least = compare == Compare::kGreaterThan ? value + 1.0F : value;
			const auto* form = static_cast<const RE::TESForm*>(data.functionData.params[0]);
			switch (data.functionData.function.get()) {
			case Function::kHasPerk:
				a_holds = yes && std::ranges::any_of(a_given, [&](const RE::BGSPerk* a_rank) { return a_rank == form; });
				return yes && Named(form) ? std::optional<Need>(Need{ "perk", form, NameOf(*form) }) : std::nullopt;
			case Function::kHasSpell:
				return yes && Named(form) ? std::optional<Need>(Need{ "spell", form, form->GetName() }) : std::nullopt;
			case Function::kGetEquipped:
				if (no && Named(form)) {
					// a smelter melts a piece down only when it isn't the one worn, said once for all its recipes
					const bool taken = std::ranges::any_of(PartsOf(a_recipe), [&](const RE::ContainerObject* a_entry) { return a_entry && a_entry->obj == form; });
					return taken ? Need{ "unworn" } : Need{ "notWearing", form, NameOf(*form) };
				}
				return yes && Named(form) ? std::optional<Need>(Need{ "worn", form, NameOf(*form) }) : std::nullopt;
			case Function::kGetQuestCompleted:
				return yes && Named(form) ? std::optional<Need>(Need{ "quest", form, NameOf(*form) }) : std::nullopt;
			case Function::kGetStageDone:
				return yes && Named(form) ? std::optional<Need>(Need{ "stage", form, NameOf(*form), static_cast<double>(reinterpret_cast<std::uintptr_t>(data.functionData.params[1])) }) :
				                            std::nullopt;
			case Function::kGetStage:
				if (!Named(form)) {
					return std::nullopt;
				}
				if (atLeast) {
					return Need{ "stage", form, NameOf(*form), least };
				}
				return compare == Compare::kLessThan ? std::optional<Need>(Need{ "before", form, NameOf(*form), value }) : std::nullopt;
			case Function::kEPTemperingItemIsEnchanted:
				return yes || no ? std::optional<Need>(Need{ yes ? "enchanted" : "plain" }) : std::nullopt;
			case Function::kGetItemCount:
				if (!atLeast || least < 1.0F || !Named(form)) {
					return std::nullopt;
				}
				for (const auto* entry : PartsOf(a_recipe)) {
					if (entry && entry->obj == form && entry->count >= least) {
						a_holds = true;
					}
				}
				return Need{ "carried", form, NameOf(*form), least };
			case Function::kGetBaseActorValue:
			case Function::kGetActorValue: {
				const auto* skill = RE::ActorValueList::GetActorValueInfo(static_cast<RE::ActorValue>(reinterpret_cast<std::uintptr_t>(data.functionData.params[0])));
				const char* named = skill ? skill->GetFullName() : nullptr;
				return atLeast && named && *named ? std::optional<Need>(Need{ "skill", nullptr, named, least }) : std::nullopt;
			}
			default:
				return std::nullopt;
			}
		}

		// A recipe as its bench, what it takes and what it asks besides
		RE::GFxValue Recipe(RE::GFxMovie& a_movie, const Known& a_known, const RE::BGSConstructibleObject& a_recipe)
		{
			RE::GFxValue recipe;
			a_movie.CreateObject(&recipe);
			recipe.SetMember("bench", BenchOf(a_known, a_recipe).c_str());
			recipe.SetMember("count", static_cast<double>(std::max<std::uint16_t>(a_recipe.data.numConstructed, 1)));
			RE::GFxValue takes;
			a_movie.CreateArray(&takes);
			for (std::uint32_t i = 0; i < a_recipe.requiredItems.numContainerObjects; ++i) {
				const auto* entry = a_recipe.requiredItems.containerObjects[i];
				if (entry && Named(entry->obj)) {
					RE::GFxValue one;
					a_movie.CreateObject(&one);
					one.SetMember("name", NameOf(*entry->obj).c_str());
					one.SetMember("count", static_cast<double>(entry->count));
					one.SetMember("id", static_cast<double>(LinkOf(entry->obj)));
					takes.PushBack(one);
				}
			}
			recipe.SetMember("takes", takes);
			PutNeeds(a_movie, recipe, a_recipe.conditions, &a_recipe);
			return recipe;
		}

		void PutRecipes(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const Known& a_known, const RE::TESForm& a_item, bool a_whole)
		{
			RE::GFxValue made;
			RE::GFxValue improved;
			a_movie.CreateArray(&made);
			a_movie.CreateArray(&improved);
			const auto [first, end] = a_known.makes.equal_range(&a_item);
			for (auto at = first; at != end; ++at) {
				auto recipe = Recipe(a_movie, a_known, *at->second);
				(Improves(*at->second) ? improved : made).PushBack(recipe);
			}
			a_object.SetMember("made", made);
			a_object.SetMember("improved", improved);

			// what the item goes into, each made thing once, the improving of other things left out
			Names uses;
			const auto [from, to] = a_known.takes.equal_range(&a_item);
			for (auto at = from; at != to; ++at) {
				if (!Improves(*at->second) && Named(at->second->createdItem)) {
					const auto bench = BenchOf(a_known, *at->second);
					const auto* created = at->second->createdItem;
					uses.Add(bench.empty() ? NameOf(*created) : std::format("{} ({})", NameOf(*created), bench), 1, LinkOf(created));
				}
			}
			uses.Put(a_movie, a_object, "uses", kUsesShown, a_whole);
		}

		// Every thing a list or an outfit can give, lists in lists walked, each thing once
		void AddLeaves(const RE::TESForm* a_form, Names& a_names, std::unordered_set<const RE::TESForm*>& a_seen, Tag a_tag = {})
		{
			if (!a_form || a_seen.size() > kMostLeaves || !a_seen.insert(a_form).second) {
				return;
			}
			if (const auto* list = a_form->As<RE::TESLevItem>()) {
				for (const auto& entry : list->entries) {
					AddLeaves(entry.form, a_names, a_seen, a_tag);
				}
				return;
			}
			if (const auto* outfit = a_form->As<RE::BGSOutfit>()) {
				for (const auto* piece : outfit->outfitItems) {
					AddLeaves(piece, a_names, a_seen, a_tag);
				}
				return;
			}
			a_names.Add(a_form, 1, a_tag);
		}

		// what mods hand a person while the game runs: SPID's gifts to the living and the dead, CID's stock of the
		// person, each line walked on its own so its chance stays its own
		void AddHandedTo(const RE::TESNPC& a_person, Names& a_carried, Names& a_dropped)
		{
			for (const auto* gift : BrowserHandouts::GiftsTo(a_person)) {
				std::unordered_set<const RE::TESForm*> seen;
				AddLeaves(gift->gives, gift->death ? a_dropped : a_carried, seen, gift->tag);
			}
			for (const auto* stock : BrowserHandouts::StocksInto(a_person)) {
				std::unordered_set<const RE::TESForm*> seen;
				AddLeaves(stock->gives, a_carried, seen, stock->tag);
			}
		}

		// what CID puts into a merchant's chest, by its base or the chest itself
		void AddStocked(const RE::TESObjectREFR& a_chest, Names& a_sells)
		{
			const auto* base = a_chest.GetBaseObject();
			for (const auto* stock : base ? BrowserHandouts::StocksInto(*base) : std::vector<const BrowserHandouts::Stock*>{}) {
				if (stock->reference == 0 || stock->reference == a_chest.GetFormID()) {
					std::unordered_set<const RE::TESForm*> seen;
					AddLeaves(stock->gives, a_sells, seen, stock->tag);
				}
			}
		}

		// the ways an item comes, each a list of the page
		struct Ways
		{
			Names sold;
			Names carried;
			Names dropped;
			Names harvested;
			Names chests;
			// the things and the single spots Base Object Swapper puts the item in place of
			Names swappedFor;
			Names swappedAt;
		};

		// the merchants who sell from a chest of their faction, the faction itself when no record names its people
		void AddSellers(const Known& a_known, const std::vector<const RE::TESFaction*>& a_factions, std::uint16_t a_level, Names& a_sold, Tag a_tag = {})
		{
			for (const auto* faction : a_factions) {
				const auto people = a_known.vendors.find(faction);
				if (people == a_known.vendors.end()) {
					a_sold.Add(faction, a_level, a_tag);
					continue;
				}
				for (const auto* person : people->second) {
					a_sold.Add(person, a_level, a_tag);
				}
			}
		}

		// A chest made for this thing tells where it stands, its name alone when it stands nowhere. A spot with no name
		// is a developers' test room or the cell the game keeps things in, no player comes there. So a chest that
		// stands only in such spots says nothing
		void AddChest(const RE::TESForm& a_chest, std::uint16_t a_level, Names& a_chests, Tag a_tag = {})
		{
			const std::string chest = Named(&a_chest) ? a_chest.GetName() : "";
			const bool stands = ForEachPlace(a_chest, [&](Where a_where) {
				if (!a_where.name.empty()) {
					a_where.thing = chest;
					a_chests.Add(std::move(a_where), a_level, 0, a_tag);
				}
			});
			if (!stands) {
				a_chests.Add(&a_chest, a_level, a_tag);
			}
		}

		// the place of one reference comes from the plugin files once read, from the game while its cell is loaded
		Where PlaceOfReference(RE::FormID a_reference)
		{
			if (const auto* spot = BrowserFiles::SpotOfReference(a_reference)) {
				return Places::At(spot->cell, spot->world, spot->position);
			}
			return Places::OfReference(a_reference);
		}

		// A CID line puts the thing into a person, a merchant's chest, every chest of a kind or one chest in the world
		void AddStock(const Known& a_known, const BrowserHandouts::Stock& a_stock, std::uint16_t a_level, Ways& a_ways)
		{
			if (a_stock.into->Is(RE::FormType::NPC)) {
				a_ways.carried.Add(a_stock.into, a_level, a_stock.tag);
				return;
			}
			if (const auto vendor = a_known.vendorChests.find(a_stock.into); vendor != a_known.vendorChests.end()) {
				std::vector<const RE::TESFaction*> factions;
				for (const auto* faction : vendor->second) {
					const auto* chest = faction->vendorData.merchantContainer;
					if (a_stock.reference == 0 || (chest && chest->GetFormID() == a_stock.reference)) {
						factions.push_back(faction);
					}
				}
				if (!factions.empty()) {
					AddSellers(a_known, factions, a_level, a_ways.sold, a_stock.tag);
					return;
				}
			}
			if (a_stock.reference == 0) {
				AddChest(*a_stock.into, a_level, a_ways.chests, a_stock.tag);
				return;
			}
			auto where = PlaceOfReference(a_stock.reference);
			if (where.name.empty()) {
				a_ways.chests.Add(a_stock.into, a_level, a_stock.tag);
				return;
			}
			where.thing = Named(a_stock.into) ? a_stock.into->GetName() : "";
			a_ways.chests.Add(std::move(where), a_level, 0, a_stock.tag);
		}

		// What mods hand out of a giver while the game runs: SPID to the people whose records fit its line, CID into
		// containers, BOS in place of other things. A SPID line no record can be named for is told by its filters
		void AddHandouts(const Known& a_known, const RE::TESForm& a_giver, std::uint16_t a_level, Ways& a_ways, std::vector<const BrowserHandouts::Gift*>& a_told)
		{
			for (const auto* gift : BrowserHandouts::GiftsOf(a_giver)) {
				const auto& people = BrowserHandouts::PeopleOf(*gift);
				if (people.empty()) {
					a_told.push_back(gift);
					continue;
				}
				for (const auto* person : people) {
					(gift->death ? a_ways.dropped : a_ways.carried).Add(person, a_level, gift->tag);
				}
			}
			for (const auto* stock : BrowserHandouts::StocksOf(a_giver)) {
				AddStock(a_known, *stock, a_level, a_ways);
			}
			for (const auto* swap : BrowserHandouts::SwapsTo(a_giver)) {
				if (swap->reference != 0) {
					a_ways.swappedAt.Add(PlaceOfReference(swap->reference), a_level, 0, swap->tag);
				} else {
					a_ways.swappedFor.Add(swap->from, a_level, swap->tag);
				}
			}
		}

		// the SPID lines told by their filters, each once
		void PutTold(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const std::vector<const BrowserHandouts::Gift*>& a_told)
		{
			RE::GFxValue list;
			a_movie.CreateArray(&list);
			std::set<std::string> seen;
			for (const auto* gift : a_told) {
				if (!seen.insert(std::format("{}|{}|{}", gift->death, gift->tag.chance, gift->filters)).second) {
					continue;
				}
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				one.SetMember("filters", gift->filters.c_str());
				one.SetMember("death", gift->death);
				PutTag(one, gift->tag);
				list.PushBack(one);
			}
			a_object.SetMember("told", list);
		}

		// what a person carries and what it drops, each walked on its own
		struct Belongings
		{
			Names carried;
			Names dropped;
			std::unordered_set<const RE::TESForm*> carriedSeen;
			std::unordered_set<const RE::TESForm*> droppedSeen;
		};

		// A person that takes its things from a template carries what the template carries, a list of templates gives
		// what any of them carries. What the record names itself comes first, its sword and its armor, then the lists
		// of what could be in the pockets of anyone, because they give a hundred things and would bury those
		void AddCarried(const RE::TESNPC& a_person, Belongings& a_things, bool a_lists, int a_depth = 0)
		{
			// the flags count wherever the record names a template, the game's "uses template" bit is often unset
			const auto* inherits = a_person.actorData.templateUseFlags.any(RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG::kInventory) ? a_person.baseTemplateForm : nullptr;
			if (inherits && a_depth < 4) {
				if (const auto* model = inherits->As<RE::TESNPC>()) {
					AddCarried(*model, a_things, a_lists, a_depth + 1);
				} else if (const auto* list = inherits->As<RE::TESLevCharacter>()) {
					for (const auto& entry : list->entries) {
						if (const auto* one = entry.form ? entry.form->As<RE::TESNPC>() : nullptr) {
							AddCarried(*one, a_things, a_lists, a_depth + 1);
						}
					}
				}
				return;
			}
			std::vector<const RE::TESForm*> own;
			for (std::uint32_t i = 0; i < a_person.numContainerObjects; ++i) {
				const auto* entry = a_person.containerObjects[i];
				own.push_back(entry ? entry->obj : nullptr);
			}
			for (const auto* outfit : { a_person.defaultOutfit, a_person.sleepOutfit }) {
				if (!outfit) {
					continue;
				}
				for (const auto* piece : outfit->outfitItems) {
					own.push_back(piece);
				}
			}
			for (const auto* thing : own) {
				if (thing && (thing->As<RE::TESLevItem>() != nullptr) == a_lists) {
					AddLeaves(thing, a_things.carried, a_things.carriedSeen);
				}
			}
			if (a_lists) {
				AddLeaves(a_person.deathItem, a_things.dropped, a_things.droppedSeen);
			}
		}

		void FillLiving(RE::GFxMovie& a_movie, RE::GFxValue& a_object, Known& a_known, const std::vector<RE::TESNPC*>& a_people, bool a_whole)
		{
			Belongings things;
			Names sells;
			Names stands;
			std::unordered_set<const RE::TESForm*> stocked;
			std::set<const RE::TESForm*> lists;
			std::uint16_t first = 0xFFFF;
			// the things the records name, then what mods hand out, then the lists of anyone's pockets
			for (const auto* person : a_people) {
				AddCarried(*person, things, false);
			}
			for (const auto* person : a_people) {
				AddHandedTo(*person, things.carried, things.dropped);
			}
			for (const auto* person : a_people) {
				AddCarried(*person, things, true);
				for (const auto& rank : person->factions) {
					const auto* chest = rank.faction ? rank.faction->vendorData.merchantContainer : nullptr;
					const auto* base = chest ? chest->GetBaseObject() : nullptr;
					if (const auto* stock = base ? base->As<RE::TESObjectCONT>() : nullptr) {
						for (std::uint32_t i = 0; i < stock->numContainerObjects; ++i) {
							AddLeaves(stock->containerObjects[i] ? stock->containerObjects[i]->obj : nullptr, sells, stocked);
						}
						AddStocked(*chest, sells);
					}
				}
				ForEachPlace(*person, [&](Where a_where) { stands.Add(std::move(a_where), 1); });
				for (const auto& [list, level] : Givers(a_known.encounters, *person)) {
					if (list != person) {
						lists.insert(list);
						first = std::min(first, level);
					}
				}
			}
			things.carried.Put(a_movie, a_object, "carries", kThingsShown, a_whole);
			things.dropped.Put(a_movie, a_object, "drops", kThingsShown, a_whole);
			sells.Put(a_movie, a_object, "sells", kThingsShown, a_whole);
			stands.Put(a_movie, a_object, "stands", kPlacesShown, a_whole);
			a_object.SetMember("reading", Reading());
			a_object.SetMember("encounters", static_cast<double>(lists.size()));
			a_object.SetMember("encounterLevel", static_cast<double>(lists.empty() ? 0 : first));
		}

		// Where the one of a unique entry is now, alive or dead. The game moves people about, the plugin files only say
		// where they were put
		void PutNow(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const BrowserIndex::Item& a_entry, bool a_whole)
		{
			Names alive;
			Names dead;
			for (auto* actor : BrowserActors::Of(a_entry)) {
				auto where = Places::Of(*actor);
				const auto link = LinkOf(where.location);
				(actor->IsDead() ? dead : alive).Add(std::move(where), 1, link);
			}
			alive.Put(a_movie, a_object, "now", kPlacesShown, a_whole);
			dead.Put(a_movie, a_object, "deadNow", kPlacesShown, a_whole);
		}

		// A rank of a relationship as Papyrus counts it, 4 a lover down to -4 an archnemesis, 0 an acquaintance as everyone
		// is to everyone at first
		int RankOf(const RE::BGSRelationship& a_relation)
		{
			return 4 - static_cast<int>(a_relation.level.underlying());
		}

		// what the other one of a relationship is to the person, by the association's words for that side and their sex:
		// "husband", "daughter", "employer"
		std::string RoleOf(const RE::BGSRelationship& a_relation, const RE::TESNPC& a_person, const RE::TESNPC& a_other)
		{
			using Members = RE::BGSAssociationType::Members;
			const auto* kind = a_relation.assocType;
			if (!kind) {
				return {};
			}
			const auto side = a_relation.npc1 == &a_person ? Members::kChild : Members::kParent;
			const auto sex = a_other.GetSex() == RE::SEX::kFemale ? RE::SEXES::kFemale : RE::SEXES::kMale;
			const char* role = kind->associationLabels[side][sex].c_str();
			return role ? role : "";
		}

		// The relationships of the people of a living entry past what everyone is to everyone: lovers, allies, friends,
		// rivals, foes, enemies, each with who they are to the person. The one with the player goes into its own part
		void PutRelations(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const std::vector<RE::TESNPC*>& a_people)
		{
			struct Relation
			{
				std::string name;
				RE::FormID link;
				int rank;
				std::string role;
				bool secret;
			};
			std::vector<Relation> found;
			std::optional<Relation> player;
			for (const auto* person : a_people) {
				// What a bug report needs: the records the game keeps on the person, each with the other one and its
				// rank. Lydia's bond of an ally can have no other one (00000000), Jordis's names the player
				std::string told;
				for (const auto* relation : person->relationships ? *person->relationships : RE::BSTArray<RE::BGSRelationship*>{}) {
					const auto* other = relation ? (relation->npc1 == person ? relation->npc2 : relation->npc1) : nullptr;
					told += std::format(" {:08X}:{}", other ? other->GetFormID() : 0, relation ? RankOf(*relation) : 99);
				}
				if (!told.empty() || a_people.size() == 1) {
					logger::info("relationships of {:08X}:{}", person->GetFormID(), told.empty() ? " none" : told);
				}
				for (const auto* relation : person->relationships ? *person->relationships : RE::BSTArray<RE::BGSRelationship*>{}) {
					const auto* other = relation ? (relation->npc1 == person ? relation->npc2 : relation->npc1) : nullptr;
					if (!other || other == person) {
						continue;
					}
					Relation one{ NameOf(*other), LinkOf(other), RankOf(*relation), RoleOf(*relation, *person, *other), relation->flags.any(RE::BGSRelationship::Flag::kSecret) };
					if (other->GetFormID() == kPlayerBase) {
						player = std::move(one);
						continue;
					}
					if (one.rank == 0 || one.name.empty() || BrowserIndex::Unshown(*other, one.name) ||
						std::ranges::any_of(found, [&](const Relation& a_had) { return a_had.name == one.name && a_had.rank == one.rank; })) {
						continue;
					}
					found.push_back(std::move(one));
				}
			}
			std::ranges::stable_sort(found, std::greater{}, &Relation::rank);
			RE::GFxValue list;
			a_movie.CreateArray(&list);
			for (std::size_t i = 0; i < found.size() && i < kWholeList; ++i) {
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				one.SetMember("name", found[i].name.c_str());
				one.SetMember("rank", static_cast<double>(found[i].rank));
				if (found[i].link != 0) {
					one.SetMember("id", static_cast<double>(found[i].link));
				}
				if (!found[i].role.empty()) {
					one.SetMember("role", found[i].role.c_str());
				}
				if (found[i].secret) {
					one.SetMember("secret", true);
				}
				list.PushBack(one);
			}
			a_object.SetMember("relations", list);
			if (player) {
				a_object.SetMember("playerRank", static_cast<double>(player->rank));
				if (!player->role.empty()) {
					a_object.SetMember("playerRole", player->role.c_str());
				}
			}
		}

		// the first person a list of templates gives, lists inside it walked
		const RE::TESNPC* FirstOf(const RE::TESForm* a_form, int a_depth = 0)
		{
			if (const auto* person = a_form ? a_form->As<RE::TESNPC>() : nullptr) {
				return person;
			}
			const auto* list = a_form ? a_form->As<RE::TESLevCharacter>() : nullptr;
			for (std::size_t i = 0; list && a_depth < 8 && i < list->entries.size(); ++i) {
				if (const auto* found = FirstOf(list->entries[i].form, a_depth + 1)) {
					return found;
				}
			}
			return nullptr;
		}

		// The record a person takes its fight from, the template's when the record says so. A bandit chief takes it from
		// a list of lists of bandits
		const RE::TESNPC& FightOf(const RE::TESNPC& a_person)
		{
			using Use = RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG;
			const RE::TESNPC* at = &a_person;
			for (int depth = 0; depth < 6 && at->baseTemplateForm && at->actorData.templateUseFlags.any(Use::kAIData); ++depth) {
				const auto* next = FirstOf(at->baseTemplateForm);
				if (!next || next == at) {
					break;
				}
				at = next;
			}
			return *at;
		}

		// How a living thing meets the player: how it starts fights and whom it helps, by its record, and for the one
		// there is of a unique one, whether it's hostile to the player now and what its factions make of the player
		void PutToward(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const BrowserIndex::Item& a_entry, const std::vector<RE::TESNPC*>& a_people)
		{
			if (a_people.empty()) {
				return;
			}
			// the fight most of the records of an entry have. Of 83 bandit chiefs the first, a boss of an ambush, never
			// starts one, the other 82 attack enemies and strangers
			std::map<std::pair<int, int>, std::size_t> votes;
			for (const auto* person : a_people) {
				const auto& fight = FightOf(*person);
				votes[{ std::to_underlying(fight.GetAggressionLevel()), std::to_underlying(fight.GetAssistanceLevel()) }]++;
			}
			const auto most = std::ranges::max_element(votes, {}, &decltype(votes)::value_type::second)->first;
			a_object.SetMember("aggression", static_cast<double>(most.first));
			a_object.SetMember("assistance", static_cast<double>(most.second));
			std::vector<RE::Actor*> alive;
			for (auto* actor : BrowserActors::Of(a_entry)) {
				if (!actor->IsDead()) {
					alive.push_back(actor);
				}
			}
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (alive.size() != 1 || !player) {
				return;
			}
			a_object.SetMember("hostile", alive.front()->IsHostileToActor(player));
			static constexpr std::array<const char*, 4> kReactions{ "", "enemy", "ally", "friend" };
			const auto reaction = std::to_underlying(alive.front()->GetFactionReaction(player));
			if (reaction > 0 && static_cast<std::size_t>(reaction) < kReactions.size()) {
				a_object.SetMember("reaction", kReactions[reaction]);
			}
		}

		// The quests with a name the forms take part in, each once with how far the player is with it
		void PutQuests(RE::GFxMovie& a_movie, RE::GFxValue& a_object, std::span<const RE::TESForm* const> a_forms, bool a_whole)
		{
			std::vector<const RE::TESQuest*> quests;
			for (const auto* form : a_forms) {
				for (const auto* quest : BrowserQuests::QuestsOf(*form)) {
					if (std::ranges::find(quests, quest) == quests.end()) {
						quests.push_back(quest);
					}
				}
			}
			RE::GFxValue list;
			a_movie.CreateArray(&list);
			// a quest the browser has no entry of is one of the game's own workings, it has no page to lead to
			std::erase_if(quests, [](const RE::TESQuest* a_quest) { return LinkOf(a_quest) == 0; });
			const auto sent = a_whole ? kWholeList : kThingsShown;
			for (std::size_t i = 0; i < quests.size() && i < sent; ++i) {
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				one.SetMember("name", NameOf(*quests[i]).c_str());
				one.SetMember("id", static_cast<double>(LinkOf(quests[i])));
				one.SetMember("state", static_cast<double>(BrowserQuests::StateOf(*quests[i])));
				list.PushBack(one);
			}
			a_object.SetMember("quests", list);
			a_object.SetMember("questsMore", static_cast<double>(quests.size() > sent ? quests.size() - sent : 0));
			a_object.SetMember("questsShown", static_cast<double>(kThingsShown));
		}

		void FillQuest(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESQuest& a_quest, bool a_whole)
		{
			const auto& ties = BrowserQuests::TiesOf(a_quest);
			Names things;
			Names people;
			Names places;
			for (const auto* thing : ties.things) {
				things.Add(thing, 1);
			}
			for (const auto* person : ties.people) {
				people.Add(person, 1);
			}
			for (const auto reference : ties.references) {
				places.Add(Places::OfReference(reference), 1);
			}
			things.Put(a_movie, a_object, "things", kThingsShown, a_whole);
			people.Put(a_movie, a_object, "people", kThingsShown, a_whole);
			places.Put(a_movie, a_object, "places", kNamesShown, a_whole);
			BrowserQuestLinks::Fill(a_movie, a_object, a_quest, a_whole);
			a_object.SetMember("reading", Reading());
		}

		// the quests that give the thing, as a reward of its own or as one of a list the reward is picked from
		void PutRewardedBy(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESForm& a_item, bool a_whole)
		{
			Names quests;
			Names picked;
			for (const auto& [quest, list] : BrowserQuestLinks::RewardedBy(a_item)) {
				(list ? picked : quests).Add(quest, 1);
			}
			quests.Put(a_movie, a_object, "rewardOf", kNamesShown, a_whole);
			picked.Put(a_movie, a_object, "rewardPickedOf", kNamesShown, a_whole);
		}
	}

	RE::FormID LinkOf(const RE::TESForm* a_form)
	{
		const auto* entry = a_form ? BrowserIndex::Find(a_form->GetFormID()) : nullptr;
		return entry ? entry->form->GetFormID() : 0;
	}

	std::string NameOf(const RE::TESForm& a_form)
	{
		if (const auto* entry = BrowserIndex::Find(a_form.GetFormID())) {
			return entry->name;
		}
		std::string name = a_form.GetName() ? a_form.GetName() : "";
		if (name.starts_with('$')) {
			if (auto words = Translations::Game(name); !words.empty()) {
				return words;
			}
		}
		return name;
	}

	// The conditions as the game reads them. One joined to the next by "or" makes a group with it and every group has
	// to hold. A group that holds anyway says nothing, one is told when the page can tell each of its conditions, else
	// it's only counted
	std::vector<const RE::TESForm*> PutNeeds(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESCondition& a_conditions, const RE::BGSConstructibleObject* a_recipe,
		std::span<const RE::BGSPerk* const> a_given)
	{
		RE::GFxValue groups;
		a_movie.CreateArray(&groups);
		std::uint32_t more = 0;
		std::vector<std::optional<Need>> group;
		std::vector<const RE::TESForm*> perks;
		bool holds = false;
		for (const auto* condition = a_conditions.head; condition; condition = condition->next) {
			bool one = false;
			group.push_back(NeedOf(*condition, a_recipe, a_given, one));
			holds = holds || one;
			if (condition->data.flags.isOR && condition->next) {
				continue;
			}
			if (holds) {
				// the materials or an earlier rank say it already
			} else if (std::ranges::any_of(group, [](const auto& a_need) { return !a_need; })) {
				more++;
			} else {
				RE::GFxValue told;
				a_movie.CreateArray(&told);
				for (const auto& need : group) {
					RE::GFxValue one;
					a_movie.CreateObject(&one);
					one.SetMember("kind", need->kind);
					one.SetMember("name", need->name.c_str());
					one.SetMember("number", need->number);
					if (const auto link = LinkOf(need->form); link != 0) {
						one.SetMember("id", static_cast<double>(link));
					}
					if (std::string_view(need->kind) == "perk") {
						perks.push_back(need->form);
					}
					told.PushBack(one);
				}
				groups.PushBack(told);
			}
			group.clear();
			holds = false;
		}
		a_object.SetMember("needs", groups);
		a_object.SetMember("more", static_cast<double>(more));
		return perks;
	}

	std::string BenchOf(const RE::BGSConstructibleObject& a_recipe)
	{
		return BenchOf(Read(), a_recipe);
	}

	std::vector<RE::FormID> LastingOf(const RE::TESForm& a_base)
	{
		Read();
		const auto found = spread.placed.find(&a_base);
		return found != spread.placed.end() ? found->second : std::vector<RE::FormID>{};
	}

	void Fill(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESForm& a_item, bool a_whole)
	{
		auto& known = Read();
		a_movie.CreateObject(&a_object);
		a_object.SetMember("whole", a_whole);
		if (const auto text = ItemTexts::Of(a_item.GetFormID()); !text.empty()) {
			a_object.SetMember("description", std::string(text).c_str());
		}
		if (const auto* quest = a_item.As<RE::TESQuest>()) {
			FillQuest(a_movie, a_object, *quest, a_whole);
			if (Blockers::TargetsHere(*quest)) {
				Blockers::Put(a_movie, a_object);
			}
			return;
		}
		if (const auto* place = a_item.As<RE::BGSLocation>()) {
			BrowserPlaces::Fill(a_movie, a_object, *place, a_whole);
			BrowserQuestLinks::FillPlace(a_movie, a_object, *place, a_whole);
			if (BrowserPlaces::PlayerRightIn(*place)) {
				Blockers::Put(a_movie, a_object);
			}
			a_object.SetMember("reading", Reading());
			return;
		}
		if (const auto* perk = a_item.Is(RE::FormType::Perk) ? BrowserPerks::Of(a_item) : nullptr) {
			BrowserPerks::Fill(a_movie, a_object, *perk, a_whole);
			a_object.SetMember("reading", Reading());
			return;
		}
		if (const auto* entry = BrowserIndex::Find(a_item.GetFormID()); entry && entry->unit != BrowserIndex::kNone) {
			const auto& people = BrowserIndex::Units()[entry->unit];
			FillLiving(a_movie, a_object, known, people, a_whole);
			PutNow(a_movie, a_object, *entry, a_whole);
			const std::vector<const RE::TESForm*> forms(people.begin(), people.end());
			PutQuests(a_movie, a_object, forms, a_whole);
			PutRelations(a_movie, a_object, people);
			PutToward(a_movie, a_object, *entry, people);
			return;
		}
		const RE::TESForm* itself = &a_item;
		PutQuests(a_movie, a_object, std::span<const RE::TESForm* const>(&itself, 1), a_whole);
		const auto givers = Givers(known.parents, a_item);

		Ways ways;
		std::set<const RE::TESForm*> common;
		std::vector<const BrowserHandouts::Gift*> told;
		for (const auto& [giver, level] : givers) {
			const bool own = Own(*giver, a_item);
			if (own) {
				if (const auto grown = spread.grows.find(giver); grown != spread.grows.end()) {
					for (const auto* plant : grown->second) {
						ways.harvested.Add(plant, level);
					}
				}
			}
			// a mod's line names its people or chests itself, so it counts through a big list too. SPID's death items
			// of Abyss are one list of every tome
			AddHandouts(known, *giver, level, ways, told);
			const auto held = known.holders.find(giver);
			if (held == known.holders.end()) {
				continue;
			}
			for (const auto& holder : held->second) {
				if (holder.way == Way::kDropped) {
					ways.dropped.Add(holder.by, level);
					continue;
				}
				if (const auto vendor = known.vendorChests.find(holder.by); vendor != known.vendorChests.end()) {
					AddSellers(known, vendor->second, level, ways.sold);
					continue;
				}
				if (!own) {
					common.insert(holder.by);
					continue;
				}
				if (holder.way == Way::kCarried) {
					ways.carried.Add(holder.by, level);
					continue;
				}
				AddChest(*holder.by, level, ways.chests);
			}
		}
		ways.sold.Put(a_movie, a_object, "sold", kNamesShown, a_whole);
		ways.carried.Put(a_movie, a_object, "carried", kNamesShown, a_whole);
		ways.dropped.Put(a_movie, a_object, "dropped", kNamesShown, a_whole);
		ways.harvested.Put(a_movie, a_object, "harvested", kNamesShown, a_whole);
		ways.chests.Put(a_movie, a_object, "chests", kPlacesShown, a_whole);
		ways.swappedFor.Put(a_movie, a_object, "swappedFor", kNamesShown, a_whole);
		ways.swappedAt.Put(a_movie, a_object, "swappedAt", kPlacesShown, a_whole);
		PutRewardedBy(a_movie, a_object, a_item, a_whole);
		a_object.SetMember("common", static_cast<double>(common.size()));
		PutTold(a_movie, a_object, told);

		Names swappedBy;
		for (const auto* swap : BrowserHandouts::SwapsFrom(a_item)) {
			std::unordered_set<const RE::TESForm*> seen;
			AddLeaves(swap->to, swappedBy, seen, swap->tag);
		}
		swappedBy.Put(a_movie, a_object, "swappedBy", kNamesShown, a_whole);

		Names lies;
		ForEachPlace(a_item, [&](Where a_where) { lies.Add(std::move(a_where), 1); });
		lies.Put(a_movie, a_object, "lies", kPlacesShown, a_whole);
		a_object.SetMember("reading", Reading());

		PutRecipes(a_movie, a_object, known, a_item, a_whole);
		const auto lists = known.parents.find(&a_item);
		a_object.SetMember("lists", static_cast<double>(lists != known.parents.end() ? lists->second.size() : 0));
	}
}
