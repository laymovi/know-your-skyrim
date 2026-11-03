#include "PCH.h"

#include "Browser.h"

#include "BrowserActors.h"
#include "BrowserFavorites.h"
#include "BrowserIndex.h"
#include "BrowserMap.h"
#include "BrowserPerks.h"
#include "BrowserPlaces.h"
#include "PointerFirst.h"
#include "BrowserQuestLinks.h"
#include "BrowserQuests.h"
#include "BrowserSources.h"
#include "BrowserTextures.h"
#include "Functions.h"
#include "Knowledge.h"
#include "People.h"
#include "Places.h"

namespace Browser
{
	namespace
	{
		using Functions::Flag;
		using Functions::Number;
		using Functions::Params;
		using Functions::Text;
		using BrowserIndex::Term;

		constexpr std::size_t kMostRows = 200;
		// the mod picked is one that only changes textures
		constexpr std::string_view kTextures = "textures:";
		// a chip of the skill trainers train, by its actor value
		constexpr std::string_view kSkill = "skill:";
		// the chips of how far the player is with a quest, the search asks the game for it as it runs
		const std::unordered_map<std::string, BrowserIndex::QuestProgress> kProgress{ { "unfinishedQuests", BrowserIndex::QuestProgress::kUnfinished },
			{ "activeQuests", BrowserIndex::QuestProgress::kGoing }, { "doneQuests", BrowserIndex::QuestProgress::kDone } };
		// the page shows the first few and opens the rest on a press
		constexpr std::size_t kVariantsShown = 200;
		constexpr std::size_t kRacesShown = 3;
		// a level the game keeps as a share of the player's one is that share times a thousand
		constexpr double kLevelShare = 1000.0;

		RE::GFxValue Member(const RE::GFxValue& a_object, const char* a_name)
		{
			RE::GFxValue value;
			if (a_object.IsObject()) {
				a_object.GetMember(a_name, &value);
			}
			return value;
		}

		Term::Kind KindNamed(std::string_view a_name)
		{
			if (a_name == "mod") {
				return Term::Kind::kMod;
			}
			if (a_name == "keyword") {
				return Term::Kind::kKeyword;
			}
			if (a_name == "effect") {
				return Term::Kind::kEffect;
			}
			return Term::Kind::kName;
		}

		// The browser splits what the player typed into terms, each [kind, words, left out], and names the one plugin
		// the player picked, if any
		BrowserIndex::Query QueryOf(const RE::GFxValue& a_told)
		{
			BrowserIndex::Query query;
			const auto family = Text(Member(a_told, "family"));
			query.family = BrowserIndex::FamilyNamed(family);
			query.favorites = family == "favorites";
			const auto within = Text(Member(a_told, "within"));
			query.within = within == "texts" ? BrowserIndex::Within::kTexts : (within == "both" ? BrowserIndex::Within::kBoth : BrowserIndex::Within::kNames);
			const auto chips = Member(a_told, "chips");
			for (std::uint32_t i = 0; chips.IsArray() && i < chips.GetArraySize(); ++i) {
				RE::GFxValue key;
				chips.GetElement(i, &key);
				const auto named = Text(key);
				if (named.starts_with(kSkill)) {
					query.trains = static_cast<RE::ActorValue>(std::atoi(named.c_str() + kSkill.size()));
					continue;
				}
				if (const auto progress = kProgress.find(named); progress != kProgress.end()) {
					query.progress = progress->second;
					continue;
				}
				if (const auto tree = BrowserPerks::TreeNamed(named)) {
					query.tree = tree;
					continue;
				}
				query.chips |= BrowserIndex::ChipNamed(named);
			}
			const auto terms = Member(a_told, "terms");
			for (std::uint32_t i = 0; terms.IsArray() && i < terms.GetArraySize(); ++i) {
				RE::GFxValue term;
				terms.GetElement(i, &term);
				if (!term.IsArray()) {
					continue;
				}
				RE::GFxValue kind;
				RE::GFxValue words;
				RE::GFxValue left;
				term.GetElement(0, &kind);
				term.GetElement(1, &words);
				term.GetElement(2, &left);
				query.terms.push_back({ KindNamed(Text(kind)), BrowserIndex::Lowered(Text(words)), Flag(left) });
			}
			if (const auto mod = Text(Member(a_told, "mod")); mod.starts_with(kTextures)) {
				// a mod that only changes textures lists what it changes once every model is read
				BrowserTextures::WantAll();
				query.textures = BrowserTextures::ModNamed(std::string_view(mod).substr(kTextures.size())).value_or(0xFFFF);
			} else if (!mod.empty()) {
				// a plugin the load order no longer has finds nothing
				query.file = BrowserIndex::FileNamed(mod).value_or(0xFFFF);
			}
			return query;
		}

