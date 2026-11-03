#include "PCH.h"

#include "BrowserQuestLinks.h"

#include <set>
#include <thread>
#include <unordered_set>

#include "BrowserFiles.h"
#include "BrowserIndex.h"
#include "BrowserQuests.h"
#include "BrowserSources.h"
#include "GameArchives.h"
#include "PapyrusDeeds.h"
#include "Places.h"
#include "PluginSources.h"
#include "Profile.h"
#include "QuestRecords.h"

namespace BrowserQuestLinks
{
	namespace
	{
		using PapyrusDeeds::Deed;
		using PapyrusDeeds::Symbol;
		using QuestRecords::Instance;
		using QuestRecords::Property;

		constexpr RE::FormID kPlayerRef = 0x14;
		constexpr RE::FormID kPlayerBase = 0x7;
		constexpr std::uint32_t kQuestObject = 0x4;
		// a thing taken back from the player by its form, whatever alias holds it
		constexpr std::uint32_t kAnyAlias = 0xFFFFFFFF;
		// symbols of symbols and parents of scripts go this deep at most
		constexpr int kDeepest = 8;
		// a hidden quest that starts more than this many is a hub of radiant quests, a chain doesn't run through it
		constexpr std::size_t kHubStarts = 8;
		constexpr std::size_t kChainsShown = 4;
		constexpr std::size_t kChainsWhole = 24;
		constexpr std::size_t kLongestChain = 40;
		constexpr std::size_t kQuestsShown = 8;
		constexpr std::size_t kGroupsShown = 6;
		constexpr std::size_t kPlacesShown = 8;
		// a list the player opened on the page names this many at most
		constexpr std::size_t kWholeList = 200;
		// the things a reward picked from a list may be, the page names the first few
		constexpr std::size_t kOptionsShown = 6;
		constexpr std::size_t kMostOptions = 60;
		// the scripts of the game's own types are no quest's, the walk up the parents stops at them
		constexpr std::array<std::string_view, 12> kNative{ "quest", "referencealias", "locationalias", "alias", "topicinfo", "form", "objectreference", "actor",
			"scene", "package", "activemagiceffect", "magiceffect" };

		enum Verdict : std::uint8_t
		{
			kStarts = 1,
			kCloses = 2,
			kMoves = 4
		};

		// what a quest puts into the player's hands, a thing an alias holds by the alias of the quest
		struct Gift
		{
			enum class Kind : std::uint8_t
			{
				kItem,
				kHeld,
				kSpell,
				kPerk,
				kShout,
				kWord,
				kSkill,
				kPerkPoints,
				kSouls
			};

			Kind kind = Kind::kItem;
			RE::FormID form = 0;
			std::uint32_t alias = 0;
			// 0 when the script counts it as it runs, or the global it takes the count from
			std::int32_t count = 0;
			RE::FormID countOf = 0;
			std::string skill;
			// the stage whose fragment gives it, -1 for a line of dialogue or a function of a script
			std::int32_t stage = -1;
			bool dialogue = false;

			[[nodiscard]] bool Same(const Gift& a_other) const
			{
				return kind == a_other.kind && form == a_other.form && alias == a_other.alias && count == a_other.count && countOf == a_other.countOf && skill == a_other.skill;
			}
		};

		struct Facts
		{
			std::vector<QuestRecords::Stage> stages;
			std::vector<Gift> gifts;
			// while the scripts are read, the things of the quest's own it puts into the player's hands and the ones of
			// them it takes back from the player
			std::vector<Gift> lent;
			std::vector<std::pair<RE::FormID, std::uint32_t>> takenBack;
			std::unordered_map<RE::FormID, std::uint8_t> calls;
			// the forms its scripts name and the locations its aliases are forced to
			std::vector<RE::FormID> pointed;
		};

		struct State
		{
			std::vector<PluginSources::Source> sources;
			std::vector<std::string> archives;
			std::string language;
			std::unordered_map<RE::FormID, Facts> facts;
			std::atomic<bool> ready = false;
		};

		// the thread may still read when the game quits, so the state is never freed
		State* state = nullptr;

		std::string Lower(std::string_view a_text)
		{
			std::string lower(a_text);
			for (auto& letter : lower) {
				letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
			}
			return lower;
		}

		struct Marks
		{
			std::uint16_t first = 0;
			std::uint16_t last = 0;
		};

		// the first stage the journal tells, the stage that finishes the quest
		Marks MarksOf(const std::vector<QuestRecords::Stage>& a_stages)
		{
			Marks marks{ 0xFFFF, 0xFFFF };
			std::uint16_t lowest = 0xFFFF;
			std::uint16_t highest = 0;
			for (const auto& stage : a_stages) {
				lowest = std::min(lowest, stage.index);
				highest = std::max(highest, stage.index);
				for (const auto& entry : stage.entries) {
					if (!entry.text.empty()) {
						marks.first = std::min(marks.first, stage.index);
					}
					if (entry.finishes) {
						marks.last = std::min(marks.last, stage.index);
					}
				}
			}
			marks.first = marks.first != 0xFFFF ? marks.first : (lowest != 0xFFFF ? lowest : 0);
			marks.last = marks.last != 0xFFFF ? marks.last : highest;
			return marks;
		}

		// What a symbol of a script stands for in one quest: the player, a quest, a form, an alias of a quest, the
		// thing an alias holds, a number, a text, the value of a global
		struct Bound
		{
			enum class Kind : std::uint8_t
			{
				kPlayer,
				kQuest,
				kForm,
				kAlias,
				kHeld,
				kNumber,
				kText,
				kGlobal
			};

			Kind kind = Kind::kNumber;
			RE::FormID form = 0;
			std::uint32_t alias = 0;
			std::int32_t number = 0;
			std::string text;
		};

		bool Has(const std::vector<Bound>& a_bound, Bound::Kind a_kind)
		{
			return std::ranges::any_of(a_bound, [&](const Bound& a_one) { return a_one.kind == a_kind; });
		}

		class Job
		{
		public:
			explicit Job(State& a_state) :
				state(a_state), files(a_state.archives, { "scripts", "strings" }) {}

			void Run()
			{
				const auto start = std::chrono::steady_clock::now();
				const auto held = files.Hold("scripts");
				logger::info("quest scripts: {:.1f} MB of the archives read in {:.0f} ms", static_cast<double>(held) / (1024.0 * 1024.0),
					std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
				read = QuestRecords::ReadAll(state.sources, files, state.language);
				const auto recordsTook = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
				for (const auto& [id, quest] : read.quests) {
					if (!quest.editorID.empty()) {
						named.emplace(Lower(quest.editorID), id);
					}
				}
				for (std::size_t i = 0; i < state.sources.size(); ++i) {
					plugins.emplace(Lower(GameArchives::Narrow(state.sources[i].path.filename())), i);
				}
				std::size_t gifts = 0;
				std::size_t calls = 0;
				for (const auto& [id, quest] : read.quests) {
					auto& facts = state.facts[id];
					Gather(quest, facts);
					gifts += facts.gifts.size();
					calls += facts.calls.size();
				}
				const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
				logger::info("quest records: {} quests and {} lines of dialogue of {} files in {:.0f} ms ({} journal entries named, {} packed records left out, {} files unread); "
							 "{} scripts read of {} archives and {} loose files, {} not found, {} unreadable ({:.0f} ms reading, {:.0f} ms reading their code); {} rewards, {} quests set by others, {:.0f} ms in all",
					read.quests.size(), read.infos, read.files, recordsTook, read.texts, read.packed, read.unread, scriptsRead, files.Archives(), files.Loose(), scriptsMissing,
					scriptsBroken, readTook, parseTook, gifts, calls, took);
				const auto& taken = files.Taken();
				logger::info("quest files: {} loose in {:.0f} ms, {} of the archives in {:.0f} ms, {} packed ones in {:.0f} ms", taken.looseFiles, taken.loose,
					taken.archivedFiles, taken.archived, taken.unpackedFiles, taken.unpacked);
			}

		private:
			struct Context
			{
				const QuestRecords::Quest* quest = nullptr;
				const Instance* instance = nullptr;
				std::optional<std::uint32_t> alias;
				bool dialogue = false;
			};

			const PapyrusDeeds::Script* ScriptOf(const std::string& a_name)
			{
				const auto [at, fresh] = scripts.try_emplace(a_name);
				if (fresh) {
					const auto start = std::chrono::steady_clock::now();
					auto bytes = files.Read(std::format("scripts\\{}.pex", a_name));
					const auto read = std::chrono::steady_clock::now();
					readTook += std::chrono::duration<double, std::milli>(read - start).count();
					if (bytes) {
						at->second = PapyrusDeeds::Read(*bytes);
						parseTook += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - read).count();
						(at->second ? scriptsRead : scriptsBroken)++;
					} else {
						scriptsMissing++;
					}
				}
				return at->second ? &*at->second : nullptr;
			}

