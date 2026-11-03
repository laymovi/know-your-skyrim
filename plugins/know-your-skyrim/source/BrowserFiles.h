#pragma once

#include <unordered_set>

// Where the plugin files place people, items and chests. The game keeps in memory only the references that last the
// whole game. The rest of a cell comes from its plugin file when the cell loads, and that's most people and the loot of
// dungeons. The files are read once, on a thread of their own, from the first time the browser asks where things are. A
// later plugin that moves, disables or deletes a reference wins, as in the game
namespace BrowserFiles
{
	struct Spot
	{
		RE::FormID reference = 0;
		RE::FormID base = 0;
		RE::FormID cell = 0;
		// the world of a cell outside, 0 inside
		RE::FormID world = 0;
		RE::NiPoint3 position;
		bool disabled = false;
	};

	// starts the reading, once, with the forms whose references are wanted
	void Start(std::unordered_set<RE::FormID> a_wanted);
	bool Started();
	bool Ready();

	// the references of a form in the plugin files, empty until Ready
	std::span<const Spot> SpotsOf(RE::FormID a_base);

	// the spot of one reference of a wanted form, none until Ready
	const Spot* SpotOfReference(RE::FormID a_reference);
}
