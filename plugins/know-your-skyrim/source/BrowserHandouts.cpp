#include "PCH.h"

#include "BrowserHandouts.h"

#include <bit>
#include <fstream>
#include <map>
#include <thread>
#include <unordered_set>

#include "GameArchives.h"
#include "HandoutLines.h"
#include "Holders.h"

namespace BrowserHandouts
{
	namespace
	{
		using HandoutLines::Record;
		using Use = RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG;

		// the frameworks in the order of kinds below, each with its DLL and the part of the name of its files
		enum Kind : std::size_t
		{
			kSpid,
			kCid,
			kBos,
			kKinds
		};

		struct Framework
		{
			const char* name;
			const wchar_t* module;
			std::string_view mark;
		};

		constexpr std::array<Framework, kKinds> kFrameworks{ {
			{ "SPID", L"po3_SpellPerkItemDistributor.dll", "_DISTR" },
			{ "CID", L"ContainerItemDistributor.dll", "_CID" },
			{ "BOS", L"po3_BaseObjectSwapper.dll", "_SWAP" },
		} };

		// a template walk stops here, broken plugins can loop
		constexpr int kTemplateDepth = 8;
		// SPID counts the people of PotentialFollowerFaction as teammates
		constexpr RE::FormID kPotentialFollower = 0x5C84D;
		constexpr RE::FormID kPlayerBase = 0x7;

		struct File
		{
			std::string name;
			std::vector<HandoutLines::Line> lines;
		};

		// what the thread reads: the files of each framework the game loaded, in the order the frameworks read them
		struct Texts
		{
			std::array<bool, kKinds> loaded{};
			std::array<std::vector<File>, kKinds> files;
			std::atomic<bool> ready = false;
		};

		// the thread may still read when the game quits, so the texts are never freed
		Texts* texts = nullptr;

		// a filter of a SPID line is a form or a whole plugin
		struct Filter
		{
			const RE::TESForm* form = nullptr;
			const RE::TESFile* file = nullptr;
		};

		struct Rule
		{
			HandoutLines::Gift line;
			std::string file;
			const RE::TESForm* gives = nullptr;
			std::vector<Filter> all;
			std::vector<Filter> none;
			std::vector<Filter> match;
			// a thing, a loot list or an outfit, the rest SPID gives are spells, perks and such
			bool thing = false;
			// the lines a linked line goes with
			std::vector<std::size_t> parents;
			std::optional<std::vector<const RE::TESNPC*>> people;
		};

		struct Known
		{
			std::vector<Rule> rules;
			std::vector<Gift> gifts;
			std::vector<Stock> stocks;
			std::vector<Swap> swaps;
			std::unordered_map<const RE::TESForm*, std::vector<std::size_t>> giftsOf;
			std::unordered_map<const RE::TESForm*, std::vector<std::size_t>> stocksOf;
			std::unordered_map<const RE::TESForm*, std::vector<std::size_t>> stocksInto;
			std::unordered_map<const RE::TESForm*, std::vector<std::size_t>> swapsTo;
			std::unordered_map<const RE::TESForm*, std::vector<std::size_t>> swapsFrom;
		};

		std::optional<Known> known;

