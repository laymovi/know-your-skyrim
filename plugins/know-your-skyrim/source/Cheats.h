#pragma once

// What the browser does to the game when the player switched cheats on in KnowYourSkyrim.ini (Options): gives the
// player one of the item of an entry, kills the nearest living one of a person or creature or brings the nearest dead
// one back, starts or finishes a quest, gives or takes a rank of a perk. Off by default, every function does nothing
// while the switch is off
namespace Cheats
{
	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root);

	// A deed is done on the game's own thread some time after the press. Done tells the browser once it is, the browser
	// asks again what changed, the open inventory shows what the player has now
	void Done();

	// HESOYAM in the game's console switches the cheats on or off, the same as the mod's menu does
	void AddConsoleWord();
}
