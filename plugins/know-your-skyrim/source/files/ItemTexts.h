#pragma once

// The descriptions of things as their plugins write them, the game reads them from the files only when it shows one:
// the text of a book or a note, the line under a piece of armor, a weapon, a scroll or a perk. The markup of books is left out,
// a page break is an empty line. A later plugin that changes the description wins. Read once, on a thread of their
// own, the search by descriptions and the page of a thing ask
namespace ItemTexts
{
	void Start();
	bool Started();
	bool Ready();

	// the description of a thing, empty for one with none or before the files are read
	std::string_view Of(RE::FormID a_form);
}
