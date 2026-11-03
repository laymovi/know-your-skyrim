#pragma once

// The log tells each step the mod takes, and while it reads the game's records which plugin they come from, so the
// last line before a crash names the step and the mod. CommonLib writes every line to disk at once
namespace Trail
{
	void Step(std::string_view a_step);

	// a line when the plugin of the records a step reads changes, from the game's thread only
	void Reading(std::string_view a_step, const RE::TESForm* a_form);
}
