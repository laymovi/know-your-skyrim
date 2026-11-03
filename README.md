# Know your Skyrim

English | [Українська](README.uk.md)

Big Brother is watching your load order. Nothing and no one escapes the invasion of privacy. Every item, person, creature and quest: where to get it, who sells it, who carries it, what it's made from and where it lies. Best together with Inventory Reforged for SkyUI.

Basically it's a browser of all your plugins, after the idea of JEI from Minecraft. Right in the game, in your inventory, you get a full catalogue of everything in your load order with as much information as possible. It's made for SkyUI as it comes, no skin needed: every picture on this page but one is plain SkyUI 6.11. UI skins on top of it work too (see [Compatibility](#compatibility)). Personally tested on Skyrim Special Edition 1.7.104 in a load order of 230+ mods.

## Requirements

- Skyrim Special Edition or Anniversary Edition with SKSE64 and Address Library for SKSE Plugins
- SkyUI
- No DLC
- Recommended: [Inventory Reforged for SkyUI](https://github.com/laymovi/inventory-reforged), the browser then opens in its panel
- Built and tested on game versions 1.7.104 (SKSE64 2.3.1), 1.6.1170 (SKSE64 2.2.6) and 1.5.97 (SKSE64 2.0.20), with Address Library for SKSE Plugins 13 (All in One) and SkyUI 6.11

## Installation

Download the archive from [Releases](../../releases) and install it with Mod Organizer 2 or Vortex.

By hand: copy the content of the folder `core` of the archive into `Data`.

## In short

1. The bottom bar of the inventory gets an eye button. It opens the browser on the right side of the screen, where SkyUI shows the item card

![browser](pictures/01-browser.jpg)

2. Every item tells where to get it: the recipe and the station, merchants, people who wear or drop it, plants it grows on, chests and places where it lies. A click on a place opens the world map right there

![where to get](pictures/p01-where.jpg)

3. Mods get the same pages as Skyrim itself, no patches needed. Nothing in the browser is written by hand, it reads what each plugin of your load order holds. In the pictures are Alberthor from Wyrmstooth, Ashe from Ashe - Fire and Blood, a quest from Interesting NPCs and a dwarven pipe from Clockwork

![Alberthor](pictures/p09-mod-person.jpg)

![Ashe](pictures/p09-mod-person-2.jpg)

![a quest of a mod](pictures/p10-mod-quest.jpg)

![a thing of a mod](pictures/p10-mod-thing.jpg)

4. The box beside the search limits it to one plugin, and its list shows how many items, people and quests each mod adds. The base game, its DLC and Creation Club are marked

![mods](pictures/p03-mods.jpg)

5. Search: words of the name, `@` for a mod, `#` for a keyword, `$` for an effect, a minus to leave something out. `$fortify smithing` shows everything that fortifies smithing, `$archery` also the archery trainers, `#ArmorMaterialDaedric` all the daedric gear of your game, `sword -@skyrim` every sword mods added. The button before the box picks where to look: in names, in descriptions (the text of a book, the line under an armor, a quest's journal) or in both

![search](pictures/p02-search.jpg)

![search by mods](pictures/p02-search-mods.jpg)

![descriptions](pictures/p18-descriptions.jpg)

6. Progression mode, if you'd like to discover the world yourself: the browser shows only what your character has already met. One switch in the mod's menu, any time in the middle of a game (see [Progression mode](#progression-mode) below)

![progression](pictures/p22-progression.jpg)

![progression, a page](pictures/p22-progression-sword.jpg)

![progression menu](pictures/p23-progression-menu.jpg)

7. People and creatures: portrait, race, levels, a shield for those the game keeps alive, what they wear and sell, where they stand now. Followers, merchants and people of the story are one click away. At the end of the page are relationships (lovers, allies, friends, rivals, enemies) and the attitude to you: whether they're hostile right now, whom they attack, whom they help

![person](pictures/p04-person.jpg)

![relations](pictures/p20-relations.jpg)

8. Quests: which stage you're at, every objective and every stage with its journal entry, rewards, the quest chain and the people, items and places the quest is tied to. Rewards are read from the quest's own compiled scripts, the game's and every mod's alike: gold, an item, a choice from a list, a spell, a shout, a word of power, with the stage or the talk they come at

![quest](pictures/p05-quest.jpg)

![quest chain](pictures/p16-quest-chain.jpg)

![quest stages](pictures/p16-quest-stages.jpg)

![reward](pictures/p17-reward.jpg)

9. Perks: every perk of the game's skill trees, mods that replace them too, since the trees are read from the game itself. Description, ranks with yours, requirements, neighbours in the tree, what the perk unlocks and which people have it

![perks](pictures/p21-perks.jpg)

10. Places: every named place of the game and your mods. Who's there now, alive or dead, who lives there, places inside, quests about it, and the map

![places](pictures/p11-places.jpg)

11. Stuck at a gate? Shut around you tells what's still shut around you and what opens it (see [Shut around you](#shut-around-you) below)

![shut around you](pictures/p24-shut.jpg)

![a raised bridge](pictures/p24-shut-bridge.jpg)

12. Trainers: their own subcategory in Living, "Trains Archery up to level 75" on their page and a choice of skill

![trainers](pictures/p12-trainers.jpg)

![trainers by skill](pictures/p12-trainers-menu.jpg)

13. A page looks like the game's journal, and every name on it is a link: an ingot in a recipe, a merchant, a quest. The arrow or a right click go back. The box under the picture searches words on the page itself, Enter goes to the next one

![find on the page](pictures/p19-find.jpg)

14. Any item in 3D beside its description, people with their faces, and a full inspect to turn the item and zoom in

![inspect](pictures/p06-inspect.jpg)

15. INVENTORY on the page of someone who's in the game shows their things where your list stands

![things](pictures/p14-things.jpg)

16. Favorites: the star in the corner of a page puts the entry into the Favorites tab, one list for all saves

![favorites](pictures/p13-favorites.jpg)

17. Mods that only change textures: pick one and the browser shows the items whose look it changes. They come from Mod Organizer 2, from what Vortex deployed and from texture archives

18. For the curious and for mod authors: the plugin an entry comes from, the mods that changed it after, its form id the way the console takes it, keywords and effects

19. Cheats, if you want them: TAKE, KILL, RESURRECT, START, COMPLETE and perks. Off by default (see [Cheats](#cheats) below)

![take](pictures/p07-cheats.jpg)

![kill](pictures/p07-cheats-kill.jpg)

![perk](pictures/p07-cheats-perk.jpg)

![quest](pictures/p07-cheats-quest.jpg)

20. The browser stays where you left it. **Only the eye closes it.** Close the inventory with the browser open and next time it opens on the same page, with the same search and the way back

21. With [Inventory Reforged](https://github.com/laymovi/inventory-reforged) the browser opens in the panel, in place of your character and the slots, and the panel's windows and menus stay around it

![with the panel](pictures/p25-with-panel.jpg)

22. Gamepad: the click of the right stick opens the browser and moves control into it (see [Gamepad](#gamepad) below)

![gamepad](pictures/p15-gamepad.jpg)

## Important

- No file of SkyUI is replaced, the browser loads into its inventory as it opens
- No record of the game is changed, the mod's light plugin holds only its menu
- It can go into a game already under way and works at once. It can be removed too
- **The browser lives only in the inventory. Chests and shops don't have it**
- Inventory Reforged, an equipment panel in the inventory, is a separate mod. Each of the two works on its own, but they're made to go together, I recommend having both

## Where to get

The browser also reads the files of your plugins, so it finds loot a mod placed in its own dungeons too.

What mods hand out while the game runs through Spell Perk Item Distributor, Container Item Distributor and Base Object Swapper is here too: people, merchants and chests are named, with the framework and the chance, like "Thalmor Soldier (SPID, 20%)". Only frameworks your game really loads count.

## Search

Basically a stand-in for subcategories, if you like text better. For me the best use is quickly finding trainers of a given skill and items with a specific enchantment. The box takes words and signs, in any mix:

- words: they're in the name, `steel sword`
- `@`: the plugin an entry comes from, `@dawnguard`
- `#`: a keyword, `#VendorItemGem`, `#ArmorHeavy`, the ones mods hand out while the game runs too
- `$`: an effect, `$fortify smithing`, `$frost`, and for a trainer the skill they teach, `$archery`
- a minus in front leaves out what follows it: `-@skyrim`, `-#ArmorHeavy`
- quotes keep words together: `@"Legacy of the Dragonborn"`

## Progression mode

Say yes to spoilerophobia and turn on progression mode in the mod's settings. It removes from the browser information about items, places, quests and everything else you haven't met in the game yet.

It goes on in the mod's menu: journal, Mod Configuration, Know your Skyrim, Progression. It takes effect the next time the inventory opens.

- What stays is quests in your journal, places you found, people you talked to, fought or put your crosshair on, things that were in your hands, in a chest or in a shop, and everything you can make right now
- **The mod records what you meet all the time, in the save, even with the mode off**, so the mode already knows your way whenever you switch it on
- The unknown is named for what it is: "a place you haven't found (Whiterun)", "12 things you haven't found (Apparel, Clothing, Body)"
- A save made before this version starts from what the game remembers: quests in the journal, places found, books read, spells learned, people who talked to you or follow you, and what you carry. Let me know if you happen to know how to put all Skyrim players under a full 1984, to fix this unfortunate gap!
- Perks of the trees are always known, the way the game shows them, favorites too

## Shut around you

If you spend half an hour at a time running around a place like a lost puppy and that's your life's calling, my respect, you can safely skip this section! Otherwise, if like me you sometimes feel like a dimwit and can't see how to get further through a place or a quest, the cards it applies to have a Shut around you section.

The section stands on the page of the place you're in and on the card of a quest with its objective there. The Here subcategory in Places opens that page at once. A line looks like this: "A shut gate, 27 m ahead on the left, 7 m below you. Opened by Lever, 26 m ahead on the left, 7 m below you".

- It shows portcullises and gates, raised bridges and doors with the name of their key, and the levers, chains, buttons and plates that open them
- It tells only where the lever is. **There's no route to it (at least for now)**
- Doors of someone's home or shop aren't shown, since you can usually get in there without the key anyway :)
- "What opens it isn't here yet, it depends on a quest": what opens it is still switched off, a quest will switch it on
- "N more stay shut, nothing here says what opens them": a puzzle made only of scripts names nothing. By the game's records what opens something has a name in about 80% of cases

## Cheats

Off by default. Switch them on in the mod's menu (journal, Mod Configuration, Know your Skyrim, Cheats). Or just type `HESOYAM` into the game's console 🌚. It takes effect the next time the inventory opens.

![menu](pictures/p08-menu.jpg)

- TAKE puts the item into your inventory, as many as you write in the box beside it
- KILL kills a unique person or creature wherever they are. Essential ones too: Jarl Balgruuf and the Greybeards are no exception, that's what the cheat is for. **A quest that needs them can break, so keep a save**
- RESURRECT brings a dead unique one back with everything they had, essential again if they were
- Guards, bandits, wolves and everything else the game sets out by the dozen have no KILL: "one of them" could be anyone
- START starts a quest, COMPLETE finishes it the way its own scripts do, so the reward comes and the next quest of the chain starts. The stages between aren't played
- ADD gives the next rank of a perk, REMOVE takes the highest, like the console's addperk and removeperk: no perk point is spent or returned
- With cheats a right click on a thing of a person takes it from them or gives them yours

## Gamepad

The click of the right stick in the inventory opens the browser and moves control into it, a gold outline shows where it is now. Another click goes back to your list, holding the stick closes the browser. With Inventory Reforged the click goes from your list to the panel, then to the browser and back to the list.

- D-pad up and down: rows. Left and right: categories. LB and RB: subcategories
- LT and RT: links of the page, starting with the part of it on the screen. A opens the lit link, without one it inspects the item, shows a person's things or opens a place's map
- Y: the star. X: the page's cheat. B: back along the way you came, then to the list. Back: close the browser. Right stick: scroll the page
- In inspect the right stick turns the item, up and down zoom, B closes
- Mouse only: several quest subcategories at once, typing, the Names button and picking a trainer's skill

## Settings

The mod's menu has two pages, Progression and Cheats, one switch on each. The same are in `Data/SKSE/Plugins/KnowYourSkyrim.ini`, it's read again each time the inventory opens, and what's set in the menu is kept in `Data/MCM/Settings/KnowYourSkyrim.ini` and wins.

## Compatibility

All the details are in [compatibilities.md](compatibilities.md): what mods hand out while the game runs, perks, textures, skins, the camera, Inventory Reforged.

In short, any mod that adds items, people or quests with a plugin of its own is in the browser with no patches. The mod adds its own to SkyUI's inventory and changes no record of the game. A mod that replaces SkyUI's inventory with a menu of its own not built on SkyUI has no room for the browser, the browser then just doesn't appear.

Skins built on SkyUI's inventory (Nordic UI, Edge UI, Vel'dun UI and the like) work too, the eye stands beside their hints in the bottom bar. Of the perk mods I checked Adamant, Ordinator and Vokrii should show the same way, since the trees are read from the game.

## Under the hood

- The index of your load order is made once a game, when you first open the browser: one pass over the records the game holds in memory, 0.1 s for 10,800 records of 284 plugins in my game. The plugin files are read once more, on a thread of its own, for what the game doesn't keep in memory: half a second for 128,000 placed things
- Records of quests and their compiled scripts are read once a game, on a thread of its own: about 3 s for 23,500 scripts of 290 archives in my game. The scripts aren't run, only their bytecode is read
- The first look at an item reads its model from disk, a few dozen milliseconds on that click
- A search takes a millisecond or two, and the browser asks for rows one page at a time, so a load order of tens of thousands of records costs the menu no more than one page
- Progression mode records what you meet on the game's events: a thing coming to you, a talk, the crosshair, a fight, a new place. Outside the inventory the mod does nothing per frame
- Shut around you reads around you only when a page asks for it, under 1 ms where I measured
- The 3D tile and inspect use the same small 3D scene the game shows the selected item in

## Good to know

- Quest rewards are read from the scripts as written: an item given in one branch and another in a second one both show. What a script works out while the game runs shows as far as the script names it
- Textures put into `Data` by hand have no mod's name, the browser can't tell them apart. When Mod Organizer isn't found by itself, its instance folder goes into `sModOrganizer` in `KnowYourSkyrim.ini`
- Custom Skills Framework keeps its trees in its own files, they aren't in the browser yet

## How to report a bug

1. Close the game. From `Documents/My Games/Skyrim Special Edition/SKSE` take `KnowYourSkyrim.log`, and if the game crashed, the newest crash log of Crash Logger or Trainwreck too
2. A freeze (the game stands but doesn't close): while it hangs, press Ctrl+Shift+Esc, open the Details tab of Task Manager, right click SkyrimSE.exe and Create memory dump file. Zip the .DMP and upload it to Google Drive, Mega or the like
3. If you can, write what you did, what you expected and what happened, a screenshot if it shows, and links to the files. Put the log on pastebin.com or a file host and give the link

## Translations

Every word of the browser comes from `Data/Interface/Translations/KnowYourSkyrim_<LANGUAGE>.txt`. The mod ships such a file for every language of the game, all in English at first. To translate, open the file of your game's language and change the text after the tab on each line, leave the part before it as it is. `#` stands for a number or a name the mod puts in, `#1` and `#2` can swap places. Save as UTF-16 LE (Unicode in Notepad). The mod's menu is in English. Translations can be released as mods of their own with just this file.

## The code

A SKSE plugin in C++ and an ActionScript 2 movie the plugin loads into SkyUI's inventory. Part of the code the browser shares with [Inventory Reforged](https://github.com/laymovi/inventory-reforged): the menu's 3D scene, models, inspect and drawing helpers. Here lies exactly what this mod is built from, in the same places as in the panel's repository.

| Folder                               | What's there                                                                   |
| ------------------------------------ | ------------------------------------------------------------------------------ |
| `plugins/know-your-skyrim`           | The SKSE plugin                                                                |
| `plugins/equipment-panel/source`     | Files of Inventory Reforged's plugin that this plugin compiles too             |
| `plugins/shared/source`              | C++ shared with my other mods                                                  |
| `plugins/cmake/ports`                | A port of CommonLibSSE-NG for vcpkg                                            |
| `scripts/interface/know_your_skyrim` | The browser: `source` parts of the movie, `menu` the mod's menu, `tests` in node |
| `scripts/interface/equipment`        | Parts of the panel `borrowed.py` takes helpers from by name                    |

### Build

The plugin needs Visual Studio with its C++ tools, its CMake, Ninja and vcpkg. The first run builds CommonLibSSE-NG and takes a few minutes:

```
plugins\build.ps1 know-your-skyrim
```

The whole package also needs:

- Python 3.11 or newer with Pillow
- Java and [JPEXS Free Flash Decompiler](https://github.com/jindrapetrik/jpexs-decompiler) in `tools/ffdec/app/ffdec.jar`
- [Caprica](https://github.com/Orvid/Caprica) in `tools/caprica/app/Caprica.exe`
- Papyrus sources of the game, SKSE and the SkyUI SDK, and `TESV_Papyrus_Flags.flg` from the Creation Kit

`KYSK_TOOLS` points at the tools if they lie elsewhere.

```
set PAPYRUS_IMPORTS=C:\path\to\skse\Scripts\Source;C:\path\to\game\Scripts\Source;C:\path\to\skyui-sdk
set PAPYRUS_FLAGS=C:\path\to\TESV_Papyrus_Flags.flg
cd scripts
python -m interface.know_your_skyrim.build
python -m interface.know_your_skyrim.release --no-archive
```

The result lands in `out/know-your-skyrim`.

### Tests

```
node scripts/interface/know_your_skyrim/tests/browser.test.cjs
node scripts/interface/know_your_skyrim/tests/contract.test.cjs
```

Functions marked `for the test stand` stay in the release movie on purpose: an automated run of the real game drives them. That stand isn't in this repository.

### Words the code uses

- **entry** - a record of the browser: an item, person, quest, perk or place
- **tile** - the 3D tile beside the page
- **chip** - a subcategory, a small filter button above the rows
- **room** - the place the browser takes in the menu, alone or in the panel
- **panel** - Inventory Reforged's panel, which the browser shares the menu with
- **told** - a value from a script or a mod not checked yet

## Licence

GNU General Public License 3, see [LICENSE](LICENSE). The plugin links CommonLibSSE-NG, so its exceptions for mods and linking apply too. [licences](licences/README.txt) lists everything the mod is built from. The mod's archive holds a copy of the code in the folder `source`, the installer never copies it into the game.

## Credits

SkyUI by the SkyUI team, the base this mod lives on. JEI (Just Enough Items) for Minecraft, the idea. Built with CommonLibSSE-NG.
