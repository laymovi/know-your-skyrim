Know your Skyrim 1.3.0
======================

Every item, person, quest, perk and place of your load order in a browser inside SkyUI's inventory, with where to get
each and the mod it comes from.

Requirements
- Skyrim Special Edition or Anniversary Edition with SKSE64
- Address Library for SKSE Plugins
- SkyUI
- Recommended: Inventory Reforged for SkyUI, the browser then opens in its panel
- Built and tested on games 1.7.104 (SKSE64 2.3.1), 1.6.1170 (SKSE64 2.2.6) and 1.5.97 (SKSE64 2.0.20), with Address
  Library 13 (All in One) and SkyUI 6.11

Install with a mod manager
Install the archive with Mod Organizer 2 or Vortex. The installer shows one page with nothing to choose, press Install.

Install by hand
Copy the content of the folder "core" into the Data folder of the game.

What goes where
- SKSE/Plugins/KnowYourSkyrim.dll          the plugin, it loads the browser into SkyUI's inventory
- SKSE/Plugins/KnowYourSkyrim.ini          the switches of the cheats and the progression mode, both off, and where
                                           Mod Organizer 2 is when the browser can't find it by itself
- KnowYourSkyrim.esp, Scripts/*.pex        the mod's menu in SkyUI's Mod Configuration, a light plugin, its place
                                           in the load order doesn't matter
- Interface/KnowYourSkyrim/browser.swf     the browser itself. No file of SkyUI is replaced
- Interface/Translations/KnowYourSkyrim_<LANGUAGE>.txt   every word of the browser, the menu is in English

Progression mode
Off as it comes. Switch it on in the mod's menu (journal, Mod Configuration, Know your Skyrim, Progression) at any time
in a game. The browser then shows only what this character has met. What was met is kept in the save all the time.

Cheats
Off as it comes. Switch them on in the mod's menu, or set bEnabled=1 under [Cheats] in
Data/SKSE/Plugins/KnowYourSkyrim.ini (in Mod Organizer 2: the mod's own copy of the file). The menu keeps what you set in
Data/MCM/Settings/KnowYourSkyrim.ini, which wins over the ini. The next time you open the inventory the pages of the
browser get buttons:
- TAKE puts the item into your inventory, as many as the box beside it says (1 to begin with, up to 9999)
- KILL kills a unique person or creature (Nazeem, your housecarl, a named dragon) wherever they are, essential ones
  too, RESURRECT brings a dead one back with what it carried and essential again. Killing an essential one can break
  a quest that needs them, keep a save
- Guards, bandits, wolves and others the game spawns by the dozen get no KILL or RESURRECT
- START begins a quest, COMPLETE finishes it the way its own scripts do. The stages between are not played, keep a save
- ADD gives the next rank of a perk, REMOVE takes the highest away. No perk point is spent or given back

Gamepad
In the inventory a click of the right stick opens the browser and moves the input into it, another click takes it
back to your list, holding the stick closes the browser. With Inventory Reforged the click goes from your list to its
panel, then to the browser. In the browser: d-pad rows and categories, LB/RB subcategories, LT/RT the links of the
page, A opens the lit link or inspects, Y favorite, X the cheat, B back, Back closes the browser, the right stick
scrolls. The buttons at the bottom say what each does

Compatibility
Everything checked, by kind of mod, is in compatibilities.md beside this readme.

Translate
Every word the browser shows is in Interface/Translations/KnowYourSkyrim_<LANGUAGE>.txt, one file for each language
of the game, all of them in English to begin with. Open the one of your game's language (sLanguage in Skyrim.ini) and
change the text after the tab on each line. Keep the part before the tab as it is. A # stands for a number or a name
the mod puts in, #1 and #2 may change places. Save the file as UTF-16 LE (Unicode in Notepad).

Bug reports
Send KnowYourSkyrim.log from Documents/My Games/Skyrim Special Edition/SKSE, and the newest crash log of Crash Logger or
Trainwreck if the game went down.

Source
The code is open, in the repository the archive comes from. The folder "source" holds a copy of the plugin and the
browser, see its readme. The plugin links CommonLibSSE-NG and is under the GNU GPL 3 like it. Nothing in that folder
goes into the game.

Uninstall
Remove the files. A save made with the mod loads fine without it. What you took, killed or brought back with the cheats
stays as it is.
