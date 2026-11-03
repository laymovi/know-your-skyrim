// What the player typed as terms for the plugin: words a name holds, @ a plugin's file, # a keyword, $ an effect,
// a - in front leaves out what it names. Quotes keep words together
KnowYourSkyrim.browserTerms = function(text)
{
   var source = String(text);
   var terms = [];
   var i = 0;
   var end;
   var kind;
   var negated;
   var word;
   var sign;
   while(i < source.length)
   {
      while(i < source.length && source.charAt(i) == " ")
      {
         i++;
      }
      if(i >= source.length)
      {
         break;
      }
      negated = source.charAt(i) == "-";
      if(negated)
      {
         i++;
      }
      sign = source.charAt(i);
      kind = sign == "@" ? "mod" : (sign == "#" ? "keyword" : (sign == "$" ? "effect" : "name"));
      if(kind != "name")
      {
         i++;
      }
      if(source.charAt(i) == "\"")
      {
         end = source.indexOf("\"", i + 1);
         end = end < 0 ? source.length : end;
         word = source.substring(i + 1, end);
         i = end + 1;
      }
      else
      {
         end = source.indexOf(" ", i);
         end = end < 0 ? source.length : end;
         word = source.substring(i, end);
         i = end;
      }
      if(word.length > 0)
      {
         terms.push([kind, word, negated]);
      }
   }
   return terms;
};

// A field to type into, a sign before the words and a hint of what can be typed while it's empty
KnowYourSkyrim.buildBrowserField = function(holder, name, depth, x, y, wide, sign, hint, which)
{
   var high = KnowYourSkyrim.BROWSER_HEAD;
   var box = holder.createEmptyMovieClip(name, depth);
   var field;
   box._x = x;
   box._y = y;
   box.wide = wide;
   box.sign = sign;
   box.which = which;
   box.createEmptyMovieClip("look", 0);
   box.createTextField("field", 1, 26, 3, wide - 32, high - 6);
   field = box.field;
   KnowYourSkyrim.styleText(field, KnowYourSkyrim.BROWSER_TEXT, "left", KnowYourSkyrim.WHITE);
   field.type = "input";
   field.selectable = true;
   field.noTranslate = true;
   KnowYourSkyrim.writeLine(box, "hint", 2, hint, KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.HERO_FAINT);
   box.hint._x = 28;
   box.hint._y = Math.round((high - box.hint._height) / 2);
   box.hint.hitTestDisable = true;
   KnowYourSkyrim.fitHint(box.hint, wide - box.hint._x - 6);
   box.createTextField("touch", 3, 0, 0, wide, high);
   KnowYourSkyrim.listenField(box);
   return box;
};

// A hint wider than its box (the search beside the panel, a translation's longer words) loses the last parts of its
// list first, "-leave out", then its last letters for three dots
KnowYourSkyrim.fitHint = function(hint, room)
{
   var text = hint.text;
   var cut;
   while(hint._width > room && text.lastIndexOf(", ") > 0)
   {
      text = text.substring(0, text.lastIndexOf(", "));
      hint.text = text;
   }
   while(hint._width > room && text.length > 1)
   {
      text = text.substring(0, text.length - 1);
      cut = text;
      while(cut.length > 0 && cut.charAt(cut.length - 1) == " ")
      {
         cut = cut.substring(0, cut.length - 1);
      }
      hint.text = cut + "...";
   }
};

// The field of a box takes the focus only from a press on the box. SkyUI's own search hands the focus back to
// whatever had it before, this field too, and the list gets it then. The box holds a touch field over all of it
KnowYourSkyrim.listenField = function(box)
{
   var field = box.field;
   // the field's own look, taken while it holds no focus because with the focus the game gives its first format
   box.format = field.getNewTextFormat();
   field.onSetFocus = function()
   {
      if(KnowYourSkyrim.browserTyping != this._parent.which)
      {
         Selection.setFocus(KnowYourSkyrim.menu.inventoryLists.itemList, 0);
      }
   };
   field.onKillFocus = function()
   {
      if(KnowYourSkyrim.browserTyping == this._parent.which)
      {
         KnowYourSkyrim.endBrowserTyping(true);
      }
   };
   field.onChanged = function()
   {
      KnowYourSkyrim.browserTyped();
   };
   box.touch.selectable = false;
   // a press anywhere on the box puts the caret at the end of the words
   box.onPress = function()
   {
      KnowYourSkyrim.startBrowserTyping(this.which);
   };
   box.onPressAux = function()
   {
   };
};

