#include "PCH.h"

#include "QuestRecords.h"

#include "GameArchives.h"
#include "PluginRecords.h"

namespace QuestRecords
{
	namespace
	{
		using PluginSources::Source;

		using PluginRecords::Fields;
		using PluginRecords::kCompressed;
		using PluginRecords::kHeader;
		using PluginRecords::Tag;
		using PluginRecords::Take;
		using PluginRecords::Zero;

		constexpr std::int32_t kTopicChildren = 7;

		std::string Lower(std::string_view a_text)
		{
			return GameArchives::Key(a_text);
		}

		// the scripts and the fragments of a record's VMAD, little endian, every read inside the field
		class Machine
		{
		public:
			Machine(std::span<const std::byte> a_data, const Source& a_source) :
				data(a_data), source(a_source) {}

			[[nodiscard]] bool Good() const { return good; }

			template <class T>
			T Next()
			{
				if (at + sizeof(T) > data.size()) {
					good = false;
					return T{};
				}
				const auto value = Take<T>(data, at);
				at += sizeof(T);
				return value;
			}

			std::string Text()
			{
				const auto size = Next<std::uint16_t>();
				if (!good || at + size > data.size()) {
					good = false;
					return {};
				}
				std::string text(reinterpret_cast<const char*>(data.data() + at), size);
				at += size;
				return text;
			}

			[[nodiscard]] bool AtEnd() const { return at >= data.size(); }

			void Head()
			{
				version = Next<std::int16_t>();
				format = Next<std::int16_t>();
			}

			// a form and the alias of it, in the order of the field's format
			std::pair<RE::FormID, std::int16_t> Object()
			{
				std::uint32_t raw = 0;
				std::int16_t alias = -1;
				if (format == 1) {
					raw = Next<std::uint32_t>();
					alias = Next<std::int16_t>();
					Next<std::uint16_t>();
				} else {
					Next<std::uint16_t>();
					alias = Next<std::int16_t>();
					raw = Next<std::uint32_t>();
				}
				return { raw == 0 ? 0 : source.Runtime(raw), alias };
			}

			Property Value(std::uint8_t a_type)
			{
				Property property;
				property.type = static_cast<Property::Type>(a_type);
				switch (a_type) {
				case 1:
					std::tie(property.form, property.alias) = Object();
					break;
				case 2:
					property.text = Text();
					break;
				case 3:
					property.number = Next<std::int32_t>();
					break;
				case 4:
					property.number = static_cast<std::int32_t>(Next<float>());
					break;
				case 5:
					property.number = Next<std::uint8_t>();
					break;
				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
					{
						const auto count = Next<std::uint32_t>();
						for (std::uint32_t i = 0; i < count && good; ++i) {
							const auto one = Value(static_cast<std::uint8_t>(a_type - 10));
							if (a_type == 11 && one.form != 0) {
								property.forms.push_back(one.form);
							}
						}
						break;
					}
				default:
					good = false;
					break;
				}
				return property;
			}

			std::vector<Instance> Scripts()
			{
				std::vector<Instance> scripts;
				const auto count = Next<std::uint16_t>();
				for (std::uint16_t s = 0; s < count && good; ++s) {
					Instance instance{ Lower(Text()) };
					if (version >= 4) {
						Next<std::uint8_t>();
					}
					const auto properties = Next<std::uint16_t>();
					for (std::uint16_t p = 0; p < properties && good; ++p) {
						auto name = Lower(Text());
						const auto type = Next<std::uint8_t>();
						if (version >= 4) {
							Next<std::uint8_t>();
						}
						instance.properties[std::move(name)] = Value(type);
					}
					scripts.push_back(std::move(instance));
				}
				return scripts;
			}

		private:
			std::span<const std::byte> data;
			const Source& source;
			std::size_t at = 0;
			std::int16_t version = 0;
			std::int16_t format = 2;
			bool good = true;
		};

		struct Info
		{
			RE::FormID topic = 0;
			std::vector<Instance> scripts;
		};

		// an entry by its stage and its place there, the text by its number in the strings file of the plugin
		struct Unnamed
		{
			std::size_t stage = 0;
			std::size_t entry = 0;
			std::uint32_t text = 0;
			std::string stem;
		};

		struct Reading
		{
			Read& read;
			std::unordered_map<RE::FormID, RE::FormID> topicQuests;
			std::unordered_map<RE::FormID, Info> infos;
			// the journal entries of a localized plugin wait for its strings file, by the quest of the plugin that won
			std::unordered_map<RE::FormID, std::vector<Unnamed>> unnamed;
		};