			const Marks& MarksFor(RE::FormID a_quest)
			{
				const auto [at, fresh] = marks.try_emplace(a_quest);
				if (fresh) {
					if (const auto quest = read.quests.find(a_quest); quest != read.quests.end()) {
						at->second = MarksOf(quest->second.stages);
					}
				}
				return at->second;
			}

			const QuestRecords::Alias* AliasOf(RE::FormID a_quest, std::uint32_t a_alias) const
			{
				const auto quest = read.quests.find(a_quest);
				if (quest == read.quests.end()) {
					return nullptr;
				}
				const auto found = std::ranges::find(quest->second.aliases, a_alias, &QuestRecords::Alias::id);
				return found != quest->second.aliases.end() ? &*found : nullptr;
			}

			Bound Object(RE::FormID a_form, std::int16_t a_alias) const
			{
				if (a_form == kPlayerRef) {
					return { Bound::Kind::kPlayer };
				}
				if (a_alias >= 0) {
					return { Bound::Kind::kAlias, a_form, static_cast<std::uint32_t>(a_alias) };
				}
				return { read.quests.contains(a_form) ? Bound::Kind::kQuest : Bound::Kind::kForm, a_form };
			}

			// the reference an alias holds, the player for an alias forced on the player
			Bound Held(const Bound& a_alias) const
			{
				const auto* alias = AliasOf(a_alias.form, a_alias.alias);
				if (alias && (alias->forcedReference == kPlayerRef || alias->uniqueActor == kPlayerBase)) {
					return { Bound::Kind::kPlayer };
				}
				return { Bound::Kind::kHeld, a_alias.form, a_alias.alias };
			}

			// a property as the plugin sets it for this script, or the number the script gives it itself
			std::vector<Bound> PropertyOf(const Context& a_context, const PapyrusDeeds::Script& a_script, const std::string& a_name) const
			{
				if (const auto found = a_context.instance->properties.find(a_name); found != a_context.instance->properties.end()) {
					const auto& value = found->second;
					switch (value.type) {
					case Property::Type::kObject:
						return value.form != 0 ? std::vector<Bound>{ Object(value.form, value.alias) } : std::vector<Bound>{};
					case Property::Type::kInteger:
					case Property::Type::kFloat:
					case Property::Type::kBool:
						return { { Bound::Kind::kNumber, 0, 0, value.number } };
					case Property::Type::kText:
						return { { Bound::Kind::kText, 0, 0, 0, Lower(value.text) } };
					default:
						return {};
					}
				}
				if (const auto number = a_script.numbers.find(a_name); number != a_script.numbers.end()) {
					return { { Bound::Kind::kNumber, 0, 0, number->second } };
				}
				return {};
			}

			std::vector<Bound> Bind(const Context& a_context, const PapyrusDeeds::Script& a_script, std::int32_t a_symbol, int a_depth = 0)
			{
				std::vector<Bound> bound;
				if (a_symbol < 0 || static_cast<std::size_t>(a_symbol) >= a_script.symbols.size() || a_depth > kDeepest) {
					return bound;
				}
				const auto& symbol = a_script.symbols[a_symbol];
				const auto inner = [&]() { return Bind(a_context, a_script, symbol.of, a_depth + 1); };
				switch (symbol.kind) {
				case Symbol::Kind::kPlayer:
					bound.push_back({ Bound::Kind::kPlayer });
					break;
				case Symbol::Kind::kOwnQuest:
					bound.push_back({ Bound::Kind::kQuest, a_context.quest->id });
					break;
				case Symbol::Kind::kSelf:
					if (a_context.alias) {
						bound.push_back({ Bound::Kind::kAlias, a_context.quest->id, *a_context.alias });
					} else if (!a_context.dialogue) {
						bound.push_back({ Bound::Kind::kQuest, a_context.quest->id });
					}
					break;
				case Symbol::Kind::kProperty:
					return PropertyOf(a_context, a_script, symbol.name);
				case Symbol::Kind::kNumber:
					bound.push_back({ Bound::Kind::kNumber, 0, 0, symbol.number });
					break;
				case Symbol::Kind::kText:
					bound.push_back({ Bound::Kind::kText, 0, 0, 0, symbol.name });
					break;
				case Symbol::Kind::kAliasRef:
					for (const auto& one : inner()) {
						if (one.kind == Bound::Kind::kAlias) {
							bound.push_back(Held(one));
						}
					}
					break;
				case Symbol::Kind::kValueOf:
					for (const auto& one : inner()) {
						if (one.kind == Bound::Kind::kForm) {
							bound.push_back({ Bound::Kind::kGlobal, one.form });
						}
					}
					break;
				case Symbol::Kind::kPropertyOf:
					// (GetOwningQuest() as BQScript).RewardAmount: the property of a script of that quest
					for (const auto& one : inner()) {
						const auto other = one.kind == Bound::Kind::kQuest ? read.quests.find(one.form) : read.quests.end();
						if (other == read.quests.end()) {
							continue;
						}
						for (const auto& instance : other->second.scripts) {
							const auto* script = ScriptOf(instance.script);
							if (script && (instance.properties.contains(symbol.name) || script->numbers.contains(symbol.name))) {
								const auto found = PropertyOf({ &other->second, &instance }, *script, symbol.name);
								bound.insert(bound.end(), found.begin(), found.end());
								break;
							}
						}
					}
					break;
				case Symbol::Kind::kQuestNamed:
					if (const auto quest = named.find(symbol.name); quest != named.end()) {
						bound.push_back({ Bound::Kind::kQuest, quest->second });
					}
					break;
				case Symbol::Kind::kFormFromFile:
					if (const auto plugin = plugins.find(symbol.name); plugin != plugins.end()) {
						const auto& source = state.sources[plugin->second];
						const auto own = static_cast<std::uint32_t>(source.owners.size() - 1) << 24;
						bound.push_back(Object(source.Runtime(own | (static_cast<std::uint32_t>(symbol.number) & 0xFFFFFF)), -1));
					}
					break;
				}
				return bound;
			}

			std::vector<Bound> Argument(const Context& a_context, const PapyrusDeeds::Script& a_script, const PapyrusDeeds::Call& a_call, std::size_t a_at)
			{
				std::vector<Bound> bound;
				if (a_at < a_call.arguments.size()) {
					for (const auto symbol : a_call.arguments[a_at]) {
						auto one = Bind(a_context, a_script, symbol);
						bound.insert(bound.end(), one.begin(), one.end());
					}
				}
				return bound;
			}

			void Give(Facts& a_facts, const Gift& a_gift) const
			{
				if (std::ranges::none_of(a_facts.gifts, [&](const Gift& a_had) { return a_had.Same(a_gift); })) {
					a_facts.gifts.push_back(a_gift);
				}
			}

