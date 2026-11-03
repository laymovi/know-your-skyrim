// The item browser: every item and every person or creature of the load order by category and search, the mod each
// comes from, and where an item can be got. A button in the bottom bar opens and shuts it. The plugin keeps the index,
// searches it and hands over one page of rows at a time, so a load order of tens of thousands costs the menu no more
// than a page. Nothing is read before the first opening. With Inventory Reforged the browser stands in the panel's
// room, the panel makes way for it and its windows lie over it (39-browser-room.as of the panel holds the other half of
// this). Alone it takes the room right of the list where SkyUI keeps its item card
KnowYourSkyrim.start = function(menu)
{
   var plugin = KnowYourSkyrim.plugin();
   var told = plugin != undefined && plugin.BrowserKept != undefined ? plugin.BrowserKept() : undefined;
   if(KnowYourSkyrim.menu != undefined || told == undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.menu = menu;
   KnowYourSkyrim.browsing = false;
   // paper skins get dark ink, a stage 2560 wide gets the drawing squeezed back, the same reading as the panel's
   KnowYourSkyrim.readSkin(menu._config);
   KnowYourSkyrim.readSquash();
   // the text fields of the details tell which letter lies under the pointer, a link is found by it
   _global.gfxExtensions = true;
   KnowYourSkyrim.browserState = {open:told.open == true, words:String(told.words), family:String(told.family), chips:KnowYourSkyrim.keptChips(String(told.family), String(told.chips)), skill:KnowYourSkyrim.keptSkill(String(told.chips)), first:KnowYourSkyrim.number(told.first), selected:KnowYourSkyrim.number(told.selected), mod:told.mod != undefined ? String(told.mod) : "", within:KnowYourSkyrim.keptWithin(told.within)};
   KnowYourSkyrim.browserHistory = KnowYourSkyrim.keptIds(told.history);
   // the lists of the page opened and how far it was scrolled come back once, with the item they were left on
   KnowYourSkyrim.keptPage = {lists:told.lists != undefined && told.lists.length > 0 ? String(told.lists).split(",") : [], first:KnowYourSkyrim.number(told.page)};
   KnowYourSkyrim.buildBrowseButton();
   KnowYourSkyrim.listenBrowser();
   KnowYourSkyrim.buildInspect();
   KnowYourSkyrim.watchCard();
   // the last over the game's calls for the selected row, because RB of a gamepad charges and the card mustn't ask for
   // its room while the browser holds the input
   KnowYourSkyrim.buildPad();
   // a panel laid out already is met now, one still on its way meets the browser itself
   if(_global.Equipment != undefined && _global.Equipment.browserRoomReady)
   {
      KnowYourSkyrim.meetPanel(_global.Equipment);
   }
   // On the next frame SkyUI has moved the numbers of the player, the button finds its place then. A browser left open
   // opens again, once the panel on its way meets it, or alone when no panel comes in a while
   KnowYourSkyrim.browseButton.waited = 0;
   KnowYourSkyrim.browseButton.onEnterFrame = function()
   {
      if(this.waited == 0)
      {
         KnowYourSkyrim.placeBrowseButton();
      }
      this.waited++;
      if(KnowYourSkyrim.panelComing())
      {
         if(this.waited <= KnowYourSkyrim.PANEL_WAIT)
         {
            return undefined;
         }
         KnowYourSkyrim.panelLate = true;
      }
      delete this.onEnterFrame;
      if(KnowYourSkyrim.browserState.open && !KnowYourSkyrim.browsing)
      {
         KnowYourSkyrim.openBrowser();
      }
   };
};

// The panel of Inventory Reforged, laid out. A browser already laid out alone moves into the panel's room
KnowYourSkyrim.meetPanel = function(panel)
{
   var open;
   if(KnowYourSkyrim.panel == panel || KnowYourSkyrim.menu == undefined)
   {
      return undefined;
   }
   open = KnowYourSkyrim.browsing;
   if(open)
   {
      KnowYourSkyrim.hideBrowser();
   }
   KnowYourSkyrim.panel = panel;
   KnowYourSkyrim.padMeetPanel(panel);
   if(KnowYourSkyrim.browser != undefined)
   {
      KnowYourSkyrim.browser.removeMovieClip();
      KnowYourSkyrim.browser = undefined;
      KnowYourSkyrim.browserLaid = false;
   }
   KnowYourSkyrim.drawBrowseButton();
   if(open || KnowYourSkyrim.browserState.open)
   {
      KnowYourSkyrim.openBrowser();
   }
};

// The panel's movie is loading into the menu and hasn't met the browser yet. A panel from before the browser was a mod
// of its own has no room for it and never comes
KnowYourSkyrim.panelComing = function()
{
   var panel = _global.Equipment;
   return KnowYourSkyrim.panel == undefined && _root.equipmentPanelScript != undefined && !KnowYourSkyrim.panelLate && (panel == undefined || panel.buildBrowserRoom != undefined);
};

// The panel's room moved, the list got narrower or wider. The browser keeps its width and stands at the room's right
KnowYourSkyrim.roomChanged = function()
{
   var room = KnowYourSkyrim.room();
   if(KnowYourSkyrim.browserLaid && room != undefined)
   {
      KnowYourSkyrim.browser._x = room.right - KnowYourSkyrim.browser.wide;
      KnowYourSkyrim.tellPointerArea();
   }
   KnowYourSkyrim.placeBrowseButton();
};

// the clip the browser draws in, under the panel's windows or a layer of its own in the menu
KnowYourSkyrim.roomClip = function()
{
   return KnowYourSkyrim.panel != undefined ? KnowYourSkyrim.panel.browserRoom : KnowYourSkyrim.aloneLayer();
};

// the room of the browser, {left, top, right, bottom} in the units of roomClip
KnowYourSkyrim.room = function()
{
   return KnowYourSkyrim.panel != undefined ? KnowYourSkyrim.panel.browserFrame() : KnowYourSkyrim.aloneRoom();
};

// Alone the browser draws in a layer of the menu made the first time it's needed. A skin on SkyUI 5 stretches a
// stage 2560 wide over the screen, the layer is stretched back by squash so the browser keeps its proportions. It
// stays 1 until the plugin can tell the size of the screen (the same work the panel does for Dear Diary)
KnowYourSkyrim.squash = 1;

KnowYourSkyrim.aloneLayer = function()
{
   var menu = KnowYourSkyrim.menu;
   if(menu.knowYourSkyrimLayer == undefined)
   {
      menu.createEmptyMovieClip("knowYourSkyrimLayer", menu.getNextHighestDepth());
      menu.knowYourSkyrimLayer._xscale = 100 / KnowYourSkyrim.squash;
   }
   return menu.knowYourSkyrimLayer;
};

// From the frame of the list to the right edge of the screen, from the top down to the bottom bar, in the units of
// the layer
KnowYourSkyrim.aloneRoom = function()
{
   var menu = KnowYourSkyrim.menu;
   var lists = menu.inventoryLists;
   var bounds = lists.getContentBounds != undefined ? lists.getContentBounds() : undefined;
   var screen = Stage.visibleRect;
   var margin = KnowYourSkyrim.ROOM_MARGIN;
   // the edge of the list and the bar in the units of the menu, the screen's edges on the stage
   var edge = {x:bounds != undefined ? lists._x + bounds[0] + bounds[2] + KnowYourSkyrim.LIST_FRAME : 0, y:menu.bottomBar._y};
   var from;
   var to;
   var layer = KnowYourSkyrim.aloneLayer();
   menu.localToGlobal(edge);
   from = {x:bounds != undefined ? edge.x : screen.x + screen.width / 2, y:screen.y + Stage.safeRect.y};
   to = {x:screen.x + screen.width - Stage.safeRect.x, y:edge.y};
   layer.globalToLocal(from);
   layer.globalToLocal(to);
   return {left:from.x + margin / 2, top:from.y + margin, right:to.x - margin, bottom:to.y - margin};
};

// the chips the player left, each in the place of its group among the chips of the category
KnowYourSkyrim.keptChips = function(family, joined)
{
   var chips = [];
   var keys = joined.length > 0 ? joined.split(",") : [];
   var rows = KnowYourSkyrim.CHIP_ROWS[family];
   var g;
   var c;
   var k = 0;
   while(rows != undefined && k < keys.length)
   {
      g = 0;
      while(g < rows.length)
      {
         c = 0;
         while(c < rows[g].length)
         {
            if(rows[g][c].key == keys[k])
            {
               chips[g] = keys[k];
            }
            c++;
         }
         g++;
      }
      k++;
   }
   return chips;
};

// the skill of the trainers kept among the chips, none when no chip names one
KnowYourSkyrim.keptSkill = function(joined)
{
   var keys = joined.length > 0 ? joined.split(",") : [];
   var i = 0;
   while(i < keys.length)
   {
      if(keys[i].indexOf(KnowYourSkyrim.SKILL_CHIP) == 0)
      {
         return Number(keys[i].substr(KnowYourSkyrim.SKILL_CHIP.length));
      }
      i++;
   }
   return undefined;
};

// the chips chosen now, without the groups that have none, the skill of the trainers after them while Trainers is
// chosen
KnowYourSkyrim.chosenBrowserChips = function()
{
   var chosen = [];
   var state = KnowYourSkyrim.browserState;
   var chips = state.chips;
   var i = 0;
   while(i < chips.length)
   {
      if(chips[i] != undefined)
      {
         chosen.push(chips[i]);
      }
      i++;
   }
   if(state.skill != undefined && KnowYourSkyrim.has(chosen, "trainers"))
   {
      chosen.push(KnowYourSkyrim.SKILL_CHIP + state.skill);
   }
   return chosen;
};

// the skill goes with Trainers, a chip that lets Trainers go lets it go too
KnowYourSkyrim.keepSkillWithTrainers = function()
{
   var state = KnowYourSkyrim.browserState;
   if(!KnowYourSkyrim.has(state.chips, "trainers"))
   {
      state.skill = undefined;
   }
};

// The lists switch themselves off while the card asks something, a search is typed or an item is inspected, by the
// browser or by the panel. The browser does nothing for that time
KnowYourSkyrim.listsAnswer = function()
{
   var lists = KnowYourSkyrim.menu.inventoryLists;
   return KnowYourSkyrim.menu.bFadedIn && !lists.itemList.disableInput && !lists.itemList.disableSelection && !lists.categoryList.disableInput && !KnowYourSkyrim.anyInspect();
};

KnowYourSkyrim.anyInspect = function()
{
   return KnowYourSkyrim.inspecting == true || KnowYourSkyrim.panel != undefined && KnowYourSkyrim.panel.inspecting == true;
};

KnowYourSkyrim.toggleBrowser = function()
{
   if(KnowYourSkyrim.browsing)
   {
      KnowYourSkyrim.closeBrowser();
   }
   else if(KnowYourSkyrim.listsAnswer())
   {
      KnowYourSkyrim.openBrowser();
   }
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// With the panel the doll, the slots, their models and the drawer go and the browser stands where they stood. Alone
// the item card and the game's model of the selected item go, the browser stands where they stood
KnowYourSkyrim.openBrowser = function()
{
   var state = KnowYourSkyrim.browserState;
   KnowYourSkyrim.browsing = true;
   state.open = true;
   if(KnowYourSkyrim.panel != undefined)
   {
      KnowYourSkyrim.panel.browserShown(true);
   }
   else
   {
      KnowYourSkyrim.menu.itemCard._parent._visible = false;
      KnowYourSkyrim.plugin().ShowPreview(false);
      KnowYourSkyrim.keepTurnAreaOff(true);
   }
   if(!KnowYourSkyrim.browserLaid)
   {
      KnowYourSkyrim.layBrowser();
   }
   KnowYourSkyrim.browser._visible = true;
   KnowYourSkyrim.tellPointerArea();
   KnowYourSkyrim.findBrowser(state.first, true);
   KnowYourSkyrim.showBrowserItem(state.selected);
   KnowYourSkyrim.restorePage();
   KnowYourSkyrim.drawBrowseButton();
   KnowYourSkyrim.keepBrowser();
};

// The page opens its lists again and scrolls back where it was once the places of the item come in
KnowYourSkyrim.restorePage = function()
{
   var kept = KnowYourSkyrim.keptPage;
   var i = 0;
   KnowYourSkyrim.keptPage = undefined;
   if(kept == undefined || KnowYourSkyrim.browserItem == undefined)
   {
      return undefined;
   }
   while(i < kept.lists.length)
   {
      KnowYourSkyrim.browserOpen[kept.lists[i]] = true;
      i++;
   }
   KnowYourSkyrim.browser.details.page.first = kept.first;
};

// the ids of a kept chain joined by commas
KnowYourSkyrim.keptIds = function(joined)
{
   var ids = [];
   var parts = joined != undefined && joined.length > 0 ? String(joined).split(",") : [];
   var i = 0;
   while(i < parts.length)
   {
      ids.push(Number(parts[i]));
      i++;
   }
   return ids;
};

// Only the button shuts the browser. Anything else it makes way for leaves it open, the next inventory opens with it
// as it was left
KnowYourSkyrim.closeBrowser = function()
{
   KnowYourSkyrim.hideBrowser();
   KnowYourSkyrim.browserState.open = false;
   KnowYourSkyrim.drawBrowseButton();
   KnowYourSkyrim.keepBrowser();
};

// What stood there comes back as it was, the panel makes its models and the doll again
KnowYourSkyrim.hideBrowser = function()
{
   var plugin = KnowYourSkyrim.plugin();
   KnowYourSkyrim.endBrowserTyping(false);
   KnowYourSkyrim.closeContext();
   KnowYourSkyrim.closeBelongings();
   KnowYourSkyrim.browsing = false;
   KnowYourSkyrim.browser._visible = false;
   KnowYourSkyrim.tellPointerArea();
   KnowYourSkyrim.browser.modList._visible = false;
   plugin.ShowTile();
   plugin.Cover();
   if(KnowYourSkyrim.panel != undefined)
   {
      KnowYourSkyrim.panel.browserShown(false);
   }
   else
   {
      KnowYourSkyrim.menu.itemCard._parent._visible = true;
      plugin.ShowPreview(true);
      KnowYourSkyrim.keepTurnAreaOff(false);
   }
   // the last, the bottom bar the list gets back says the browser is shut
   KnowYourSkyrim.leavePad(false);
};

// The plugin gives the mouse over the browser to this movie ahead of other mods' hooks on the menu, the whole screen
// while the view of an item is up
KnowYourSkyrim.tellPointerArea = function()
{
   var plugin = KnowYourSkyrim.plugin();
   var box;
   if(plugin.BrowserArea == undefined)
   {
      return undefined;
   }
   if(KnowYourSkyrim.inspecting == true)
   {
      plugin.BrowserArea(0, 0, 1, 1);
   }
   else if(KnowYourSkyrim.browsing && KnowYourSkyrim.browser != undefined)
   {
      box = KnowYourSkyrim.shareBox(KnowYourSkyrim.browser);
      plugin.BrowserArea(box.from.x, box.from.y, box.to.x, box.to.y);
   }
   else
   {
      plugin.BrowserArea(0, 0, 0, 0);
   }
};

// Alone the browser lies over the area of SkyUI the game's model of the selected row is turned and zoomed in. SkyUI
// asks that area by where the pointer is, whatever lies over it, so the wheel over the browser would zoom the hidden
// model and fade the menu out, and a press there could start the game turning it. While the browser is open the area
// takes nothing, the way the panel of Inventory Reforged keeps it off, and it comes back as it was
KnowYourSkyrim.keepTurnAreaOff = function(off)
{
   var area = KnowYourSkyrim.menu.mouseRotationRect;
   if(area == undefined || KnowYourSkyrim.panel != undefined)
   {
      return undefined;
   }
   if(off && area.knowYourSkyrimScale == undefined)
   {
      area.knowYourSkyrimScale = {x:area._xscale, y:area._yscale};
      area._xscale = area._yscale = 0;
   }
   else if(!off && area.knowYourSkyrimScale != undefined)
   {
      area._xscale = area.knowYourSkyrimScale.x;
      area._yscale = area.knowYourSkyrimScale.y;
      area.knowYourSkyrimScale = undefined;
   }
};

// Alone the browser lies over the item card. A card that asks something, how many to drop or which soul gem to charge
// with, needs its place back for a while
KnowYourSkyrim.watchCard = function()
{
   var menu = KnowYourSkyrim.menu;
   var card = menu.itemCard;
   if(card != undefined && card.ShowQuantityMenu != undefined)
   {
      card.knowYourSkyrimQuantity = card.ShowQuantityMenu;
      card.ShowQuantityMenu = function(count)
      {
         KnowYourSkyrim.makeWayForCard();
         this.knowYourSkyrimQuantity(count);
      };
   }
   if(menu.AttemptChargeItem != undefined)
   {
      menu.knowYourSkyrimCharge = menu.AttemptChargeItem;
      menu.AttemptChargeItem = function()
      {
         KnowYourSkyrim.makeWayForCard();
         this.knowYourSkyrimCharge();
      };
   }
   // The menu fades out for the game's zoom of the selected item and as it closes. The browser fades with it, only the
   // models change places. The tile and the inspect view would stay over the world, and alone the zoom shows the game's
   // model of the item. Back from the zoom the tile comes again
   if(menu.ToggleMenuFade != undefined)
   {
      menu.knowYourSkyrimFade = menu.ToggleMenuFade;
      menu.ToggleMenuFade = function()
      {
         var fading = this.bFadedIn;
         if(fading)
         {
            KnowYourSkyrim.leaveInspect();
         }
         KnowYourSkyrim.fadeModels(fading);
         this.knowYourSkyrimFade();
      };
   }
   // the fade that closes the menu shows no model of the game's, the browser alone keeps it out to the end
   if(menu.startMenuFade != undefined)
   {
      menu.knowYourSkyrimClosing = menu.startMenuFade;
      menu.startMenuFade = function()
      {
         KnowYourSkyrim.menuClosing = true;
         this.knowYourSkyrimClosing();
      };
   }
};

KnowYourSkyrim.fadeModels = function(fading)
{
   var plugin = KnowYourSkyrim.plugin();
   if(!KnowYourSkyrim.browsing)
   {
      return undefined;
   }
   if(fading)
   {
      plugin.ShowTile();
      // the menu may be closing, the next one opens the browser just as it is now
      KnowYourSkyrim.keepBrowser();
   }
   else
   {
      KnowYourSkyrim.showTile(KnowYourSkyrim.browserState.selected, KnowYourSkyrim.browserItem);
   }
   if(KnowYourSkyrim.panel == undefined && !KnowYourSkyrim.menuClosing)
   {
      plugin.ShowPreview(fading);
   }
};

// The browser steps aside while the card asks and comes back as it was once the lists answer again. The game may open
// its list of soul gems a few frames later, or not at all when there's none
KnowYourSkyrim.makeWayForCard = function()
{
   var browser = KnowYourSkyrim.browser;
   if(!KnowYourSkyrim.browsing || KnowYourSkyrim.panel != undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.hideBrowser();
   KnowYourSkyrim.drawBrowseButton();
   browser.asked = false;
   browser.waited = 0;
   browser.onEnterFrame = function()
   {
      var answer = KnowYourSkyrim.listsAnswer();
      this.waited++;
      this.asked = this.asked || !answer;
      if(answer && (this.asked || this.waited > KnowYourSkyrim.CARD_WAIT))
      {
         delete this.onEnterFrame;
         if(KnowYourSkyrim.browserState.open && !KnowYourSkyrim.browsing)
         {
            KnowYourSkyrim.comeBack();
         }
      }
   };
};

KnowYourSkyrim.comeBack = function()
{
   KnowYourSkyrim.browsing = true;
   KnowYourSkyrim.menu.itemCard._parent._visible = false;
   KnowYourSkyrim.plugin().ShowPreview(false);
   KnowYourSkyrim.keepTurnAreaOff(true);
   KnowYourSkyrim.browser._visible = true;
   KnowYourSkyrim.tellPointerArea();
   KnowYourSkyrim.showTile(KnowYourSkyrim.browserState.selected, KnowYourSkyrim.browserItem);
   KnowYourSkyrim.drawBrowseButton();
   // the way out for the card took the frame the page was waiting on
   if(KnowYourSkyrim.browserItem != undefined && (KnowYourSkyrim.browserSources == undefined || KnowYourSkyrim.browserSources.reading == true))
   {
      KnowYourSkyrim.askBrowserSources(KnowYourSkyrim.browserItem.id, 1);
   }
};

// the plugin keeps the browser for the next opening of a menu
KnowYourSkyrim.keepBrowser = function()
{
   var state = KnowYourSkyrim.browserState;
   var page = KnowYourSkyrim.browserLaid ? KnowYourSkyrim.browser.details.page : undefined;
   var scrolled = page != undefined && page.first != undefined ? page.first : 0;
   KnowYourSkyrim.plugin().KeepBrowser(state.open, state.words, state.family, KnowYourSkyrim.chosenBrowserChips().join(","), state.first, state.selected, state.mod, KnowYourSkyrim.browserHistory.join(","), KnowYourSkyrim.openListKeys().join(","), scrolled, state.within);
};

// The browser takes its room: the search and the plugin over the categories and their chips, then the rows on the
// left and the details of the chosen entry on the right
KnowYourSkyrim.layBrowser = function()
{
   var frame = KnowYourSkyrim.room();
   var holder = KnowYourSkyrim.roomClip();
   var browser = holder.createEmptyMovieClip("knowYourSkyrimBrowser", holder.getNextHighestDepth());
   var pad = KnowYourSkyrim.BROWSER_PAD;
   var wide = frame.right - frame.left;
   var high = frame.bottom - frame.top;
   var searchWide = Math.round(wide * KnowYourSkyrim.BROWSER_SEARCH_SHARE);
   var top;
   var column;
   KnowYourSkyrim.browser = browser;
   KnowYourSkyrim.browserLaid = true;
   browser._x = frame.left;
   browser._y = frame.top;
   browser.wide = wide;
   browser.high = high;
   browser.createEmptyMovieClip("back", 0);
   KnowYourSkyrim.panelBox(browser.back, wide, high, KnowYourSkyrim.BROWSER_SHADE, 40);
   // a drawn shape takes no mouse in this game, a press on the bare back would reach what lies under the browser
   browser.back.createTextField("touch", 0, 0, 0, wide, high);
   browser.back.touch.selectable = false;
   browser.back.onPress = function()
   {
      KnowYourSkyrim.endBrowserTyping(false);
   };
   // the other button of the mouse steps back through what was looked at
   browser.back.onPressAux = function()
   {
      KnowYourSkyrim.browserBack();
   };
   KnowYourSkyrim.buildBrowserSearch(browser, pad, pad, searchWide);
   KnowYourSkyrim.buildBrowserMods(browser, pad + searchWide + KnowYourSkyrim.BROWSER_FIELD_GAP, pad, Math.round(wide * KnowYourSkyrim.BROWSER_MODS_SHARE));
   KnowYourSkyrim.writeLine(browser, "count", 2, "", KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.HERO_DIM);
   browser.createEmptyMovieClip("tabs", 3);
   browser.tabs._y = pad + KnowYourSkyrim.BROWSER_HEAD + 6;
   browser.createEmptyMovieClip("chips", 4);
   browser.chips._y = browser.tabs._y + KnowYourSkyrim.BROWSER_STRIP + 2;
   browser.chipLines = KnowYourSkyrim.chipLinesOf(KnowYourSkyrim.browserFamily(KnowYourSkyrim.browserState.family).key);
   top = browser.chips._y + KnowYourSkyrim.BROWSER_STRIP * browser.chipLines + 8;
   column = Math.floor((wide - 3 * pad) / 2);
   KnowYourSkyrim.buildBrowserRows(browser, pad, top, column, high - top - pad);
   KnowYourSkyrim.buildBrowserDetails(browser, 2 * pad + column, top, wide - 3 * pad - column, high - top - pad);
   KnowYourSkyrim.buildModList(browser);
};

// The browser laid again with what it showed, because the chips of a category took another number of lines
KnowYourSkyrim.relayBrowser = function()
{
   KnowYourSkyrim.endBrowserTyping(false);
   KnowYourSkyrim.closeContext();
   KnowYourSkyrim.browser.removeMovieClip();
   KnowYourSkyrim.layBrowser();
   KnowYourSkyrim.browser._visible = KnowYourSkyrim.browsing;
   KnowYourSkyrim.tellPointerArea();
   KnowYourSkyrim.findBrowser(KnowYourSkyrim.browserState.first, true);
   KnowYourSkyrim.showBrowserItem(KnowYourSkyrim.browserState.selected);
   KnowYourSkyrim.drawBrowseButton();
   KnowYourSkyrim.padMark();
};

// The keys of the fields, the wheel over the rows, the details and the list of plugins, the bar of the rows held and
// dragged, the links of the details under the pointer
KnowYourSkyrim.listenBrowser = function()
{
   var menu = KnowYourSkyrim.menu;
   menu.knowYourSkyrimHandleInput = menu.handleInput;
   menu.handleInput = function(details, pathToFocus)
   {
      if(KnowYourSkyrim.contextKey(details) || KnowYourSkyrim.inspectKey(details) || KnowYourSkyrim.browserKey(details) || KnowYourSkyrim.belongingsKey(details))
      {
         return true;
      }
      return this.knowYourSkyrimHandleInput(details, pathToFocus);
   };
   KnowYourSkyrim.browserMouse = {};
   KnowYourSkyrim.browserMouse.onMouseWheel = function(delta)
   {
      var step = delta > 0 ? -1 : 1;
      if(!KnowYourSkyrim.browsing || KnowYourSkyrim.anyInspect())
      {
         return undefined;
      }
      if(KnowYourSkyrim.browser.modList._visible && KnowYourSkyrim.inBrowserClip(KnowYourSkyrim.browser.modList))
      {
         KnowYourSkyrim.scrollModList(step * KnowYourSkyrim.BROWSER_WHEEL);
      }
      else if(KnowYourSkyrim.belongings != undefined && KnowYourSkyrim.inBrowserClip(KnowYourSkyrim.belongingsClip))
      {
         KnowYourSkyrim.scrollBelongings(step * KnowYourSkyrim.BROWSER_WHEEL);
      }
      else if(KnowYourSkyrim.inBrowserClip(KnowYourSkyrim.browser.rows.view))
      {
         KnowYourSkyrim.scrollBrowserRows(step * KnowYourSkyrim.BROWSER_WHEEL);
      }
      else if(KnowYourSkyrim.inBrowserClip(KnowYourSkyrim.browser.details.view))
      {
         KnowYourSkyrim.scrollBrowserPage(step);
         KnowYourSkyrim.keepBrowser();
      }
   };
   KnowYourSkyrim.browserMouse.onMouseDown = function()
   {
      var link;
      if(!KnowYourSkyrim.browsing || KnowYourSkyrim.anyInspect())
      {
         return undefined;
      }
      if(KnowYourSkyrim.onBrowserBar())
      {
         KnowYourSkyrim.browserBarHeld = true;
         KnowYourSkyrim.dragBrowserBar();
         return undefined;
      }
      if(KnowYourSkyrim.browser.modList._visible && !KnowYourSkyrim.inBrowserClip(KnowYourSkyrim.browser.modList) && !KnowYourSkyrim.inBrowserClip(KnowYourSkyrim.browser.mods))
      {
         KnowYourSkyrim.endBrowserTyping(false);
      }
      link = KnowYourSkyrim.browserLinkUnderMouse();
      if(link != undefined)
      {
         KnowYourSkyrim.followBrowserLink(link.link);
      }
   };
   KnowYourSkyrim.browserMouse.onMouseMove = function()
   {
      if(KnowYourSkyrim.browserBarHeld)
      {
         KnowYourSkyrim.dragBrowserBar();
      }
      else if(KnowYourSkyrim.browsing && !KnowYourSkyrim.anyInspect())
      {
         KnowYourSkyrim.lightBrowserLink(KnowYourSkyrim.browserLinkUnderMouse());
      }
   };
   KnowYourSkyrim.browserMouse.onMouseUp = function()
   {
      if(KnowYourSkyrim.browserBarHeld)
      {
         KnowYourSkyrim.browserBarHeld = false;
         KnowYourSkyrim.drawBrowserBar();
      }
   };
   Mouse.addListener(KnowYourSkyrim.browserMouse);
};

KnowYourSkyrim.inBrowserClip = function(clip)
{
   var bounds;
   if(clip == undefined)
   {
      return false;
   }
   bounds = clip.getBounds(_root);
   return _root._xmouse >= bounds.xMin && _root._xmouse <= bounds.xMax && _root._ymouse >= bounds.yMin && _root._ymouse <= bounds.yMax;
};
