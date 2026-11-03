#include "HandoutLines.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>

namespace HandoutLines
{
	namespace
	{
		constexpr std::array<std::string_view, 12> kKinds{ "Form", "Spell", "Perk", "Item", "Shout", "LevSpell", "Package", "Outfit", "Keyword", "Faction", "SleepOutfit", "Skin" };

		std::string_view Trim(std::string_view a_text)
		{
			const auto first = a_text.find_first_not_of(" \t\r\n");
			if (first == std::string_view::npos) {
				return {};
			}
			return a_text.substr(first, a_text.find_last_not_of(" \t\r\n") - first + 1);
		}

		bool Same(std::string_view a_one, std::string_view a_other)
		{
			return a_one.size() == a_other.size() && std::ranges::equal(a_one, a_other, [](unsigned char a_a, unsigned char a_b) {
				return std::tolower(a_a) == std::tolower(a_b);
			});
		}

		// an empty section and NONE say "no filter"
		bool Given(std::string_view a_text)
		{
			return !a_text.empty() && !Same(a_text, "NONE");
		}

		// the pieces between the marks, trimmed, empty ones left out
		std::vector<std::string> Split(std::string_view a_text, std::string_view a_mark)
		{
			std::vector<std::string> pieces;
			while (true) {
				const auto at = a_text.find(a_mark);
				if (const auto piece = Trim(a_text.substr(0, at)); !piece.empty()) {
					pieces.emplace_back(piece);
				}
				if (at == std::string_view::npos) {
					return pieces;
				}
				a_text.remove_prefix(at + a_mark.size());
			}
		}

		// the sections of a line in order, empty ones kept, so the n-th section stays the n-th
		std::vector<std::string> Sections(std::string_view a_text)
		{
			std::vector<std::string> sections;
			while (true) {
				const auto at = a_text.find('|');
				sections.emplace_back(Trim(a_text.substr(0, at)));
				if (at == std::string_view::npos) {
					return sections;
				}
				a_text.remove_prefix(at + 1);
			}
		}

		std::optional<std::uint32_t> Hex(std::string_view a_text, bool a_prefixed)
		{
			const bool prefix = a_text.starts_with("0x") || a_text.starts_with("0X");
			if (prefix) {
				a_text.remove_prefix(2);
			} else if (a_prefixed) {
				return std::nullopt;
			}
			std::uint32_t value = 0;
			const auto [end, error] = std::from_chars(a_text.data(), a_text.data() + a_text.size(), value, 16);
			if (a_text.empty() || error != std::errc{} || end != a_text.data() + a_text.size()) {
				return std::nullopt;
			}
			return value;
		}

		std::optional<float> Number(std::string_view a_text)
		{
			float value = 0.0f;
			const auto [end, error] = std::from_chars(a_text.data(), a_text.data() + a_text.size(), value);
			if (a_text.empty() || error != std::errc{}) {
				return std::nullopt;
			}
			return value;
		}

		// the whole numbers of a text in order, "14(50/100)" gives 14, 50 and 100
		std::vector<std::uint32_t> Numbers(std::string_view a_text)
		{
			std::vector<std::uint32_t> numbers;
			std::size_t at = 0;
			while (at < a_text.size()) {
				if (!std::isdigit(static_cast<unsigned char>(a_text[at]))) {
					at++;
					continue;
				}
				std::uint32_t value = 0;
				const auto [end, error] = std::from_chars(a_text.data() + at, a_text.data() + a_text.size(), value);
				numbers.push_back(value);
				at = static_cast<std::size_t>(end - a_text.data());
			}
			return numbers;
		}

		bool TakePrefix(std::string_view& a_text, std::string_view a_prefix)
		{
			if (!a_text.starts_with(a_prefix)) {
				return false;
			}
			a_text.remove_prefix(a_prefix.size());
			return true;
		}

		// what SPID's sanitizing turns an old line into: "0x12345 - Mod.esp" names a form of the mod
		std::string Sanitized(std::string_view a_value)
		{
			std::string value(a_value);
			if (value.find('~') == std::string::npos) {
				if (const auto dash = value.find(" - "); dash != std::string::npos) {
					value.replace(dash, 3, "~");
				}
			}
			return value;
		}

