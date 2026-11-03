KnowYourSkyrim.CHEAT_GAP = 14;
KnowYourSkyrim.CHEAT_COUNT_GAP = 4;
KnowYourSkyrim.CHEAT_COUNT_WIDE = 44;
KnowYourSkyrim.CHEAT_MOST = 9999;
KnowYourSkyrim.cheatCount = 1;
// frames a quest's scripts get to run the fragment of the stage a cheat set before its page is asked once more
KnowYourSkyrim.DEED_SETTLE = 45;
KnowYourSkyrim.PERK_SETTLE = 30;
KnowYourSkyrim.PERK_TRIES = 10;

// The cheats, while the player switched them on in Data/SKSE/Plugins/KnowYourSkyrim.ini: buttons on the line of
// INSPECT under the head of the details. TAKE gives as many of an item as its box says, KILL kills a unique person or
// creature, essential ones too, RESURRECT brings a dead one back, START starts a quest and COMPLETE finishes it, ADD
// gives the player the next rank of a perk they lack and REMOVE takes away the highest one they have. The plugin does
// the deed and checks the switch itself too
KnowYourSkyrim.buildCheats = function(details)
{
   var cheats = details.createEmptyMovieClip("cheats", 7);
   cheats._visible = false;
   KnowYourSkyrim.cheatButton(cheats, "take", 0, KnowYourSkyrim.say("Take"), "plus");
   KnowYourSkyrim.cheatButton(cheats, "kill", 1, KnowYourSkyrim.say("Kill"), "skull");
   KnowYourSkyrim.cheatButton(cheats, "raise", 2, KnowYourSkyrim.say("Resurrect"), "heart");
   KnowYourSkyrim.cheatButton(cheats, "start", 4, KnowYourSkyrim.say("Start"), "quest");
   KnowYourSkyrim.cheatButton(cheats, "finish", 5, KnowYourSkyrim.say("Complete"), "check");
   KnowYourSkyrim.cheatButton(cheats, "add", 6, KnowYourSkyrim.say("Add"), "plus");
   KnowYourSkyrim.cheatButton(cheats, "remove", 7, KnowYourSkyrim.say("Remove"), "cross");
   KnowYourSkyrim.buildCheatCount(cheats);
};

// how many TAKE gives, typed into a small box after it the way the search is typed
KnowYourSkyrim.buildCheatCount = function(cheats)
{
   var high = KnowYourSkyrim.BROWSER_INSPECT_HIGH;
   var wide = KnowYourSkyrim.CHEAT_COUNT_WIDE;
   var box = cheats.createEmptyMovieClip("count", 3);
   var field;
   box.wide = wide;
   box.which = "count";
   box.createEmptyMovieClip("look", 0);
   box.createTextField("field", 1, 2, 0, wide - 4, high);
   field = box.field;
   KnowYourSkyrim.styleText(field, KnowYourSkyrim.BROWSER_SMALL, "center", KnowYourSkyrim.WHITE);
   field.type = "input";
   field.selectable = true;
   field.noTranslate = true;
   field.restrict = "0-9";
   field.maxChars = String(KnowYourSkyrim.CHEAT_MOST).length;
   field.text = String(KnowYourSkyrim.cheatCount);
   box.createTextField("touch", 2, 0, 0, wide, high);
   KnowYourSkyrim.listenField(box);
   KnowYourSkyrim.drawCheatCount(box);
};

KnowYourSkyrim.drawCheatCount = function(box)
{
   var typing = KnowYourSkyrim.browserTyping == "count";
   box.look.clear();
   box.look.lineStyle(1, typing ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE, typing ? 90 : 30, true);
   box.look.beginFill(0, 55);
   KnowYourSkyrim.roundBox(box.look, 0, 0, box.wide, KnowYourSkyrim.BROWSER_INSPECT_HIGH, 4);
   box.look.endFill();
   box.look.lineStyle();
};

