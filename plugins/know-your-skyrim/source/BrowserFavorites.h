#pragma once

// The entries the player starred, one list for every save and character. It is kept in the folder of SKSE's logs
// (My Games/Skyrim Special Edition/SKSE/KnowYourSkyrim.favorites.txt), each line the plugin and the number the form
// has in it, so a new load order keeps it
namespace BrowserFavorites
{
	bool Has(RE::FormID a_form);
	void Set(RE::FormID a_form, bool a_on);
	// grows on every change, a search kept from before a change is done again
	std::uint32_t Version();
}