			// Things into the player's hands. A thing of the quest's own, an alias marked as a quest object, is lent
			// until the scripts are all read. A letter to deliver or a shield to take to the smith is taken back from
			// the player, the Wabbajack of The Mind of Madness stays and is a reward
			void GiveThings(Facts& a_facts, const std::vector<Bound>& a_things, const std::vector<Bound>& a_counts, Gift a_gift) const
			{
				for (const auto& count : a_counts) {
					if (count.kind == Bound::Kind::kNumber) {
						a_gift.count = count.number;
						break;
					}
					if (count.kind == Bound::Kind::kGlobal) {
						a_gift.countOf = count.form;
						break;
					}
				}
				if (a_gift.count < 0) {
					return;
				}
				for (const auto& thing : a_things) {
					if (thing.kind == Bound::Kind::kForm) {
						a_gift.kind = Gift::Kind::kItem;
						a_gift.form = thing.form;
					} else if (thing.kind == Bound::Kind::kHeld) {
						a_gift.kind = Gift::Kind::kHeld;
						a_gift.form = thing.form;
						a_gift.alias = thing.alias;
						const auto* alias = AliasOf(thing.form, thing.alias);
						if (alias && (alias->flags & kQuestObject)) {
							if (std::ranges::none_of(a_facts.lent, [&](const Gift& a_had) { return a_had.Same(a_gift); })) {
								a_facts.lent.push_back(a_gift);
							}
							continue;
						}
					} else {
						continue;
					}
					Give(a_facts, a_gift);
				}
			}

			void GiveForms(Facts& a_facts, const std::vector<Bound>& a_forms, Gift a_gift) const
			{
				for (const auto& form : a_forms) {
					if (form.kind == Bound::Kind::kForm) {
						a_gift.form = form.form;
						Give(a_facts, a_gift);
					}
				}
			}

			static bool Usable(const RE::TESForm& a_thing)
			{
				if (const auto* book = a_thing.As<RE::TESObjectBOOK>()) {
					return book->data.flags.any(RE::OBJ_BOOK::Flag::kTeachesSpell, RE::OBJ_BOOK::Flag::kAdvancesActorValue);
				}
				return a_thing.Is(RE::FormType::Weapon) || a_thing.Is(RE::FormType::Armor) || a_thing.Is(RE::FormType::Ammo) || a_thing.Is(RE::FormType::SoulGem);
			}

			static std::int32_t FirstNumber(const std::vector<Bound>& a_bound, std::int32_t a_none)
			{
				const auto found = std::ranges::find(a_bound, Bound::Kind::kNumber, &Bound::kind);
				return found != a_bound.end() ? found->number : a_none;
			}

			// A script that starts another quest or sets it to a stage up to its first journal entry leads to it, one
			// that finishes it or sets it to its finishing stage comes after it, any other stage moves it on. A
			// fragment of a stage before the quest's own first entry is how the developers skip ahead, it says nothing
			void Link(const Context& a_context, const PapyrusDeeds::Call& a_call, const std::vector<Bound>& a_target, const std::vector<Bound>& a_stages,
				const std::vector<std::uint16_t>& a_runsAt, Facts& a_facts)
			{
				const auto own = MarksFor(a_context.quest->id);
				const bool skipping = !a_runsAt.empty() && std::ranges::all_of(a_runsAt, [&](std::uint16_t a_stage) { return a_stage > 0 && a_stage < own.first; });
				if (skipping) {
					return;
				}
				for (const auto& target : a_target) {
					if (target.kind != Bound::Kind::kQuest || target.form == a_context.quest->id) {
						continue;
					}
					std::uint8_t verdict = kMoves;
					if (a_call.deed == Deed::kStart) {
						verdict = kStarts;
					} else if (a_call.deed == Deed::kStop || a_call.deed == Deed::kComplete) {
						verdict = kCloses;
					} else {
						std::int32_t lowest = INT32_MAX;
						std::int32_t highest = INT32_MIN;
						for (const auto& stage : a_stages) {
							if (stage.kind == Bound::Kind::kNumber) {
								lowest = std::min(lowest, stage.number);
								highest = std::max(highest, stage.number);
							}
						}
						const auto& marks = MarksFor(target.form);
						if (lowest != INT32_MAX) {
							verdict = lowest <= marks.first ? kStarts : (highest >= marks.last ? kCloses : kMoves);
						}
					}
					a_facts.calls[target.form] |= verdict;
				}
			}

			void Use(const Context& a_context, const PapyrusDeeds::Script& a_script, const PapyrusDeeds::Call& a_call, const std::vector<std::uint16_t>& a_runsAt, Facts& a_facts)
			{
				std::vector<Bound> target;
				for (const auto symbol : a_call.target) {
					auto one = Bind(a_context, a_script, symbol);
					target.insert(target.end(), one.begin(), one.end());
				}
				const bool player = Has(target, Bound::Kind::kPlayer);
				const auto argument = [&](std::size_t a_at) { return Argument(a_context, a_script, a_call, a_at); };
				Gift gift;
				gift.stage = a_runsAt.empty() ? -1 : *std::ranges::min_element(a_runsAt);
				gift.dialogue = a_context.dialogue;
				switch (a_call.deed) {
				case Deed::kAddItem:
					if (player) {
						GiveThings(a_facts, argument(0), argument(1), gift);
					}
					break;
				case Deed::kRemoveItem:
					// the giver's RemoveItem into the player's inventory, the player's own takes a thing back
					if (Has(argument(3), Bound::Kind::kPlayer)) {
						GiveThings(a_facts, argument(0), argument(1), gift);
					} else if (player) {
						for (const auto& thing : argument(0)) {
							if (thing.kind == Bound::Kind::kHeld) {
								a_facts.takenBack.emplace_back(thing.form, thing.alias);
							} else if (thing.kind == Bound::Kind::kForm) {
								a_facts.takenBack.emplace_back(thing.form, kAnyAlias);
							}
						}
					}
					break;
				case Deed::kAddSpell:
				case Deed::kAddPerk:
				case Deed::kAddShout:
					if (player) {
						gift.kind = a_call.deed == Deed::kAddSpell ? Gift::Kind::kSpell : (a_call.deed == Deed::kAddPerk ? Gift::Kind::kPerk : Gift::Kind::kShout);
						GiveForms(a_facts, argument(0), gift);
					}
					break;
				case Deed::kTeachWord:
					gift.kind = Gift::Kind::kWord;
					GiveForms(a_facts, argument(0), gift);
					break;
				case Deed::kIncrementSkill:
				case Deed::kAdvanceSkill:
					for (const auto& skill : argument(0)) {
						if (skill.kind == Bound::Kind::kText) {
							gift.kind = Gift::Kind::kSkill;
							gift.skill = skill.text;
							gift.count = a_call.deed == Deed::kIncrementSkill ? FirstNumber(argument(1), 1) : 0;
							Give(a_facts, gift);
						}
					}
					break;
				case Deed::kAddPerkPoints:
					gift.kind = Gift::Kind::kPerkPoints;
					gift.count = FirstNumber(argument(0), 0);
					if (gift.count > 0) {
						Give(a_facts, gift);
					}
					break;
				case Deed::kModActorValue:
					// only what grows, because learning a word spends the dragon souls with a modded value below zero
					if (player) {
						const auto by = FirstNumber(argument(1), 0);
						for (const auto& value : argument(0)) {
							if (value.kind == Bound::Kind::kText && by > 0) {
								gift.kind = value.text == "dragonsouls" ? Gift::Kind::kSouls : Gift::Kind::kSkill;
								gift.skill = value.text;
								gift.count = by;
								Give(a_facts, gift);
							}
						}
					}
					break;
				case Deed::kSetStage:
				case Deed::kStart:
				case Deed::kStop:
				case Deed::kComplete:
					Link(a_context, a_call, target, a_call.deed == Deed::kSetStage ? argument(0) : std::vector<Bound>{}, a_runsAt, a_facts);
					break;
				}
			}

