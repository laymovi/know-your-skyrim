// Know your Skyrim: every item, person, quest and pseudo-item of the load order in SkyUI's inventory, by category,
// search and the mod each comes from, with where to get it. Its own global, the panel of Inventory Reforged shares
// _global with it in the same menu. The helpers it draws and words with come from the panel's parts under this name
// (interface/know_your_skyrim/borrowed.py)
var KnowYourSkyrim = {};
_global.KnowYourSkyrim = KnowYourSkyrim;

KnowYourSkyrim.BROWSER_PAD = 10;
KnowYourSkyrim.BROWSER_HEAD = 26;
// as high as the strip of chips over the list, the chips are drawn the same way
KnowYourSkyrim.BROWSER_STRIP = 30;
KnowYourSkyrim.BROWSER_ROW = 22;
KnowYourSkyrim.BROWSER_ICON = 16;
KnowYourSkyrim.BROWSER_TEXT = 14;
KnowYourSkyrim.BROWSER_SMALL = 12;
KnowYourSkyrim.BROWSER_WHEEL = 3;
KnowYourSkyrim.BROWSER_PAGE_STEP = 24;
KnowYourSkyrim.BROWSER_TILE = 112;
KnowYourSkyrim.BROWSER_SHADE = 82;
KnowYourSkyrim.BROWSER_BAR = 4;
KnowYourSkyrim.BROWSER_DELAY = 250;
KnowYourSkyrim.BROWSER_BUTTON_HIGH = 22;
KnowYourSkyrim.BROWSER_BUTTON_WIDE = 32;
KnowYourSkyrim.BROWSER_BUTTON_SIGN = 22;
KnowYourSkyrim.BROWSER_BUTTON_GAP = 24;
KnowYourSkyrim.BROWSER_TAB_GAP = 10;
KnowYourSkyrim.BROWSER_MOD_SHARE = 0.36;
KnowYourSkyrim.BROWSER_SEARCH_SHARE = 0.46;
KnowYourSkyrim.BROWSER_MODS_SHARE = 0.27;
KnowYourSkyrim.BROWSER_FIELD_GAP = 8;
KnowYourSkyrim.BROWSER_LIST_ROW = 20;
KnowYourSkyrim.BROWSER_LIST_ROWS = 12;
KnowYourSkyrim.BROWSER_BACK = 22;
KnowYourSkyrim.BROWSER_STAR = 18;
KnowYourSkyrim.BROWSER_HISTORY = 60;
// frames between two asks while the plugin files are read
KnowYourSkyrim.BROWSER_READ_WAIT = 30;
// frames to look for SkyUI's inventory before the browser stays out
KnowYourSkyrim.MENU_WAIT = 60;
// frames a browser left open waits for the panel of Inventory Reforged before it opens alone
KnowYourSkyrim.PANEL_WAIT = 60;
// frames the browser that stepped aside for the item card waits for the card to ask before it comes back
KnowYourSkyrim.CARD_WAIT = 15;
KnowYourSkyrim.BROWSER_EDITS_NAMED = 2;
KnowYourSkyrim.BROWSER_VARIANTS_SHOWN = 12;
KnowYourSkyrim.BROWSER_INSPECT_HIGH = 18;
// recipes of one bench and the same needs, each from a single thing, go on one line from this many on
KnowYourSkyrim.BROWSER_RECIPES_JOINED = 3;
KnowYourSkyrim.BROWSER_NAMES_SHOWN = 6;
// the stages of a quest the page tells before the rest opens on a press
KnowYourSkyrim.BROWSER_STEPS_SHOWN = 8;
// the letters of a description the page shows before the rest opens on a press
KnowYourSkyrim.DESCRIPTION_SHOWN = 420;
// the chips of a category take a line for each group when one line would shrink them below this, at most so many. Perks
// has four groups, two lines of its 21 trees are as small as one line of Living's twelve
KnowYourSkyrim.CHIP_LINE_FIT = 0.85;
KnowYourSkyrim.CHIP_LINES_MOST = 4;
// the actor value of the first skill, the game keeps six skills to a school
KnowYourSkyrim.FIRST_SKILL = 6;
KnowYourSkyrim.SKILLS_A_SCHOOL = 6;
// a chip of the browser's state that holds the skill trainers train, "skill:<actor value>"
KnowYourSkyrim.SKILL_CHIP = "skill:";
// what a gamepad walks while the browser holds its input, nothing while the list has it (39-pad.as)
KnowYourSkyrim.padFocus = undefined;
// the lists of the page the player opened, by their keys, shut again for every new page
KnowYourSkyrim.browserOpen = {};
// a shield after the name of one the game keeps alive, full for essential, faint for protected (only the player can kill)
KnowYourSkyrim.BROWSER_MARK = 12;
KnowYourSkyrim.BROWSER_GUARD = 0x9CC3E6;
KnowYourSkyrim.ESSENTIAL = 2;
// Alone, the browser takes the room right of the list as SkyUI keeps it for its item card. The frame of the list
// reaches this far past its rows, the room keeps this much clear of the edges
KnowYourSkyrim.LIST_FRAME = 25;
KnowYourSkyrim.ROOM_MARGIN = 14;

