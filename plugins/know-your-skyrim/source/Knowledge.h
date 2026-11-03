#pragma once

#include "BrowserIndex.h"

// What this character met in this save, the browser shows only that while the progression mode is on. The game keeps
// part of it in the save itself, that is quests in the journal, places with a found marker, books read, spells learnt
// and people who spoke to the player. The rest is seen while the game runs and kept in the save's SKSE file: things
// held or seen in a chest, a shop or a follower's trade, people and creatures in the crosshair, in a fight or killed by
// the player, places the player was in. It's seen always, so the mode switched on in the middle of a game shows what
// was met. A thing a recipe makes is known while the player could make it
namespace Knowledge
{
	// the switch of the mode, [Progression] bEnabled of the settings
	bool On();

	// whether the character met the entry, true for every entry while the mode is off
	bool Known(const BrowserIndex::Item& a_item);

	// grows with every thing met, a search of the browser asks again when it moved
	std::uint32_t Version();

	// the sinks of the game's events, once the data is loaded
	void Listen();

	// the record in the save's SKSE file
	void Register();

	// after a load, a save made before the mod or before this version has no record, so the first one is taken from
	// what the game keeps and what the player carries
	void Loaded();

	// the things the recipes make that the player could make now, asked again each time the inventory opens
	void MenuOpened();
}
