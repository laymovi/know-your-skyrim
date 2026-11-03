#pragma once

#include "BrowserIndex.h"

// Mods that only change how things look, with no records of their own and files that are textures and readmes. They are
// found in the profile Mod Organizer 2 runs the game with, in what Vortex deployed into Data, and in the archives of
// plugins with no records. That is read from the files on disk on a thread of its own, on the first ask. The textures a
// thing's models name are read from the models through the game's own files on the game's thread, a thing's own at once
// for its page and every thing's a few each frame once the list of what a mod changes is asked for
namespace BrowserTextures
{
	struct Mod
	{
		std::string name;
		// where it was found: Mod Organizer, Vortex or an archive
		std::string_view from;
		std::size_t textures = 0;
	};

	void Start();
	bool Ready();
	// none until they are read
	std::span<const Mod> Mods();
	std::optional<std::uint16_t> ModNamed(std::string_view a_name);

	// the mods that change a texture of the thing, its models read now
	std::vector<std::uint16_t> ChangersOf(const BrowserIndex::Item& a_item);

	// whether the mod changes a texture of the thing, false while its models aren't read yet
	bool Changes(std::uint16_t a_mod, const BrowserIndex::Item& a_item);

	// The models of every thing are read a few each frame once asked, how many are read of how many
	void WantAll();
	std::pair<std::size_t, std::size_t> Progress();
	void Frame();

	// how many things each mod changes, known once every model is read
	std::optional<std::size_t> ChangedBy(std::uint16_t a_mod);
}
