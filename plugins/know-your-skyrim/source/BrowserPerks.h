#pragma once

// The perks of the skill trees, read from the trees themselves. That is every actor value with a tree of perks, the
// eighteen skills, the vampire's and the werewolf's and any other a plugin gives, walked from the root down its
// children. So an overhaul that replaces the trees (Ordinator, Adamant, Vokrii) comes in by itself. The ranks of one
// chain (nextPerk) are one perk, its first rank gives the entry. A perk in no tree is none of these. Read once on the
// first ask and kept while the game runs, the records don't change after the data is loaded
namespace BrowserPerks
{
	// the chips of Perks: the eighteen skills in the game's order, the vampire's tree, the werewolf's, any other
	inline constexpr std::uint8_t kTrees = 21;
	inline constexpr std::uint8_t kVampireTree = 18;
	inline constexpr std::uint8_t kWerewolfTree = 19;
	inline constexpr std::uint8_t kOtherTree = 20;

	struct Perk
	{
		// the ranks in their order, the first gives the entry
		std::vector<RE::BGSPerk*> ranks;
		std::uint8_t tree = kOtherTree;
		// the perks right before and right after it in the tree, by their place in All()
		std::vector<std::uint32_t> parents;
		std::vector<std::uint32_t> children;
	};

	const std::vector<Perk>& All();
	// the perk a rank belongs to, any rank of the chain
	const Perk* Of(const RE::TESForm& a_form);

	// the key of a tree's chip, "oneHandedPerks"
	std::string_view TreeKey(std::uint8_t a_tree);
	std::optional<std::uint8_t> TreeNamed(std::string_view a_key);
	// the name the game gives the tree's actor value, empty when it gives none
	std::string_view TreeName(std::uint8_t a_tree);

	// whether the player has each rank, and the highest rank the player has, 0 for none
	std::vector<bool> Held(const Perk& a_perk);
	std::uint32_t RankOf(const Perk& a_perk);

	// The page of a perk: each rank with its description and what it asks, its neighbours in the tree, what it unlocks
	// (the recipes and perks that ask for any rank of it) and the people whose records have it. The first few of each
	// list, or every list whole up to a cap once the player opened one
	void Fill(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const Perk& a_perk, bool a_whole);
}