		void ReadQuestVirtualMachine(Quest& a_quest, std::span<const std::byte> a_field, const Source& a_source)
		{
			Machine machine(a_field, a_source);
			machine.Head();
			a_quest.scripts = machine.Scripts();
			if (!machine.Good() || machine.AtEnd()) {
				return;
			}
			machine.Next<std::uint8_t>();
			const auto fragments = machine.Next<std::uint16_t>();
			a_quest.fragments = Lower(machine.Text());
			for (std::uint16_t i = 0; i < fragments && machine.Good(); ++i) {
				const auto stage = machine.Next<std::uint16_t>();
				machine.Next<std::int16_t>();
				machine.Next<std::int32_t>();
				machine.Next<std::uint8_t>();
				machine.Text();
				a_quest.fragmentStages[Lower(machine.Text())].push_back(stage);
			}
			const auto aliases = machine.Next<std::uint16_t>();
			for (std::uint16_t i = 0; i < aliases && machine.Good(); ++i) {
				const auto [form, alias] = machine.Object();
				machine.Next<std::int16_t>();
				machine.Next<std::int16_t>();
				auto scripts = machine.Scripts();
				const auto found = std::ranges::find(a_quest.aliases, static_cast<std::uint32_t>(alias), &Alias::id);
				if (found != a_quest.aliases.end()) {
					found->scripts = std::move(scripts);
				} else {
					a_quest.aliases.push_back({ static_cast<std::uint32_t>(alias), false, 0, 0, 0, 0, 0, std::move(scripts) });
				}
			}
		}

		void ReadQuest(Reading& a_reading, std::span<const std::byte> a_record, RE::FormID a_id, const Source& a_source, bool a_localized, const std::string& a_stem)
		{
			Quest quest{ a_id };
			Stage* stage = nullptr;
			Alias* alias = nullptr;
			// the aliases' scripts may come before their aliases, the VMAD stands first in the record
			std::optional<std::span<const std::byte>> machine;
			std::vector<Unnamed> unnamed;
			Fields(a_record, [&](std::uint32_t a_type, std::span<const std::byte> a_field) {
				switch (a_type) {
				case Tag("EDID"):
					quest.editorID = Zero(a_field);
					break;
				case Tag("VMAD"):
					machine = a_field;
					break;
				case Tag("INDX"):
					stage = &quest.stages.emplace_back();
					stage->index = Take<std::uint16_t>(a_field, 0);
					stage->startUp = (Take<std::uint8_t>(a_field, 2) & 0x2) != 0;
					stage->shutDown = (Take<std::uint8_t>(a_field, 2) & 0x4) != 0;
					alias = nullptr;
					break;
				case Tag("QSDT"):
					if (stage) {
						auto& entry = stage->entries.emplace_back();
						entry.finishes = (Take<std::uint8_t>(a_field, 0) & 0x1) != 0;
						entry.fails = (Take<std::uint8_t>(a_field, 0) & 0x2) != 0;
					}
					break;
				case Tag("CNAM"):
					if (stage && !stage->entries.empty()) {
						if (a_localized) {
							unnamed.push_back({ quest.stages.size() - 1, stage->entries.size() - 1, Take<std::uint32_t>(a_field, 0), a_stem });
						} else {
							stage->entries.back().text = Zero(a_field);
						}
					}
					break;
				case Tag("NAM0"):
					if (stage && !stage->entries.empty()) {
						stage->entries.back().next = a_source.Runtime(Take<std::uint32_t>(a_field, 0));
					}
					break;
				case Tag("QOBJ"):
					stage = nullptr;
					break;
				case Tag("ALST"):
				case Tag("ALLS"):
					stage = nullptr;
					alias = &quest.aliases.emplace_back();
					alias->id = Take<std::uint32_t>(a_field, 0);
					alias->location = a_type == Tag("ALLS");
					break;
				case Tag("FNAM"):
					if (alias) {
						alias->flags = Take<std::uint32_t>(a_field, 0);
					}
					break;
				case Tag("ALFR"):
					if (alias) {
						alias->forcedReference = a_source.Runtime(Take<std::uint32_t>(a_field, 0));
					}
					break;
				case Tag("ALUA"):
					if (alias) {
						alias->uniqueActor = a_source.Runtime(Take<std::uint32_t>(a_field, 0));
					}
					break;
				case Tag("ALCO"):
					if (alias) {
						alias->created = a_source.Runtime(Take<std::uint32_t>(a_field, 0));
					}
					break;
				case Tag("ALFL"):
					if (alias) {
						alias->forcedLocation = a_source.Runtime(Take<std::uint32_t>(a_field, 0));
					}
					break;
				case Tag("ALED"):
					alias = nullptr;
					break;
				default:
					break;
				}
			});
			if (machine) {
				ReadQuestVirtualMachine(quest, *machine, a_source);
			}
			// a later plugin's quest takes the place of the earlier one, the lines of dialogue are joined after all files
			a_reading.read.quests[a_id] = std::move(quest);
			a_reading.unnamed[a_id] = std::move(unnamed);
		}

		void ReadInfo(Reading& a_reading, std::span<const std::byte> a_record, RE::FormID a_id, RE::FormID a_topic, const Source& a_source)
		{
			Info info{ a_topic };
			Fields(a_record, [&](std::uint32_t a_type, std::span<const std::byte> a_field) {
				if (a_type == Tag("VMAD")) {
					Machine machine(a_field, a_source);
					machine.Head();
					info.scripts = machine.Scripts();
				}
			});
			a_reading.read.infos++;
			a_reading.infos[a_id] = std::move(info);
		}