// The words of the search look in the names, in the descriptions or in both, a button before the field says which and
// a press on it takes the next
KnowYourSkyrim.SEARCH_WITHIN = [
   {key:"names", words:"Names", sign:"lens"},
   {key:"texts", words:"Descriptions", sign:"book"},
   {key:"both", words:"Both", sign:"browser"}
];

KnowYourSkyrim.buildBrowserSearch = function(browser, x, y, wide)
{
   var within = KnowYourSkyrim.buildSearchWithin(browser, x, y);
   var gap = KnowYourSkyrim.BROWSER_FIELD_GAP;
   var search = KnowYourSkyrim.buildBrowserField(browser, "search", 1, x + within.wide + gap, y, wide - within.wide - gap, "lens", KnowYourSkyrim.say("Search: words, @mod, #keyword, $effect, -leave out"), "search");
   search.field.text = KnowYourSkyrim.browserState.words;
   KnowYourSkyrim.drawBrowserField(search);
};

KnowYourSkyrim.keptWithin = function(value)
{
   var i = 0;
   while(i < KnowYourSkyrim.SEARCH_WITHIN.length)
   {
      if(KnowYourSkyrim.SEARCH_WITHIN[i].key == String(value))
      {
         return KnowYourSkyrim.SEARCH_WITHIN[i].key;
      }
      i++;
   }
   return "names";
};

// as wide as its longest word, so a press never moves the field beside it
KnowYourSkyrim.buildSearchWithin = function(browser, x, y)
{
   var button = browser.createEmptyMovieClip("within", 9);
   var high = KnowYourSkyrim.BROWSER_HEAD;
   var widest = 0;
   var i = 0;
   button._x = x;
   button._y = y;
   button.createEmptyMovieClip("look", 0);
   KnowYourSkyrim.writeLine(button, "label", 1, "", KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.PARCHMENT);
   while(i < KnowYourSkyrim.SEARCH_WITHIN.length)
   {
      button.label.text = KnowYourSkyrim.say(KnowYourSkyrim.SEARCH_WITHIN[i].words).toUpperCase();
      widest = Math.max(widest, button.label.textWidth);
      i++;
   }
   button.wide = Math.ceil(widest) + high + 8;
   button.label._x = high - 2;
   button.createTextField("touch", 2, 0, 0, button.wide, high);
   button.touch.selectable = false;
   button.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawSearchWithin();
   };
   button.onRollOut = button.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawSearchWithin();
   };
   button.onPress = function()
   {
      KnowYourSkyrim.nextSearchWithin();
   };
   KnowYourSkyrim.drawSearchWithin();
   return button;
};

KnowYourSkyrim.searchWithin = function()
{
   var i = 0;
   while(i < KnowYourSkyrim.SEARCH_WITHIN.length)
   {
      if(KnowYourSkyrim.SEARCH_WITHIN[i].key == KnowYourSkyrim.browserState.within)
      {
         return KnowYourSkyrim.SEARCH_WITHIN[i];
      }
      i++;
   }
   return KnowYourSkyrim.SEARCH_WITHIN[0];
};

// a search past the names is told by gold, the way a mod picked is
KnowYourSkyrim.drawSearchWithin = function()
{
   var button = KnowYourSkyrim.browser.within;
   var within = KnowYourSkyrim.searchWithin();
   var high = KnowYourSkyrim.BROWSER_HEAD;
   var chosen = within.key != "names";
   var color = chosen || button.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT;
   button.look.clear();
   button.look.lineStyle(1, chosen ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE, chosen ? 60 : 30, true);
   button.look.beginFill(0, 55);
   KnowYourSkyrim.roundBox(button.look, 0, 0, button.wide, high, 4);
   button.look.endFill();
   button.look.lineStyle();
   KnowYourSkyrim.drawIcon(button.look, within.sign, 6, 4, high - 8, color, 80);
   button.label.text = KnowYourSkyrim.say(within.words).toUpperCase();
   button.label.textColor = color;
   button.label._y = Math.round((high - button.label._height) / 2);
};