		// The nearest place around a place that has an entry of its own and another name. A location with no name
		// between is skipped, and so is the settlement a house of the same name stands in (Half-Moon Mill in Half-Moon
		// Mill says nothing)
		const BrowserIndex::Item* AroundOf(const RE::BGSLocation& a_place)
		{
			const auto* own = BrowserIndex::Find(a_place.GetFormID());
			const auto* around = a_place.parentLoc;
			for (int depth = 0; around && depth < 16; ++depth, around = around->parentLoc) {
				const auto* entry = BrowserIndex::Find(around->GetFormID());
				if (entry && (!own || entry->lowered != own->lowered)) {
					return entry;
				}
			}
			return nullptr;
		}

		void PutRow(RE::GFxMovie& a_movie, RE::GFxValue& a_rows, const BrowserIndex::Item& a_item)
		{
			RE::GFxValue row;
			a_movie.CreateObject(&row);
			row.SetMember("id", static_cast<double>(a_item.form->GetFormID()));
			row.SetMember("name", a_item.name.c_str());
			row.SetMember("mod", std::string(BrowserIndex::Stem(BrowserIndex::FileName(a_item.file))).c_str());
			row.SetMember("icon", a_item.icon);
			row.SetMember("family", std::string(BrowserIndex::FamilyName(a_item.family)).c_str());
			row.SetMember("variants", static_cast<double>(a_item.variants));
			row.SetMember("variant", a_item.original != BrowserIndex::kNone);
			if (a_item.guard != BrowserIndex::Guard::kNone) {
				row.SetMember("guard", static_cast<double>(a_item.guard));
			}
			if (const auto* quest = a_item.form->As<RE::TESQuest>()) {
				row.SetMember("state", static_cast<double>(BrowserQuests::StateOf(*quest)));
			}
			// a perk's row says how far the player has it
			if (const auto* perk = a_item.family == BrowserIndex::Family::kPerks ? BrowserPerks::Of(*a_item.form) : nullptr) {
				row.SetMember("rank", static_cast<double>(BrowserPerks::RankOf(*perk)));
				row.SetMember("ranks", static_cast<double>(perk->ranks.size()));
			}
			// things of one name are told apart by a number: the damage of a weapon, the armor of a piece, the value of the
			// rest. Places of one name by the place they lie in
			if (const auto* weapon = a_item.form->As<RE::TESObjectWEAP>()) {
				row.SetMember("number", static_cast<double>(weapon->attackDamage));
				row.SetMember("numberKind", "damage");
			} else if (const auto* ammo = a_item.form->As<RE::TESAmmo>()) {
				row.SetMember("number", static_cast<double>(ammo->GetRuntimeData().data.damage));
				row.SetMember("numberKind", "damage");
			} else if (const auto* armor = a_item.form->As<RE::TESObjectARMO>()) {
				row.SetMember("number", static_cast<double>(armor->armorRating) / 100.0);
				row.SetMember("numberKind", "armor");
			} else if (const auto value = a_item.unit == BrowserIndex::kNone ? a_item.form->GetGoldValue() : -1; value >= 0) {
				row.SetMember("number", static_cast<double>(value));
				row.SetMember("numberKind", "value");
			}
			if (const auto* place = a_item.form->As<RE::BGSLocation>()) {
				// a hold and its city often share a name, the kind tells them apart
				for (const auto key : BrowserIndex::ChipKeys(a_item.chips)) {
					row.SetMember("placeKind", std::string(key).c_str());
				}
				if (const auto* around = AroundOf(*place)) {
					row.SetMember("around", around->name.c_str());
				}
			}
			a_rows.PushBack(row);
		}

