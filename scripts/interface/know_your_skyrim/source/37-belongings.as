KnowYourSkyrim.BELONGINGS_HEAD = 30;
KnowYourSkyrim.BELONGINGS_FOOT = 28;

// The things one of a living entry has on them now, in a list of the browser's own over SkyUI's list. SkyUI's list and
// the hints of the bar step out while it's open and the keys of the menu don't reach them, a key there would drop or
// equip a thing of the player's. A press on a thing shows its page in the browser. With the cheats on a tab beside
// holds the player's things, and a right click on a thing opens a menu that takes one or all of it from them or gives
// one or all of the player's. Esc or Tab puts SkyUI's list back
KnowYourSkyrim.openBelongings = function(id)
{
   var told = KnowYourSkyrim.plugin().Belongings(id, false);
   if(told == undefined)
   {
      KnowYourSkyrim.tellBrowser(KnowYourSkyrim.say("No one of them is about"));
      return undefined;
   }
   KnowYourSkyrim.closeBelongings();
   KnowYourSkyrim.belongings = {entry:id, player:false, told:told, first:0, selected:0};
   KnowYourSkyrim.layBelongings();
   KnowYourSkyrim.menu.inventoryLists._visible = false;
   KnowYourSkyrim.menu.bottomBar.buttonPanel._visible = false;
   KnowYourSkyrim.drawBelongings();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.closeBelongings = function()
{
   if(KnowYourSkyrim.belongings == undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.closeContext();
   KnowYourSkyrim.belongings = undefined;
   KnowYourSkyrim.belongingsClip.removeMovieClip();
   KnowYourSkyrim.belongingsClip = undefined;
   KnowYourSkyrim.menu.inventoryLists._visible = true;
   KnowYourSkyrim.menu.bottomBar.buttonPanel._visible = true;
};

// The list takes the room of SkyUI's list frame, from the top of the screen down to the bottom bar, in the units of
// the browser's own layer. The lists' own box reaches far above the screen, the frame is what SkyUI draws
KnowYourSkyrim.belongingsRoom = function()
{
   var menu = KnowYourSkyrim.menu;
   var lists = menu.inventoryLists;
   var bounds = lists.getContentBounds != undefined ? lists.getContentBounds() : [0, 0, Stage.visibleRect.width / 2, 0];
   var layer = KnowYourSkyrim.aloneLayer();
   var screen = Stage.visibleRect;
   var margin = KnowYourSkyrim.ROOM_MARGIN;
   var from = {x:lists._x + bounds[0], y:0};
   var to = {x:lists._x + bounds[0] + bounds[2] + KnowYourSkyrim.LIST_FRAME, y:menu.bottomBar._y};
   menu.localToGlobal(from);
   menu.localToGlobal(to);
   from.y = screen.y + Stage.safeRect.y;
   layer.globalToLocal(from);
   layer.globalToLocal(to);
   return {left:from.x + margin / 2, top:from.y + margin, right:to.x - margin / 2, bottom:to.y - margin};
};

KnowYourSkyrim.layBelongings = function()
{
   var layer = KnowYourSkyrim.aloneLayer();
   var room = KnowYourSkyrim.belongingsRoom();
   var clip = layer.createEmptyMovieClip("belongings", layer.getNextHighestDepth());
   var pad = KnowYourSkyrim.BROWSER_PAD;
   var wide = room.right - room.left;
   var high = room.bottom - room.top;
   var listHigh = high - KnowYourSkyrim.BELONGINGS_HEAD - KnowYourSkyrim.BELONGINGS_FOOT - 2 * pad;
   var count = KnowYourSkyrim.browserRowsFit(listHigh);
   var row;
   var cross;
   var size = KnowYourSkyrim.BROWSER_HEAD - 8;
   var i = 0;
   KnowYourSkyrim.belongingsClip = clip;
   clip._x = room.left;
   clip._y = room.top;
   clip.wide = wide;
   clip.high = high;
   clip.count = count;
   clip.createEmptyMovieClip("back", 0);
   KnowYourSkyrim.panelBox(clip.back, wide, high, KnowYourSkyrim.BROWSER_SHADE, 40);
   // a drawn shape takes no mouse, a press on the bare back would reach what lies under the list
   clip.back.createTextField("touch", 0, 0, 0, wide, high);
   clip.back.touch.selectable = false;
   clip.back.onPress = function()
   {
   };
   cross = clip.createEmptyMovieClip("cross", 2);
   cross._x = wide - pad - size;
   cross._y = pad + Math.round((KnowYourSkyrim.BELONGINGS_HEAD - size) / 2);
   cross.createTextField("touch", 0, 0, 0, size, size);
   cross.touch.selectable = false;
   cross.createEmptyMovieClip("sign", 1);
   cross.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawBelongingsCross();
   };
   cross.onRollOut = cross.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawBelongingsCross();
   };
   cross.onPress = function()
   {
      KnowYourSkyrim.closeBelongings();
      gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
   };
   clip.createEmptyMovieClip("rows", 3);
   clip.rows._x = pad;
   clip.rows._y = pad + KnowYourSkyrim.BELONGINGS_HEAD + 4;
   clip.rows.wide = wide - 2 * pad - KnowYourSkyrim.BROWSER_BAR - 6;
   clip.rows.high = count * KnowYourSkyrim.BROWSER_ROW;
   while(i < count)
   {
      row = clip.rows.createEmptyMovieClip("row" + i, i + 1);
      KnowYourSkyrim.buildBelongingsRow(row, clip.rows.wide);
      row._y = i * KnowYourSkyrim.BROWSER_ROW;
      row.index = i;
      i++;
   }
   KnowYourSkyrim.writeLine(clip.rows, "none", count + 1, KnowYourSkyrim.say("Carries nothing"), KnowYourSkyrim.BROWSER_TEXT, KnowYourSkyrim.HERO_DIM);
   clip.rows.none._x = 6;
   clip.rows.none._y = 4;
   clip.rows.createEmptyMovieClip("bar", count + 2);
   clip.rows.bar._x = wide - 2 * pad - KnowYourSkyrim.BROWSER_BAR;
   clip.createEmptyMovieClip("foot", 4);
   clip.foot._x = pad;
   clip.foot._y = high - pad - KnowYourSkyrim.BELONGINGS_FOOT + Math.round((KnowYourSkyrim.BELONGINGS_FOOT - KnowYourSkyrim.BROWSER_INSPECT_HIGH) / 2);
   KnowYourSkyrim.writeLine(clip.foot, "hint", 0, "", KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.HERO_DIM);
};