KnowYourSkyrim.nextSearchWithin = function()
{
   var state = KnowYourSkyrim.browserState;
   var all = KnowYourSkyrim.SEARCH_WITHIN;
   var i = 0;
   while(i < all.length && all[i].key != state.within)
   {
      i++;
   }
   state.within = all[(i + 1) % all.length].key;
   KnowYourSkyrim.drawSearchWithin();
   KnowYourSkyrim.findBrowser(0, true);
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// The plugin to look in. Typed letters list the plugins that have them, a press on one keeps to it, a cross lets it go
KnowYourSkyrim.buildBrowserMods = function(browser, x, y, wide)
{
   var mods = KnowYourSkyrim.buildBrowserField(browser, "mods", 7, x, y, wide, "browser", KnowYourSkyrim.say("Every mod"), "mods");
   // the cross lies over the field as its own clip, a clip that takes presses keeps them from what it holds
   var cross = browser.createEmptyMovieClip("modsCross", 8);
   var size = KnowYourSkyrim.BROWSER_HEAD - 8;
   mods.cross = cross;
   cross._x = x + wide - size - 4;
   cross._y = y + 4;
   cross.createTextField("touch", 0, 0, 0, size, size);
   cross.touch.selectable = false;
   cross.createEmptyMovieClip("sign", 1);
   cross.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawBrowserField(KnowYourSkyrim.browser.mods);
   };
   cross.onRollOut = cross.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawBrowserField(KnowYourSkyrim.browser.mods);
   };
   cross.onPress = function()
   {
      KnowYourSkyrim.pickBrowserMod("");
   };
   mods.field._width = wide - 32 - size;
   KnowYourSkyrim.showModLabel(mods);
   KnowYourSkyrim.drawBrowserField(mods);
};

// the mod picked in its field while nothing is typed there, a long name ends in dots instead of running under the cross
KnowYourSkyrim.showModLabel = function(box)
{
   KnowYourSkyrim.fitText(box.field, KnowYourSkyrim.modLabel(KnowYourSkyrim.browserState.mod));
};

// a plugin's file name as the player reads it, without .esp, .esm or .esl
KnowYourSkyrim.modStem = function(name)
{
   var text = String(name);
   var ending = text.substr(-4).toLowerCase();
   return ending == ".esp" || ending == ".esm" || ending == ".esl" ? text.substr(0, text.length - 4) : text;
};

// A mod that only changes textures is named "textures:<mod>" by the plugin, it has no plugin file of its own
KnowYourSkyrim.TEXTURES = "textures:";
KnowYourSkyrim.texturesMod = function(name)
{
   return String(name).substr(0, KnowYourSkyrim.TEXTURES.length) == KnowYourSkyrim.TEXTURES;
};

// The files of the game itself told apart from mods: Skyrim and Update are the base game, Dawnguard, HearthFires
// and Dragonborn its DLC, the cc files and the resource pack of the Anniversary Edition the Creation Club
KnowYourSkyrim.modLabel = function(name)
{
   var stem = KnowYourSkyrim.modStem(name);
   var lower = stem.toLowerCase();
   var kind;
   if(KnowYourSkyrim.texturesMod(name))
   {
      return KnowYourSkyrim.say("# (textures)", String(name).substr(KnowYourSkyrim.TEXTURES.length));
   }
   if(lower == "skyrim" || lower == "update")
   {
      kind = KnowYourSkyrim.say("base game");
   }
   else if(lower == "dawnguard" || lower == "hearthfires" || lower == "dragonborn")
   {
      kind = KnowYourSkyrim.say("DLC");
   }
   else if(lower == "_resourcepack" || lower.substr(0, 2) == "cc" && lower.substr(5, 3) == "sse")
   {
      kind = KnowYourSkyrim.say("Creation Club");
   }
   return kind != undefined ? stem + " (" + kind + ")" : stem;
};