			// the calls of a script and of the scripts it extends, the fragments of a quest's stages know their stages
			void Walk(const Context& a_context, Facts& a_facts)
			{
				const auto* script = ScriptOf(a_context.instance->script);
				const bool fragments = !a_context.alias && !a_context.dialogue && a_context.instance->script == a_context.quest->fragments;
				static const std::vector<std::uint16_t> anywhere;
				for (int depth = 0; script && depth < kDeepest; ++depth) {
					for (const auto& call : script->calls) {
						const auto* runsAt = &anywhere;
						if (fragments && depth == 0 && call.function < script->functions.size()) {
							if (const auto found = a_context.quest->fragmentStages.find(script->functions[call.function]); found != a_context.quest->fragmentStages.end()) {
								runsAt = &found->second;
							}
						}
						Use(a_context, *script, call, *runsAt, a_facts);
					}
					if (script->parent.empty() || std::ranges::find(kNative, script->parent) != kNative.end()) {
						break;
					}
					script = ScriptOf(script->parent);
				}
			}

			static void Point(Facts& a_facts, std::unordered_set<RE::FormID>& a_seen, const Instance& a_instance)
			{
				for (const auto& [name, property] : a_instance.properties) {
					if (property.type == Property::Type::kObject && property.alias < 0 && property.form != 0 && property.form != kPlayerRef && a_seen.insert(property.form).second) {
						a_facts.pointed.push_back(property.form);
					}
					for (const auto form : property.forms) {
						if (form != kPlayerRef && a_seen.insert(form).second) {
							a_facts.pointed.push_back(form);
						}
					}
				}
			}

			void Gather(const QuestRecords::Quest& a_quest, Facts& a_facts)
			{
				a_facts.stages = a_quest.stages;
				std::unordered_set<RE::FormID> seen;
				for (const auto& instance : a_quest.scripts) {
					Walk({ &a_quest, &instance }, a_facts);
					Point(a_facts, seen, instance);
				}
				for (const auto& alias : a_quest.aliases) {
					for (const auto& instance : alias.scripts) {
						Walk({ &a_quest, &instance, alias.id }, a_facts);
						Point(a_facts, seen, instance);
					}
					if (alias.forcedLocation != 0 && seen.insert(alias.forcedLocation).second) {
						a_facts.pointed.push_back(alias.forcedLocation);
					}
				}
				for (const auto& instance : a_quest.dialogue) {
					Walk({ &a_quest, &instance, std::nullopt, true }, a_facts);
					Point(a_facts, seen, instance);
				}
				// A thing is taken back by its alias or by the thing the alias makes. What the player is left with is a
				// reward when it's one to use: a weapon, armor, arrows, a soul gem, a book that teaches. A key, a letter
				// to plant or a misc thing is a tool of the quest, Pelagius' Hip Bone and the Incriminating Letter stay too
				for (const auto& lent : a_facts.lent) {
					const auto* alias = AliasOf(lent.form, lent.alias);
					const auto* made = alias && alias->created != 0 ? RE::TESForm::LookupByID(alias->created) : nullptr;
					const bool back = std::ranges::find(a_facts.takenBack, std::pair{ lent.form, lent.alias }) != a_facts.takenBack.end() ||
					                  (alias && alias->created != 0 && std::ranges::find(a_facts.takenBack, std::pair{ alias->created, kAnyAlias }) != a_facts.takenBack.end());
					if (!back && made && Usable(*made)) {
						Give(a_facts, lent);
					}
				}
				a_facts.lent = {};
				a_facts.takenBack = {};
				std::ranges::stable_sort(a_facts.gifts, [](const Gift& a_one, const Gift& a_other) {
					const auto one = a_one.stage >= 0 ? a_one.stage : (a_one.dialogue ? 0x10000 : 0x20000);
					const auto other = a_other.stage >= 0 ? a_other.stage : (a_other.dialogue ? 0x10000 : 0x20000);
					return one < other;
				});
			}

			State& state;
			GameArchives::Files files;
			QuestRecords::Read read;
			std::unordered_map<std::string, std::optional<PapyrusDeeds::Script>> scripts;
			std::unordered_map<RE::FormID, Marks> marks;
			std::unordered_map<std::string, RE::FormID> named;
			std::unordered_map<std::string, std::size_t> plugins;
			std::size_t scriptsRead = 0;
			std::size_t scriptsMissing = 0;
			std::size_t scriptsBroken = 0;
			double readTook = 0.0;
			double parseTook = 0.0;
		};

		void Run(State* a_state)
		{
			try {
				Job(*a_state).Run();
			} catch (const std::exception& a_error) {
				logger::error("quest records: the reading stopped, {}", a_error.what());
			}
			a_state->ready = true;
		}

		const Facts* FactsOf(const RE::TESForm& a_quest)
		{
			if (!Ready()) {
				return nullptr;
			}
			const auto found = state->facts.find(a_quest.GetFormID());
			return found != state->facts.end() ? &found->second : nullptr;
		}

		bool Shown(RE::FormID a_quest)
		{
			const auto* entry = BrowserIndex::Find(a_quest);
			return entry && entry->family == BrowserIndex::Family::kQuests;
		}

		// Which quest leads to which, among the quests the browser shows: a hidden quest between two is walked
		// through, two quests that lead to each other and a loop say nothing of an order, and an edge another way
		// makes already is left out (Bleak Falls Barrow leads to The Way of the Voice through Dragon Rising)
		struct Graph
		{
			std::unordered_map<RE::FormID, std::vector<RE::FormID>> next;
			std::unordered_map<RE::FormID, std::vector<RE::FormID>> back;
		};
		std::optional<Graph> graph;

		using Edges = std::unordered_map<RE::FormID, std::unordered_set<RE::FormID>>;

		// the hidden quests a link of two shown ones goes through, for the log of a page
		std::unordered_map<std::uint64_t, std::vector<RE::FormID>> through;

		std::uint64_t Pair(RE::FormID a_from, RE::FormID a_to)
		{
			return static_cast<std::uint64_t>(a_from) << 32 | a_to;
		}

		// a quest that comes after another, and whether it's after because it closes the other one
		struct After
		{
			RE::FormID quest = 0;
			bool closes = false;
		};

		// Between two hidden quests the walk goes only where one starts the other. A hidden quest that closes another
		// hidden one manages it or fixes saves, it tells nothing of what comes next. The civil war's quest sets the
		// last stage of Season Unending, USSEP's USKPRetroactive132 stops the civil war's quest and sets Taking Care of
		// Business going, that way the main quest would run on into the Thieves Guild. Vanilla Skyrim.esm loses no link
		// by it, The Silver Hand still leads to Blood's Honor through C03PostQuest
		std::unordered_set<RE::FormID> ShownAfter(const std::unordered_map<RE::FormID, std::vector<After>>& a_after, RE::FormID a_from)
		{
			std::unordered_set<RE::FormID> found;
			std::unordered_set<RE::FormID> seen{ a_from };
			std::unordered_map<RE::FormID, RE::FormID> came;
			std::vector<RE::FormID> waiting{ a_from };
			while (!waiting.empty()) {
				const auto quest = waiting.back();
				waiting.pop_back();
				const auto next = a_after.find(quest);
				if (next == a_after.end() || (quest != a_from && next->second.size() > kHubStarts)) {
					continue;
				}
				for (const auto [other, closes] : next->second) {
					if (closes && quest != a_from && !Shown(other)) {
						continue;
					}
					if (!seen.insert(other).second) {
						continue;
					}
					came[other] = quest;
					if (!Shown(other)) {
						waiting.push_back(other);
						continue;
					}
					found.insert(other);
					auto& hidden = through[Pair(a_from, other)];
					for (auto at = came[other]; at != a_from; at = came[at]) {
						hidden.insert(hidden.begin(), at);
					}
				}
			}
			return found;
		}