		void ReadTexts(Texts* a_texts)
		{
			const auto start = std::chrono::steady_clock::now();
			std::array<std::vector<std::filesystem::path>, kKinds> paths;
			std::error_code error;
			for (std::filesystem::directory_iterator at("Data", error), end; !error && at != end; at.increment(error)) {
				const auto& path = at->path();
				// as the frameworks look, an .ini right in Data with the mark anywhere in its name, the case kept
				if (path.extension() != ".ini") {
					continue;
				}
				const auto name = GameArchives::Narrow(path.filename());
				for (std::size_t kind = 0; kind < kKinds; ++kind) {
					if (a_texts->loaded[kind] && name.find(kFrameworks[kind].mark) != std::string::npos) {
						paths[kind].push_back(path);
					}
				}
			}
			std::size_t lines = 0;
			for (std::size_t kind = 0; kind < kKinds; ++kind) {
				std::ranges::sort(paths[kind]);
				for (const auto& path : paths[kind]) {
					std::ifstream stream(path, std::ios::binary);
					const std::string text{ std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
					auto& file = a_texts->files[kind].emplace_back(File{ GameArchives::Narrow(path.filename()), HandoutLines::ReadIni(text) });
					lines += file.lines.size();
				}
			}
			const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			logger::info("the hand out files: {} of SPID, {} of CID, {} of BOS, {} lines, read in {:.0f} ms", a_texts->files[kSpid].size(), a_texts->files[kCid].size(),
				a_texts->files[kBos].size(), lines, took);
			a_texts->ready = true;
		}

		const RE::TESForm* FormOf(const Record& a_record)
		{
			if (a_record.id && !a_record.plugin.empty()) {
				return RE::TESDataHandler::GetSingleton()->LookupForm(*a_record.id, a_record.plugin);
			}
			if (a_record.id) {
				return RE::TESForm::LookupByID(*a_record.id);
			}
			return a_record.editorID.empty() ? nullptr : RE::TESForm::LookupByEditorID(a_record.editorID);
		}

		// the number of a form, also of a reference whose cell isn't loaded, as BOS and CID take it
		RE::FormID IdOf(const Record& a_record)
		{
			if (a_record.id && !a_record.plugin.empty()) {
				return RE::TESDataHandler::GetSingleton()->LookupFormID(*a_record.id, a_record.plugin);
			}
			if (a_record.id) {
				return *a_record.id;
			}
			const auto* form = FormOf(a_record);
			return form ? form->GetFormID() : 0;
		}

		std::optional<Filter> FilterOf(const Record& a_record)
		{
			if (!a_record.id && a_record.editorID.empty()) {
				const auto* file = RE::TESDataHandler::GetSingleton()->LookupModByName(a_record.plugin);
				return file ? std::optional<Filter>(Filter{ nullptr, file }) : std::nullopt;
			}
			const auto* form = FormOf(a_record);
			return form ? std::optional<Filter>(Filter{ form, nullptr }) : std::nullopt;
		}

		// SPID drops a line whose forms aren't all there
		bool FiltersOf(const std::vector<Record>& a_records, std::vector<Filter>& a_filters)
		{
			for (const auto& record : a_records) {
				const auto filter = FilterOf(record);
				if (!filter) {
					return false;
				}
				a_filters.push_back(*filter);
			}
			return true;
		}

		bool Thing(const RE::TESForm& a_form)
		{
			return a_form.IsInventoryObject() || a_form.Is(RE::FormType::LeveledItem) || a_form.Is(RE::FormType::Outfit);
		}

		// a filter a person's record can't answer, like where the person stands or a single person in the world
		bool Unanswered(const Filter& a_filter)
		{
			return a_filter.form && (a_filter.form->Is(RE::FormType::Location) || a_filter.form->Is(RE::FormType::Reference) || a_filter.form->Is(RE::FormType::ActorCharacter));
		}

		void AddRules(Known& a_known, const File& a_file, std::size_t& a_dropped)
		{
			for (const auto& line : a_file.lines) {
				// SPID reads only the lines before the first section
				if (!line.section.empty()) {
					continue;
				}
				auto gift = HandoutLines::ReadGift(line.key, line.value);
				if (!gift) {
					continue;
				}
				Rule rule{ std::move(*gift), a_file.name };
				rule.gives = FormOf(rule.line.gives);
				if (!rule.gives || !FiltersOf(rule.line.formsAll, rule.all) || !FiltersOf(rule.line.formsNone, rule.none) || !FiltersOf(rule.line.formsMatch, rule.match)) {
					a_dropped++;
					continue;
				}
				const auto kind = std::string_view(rule.line.kind);
				rule.thing = (kind == "Item" || kind == "Outfit" || kind == "SleepOutfit" || kind == "Form") && Thing(*rule.gives);
				a_known.rules.push_back(std::move(rule));
			}
		}

		// a linked line goes with the lines that give one of its forms, the ones of its own file unless it's global
		void LinkRules(Known& a_known)
		{
			std::unordered_map<const RE::TESForm*, std::vector<std::size_t>> givers;
			for (std::size_t i = 0; i < a_known.rules.size(); ++i) {
				if (!a_known.rules[i].line.linked) {
					givers[a_known.rules[i].gives].push_back(i);
				}
			}
			for (auto& rule : a_known.rules) {
				if (!rule.line.linked) {
					continue;
				}
				for (const auto& filter : rule.match) {
					const auto found = givers.find(filter.form);
					if (found == givers.end()) {
						continue;
					}
					for (const auto at : found->second) {
						const auto& other = a_known.rules[at];
						if (other.line.death == rule.line.death && (rule.line.global || other.file == rule.file)) {
							rule.parents.push_back(at);
						}
					}
				}
			}
		}

		void AddGifts(Known& a_known)
		{
			for (std::size_t i = 0; i < a_known.rules.size(); ++i) {
				const auto& rule = a_known.rules[i];
				if (!rule.thing) {
					continue;
				}
				Gift gift;
				gift.gives = rule.gives;
				gift.death = rule.line.death;
				gift.tag = { kFrameworks[kSpid].name, rule.line.chance, false };
				gift.rule = i;
				gift.filters = rule.line.filters;
				const auto& line = rule.line;
				gift.everyone = !line.linked && line.wordsAll.empty() && line.wordsNone.empty() && line.wordsMatch.empty() && line.wordsPart.empty() && rule.all.empty() &&
				                rule.none.empty() && rule.match.empty();
				gift.told = gift.everyone || line.dead.value_or(false) || std::ranges::any_of(rule.all, Unanswered) || std::ranges::any_of(rule.match, Unanswered);
				const auto at = a_known.gifts.size();
				a_known.gifts.push_back(std::move(gift));
				a_known.giftsOf[rule.gives].push_back(at);
				// an outfit gives each of its pieces, a piece can be a loot list
				if (const auto* outfit = rule.gives->As<RE::BGSOutfit>()) {
					for (const auto* piece : outfit->outfitItems) {
						if (piece) {
							a_known.giftsOf[piece].push_back(at);
						}
					}
				}
			}
		}

		void AddStocks(Known& a_known, const File& a_file, std::size_t& a_dropped)
		{
			for (const auto& line : a_file.lines) {
				const auto stock = HandoutLines::ReadStock(line.key, line.value);
				if (!stock) {
					continue;
				}
				const auto* gives = FormOf(stock->gives);
				const auto* into = FormOf(stock->into);
				const auto* reference = into ? into->As<RE::TESObjectREFR>() : nullptr;
				const auto* base = reference ? reference->GetBaseObject() : into;
				if (!gives || !base) {
					a_dropped++;
					continue;
				}
				const auto at = a_known.stocks.size();
				a_known.stocks.push_back({ gives, base, reference ? reference->GetFormID() : 0, { kFrameworks[kCid].name, stock->chance, stock->somewhere } });
				a_known.stocksOf[gives].push_back(at);
				a_known.stocksInto[base].push_back(at);
			}
		}

		void AddSwaps(Known& a_known, const File& a_file, std::size_t& a_dropped)
		{
			for (const auto& line : a_file.lines) {
				const auto swap = HandoutLines::ReadSwap(line);
				if (!swap) {
					continue;
				}
				std::vector<const RE::TESForm*> to;
				for (const auto& record : swap->to) {
					if (const auto* form = FormOf(record)) {
						to.push_back(form);
					}
				}
				if (to.empty()) {
					a_dropped++;
					continue;
				}
				// one of several new bases is picked for each spot
				const float chance = swap->chance / static_cast<float>(to.size());
				for (const auto& record : swap->from) {
					// a reference stands in a cell that may not be loaded, it's kept by its number
					const auto id = swap->reference ? IdOf(record) : 0;
					const auto* from = swap->reference ? nullptr : FormOf(record);
					if (!from && id == 0) {
						a_dropped++;
						continue;
					}
					for (const auto* one : to) {
						const auto at = a_known.swaps.size();
						a_known.swaps.push_back({ from, id, one, { kFrameworks[kBos].name, chance, swap->somewhere } });
						a_known.swapsTo[one].push_back(at);
						if (from) {
							a_known.swapsFrom[from].push_back(at);
						}
					}
				}
			}
		}

		Known* Resolved()
		{
			if (known) {
				return &*known;
			}
			if (!Ready()) {
				return nullptr;
			}
			const auto start = std::chrono::steady_clock::now();
			known.emplace();
			std::size_t dropped = 0;
			for (const auto& file : texts->files[kSpid]) {
				AddRules(*known, file, dropped);
			}
			LinkRules(*known);
			AddGifts(*known);
			for (const auto& file : texts->files[kCid]) {
				AddStocks(*known, file, dropped);
			}
			for (const auto& file : texts->files[kBos]) {
				AddSwaps(*known, file, dropped);
			}
			const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			logger::info("the hand outs: SPID gives {} things in {} lines, CID stocks {}, BOS swaps {}, {} lines name forms the game doesn't have, looked up in {:.1f} ms",
				known->gifts.size(), known->rules.size(), known->stocks.size(), known->swaps.size(), dropped, took);
			return &*known;
		}

		// a list of templates gives the data of this many of its people at most
		constexpr std::size_t kMostSources = 64;

		void AddSources(const RE::TESNPC& a_person, Use a_use, std::vector<const RE::TESNPC*>& a_found, int a_depth);

		void AddEntries(const RE::TESLevCharacter& a_list, Use a_use, std::vector<const RE::TESNPC*>& a_found, int a_depth)
		{
			for (const auto& entry : a_list.entries) {
				if (a_found.size() >= kMostSources || a_depth >= kTemplateDepth || !entry.form) {
					return;
				}
				if (const auto* person = entry.form->As<RE::TESNPC>()) {
					AddSources(*person, a_use, a_found, a_depth + 1);
				} else if (const auto* list = entry.form->As<RE::TESLevCharacter>()) {
					AddEntries(*list, a_use, a_found, a_depth + 1);
				}
			}
		}

		void AddSources(const RE::TESNPC& a_person, Use a_use, std::vector<const RE::TESNPC*>& a_found, int a_depth)
		{
			const auto* base = a_person.actorData.templateUseFlags.any(a_use) && a_depth < kTemplateDepth ? a_person.baseTemplateForm : nullptr;
			if (const auto* next = base ? base->As<RE::TESNPC>() : nullptr) {
				AddSources(*next, a_use, a_found, a_depth + 1);
			} else if (const auto* list = base ? base->As<RE::TESLevCharacter>() : nullptr) {
				AddEntries(*list, a_use, a_found, a_depth + 1);
			} else if (std::ranges::find(a_found, &a_person) == a_found.end()) {
				a_found.push_back(&a_person);
			}
		}

		// The records a person takes a part of its data from, as the template flags say. A list of templates gives the
		// data of any of its people, the game picks one when the person loads, so a line fits when one of them fits
		std::map<std::pair<const RE::TESNPC*, Use>, std::vector<const RE::TESNPC*>> sourcesOf;

		const std::vector<const RE::TESNPC*>& From(const RE::TESNPC& a_person, Use a_use)
		{
			const auto key = std::make_pair(&a_person, a_use);
			if (const auto found = sourcesOf.find(key); found != sourcesOf.end()) {
				return found->second;
			}
			std::vector<const RE::TESNPC*> sources;
			AddSources(a_person, a_use, sources, 0);
			if (sources.empty()) {
				sources.push_back(&a_person);
			}
			return sourcesOf.emplace(key, std::move(sources)).first->second;
		}

		template <class F>
		bool AnyOf(const RE::TESNPC& a_person, Use a_use, F&& a_test)
		{
			return std::ranges::any_of(From(a_person, a_use), [&](const RE::TESNPC* a_source) { return a_test(*a_source); });
		}

		// The game keeps few editor ids, powerofthree's Tweaks keeps the rest and SPID asks it the same way
		std::string EditorIdOf(const RE::TESForm& a_form)
		{
			using Ask = const char* (*)(std::uint32_t);
			static const Ask tweaks = [] {
				const auto module = GetModuleHandleW(L"po3_Tweaks.dll");
				return module ? reinterpret_cast<Ask>(GetProcAddress(module, "GetFormEditorID")) : nullptr;
			}();
			if (const char* id = a_form.GetFormEditorID(); id && *id) {
				return id;
			}
			const char* id = tweaks ? tweaks(a_form.GetFormID()) : nullptr;
			return id ? id : "";
		}

		bool SameWords(std::string_view a_one, std::string_view a_other)
		{
			return a_one.size() == a_other.size() && std::ranges::equal(a_one, a_other, [](unsigned char a_a, unsigned char a_b) {
				return std::tolower(a_a) == std::tolower(a_b);
			});
		}

		bool HoldsWords(std::string_view a_text, std::string_view a_part)
		{
			return !std::ranges::search(a_text, a_part, [](unsigned char a_a, unsigned char a_b) { return std::tolower(a_a) == std::tolower(a_b); }).empty();
		}

		// what the words of a SPID line look at: the names, the editor ids of the record and its template, the keywords
		// of the record and its race
		struct Words
		{
			std::vector<std::string> names;
			std::vector<std::string> ids;
			std::vector<std::string> keywords;

			[[nodiscard]] bool Has(std::string_view a_word) const
			{
				const auto same = [&](const std::string& a_text) { return SameWords(a_text, a_word); };
				return std::ranges::any_of(names, same) || std::ranges::any_of(ids, same) || std::ranges::any_of(keywords, same);
			}

			[[nodiscard]] bool HasPart(std::string_view a_part) const
			{
				const auto holds = [&](const std::string& a_text) { return HoldsWords(a_text, a_part); };
				return std::ranges::any_of(names, holds) || std::ranges::any_of(ids, holds) || std::ranges::any_of(keywords, holds);
			}
		};

		std::unordered_map<const RE::TESNPC*, Words> wordsOf;

		void AddKeywords(const RE::BGSKeywordForm& a_form, std::vector<std::string>& a_keywords)
		{
			for (const auto* keyword : a_form.GetKeywords()) {
				if (keyword) {
					a_keywords.emplace_back(keyword->GetFormEditorID());
				}
			}
		}

		const Words& WordsOf(const RE::TESNPC& a_person)
		{
			if (const auto found = wordsOf.find(&a_person); found != wordsOf.end()) {
				return found->second;
			}
			Words words;
			for (const auto* source : From(a_person, Use::kBaseData)) {
				if (const char* name = source->GetName(); name && *name) {
					words.names.emplace_back(name);
				}
			}
			words.ids.push_back(EditorIdOf(a_person));
			if (a_person.baseTemplateForm) {
				words.ids.push_back(EditorIdOf(*a_person.baseTemplateForm));
			}
			for (const auto* source : From(a_person, Use::kKeywords)) {
				AddKeywords(*source, words.keywords);
			}
			for (const auto* source : From(a_person, Use::kTraits)) {
				if (source->race) {
					AddKeywords(*source->race, words.keywords);
				}
			}
			return wordsOf.emplace(&a_person, std::move(words)).first->second;
		}

		bool InFaction(const RE::TESNPC& a_person, const RE::TESForm* a_faction)
		{
			return AnyOf(a_person, Use::kFactions, [&](const RE::TESNPC& a_source) {
				return std::ranges::any_of(a_source.factions, [&](const RE::FACTION_RANK& a_rank) { return a_rank.faction == a_faction && a_rank.rank >= 0; });
			});
		}

		// a form of SPID's form filters, the way SPID asks an actor about it, with the records' data in its place
		bool HasForm(const RE::TESNPC& a_person, const RE::TESForm& a_form, int a_depth = 0)
		{
			switch (a_form.GetFormType()) {
			case RE::FormType::CombatStyle:
				return AnyOf(a_person, Use::kAIData, [&](const RE::TESNPC& a_source) { return a_source.combatStyle == &a_form; });
			case RE::FormType::Class:
				return AnyOf(a_person, Use::kStats, [&](const RE::TESNPC& a_source) { return a_source.npcClass == &a_form; });
			case RE::FormType::Faction:
				return InFaction(a_person, &a_form);
			case RE::FormType::Race:
				return AnyOf(a_person, Use::kTraits, [&](const RE::TESNPC& a_source) { return a_source.race == &a_form; });
			case RE::FormType::Outfit:
				return AnyOf(a_person, Use::kInventory, [&](const RE::TESNPC& a_source) { return a_source.defaultOutfit == &a_form; });
			case RE::FormType::NPC:
				return &a_person == &a_form || a_person.baseTemplateForm == &a_form;
			case RE::FormType::VoiceType:
				return AnyOf(a_person, Use::kTraits, [&](const RE::TESNPC& a_source) { return a_source.voiceType == &a_form; });
			case RE::FormType::Armor:
				return AnyOf(a_person, Use::kTraits, [&](const RE::TESNPC& a_source) { return a_source.skin == &a_form; });
			case RE::FormType::Spell:
				return AnyOf(a_person, Use::kSpells, [&](const RE::TESNPC& a_source) {
					const auto* spells = const_cast<RE::TESNPC&>(a_source).GetSpellList();
					return spells && spells->GetIndex(a_form.As<RE::SpellItem>()).has_value();
				});
			case RE::FormType::Perk:
				return AnyOf(a_person, Use::kSpells, [&](const RE::TESNPC& a_source) {
					return std::ranges::any_of(std::span(a_source.perks, a_source.perkCount), [&](const RE::PerkRankData& a_rank) { return a_rank.perk == &a_form; });
				});
			case RE::FormType::FormList:
				{
					const auto* list = a_form.As<RE::BGSListForm>();
					return a_depth < kTemplateDepth && std::ranges::any_of(list->forms, [&](const RE::TESForm* a_one) { return a_one && HasForm(a_person, *a_one, a_depth + 1); });
				}
			default:
				return false;
			}
		}

		bool HasFilter(const RE::TESNPC& a_person, const Filter& a_filter)
		{
			if (a_filter.file) {
				return a_filter.file->IsFormInMod(a_person.GetFormID()) || (a_person.baseTemplateForm && a_filter.file->IsFormInMod(a_person.baseTemplateForm->GetFormID()));
			}
			return HasForm(a_person, *a_filter.form);
		}

		bool InRange(const HandoutLines::Range& a_range, std::uint16_t a_low, std::uint16_t a_high)
		{
			return a_high >= a_range.min && a_low <= a_range.max;
		}

		bool FitsStats(const RE::TESNPC& a_stats, const HandoutLines::Gift& a_line)
		{
			const auto& data = a_stats.actorData;
			// a level that grows with the player's spans the range the record allows it
			const bool grows = a_stats.HasPCLevelMult();
			const auto low = grows ? std::max<std::uint16_t>(data.calcLevelMin, 1) : data.level;
			const auto high = grows ? (data.calcLevelMax == 0 ? std::uint16_t{ 0xFFFF } : data.calcLevelMax) : data.level;
			if (!InRange(a_line.level, low, high)) {
				return false;
			}
			for (const auto& skill : a_line.skills) {
				std::uint8_t value = 0;
				if (skill.weight) {
					// the weights of a class follow the skills in the game's order
					const auto weights = a_stats.npcClass ? std::bit_cast<std::array<std::uint8_t, 18>>(a_stats.npcClass->data.skillWeights) : std::array<std::uint8_t, 18>{};
					value = weights[skill.type];
				} else {
					value = a_stats.playerSkills.values[skill.type];
				}
				if (!InRange(skill.range, value, value)) {
					return false;
				}
			}
			return true;
		}

		bool Child(const RE::TESNPC& a_traits)
		{
			const auto* race = a_traits.race;
			return race && (race->data.flags.any(RE::RACE_DATA::Flag::kChild) || std::string_view(race->GetFormEditorID()).contains("RaceChild"));
		}

		bool FitsTraits(const RE::TESNPC& a_person, const HandoutLines::Gift& a_line)
		{
			const auto fits = [&](Use a_use, const std::optional<bool>& a_asked, auto&& a_is) {
				return !a_asked || AnyOf(a_person, a_use, [&](const RE::TESNPC& a_source) { return a_is(a_source) == *a_asked; });
			};
			if (!fits(Use::kTraits, a_line.female, [](const RE::TESNPC& a_one) { return a_one.IsFemale(); }) ||
				!fits(Use::kBaseData, a_line.unique, [](const RE::TESNPC& a_one) { return a_one.IsUnique(); }) ||
				!fits(Use::kBaseData, a_line.summonable, [](const RE::TESNPC& a_one) { return a_one.IsSummonable(); }) ||
				!fits(Use::kTraits, a_line.child, Child)) {
				return false;
			}
			if (a_line.leveled) {
				// a record the game levels is one a list of templates gives its data, or one a list of encounters holds
				const bool leveled = (a_person.baseTemplateForm && a_person.baseTemplateForm->Is(RE::FormType::LeveledNPC)) || Holders::Read().encounters.contains(&a_person);
				if (*a_line.leveled != leveled) {
					return false;
				}
			}
			return !a_line.teammate || InFaction(a_person, RE::TESForm::LookupByID(kPotentialFollower)) == *a_line.teammate;
		}

		bool Fits(const Rule& a_rule, const RE::TESNPC& a_person)
		{
			const auto& line = a_rule.line;
			if (!line.wordsAll.empty() || !line.wordsNone.empty() || !line.wordsMatch.empty() || !line.wordsPart.empty()) {
				const auto& words = WordsOf(a_person);
				const auto has = [&](const std::string& a_word) { return words.Has(a_word); };
				if (!std::ranges::all_of(line.wordsAll, has) || std::ranges::any_of(line.wordsNone, has) || (!line.wordsMatch.empty() && !std::ranges::any_of(line.wordsMatch, has)) ||
					(!line.wordsPart.empty() && !std::ranges::any_of(line.wordsPart, [&](const std::string& a_part) { return words.HasPart(a_part); }))) {
					return false;
				}
			}
			const auto has = [&](const Filter& a_filter) { return HasFilter(a_person, a_filter); };
			if (!std::ranges::all_of(a_rule.all, has) || std::ranges::any_of(a_rule.none, has) || (!a_rule.match.empty() && !std::ranges::any_of(a_rule.match, has))) {
				return false;
			}
			return AnyOf(a_person, Use::kStats, [&](const RE::TESNPC& a_stats) { return FitsStats(a_stats, line); }) && FitsTraits(a_person, line);
		}

		bool FitsRule(const Known& a_known, const Rule& a_rule, const RE::TESNPC& a_person)
		{
			if (!a_rule.line.linked) {
				return Fits(a_rule, a_person);
			}
			return std::ranges::any_of(a_rule.parents, [&](std::size_t a_parent) { return Fits(a_known.rules[a_parent], a_person); });
		}

		template <class T>
		std::vector<const T*> Pick(const std::vector<T>& a_all, const std::unordered_map<const RE::TESForm*, std::vector<std::size_t>>& a_index, const RE::TESForm& a_form)
		{
			std::vector<const T*> picked;
			if (const auto found = a_index.find(&a_form); found != a_index.end()) {
				for (const auto at : found->second) {
					picked.push_back(&a_all[at]);
				}
			}
			return picked;
		}
	}