// a row is the sign of the thing, its name and how many there are on the right
KnowYourSkyrim.buildBelongingsRow = function(row, wide)
{
   var high = KnowYourSkyrim.BROWSER_ROW;
   row.createTextField("touch", 0, 0, 0, wide, high);
   row.touch.selectable = false;
   row.createEmptyMovieClip("light", 1);
   row.createEmptyMovieClip("sign", 2);
   row.createTextField("words", 3, KnowYourSkyrim.BROWSER_ICON + 10, Math.round((high - 20) / 2), wide - KnowYourSkyrim.BROWSER_ICON - 76, high);
   KnowYourSkyrim.styleText(row.words, KnowYourSkyrim.BROWSER_TEXT, "left", KnowYourSkyrim.WHITE);
   row.createTextField("count", 4, wide - 60, Math.round((high - 18) / 2), 56, high);
   KnowYourSkyrim.styleText(row.count, KnowYourSkyrim.BROWSER_SMALL, "right", KnowYourSkyrim.HERO_DIM);
   row.wide = wide;
   row.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawBelongingsRow(this);
   };
   row.onRollOut = row.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawBelongingsRow(this);
   };
   row.onPress = function()
   {
      KnowYourSkyrim.pickBelonging(this.thing);
   };
   row.onPressAux = function()
   {
      KnowYourSkyrim.belongingsMenu(this.thing);
   };
};