KnowYourSkyrim.drawBrowserField = function(box)
{
   var high = KnowYourSkyrim.BROWSER_HEAD;
   var typing = KnowYourSkyrim.browserTyping == box.which;
   var chosen = box.which == "mods" && KnowYourSkyrim.browserState.mod != "";
   box.look.clear();
   box.look.lineStyle(1, typing || chosen ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE, typing ? 90 : (chosen ? 60 : 30), true);
   box.look.beginFill(0, 55);
   KnowYourSkyrim.roundBox(box.look, 0, 0, box.wide, high, 4);
   box.look.endFill();
   box.look.lineStyle();
   KnowYourSkyrim.drawIcon(box.look, box.sign, 6, 4, high - 8, typing || chosen ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 80);
   box.hint._visible = !typing && box.field.text.length == 0;
   if(box.cross != undefined)
   {
      box.cross._visible = chosen && !typing;
      box.cross.sign.clear();
      KnowYourSkyrim.drawIcon(box.cross.sign, "cross", 3, 3, KnowYourSkyrim.BROWSER_HEAD - 14, box.cross.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 90);
   }
};

KnowYourSkyrim.typingBox = function()
{
   var typing = KnowYourSkyrim.browserTyping;
   if(typing == "count")
   {
      return KnowYourSkyrim.browser.details.cheats.count;
   }
   if(typing == "find")
   {
      return KnowYourSkyrim.browser.details.find;
   }
   return typing == "mods" ? KnowYourSkyrim.browser.mods : KnowYourSkyrim.browser.search;
};

KnowYourSkyrim.drawTypingBox = function(box)
{
   if(box.which == "count")
   {
      KnowYourSkyrim.drawCheatCount(box);
   }
   else if(box.which == "find")
   {
      KnowYourSkyrim.drawFindBox(box);
   }
   else
   {
      KnowYourSkyrim.drawBrowserField(box);
   }
};

// While the player types, the keys write into the field and the lists of the menu don't act on them, as SkyUI does for
// its own search, because a typed R would drop the selected item otherwise. It starts only while the lists answer, so
// they answer again when it ends. SkyUI's own search lets go first, it would give the lists back meanwhile. The plugin
// hears of it too, a script that closes the menu while typing would leave the keys taken
KnowYourSkyrim.startBrowserTyping = function(which)
{
   var lists = KnowYourSkyrim.menu.inventoryLists;
   var focus;
   var box;
   if(which == undefined)
   {
      which = "search";
   }
   if(KnowYourSkyrim.browserTyping == which)
   {
      box = KnowYourSkyrim.typingBox();
      Selection.setSelection(box.field.text.length, box.field.text.length);
      return undefined;
   }
   if(KnowYourSkyrim.browserTyping != undefined && KnowYourSkyrim.browserTyping != false)
   {
      KnowYourSkyrim.endBrowserTyping(false);
   }
   if(lists.searchWidget._bActive)
   {
      lists.searchWidget.endInput();
   }
   if(!KnowYourSkyrim.listsAnswer())
   {
      return undefined;
   }
   focus = gfx.managers.FocusHandler.instance.getFocus(0);
   KnowYourSkyrim.browserFocusWas = focus == undefined || focus instanceof TextField ? lists.itemList : focus;
   KnowYourSkyrim.browserTyping = which;
   box = KnowYourSkyrim.typingBox();
   lists.itemList.disableInput = lists.itemList.disableSelection = lists.categoryList.disableInput = true;
   skse.AllowTextInput(true);
   KnowYourSkyrim.plugin().BrowserTyping(true);
   Selection.setFocus(box.field);
   if(which == "mods")
   {
      // the plugin chosen before stays until another is picked, the letters start from nothing
      box.field.text = "";
      KnowYourSkyrim.showModList();
   }
   // a count is typed anew over the old one
   Selection.setSelection(which == "count" ? 0 : box.field.text.length, box.field.text.length);
   KnowYourSkyrim.drawTypingBox(box);
};