// what was typed becomes the count, anything that isn't one gives 1
KnowYourSkyrim.countTyped = function()
{
   var box = KnowYourSkyrim.browser.details.cheats.count;
   var count = Math.floor(Number(box.field.text));
   KnowYourSkyrim.cheatCount = isNaN(count) || count < 1 ? 1 : Math.min(count, KnowYourSkyrim.CHEAT_MOST);
   box.field.text = String(KnowYourSkyrim.cheatCount);
   KnowYourSkyrim.drawCheatCount(box);
};

// a button like INSPECT: a sign and words in small capitals, gold under the pointer
KnowYourSkyrim.cheatButton = function(cheats, name, depth, words, sign)
{
   var button = cheats.createEmptyMovieClip(name, depth);
   button.createTextField("touch", 0, 0, 0, 10, 10);
   button.touch.selectable = false;
   button.createEmptyMovieClip("sign", 1);
   KnowYourSkyrim.writeLine(button, "label", 2, words.toUpperCase(), KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.PARCHMENT);
   button.label._x = 20;
   button.label._y = Math.round((KnowYourSkyrim.BROWSER_INSPECT_HIGH - button.label._height) / 2);
   button.wide = button.label._x + button.label._width + 4;
   button.touch._width = button.wide;
   button.touch._height = KnowYourSkyrim.BROWSER_INSPECT_HIGH;
   button.icon = sign;
   button.deed = name;
   button.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawCheatButton(this);
   };
   button.onRollOut = button.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawCheatButton(this);
   };
   button.onPress = function()
   {
      KnowYourSkyrim.cheat(this.deed);
   };
   KnowYourSkyrim.drawCheatButton(button);
};

KnowYourSkyrim.drawCheatButton = function(button)
{
   button.sign.clear();
   KnowYourSkyrim.drawIcon(button.sign, button.icon, 0, 2, 14, button.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 100);
   button.label.textColor = button.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT;
};

// What the entry offers: an item to take, the living and the dead of a person or creature the game holds now, a
// quest to start until the player has it in the journal and to finish until it's done, a rank of a perk to add while
// the player lacks one and to remove while they have one. A quest that waits for the player runs already, START gives
// it its first journal stage, one with no journal can't be started. A place and a pseudo-item offer nothing
KnowYourSkyrim.cheatDeeds = function(told, targets)
{
   var state;
   var deeds = [];
   if(told != undefined && told.quest == true)
   {
      state = Number(told.state);
      if(state == 0 || state == 4 || (state == 1 && told.beginsAt != undefined))
      {
         return ["start", "finish"];
      }
      return state != 3 ? ["finish"] : [];
   }
   if(told != undefined && told.perk == true)
   {
      if(KnowYourSkyrim.number(told.rank) < KnowYourSkyrim.number(told.ranks))
      {
         deeds.push("add");
      }
      if(KnowYourSkyrim.number(told.rank) > 0)
      {
         deeds.push("remove");
      }
      return deeds;
   }
   if(told == undefined || told.pseudo == true || told.place == true)
   {
      return [];
   }
   if(told.living != true)
   {
      return ["take"];
   }
   if(targets != undefined && targets.alive > 0)
   {
      deeds.push("kill");
   }
   if(targets != undefined && targets.dead > 0)
   {
      deeds.push("raise");
   }
   return deeds;
};

// the buttons of the entry in a row, the count right after TAKE, placeButtonLine lays the row on the line of INSPECT
KnowYourSkyrim.showCheats = function(told)
{
   var cheats = KnowYourSkyrim.browser.details.cheats;
   var plugin = KnowYourSkyrim.plugin();
   var on = told != undefined && plugin.Cheats() == true;
   var deeds = on ? KnowYourSkyrim.cheatDeeds(told, told.living == true ? plugin.CheatTargets(told.id) : undefined) : [];
   var names = ["take", "count", "kill", "raise", "start", "finish", "add", "remove"];
   var one;
   var x = 0;
   var i = 0;
   while(i < names.length)
   {
      one = cheats[names[i]];
      one._visible = KnowYourSkyrim.has(deeds, names[i] == "count" ? "take" : names[i]);
      if(one._visible)
      {
         one._x = x;
         x += one.wide + (names[i] == "take" ? KnowYourSkyrim.CHEAT_COUNT_GAP : KnowYourSkyrim.CHEAT_GAP);
      }
      i++;
   }
   cheats._visible = deeds.length > 0;
};

