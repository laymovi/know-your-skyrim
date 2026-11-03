#pragma once

// A gamepad in the browser (39-pad.as holds the rest). The click of the right stick, the game's item zoom, goes to the
// browser. A short one moves the input between SkyUI's list and the browser, a hold opens or shuts it. The right stick
// scrolls the page while the browser has the input, and turns the inspected item. With Inventory Reforged its panel
// takes the zoom key and the click, the browser's script is told all the same and knows when the click is its own
namespace BrowserPad
{
	// at the plugin's load, the key only when Inventory Reforged isn't there to take it
	void InstallZoomKey();
	// on kDataLoaded
	void Listen();

	// what the stick does now, the script tells it, and only while a gamepad plays
	enum class Turns : std::int32_t
	{
		kNothing,
		kPage,
		kItem
	};
	void SetTurns(Turns a_what);

	// once a frame of the inventory
	void Frame();
	void Closed();

	// BrowserPadTurns(what) for the script: 0 nothing, 1 the page, 2 the inspected item
	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root);
}
