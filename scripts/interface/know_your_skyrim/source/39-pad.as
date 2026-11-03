KnowYourSkyrim.PAD_A = 276;
KnowYourSkyrim.PAD_B = 277;
KnowYourSkyrim.PAD_X = 278;
KnowYourSkyrim.PAD_Y = 279;
KnowYourSkyrim.PAD_LB = 274;
KnowYourSkyrim.PAD_RB = 275;
KnowYourSkyrim.PAD_RT = 281;
KnowYourSkyrim.PAD_BACK = 271;
KnowYourSkyrim.PAD_STICK = 273;
// SKSE's codes of a gamepad's buttons, the cross up to the right trigger
KnowYourSkyrim.PAD_FIRST = 266;
KnowYourSkyrim.PAD_CROSS_LAST = 269;
KnowYourSkyrim.PAD_LAST = 281;
KnowYourSkyrim.PAD_TURN_STEP = 15;
KnowYourSkyrim.PAD_RING_GAP = 3;
KnowYourSkyrim.PAD_HINTS = 6;
KnowYourSkyrim.PAD_TURNS_NOTHING = 0;
KnowYourSkyrim.PAD_TURNS_PAGE = 1;
KnowYourSkyrim.PAD_TURNS_ITEM = 2;
// the game calls the menu with the hand of a trigger, RT for the right one
KnowYourSkyrim.PAD_RIGHT_HAND = 0;
KnowYourSkyrim.PAD_INSPECT_HINT = KnowYourSkyrim.say("Right stick to turn") + "     " + KnowYourSkyrim.say("Up and down to zoom") + "     " + KnowYourSkyrim.say("B to close");

// A gamepad in the browser. The click of the right stick, the game's item zoom, moves the input from SkyUI's list into
// the browser and back, it opens the browser when it's shut, a hold shuts it. In the browser the cross walks the rows
// and the categories, LB and RB the subcategories, LT and RT the links of the page, A does what the entry or the lit
// link offers, Y stars, X is the cheat, B steps back, Back shuts the browser, the right stick scrolls the page. With
// the panel of Inventory Reforged the click walks the list, the panel and the browser in turn. Nothing of this acts
// while the mouse and the keyboard play
KnowYourSkyrim.buildPad = function()
{
   var menu = KnowYourSkyrim.menu;
   KnowYourSkyrim.padFocus = undefined;
   KnowYourSkyrim.padTurned = KnowYourSkyrim.PAD_TURNS_NOTHING;
   menu.knowYourSkyrimPadInput = menu.handleInput;
   menu.handleInput = function(details, pathToFocus)
   {
      return KnowYourSkyrim.padInput(details) || this.knowYourSkyrimPadInput(details, pathToFocus);
   };
   // the game calls these for the selected row of the list, while the browser holds the input there's none. The
   // triggers come as AttemptEquip and walk the links
   KnowYourSkyrim.padBlock(menu, "AttemptEquip", KnowYourSkyrim.padTrigger);
   KnowYourSkyrim.padBlock(menu, "DropItem");
   KnowYourSkyrim.padBlock(menu, "AttemptChargeItem");
   menu.knowYourSkyrimBottomBar = menu.updateBottomBar;
   menu.updateBottomBar = function(a_bSelected)
   {
      if(KnowYourSkyrim.padFocus != undefined)
      {
         KnowYourSkyrim.padHints();
         return undefined;
      }
      this.knowYourSkyrimBottomBar(a_bSelected);
      KnowYourSkyrim.padListHint();
   };
   // the game tells the menu when a gamepad or the keyboard takes over
   menu.knowYourSkyrimPlatform = menu.SetPlatform;
   menu.SetPlatform = function(a_platform, a_bPS3Switch)
   {
      this.knowYourSkyrimPlatform(a_platform, a_bPS3Switch);
      KnowYourSkyrim.padPlatform();
   };
};

// The menu's own call and the one taken instead ride on the wrapper itself. Another mod's native hook may keep the
// wrapper as its original and call it from there, and a function called that way sees nothing of the one it was made in
KnowYourSkyrim.padBlock = function(menu, name, instead)
{
   var blocked;
   if(menu[name] == undefined)
   {
      return undefined;
   }
   blocked = function()
   {
      var self = arguments.callee;
      if(KnowYourSkyrim.padFocus == undefined && !KnowYourSkyrim.inspecting)
      {
         self.own.apply(this, arguments);
      }
      else if(self.instead != undefined && !KnowYourSkyrim.inspecting)
      {
         self.instead.apply(KnowYourSkyrim, arguments);
      }
   };
   blocked.own = menu[name];
   blocked.instead = instead;
   menu[name] = blocked;
};