KnowYourSkyrim.cheat = function(deed)
{
   var told = KnowYourSkyrim.browserItem;
   var plugin = KnowYourSkyrim.plugin();
   var done;
   KnowYourSkyrim.endBrowserTyping(false);
   if(told == undefined || !KnowYourSkyrim.listsAnswer())
   {
      return undefined;
   }
   if(deed == "take")
   {
      KnowYourSkyrim.tellBrowser(plugin.CheatTake(told.id, KnowYourSkyrim.cheatCount) == true ? KnowYourSkyrim.takenText(told) : KnowYourSkyrim.say("Nothing to take"));
   }
   else if(deed == "start" || deed == "finish")
   {
      done = plugin.CheatQuest(told.id, deed == "finish");
      KnowYourSkyrim.tellBrowser(KnowYourSkyrim.questDeedText(deed, told, done));
   }
   else if(deed == "add" || deed == "remove")
   {
      done = plugin.CheatPerk(told.id, deed == "add");
      KnowYourSkyrim.tellBrowser(KnowYourSkyrim.perkDeedText(deed, told, done));
   }
   else
   {
      done = deed == "kill" ? plugin.CheatKill(told.id) : plugin.CheatRaise(told.id);
      if(done == undefined)
      {
         KnowYourSkyrim.tellBrowser(KnowYourSkyrim.say("No one of them is about"));
      }
      else
      {
         KnowYourSkyrim.tellBrowser(KnowYourSkyrim.cheatDoneText(deed, done));
      }
   }
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// a quest is finished by the stage its journal entry finishes it at, when one does, and one that waits for the
// player is started by its first journal stage
KnowYourSkyrim.questDeedText = function(deed, told, done)
{
   var name = KnowYourSkyrim.gameName(told.name);
   if(done == undefined)
   {
      return KnowYourSkyrim.say("Cheats are off");
   }
   if(deed == "start" && done.stage == undefined)
   {
      return KnowYourSkyrim.say("Started: #", name);
   }
   return done.stage != undefined ? KnowYourSkyrim.say("#1 set to stage #2", name, done.stage) : KnowYourSkyrim.say("Completed: #", name);
};

// the rank given or taken away, said by its number when the perk has several
KnowYourSkyrim.perkDeedText = function(deed, told, done)
{
   var name = KnowYourSkyrim.gameName(told.name);
   if(done == undefined)
   {
      return KnowYourSkyrim.say("Cheats are off");
   }
   if(deed == "add")
   {
      return done.ranks > 1 ? KnowYourSkyrim.say("Added: #1, rank #2 of #3", name, done.rank, done.ranks) : KnowYourSkyrim.say("Added: #", name);
   }
   return done.ranks > 1 ? KnowYourSkyrim.say("Removed: #1, rank #2 of #3", name, done.rank, done.ranks) : KnowYourSkyrim.say("Removed: #", name);
};

KnowYourSkyrim.takenText = function(told)
{
   var name = KnowYourSkyrim.gameName(told.name);
   return KnowYourSkyrim.cheatCount > 1 ? KnowYourSkyrim.say("Taken: #1 (#2)", name, KnowYourSkyrim.cheatCount) : KnowYourSkyrim.say("Taken: #", name);
};

KnowYourSkyrim.cheatDoneText = function(deed, done)
{
   var place = done.place != undefined && done.place != "" ? done.place : undefined;
   if(deed == "kill")
   {
      return place != undefined ? KnowYourSkyrim.say("#1 killed, #2", done.name, place) : KnowYourSkyrim.say("# killed", done.name);
   }
   return place != undefined ? KnowYourSkyrim.say("#1 brought back, #2", done.name, place) : KnowYourSkyrim.say("# brought back", done.name);
};

// The plugin calls this once the game did a deed of the cheats, on its own thread or by its scripts some time after the
// press, so the buttons of the entry and the list of someone's things are asked again
KnowYourSkyrim.deedDone = function()
{
   var told = KnowYourSkyrim.browserItem;
   if(!KnowYourSkyrim.browsing)
   {
      return undefined;
   }
   if(told != undefined && told.quest == true)
   {
      KnowYourSkyrim.entryMoved(told.id);
      KnowYourSkyrim.settleEntry(told.id);
   }
   else if(told != undefined && told.perk == true)
   {
      KnowYourSkyrim.entryMoved(told.id);
      KnowYourSkyrim.settlePerk(told.id, KnowYourSkyrim.number(told.rank));
   }
   else if(told != undefined)
   {
      KnowYourSkyrim.showCheats(told);
      KnowYourSkyrim.placeButtonLine();
   }
   if(KnowYourSkyrim.belongings != undefined)
   {
      KnowYourSkyrim.askBelongings();
   }
};

// A quest or a perk the cheats moved has its head, its page and its row asked anew. The game runs a quest stage's
// fragment in its scripts a while later, the page of a quest asks once more after that (settleEntry), a perk's page
// asks until the game gave or took the rank (settlePerk)
KnowYourSkyrim.entryMoved = function(id)
{
   var plugin = KnowYourSkyrim.plugin();
   var told;
   if(!KnowYourSkyrim.browsing || KnowYourSkyrim.browserItem == undefined || KnowYourSkyrim.browserItem.id != id)
   {
      return undefined;
   }
   told = plugin.BrowserItem(id);
   if(told == undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.browserItem = told;
   KnowYourSkyrim.browser.details.createEmptyMovieClip("head", 1);
   KnowYourSkyrim.drawBrowserHead(told);
   KnowYourSkyrim.showCheats(told);
   KnowYourSkyrim.placeButtonLine();
   KnowYourSkyrim.browserSources = plugin.BrowserSources(id, KnowYourSkyrim.anyListOpen());
   KnowYourSkyrim.drawBrowserPage(true);
   KnowYourSkyrim.findBrowser(KnowYourSkyrim.browserState.first, false);
};

// The game gives or takes a rank the console asked for some frames later while the menu is open, a second rank of Armor
// Breaker took more than 45 frames. So the page is asked every half a second until the rank moved, five seconds at most
KnowYourSkyrim.settlePerk = function(id, before)
{
   var cheats = KnowYourSkyrim.browser.details.cheats;
   var told = KnowYourSkyrim.browserItem;
   if(told == undefined || told.id != id || KnowYourSkyrim.number(told.rank) != before)
   {
      return undefined;
   }
   cheats.settle = KnowYourSkyrim.PERK_SETTLE;
   cheats.tries = KnowYourSkyrim.PERK_TRIES;
   cheats.onEnterFrame = function()
   {
      var now;
      if(--this.settle > 0)
      {
         return undefined;
      }
      KnowYourSkyrim.entryMoved(id);
      now = KnowYourSkyrim.browserItem;
      if(--this.tries <= 0 || now == undefined || now.id != id || KnowYourSkyrim.number(now.rank) != before)
      {
         delete this.onEnterFrame;
         return undefined;
      }
      this.settle = KnowYourSkyrim.PERK_SETTLE;
   };
};

KnowYourSkyrim.settleEntry = function(id)
{
   var cheats = KnowYourSkyrim.browser.details.cheats;
   cheats.settle = KnowYourSkyrim.DEED_SETTLE;
   cheats.onEnterFrame = function()
   {
      if(--this.settle > 0)
      {
         return undefined;
      }
      delete this.onEnterFrame;
      KnowYourSkyrim.entryMoved(id);
   };
};