		void ReadWords(Gift& a_gift, std::string_view a_text)
		{
			if (!Given(a_text)) {
				return;
			}
			for (auto& word : Split(a_text, ",")) {
				if (word.find('+') != std::string::npos) {
					for (auto& part : Split(word, "+")) {
						a_gift.wordsAll.push_back(std::move(part));
					}
				} else if (word.starts_with('-')) {
					a_gift.wordsNone.push_back(word.substr(1));
				} else if (word.starts_with('*')) {
					a_gift.wordsPart.push_back(word.substr(1));
				} else {
					a_gift.wordsMatch.push_back(std::move(word));
				}
			}
		}

		void ReadForms(Gift& a_gift, std::string_view a_text, bool a_plain)
		{
			if (!Given(a_text)) {
				return;
			}
			for (auto& form : Split(a_text, ",")) {
				if (!a_plain && form.find('+') != std::string::npos) {
					for (auto& part : Split(form, "+")) {
						a_gift.formsAll.push_back(RecordOf(part));
					}
				} else if (!a_plain && form.starts_with('-')) {
					a_gift.formsNone.push_back(RecordOf(std::string_view(form).substr(1)));
				} else {
					a_gift.formsMatch.push_back(RecordOf(form));
				}
			}
		}

		void ReadLevels(Gift& a_gift, std::string_view a_text)
		{
			if (!Given(a_text)) {
				return;
			}
			for (const auto& level : Split(a_text, ",")) {
				if (level.find('(') != std::string::npos) {
					// a skill and its range, "w" before it asks for the weight of the skill in the person's class
					const auto numbers = Numbers(level);
					if (numbers.size() >= 2 && numbers[0] < 18) {
						Skill skill{ numbers[0], { static_cast<std::uint16_t>(numbers[1]), static_cast<std::uint16_t>(numbers.size() > 2 ? numbers[2] : 0xFF) }, level.starts_with('w') };
						a_gift.skills.push_back(skill);
					}
					continue;
				}
				const auto numbers = Numbers(level);
				if (!numbers.empty()) {
					a_gift.level = { static_cast<std::uint16_t>(numbers[0]), static_cast<std::uint16_t>(level.find('/') != std::string::npos && numbers.size() > 1 ? numbers[1] : 0xFFFF) };
				}
			}
		}

		void ReadTraits(Gift& a_gift, std::string_view a_text)
		{
			if (!Given(a_text)) {
				return;
			}
			for (const auto& trait : Split(a_text, "/")) {
				const bool no = trait.starts_with('-');
				const std::string_view letter = no ? std::string_view(trait).substr(1) : std::string_view(trait);
				if (letter == "M" || letter == "F") {
					a_gift.female = (letter == "F") != no;
				} else if (letter == "U") {
					a_gift.unique = !no;
				} else if (letter == "S") {
					a_gift.summonable = !no;
				} else if (letter == "C") {
					a_gift.child = !no;
				} else if (letter == "L") {
					a_gift.leveled = !no;
				} else if (letter == "T") {
					a_gift.teammate = !no;
				} else if (letter == "D") {
					a_gift.dead = !no;
				}
			}
		}

		std::optional<float> ReadChance(std::string_view a_text)
		{
			if (!Given(a_text)) {
				return std::nullopt;
			}
			// a "!" at the end makes the roll the same in every game
			if (a_text.ends_with('!')) {
				a_text.remove_suffix(1);
			}
			return Number(a_text);
		}

		bool KnownKind(std::string_view a_kind)
		{
			return std::ranges::find(kKinds, a_kind) != kKinds.end();
		}

		void AddFilters(std::string& a_filters, std::string_view a_section)
		{
			if (Given(a_section)) {
				a_filters += a_filters.empty() ? "" : " | ";
				a_filters += a_section;
			}
		}
	}

	std::vector<Line> ReadIni(std::string_view a_text)
	{
		if (a_text.starts_with("\xEF\xBB\xBF")) {
			a_text.remove_prefix(3);
		}
		std::vector<Line> lines;
		std::string section;
		while (!a_text.empty()) {
			const auto end = a_text.find('\n');
			const auto line = Trim(a_text.substr(0, end));
			a_text = end == std::string_view::npos ? std::string_view{} : a_text.substr(end + 1);
			if (line.empty() || line.front() == ';' || line.front() == '#') {
				continue;
			}
			if (line.front() == '[') {
				section = Trim(line.substr(1, line.find(']') - 1));
				continue;
			}
			const auto equals = line.find('=');
			lines.push_back({ section, std::string(Trim(line.substr(0, equals))), equals == std::string_view::npos ? std::string() : std::string(Trim(line.substr(equals + 1))) });
		}
		return lines;
	}

