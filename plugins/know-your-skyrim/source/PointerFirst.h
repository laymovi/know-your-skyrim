#pragma once

// The mouse over the open browser goes right to the menu's movie, ahead of what other mods hook on the inventory's
// messages. A mod with a camera of the menus takes the left button and the wheel on that side of the screen, where
// SkyUI has nothing to press and the browser has everything (Menu Studio from 0.44 of the width)
namespace PointerFirst
{
	// once the plugins have all made their hooks of the load
	void Install();

	// where the browser lies now in shares of the screen, nothing while it's shut
	void SetArea(float a_left, float a_top, float a_right, float a_bottom);

	void MenuClosed();
}