		bool Reaches(const Edges& a_edges, RE::FormID a_from, RE::FormID a_to)
		{
			std::unordered_set<RE::FormID> seen;
			std::vector<RE::FormID> waiting;
			if (const auto start = a_edges.find(a_from); start != a_edges.end()) {
				for (const auto next : start->second) {
					if (next != a_to) {
						waiting.push_back(next);
					}
				}
			}
			while (!waiting.empty()) {
				const auto quest = waiting.back();
				waiting.pop_back();
				if (quest == a_to) {
					return true;
				}
				if (!seen.insert(quest).second) {
					continue;
				}
				if (const auto next = a_edges.find(quest); next != a_edges.end()) {
					waiting.insert(waiting.end(), next->second.begin(), next->second.end());
				}
			}
			return false;
		}

		// an edge back to a quest still on the way is a loop, it's left out. The walk starts from the quests nothing
		// leads to, in the order of their ids, the same in every game
		void BreakLoops(Edges& a_edges)
		{
			std::unordered_set<RE::FormID> led;
			for (const auto& [quest, next] : a_edges) {
				led.insert(next.begin(), next.end());
			}
			std::vector<RE::FormID> starts;
			for (const auto& [quest, next] : a_edges) {
				starts.push_back(quest);
			}
			std::ranges::sort(starts, [&](RE::FormID a_one, RE::FormID a_other) {
				return led.contains(a_one) != led.contains(a_other) ? !led.contains(a_one) : a_one < a_other;
			});
			enum class Mark : std::uint8_t
			{
				kOn,
				kDone
			};
			std::unordered_map<RE::FormID, Mark> marks;
			for (const auto start : starts) {
				if (marks.contains(start)) {
					continue;
				}
				std::vector<std::pair<RE::FormID, std::vector<RE::FormID>>> stack;
				const auto enter = [&](RE::FormID a_quest) {
					marks[a_quest] = Mark::kOn;
					std::vector<RE::FormID> next;
					if (const auto found = a_edges.find(a_quest); found != a_edges.end()) {
						next.assign(found->second.begin(), found->second.end());
						std::ranges::sort(next);
					}
					stack.emplace_back(a_quest, std::move(next));
				};
				enter(start);
				while (!stack.empty()) {
					auto& [quest, next] = stack.back();
					if (next.empty()) {
						marks[quest] = Mark::kDone;
						stack.pop_back();
						continue;
					}
					const auto other = next.back();
					next.pop_back();
					const auto mark = marks.find(other);
					if (mark == marks.end()) {
						enter(other);
					} else if (mark->second == Mark::kOn) {
						a_edges[quest].erase(other);
					}
				}
			}
		}

		const Graph& GraphOf()
		{
			if (graph) {
				return *graph;
			}
			auto& made = graph.emplace();
			std::unordered_map<RE::FormID, std::vector<After>> after;
			for (const auto& [quest, facts] : state->facts) {
				for (const auto& [other, verdict] : facts.calls) {
					if (verdict & kStarts) {
						after[quest].push_back({ other, false });
					} else if (verdict & kCloses) {
						after[other].push_back({ quest, true });
					}
				}
			}
			Edges edges;
			for (const auto& [quest, facts] : state->facts) {
				if (Shown(quest)) {
					if (auto next = ShownAfter(after, quest); !next.empty()) {
						edges[quest] = std::move(next);
					}
				}
			}
			for (auto& [quest, next] : edges) {
				for (const auto other : std::vector<RE::FormID>(next.begin(), next.end())) {
					if (const auto back = edges.find(other); back != edges.end() && back->second.contains(quest)) {
						next.erase(other);
						back->second.erase(quest);
					}
				}
			}
			BreakLoops(edges);
			for (auto& [quest, next] : edges) {
				for (const auto other : std::vector<RE::FormID>(next.begin(), next.end())) {
					if (Reaches(edges, quest, other)) {
						next.erase(other);
					}
				}
			}
			std::size_t count = 0;
			for (const auto& [quest, next] : edges) {
				for (const auto other : next) {
					made.next[quest].push_back(other);
					made.back[other].push_back(quest);
					count++;
				}
			}
			logger::info("quest chains: {} links between {} quests", count, made.next.size());
			return made;
		}

		// the longest chain from a quest on, the line that goes furthest is shown first
		std::size_t Depth(const std::unordered_map<RE::FormID, std::vector<RE::FormID>>& a_links, RE::FormID a_quest, std::unordered_map<RE::FormID, std::size_t>& a_known, std::size_t a_deep = 0)
		{
			if (const auto known = a_known.find(a_quest); known != a_known.end()) {
				return known->second;
			}
			std::size_t depth = 0;
			if (const auto next = a_links.find(a_quest); next != a_links.end() && a_deep < kLongestChain) {
				for (const auto other : next->second) {
					depth = std::max(depth, Depth(a_links, other, a_known, a_deep + 1) + 1);
				}
			}
			a_known[a_quest] = depth;
			return depth;
		}

		void Walk(const std::unordered_map<RE::FormID, std::vector<RE::FormID>>& a_links, RE::FormID a_from, std::vector<RE::FormID>& a_path,
			std::vector<std::vector<RE::FormID>>& a_chains, std::size_t a_most, std::unordered_map<RE::FormID, std::size_t>& a_depths)
		{
			std::vector<RE::FormID> next;
			if (const auto found = a_links.find(a_from); found != a_links.end() && a_path.size() < kLongestChain) {
				next = found->second;
			}
			std::erase_if(next, [&](RE::FormID a_quest) { return std::ranges::find(a_path, a_quest) != a_path.end(); });
			std::ranges::stable_sort(next, [&](RE::FormID a_one, RE::FormID a_other) { return Depth(a_links, a_one, a_depths) > Depth(a_links, a_other, a_depths); });
			if (next.empty()) {
				if (!a_path.empty() && a_chains.size() < a_most) {
					a_chains.push_back(a_path);
				}
				return;
			}
			for (const auto quest : next) {
				if (a_chains.size() >= a_most) {
					return;
				}
				a_path.push_back(quest);
				Walk(a_links, quest, a_path, a_chains, a_most, a_depths);
				a_path.pop_back();
			}
		}

		void PutQuestLink(RE::GFxMovie& a_movie, RE::GFxValue& a_list, const RE::TESForm& a_quest)
		{
			const auto* quest = a_quest.As<RE::TESQuest>();
			RE::GFxValue one;
			a_movie.CreateObject(&one);
			one.SetMember("name", BrowserSources::NameOf(a_quest).c_str());
			one.SetMember("id", static_cast<double>(BrowserSources::LinkOf(&a_quest)));
			one.SetMember("state", static_cast<double>(quest ? BrowserQuests::StateOf(*quest) : 0));
			a_list.PushBack(one);
		}

		// the chains before a quest and after it, the quest itself left out of each
		void PutChains(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESQuest& a_quest, bool a_whole)
		{
			const auto& links = GraphOf();
			for (const auto& [name, edges, reversed] : { std::tuple{ "before", &links.back, true }, std::tuple{ "after", &links.next, false } }) {
				std::vector<std::vector<RE::FormID>> chains;
				std::vector<RE::FormID> path;
				std::unordered_map<RE::FormID, std::size_t> depths;
				// counted up to the cap whether shown or not, the page says how many more there are
				Walk(*edges, a_quest.GetFormID(), path, chains, kChainsWhole, depths);
				const auto sent = a_whole ? chains.size() : std::min(chains.size(), kChainsShown);
				RE::GFxValue list;
				a_movie.CreateArray(&list);
				for (std::size_t i = 0; i < sent; ++i) {
					if (reversed) {
						std::ranges::reverse(chains[i]);
					}
					RE::GFxValue chain;
					a_movie.CreateArray(&chain);
					for (const auto id : chains[i]) {
						if (const auto* quest = RE::TESForm::LookupByID(id)) {
							PutQuestLink(a_movie, chain, *quest);
						}
					}
					list.PushBack(chain);
				}
				a_object.SetMember(name, list);
				a_object.SetMember(std::format("{}More", name).c_str(), static_cast<double>(chains.size() - sent));
				a_object.SetMember(std::format("{}Shown", name).c_str(), static_cast<double>(kChainsShown));
			}
		}