	Record RecordOf(std::string_view a_text)
	{
		a_text = Trim(a_text);
		Record record;
		if (const auto tilde = a_text.find('~'); tilde != std::string_view::npos) {
			record.id = Hex(Trim(a_text.substr(0, tilde)), false);
			record.plugin = Trim(a_text.substr(tilde + 1));
		} else if (a_text.find(".es") != std::string_view::npos) {
			record.plugin = a_text;
		} else if (const auto id = Hex(a_text, !a_text.starts_with("00"))) {
			// SPID reads "00012345" as 0x12345
			record.id = id;
		} else {
			record.editorID = a_text;
		}
		return record;
	}

	std::optional<Gift> ReadGift(std::string_view a_key, std::string_view a_value)
	{
		Gift gift;
		auto key = Trim(a_key);
		gift.global = TakePrefix(key, "Global");
		gift.linked = TakePrefix(key, "Linked");
		if (gift.global && !gift.linked) {
			return std::nullopt;
		}
		TakePrefix(key, "Final");
		gift.death = TakePrefix(key, "Death");
		if (!KnownKind(key)) {
			return std::nullopt;
		}
		gift.kind = key;

		const auto sections = Sections(Sanitized(a_value));
		// a linked line names what it goes with instead of filters: form, parent forms, count, chance
		const std::size_t most = gift.linked ? 4 : 7;
		if (sections.empty() || sections.size() > most || !Given(sections[0])) {
			return std::nullopt;
		}
		const auto section = [&](std::size_t a_at) { return a_at < sections.size() ? std::string_view(sections[a_at]) : std::string_view(); };
		gift.gives = RecordOf(section(0));
		if (gift.linked) {
			ReadForms(gift, section(1), true);
			if (gift.formsMatch.empty()) {
				return std::nullopt;
			}
			AddFilters(gift.filters, section(1));
			gift.chance = ReadChance(section(3)).value_or(100.0f);
			return gift;
		}
		ReadWords(gift, section(1));
		ReadForms(gift, section(2), false);
		ReadLevels(gift, section(3));
		ReadTraits(gift, section(4));
		gift.chance = ReadChance(section(6)).value_or(100.0f);
		for (std::size_t i = 1; i <= 4; ++i) {
			AddFilters(gift.filters, section(i));
		}
		return gift;
	}

	std::optional<Stock> ReadStock(std::string_view a_key, std::string_view a_value)
	{
		auto value = Trim(a_value);
		if (value.empty() || value.starts_with('-')) {
			return std::nullopt;
		}
		Stock stock;
		stock.into = RecordOf(a_key);
		if (const auto ask = value.rfind('?'); ask != std::string_view::npos) {
			stock.chance = Number(Trim(value.substr(ask + 1))).value_or(100.0f);
			value = value.substr(0, ask);
		}
		if (const auto at = value.find('@'); at != std::string_view::npos) {
			stock.somewhere = true;
			value = value.substr(0, at);
		}
		const auto sections = Sections(value);
		if (!Given(sections[0])) {
			return std::nullopt;
		}
		stock.gives = RecordOf(sections[0]);
		stock.somewhere = stock.somewhere || (sections.size() > 2 && Given(sections[2]));
		return stock;
	}

	std::optional<Swap> ReadSwap(const Line& a_line)
	{
		// BOS reads [Forms], [Forms|conditions] and any other plain section as references, the rest move things
		const auto bar = a_line.section.find('|');
		const std::string_view section = std::string_view(a_line.section).substr(0, bar);
		if (bar != std::string::npos ? section != "Forms" : (section == "Transforms" || section == "Properties")) {
			return std::nullopt;
		}
		const auto sections = Sections(a_line.key);
		if (sections.size() < 2 || !Given(sections[0]) || !Given(sections[1])) {
			return std::nullopt;
		}
		Swap swap;
		swap.reference = section != "Forms";
		swap.somewhere = bar != std::string::npos;
		for (const auto& from : Split(sections[0], ",")) {
			swap.from.push_back(RecordOf(from));
		}
		for (const auto& to : Split(sections[1], ",")) {
			swap.to.push_back(RecordOf(to));
		}
		// "chance(50)", "chanceR(50)" or "chanceL(50, seed)"
		if (sections.size() > 3 && sections[3].find("chance") != std::string::npos) {
			const auto open = sections[3].find('(');
			if (open != std::string::npos) {
				const auto inside = std::string_view(sections[3]).substr(open + 1);
				swap.chance = Number(Trim(inside.substr(0, inside.find_first_of(",)")))).value_or(100.0f);
			}
		}
		return swap;
	}
}
