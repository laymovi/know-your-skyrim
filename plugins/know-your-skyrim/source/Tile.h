#pragma once

// The 3D of the browser: the model of an entry in the tile of its details, the inspect view, a list of the movie lying
// over the tile, and the game's own preview of the selected item stepping out while the browser stands where it
// shows. ItemView, Inspect and Scene are the code Inventory Reforged draws its panel with
namespace Tile
{
	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root);

	// every frame of the inventory, so a preview the game loaded meanwhile follows the browser's wish
	void MenuFrame();
	void MenuClosed();
}
