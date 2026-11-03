#include "PCH.h"

#include "ItemTexts.h"

#include <thread>
#include <variant>

#include "GameArchives.h"
#include "PluginRecords.h"

namespace ItemTexts
{
	namespace
	{
		using PluginRecords::Tag;
		using PluginRecords::Take;

		// the kinds of things whose records carry a description, perks among them
		constexpr std::array<std::uint32_t, 6> kDescribed{ Tag("ARMO"), Tag("WEAP"), Tag("AMMO"), Tag("BOOK"), Tag("SCRL"), Tag("PERK") };

		struct State
		{
			std::vector<PluginSources::Source> sources;
			std::vector<std::string> archives;
			std::string language;
			std::unordered_map<RE::FormID, std::string> texts;
			std::atomic<bool> ready = false;
		};

		// the thread may still read when the game quits, so the state is never freed
		State* state = nullptr;

		// a description in the strings file of its plugin, by its number there
		struct Pending
		{
			std::string stem;
			std::uint32_t id = 0;
		};

		// The big first letter of a book is a picture of it, "img://Textures/Interface/Books/Illuminated_Letters/W_letter.png",
		// the letter stands before "_letter" in the picture's name
		std::string LetterOf(std::string_view a_tag)
		{
			// the folder is Illuminated_Letters too, the letter's own name is the last
			const auto key = GameArchives::Key(a_tag);
			const auto at = key.rfind("_letter");
			return key.starts_with("img") && at != std::string::npos && at >= 2 && (key[at - 2] == '/' || key[at - 2] == '\\') ?
			           std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(key[at - 1])))) :
			           std::string();
		}

		// The markup of a book left out: a tag is gone, a paragraph or a line break is a new line, a page break an
		// empty line, the picture of a big first letter and the few named letters are letters again
		std::string Plain(std::string_view a_text)
		{
			std::string plain;
			for (std::size_t i = 0; i < a_text.size(); ++i) {
				const auto close = a_text[i] == '<' ? a_text.find('>', i) : std::string_view::npos;
				const bool tag = close != std::string_view::npos && i + 1 < a_text.size() && (std::isalpha(static_cast<unsigned char>(a_text[i + 1])) || a_text[i + 1] == '/') &&
				                 a_text.substr(i, close - i).find('\n') == std::string_view::npos;
				if (tag) {
					const auto tag = GameArchives::Key(a_text.substr(i + 1, close - i - 1));
					if (tag.starts_with("br") || tag.starts_with("/p") || tag.starts_with("p ") || tag == "p") {
						plain += '\n';
					}
					plain += LetterOf(a_text.substr(i + 1, close - i - 1));
					i = close;
					continue;
				}
				if (a_text.substr(i).starts_with("[pagebreak]")) {
					plain += "\n\n";
					i += std::string_view("[pagebreak]").size() - 1;
					continue;
				}
				if (a_text[i] == '&') {
					static constexpr std::array<std::pair<std::string_view, char>, 4> kNamed{ { { "&lt;", '<' }, { "&gt;", '>' }, { "&amp;", '&' }, { "&quot;", '"' } } };
					const auto named = std::ranges::find_if(kNamed, [&](const auto& a_one) { return a_text.substr(i).starts_with(a_one.first); });
					if (named != kNamed.end()) {
						plain += named->second;
						i += named->first.size() - 1;
						continue;
					}
				}
				plain += a_text[i] == '\r' ? '\n' : a_text[i];
			}
			// no more than one empty line in a row, none at the ends
			std::string kept;
			std::size_t breaks = 0;
			for (const auto letter : plain) {
				if (letter == '\n') {
					breaks++;
					continue;
				}
				if (!kept.empty() && breaks > 0) {
					kept += breaks > 1 ? "\n\n" : "\n";
				}
				breaks = 0;
				kept += letter;
			}
			const auto first = kept.find_first_not_of(" \t\n");
			const auto last = kept.find_last_not_of(" \t\n");
			return first == std::string::npos ? std::string() : kept.substr(first, last - first + 1);
		}

		using Found = std::unordered_map<RE::FormID, std::variant<std::string, Pending>>;

		// the descriptions of the groups of described things of one file, a later file's record takes the place of
		// the earlier one
		void Walk(const PluginSources::Source& a_source, std::span<const std::byte> a_data, Found& a_found, std::size_t& a_packed)
		{
			const bool localized = PluginRecords::Localized(a_data);
			const auto stem = PluginRecords::Stem(a_source);
			std::size_t at = 0;
			while (at + PluginRecords::kHeader <= a_data.size()) {
				const auto type = Take<std::uint32_t>(a_data, at);
				const auto length = Take<std::uint32_t>(a_data, at + 4);
				if (type != Tag("GRUP")) {
					at += PluginRecords::kHeader + length;
					continue;
				}
				if (length < PluginRecords::kHeader || at + length > a_data.size()) {
					return;
				}
				if (std::ranges::find(kDescribed, Take<std::uint32_t>(a_data, at + 8)) == kDescribed.end()) {
					at += length;
					continue;
				}
				const auto end = at + length;
				auto record = at + PluginRecords::kHeader;
				while (record + PluginRecords::kHeader <= end) {
					const auto size = Take<std::uint32_t>(a_data, record + 4);
					const auto flags = Take<std::uint32_t>(a_data, record + 8);
					const auto id = a_source.Runtime(Take<std::uint32_t>(a_data, record + 12));
					if (record + PluginRecords::kHeader + size > end) {
						break;
					}
					if (flags & PluginRecords::kCompressed) {
						a_packed++;
					} else {
						// a record of a later plugin without a description takes the earlier one away
						a_found.erase(id);
						PluginRecords::Fields(a_data.subspan(record + PluginRecords::kHeader, size), [&](std::uint32_t a_type, std::span<const std::byte> a_field) {
							if (a_type != Tag("DESC")) {
								return;
							}
							if (localized) {
								a_found[id] = Pending{ stem, Take<std::uint32_t>(a_field, 0) };
							} else {
								a_found[id] = PluginRecords::Zero(a_field);
							}
						});
					}
					record += PluginRecords::kHeader + size;
				}
				at += length;
			}
		}

		void Run(State* a_state)
		{
			try {
				const auto start = std::chrono::steady_clock::now();
				GameArchives::Files files(a_state->archives, { "strings" });
				Found found;
				std::size_t packed = 0;
				for (const auto& source : a_state->sources) {
					// a broken file is left out, the others still count
					try {
						PluginRecords::Map(source, [&](std::span<const std::byte> a_data) { Walk(source, a_data, found, packed); });
					} catch (const std::exception& a_error) {
						logger::warn("descriptions: {} left out, {}", GameArchives::Narrow(source.path.filename()), a_error.what());
					}
				}
				std::unordered_map<std::string, std::vector<std::pair<std::uint32_t, RE::FormID>>> byFile;
				for (const auto& [id, text] : found) {
					if (const auto* pending = std::get_if<Pending>(&text)) {
						byFile[pending->stem].emplace_back(pending->id, id);
					} else if (auto plain = Plain(std::get<std::string>(text)); !plain.empty()) {
						a_state->texts.emplace(id, std::move(plain));
					}
				}
				for (const auto& [stem, wanted] : byFile) {
					std::vector<std::uint32_t> ids;
					for (const auto& [text, form] : wanted) {
						ids.push_back(text);
					}
					const auto texts = PluginRecords::Strings(files, stem, "dlstrings", a_state->language, ids);
					for (const auto& [text, form] : wanted) {
						if (const auto said = texts.find(text); said != texts.end()) {
							if (auto plain = Plain(said->second); !plain.empty()) {
								a_state->texts.emplace(form, std::move(plain));
							}
						}
					}
				}
				const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
				logger::info("descriptions: {} things described in {:.0f} ms, {} packed records left out", a_state->texts.size(), took, packed);
			} catch (const std::exception& a_error) {
				logger::error("descriptions: the reading stopped, {}", a_error.what());
			}
			a_state->ready = true;
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
		logger::info("step: the descriptions of things are read in the background");
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

	std::string_view Of(RE::FormID a_form)
	{
		if (!Ready()) {
			return {};
		}
		const auto found = state->texts.find(a_form);
		return found != state->texts.end() ? std::string_view(found->second) : std::string_view();
	}
}
