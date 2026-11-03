#pragma once

// Where an item can be got, as the records of the game tell it: the recipes that make or improve it and the ones it
// goes into, merchants whose chests stock it, people who carry, wear or drop it, plants it grows on, and the places
// where it or a chest that holds it stands (Holders and Places, shared with Inventory Reforged). The places come from
// the references that last the whole game until the plugin files are read for the rest (BrowserFiles). A quest tells
// what it ties to, an item and a living thing the quests they take part in, a perk its ranks and what asks for it
// (BrowserPerks). The lists of who holds what are made on the first ask and kept while the game runs
namespace BrowserSources
{
	// the first few of each list, or every list whole up to a cap once the player opened one on the page
	void Fill(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESForm& a_item, bool a_whole);

	// the references of a form that last the whole game, by their form ids
	std::vector<RE::FormID> LastingOf(const RE::TESForm& a_base);

	// the id of the browser's entry a thing links to, 0 for one the browser has no entry of
	RE::FormID LinkOf(const RE::TESForm* a_form);

	// the name as the browser shows it, a key of a translation file in the words of the file
	std::string NameOf(const RE::TESForm& a_form);

	// What conditions ask in the words of the page, as "needs" (groups the game joins by "or") and "more" (the groups
	// it can't word) on a_object. A recipe's materials hold anyway, so do the perks of a_given, the earlier ranks of a
	// perk's own chain. A perk has no recipe. Tells the perks the words name
	std::vector<const RE::TESForm*> PutNeeds(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESCondition& a_conditions,
		const RE::BGSConstructibleObject* a_recipe, std::span<const RE::BGSPerk* const> a_given = {});

	// the bench a recipe is made at, by the name of its furniture
	std::string BenchOf(const RE::BGSConstructibleObject& a_recipe);
}
