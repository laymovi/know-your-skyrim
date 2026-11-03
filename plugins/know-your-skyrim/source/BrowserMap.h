#pragma once

// A place of the browser on the world map. The item menu closes and the map opens looking at the marker of the place,
// the way the journal's "Show on Map" opens it. The game's map shows one world, the one the player is in, so a marker
// of another world can't be shown
namespace BrowserMap
{
	// tells whether the map can show the marker, and if so closes the menu and opens the map at it
	bool Show(RE::FormID a_marker);

	// empty when the map of the player's world shows the marker, the name of the marker's world when it doesn't
	std::string OtherWorld(RE::FormID a_marker);
}