		// The game keeps the quest's stages in a list that lies in the quest itself at 0xE8, where CommonLib puts two
		// pointers, executedStages and waitingStages. Each node points to a stage with its bit of being done, the way
		// GetStageDone reads it (function 25011)
		struct StageNode
		{
			const RE::TESQuestStage* stage;
			const StageNode*         next;
		};

		std::vector<const RE::TESQuestStage*> StagesOf(const RE::TESQuest& a_quest)
		{
			std::vector<const RE::TESQuestStage*> stages;
			for (const auto* node = reinterpret_cast<const StageNode*>(&a_quest.executedStages); node; node = node->next) {
				if (node->stage) {
					stages.push_back(node->stage);
				}
			}
			return stages;
		}

		bool Done(const RE::TESQuestStage& a_stage)
		{
			return (a_stage.data.flags.underlying() & 0x1) != 0;
		}

		bool StageDone(const RE::TESQuest& a_quest, std::uint16_t a_stage)
		{
			for (const auto* stage : StagesOf(a_quest)) {
				if (stage->data.index == a_stage) {
					return Done(*stage);
				}
			}
			return false;
		}

		void PutSteps(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESQuest& a_quest, const Facts& a_facts)
		{
			RE::GFxValue list;
			a_movie.CreateArray(&list);
			auto& quest = const_cast<RE::TESQuest&>(a_quest);
			for (const auto& stage : a_facts.stages) {
				// a stage with several entries shows the first one, the others tell the same stage for another way
				const auto told = std::ranges::find_if(stage.entries, [](const QuestRecords::Entry& a_entry) { return !a_entry.text.empty(); });
				if (told == stage.entries.end()) {
					continue;
				}
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				one.SetMember("stage", static_cast<double>(stage.index));
				one.SetMember("text", BrowserQuests::Filled(quest, told->text).c_str());
				// a quest stands at stage 0 before anything set it, the stage it stands at is one that was set
				const bool done = StageDone(a_quest, stage.index);
				one.SetMember("done", done);
				one.SetMember("current", done && a_quest.currentStage == stage.index);
				one.SetMember("finishes", std::ranges::any_of(stage.entries, &QuestRecords::Entry::finishes));
				one.SetMember("fails", std::ranges::any_of(stage.entries, &QuestRecords::Entry::fails));
				list.PushBack(one);
			}
			a_object.SetMember("steps", list);
		}

		// the things a list may give, each once, lists inside it walked
		void Leaves(const RE::TESForm* a_form, std::vector<const RE::TESForm*>& a_leaves, std::unordered_set<const RE::TESForm*>& a_seen, int a_depth = 0)
		{
			if (!a_form || a_leaves.size() >= kMostOptions || !a_seen.insert(a_form).second) {
				return;
			}
			if (const auto* list = a_form->As<RE::TESLevItem>()) {
				for (const auto& entry : list->entries) {
					if (a_depth < kDeepest) {
						Leaves(entry.form, a_leaves, a_seen, a_depth + 1);
					}
				}
				return;
			}
			if (const char* name = a_form->GetName(); name && *name) {
				a_leaves.push_back(a_form);
			}
		}

		// the thing an alias makes or is forced on, the same in every game
		const RE::TESForm* FixedHeldBy(const RE::TESQuest& a_quest, std::uint32_t a_alias)
		{
			for (const auto* alias : a_quest.aliases) {
				if (!alias || alias->aliasID != a_alias || alias->GetVMTypeID() != RE::BGSRefAlias::VMTYPEID) {
					continue;
				}
				const auto* reference = static_cast<const RE::BGSRefAlias*>(alias);
				if (alias->fillType.get() == RE::BGSBaseAlias::FILL_TYPE::kCreated) {
					return reference->fillData.created.object;
				}
				if (alias->fillType.get() == RE::BGSBaseAlias::FILL_TYPE::kForced) {
					const auto forced = reference->fillData.forced.forcedRef.get();
					return forced ? forced->GetBaseObject() : nullptr;
				}
			}
			return nullptr;
		}

		// the thing an alias gives is the one it makes or is forced on, or the one it holds now
		const RE::TESForm* HeldBy(const RE::TESQuest& a_quest, std::uint32_t a_alias)
		{
			for (const auto* alias : a_quest.aliases) {
				if (!alias || alias->aliasID != a_alias || alias->GetVMTypeID() != RE::BGSRefAlias::VMTYPEID) {
					continue;
				}
				const auto* reference = static_cast<const RE::BGSRefAlias*>(alias);
				if (alias->fillType.get() == RE::BGSBaseAlias::FILL_TYPE::kCreated && reference->fillData.created.object) {
					return reference->fillData.created.object;
				}
				if (alias->fillType.get() == RE::BGSBaseAlias::FILL_TYPE::kForced) {
					const auto forced = reference->fillData.forced.forcedRef.get();
					if (forced && forced->GetBaseObject()) {
						return forced->GetBaseObject();
					}
				}
				RE::ObjectRefHandle handle;
				{
					const RE::BSReadLockGuard locker{ a_quest.aliasAccessLock };
					if (const auto found = a_quest.refAliasMap.find(a_alias); found != a_quest.refAliasMap.end()) {
						handle = found->second;
					}
				}
				const auto held = handle.get();
				return held ? held->GetBaseObject() : nullptr;
			}
			return nullptr;
		}

		const RE::TESForm* ThingOf(const Gift& a_gift)
		{
			if (a_gift.kind == Gift::Kind::kHeld) {
				const auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(a_gift.form);
				return quest ? HeldBy(*quest, a_gift.alias) : nullptr;
			}
			return RE::TESForm::LookupByID(a_gift.form);
		}

		std::string SkillName(const std::string& a_skill)
		{
			const auto value = RE::ActorValueList::LookupActorValueByName(a_skill.c_str());
			const auto* info = value != RE::ActorValue::kNone ? RE::ActorValueList::GetActorValueInfo(value) : nullptr;
			const char* name = info ? info->GetFullName() : nullptr;
			return name && *name ? name : a_skill;
		}

