# Compatibility

The plugin loads a script movie of its own into SkyUI's inventory as it opens. No file of SkyUI or of another mod is replaced and no record of the game is changed, the light plugin only holds the menu. Nothing about a mod is written by hand into the browser, it reads what each plugin holds. Below is what was checked, by kind of mod. A version named is the one that ran in the game with the mod.

## Mods that add things

- Any mod that adds items, people, quests, perks or places with a plugin of its own is in the browser with no patch. Light plugins and plugins of any size count the same
- Checked on my load order of 230+ mods, 200 random cards from 45 of them looked at by eye, among them Interesting NPCs, Wyrmstooth, Clockwork and Ashe - Fire and Blood
- Items a mod keeps at the very origin of a world until a script moves them name no place, they used to read as near Bleakwind Basin

## What mods hand out while the game runs

- Spell Perk Item Distributor (items, outfits, death items), Container Item Distributor (chests and merchants) and Base Object Swapper (one thing in place of another where it stands): the browser reads their own files. Where to get names the people, merchants and chests with the framework and the chance, like "Thalmor Soldier (SPID, 20%)". A line no person can be named for shows its filters as the mod's file writes them. Only frameworks your game really loads count. Checked with SPID 7.3.3 on my load order, Container Item Distributor itself never ran
- Keyword Item Distributor and other mods that add keywords while the game runs: found with #
- SkyPatcher list edits: they are in memory before the browser reads, so they show

## Perk overhauls

- Adamant (checked), Ordinator, Vokrii and the like show by themselves, perks and trees are read from the game
- Custom Skills Framework keeps its trees in its own files, they aren't in the browser yet

## Textures

- Mods that only change textures come from Mod Organizer 2 (the profile the game runs with), from what Vortex deployed and from archives a blank plugin loads. Textures put into Data by hand have no mod's name to tell. When Mod Organizer isn't found by itself, its instance folder goes into sModOrganizer in KnowYourSkyrim.ini

## UI skins

- SkyUI 6.11 as it comes: every picture of the readme but one is plain SkyUI
- Skins built on SkyUI's inventory (Nordic UI, Edge UI, Vel'dun UI and the like) get the eye right after their hints in the bottom bar, a skin with no hints gets it by the bottom edge of the browser
- Nordic UI: its bottom bar has no MovieClip calls, the eye keeps its place there
- Dear Diary on game versions that squeeze it needs Widescreen Scale Removed, as the skin itself asks. Checked with and without Inventory Reforged
- A mod that replaces SkyUI's inventory with a menu not built on SkyUI has no room for the browser, the browser then stays away

## Inventory Reforged

- The browser opens in the panel, in place of your character and the slots, and the panel's windows, menus and look button keep their places around it. Each of the two works on its own too. On a gamepad the click of the right stick goes from your list to the panel, then to the browser
- Builds of the panel from before Know your Skyrim make no room for it, update the panel

## Camera and input

- Menu Studio 1.2.0 with FLICK 1.7.3 and Fitting Room 1.2.0: Menu Studio takes the left button and the wheel on its side of the screen, the mouse over the browser now goes to the browser first. A drag over the browser still turns Menu Studio's camera, it reads the press below the menus
- Show Player In Inventory 2.4 with FLICK 1.7.3 and ImGui Icons 1.2.0, alone and with Menu Studio and Fitting Room: tabs, rows and inspect of the browser work beside it. Its window of controls shows the first time and lies over part of the browser until you press OK. Its right and middle buttons turn the character over the browser too, it only keeps them off SkyUI's list
- Mods that hook the inventory's equip, drop and charge calls natively keep working beside the browser's gamepad

## Mod managers

- Mod Organizer 2 and Vortex: the installer shows one page with nothing to choose

## Translated games

- A translated game shows the names of items, places, stations and effects in its own language. The browser's own words come from its translation file, the mod's menu is in English
- Mods that replace the game's translator: the mod doesn't touch it
- An .ini in Data named outside the system's code page: read fine
