// A press on a row shows its details, a second press soon after looks at it from all sides
KnowYourSkyrim.pressBrowserRow = function(row)
{
   var now = getTimer();
   var id = row.item.id;
   var second = KnowYourSkyrim.browserPressed == id && now - KnowYourSkyrim.browserPressedAt < KnowYourSkyrim.DOUBLE_CLICK;
   KnowYourSkyrim.browserPressed = second ? undefined : id;
   KnowYourSkyrim.browserPressedAt = now;
   if(KnowYourSkyrim.browserTyping != undefined && KnowYourSkyrim.browserTyping != false)
   {
      KnowYourSkyrim.endBrowserTyping(false);
   }
   if(second)
   {
      KnowYourSkyrim.inspectBrowserItem();
      return undefined;
   }
   KnowYourSkyrim.goBrowserItem(id);
};

// What was looked at before stays in a chain, the arrow and the other mouse button walk it back one step at a time
KnowYourSkyrim.goBrowserItem = function(id)
{
   var state = KnowYourSkyrim.browserState;
   var history = KnowYourSkyrim.browserHistory;
   if(state.selected > 0 && state.selected != id)
   {
      history.push(state.selected);
      if(history.length > KnowYourSkyrim.BROWSER_HISTORY)
      {
         history.shift();
      }
   }
   KnowYourSkyrim.showBrowserItem(id);
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.browserBack = function()
{
   var history = KnowYourSkyrim.browserHistory;
   if(history == undefined || history.length == 0 || !KnowYourSkyrim.browsing)
   {
      return undefined;
   }
   KnowYourSkyrim.showBrowserItem(history.pop());
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// A link names an entry of the browser by its id, words to search for, a marker the map opens at, or a list of the
// page it opens or shuts. The map shows the world the player is in, a place of another world says so where the count
// of rows stands
KnowYourSkyrim.followBrowserLink = function(link)
{
   if(link.open != undefined || link.shut != undefined)
   {
      KnowYourSkyrim.openBrowserList(link.open != undefined ? link.open : link.shut, link.open != undefined);
   }
   else if(link.marker != undefined)
   {
      if(!KnowYourSkyrim.plugin().BrowserMap(link.marker))
      {
         KnowYourSkyrim.tellBrowser(KnowYourSkyrim.say("Not on this world's map"));
      }
      gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
   }
   else if(link.search != undefined)
   {
      KnowYourSkyrim.searchBrowser(link.search);
      gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
   }
   else if(link.pick != undefined)
   {
      KnowYourSkyrim.pickBrowserMod(link.pick);
   }
   else if(link.id != undefined && link.id > 0)
   {
      KnowYourSkyrim.goBrowserItem(link.id);
   }
};

// The details: a tile the model of the item stands in, beside it the name, its kind, the mod and the numbers, under
// them its effects, where it can be got and its keywords on a page of their own that scrolls. The arrow back sits on
// the corner of the tile
KnowYourSkyrim.buildBrowserDetails = function(browser, x, y, wide, high)
{
   var details = browser.createEmptyMovieClip("details", 6);
   var tile = KnowYourSkyrim.BROWSER_TILE;
   var inspect;
   var back;
   details._x = x;
   details._y = y;
   details.wide = wide;
   details.high = high;
   details.createEmptyMovieClip("tile", 0);
   details.createEmptyMovieClip("head", 1);
   details.createEmptyMovieClip("view", 2);
   details.view._y = tile + 10;
   details.view.beginFill(0, 100);
   KnowYourSkyrim.roundBox(details.view, 0, 0, wide, high - tile - 10, 0);
   details.view.endFill();
   details.createEmptyMovieClip("page", 3);
   details.page._y = details.view._y;
   details.page.setMask(details.view);
   inspect = details.createEmptyMovieClip("inspect", 4);
   inspect.createTextField("touch", 0, 0, 0, 10, 10);
   inspect.touch.selectable = false;
   inspect.createEmptyMovieClip("sign", 1);
   KnowYourSkyrim.writeLine(inspect, "label", 2, KnowYourSkyrim.say("Inspect").toUpperCase(), KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.PARCHMENT);
   inspect.label._x = 20;
   inspect.wide = inspect.label._x + inspect.label._width + 4;
   inspect.touch._width = inspect.wide;
   inspect.touch._height = KnowYourSkyrim.BROWSER_INSPECT_HIGH;
   inspect.label._y = Math.round((KnowYourSkyrim.BROWSER_INSPECT_HIGH - inspect.label._height) / 2);
   inspect._x = tile + 10;
   inspect._y = tile - KnowYourSkyrim.BROWSER_INSPECT_HIGH;
   inspect.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawBrowserInspect();
   };
   inspect.onRollOut = inspect.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawBrowserInspect();
   };
   inspect.onPress = function()
   {
      KnowYourSkyrim.inspectBrowserItem();
   };
   KnowYourSkyrim.drawBrowserInspect();
   back = details.createEmptyMovieClip("back", 5);
   back.createTextField("touch", 0, 0, 0, KnowYourSkyrim.BROWSER_BACK, KnowYourSkyrim.BROWSER_BACK);
   back.touch.selectable = false;
   back.createEmptyMovieClip("look", 1);
   back._x = back._y = 4;
   back.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawBrowserBackButton();
   };
   back.onRollOut = back.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawBrowserBackButton();
   };
   back.onPress = function()
   {
      KnowYourSkyrim.browserBack();
   };
   details.tile.lineStyle(1, KnowYourSkyrim.WHITE, 30, true);
   details.tile.beginFill(0, 55);
   KnowYourSkyrim.roundBox(details.tile, 0, 0, tile, tile, 5);
   details.tile.endFill();
   details.tile.lineStyle();
   details.createEmptyMovieClip("sign", 6);
   KnowYourSkyrim.buildCheats(details);
   KnowYourSkyrim.buildBrowserStar(details);
   KnowYourSkyrim.buildBrowserFind(details);
   // the things one of a living entry has on them now, looked through in a list where SkyUI's list stands
   KnowYourSkyrim.cheatButton(details, "things", 9, KnowYourSkyrim.say("Inventory"), "sack");
   details.things._visible = false;
   details.things.onPress = function()
   {
      KnowYourSkyrim.endBrowserTyping(false);
      KnowYourSkyrim.openBelongings(KnowYourSkyrim.browserItem.id);
   };
   KnowYourSkyrim.drawBrowserBackButton();
};