		void PutGift(RE::GFxMovie& a_movie, RE::GFxValue& a_list, const Gift& a_gift, bool a_whole)
		{
			static constexpr std::array<const char*, 9> kKinds{ "item", "item", "spell", "perk", "shout", "word", "skill", "perkPoints", "souls" };
			RE::GFxValue one;
			a_movie.CreateObject(&one);
			one.SetMember("kind", kKinds[static_cast<std::size_t>(a_gift.kind)]);
			auto count = a_gift.count;
			if (const auto* global = a_gift.countOf ? RE::TESForm::LookupByID<RE::TESGlobal>(a_gift.countOf) : nullptr) {
				count = static_cast<std::int32_t>(global->value);
			}
			one.SetMember("count", static_cast<double>(count));
			one.SetMember("stage", static_cast<double>(a_gift.stage));
			one.SetMember("dialogue", a_gift.dialogue);
			if (a_gift.kind == Gift::Kind::kSkill || a_gift.kind == Gift::Kind::kSouls) {
				one.SetMember("name", SkillName(a_gift.skill).c_str());
				a_list.PushBack(one);
				return;
			}
			if (a_gift.kind == Gift::Kind::kPerkPoints) {
				a_list.PushBack(one);
				return;
			}
			const auto* thing = ThingOf(a_gift);
			if (!thing) {
				return;
			}
			if (const auto* list = thing->As<RE::TESLevItem>()) {
				std::vector<const RE::TESForm*> leaves;
				std::unordered_set<const RE::TESForm*> seen;
				Leaves(thing, leaves, seen);
				if (leaves.empty()) {
					return;
				}
				// a list of one thing, gold by the level, is that thing in as many as its lines give
				if (leaves.size() == 1) {
					std::uint16_t fewest = 0xFFFF;
					std::uint16_t most = 0;
					for (const auto& entry : list->entries) {
						if (entry.form == leaves[0]) {
							fewest = std::min(fewest, entry.count);
							most = std::max(most, entry.count);
						}
					}
					const auto times = std::max(count, 1);
					one.SetMember("name", BrowserSources::NameOf(*leaves[0]).c_str());
					one.SetMember("id", static_cast<double>(BrowserSources::LinkOf(leaves[0])));
					if (most > 0) {
						one.SetMember("count", static_cast<double>(fewest * times));
						one.SetMember("countHigh", static_cast<double>(most * times));
					}
					a_list.PushBack(one);
					return;
				}
				// things of one name, the same necklace at two levels, are one choice of the list
				std::vector<std::pair<std::string, RE::FormID>> names;
				for (const auto* leaf : leaves) {
					auto name = BrowserSources::NameOf(*leaf);
					if (std::ranges::find(names, name, &std::pair<std::string, RE::FormID>::first) == names.end()) {
						names.emplace_back(std::move(name), BrowserSources::LinkOf(leaf));
					}
				}
				RE::GFxValue options;
				a_movie.CreateArray(&options);
				const auto shown = a_whole ? kMostOptions : kOptionsShown;
				for (std::size_t i = 0; i < names.size() && i < shown; ++i) {
					RE::GFxValue option;
					a_movie.CreateObject(&option);
					option.SetMember("name", names[i].first.c_str());
					option.SetMember("id", static_cast<double>(names[i].second));
					options.PushBack(option);
				}
				one.SetMember("options", options);
				one.SetMember("optionsMore", static_cast<double>(names.size() > shown ? names.size() - shown : 0));
				one.SetMember("optionsShown", static_cast<double>(kOptionsShown));
				a_list.PushBack(one);
				return;
			}
			const char* name = thing->GetName();
			if (!name || !*name) {
				return;
			}
			one.SetMember("name", BrowserSources::NameOf(*thing).c_str());
			one.SetMember("id", static_cast<double>(BrowserSources::LinkOf(thing)));
			if (const auto* word = thing->As<RE::TESWordOfPower>(); word && !word->translation.empty()) {
				one.SetMember("means", word->translation.c_str());
			}
			a_list.PushBack(one);
		}

		// The places of the game a quest touches, each the nearest place up that has an entry of its own: the locations
		// its scripts name and its aliases are forced to, the places of the references its scripts and aliases hold
		std::optional<std::unordered_map<RE::FormID, std::vector<const RE::BGSLocation*>>> questPlaces;
		bool questPlacesWhole = false;
		std::unordered_map<const RE::BGSLocation*, std::vector<RE::FormID>> placeQuests;

		const RE::BGSLocation* Named(const RE::BGSLocation* a_location)
		{
			for (int depth = 0; a_location && depth < 16; ++depth, a_location = a_location->parentLoc) {
				if (BrowserIndex::Find(a_location->GetFormID())) {
					return a_location;
				}
			}
			return nullptr;
		}

		const RE::BGSLocation* PlaceOf(RE::FormID a_form)
		{
			auto* form = RE::TESForm::LookupByID(a_form);
			if (!form) {
				const auto* spot = BrowserFiles::SpotOfReference(a_form);
				return spot ? Named(Places::At(spot->cell, spot->world, spot->position).location) : nullptr;
			}
			if (const auto* location = form->As<RE::BGSLocation>()) {
				return Named(location);
			}
			const auto* reference = form->As<RE::TESObjectREFR>();
			return reference && !reference->IsPlayerRef() ? Named(Places::LocationOf(*reference)) : nullptr;
		}

		// Made once the plugin files are in, the places of references of unloaded cells come from them. Made again
		// on every ask until then
		void ReadQuestPlaces()
		{
			if (questPlaces && questPlacesWhole) {
				return;
			}
			questPlacesWhole = BrowserFiles::Ready();
			static const auto work = Profile::Work("quest places");
			const Profile::Timer timer{ work };
			placeQuests.clear();
			auto& places = questPlaces.emplace();
			for (const auto& item : BrowserIndex::Items()) {
				const auto* quest = item.form->As<RE::TESQuest>();
				const auto* facts = quest ? FactsOf(*quest) : nullptr;
				if (!facts) {
					continue;
				}
				std::vector<RE::FormID> forms = facts->pointed;
				const auto& ties = BrowserQuests::TiesOf(*quest);
				forms.insert(forms.end(), ties.references.begin(), ties.references.end());
				auto& found = places[quest->GetFormID()];
				for (const auto form : forms) {
					if (const auto* place = PlaceOf(form); place && std::ranges::find(found, place) == found.end()) {
						found.push_back(place);
					}
				}
				for (const auto* place : found) {
					placeQuests[place].push_back(quest->GetFormID());
				}
			}
			if (BrowserFiles::Ready()) {
				logger::info("quest places: {} places touched by quests", placeQuests.size());
			}
		}

		void PutPlaces(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESQuest& a_quest, bool a_whole)
		{
			ReadQuestPlaces();
			RE::GFxValue list;
			a_movie.CreateArray(&list);
			const auto found = questPlaces->find(a_quest.GetFormID());
			const auto total = found != questPlaces->end() ? found->second.size() : 0;
			const auto sent = a_whole ? std::min(total, kWholeList) : std::min(total, kPlacesShown);
			for (std::size_t i = 0; i < sent; ++i) {
				const auto* entry = BrowserIndex::Find(found->second[i]->GetFormID());
				RE::GFxValue one;
				a_movie.CreateObject(&one);
				one.SetMember("name", entry->name.c_str());
				one.SetMember("id", static_cast<double>(entry->form->GetFormID()));
				one.SetMember("count", 1.0);
				list.PushBack(one);
			}
			a_object.SetMember("locations", list);
			a_object.SetMember("locationsMore", static_cast<double>(total - sent));
			a_object.SetMember("locationsShown", static_cast<double>(kPlacesShown));
		}

		// a thing by the quests that give it, the thing of a list too
		std::optional<std::unordered_map<RE::FormID, std::vector<std::pair<RE::FormID, bool>>>> rewarded;

		void ReadRewarded()
		{
			if (rewarded) {
				return;
			}
			auto& made = rewarded.emplace();
			for (const auto& [quest, facts] : state->facts) {
				if (!Shown(quest)) {
					continue;
				}
				for (const auto& gift : facts.gifts) {
					if (gift.kind != Gift::Kind::kItem && gift.kind != Gift::Kind::kHeld) {
						continue;
					}
					const auto* holder = gift.kind == Gift::Kind::kHeld ? RE::TESForm::LookupByID<RE::TESQuest>(gift.form) : nullptr;
					const auto* thing = gift.kind == Gift::Kind::kHeld ? (holder ? FixedHeldBy(*holder, gift.alias) : nullptr) : ThingOf(gift);
					if (!thing) {
						continue;
					}
					std::vector<const RE::TESForm*> leaves;
					std::unordered_set<const RE::TESForm*> seen;
					Leaves(thing, leaves, seen);
					const bool list = thing->As<RE::TESLevItem>() != nullptr;
					for (const auto* leaf : leaves) {
						auto& quests = made[leaf->GetFormID()];
						if (std::ranges::find(quests, quest, &std::pair<RE::FormID, bool>::first) == quests.end()) {
							quests.emplace_back(quest, list);
						}
					}
				}
			}
		}
	}

	namespace
	{
		std::string EditorID(RE::FormID a_quest)
		{
			const auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(a_quest);
			return quest && !quest->formEditorID.empty() ? std::string(quest->formEditorID.c_str()) : std::format("{:08X}", a_quest);
		}

