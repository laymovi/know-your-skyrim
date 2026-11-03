#pragma once

// What mods hand out while the game runs, so the records never hold it: SPID gives things to people when they load or
// die, Container Item Distributor puts things into containers when they load, Base Object Swapper puts one thing in
// place of another where it stands. Their files in Data are read on a thread of their own from the first time the
// browser asks where things are, the forms they name are looked up on the first ask after that. A framework the game
// didn't load is left out, its files do nothing
namespace BrowserHandouts
{
	// which framework hands it out and how sure it is
	struct Tag
	{
		const char* by = nullptr;
		float chance = 100.0f;
		// only in some places or under some conditions
		bool somewhere = false;
	};

	// A SPID line: the thing, a loot list or an outfit it gives, to the living or on death. A line for everyone, or one
	// whose filters a person's record can't answer (a place, a single reference, a dead body), tells its filters
	struct Gift
	{
		const RE::TESForm* gives = nullptr;
		bool death = false;
		Tag tag;
		bool told = false;
		bool everyone = false;
		std::string filters;
		std::size_t rule = 0;
	};

	// a CID line: the thing or list it puts into a chest, a person or a single reference
	struct Stock
	{
		const RE::TESForm* gives = nullptr;
		const RE::TESForm* into = nullptr;
		RE::FormID reference = 0;
		Tag tag;
	};

	// a BOS line: what takes the place of a base, or of a single reference, where it stands
	struct Swap
	{
		const RE::TESForm* from = nullptr;
		RE::FormID reference = 0;
		const RE::TESForm* to = nullptr;
		Tag tag;
	};

	void Start();
	bool Started();
	bool Ready();

	// the lines that give the form itself, or an outfit that holds it
	std::vector<const Gift*> GiftsOf(const RE::TESForm& a_form);
	// the people whose records fit a line, found once and kept
	const std::vector<const RE::TESNPC*>& PeopleOf(const Gift& a_gift);
	// the lines a person's record fits, the ones for everyone left out
	std::vector<const Gift*> GiftsTo(const RE::TESNPC& a_person);

	std::vector<const Stock*> StocksOf(const RE::TESForm& a_form);
	std::vector<const Stock*> StocksInto(const RE::TESForm& a_container);

	// the lines that put the form in place of another and the ones that put another in its place
	std::vector<const Swap*> SwapsTo(const RE::TESForm& a_form);
	std::vector<const Swap*> SwapsFrom(const RE::TESForm& a_form);
}
