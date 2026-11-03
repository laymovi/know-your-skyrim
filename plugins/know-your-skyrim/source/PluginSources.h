#pragma once

// A plugin file of the load order as the readers of the browser open it on their own threads: where it lies and how a
// form number of the file is the number the game gives it. The top byte picks a master of the file or the file itself,
// a light file gets a place among the light ones
namespace PluginSources
{
	struct Owner
	{
		std::uint32_t partial = 0;
		bool light = false;
	};

	struct Source
	{
		std::filesystem::path path;
		std::vector<Owner> owners;

		[[nodiscard]] RE::FormID Runtime(std::uint32_t a_raw) const;
	};

	Source SourceOf(const RE::TESFile& a_file);

	// every file the load order uses, in its order, made on the game's thread
	std::vector<Source> LoadOrder();
}