// A right click picks the thing and, with the cheats on, opens what can be done with it on this tab: take one or all
// from them, give one or all to them
KnowYourSkyrim.belongingsMenu = function(thing)
{
   var state = KnowYourSkyrim.belongings;
   var lines = [];
   if(thing == undefined || state == undefined || KnowYourSkyrim.plugin().Cheats() != true)
   {
      return undefined;
   }
   state.selected = thing.id;
   KnowYourSkyrim.drawBelongings();
   lines.push({key:"one", words:state.player ? KnowYourSkyrim.say("Give one") : KnowYourSkyrim.say("Take one"), icon:"plus", run:function()
   {
      KnowYourSkyrim.moveBelonging(false);
   }});
   if(thing.count > 1)
   {
      lines.push({key:"all", words:KnowYourSkyrim.say("#1 (#2)", state.player ? KnowYourSkyrim.say("Give all") : KnowYourSkyrim.say("Take all"), thing.count), icon:"transfer", run:function()
      {
         KnowYourSkyrim.moveBelonging(true);
      }});
   }
   KnowYourSkyrim.openContext(lines, _root._xmouse, _root._ymouse);
};

KnowYourSkyrim.drawBelongingsCross = function()
{
   var cross = KnowYourSkyrim.belongingsClip.cross;
   cross.sign.clear();
   KnowYourSkyrim.drawIcon(cross.sign, "cross", 3, 3, KnowYourSkyrim.BROWSER_HEAD - 14, cross.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 90);
};

// The tabs name whose things the list shows: theirs, and with the cheats on the player's beside them
KnowYourSkyrim.drawBelongingsTabs = function()
{
   var state = KnowYourSkyrim.belongings;
   // made anew each time, the tab of the player goes when the cheats went off meanwhile
   var tabs = KnowYourSkyrim.belongingsClip.createEmptyMovieClip("tabs", 1);
   var told = state.told;
   var names = [told.dead == true ? KnowYourSkyrim.say("# (dead)", told.name) : told.name];
   var x = 0;
   var tab;
   var chosen;
   var i = 0;
   tabs._x = tabs._y = KnowYourSkyrim.BROWSER_PAD;
   if(KnowYourSkyrim.plugin().Cheats() == true)
   {
      names.push(KnowYourSkyrim.say("You"));
   }
   while(i < names.length)
   {
      tab = tabs.createEmptyMovieClip("tab" + i, i);
      tab.player = i == 1;
      KnowYourSkyrim.writeLine(tab, "words", 1, names[i].toUpperCase(), KnowYourSkyrim.CHIP_TEXT, KnowYourSkyrim.WHITE);
      tab.words._y = Math.round((KnowYourSkyrim.BELONGINGS_HEAD - tab.words._height) / 2);
      tab.wide = tab.words._width;
      tab.createTextField("touch", 0, 0, 0, tab.wide, KnowYourSkyrim.BELONGINGS_HEAD);
      tab.touch.selectable = false;
      tab.createEmptyMovieClip("line", 2);
      tab._x = x;
      tab.onPress = function()
      {
         KnowYourSkyrim.belongingsTab(this.player);
      };
      x += tab.wide + KnowYourSkyrim.BROWSER_TAB_GAP * 2;
      chosen = tab.player == state.player;
      tab.words.textColor = chosen ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE;
      tab.words._alpha = chosen ? 100 : KnowYourSkyrim.CHIP_DIM;
      if(chosen && names.length > 1)
      {
         tab.line.lineStyle(2, KnowYourSkyrim.GOLD, 85, true);
         tab.line.moveTo(0, KnowYourSkyrim.BELONGINGS_HEAD - 2);
         tab.line.lineTo(tab.wide, KnowYourSkyrim.BELONGINGS_HEAD - 2);
      }
      i++;
   }
};