		// BrowserFind(query) with query {family, chips, terms, mod, first, count} gives one page of rows, how many there
		// are in all and how many the words leave in each category
		struct Find
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie || a_params.argCount != 1) {
					return;
				}
				auto& movie = *a_params.movie;
				const auto& told = a_params.args[0];
				const auto& found = BrowserIndex::Search(QueryOf(told));
				// the mods that only change textures are read on their own thread from the first look on
				BrowserTextures::Start();
				const auto& items = BrowserIndex::Items();
				const auto total = found.items.size();
				const auto first = std::min(static_cast<std::size_t>(std::max(Number(Member(told, "first")), 0.0F)), total);
				const auto count = std::min(static_cast<std::size_t>(std::max(Number(Member(told, "count")), 0.0F)), kMostRows);

				auto& page = *a_params.retVal;
				movie.CreateObject(&page);
				page.SetMember("total", static_cast<double>(total));
				page.SetMember("first", static_cast<double>(first));
				page.SetMember("all", static_cast<double>(found.all));
				if (found.readingTexts) {
					page.SetMember("readingTexts", true);
				}
				if (found.reading) {
					const auto [read, of] = BrowserTextures::Progress();
					page.SetMember("reading", true);
					page.SetMember("read", static_cast<double>(read));
					page.SetMember("readOf", static_cast<double>(of));
				}
				RE::GFxValue families;
				movie.CreateObject(&families);
				for (std::size_t i = 0; i < BrowserIndex::kFamilies; ++i) {
					families.SetMember(std::string(BrowserIndex::FamilyName(static_cast<BrowserIndex::Family>(i))).c_str(), static_cast<double>(found.perFamily[i]));
				}
				families.SetMember("favorites", static_cast<double>(found.favorites));
				page.SetMember("families", families);
				RE::GFxValue rows;
				movie.CreateArray(&rows);
				for (auto at = first; at < total && at < first + count; ++at) {
					PutRow(movie, rows, items[found.items[at]]);
				}
				page.SetMember("rows", rows);
			}
		};

		// BrowserTrainerSkills() is [{skill, name, count}] of every skill someone of the load order trains, in the game's
		// order of skills (warrior, thief, mage), the name in the game's words, count the entries that train it
		struct TrainerSkills
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie) {
					return;
				}
				const auto trainers = BrowserIndex::ChipNamed("trainers");
				std::map<RE::ActorValue, std::pair<std::string, std::uint32_t>> skills;
				for (const auto& item : BrowserIndex::Items()) {
					if ((item.chips & trainers) == 0) {
						continue;
					}
					if (const auto training = BrowserIndex::TrainingOf(item)) {
						auto& one = skills[training->value];
						one.first = training->skill;
						one.second++;
					}
				}
				auto& movie = *a_params.movie;
				movie.CreateArray(a_params.retVal);
				for (const auto& [value, named] : skills) {
					RE::GFxValue one;
					movie.CreateObject(&one);
					one.SetMember("skill", static_cast<double>(value));
					one.SetMember("name", named.first.c_str());
					one.SetMember("count", static_cast<double>(named.second));
					a_params.retVal->PushBack(one);
				}
			}
		};

		// BrowserPerkTrees() is [{key, name}] of the trees the game names, the name of a skill in the game's words for the
		// chips of Perks. Other has none
		struct PerkTrees
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie) {
					return;
				}
				auto& movie = *a_params.movie;
				movie.CreateArray(a_params.retVal);
				for (std::uint8_t tree = 0; tree < BrowserPerks::kTrees; ++tree) {
					if (const auto name = BrowserPerks::TreeName(tree); !name.empty()) {
						RE::GFxValue one;
						movie.CreateObject(&one);
						one.SetMember("key", std::string(BrowserPerks::TreeKey(tree)).c_str());
						one.SetMember("name", std::string(name).c_str());
						a_params.retVal->PushBack(one);
					}
				}
			}
		};

		RE::GFxValue Strings(RE::GFxMovie& a_movie, const std::vector<std::string>& a_strings)
		{
			RE::GFxValue list;
			a_movie.CreateArray(&list);
			for (const auto& one : a_strings) {
				list.PushBack(RE::GFxValue(one.c_str()));
			}
			return list;
		}

		RE::GFxValue Link(RE::GFxMovie& a_movie, const BrowserIndex::Item& a_item)
		{
			RE::GFxValue link;
			a_movie.CreateObject(&link);
			link.SetMember("name", a_item.name.c_str());
			link.SetMember("id", static_cast<double>(a_item.form->GetFormID()));
			return link;
		}

		void PutNumbers(RE::GFxValue& a_details, const RE::TESForm& a_form)
		{
			if (const auto* weight = skyrim_cast<const RE::TESWeightForm*>(&a_form)) {
				a_details.SetMember("weight", weight->weight);
			}
			// the game's own price, because a potion or food keeps none and the game counts it from the effects
			if (const auto value = a_form.GetGoldValue(); value >= 0) {
				a_details.SetMember("value", static_cast<double>(value));
			}
			if (const auto* weapon = a_form.As<RE::TESObjectWEAP>()) {
				a_details.SetMember("damage", static_cast<double>(weapon->attackDamage));
			} else if (const auto* ammo = a_form.As<RE::TESAmmo>()) {
				a_details.SetMember("damage", ammo->GetRuntimeData().data.damage);
			} else if (const auto* armor = a_form.As<RE::TESObjectARMO>()) {
				a_details.SetMember("armor", static_cast<double>(armor->armorRating) / 100.0);
				a_details.SetMember("partMask", static_cast<double>(armor->bipedModelData.bipedObjectSlots.underlying()));
			}
		}

		// the original of a variant, or the variants of an original
		void PutVariants(RE::GFxMovie& a_movie, RE::GFxValue& a_details, const BrowserIndex::Item& a_item)
		{
			const auto& items = BrowserIndex::Items();
			if (a_item.original != BrowserIndex::kNone) {
				a_details.SetMember("original", Link(a_movie, items[a_item.original]));
			}
			RE::GFxValue variants;
			a_movie.CreateArray(&variants);
			const auto all = BrowserIndex::VariantsOf(BrowserIndex::PlaceOf(a_item));
			for (std::size_t i = 0; i < all.size() && i < kVariantsShown; ++i) {
				variants.PushBack(Link(a_movie, items[all[i]]));
			}
			a_details.SetMember("variants", variants);
			a_details.SetMember("variantsMore", static_cast<double>(all.size() > kVariantsShown ? all.size() - kVariantsShown : 0));
		}

		// What the people of one name are: their races, the levels they come at, and what marks them out. A record
		// that takes its stats from a template has the level of the template, a list of levels, and tells none. What
		// keeps them alive is what keeps every record alive, the records of a name kept alive only in part are counted
		void PutLiving(RE::GFxMovie& a_movie, RE::GFxValue& a_details, const BrowserIndex::Item& a_item)
		{
			const auto& people = BrowserIndex::Units()[a_item.unit];
			using Use = RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG;
			std::vector<std::string> races;
			double lowest = 0.0;
			double highest = 0.0;
			double shareLow = 0.0;
			double shareHigh = 0.0;
			double followFrom = 0.0;
			double followTo = 0.0;
			bool fixed = false;
			bool follows = false;
			bool unique = false;
			std::size_t kept = 0;
			for (const auto* person : people) {
				const auto* raceOf = People::RaceOf(*person);
				const char* race = raceOf ? raceOf->GetName() : nullptr;
				if (race && *race && races.size() < kRacesShown && std::ranges::find(races, race) == races.end()) {
					races.emplace_back(race);
				}
				const auto& data = person->actorData;
				if (person->baseTemplateForm && data.templateUseFlags.any(Use::kStats)) {
					// the level of such a record is the template's
				} else if (person->HasPCLevelMult()) {
					const double share = data.level / kLevelShare;
					shareLow = follows ? std::min(shareLow, share) : share;
					shareHigh = follows ? std::max(shareHigh, share) : share;
					followFrom = follows ? std::min(followFrom, static_cast<double>(data.calcLevelMin)) : data.calcLevelMin;
					// no top is no limit, it stays none once one record has none
					followTo = follows && (followTo == 0.0 || data.calcLevelMax == 0) ? 0.0 : std::max(followTo, static_cast<double>(data.calcLevelMax));
					follows = true;
				} else {
					lowest = fixed ? std::min(lowest, static_cast<double>(data.level)) : data.level;
					highest = fixed ? std::max(highest, static_cast<double>(data.level)) : data.level;
					fixed = true;
				}
				unique = unique || person->IsUnique();
				kept += person->IsEssential() || person->IsProtected() ? 1 : 0;
			}
			a_details.SetMember("races", Strings(a_movie, races));
			a_details.SetMember("records", static_cast<double>(people.size()));
			if (fixed) {
				a_details.SetMember("levelLow", lowest);
				a_details.SetMember("levelHigh", highest);
			}
			if (follows) {
				a_details.SetMember("shareLow", shareLow);
				a_details.SetMember("shareHigh", shareHigh);
				a_details.SetMember("followFrom", followFrom);
				a_details.SetMember("followTo", followTo);
			}
			a_details.SetMember("unique", unique && people.size() == 1);
			a_details.SetMember("guard", static_cast<double>(a_item.guard));
			a_details.SetMember("guardedRecords", static_cast<double>(a_item.guard == BrowserIndex::Guard::kNone ? kept : 0));
			// the index knows who trades by the factions each record has, a template's when the record takes them from it
			a_details.SetMember("merchant", (a_item.chips & BrowserIndex::ChipNamed("merchants")) != 0);
			if (const auto training = BrowserIndex::TrainingOf(a_item)) {
				a_details.SetMember("trains", training->skill.c_str());
				a_details.SetMember("trainsTo", static_cast<double>(training->upTo));
			}
			// how many of it the game holds now, alive or dead, their things can be looked through
			a_details.SetMember("about", static_cast<double>(BrowserActors::Of(a_item).size()));
		}

		void PutQuest(RE::GFxMovie& a_movie, RE::GFxValue& a_details, RE::TESQuest& a_quest)
		{
			a_details.SetMember("state", static_cast<double>(BrowserQuests::StateOf(a_quest)));
			a_details.SetMember("stage", static_cast<double>(a_quest.currentStage));
			if (const auto stage = a_quest.IsRunning() ? BrowserQuestLinks::BeginningStage(a_quest) : std::nullopt) {
				a_details.SetMember("beginsAt", static_cast<double>(*stage));
			}
			RE::GFxValue objectives;
			a_movie.CreateArray(&objectives);
			for (const auto& objective : BrowserQuests::ObjectivesOf(a_quest)) {
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				one.SetMember("text", objective.text.c_str());
				one.SetMember("state", static_cast<double>(objective.state));
				objectives.PushBack(one);
			}
			a_details.SetMember("objectives", objectives);
		}

		void PutPlace(RE::GFxMovie& a_movie, RE::GFxValue& a_details, const RE::BGSLocation& a_place)
		{
			if (const auto* around = AroundOf(a_place)) {
				a_details.SetMember("around", Link(a_movie, *around));
			}
			// the map shows the player's world only, a place of another world says which instead
			if (const auto marker = Places::MarkerOf(&a_place); marker != 0) {
				if (const auto world = BrowserMap::OtherWorld(marker); !world.empty()) {
					a_details.SetMember("world", world.c_str());
				} else {
					a_details.SetMember("marker", static_cast<double>(marker));
				}
			}
			a_details.SetMember("here", BrowserPlaces::PlayerIn(a_place));
		}

		void PutPerk(RE::GFxValue& a_details, const BrowserPerks::Perk& a_perk)
		{
			a_details.SetMember("ranks", static_cast<double>(a_perk.ranks.size()));
			a_details.SetMember("rank", static_cast<double>(BrowserPerks::RankOf(a_perk)));
		}

		// BrowserItem(formId) is what the details of a row show: its numbers, the plugin it comes from and the ones
		// that change it after, its keywords and effects, its original or variants. A living thing tells its races,
		// levels and marks instead of numbers and effects, a quest how far the player is with it, a perk its ranks and
		// the one the player has
		struct Item
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie || a_params.argCount != 1) {
					return;
				}
				const auto* item = BrowserIndex::Find(static_cast<RE::FormID>(a_params.args[0].GetNumber()));
				if (!item) {
					return;
				}
				auto& movie = *a_params.movie;
				auto& details = *a_params.retVal;
				const auto& form = *item->form;
				movie.CreateObject(&details);
				details.SetMember("id", static_cast<double>(form.GetFormID()));
				details.SetMember("name", item->name.c_str());
				details.SetMember("icon", item->icon);
				details.SetMember("family", std::string(BrowserIndex::FamilyName(item->family)).c_str());
				details.SetMember("mod", std::string(BrowserIndex::Stem(BrowserIndex::FileName(item->file))).c_str());
				details.SetMember("living", item->unit != BrowserIndex::kNone);
				details.SetMember("quest", item->family == BrowserIndex::Family::kQuests);
				details.SetMember("pseudo", item->family == BrowserIndex::Family::kPseudo);
				details.SetMember("place", item->family == BrowserIndex::Family::kPlaces);
				details.SetMember("perk", item->family == BrowserIndex::Family::kPerks);
				details.SetMember("favorite", BrowserFavorites::Has(form.GetFormID()));
				std::vector<std::string> chips;
				for (const auto key : BrowserIndex::ChipKeys(item->chips)) {
					chips.emplace_back(key);
				}
				if (item->family == BrowserIndex::Family::kPerks) {
					chips.emplace_back(BrowserPerks::TreeKey(item->tree));
				}
				details.SetMember("chips", Strings(movie, chips));
				std::vector<std::string> edits;
				const auto* files = form.sourceFiles.array;
				for (std::uint32_t i = 1; files && i < files->size(); ++i) {
					if (const auto* file = (*files)[i]) {
						edits.emplace_back(BrowserIndex::Stem(file->GetFilename()));
					}
				}
				details.SetMember("edits", Strings(movie, edits));
				// the mods that only change textures and change one of its own
				std::vector<std::string> retextured;
				const auto texturing = BrowserTextures::Mods();
				for (const auto mod : BrowserTextures::ChangersOf(*item)) {
					retextured.push_back(texturing[mod].name);
				}
				details.SetMember("retextured", Strings(movie, retextured));
				std::vector<std::string> keywords;
				const RE::TESForm* keywordsOf = &form;
				if (item->unit != BrowserIndex::kNone) {
					PutLiving(movie, details, *item);
					keywordsOf = form.As<RE::TESNPC>() ? People::RaceOf(*form.As<RE::TESNPC>()) : nullptr;
				} else if (auto* quest = item->form->As<RE::TESQuest>()) {
					PutQuest(movie, details, *quest);
				} else if (const auto* place = item->form->As<RE::BGSLocation>()) {
					PutPlace(movie, details, *place);
				} else if (const auto* perk = item->family == BrowserIndex::Family::kPerks ? BrowserPerks::Of(form) : nullptr) {
					PutPerk(details, *perk);
				} else {
					PutNumbers(details, form);
					PutVariants(movie, details, *item);
				}
				if (const auto* keywordForm = keywordsOf ? keywordsOf->As<RE::BGSKeywordForm>() : nullptr) {
					for (const auto* keyword : keywordForm->GetKeywords()) {
						// a mod can list one keyword twice
						if (keyword && !keyword->formEditorID.empty() && std::ranges::find(keywords, std::string(keyword->formEditorID.c_str())) == keywords.end()) {
							keywords.emplace_back(keyword->formEditorID.c_str());
						}
					}
				}
				details.SetMember("keywords", Strings(movie, keywords));
				RE::GFxValue effects;
				movie.CreateArray(&effects);
				for (const auto& effect : BrowserIndex::Effects(*item)) {
					RE::GFxValue one;
					movie.CreateObject(&one);
					one.SetMember("name", effect.name.c_str());
					one.SetMember("words", effect.words.c_str());
					one.SetMember("known", effect.known);
					one.SetMember("conditional", effect.conditional);
					effects.PushBack(one);
				}
				details.SetMember("effects", effects);
			}
		};

		// BrowserSources(formId, whole) is where the item can be got, or what a living thing carries and where it stands,
		// or what a quest ties to. The first ask reads who holds what, once. Whole sends every list up to a cap
		struct Sources
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie || a_params.argCount < 1) {
					return;
				}
				if (const auto* item = BrowserIndex::Find(static_cast<RE::FormID>(a_params.args[0].GetNumber()))) {
					BrowserSources::Fill(*a_params.movie, *a_params.retVal, *item->form, a_params.argCount > 1 && Flag(a_params.args[1]));
				}
			}
		};

		// BrowserMods(words) is every plugin whose file name holds the words, [{name, shown, items, living, quests, places,
		// perks, pseudo}], the ones that give the most first. The name is the file the query asks for, shown is it without .esp
		struct Mods
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie || a_params.argCount != 1) {
					return;
				}
				auto& movie = *a_params.movie;
				movie.CreateArray(a_params.retVal);
				for (const auto& file : BrowserIndex::Files(Text(a_params.args[0]))) {
					RE::GFxValue one;
					movie.CreateObject(&one);
					one.SetMember("name", std::string(file.name).c_str());
					one.SetMember("items", static_cast<double>(file.items));
					one.SetMember("living", static_cast<double>(file.living));
					one.SetMember("quests", static_cast<double>(file.quests));
					one.SetMember("pseudo", static_cast<double>(file.pseudo));
					one.SetMember("places", static_cast<double>(file.places));
					one.SetMember("perks", static_cast<double>(file.perks));
					a_params.retVal->PushBack(one);
				}
				// the mods that only change textures after the plugins, named "textures:<mod>" for the query
				const auto words = BrowserIndex::Lowered(Text(a_params.args[0]));
				const auto texturing = BrowserTextures::Mods();
				for (std::size_t i = 0; i < texturing.size(); ++i) {
					if (BrowserIndex::Lowered(texturing[i].name).find(words) == std::wstring::npos) {
						continue;
					}
					RE::GFxValue one;
					movie.CreateObject(&one);
					one.SetMember("name", std::format("{}{}", kTextures, texturing[i].name).c_str());
					one.SetMember("textures", static_cast<double>(texturing[i].textures));
					one.SetMember("from", std::string(texturing[i].from).c_str());
					if (const auto changed = BrowserTextures::ChangedBy(static_cast<std::uint16_t>(i))) {
						one.SetMember("changes", static_cast<double>(*changed));
					}
					a_params.retVal->PushBack(one);
				}
			}
		};

		// BrowserFavorite(formId, on) puts the star on an entry or takes it off and tells whether it has one
		struct Favorite
		{
			static void Run(Params& a_params)
			{
				const auto* item = a_params.argCount == 2 && a_params.args[0].IsNumber() ? BrowserIndex::Find(static_cast<RE::FormID>(a_params.args[0].GetNumber())) : nullptr;
				if (item) {
					BrowserFavorites::Set(item->form->GetFormID(), Flag(a_params.args[1]));
				}
				if (a_params.retVal) {
					*a_params.retVal = item && BrowserFavorites::Has(item->form->GetFormID());
				}
			}
		};

		// BrowserMap(markerId) closes the menu and opens the world map at the marker, false when the map can't show it
		struct Map
		{
			static void Run(Params& a_params)
			{
				const bool shown = a_params.argCount == 1 && a_params.args[0].IsNumber() && BrowserMap::Show(static_cast<RE::FormID>(a_params.args[0].GetNumber()));
				if (a_params.retVal) {
					*a_params.retVal = shown;
				}
			}
		};

		// BrowserMask(formId) is nothing for an entry the character met, and for every entry while the progression mode is
		// off. One not met yet is told as {family, chips, around}: the page says what it is in place of its name
		struct Mask
		{
			static void Run(Params& a_params)
			{
				const auto* item = a_params.argCount == 1 && a_params.args[0].IsNumber() ? BrowserIndex::Find(static_cast<RE::FormID>(a_params.args[0].GetNumber())) : nullptr;
				if (!item || !a_params.retVal || !a_params.movie || Knowledge::Known(*item)) {
					return;
				}
				auto& told = *a_params.retVal;
				a_params.movie->CreateObject(&told);
				told.SetMember("family", std::string(BrowserIndex::FamilyName(item->family)).c_str());
				RE::GFxValue chips;
				a_params.movie->CreateArray(&chips);
				for (const auto key : BrowserIndex::ChipKeys(item->chips)) {
					chips.PushBack(RE::GFxValue(std::string(key).c_str()));
				}
				told.SetMember("chips", chips);
				const auto* place = item->form->As<RE::BGSLocation>();
				if (const auto* around = place ? AroundOf(*place) : nullptr) {
					told.SetMember("around", around->name.c_str());
				}
			}
		};

		// BrowserHere() is the id of the entry of the place the player is in, 0 for none
		struct Here
		{
			static void Run(Params& a_params)
			{
				if (a_params.retVal) {
					a_params.retVal->SetNumber(static_cast<double>(BrowserPlaces::PlayerPlace()));
				}
			}
		};

		// BrowserArea(left, top, right, bottom) is where the browser lies now in shares of the screen
		struct Area
		{
			static void Run(Params& a_params)
			{
				if (a_params.argCount == 4) {
					PointerFirst::SetArea(static_cast<float>(a_params.args[0].GetNumber()), static_cast<float>(a_params.args[1].GetNumber()), static_cast<float>(a_params.args[2].GetNumber()),
						static_cast<float>(a_params.args[3].GetNumber()));
				}
			}
		};

		// The browser says when its field of words takes the keys. A script that closes the inventory meanwhile would
		// leave the game taking every key as text, so the close lets that go
		bool typing = false;

		class CloseWatch : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				if (a_event && !a_event->opening && a_event->menuName == RE::InventoryMenu::MENU_NAME && typing) {
					typing = false;
					RE::ControlMap::GetSingleton()->AllowTextInput(false);
					logger::info("the inventory closed while the item browser took text, the keys are the game's again");
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		// BrowserTyping(typing), the browser turns the game's text input on and off itself
		struct Typing
		{
			static void Run(Params& a_params)
			{
				static CloseWatch watch;
				static bool watching = false;
				if (a_params.argCount != 1) {
					return;
				}
				if (!watching) {
					RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(&watch);
					watching = true;
				}
				typing = Flag(a_params.args[0]);
			}
		};

		// What the player left in the browser stays for the next opening of a menu while the game runs
		struct Kept
		{
			bool open = false;
			std::string words;
			std::string family;
			std::string chips;
			double first = 0.0;
			double selected = 0.0;
			std::string mod;
			std::string history;
			std::string lists;
			double page = 0.0;
			std::string within;
		};
		Kept kept;

		// BrowserKept() is an object with whether it was open, the words, the category, the chips, the first row,
		// the selected item, the plugin picked, the chain of items looked at before, the lists of the page opened and
		// how far the page was scrolled
		struct GetKept
		{
			static void Run(Params& a_params)
			{
				if (!a_params.retVal || !a_params.movie) {
					return;
				}
				auto& told = *a_params.retVal;
				a_params.movie->CreateObject(&told);
				told.SetMember("open", kept.open);
				told.SetMember("words", kept.words.c_str());
				told.SetMember("family", kept.family.c_str());
				told.SetMember("chips", kept.chips.c_str());
				told.SetMember("first", kept.first);
				told.SetMember("selected", kept.selected);
				told.SetMember("mod", kept.mod.c_str());
				told.SetMember("history", kept.history.c_str());
				told.SetMember("lists", kept.lists.c_str());
				told.SetMember("page", kept.page);
				told.SetMember("within", kept.within.c_str());
			}
		};

		// KeepBrowser(open, words, family, chips, first, selected, mod, history, lists, page, within), the chips, the ids of
		// the history and the keys of the lists joined by commas
		struct Keep
		{
			static void Run(Params& a_params)
			{
				if (a_params.argCount < 6) {
					return;
				}
				const auto& args = a_params.args;
				const auto count = a_params.argCount;
				const auto number = [&](std::uint32_t a_at) { return count > a_at && args[a_at].IsNumber() ? args[a_at].GetNumber() : 0.0; };
				const auto text = [&](std::uint32_t a_at) { return count > a_at ? Text(args[a_at]) : std::string(); };
				kept = { Flag(args[0]), Text(args[1]), Text(args[2]), Text(args[3]), number(4), number(5), text(6), text(7), text(8), number(9), text(10) };
			}
		};
	}

	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root)
	{
		Functions::Add<Find>(a_view, a_root, "BrowserFind");
		Functions::Add<TrainerSkills>(a_view, a_root, "BrowserTrainerSkills");
		Functions::Add<PerkTrees>(a_view, a_root, "BrowserPerkTrees");
		Functions::Add<Item>(a_view, a_root, "BrowserItem");
		Functions::Add<Sources>(a_view, a_root, "BrowserSources");
		Functions::Add<Mods>(a_view, a_root, "BrowserMods");
		Functions::Add<Map>(a_view, a_root, "BrowserMap");
		Functions::Add<Mask>(a_view, a_root, "BrowserMask");
		Functions::Add<Here>(a_view, a_root, "BrowserHere");
		Functions::Add<Area>(a_view, a_root, "BrowserArea");
		Functions::Add<Favorite>(a_view, a_root, "BrowserFavorite");
		Functions::Add<Typing>(a_view, a_root, "BrowserTyping");
		Functions::Add<GetKept>(a_view, a_root, "BrowserKept");
		Functions::Add<Keep>(a_view, a_root, "KeepBrowser");
	}
}