	void Start()
	{
		if (texts) {
			return;
		}
		texts = new Texts();
		for (std::size_t kind = 0; kind < kKinds; ++kind) {
			texts->loaded[kind] = GetModuleHandleW(kFrameworks[kind].module) != nullptr;
		}
		logger::info("step: the hand out files are read in the background, SPID {}, CID {}, BOS {}", texts->loaded[kSpid] ? "loaded" : "not loaded",
			texts->loaded[kCid] ? "loaded" : "not loaded", texts->loaded[kBos] ? "loaded" : "not loaded");
		std::thread([] {
			try {
				ReadTexts(texts);
			} catch (const std::exception& a_error) {
				logger::warn("the hand out files left out, {}", a_error.what());
				texts->ready = true;
			}
		}).detach();
	}

	bool Started()
	{
		return texts != nullptr;
	}

	bool Ready()
	{
		return texts && texts->ready;
	}

	std::vector<const Gift*> GiftsOf(const RE::TESForm& a_form)
	{
		const auto* all = Resolved();
		return all ? Pick(all->gifts, all->giftsOf, a_form) : std::vector<const Gift*>{};
	}

	const std::vector<const RE::TESNPC*>& PeopleOf(const Gift& a_gift)
	{
		static const std::vector<const RE::TESNPC*> nobody;
		auto* all = Resolved();
		if (!all || a_gift.told) {
			return nobody;
		}
		auto& rule = all->rules[a_gift.rule];
		if (rule.people) {
			return *rule.people;
		}
		std::vector<const RE::TESNPC*> people;
		for (const auto* person : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESNPC>()) {
			if (Holders::Lasting(person) && Holders::Named(person) && person->GetFormID() != kPlayerBase && FitsRule(*all, rule, *person)) {
				people.push_back(person);
			}
		}
		rule.people = std::move(people);
		return *rule.people;
	}