// focusLost is true when the field lost the focus by itself, the focus is left alone then. Otherwise it goes back
// where it was, the list keeps taking the keys of the game as after SkyUI's own search
KnowYourSkyrim.endBrowserTyping = function(focusLost)
{
   var lists = KnowYourSkyrim.menu.inventoryLists;
   var focus = KnowYourSkyrim.browserFocusWas;
   var which = KnowYourSkyrim.browserTyping;
   var enabled;
   if(which == undefined || which == false)
   {
      return undefined;
   }
   KnowYourSkyrim.browserTyping = undefined;
   lists.itemList.disableInput = lists.itemList.disableSelection = lists.categoryList.disableInput = false;
   skse.AllowTextInput(false);
   KnowYourSkyrim.plugin().BrowserTyping(false);
   if(!focusLost && focus != undefined)
   {
      enabled = focus.focusEnabled;
      focus.focusEnabled = true;
      Selection.setFocus(focus, 0);
      focus.focusEnabled = enabled;
   }
   else if(!focusLost)
   {
      Selection.setFocus(null);
   }
   if(which == "count")
   {
      KnowYourSkyrim.countTyped();
      return undefined;
   }
   if(which == "find")
   {
      KnowYourSkyrim.drawFindBox(KnowYourSkyrim.browser.details.find);
      return undefined;
   }
   if(which == "mods")
   {
      KnowYourSkyrim.hideModList();
      KnowYourSkyrim.showModLabel(KnowYourSkyrim.browser.mods);
      KnowYourSkyrim.drawBrowserField(KnowYourSkyrim.browser.mods);
      return undefined;
   }
   KnowYourSkyrim.browserTyped();
   KnowYourSkyrim.drawBrowserField(KnowYourSkyrim.browser.search);
};

// The search runs a moment after the last key. The list of plugins follows every key, a count counts once typed
KnowYourSkyrim.browserTyped = function()
{
   var box = KnowYourSkyrim.typingBox();
   // words put into a field that holds the focus and never held any come in the field's first format, black in a font
   // the game hasn't got, and show as boxes
   if(box.format != undefined && box.field.text.length > 0)
   {
      box.field.setTextFormat(box.format);
      box.field.setNewTextFormat(box.format);
   }
   if(KnowYourSkyrim.browserTyping == "count")
   {
      return undefined;
   }
   if(KnowYourSkyrim.browserTyping == "find")
   {
      KnowYourSkyrim.findTyped();
      return undefined;
   }
   if(KnowYourSkyrim.browserTyping == "mods")
   {
      KnowYourSkyrim.showModList();
      KnowYourSkyrim.drawBrowserField(KnowYourSkyrim.browser.mods);
      return undefined;
   }
   clearInterval(KnowYourSkyrim.browserTimer);
   KnowYourSkyrim.browserTimer = setInterval(KnowYourSkyrim.browserSearchNow, KnowYourSkyrim.BROWSER_DELAY);
};

KnowYourSkyrim.browserSearchNow = function()
{
   var words = KnowYourSkyrim.browser.search.field.text;
   clearInterval(KnowYourSkyrim.browserTimer);
   KnowYourSkyrim.drawBrowserField(KnowYourSkyrim.browser.search);
   if(words == KnowYourSkyrim.browserState.words)
   {
      return undefined;
   }
   KnowYourSkyrim.browserState.words = words;
   KnowYourSkyrim.findBrowser(0, true);
   KnowYourSkyrim.keepBrowser();
};