// The star in the top right corner of the details puts the entry among the favorites or takes it out. It stays clear of
// the tile, the model there is drawn over the menu
KnowYourSkyrim.buildBrowserStar = function(details)
{
   var star = details.createEmptyMovieClip("star", 8);
   var size = KnowYourSkyrim.BROWSER_STAR;
   star._x = details.wide - size;
   star._visible = false;
   star.createTextField("touch", 0, 0, 0, size, size);
   star.touch.selectable = false;
   star.createEmptyMovieClip("sign", 1);
   star.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawBrowserStar();
   };
   star.onRollOut = star.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawBrowserStar();
   };
   star.onPress = function()
   {
      KnowYourSkyrim.toggleFavorite();
   };
};

KnowYourSkyrim.drawBrowserStar = function()
{
   var star = KnowYourSkyrim.browser.details.star;
   var told = KnowYourSkyrim.browserItem;
   var on = told != undefined && told.favorite == true;
   star._visible = told != undefined;
   star.sign.clear();
   KnowYourSkyrim.drawIcon(star.sign, "star", 1, 1, KnowYourSkyrim.BROWSER_STAR - 2, on || star.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, on ? 100 : (star.lit ? 70 : 30));
};

// The counts of the tabs follow, a row taken out of the favorites leaves their list at once
KnowYourSkyrim.toggleFavorite = function()
{
   var told = KnowYourSkyrim.browserItem;
   if(told == undefined)
   {
      return undefined;
   }
   told.favorite = KnowYourSkyrim.plugin().BrowserFavorite(told.id, told.favorite != true) == true;
   KnowYourSkyrim.drawBrowserStar();
   KnowYourSkyrim.findBrowser(KnowYourSkyrim.browserState.first, true);
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.drawBrowserInspect = function()
{
   var inspect = KnowYourSkyrim.browser.details.inspect;
   inspect.sign.clear();
   KnowYourSkyrim.drawIcon(inspect.sign, "lens", 0, 2, 14, inspect.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 100);
   inspect.label.textColor = inspect.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT;
};

KnowYourSkyrim.drawBrowserBackButton = function()
{
   var back = KnowYourSkyrim.browser.details.back;
   var size = KnowYourSkyrim.BROWSER_BACK;
   var history = KnowYourSkyrim.browserHistory;
   back._visible = history != undefined && history.length > 0 && KnowYourSkyrim.browserItem != undefined;
   back.look.clear();
   back.look.lineStyle(1, KnowYourSkyrim.GOLD, back.lit ? 100 : 70, true);
   back.look.beginFill(0, back.lit ? 80 : 60);
   KnowYourSkyrim.roundBox(back.look, 0, 0, size, size, 4);
   back.look.endFill();
   back.look.lineStyle();
   KnowYourSkyrim.drawIcon(back.look, "unequip", 3, 3, size - 6, back.lit ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 100);
};

// The details of an entry, the model of an item in the tile and the sign of a living thing, and a frame later where it
// can be got or what it carries. The first ask makes the plugin read who holds what, the details are up before that
KnowYourSkyrim.showBrowserItem = function(id)
{
   var details = KnowYourSkyrim.browser.details;
   var plugin = KnowYourSkyrim.plugin();
   var told = id > 0 ? plugin.BrowserItem(id) : undefined;
   var tile = KnowYourSkyrim.tileOf(told);
   var modelled;
   KnowYourSkyrim.drawTileBox(details, tile);
   details.createEmptyMovieClip("head", 1);
   details.page.createEmptyMovieClip("lines", 0);
   details.sign.clear();
   details.inspect._visible = told != undefined && told.living != true && !KnowYourSkyrim.signOnly(told);
   details.things._visible = told != undefined && told.living == true && told.about > 0;
   details.tile._visible = told != undefined;
   details.find._visible = told != undefined;
   KnowYourSkyrim.showCheats(told);
   KnowYourSkyrim.browserItem = told;
   KnowYourSkyrim.browserSources = undefined;
   KnowYourSkyrim.browserOpen = {};
   KnowYourSkyrim.browserLinkLit = undefined;
   KnowYourSkyrim.drawBrowserBackButton();
   KnowYourSkyrim.drawBrowserStar();
   if(told == undefined)
   {
      KnowYourSkyrim.browserState.selected = 0;
      plugin.ShowTile();
      KnowYourSkyrim.refreshBrowserRows();
      KnowYourSkyrim.padHints();
      return undefined;
   }
   KnowYourSkyrim.browserState.selected = id;
   modelled = KnowYourSkyrim.showTile(id, told);
   // a small tile keeps its sign clear of the back button in its corner
   if((told.living == true || KnowYourSkyrim.signOnly(told)) && !modelled)
   {
      if(tile < KnowYourSkyrim.BROWSER_TILE)
      {
         KnowYourSkyrim.drawIcon(details.sign, told.icon, tile * 0.42, tile * 0.36, tile * 0.5, KnowYourSkyrim.PARCHMENT, 70);
      }
      else
      {
         KnowYourSkyrim.drawIcon(details.sign, told.icon, tile * 0.2, tile * 0.2, tile * 0.6, KnowYourSkyrim.PARCHMENT, 70);
      }
   }
   KnowYourSkyrim.drawBrowserHead(told);
   KnowYourSkyrim.drawBrowserPage();
   KnowYourSkyrim.askBrowserSources(id, 1);
   KnowYourSkyrim.refreshBrowserRows();
};

// A person shows the face, a creature its body, a quest and anything with no model its sign. The tile is measured on the
// stage, a room stretched by a skin's stage keeps the model square. True when a model shows
KnowYourSkyrim.showTile = function(id, told)
{
   var details = KnowYourSkyrim.browser.details;
   var plugin = KnowYourSkyrim.plugin();
   var tile = KnowYourSkyrim.BROWSER_TILE;
   var inset = KnowYourSkyrim.MODEL_INSET;
   var top;
   var bottom;
   if(told == undefined || KnowYourSkyrim.signOnly(told))
   {
      plugin.ShowTile();
      return false;
   }
   top = {x:tile / 2, y:inset};
   bottom = {x:tile / 2, y:tile - inset};
   details.localToGlobal(top);
   details.localToGlobal(bottom);
   top = KnowYourSkyrim.stageShare(top.x, top.y);
   bottom = KnowYourSkyrim.stageShare(bottom.x, bottom.y);
   return plugin.ShowTile(id, top.x, (top.y + bottom.y) / 2, bottom.y - top.y) == true;
};

// Where it is got or met is asked a frame after the details are up, the first ask reads who holds what. While the
// plugin files are still read on their thread the page asks again every little while, the places come in then
KnowYourSkyrim.askBrowserSources = function(id, wait)
{
   KnowYourSkyrim.browser.sourcesWait = wait;
   KnowYourSkyrim.browser.onEnterFrame = function()
   {
      if(--this.sourcesWait > 0)
      {
         return undefined;
      }
      delete this.onEnterFrame;
      if(KnowYourSkyrim.browserItem == undefined || KnowYourSkyrim.browserItem.id != id || !KnowYourSkyrim.browsing)
      {
         return undefined;
      }
      KnowYourSkyrim.browserSources = KnowYourSkyrim.plugin().BrowserSources(id, KnowYourSkyrim.anyListOpen());
      KnowYourSkyrim.drawBrowserPage(true);
      // the first journal stage of a quest is known once the quests' files are read, START may come with it
      if(KnowYourSkyrim.browserSources.beginsAt != KnowYourSkyrim.browserItem.beginsAt)
      {
         KnowYourSkyrim.browserItem.beginsAt = KnowYourSkyrim.browserSources.beginsAt;
         KnowYourSkyrim.showCheats(KnowYourSkyrim.browserItem);
         KnowYourSkyrim.placeButtonLine();
      }
      if(KnowYourSkyrim.browserSources.reading == true)
      {
         KnowYourSkyrim.askBrowserSources(id, KnowYourSkyrim.BROWSER_READ_WAIT);
      }
   };
};

KnowYourSkyrim.refreshBrowserRows = function()
{
   var rows = KnowYourSkyrim.browser.rows;
   var i = 0;
   while(i < rows.count)
   {
      KnowYourSkyrim.drawBrowserRow(rows["row" + i]);
      i++;
   }
};

KnowYourSkyrim.drawBrowserHead = function(told)
{
   var details = KnowYourSkyrim.browser.details;
   var head = details.head;
   var x = KnowYourSkyrim.tileOf(told) + 10;
   var wide = details.wide - x;
   var y = 0;
   var lines = KnowYourSkyrim.browserHeadLines(told);
   var field;
   var signed;
   var i = 0;
   head.next = 0;
   head.linkFields = [];
   head.fields = [];
   head.createEmptyMovieClip("signs", 1000);
   // the name leaves the corner to the star
   field = KnowYourSkyrim.heroWrapped(head, told.name, x, y, wide - KnowYourSkyrim.BROWSER_STAR - 6, 16, KnowYourSkyrim.GOLD);
   head.fields.push(field);
   y += field.textHeight + 6;
   // every line goes in, the mod and the form id come last
   while(i < lines.length)
   {
      // a line with a sign has it before its words
      signed = lines[i].sign != undefined ? KnowYourSkyrim.BROWSER_MARK + 5 : 0;
      field = KnowYourSkyrim.richLine(head, lines[i].pieces, x + signed, y, wide - signed, lines[i].size, lines[i].color);
      head.fields.push(field);
      if(signed > 0)
      {
         KnowYourSkyrim.drawIcon(head.signs, lines[i].sign, x, y + (field.textHeight - KnowYourSkyrim.BROWSER_MARK) / 2 + 2, KnowYourSkyrim.BROWSER_MARK, lines[i].color, lines[i].alpha);
      }
      y += field.textHeight + 3;
      i++;
   }
   details.headEnd = y;
   KnowYourSkyrim.placeButtonLine();
};

// INSPECT and the cheats stand on one line at the foot of the tile, or under the head when the head runs past it, and
// a head that wraps past the tile pushes the page down instead of lying over its first section. The find field takes
// a line of its own across the whole card under the tile and the buttons
KnowYourSkyrim.placeButtonLine = function()
{
   var details = KnowYourSkyrim.browser.details;
   var high = KnowYourSkyrim.BROWSER_INSPECT_HIGH;
   var tile = KnowYourSkyrim.tileOf(KnowYourSkyrim.browserItem);
   var line = Math.max(tile - high, Math.ceil(details.headEnd) + 2);
   var y = Math.max(tile, Math.ceil(details.headEnd));
   var x = tile + 10;
   details.inspect._x = x;
   details.inspect._y = details.cheats._y = details.things._y = line;
   // INSPECT of an item and INVENTORY of a living thing never show together, each takes the start of the line
   details.things._x = x;
   if(details.inspect._visible || details.things._visible)
   {
      x += (details.inspect._visible ? details.inspect.wide : details.things.wide) + KnowYourSkyrim.CHEAT_GAP;
   }
   details.cheats._x = x;
   if(details.inspect._visible || details.things._visible || details.cheats._visible)
   {
      y = Math.max(y, line + high);
   }
   if(details.find._visible)
   {
      KnowYourSkyrim.sizeFindBox(details.find, details.wide);
      details.find._x = 0;
      details.find._y = y + 6;
      y = details.find._y + high;
   }
   KnowYourSkyrim.placeBrowserPage(Math.max(tile + 10, Math.ceil(y) + 6));
};

// a word of the browser where the count of rows stands, until the next search
KnowYourSkyrim.tellBrowser = function(words)
{
   var count = KnowYourSkyrim.browser.count;
   count.text = words;
   count._x = KnowYourSkyrim.browser.wide - KnowYourSkyrim.BROWSER_PAD - count._width;
};

KnowYourSkyrim.placeBrowserPage = function(top)
{
   var details = KnowYourSkyrim.browser.details;
   details.view._y = top;
   details.view._height = Math.max(1, details.high - top);
   details.page._y = top;
};

KnowYourSkyrim.drawBrowserPage = function(keepPlace)
{
   var details = KnowYourSkyrim.browser.details;
   var told = KnowYourSkyrim.browserItem;
   var page = details.page;
   var lines = details.page.createEmptyMovieClip("lines", 0);
   var wide = details.wide - 8;
   var pad = KnowYourSkyrim.PANEL_PAD;
   var y = 0;
   var top;
   var sections;
   var s;
   var i;
   var field;
   if(told == undefined)
   {
      return undefined;
   }
   sections = KnowYourSkyrim.pageSections(told, KnowYourSkyrim.browserSources);
   if(KnowYourSkyrim.browserOpen.edits == true)
   {
      sections.unshift({head:KnowYourSkyrim.say("Changed by"), lines:[[told.edits.join(", ")].concat(KnowYourSkyrim.cutTail(KnowYourSkyrim.editsCut(told), "   ", ""))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   lines.next = 0;
   lines.linkFields = [];
   lines.fields = [];
   lines.heads = [];
   KnowYourSkyrim.browserLinkLit = undefined;
   // each section a head and a panel with its lines (33-journal.as)
   s = 0;
   while(s < sections.length)
   {
      lines.heads.push({head:sections[s].head, y:y});
      y += KnowYourSkyrim.drawSectionHead(lines, sections[s], y, wide);
      top = y;
      y += 6;
      if(KnowYourSkyrim.isSection(sections[s], "Quest chain") && KnowYourSkyrim.browserSources != undefined)
      {
         y += KnowYourSkyrim.drawChainTree(lines, sections[s], pad, y, wide - 2 * pad);
      }
      else
      {
         i = 0;
         while(i < sections[s].lines.length)
         {
            if(sections[s].lines[i].quests != undefined)
            {
               y += KnowYourSkyrim.drawQuestGroup(lines, sections[s], sections[s].lines[i], pad, y, wide - 2 * pad);
            }
            else
            {
               field = KnowYourSkyrim.drawSectionLine(lines, sections[s], sections[s].lines[i], pad, y, wide - 2 * pad);
               lines.fields.push(field);
               y += field.textHeight + 4;
            }
            i++;
         }
      }
      y += 2;
      KnowYourSkyrim.drawSectionPanel(lines, top, y, wide);
      y += KnowYourSkyrim.SECTION_GAP;
      s++;
   }
   page.full = y;
   // a list opened or shut keeps the page where it was, the press was on it
   page.first = keepPlace == true ? page.first : 0;
   KnowYourSkyrim.scrollBrowserPage(0);
   KnowYourSkyrim.markFound(false);
   // what a gamepad's buttons do follows the entry and its links
   KnowYourSkyrim.padHints();
};

// A wrapped line of pieces. The links are gold and kept with the letters they cover, so a press on the field finds
// its link by the letter under the pointer
KnowYourSkyrim.richLine = function(holder, pieces, x, y, wide, size, color)
{
   var field = KnowYourSkyrim.heroWrapped(holder, KnowYourSkyrim.lineText(pieces), x, y, wide, size, color);
   var links = [];
   var at = 0;
   var i = 0;
   var piece;
   var format;
   while(KnowYourSkyrim.isPieces(pieces) && i < pieces.length)
   {
      piece = pieces[i];
      if(typeof(piece) == "object")
      {
         links.push({from:at, to:at + piece.text.length, link:piece});
         at += piece.text.length;
      }
      else
      {
         at += String(piece).length;
      }
      i++;
   }
   field.links = links;
   if(links.length > 0)
   {
      format = new TextFormat();
      format.color = KnowYourSkyrim.GOLD;
      i = 0;
      while(i < links.length)
      {
         field.setTextFormat(links[i].from, links[i].to, format);
         i++;
      }
      holder.linkFields.push(field);
   }
   return field;
};

// The link under the pointer, in the head beside the tile or on the page, as {field, from, to, link}
KnowYourSkyrim.browserLinkUnderMouse = function()
{
   var details = KnowYourSkyrim.browser.details;
   var holders = [details.head, details.page.lines];
   var h = 0;
   var f;
   var field;
   var point;
   var index;
   var l;
   if(!KnowYourSkyrim.browserLaid || !KnowYourSkyrim.browser._visible || details.page.lines == undefined)
   {
      return undefined;
   }
   if(KnowYourSkyrim.browser.modList._visible && KnowYourSkyrim.inBrowserClip(KnowYourSkyrim.browser.modList))
   {
      return undefined;
   }
   while(h < holders.length)
   {
      // the page shows through its mask only, a line scrolled out of it isn't there to press
      if(h == 1 && !KnowYourSkyrim.inBrowserClip(details.view))
      {
         h++;
         continue;
      }
      f = 0;
      while(holders[h].linkFields != undefined && f < holders[h].linkFields.length)
      {
         field = holders[h].linkFields[f];
         f++;
         point = {x:_root._xmouse, y:_root._ymouse};
         field._parent.globalToLocal(point);
         if(point.x < field._x || point.x > field._x + field._width || point.y < field._y || point.y > field._y + field._height)
         {
            continue;
         }
         index = field.getCharIndexAtPoint(point.x - field._x, point.y - field._y);
         l = 0;
         while(index >= 0 && l < field.links.length)
         {
            if(index >= field.links[l].from && index < field.links[l].to)
            {
               return {field:field, from:field.links[l].from, to:field.links[l].to, link:field.links[l].link};
            }
            l++;
         }
      }
      h++;
   }
   return undefined;
};

// the link under the pointer is underlined, the one before loses it
KnowYourSkyrim.lightBrowserLink = function(found)
{
   var was = KnowYourSkyrim.browserLinkLit;
   var format;
   if(was != undefined && found != undefined && was.field == found.field && was.from == found.from)
   {
      return undefined;
   }
   if(was != undefined && was.field.text != undefined)
   {
      format = new TextFormat();
      format.underline = false;
      was.field.setTextFormat(was.from, was.to, format);
   }
   KnowYourSkyrim.browserLinkLit = found;
   if(found != undefined)
   {
      format = new TextFormat();
      format.underline = true;
      found.field.setTextFormat(found.from, found.to, format);
   }
};

KnowYourSkyrim.scrollBrowserPage = function(by)
{
   var details = KnowYourSkyrim.browser.details;
   var page = details.page;
   var room = details.view._height;
   var most;
   if(page.full == undefined)
   {
      return undefined;
   }
   most = Math.max(0, Math.ceil((page.full - room) / KnowYourSkyrim.BROWSER_PAGE_STEP));
   page.first = Math.max(0, Math.min(most, page.first + by));
   page.lines._y = -Math.min(page.first * KnowYourSkyrim.BROWSER_PAGE_STEP, Math.max(0, page.full - room));
};