		class Walker
		{
		public:
			Walker(Reading& a_reading, const Source& a_source, std::span<const std::byte> a_data) :
				reading(a_reading), source(a_source), data(a_data), stem(PluginRecords::Stem(a_source)), localized(PluginRecords::Localized(a_data)) {}

			void Walk()
			{
				std::size_t at = 0;
				while (at + kHeader <= data.size()) {
					const auto type = Take<std::uint32_t>(data, at);
					const auto length = Take<std::uint32_t>(data, at + 4);
					if (type != Tag("GRUP")) {
						at += kHeader + length;
						continue;
					}
					if (length < kHeader || at + length > data.size()) {
						return;
					}
					const auto label = Take<std::uint32_t>(data, at + 8);
					if (label == Tag("QUST") || label == Tag("DIAL")) {
						Group(at + kHeader, at + length, 0);
					}
					at += length;
				}
			}

		private:
			void Group(std::size_t a_from, std::size_t a_to, RE::FormID a_topic)
			{
				auto at = a_from;
				while (at + kHeader <= a_to) {
					const auto type = Take<std::uint32_t>(data, at);
					const auto length = Take<std::uint32_t>(data, at + 4);
					if (type == Tag("GRUP")) {
						if (length < kHeader || at + length > a_to) {
							return;
						}
						const auto kind = Take<std::int32_t>(data, at + 12);
						if (kind == kTopicChildren) {
							Group(at + kHeader, at + length, source.Runtime(Take<std::uint32_t>(data, at + 8)));
						}
						at += length;
						continue;
					}
					if (at + kHeader + length > a_to) {
						return;
					}
					const auto flags = Take<std::uint32_t>(data, at + 8);
					const auto id = source.Runtime(Take<std::uint32_t>(data, at + 12));
					const auto record = data.subspan(at + kHeader, length);
					if (flags & kCompressed) {
						reading.read.packed++;
					} else if (type == Tag("QUST")) {
						ReadQuest(reading, record, id, source, localized, stem);
					} else if (type == Tag("DIAL")) {
						Fields(record, [&](std::uint32_t a_type, std::span<const std::byte> a_field) {
							if (a_type == Tag("QNAM")) {
								reading.topicQuests[id] = source.Runtime(Take<std::uint32_t>(a_field, 0));
							}
						});
					} else if (type == Tag("INFO") && a_topic != 0) {
						ReadInfo(reading, record, id, a_topic, source);
					}
					at += kHeader + length;
				}
			}

			Reading& reading;
			const Source& source;
			std::span<const std::byte> data;
			std::string stem;
			bool localized = false;
		};

		void ReadFile(Reading& a_reading, const Source& a_source)
		{
			const bool read = PluginRecords::Map(a_source, [&](std::span<const std::byte> a_data) { Walker(a_reading, a_source, a_data).Walk(); });
			(read ? a_reading.read.files : a_reading.read.unread)++;
		}

		// The journal entries of the localized plugins from their strings files, each file read once
		void NameEntries(Reading& a_reading, const GameArchives::Files& a_files, std::string_view a_language)
		{
			std::unordered_map<std::string, std::vector<std::pair<std::uint32_t, Entry*>>> byFile;
			for (const auto& [quest, entries] : a_reading.unnamed) {
				auto& stages = a_reading.read.quests[quest].stages;
				for (const auto& one : entries) {
					if (one.stage < stages.size() && one.entry < stages[one.stage].entries.size()) {
						byFile[one.stem].emplace_back(one.text, &stages[one.stage].entries[one.entry]);
					}
				}
			}
			for (const auto& [stem, wanted] : byFile) {
				std::vector<std::uint32_t> ids;
				for (const auto& [id, entry] : wanted) {
					ids.push_back(id);
				}
				const auto texts = PluginRecords::Strings(a_files, stem, "dlstrings", a_language, ids);
				for (const auto& [id, entry] : wanted) {
					if (const auto found = texts.find(id); found != texts.end()) {
						entry->text = found->second;
						a_reading.read.texts++;
					}
				}
			}
		}
	}

	Read ReadAll(const std::vector<PluginSources::Source>& a_sources, const GameArchives::Files& a_files, std::string_view a_language)
	{
		Read read;
		Reading reading{ read };
		for (const auto& source : a_sources) {
			try {
				ReadFile(reading, source);
			} catch (const std::exception& a_error) {
				read.unread++;
				logger::warn("quest records: {} left out, {}", GameArchives::Narrow(source.path.filename()), a_error.what());
			}
		}
		NameEntries(reading, a_files, a_language);
		for (auto& [id, info] : reading.infos) {
			const auto quest = reading.topicQuests.find(info.topic);
			if (info.scripts.empty() || quest == reading.topicQuests.end()) {
				continue;
			}
			if (auto known = read.quests.find(quest->second); known != read.quests.end()) {
				for (auto& script : info.scripts) {
					known->second.dialogue.push_back(std::move(script));
				}
			}
		}
		return read;
	}
}