// Enter and Tab leave the field with its words, Esc empties it first. In the plugin field the arrows walk the list
// and Enter picks. While typing nothing else takes a key
KnowYourSkyrim.browserKey = function(details)
{
   var code = details.navEquivalent;
   var typing = KnowYourSkyrim.browserTyping;
   if(typing == undefined || typing == false)
   {
      return false;
   }
   if(!Shared.GlobalFunc.IsKeyPressed(details))
   {
      return true;
   }
   if(typing == "mods" && (code == gfx.ui.NavigationCode.UP || code == gfx.ui.NavigationCode.DOWN))
   {
      KnowYourSkyrim.moveModLight(code == gfx.ui.NavigationCode.UP ? -1 : 1);
   }
   else if(typing == "find" && (code == gfx.ui.NavigationCode.UP || code == gfx.ui.NavigationCode.DOWN || code == gfx.ui.NavigationCode.ENTER))
   {
      // Enter and down go to the next place the words stand, up to the one before
      KnowYourSkyrim.findNext(code == gfx.ui.NavigationCode.UP ? -1 : 1);
   }
   else if(typing == "find" && code == gfx.ui.NavigationCode.ESCAPE)
   {
      KnowYourSkyrim.typingBox().field.text = "";
      KnowYourSkyrim.findInPage();
      KnowYourSkyrim.endBrowserTyping(false);
   }
   else if(code == gfx.ui.NavigationCode.ESCAPE)
   {
      KnowYourSkyrim.typingBox().field.text = KnowYourSkyrim.untyped(typing);
      KnowYourSkyrim.endBrowserTyping(false);
   }
   else if(code == gfx.ui.NavigationCode.ENTER && typing == "mods")
   {
      KnowYourSkyrim.pickLitMod();
   }
   else if(code == gfx.ui.NavigationCode.ENTER || code == gfx.ui.NavigationCode.TAB || code == gfx.ui.NavigationCode.SHIFT_TAB)
   {
      KnowYourSkyrim.endBrowserTyping(false);
   }
   return true;
};

// what Esc leaves in a field: the search and the words to find empty, the plugin and the count as they were
KnowYourSkyrim.untyped = function(typing)
{
   if(typing == "mods")
   {
      return KnowYourSkyrim.modLabel(KnowYourSkyrim.browserState.mod);
   }
   return typing == "count" ? String(KnowYourSkyrim.cheatCount) : "";
};

// The list of plugins hangs under the field, the ones that give the most first, a row each with how many items,
// living things, quests, places and perks it gives and, after a ghost, how many pseudo-items. It shows a window of the list and
// scrolls under the wheel
KnowYourSkyrim.buildModList = function(browser)
{
   var list = browser.createEmptyMovieClip("modList", 20);
   var box = browser.mods;
   var row;
   var i = 0;
   list._x = box._x;
   list._y = box._y + KnowYourSkyrim.BROWSER_HEAD + 2;
   list.wide = Math.max(box.wide, Math.round(browser.wide * 0.4));
   list._visible = false;
   list.createEmptyMovieClip("back", 0);
   while(i < KnowYourSkyrim.BROWSER_LIST_ROWS)
   {
      row = list.createEmptyMovieClip("row" + i, i + 1);
      row._y = 2 + i * KnowYourSkyrim.BROWSER_LIST_ROW;
      row.createTextField("touch", 0, 0, 0, list.wide, KnowYourSkyrim.BROWSER_LIST_ROW);
      row.touch.selectable = false;
      row.createEmptyMovieClip("light", 1);
      row.createTextField("words", 2, 6, 0, list.wide - 120, KnowYourSkyrim.BROWSER_LIST_ROW);
      KnowYourSkyrim.styleText(row.words, KnowYourSkyrim.BROWSER_SMALL + 1, "left", KnowYourSkyrim.WHITE);
      row.createTextField("count", 3, list.wide - 114, 0, 54, KnowYourSkyrim.BROWSER_LIST_ROW);
      KnowYourSkyrim.styleText(row.count, KnowYourSkyrim.BROWSER_SMALL, "right", KnowYourSkyrim.HERO_DIM);
      row.createEmptyMovieClip("ghost", 4);
      row.ghost._x = list.wide - 54;
      row.ghost._y = Math.round((KnowYourSkyrim.BROWSER_LIST_ROW - 12) / 2);
      KnowYourSkyrim.drawIcon(row.ghost, "ghost", 0, 0, 12, KnowYourSkyrim.HERO_DIM, 80);
      row.createTextField("pseudo", 5, list.wide - 42, 0, 34, KnowYourSkyrim.BROWSER_LIST_ROW);
      KnowYourSkyrim.styleText(row.pseudo, KnowYourSkyrim.BROWSER_SMALL, "left", KnowYourSkyrim.HERO_DIM);
      row.index = i;
      row.onRollOver = function()
      {
         KnowYourSkyrim.modLight = KnowYourSkyrim.modFirst + this.index;
         KnowYourSkyrim.drawModList();
      };
      row.onPress = function()
      {
         KnowYourSkyrim.pickBrowserMod(KnowYourSkyrim.modsFound[KnowYourSkyrim.modFirst + this.index].name);
      };
      row.onPressAux = function()
      {
      };
      i++;
   }
   KnowYourSkyrim.writeLine(list, "none", KnowYourSkyrim.BROWSER_LIST_ROWS + 1, KnowYourSkyrim.say("No such mod"), KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.HERO_DIM);
   list.none._x = 6;
   list.none._y = 3;
   list.createEmptyMovieClip("bar", KnowYourSkyrim.BROWSER_LIST_ROWS + 2);
};