KnowYourSkyrim.plugin = function()
{
   return skse.plugins.KnowYourSkyrim;
};

// Every word the browser shows goes through here. Interface/Translations/KnowYourSkyrim_<LANGUAGE>.txt gives it as
// "$KYSK_<the English words>" in the language of the game, words it doesn't have stay English. A "#" takes a value
// given after the words, "#1", "#2" take them by number so a language can put them in its own order. The build
// writes the English file from every say("...") of the parts, so only plain words go in here. The plugin reads the
// file for the browser, the game's translator would hand back HEALTH for Health
KnowYourSkyrim.said = {};
KnowYourSkyrim.say = function(words)
{
   var shown = KnowYourSkyrim.said[words];
   var plugin;
   var out = "";
   var from = 0;
   var next = 1;
   var at;
   var number;
   if(shown == undefined)
   {
      if(KnowYourSkyrim.fileWords == undefined)
      {
         plugin = KnowYourSkyrim.plugin();
         KnowYourSkyrim.fileWords = plugin != undefined && plugin.Words != undefined ? plugin.Words() : undefined;
      }
      shown = KnowYourSkyrim.fileWords != undefined ? KnowYourSkyrim.fileWords["$KYSK_" + words] : undefined;
      if(shown == undefined)
      {
         shown = skyui.util.Translator.translate("$KYSK_" + words);
      }
      if(shown == undefined || shown == "" || shown.charAt(0) == "$")
      {
         shown = words;
      }
      KnowYourSkyrim.said[words] = shown;
   }
   while((at = shown.indexOf("#", from)) >= 0)
   {
      number = Number(shown.charAt(at + 1));
      out += shown.substring(from, at);
      if(number >= 1 && number < arguments.length)
      {
         out += arguments[number];
         from = at + 2;
      }
      else
      {
         out += next < arguments.length ? arguments[next++] : "#";
         from = at + 1;
      }
   }
   return out + shown.substring(from);
};

// A point of the stage, as localToGlobal gives it, as shares of the screen. The plugin takes places in that form
KnowYourSkyrim.stageShare = function(x, y)
{
   var screen = Stage.visibleRect;
   return {x:(x - screen.x) / screen.width, y:(y - screen.y) / screen.height};
};

// a box of a clip on the stage as shares of the screen, {from, to}
KnowYourSkyrim.shareBox = function(clip)
{
   var bounds = clip.getBounds(_root);
   return {from:KnowYourSkyrim.stageShare(bounds.xMin, bounds.yMin), to:KnowYourSkyrim.stageShare(bounds.xMax, bounds.yMax)};
};
