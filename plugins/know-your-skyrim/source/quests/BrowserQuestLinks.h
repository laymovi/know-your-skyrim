#pragma once

// What a quest gives, leads to and touches, read from its plugin records and from the compiled scripts of the quest,
// its aliases and its dialogue (QuestRecords, PapyrusDeeds), never by a list made by hand. A reward is what those
// scripts put into the player's hands: AddItem and RemoveItem into the player, a spell, a perk, a shout, a word of
// power, a skill. A chain is which quest starts which. A script that starts another quest or sets it to its first
// stages leads to it, one that sets another to its last stages follows it. The places of a quest are the locations its
// scripts and aliases name and the places of the references they hold. The files are read once, on a thread of their
// own, from the first look of the browser
namespace BrowserQuestLinks
{
	void Start();
	bool Started();
	bool Ready();

	// the stages with their journal entries, the rewards, the chains of quests before and after it and its places
	void Fill(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::TESQuest& a_quest, bool a_whole);

	// the quests of a place's page, the ones that touch the place itself first, then by the places right inside it
	void FillPlace(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::BGSLocation& a_place, bool a_whole);

	// the quests that give a thing, true for a reward picked from a list the thing is one of
	std::vector<std::pair<const RE::TESQuest*, bool>> RewardedBy(const RE::TESForm& a_thing);

	// the journal entries of the quest's stages one after another, the search by descriptions looks in them
	std::string TextsOf(const RE::TESQuest& a_quest);

	// what the places and rewards learned of a save, the next game reads it anew
	void Forget();

	// the first stage whose journal entry finishes the quest, none when no entry says or before the files are read
	std::optional<std::uint16_t> FinishingStage(const RE::TESQuest& a_quest);

	// the first stage past the current one whose journal entry tells the quest on, none before the files are read
	std::optional<std::uint16_t> BeginningStage(const RE::TESQuest& a_quest);
}