KnowYourSkyrim.showModList = function()
{
   var words = KnowYourSkyrim.browser.mods.field.text;
   KnowYourSkyrim.modsFound = KnowYourSkyrim.plugin().BrowserMods(words);
   if(KnowYourSkyrim.modsFound == undefined)
   {
      KnowYourSkyrim.modsFound = [];
   }
   KnowYourSkyrim.modFirst = 0;
   KnowYourSkyrim.modLight = 0;
   KnowYourSkyrim.browser.modList._visible = true;
   KnowYourSkyrim.drawModList();
   KnowYourSkyrim.coverModList();
};

KnowYourSkyrim.hideModList = function()
{
   KnowYourSkyrim.browser.modList._visible = false;
   KnowYourSkyrim.coverModList();
};

// The model in the tile is drawn over the menu, the part of it the list lies over steps out while the list is open
KnowYourSkyrim.coverModList = function()
{
   var list = KnowYourSkyrim.browser.modList;
   var box;
   if(!list._visible)
   {
      KnowYourSkyrim.plugin().Cover();
      return undefined;
   }
   box = KnowYourSkyrim.shareBox(list);
   KnowYourSkyrim.plugin().Cover(box.from.x, box.from.y, box.to.x, box.to.y);
};

// For the test stand, words put in a field of the browser as if typed, the keys of the test stand reach the game as
// keys and never as letters. Papyrus hands a word over in the case its VM met it first, "Find"
KnowYourSkyrim.typeInBrowser = function(which, words)
{
   KnowYourSkyrim.startBrowserTyping(String(which).toLowerCase());
   KnowYourSkyrim.typingBox().field.text = String(words);
   KnowYourSkyrim.browserTyped();
};

