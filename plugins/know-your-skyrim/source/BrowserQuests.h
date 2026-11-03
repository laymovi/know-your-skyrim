#pragma once

// What a quest ties to, as its records and its scripts tell: the things and the people its aliases hold, make or
// name, and its scripts point at, and the references it is forced on. Scripts keep their values in the save, so the
// ties are read on the first ask after a load
namespace BrowserQuests
{
	struct Ties
	{
		std::vector<const RE::TESForm*> things;
		std::vector<const RE::TESNPC*> people;
		std::vector<RE::FormID> references;
	};

	const Ties& TiesOf(const RE::TESQuest& a_quest);

	// the quests with a name that tie to an item or a person
	std::vector<const RE::TESQuest*> QuestsOf(const RE::TESForm& a_form);

	// how far the player is with a quest: 0 not begun, 1 waits for the player (running with nothing in the journal
	// yet), 2 going, 3 done, 4 stopped unfinished
	int StateOf(const RE::TESQuest& a_quest);

	// Every objective of the quest, in the journal's words with the names of what the aliases hold, and how each
	// stands: 0 open, 1 done, 2 failed, 3 not given yet
	struct Objective
	{
		std::string text;
		int state = 0;
		std::uint16_t index = 0;
	};
	std::vector<Objective> ObjectivesOf(RE::TESQuest& a_quest);

	// a journal text with "<Alias=Item>" and "<Global=Reward>" given the names and the values they stand for now
	std::string Filled(RE::TESQuest& a_quest, std::string_view a_text);

	void Forget();
}