	std::vector<const Gift*> GiftsTo(const RE::TESNPC& a_person)
	{
		std::vector<const Gift*> gifts;
		auto* all = Resolved();
		if (!all) {
			return gifts;
		}
		for (const auto& gift : all->gifts) {
			if (!gift.told && FitsRule(*all, all->rules[gift.rule], a_person)) {
				gifts.push_back(&gift);
			}
		}
		return gifts;
	}

	std::vector<const Stock*> StocksOf(const RE::TESForm& a_form)
	{
		const auto* all = Resolved();
		return all ? Pick(all->stocks, all->stocksOf, a_form) : std::vector<const Stock*>{};
	}

	std::vector<const Stock*> StocksInto(const RE::TESForm& a_container)
	{
		const auto* all = Resolved();
		return all ? Pick(all->stocks, all->stocksInto, a_container) : std::vector<const Stock*>{};
	}

	std::vector<const Swap*> SwapsTo(const RE::TESForm& a_form)
	{
		const auto* all = Resolved();
		return all ? Pick(all->swaps, all->swapsTo, a_form) : std::vector<const Swap*>{};
	}

	std::vector<const Swap*> SwapsFrom(const RE::TESForm& a_form)
	{
		const auto* all = Resolved();
		return all ? Pick(all->swaps, all->swapsFrom, a_form) : std::vector<const Swap*>{};
	}
}