KnowYourSkyrim.belongingsTab = function(player)
{
   var state = KnowYourSkyrim.belongings;
   if(state == undefined || state.player == player)
   {
      return undefined;
   }
   state.player = player;
   state.first = 0;
   state.selected = 0;
   KnowYourSkyrim.askBelongings();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// the list asked again of the plugin, what was picked stays picked while it's there
KnowYourSkyrim.askBelongings = function()
{
   var state = KnowYourSkyrim.belongings;
   var told = KnowYourSkyrim.plugin().Belongings(state.entry, state.player);
   if(told == undefined)
   {
      KnowYourSkyrim.closeBelongings();
      KnowYourSkyrim.tellBrowser(KnowYourSkyrim.say("No one of them is about"));
      return undefined;
   }
   state.told = told;
   KnowYourSkyrim.drawBelongings();
};

KnowYourSkyrim.drawBelongings = function()
{
   var state = KnowYourSkyrim.belongings;
   var clip = KnowYourSkyrim.belongingsClip;
   var rows = state.told.rows != undefined ? state.told.rows : [];
   var row;
   var i = 0;
   state.first = Math.max(0, Math.min(state.first, rows.length - clip.count));
   if(state.selected != 0 && KnowYourSkyrim.belongingAt(state.selected) < 0)
   {
      state.selected = 0;
   }
   KnowYourSkyrim.drawBelongingsTabs();
   KnowYourSkyrim.drawBelongingsCross();
   while(i < clip.count)
   {
      row = clip.rows["row" + i];
      row.thing = rows[state.first + i];
      row._visible = row.thing != undefined;
      if(row._visible)
      {
         KnowYourSkyrim.fitText(row.words, KnowYourSkyrim.gameName(row.thing.name), row.thing.worn == true ? "  " + KnowYourSkyrim.say("worn") : "");
         row.more = row.thing.worn == true ? "  " + KnowYourSkyrim.say("worn") : undefined;
         row.count.text = row.thing.count > 1 ? String(row.thing.count) : "";
         row.sign.clear();
         KnowYourSkyrim.drawIcon(row.sign, row.thing.icon, 4, (KnowYourSkyrim.BROWSER_ROW - KnowYourSkyrim.BROWSER_ICON) / 2, KnowYourSkyrim.BROWSER_ICON, KnowYourSkyrim.PARCHMENT, 85);
         KnowYourSkyrim.drawBelongingsRow(row);
      }
      i++;
   }
   clip.rows.none._visible = rows.length == 0;
   KnowYourSkyrim.drawBelongingsBar();
   KnowYourSkyrim.drawBelongingsFoot();
};

KnowYourSkyrim.belongingAt = function(id)
{
   var rows = KnowYourSkyrim.belongings.told.rows;
   var i = 0;
   while(rows != undefined && i < rows.length)
   {
      if(rows[i].id == id)
      {
         return i;
      }
      i++;
   }
   return -1;
};

KnowYourSkyrim.drawBelongingsRow = function(row)
{
   var chosen = row.thing != undefined && row.thing.id == KnowYourSkyrim.belongings.selected;
   var format;
   row.light.clear();
   if(row.thing == undefined)
   {
      return undefined;
   }
   if(chosen || row.lit)
   {
      row.light.beginFill(chosen ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE, chosen ? 16 : 8);
      KnowYourSkyrim.roundBox(row.light, 0, 1, row.wide, KnowYourSkyrim.BROWSER_ROW - 2, 3);
      row.light.endFill();
   }
   row.words.textColor = chosen ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE;
   // worn stays dim after the name
   if(row.more != undefined)
   {
      format = new TextFormat();
      format.color = KnowYourSkyrim.HERO_DIM;
      format.size = KnowYourSkyrim.BROWSER_SMALL;
      row.words.setTextFormat(row.words.text.length - row.more.length, row.words.text.length, format);
   }
};

KnowYourSkyrim.drawBelongingsBar = function()
{
   var state = KnowYourSkyrim.belongings;
   var clip = KnowYourSkyrim.belongingsClip;
   var bar = clip.rows.bar;
   var total = state.told.rows.length;
   var track = clip.rows.high;
   var thumb;
   bar.clear();
   if(total <= clip.count)
   {
      return undefined;
   }
   thumb = Math.max(16, track * clip.count / total);
   bar.beginFill(KnowYourSkyrim.WHITE, 12);
   KnowYourSkyrim.roundBox(bar, 0, 0, KnowYourSkyrim.BROWSER_BAR, track, 2);
   bar.endFill();
   bar.beginFill(KnowYourSkyrim.GOLD, 70);
   KnowYourSkyrim.roundBox(bar, 0, (track - thumb) * state.first / Math.max(1, total - clip.count), KnowYourSkyrim.BROWSER_BAR, thumb, 2);
   bar.endFill();
};

// The foot says what the list is for, with the cheats on where taking and giving are, off that it's to look at only
KnowYourSkyrim.drawBelongingsFoot = function()
{
   var hint = KnowYourSkyrim.belongingsClip.foot.hint;
   var on = KnowYourSkyrim.plugin().Cheats() == true;
   if(!on)
   {
      hint.text = KnowYourSkyrim.say("Look only, the cheats let you take and give");
   }
   else
   {
      hint.text = KnowYourSkyrim.belongings.player ? KnowYourSkyrim.say("Right click a thing of yours to give it") : KnowYourSkyrim.say("Right click a thing to take it");
   }
   hint._width = hint.textWidth + 6;
   hint._y = Math.round((KnowYourSkyrim.BROWSER_INSPECT_HIGH - hint._height) / 2);
};

// A thing picked shows its page in the browser when the browser has one
KnowYourSkyrim.pickBelonging = function(thing)
{
   var state = KnowYourSkyrim.belongings;
   if(thing == undefined || state == undefined)
   {
      return undefined;
   }
   state.selected = thing.id;
   KnowYourSkyrim.drawBelongings();
   if(thing.entry > 0 && KnowYourSkyrim.browsing)
   {
      KnowYourSkyrim.goBrowserItem(thing.entry);
   }
   else
   {
      gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
   }
};

KnowYourSkyrim.moveBelonging = function(all)
{
   var state = KnowYourSkyrim.belongings;
   var at = state != undefined ? KnowYourSkyrim.belongingAt(state.selected) : -1;
   var thing;
   var done;
   if(at < 0)
   {
      return undefined;
   }
   thing = state.told.rows[at];
   done = KnowYourSkyrim.plugin().BelongingsMove(state.entry, thing.id, all, state.player) == true;
   if(done)
   {
      KnowYourSkyrim.tellBrowser(KnowYourSkyrim.movedText(state, thing, all));
   }
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.movedText = function(state, thing, all)
{
   var name = KnowYourSkyrim.gameName(thing.name);
   var count = all ? thing.count : 1;
   var what = count > 1 ? KnowYourSkyrim.say("#1 (#2)", name, count) : name;
   return state.player ? KnowYourSkyrim.say("Given: #", what) : KnowYourSkyrim.say("Taken: #", what);
};

KnowYourSkyrim.scrollBelongings = function(by)
{
   var state = KnowYourSkyrim.belongings;
   KnowYourSkyrim.closeContext();
   state.first = Math.max(0, Math.min(state.told.rows.length - KnowYourSkyrim.belongingsClip.count, state.first + by));
   KnowYourSkyrim.drawBelongings();
};

// While the list is open no key reaches SkyUI's list. Esc and Tab put it back, the arrows walk the things
KnowYourSkyrim.belongingsKey = function(details)
{
   var code = details.navEquivalent;
   var state = KnowYourSkyrim.belongings;
   var at;
   var rows;
   if(state == undefined)
   {
      return false;
   }
   if(!Shared.GlobalFunc.IsKeyPressed(details))
   {
      return true;
   }
   if(code == gfx.ui.NavigationCode.ESCAPE || code == gfx.ui.NavigationCode.TAB)
   {
      KnowYourSkyrim.closeBelongings();
      gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
   }
   else if(code == gfx.ui.NavigationCode.UP || code == gfx.ui.NavigationCode.DOWN)
   {
      rows = state.told.rows;
      at = Math.max(0, Math.min(rows.length - 1, KnowYourSkyrim.belongingAt(state.selected) + (code == gfx.ui.NavigationCode.UP ? -1 : 1)));
      if(rows[at] != undefined)
      {
         state.first = at < state.first ? at : (at >= state.first + KnowYourSkyrim.belongingsClip.count ? at - KnowYourSkyrim.belongingsClip.count + 1 : state.first);
         KnowYourSkyrim.pickBelonging(rows[at]);
      }
   }
   return true;
};