		// What a bug report needs to see of a quest's page: each link with the hidden quests it goes through, and the
		// stages the game keeps for the quest, a done one with +
		void Tell(const RE::TESQuest& a_quest)
		{
			const auto& links = GraphOf();
			std::string said;
			for (const auto& [name, edges, back] : { std::tuple{ "after", &links.next, false }, std::tuple{ "before", &links.back, true } }) {
				const auto found = edges->find(a_quest.GetFormID());
				for (const auto other : found != edges->end() ? found->second : std::vector<RE::FormID>{}) {
					said += std::format(" {} {}", name, EditorID(other));
					const auto hidden = through.find(back ? Pair(other, a_quest.GetFormID()) : Pair(a_quest.GetFormID(), other));
					for (const auto one : hidden != through.end() ? hidden->second : std::vector<RE::FormID>{}) {
						said += std::format(" <{}", EditorID(one));
					}
				}
			}
			std::string stages;
			for (const auto* stage : StagesOf(a_quest)) {
				stages += std::format(" {}{}", stage->data.index, Done(*stage) ? "+" : "");
			}
			logger::info("quest {} [{:08X}] stage {}, state {}:{}; stages{}", EditorID(a_quest.GetFormID()), a_quest.GetFormID(), a_quest.currentStage,
				BrowserQuests::StateOf(a_quest), said, stages);
		}
	}

	void Start()
	{
		if (state) {
			return;
		}
		state = new State();
		state->sources = PluginSources::LoadOrder();
		state->archives = GameArchives::LoadOrderArchives();
		state->language = GameArchives::Language();
		logger::info("step: the quests' records and scripts are read in the background, {} plugin files, {} archives, language {}", state->sources.size(),
			state->archives.size(), state->language);
		std::thread(Run, state).detach();
	}

	bool Started()
	{
		return state != nullptr;
	}

	bool Ready()
	{
		return state && state->ready;
	}

	void Fill(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESQuest& a_quest, bool a_whole)
	{
		static std::unordered_set<RE::FormID> told;
		Start();
		const auto* facts = FactsOf(a_quest);
		a_object.SetMember("questsRead", facts != nullptr || Ready());
		if (!facts) {
			return;
		}
		PutSteps(a_movie, a_object, a_quest, *facts);
		// a quest that runs and waits for the player can be started by the cheats only with a stage to give it
		if (const auto stage = a_quest.IsRunning() ? BeginningStage(a_quest) : std::nullopt) {
			a_object.SetMember("beginsAt", static_cast<double>(*stage));
		}
		RE::GFxValue gifts;
		a_movie.CreateArray(&gifts);
		for (const auto& gift : facts->gifts) {
			PutGift(a_movie, gifts, gift, a_whole);
		}
		a_object.SetMember("rewards", gifts);
		PutChains(a_movie, a_object, a_quest, a_whole);
		PutPlaces(a_movie, a_object, a_quest, a_whole);
		if (told.insert(a_quest.GetFormID()).second) {
			Tell(a_quest);
		}
	}

	void FillPlace(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::BGSLocation& a_place, bool a_whole)
	{
		Start();
		a_object.SetMember("questsRead", Ready());
		if (!Ready()) {
			return;
		}
		ReadQuestPlaces();
		// the place itself first, then each place right inside it with the quests of everything inside that one
		struct Group
		{
			const RE::BGSLocation* place;
			std::vector<RE::FormID> quests;
		};
		std::vector<Group> groups{ { &a_place, {} } };
		std::unordered_set<RE::FormID> seen;
		for (const auto& [place, quests] : placeQuests) {
			const RE::BGSLocation* within = nullptr;
			int depth = 0;
			for (const auto* up = place; up && !within && depth < 16; up = up->parentLoc, ++depth) {
				if (up == &a_place) {
					within = &a_place;
				} else if (up->parentLoc == &a_place && BrowserIndex::Find(up->GetFormID())) {
					within = up;
				}
			}
			if (!within) {
				continue;
			}
			auto group = std::ranges::find(groups, within, &Group::place);
			if (group == groups.end()) {
				group = groups.insert(groups.end(), { within, {} });
			}
			for (const auto quest : quests) {
				if (std::ranges::find(group->quests, quest) == group->quests.end()) {
					group->quests.push_back(quest);
				}
			}
		}
		std::ranges::stable_sort(groups.begin() + 1, groups.end(), [](const Group& a_one, const Group& a_other) { return a_one.quests.size() > a_other.quests.size(); });
		std::erase_if(groups, [](const Group& a_group) { return a_group.quests.empty(); });
		const auto sent = a_whole ? std::min(groups.size(), kWholeList) : std::min(groups.size(), kGroupsShown);
		RE::GFxValue list;
		a_movie.CreateArray(&list);
		for (std::size_t g = 0; g < sent; ++g) {
			auto& group = groups[g];
			std::ranges::sort(group.quests, [](RE::FormID a_one, RE::FormID a_other) {
				const auto* one = BrowserIndex::Find(a_one);
				const auto* other = BrowserIndex::Find(a_other);
				return BrowserIndex::PlaceOf(*one) < BrowserIndex::PlaceOf(*other);
			});
			RE::GFxValue one;
			a_movie.CreateObject(&one);
			if (group.place != &a_place) {
				const auto* entry = BrowserIndex::Find(group.place->GetFormID());
				one.SetMember("place", entry->name.c_str());
				one.SetMember("id", static_cast<double>(entry->form->GetFormID()));
			}
			RE::GFxValue quests;
			a_movie.CreateArray(&quests);
			const auto shown = a_whole ? std::min(group.quests.size(), kWholeList) : std::min(group.quests.size(), kQuestsShown);
			for (std::size_t i = 0; i < shown; ++i) {
				if (const auto* quest = RE::TESForm::LookupByID(group.quests[i])) {
					PutQuestLink(a_movie, quests, *quest);
				}
			}
			one.SetMember("quests", quests);
			one.SetMember("questsMore", static_cast<double>(group.quests.size() - shown));
			one.SetMember("questsShown", static_cast<double>(kQuestsShown));
			list.PushBack(one);
		}
		a_object.SetMember("questGroups", list);
		a_object.SetMember("questGroupsMore", static_cast<double>(groups.size() - sent));
		a_object.SetMember("questGroupsShown", static_cast<double>(kGroupsShown));
	}

	std::vector<std::pair<const RE::TESQuest*, bool>> RewardedBy(const RE::TESForm& a_thing)
	{
		std::vector<std::pair<const RE::TESQuest*, bool>> quests;
		if (!Ready()) {
			return quests;
		}
		ReadRewarded();
		if (const auto found = rewarded->find(a_thing.GetFormID()); found != rewarded->end()) {
			for (const auto& [id, list] : found->second) {
				if (const auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(id)) {
					quests.emplace_back(quest, list);
				}
			}
		}
		return quests;
	}

	std::string TextsOf(const RE::TESQuest& a_quest)
	{
		std::string texts;
		if (const auto* facts = FactsOf(a_quest)) {
			for (const auto& stage : facts->stages) {
				for (const auto& entry : stage.entries) {
					texts += entry.text;
					texts += '\n';
				}
			}
		}
		return texts;
	}

	void Forget()
	{
		questPlaces.reset();
		questPlacesWhole = false;
		placeQuests.clear();
		rewarded.reset();
	}

	std::optional<std::uint16_t> FinishingStage(const RE::TESQuest& a_quest)
	{
		const auto* facts = FactsOf(a_quest);
		std::optional<std::uint16_t> stage;
		if (!facts) {
			return stage;
		}
		for (const auto& one : facts->stages) {
			if (std::ranges::any_of(one.entries, &QuestRecords::Entry::finishes) && (!stage || one.index < *stage)) {
				stage = one.index;
			}
		}
		return stage;
	}

	std::optional<std::uint16_t> BeginningStage(const RE::TESQuest& a_quest)
	{
		const auto* facts = FactsOf(a_quest);
		std::optional<std::uint16_t> stage;
		if (!facts) {
			return stage;
		}
		const auto tells = [](const QuestRecords::Entry& a_entry) { return !a_entry.text.empty() && !a_entry.finishes && !a_entry.fails; };
		for (const auto& one : facts->stages) {
			if (one.index > a_quest.currentStage && std::ranges::any_of(one.entries, tells) && (!stage || one.index < *stage)) {
				stage = one.index;
			}
		}
		return stage;
	}
}
