#pragma once

// What the player set: Data/SKSE/Plugins/KnowYourSkyrim.ini ships with the mod, Data/MCM/Settings/KnowYourSkyrim.ini
// is where the mod's menu in SkyUI keeps what was set there and wins. Both are read again each time the inventory
// opens, a change holds the next time without a restart of the game
namespace Options
{
	void Load();

	// the buttons of the cheats in the details: take an item, kill or bring back a person or creature
	bool Cheats();

	// the progression mode shows in the browser only what this character has met
	bool Progression();

	// A setting of the menu by its id, "bEnabled:Cheats" is bEnabled in [Cheats]. Set writes it into the menu's file
	float Value(std::string_view a_id);
	void Set(std::string_view a_id, float a_value);

	// a setting of the files as it's written, "sModOrganizer:Mods"
	std::string Text(std::string_view a_id);
}
