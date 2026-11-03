#pragma once

// What stays shut around the player and what opens it, read from the references themselves, from the activate parents a
// gate names (XAPR in the plugin), the key a lock names and the state the scripts hold. The player's cell, outdoors also
// the cells around it. Read once per opening of the inventory on the first page that asks
namespace Blockers
{
	void MenuOpened();

	// the list of the page of a place or of a quest, under "blockers"
	void Put(RE::GFxMovie& a_movie, RE::GFxValue& a_object);

	// whether a target of the quest stands in the player's cell or place, the way the game's own quest arrow reads it
	bool TargetsHere(const RE::TESQuest& a_quest);
}