KnowYourSkyrim.padReady = function()
{
   return KnowYourSkyrim.menu != undefined && KnowYourSkyrim.menu._platform != 0;
};

// The keyboard or the mouse took over, the list has the input again. SkyUI's sign of the game's zoom goes while a
// gamepad plays, the click is the browser's. The panel of Inventory Reforged does the same for its own
KnowYourSkyrim.padPlatform = function()
{
   if(!KnowYourSkyrim.padReady())
   {
      KnowYourSkyrim.leavePad(false);
   }
   if(KnowYourSkyrim.panel == undefined)
   {
      KnowYourSkyrim.menu.inventoryLists.zoomButtonHolder._visible = !KnowYourSkyrim.padReady();
   }
   KnowYourSkyrim.padViewerHint();
   KnowYourSkyrim.padTurns();
   KnowYourSkyrim.padHintsNow();
};

KnowYourSkyrim.padViewerHint = function()
{
   var viewer = KnowYourSkyrim.viewer;
   var hint = viewer != undefined ? viewer.hint : undefined;
   if(hint == undefined)
   {
      return undefined;
   }
   hint.text = KnowYourSkyrim.padReady() ? KnowYourSkyrim.PAD_INSPECT_HINT : KnowYourSkyrim.INSPECT_HINT;
   hint._x = KnowYourSkyrim.screenRect().x + (KnowYourSkyrim.screenRect().width - hint._width) / 2;
};