KnowYourSkyrim.drawModList = function()
{
   var list = KnowYourSkyrim.browser.modList;
   var found = KnowYourSkyrim.modsFound;
   var shown = Math.min(found.length, KnowYourSkyrim.BROWSER_LIST_ROWS);
   var high = 4 + Math.max(shown, 1) * KnowYourSkyrim.BROWSER_LIST_ROW;
   var row;
   var mod;
   var i = 0;
   list.back.clear();
   list.back.lineStyle(1, KnowYourSkyrim.GOLD, 60, true);
   list.back.beginFill(0x111111, 96);
   KnowYourSkyrim.roundBox(list.back, 0, 0, list.wide, high, 4);
   list.back.endFill();
   list.back.lineStyle();
   // the list takes the mouse over all of it, nothing under it is pressed through
   if(list.back.touch == undefined)
   {
      list.back.createTextField("touch", 0, 0, 0, 10, 10);
      list.back.touch.selectable = false;
   }
   list.back.touch._width = list.wide;
   list.back.touch._height = high;
   while(i < KnowYourSkyrim.BROWSER_LIST_ROWS)
   {
      row = list["row" + i];
      mod = found[KnowYourSkyrim.modFirst + i];
      row._visible = i < shown && mod != undefined;
      if(row._visible)
      {
         KnowYourSkyrim.fitText(row.words, KnowYourSkyrim.modLabel(mod.name));
         // a mod of textures counts the things it changes once every model is read
         row.count.text = KnowYourSkyrim.texturesMod(mod.name) ? (mod.changes != undefined ? String(mod.changes) : "") : String(KnowYourSkyrim.number(mod.items) + KnowYourSkyrim.number(mod.living) + KnowYourSkyrim.number(mod.quests) + KnowYourSkyrim.number(mod.places) + KnowYourSkyrim.number(mod.perks));
         row.ghost._visible = KnowYourSkyrim.number(mod.pseudo) > 0;
         row.pseudo.text = KnowYourSkyrim.number(mod.pseudo) > 0 ? String(mod.pseudo) : "";
         row.light.clear();
         if(KnowYourSkyrim.modFirst + i == KnowYourSkyrim.modLight)
         {
            row.light.beginFill(KnowYourSkyrim.GOLD, 18);
            KnowYourSkyrim.roundBox(row.light, 2, 1, list.wide - 4, KnowYourSkyrim.BROWSER_LIST_ROW - 2, 3);
            row.light.endFill();
         }
         row.words.textColor = mod.name == KnowYourSkyrim.browserState.mod ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE;
      }
      i++;
   }
   list.none._visible = found.length == 0;
   list.bar.clear();
   if(found.length > KnowYourSkyrim.BROWSER_LIST_ROWS)
   {
      var track = high - 4;
      var thumb = Math.max(14, track * KnowYourSkyrim.BROWSER_LIST_ROWS / found.length);
      var at = 2 + (track - thumb) * KnowYourSkyrim.modFirst / (found.length - KnowYourSkyrim.BROWSER_LIST_ROWS);
      list.bar.beginFill(KnowYourSkyrim.GOLD, 70);
      KnowYourSkyrim.roundBox(list.bar, list.wide - KnowYourSkyrim.BROWSER_BAR - 2, at, KnowYourSkyrim.BROWSER_BAR, thumb, 2);
      list.bar.endFill();
   }
};

KnowYourSkyrim.scrollModList = function(by)
{
   var most = Math.max(0, KnowYourSkyrim.modsFound.length - KnowYourSkyrim.BROWSER_LIST_ROWS);
   KnowYourSkyrim.modFirst = Math.max(0, Math.min(most, KnowYourSkyrim.modFirst + by));
   KnowYourSkyrim.drawModList();
};

KnowYourSkyrim.moveModLight = function(by)
{
   var found = KnowYourSkyrim.modsFound;
   if(found == undefined || found.length == 0)
   {
      return undefined;
   }
   KnowYourSkyrim.modLight = Math.max(0, Math.min(found.length - 1, KnowYourSkyrim.modLight + by));
   if(KnowYourSkyrim.modLight < KnowYourSkyrim.modFirst)
   {
      KnowYourSkyrim.modFirst = KnowYourSkyrim.modLight;
   }
   else if(KnowYourSkyrim.modLight >= KnowYourSkyrim.modFirst + KnowYourSkyrim.BROWSER_LIST_ROWS)
   {
      KnowYourSkyrim.modFirst = KnowYourSkyrim.modLight - KnowYourSkyrim.BROWSER_LIST_ROWS + 1;
   }
   KnowYourSkyrim.drawModList();
};

KnowYourSkyrim.pickLitMod = function()
{
   var found = KnowYourSkyrim.modsFound;
   if(found != undefined && found[KnowYourSkyrim.modLight] != undefined)
   {
      KnowYourSkyrim.pickBrowserMod(found[KnowYourSkyrim.modLight].name);
   }
   else
   {
      KnowYourSkyrim.endBrowserTyping(false);
   }
};

// One plugin to look in, or every one again
KnowYourSkyrim.pickBrowserMod = function(name)
{
   KnowYourSkyrim.browserState.mod = String(name);
   KnowYourSkyrim.endBrowserTyping(false);
   KnowYourSkyrim.showModLabel(KnowYourSkyrim.browser.mods);
   KnowYourSkyrim.drawBrowserField(KnowYourSkyrim.browser.mods);
   KnowYourSkyrim.findBrowser(0, true);
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};
