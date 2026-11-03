#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The lines of the files SPID, Container Item Distributor and Base Object Swapper read from Data, as plain text and
// the way each of them reads its own. The forms they name are looked up in the game afterwards (BrowserHandouts)
namespace HandoutLines
{
	// a form as the files name it: a number of a plugin, a plugin alone, a number of the load order or an editor id
	struct Record
	{
		std::optional<std::uint32_t> id;
		std::string plugin;
		std::string editorID;
	};

	struct Line
	{
		std::string section;
		std::string key;
		std::string value;
	};

	// The lines of an ini file as SimpleIni hands them to the frameworks. A line without "=" is a key alone
	std::vector<Line> ReadIni(std::string_view a_text);

	Record RecordOf(std::string_view a_text);

	struct Range
	{
		std::uint16_t min = 0;
		std::uint16_t max = 0xFFFF;
	};

	struct Skill
	{
		std::uint32_t type = 0;
		Range range;
		bool weight = false;
	};

	// A SPID line: what it gives and to whom, as SPID 7 reads it. The words match a person's name, editor ids and
	// keywords, the forms match factions, classes, races and the rest. All of "all", none of "none", one of "match"
	// and a part of one of "part"
	struct Gift
	{
		// Item, Outfit, SleepOutfit, Form and every other kind SPID hands out, spells and perks too
		std::string kind;
		bool death = false;
		bool linked = false;
		bool global = false;
		Record gives;
		std::vector<std::string> wordsAll;
		std::vector<std::string> wordsNone;
		std::vector<std::string> wordsMatch;
		std::vector<std::string> wordsPart;
		std::vector<Record> formsAll;
		std::vector<Record> formsNone;
		std::vector<Record> formsMatch;
		Range level;
		std::vector<Skill> skills;
		std::optional<bool> female;
		std::optional<bool> unique;
		std::optional<bool> summonable;
		std::optional<bool> child;
		std::optional<bool> leveled;
		std::optional<bool> teammate;
		std::optional<bool> dead;
		float chance = 100.0f;
		// the filters as the file writes them, for a line the browser can't match to people
		std::string filters;
	};

	// nothing for a line SPID skips or a group of forms that exclude each other
	std::optional<Gift> ReadGift(std::string_view a_key, std::string_view a_value);

	// a CID line that adds a thing to a container, a chest, a person or a single reference of one
	struct Stock
	{
		Record into;
		Record gives;
		float chance = 100.0f;
		// only where the container stands in a location or one with a keyword
		bool somewhere = false;
	};

	// nothing for a line that takes things away
	std::optional<Stock> ReadStock(std::string_view a_key, std::string_view a_value);

	// A BOS line that puts one of its new bases in place of a base or a single reference
	struct Swap
	{
		std::vector<Record> from;
		std::vector<Record> to;
		float chance = 100.0f;
		bool reference = false;
		// a section with conditions, for example a location, a keyword or a season
		bool somewhere = false;
	};

	// nothing for a line that only moves, turns or scales what stands
	std::optional<Swap> ReadSwap(const Line& a_line);
}