// The plugin tells of a click of the stick, a hold says so. With the panel of Inventory Reforged the panel's own
// teller decides for both (padMeetPanel), the plugin's word is let go then
KnowYourSkyrim.padStick = function(held)
{
   if(KnowYourSkyrim.panel != undefined && KnowYourSkyrim.panel.knowYourSkyrimPadStick != undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.padClick(held == true, false);
};

// A click from the list opens the browser or moves the input into it, from the browser back to the list. A hold
// shuts the browser. True when the browser took the click
KnowYourSkyrim.padClick = function(held, fromPanel)
{
   if(!KnowYourSkyrim.padReady())
   {
      return false;
   }
   if(KnowYourSkyrim.inspecting)
   {
      KnowYourSkyrim.leaveInspect();
      return true;
   }
   if(held)
   {
      if(!KnowYourSkyrim.browsing || KnowYourSkyrim.panel != undefined && KnowYourSkyrim.padFocus == undefined)
      {
         return false;
      }
      KnowYourSkyrim.closeBrowser();
      gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
      return true;
   }
   if(KnowYourSkyrim.padFocus != undefined)
   {
      KnowYourSkyrim.leavePad(true);
      return true;
   }
   if(!fromPanel && !KnowYourSkyrim.listsAnswer())
   {
      return false;
   }
   if(!KnowYourSkyrim.browsing)
   {
      KnowYourSkyrim.openBrowser();
   }
   KnowYourSkyrim.enterPad();
   return true;
};

// The panel of Inventory Reforged has a click of its own, list and panel in turn, and lets it be while the browser is
// open. Its teller is wrapped so one of the two decides on the state before the click. From the panel's input the click
// opens the browser, while the browser is open it's the browser's
KnowYourSkyrim.padMeetPanel = function(panel)
{
   var own = panel.padStick;
   if(own == undefined || panel.knowYourSkyrimPadStick != undefined)
   {
      return undefined;
   }
   panel.knowYourSkyrimPadStick = own;
   panel.padStick = function(held)
   {
      var fromPanel = this.padFocus != undefined && held != true;
      if(fromPanel && this.listsAnswer())
      {
         this.leavePad(false);
         this.padMark();
         this.padTurns();
      }
      if((fromPanel || KnowYourSkyrim.browsing || KnowYourSkyrim.inspecting) && KnowYourSkyrim.padClick(held == true, fromPanel))
      {
         return undefined;
      }
      own.apply(this, arguments);
   };
};

// Alone the browser's plugin asks first when the game's item zoom key comes. On a gamepad the click is the browser's
// always, the watch of the stick tells of it. The key of the keyboard inspects the entry while the browser is open
KnowYourSkyrim.zoomKey = function()
{
   if(KnowYourSkyrim.menu == undefined)
   {
      return false;
   }
   if(KnowYourSkyrim.padReady())
   {
      return true;
   }
   if(KnowYourSkyrim.inspecting)
   {
      KnowYourSkyrim.leaveInspect();
      return true;
   }
   if(!KnowYourSkyrim.browsing)
   {
      return false;
   }
   // the game's zoom of the model the browser hides would fade the menu out under it, with no entry too
   if(KnowYourSkyrim.browserItem != undefined && KnowYourSkyrim.listsAnswer())
   {
      KnowYourSkyrim.inspectBrowserItem();
   }
   return true;
};

// The row the list was on is kept for the way back and no row is selected meanwhile, so the game's buttons for the
// selected row (favorite, drop, equip) act on nothing. The browser's first row is picked when none is
KnowYourSkyrim.enterPad = function()
{
   var list = KnowYourSkyrim.menu.inventoryLists.itemList;
   var page = KnowYourSkyrim.browserPage;
   if(!KnowYourSkyrim.browsing)
   {
      return undefined;
   }
   KnowYourSkyrim.endBrowserTyping(false);
   KnowYourSkyrim.padKept = list.selectedIndex;
   list.selectedIndex = -1;
   KnowYourSkyrim.padFocus = "browser";
   if(KnowYourSkyrim.browserItem == undefined && page != undefined && page.rows.length > 0)
   {
      KnowYourSkyrim.showBrowserItem(page.rows[0].id);
      KnowYourSkyrim.keepBrowser();
   }
   KnowYourSkyrim.padMark();
   KnowYourSkyrim.padHints();
   KnowYourSkyrim.padTurns();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.leavePad = function(sound)
{
   var list = KnowYourSkyrim.menu.inventoryLists.itemList;
   var kept = KnowYourSkyrim.padKept;
   if(KnowYourSkyrim.padFocus == undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.padFocus = undefined;
   KnowYourSkyrim.padKept = undefined;
   KnowYourSkyrim.closeBelongings();
   KnowYourSkyrim.lightBrowserLink(undefined);
   KnowYourSkyrim.padMark();
   if(list.selectedIndex < 0 && kept != undefined && kept >= 0 && list.getListEnumIndex(kept) != undefined)
   {
      list.selectedIndex = kept;
   }
   KnowYourSkyrim.menu.updateBottomBar(list.selectedIndex >= 0);
   KnowYourSkyrim.padTurns();
   if(sound)
   {
      gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
   }
};

// A held button repeats, only its press counts, and a button that did something here is kept from SkyUI to its let go,
// because B that stepped out of the browser would close the menu on its repeat. True when the browser took the input
KnowYourSkyrim.padInput = function(details)
{
   var nav = details.navEquivalent;
   var code = details.skseKeycode;
   var eating = KnowYourSkyrim.padEating;
   var pressed = Shared.GlobalFunc.IsKeyPressed(details);
   var taken;
   if(!KnowYourSkyrim.padReady())
   {
      return false;
   }
   if(eating != undefined && nav == eating.nav && code == eating.code)
   {
      if(details.value == "keyUp")
      {
         KnowYourSkyrim.padEating = undefined;
      }
      if(details.value != "keyDown")
      {
         return true;
      }
      KnowYourSkyrim.padEating = undefined;
   }
   if(KnowYourSkyrim.inspecting)
   {
      taken = pressed && KnowYourSkyrim.padInspectInput(nav, code);
   }
   else if(KnowYourSkyrim.belongings != undefined)
   {
      // what the list of someone's things doesn't take here its own keys take (belongingsKey)
      taken = pressed && KnowYourSkyrim.padBelongingsInput(nav, code);
   }
   else if(KnowYourSkyrim.padFocus == undefined || !KnowYourSkyrim.listsAnswer())
   {
      return false;
   }
   else
   {
      // a let go of the browser's buttons says nothing to SkyUI
      taken = !pressed || KnowYourSkyrim.padBrowserInput(nav, code, details.value == "keyDown");
   }
   if(taken && details.value == "keyDown" && KnowYourSkyrim.padButton(nav, code) != undefined)
   {
      KnowYourSkyrim.padEating = {nav:nav, code:code};
   }
   return taken;
};

// a way of the cross or of a stick, a button the game sends with a way of its own isn't one
KnowYourSkyrim.padWay = function(nav, code)
{
   var codes = gfx.ui.NavigationCode;
   if(code > KnowYourSkyrim.PAD_CROSS_LAST && code <= KnowYourSkyrim.PAD_LAST)
   {
      return undefined;
   }
   switch(nav)
   {
      case codes.UP:
         return "up";
      case codes.DOWN:
         return "down";
      case codes.LEFT:
         return "left";
      case codes.RIGHT:
         return "right";
   }
   return undefined;
};

// The button's own code first, because the game sends the shoulders with a Tab of their own too and RB stepped out of
// the browser as a B. B without its code comes as Tab or as Escape, whichever way the game sends it
KnowYourSkyrim.padButton = function(nav, code)
{
   var codes = gfx.ui.NavigationCode;
   switch(code)
   {
      case KnowYourSkyrim.PAD_A:
         return "a";
      case KnowYourSkyrim.PAD_B:
         return "b";
      case KnowYourSkyrim.PAD_X:
         return "x";
      case KnowYourSkyrim.PAD_Y:
         return "y";
      case KnowYourSkyrim.PAD_LB:
         return "lb";
      case KnowYourSkyrim.PAD_RB:
         return "rb";
      case KnowYourSkyrim.PAD_BACK:
         return "back";
   }
   if(code >= KnowYourSkyrim.PAD_FIRST && code <= KnowYourSkyrim.PAD_LAST)
   {
      return undefined;
   }
   if(nav == codes.ENTER)
   {
      return "a";
   }
   if(nav == codes.TAB || nav == codes.SHIFT_TAB || nav == codes.ESCAPE)
   {
      return "b";
   }
   return undefined;
};

// in the inspect view up and down bring the item closer, left and right turn it by a step, B closes it
KnowYourSkyrim.padInspectInput = function(nav, code)
{
   var way = KnowYourSkyrim.padWay(nav, code);
   var plugin = KnowYourSkyrim.plugin();
   if(KnowYourSkyrim.padButton(nav, code) == "b")
   {
      KnowYourSkyrim.leaveInspect();
      return true;
   }
   if(way == "up" || way == "down")
   {
      plugin.InspectZoom(way == "up" ? KnowYourSkyrim.INSPECT_ZOOM : 1 / KnowYourSkyrim.INSPECT_ZOOM);
   }
   else if(way != undefined)
   {
      plugin.InspectTurn(way == "left" ? -KnowYourSkyrim.PAD_TURN_STEP : KnowYourSkyrim.PAD_TURN_STEP, 0);
   }
   return true;
};

// Up and down walk the rows and pick each, left and right the categories, LB and RB the subcategories. A held
// direction walks on, a held button acts once
KnowYourSkyrim.padBrowserInput = function(nav, code, pressed)
{
   var way = KnowYourSkyrim.padWay(nav, code);
   var button = KnowYourSkyrim.padButton(nav, code);
   if(way == "up" || way == "down")
   {
      KnowYourSkyrim.padRow(way == "up" ? -1 : 1);
      return true;
   }
   if(way != undefined)
   {
      if(pressed)
      {
         KnowYourSkyrim.padFamily(way == "left" ? -1 : 1);
      }
      return true;
   }
   if(button == undefined || !pressed)
   {
      return button != undefined;
   }
   switch(button)
   {
      case "a":
         KnowYourSkyrim.padDeed();
         break;
      case "b":
         KnowYourSkyrim.padBack();
         break;
      case "x":
         KnowYourSkyrim.padCheat();
         break;
      case "y":
         KnowYourSkyrim.toggleFavorite();
         KnowYourSkyrim.padHints();
         break;
      case "lb":
      case "rb":
         KnowYourSkyrim.padChip(button == "lb" ? -1 : 1);
         break;
      case "back":
         KnowYourSkyrim.closeBrowser();
         gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
         break;
   }
   return true;
};

// The list of someone's things: the cross walks them and B shuts it (belongingsKey), with the cheats X takes or gives
// one, A all of it, LB and RB switch whose things
KnowYourSkyrim.padBelongingsInput = function(nav, code)
{
   var button = KnowYourSkyrim.padButton(nav, code);
   var cheats = KnowYourSkyrim.plugin().Cheats() == true;
   switch(button)
   {
      case "x":
      case "a":
         if(cheats)
         {
            KnowYourSkyrim.moveBelonging(button == "a");
         }
         return true;
      case "lb":
      case "rb":
         if(cheats)
         {
            KnowYourSkyrim.belongingsTab(!KnowYourSkyrim.belongings.player);
            KnowYourSkyrim.padHints();
         }
         return true;
      case "b":
      case "back":
         KnowYourSkyrim.closeBelongings();
         gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
         KnowYourSkyrim.padHintsNow();
         return true;
   }
   return false;
};

// The next row by the whole list of what the search found, the page of rows moves along when the row runs off it
KnowYourSkyrim.padRow = function(by)
{
   var page = KnowYourSkyrim.browserPage;
   var count = KnowYourSkyrim.browser.rows.count;
   var at = -1;
   var target;
   var i = 0;
   if(page == undefined || page.total == 0)
   {
      return undefined;
   }
   while(i < page.rows.length)
   {
      if(page.rows[i].id == KnowYourSkyrim.browserState.selected)
      {
         at = page.first + i;
      }
      i++;
   }
   target = at < 0 ? (by > 0 ? page.first : page.first + page.rows.length - 1) : Math.max(0, Math.min(page.total - 1, at + by));
   if(target == at)
   {
      return undefined;
   }
   if(target < page.first)
   {
      KnowYourSkyrim.findBrowser(target, false);
   }
   else if(target >= page.first + count)
   {
      KnowYourSkyrim.findBrowser(target - count + 1, false);
   }
   page = KnowYourSkyrim.browserPage;
   KnowYourSkyrim.showBrowserItem(page.rows[target - page.first].id);
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// the category beside, round the ends, its first row picked
KnowYourSkyrim.padFamily = function(by)
{
   var families = KnowYourSkyrim.BROWSER_FAMILIES;
   var at = 0;
   var i = 0;
   while(i < families.length)
   {
      if(families[i].key == KnowYourSkyrim.browserState.family)
      {
         at = i;
      }
      i++;
   }
   KnowYourSkyrim.chooseBrowserFamily(families[(at + by + families.length) % families.length].key);
   KnowYourSkyrim.padFirstRow();
};

// The subcategories of the category one at a time in the order of the strip, all of them between the last and the
// first. A chip the mouse added beside the one in turn is let go
KnowYourSkyrim.padChip = function(by)
{
   var rows = KnowYourSkyrim.CHIP_ROWS[KnowYourSkyrim.browserFamily(KnowYourSkyrim.browserState.family).key];
   var chips = KnowYourSkyrim.browserState.chips;
   var all = [];
   var at = -1;
   var g = 0;
   var c;
   var next;
   while(rows != undefined && g < rows.length)
   {
      c = 0;
      while(c < rows[g].length)
      {
         if(at < 0 && chips[g] == rows[g][c].key)
         {
            at = all.length;
         }
         if(rows[g][c].page != true)
         {
            all.push({group:g, key:rows[g][c].key});
         }
         c++;
      }
      g++;
   }
   if(all.length == 0)
   {
      return undefined;
   }
   next = (at + 1 + by + all.length + 1) % (all.length + 1) - 1;
   KnowYourSkyrim.browserState.chips = chips = [];
   if(next >= 0)
   {
      KnowYourSkyrim.pickChip(chips, rows, all[next].group, all[next].key);
   }
   KnowYourSkyrim.keepSkillWithTrainers();
   KnowYourSkyrim.findBrowser(0, true);
   KnowYourSkyrim.padFirstRow();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.padFirstRow = function()
{
   var page = KnowYourSkyrim.browserPage;
   KnowYourSkyrim.showBrowserItem(page != undefined && page.rows.length > 0 ? page.rows[0].id : 0);
   KnowYourSkyrim.keepBrowser();
};

// A follows the lit link, with none it does what the entry offers: an item is inspected, someone about shows their
// things, a place opens the map
KnowYourSkyrim.padDeed = function()
{
   var told = KnowYourSkyrim.browserItem;
   var lit = KnowYourSkyrim.browserLinkLit;
   var deed = KnowYourSkyrim.padDeedOf(told);
   if(lit != undefined)
   {
      KnowYourSkyrim.padFollow(lit);
   }
   else if(deed == "inspect")
   {
      KnowYourSkyrim.inspectBrowserItem();
   }
   else if(deed == "things")
   {
      KnowYourSkyrim.openBelongings(told.id);
      KnowYourSkyrim.padHints();
   }
   else if(deed == "map")
   {
      KnowYourSkyrim.followBrowserLink({marker:told.marker});
   }
};

KnowYourSkyrim.padDeedOf = function(told)
{
   if(told == undefined || told.quest == true || told.perk == true)
   {
      return undefined;
   }
   if(told.living == true)
   {
      return told.about > 0 ? "things" : undefined;
   }
   if(told.place == true)
   {
      return told.marker > 0 ? "map" : undefined;
   }
   return "inspect";
};

// A link to another entry walks there the way a press does. A list opened or shut keeps its link lit
KnowYourSkyrim.padFollow = function(lit)
{
   var at = KnowYourSkyrim.padLinkAt(lit);
   var id = KnowYourSkyrim.browserState.selected;
   KnowYourSkyrim.followBrowserLink(lit.link);
   if(KnowYourSkyrim.browserState.selected == id && KnowYourSkyrim.browsing)
   {
      KnowYourSkyrim.padLight(KnowYourSkyrim.padLinks()[at]);
   }
   KnowYourSkyrim.padHints();
};

// Every link of the head and the page in reading order, the way a press finds them
KnowYourSkyrim.padLinks = function()
{
   var details = KnowYourSkyrim.browser.details;
   var holders = [details.head, details.page.lines];
   var found = [];
   var h = 0;
   var f;
   var l;
   var field;
   while(h < holders.length)
   {
      f = 0;
      while(holders[h].linkFields != undefined && f < holders[h].linkFields.length)
      {
         field = holders[h].linkFields[f];
         l = 0;
         while(l < field.links.length)
         {
            found.push({field:field, from:field.links[l].from, to:field.links[l].to, link:field.links[l].link});
            l++;
         }
         f++;
      }
      h++;
   }
   return found;
};

KnowYourSkyrim.padLinkAt = function(lit)
{
   var links = KnowYourSkyrim.padLinks();
   var i = 0;
   while(lit != undefined && i < links.length)
   {
      if(links[i].field == lit.field && links[i].from == lit.from)
      {
         return i;
      }
      i++;
   }
   return -1;
};

// LT and RT, the link before or the next one, none between the last and the first. With none lit they start from
// what the page shows
KnowYourSkyrim.padTrigger = function(hand)
{
   var links;
   var at;
   var next;
   var forward = Number(hand) == KnowYourSkyrim.PAD_RIGHT_HAND;
   if(KnowYourSkyrim.padFocus == undefined || KnowYourSkyrim.belongings != undefined || !KnowYourSkyrim.browsing)
   {
      return undefined;
   }
   links = KnowYourSkyrim.padLinks();
   if(links.length == 0)
   {
      return undefined;
   }
   at = KnowYourSkyrim.padLinkAt(KnowYourSkyrim.browserLinkLit);
   next = at < 0 ? KnowYourSkyrim.padLinkInSight(links, forward) : (at + 1 + (forward ? 1 : -1) + links.length + 1) % (links.length + 1) - 1;
   KnowYourSkyrim.padLight(next >= 0 ? links[next] : undefined);
   KnowYourSkyrim.padHints();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// RT starts from the first link in sight, LT from the last, a quest's page runs long and the stick may have scrolled
// it. The head's links are in sight while the page isn't scrolled
KnowYourSkyrim.padLinkInSight = function(links, forward)
{
   var page = KnowYourSkyrim.browser.details.page;
   var room = KnowYourSkyrim.browser.details.view._height;
   var shown = page.lines != undefined ? -page.lines._y : 0;
   var found = -1;
   var box;
   var top;
   var i = 0;
   while(i < links.length)
   {
      if(links[i].field._parent != page.lines)
      {
         top = shown > 0 ? -1 : shown;
      }
      else
      {
         box = links[i].field.getCharBoundaries(links[i].from);
         top = links[i].field._y + (box != undefined ? box.y : 0);
      }
      if(top >= shown && top + KnowYourSkyrim.BROWSER_ROW <= shown + room)
      {
         found = i;
         if(forward)
         {
            return i;
         }
      }
      i++;
   }
   return found >= 0 ? found : (forward ? 0 : links.length - 1);
};

// the link lit as under the pointer, the page scrolled to it
KnowYourSkyrim.padLight = function(found)
{
   var page = KnowYourSkyrim.browser.details.page;
   var room = KnowYourSkyrim.browser.details.view._height;
   var step = KnowYourSkyrim.BROWSER_PAGE_STEP;
   var box;
   var top;
   var shown;
   KnowYourSkyrim.lightBrowserLink(found);
   if(found == undefined || found.field._parent != page.lines || page.full == undefined)
   {
      return undefined;
   }
   box = found.field.getCharBoundaries(found.from);
   top = found.field._y + (box != undefined ? box.y : 0);
   shown = -page.lines._y;
   if(top < shown)
   {
      KnowYourSkyrim.scrollBrowserPage(Math.floor(top / step) - page.first);
   }
   else if(top + KnowYourSkyrim.BROWSER_ROW > shown + room)
   {
      KnowYourSkyrim.scrollBrowserPage(Math.ceil((top + KnowYourSkyrim.BROWSER_ROW - room) / step) - page.first);
   }
};

// B puts out the lit link, then walks back what was looked at, then gives the input back to the list
KnowYourSkyrim.padBack = function()
{
   if(KnowYourSkyrim.browserLinkLit != undefined)
   {
      KnowYourSkyrim.lightBrowserLink(undefined);
      KnowYourSkyrim.padHints();
      return undefined;
   }
   if(KnowYourSkyrim.browserHistory.length > 0)
   {
      KnowYourSkyrim.browserBack();
      return undefined;
   }
   KnowYourSkyrim.leavePad(true);
};

// X does the cheat of the entry: TAKE as many as the box says, KILL or RESURRECT
KnowYourSkyrim.padCheat = function()
{
   var deed = KnowYourSkyrim.padCheatOf(KnowYourSkyrim.browserItem);
   if(deed != undefined)
   {
      KnowYourSkyrim.cheat(deed);
      KnowYourSkyrim.showCheats(KnowYourSkyrim.browserItem);
      KnowYourSkyrim.placeButtonLine();
      KnowYourSkyrim.padHints();
   }
};

KnowYourSkyrim.padCheatOf = function(told)
{
   var plugin = KnowYourSkyrim.plugin();
   var deeds;
   if(told == undefined || plugin.Cheats() != true)
   {
      return undefined;
   }
   deeds = KnowYourSkyrim.cheatDeeds(told, told.living == true ? plugin.CheatTargets(told.id) : undefined);
   return deeds.length > 0 ? deeds[0] : undefined;
};

// the plugin turns the right stick into steps of the page while the browser holds the input
KnowYourSkyrim.padScroll = function(steps)
{
   if(KnowYourSkyrim.padFocus == undefined || KnowYourSkyrim.inspecting || !KnowYourSkyrim.browsing)
   {
      return undefined;
   }
   KnowYourSkyrim.scrollBrowserPage(Number(steps));
   KnowYourSkyrim.keepBrowser();
};

// What the stick does now: turns the inspected item, scrolls the page while the browser has the input, nothing else
KnowYourSkyrim.padTurns = function()
{
   var turns = KnowYourSkyrim.PAD_TURNS_NOTHING;
   var plugin = KnowYourSkyrim.plugin();
   if(KnowYourSkyrim.padReady() && KnowYourSkyrim.inspecting)
   {
      turns = KnowYourSkyrim.PAD_TURNS_ITEM;
   }
   else if(KnowYourSkyrim.padReady() && KnowYourSkyrim.padFocus != undefined)
   {
      turns = KnowYourSkyrim.PAD_TURNS_PAGE;
   }
   if(turns != KnowYourSkyrim.padTurned && plugin != undefined && plugin.BrowserPadTurns != undefined)
   {
      KnowYourSkyrim.padTurned = turns;
      plugin.BrowserPadTurns(turns);
   }
};

// A gold line around the browser while it holds the input, so it reads from the couch
KnowYourSkyrim.padMark = function()
{
   var browser = KnowYourSkyrim.browser;
   var gap = KnowYourSkyrim.PAD_RING_GAP;
   var ring;
   if(!KnowYourSkyrim.browserLaid)
   {
      return undefined;
   }
   ring = browser.padRing != undefined ? browser.padRing : browser.createEmptyMovieClip("padRing", 30);
   ring.clear();
   if(KnowYourSkyrim.padFocus == undefined)
   {
      return undefined;
   }
   ring.lineStyle(2, KnowYourSkyrim.GOLD, 90, true);
   KnowYourSkyrim.roundBox(ring, -gap, -gap, browser.wide + 2 * gap, browser.high + 2 * gap, 6);
   ring.lineStyle();
};

KnowYourSkyrim.padHintsNow = function()
{
   var list = KnowYourSkyrim.menu.inventoryLists.itemList;
   if(KnowYourSkyrim.padFocus != undefined)
   {
      KnowYourSkyrim.padHints();
   }
   else
   {
      KnowYourSkyrim.menu.updateBottomBar(list.selectedIndex >= 0);
   }
};

// The bottom bar while the browser holds the input, as many as the bar holds: what A does, the cheat, the star, the
// links, the way back, the stick to the list, Back to shut the browser
KnowYourSkyrim.padHints = function()
{
   var menu = KnowYourSkyrim.menu;
   var told = KnowYourSkyrim.browserItem;
   var hints = [];
   var deed;
   var cheat;
   if(KnowYourSkyrim.padFocus == undefined || menu.navPanel == undefined)
   {
      return undefined;
   }
   if(KnowYourSkyrim.belongings != undefined)
   {
      KnowYourSkyrim.padShowHints(KnowYourSkyrim.padThingsHints());
      return undefined;
   }
   deed = KnowYourSkyrim.browserLinkLit != undefined ? "link" : KnowYourSkyrim.padDeedOf(told);
   if(deed != undefined)
   {
      hints.push({text:KnowYourSkyrim.padDeedWords(deed), controls:menu._acceptControls});
   }
   cheat = KnowYourSkyrim.padCheatOf(told);
   if(cheat != undefined)
   {
      hints.push({text:KnowYourSkyrim.padCheatWords(cheat), controls:{keyCode:KnowYourSkyrim.PAD_X}});
   }
   if(told != undefined)
   {
      hints.push({text:told.favorite == true ? KnowYourSkyrim.say("Unfavorite") : KnowYourSkyrim.say("Favorite"), controls:{keyCode:KnowYourSkyrim.PAD_Y}});
   }
   if(told != undefined && KnowYourSkyrim.padLinks().length > 0)
   {
      hints.push({text:KnowYourSkyrim.say("Links"), controls:{keyCode:KnowYourSkyrim.PAD_RT}});
   }
   hints.push({text:KnowYourSkyrim.say("Back"), controls:menu._cancelControls});
   hints.push({text:KnowYourSkyrim.say("List"), controls:{keyCode:KnowYourSkyrim.PAD_STICK}});
   hints.push({text:KnowYourSkyrim.say("Close"), controls:{keyCode:KnowYourSkyrim.PAD_BACK}});
   KnowYourSkyrim.padShowHints(hints);
};

KnowYourSkyrim.padDeedWords = function(deed)
{
   switch(deed)
   {
      case "link":
         return KnowYourSkyrim.say("Open link");
      case "things":
         return KnowYourSkyrim.say("Inventory");
      case "map":
         return KnowYourSkyrim.say("Map");
   }
   return KnowYourSkyrim.say("Inspect");
};

// the words of X are the words of the cheat's button
KnowYourSkyrim.padCheatWords = function(deed)
{
   switch(deed)
   {
      case "kill":
         return KnowYourSkyrim.say("Kill");
      case "raise":
         return KnowYourSkyrim.say("Resurrect");
      case "start":
         return KnowYourSkyrim.say("Start");
      case "finish":
         return KnowYourSkyrim.say("Complete");
      case "add":
         return KnowYourSkyrim.say("Add");
      case "remove":
         return KnowYourSkyrim.say("Remove");
   }
   return KnowYourSkyrim.say("Take");
};

// The list of someone's things hides SkyUI's buttons for the mouse, a gamepad shows its own there
KnowYourSkyrim.padThingsHints = function()
{
   var menu = KnowYourSkyrim.menu;
   var state = KnowYourSkyrim.belongings;
   var hints = [];
   if(KnowYourSkyrim.plugin().Cheats() == true)
   {
      hints.push({text:state.player ? KnowYourSkyrim.say("Give one") : KnowYourSkyrim.say("Take one"), controls:{keyCode:KnowYourSkyrim.PAD_X}});
      hints.push({text:state.player ? KnowYourSkyrim.say("Give all") : KnowYourSkyrim.say("Take all"), controls:menu._acceptControls});
      hints.push({text:state.player ? state.told.name : KnowYourSkyrim.say("You"), controls:{keyCode:KnowYourSkyrim.PAD_RB}});
   }
   hints.push({text:KnowYourSkyrim.say("Close"), controls:menu._cancelControls});
   return hints;
};

KnowYourSkyrim.padShowHints = function(hints)
{
   var panel = KnowYourSkyrim.menu.navPanel;
   var i = 0;
   panel.clearButtons();
   while(i < hints.length && i < KnowYourSkyrim.PAD_HINTS)
   {
      panel.addButton(hints[i]);
      i++;
   }
   panel.updateButtons(true);
   KnowYourSkyrim.menu.bottomBar.buttonPanel._visible = true;
};

// On the list the way into the browser, after SkyUI's own buttons. With the panel of Inventory Reforged only while
// the browser is open, the click is the panel's otherwise
KnowYourSkyrim.padListHint = function()
{
   var panel = KnowYourSkyrim.menu.navPanel;
   if(!KnowYourSkyrim.padReady() || KnowYourSkyrim.inspecting || panel == undefined || KnowYourSkyrim.panel != undefined && !KnowYourSkyrim.browsing)
   {
      return undefined;
   }
   panel.addButton({text:KnowYourSkyrim.say("Browser"), controls:{keyCode:KnowYourSkyrim.PAD_STICK}});
   panel.updateButtons(true);
};
