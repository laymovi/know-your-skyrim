// For the test stand, the middle of a part of the browser as aimedX, aimedY. "button", "search", "mods", "inspect",
// "back", "star", "things", "take", "count", "kill", "raise", "row,3", "tab,armor", "chip,heavy", "mod,Dawnguard.esm",
// "link,Iron Ingot", and of the list of someone's things "own,row,3", "own,tab,1", "own,one", "own,all", "own,cross"
KnowYourSkyrim.aimBrowser = function(told)
{
   var parts = String(told).split(",");
   var key = parts[0].toLowerCase();
   var browser = KnowYourSkyrim.browser;
   var clip;
   var i = 0;
   var share;
   var bounds;
   var spot;
   KnowYourSkyrim.aimedX = KnowYourSkyrim.aimedY = -1;
   if(key == "button")
   {
      clip = KnowYourSkyrim.browseButton;
   }
   else if(key == "search" || key == "mods" || key == "inspect" || key == "back" || key == "clear" || key == "within")
   {
      clip = key == "inspect" ? browser.details.inspect : (key == "back" ? browser.details.back : (key == "clear" ? browser.mods.cross : browser[key]));
   }
   else if(key == "find")
   {
      clip = browser.details.find._visible ? browser.details.find : undefined;
   }
   else if(key == "star" || key == "things")
   {
      clip = browser.details[key]._visible ? browser.details[key] : undefined;
   }
   else if(key == "page" || key == "rows")
   {
      clip = key == "page" ? browser.details.view : browser.rows.view;
   }
   else if(key == "own")
   {
      clip = KnowYourSkyrim.belongingsPart(parts[1], parts[2]);
   }
   else if(key == "take" || key == "count" || key == "kill" || key == "raise" || key == "start" || key == "finish" || key == "add" || key == "remove")
   {
      clip = browser.details.cheats[key]._visible ? browser.details.cheats[key] : undefined;
   }
   else if(key == "row")
   {
      clip = browser.rows["row" + Number(parts[1])];
   }
   else if(key == "tab")
   {
      while(browser.tabs["tab" + i] != undefined && browser.tabs["tab" + i].family.toLowerCase() != (parts[1].toLowerCase() == "all" ? "" : parts[1].toLowerCase()))
      {
         i++;
      }
      clip = browser.tabs["tab" + i];
   }
   else if(key == "chip")
   {
      i = 1;
      while(browser.chips.row["chip" + i] != undefined && browser.chips.row["chip" + i].key.toLowerCase() != parts[1].toLowerCase())
      {
         i++;
      }
      clip = browser.chips.row["chip" + i];
   }
   else if(key == "mod")
   {
      while(i < KnowYourSkyrim.BROWSER_LIST_ROWS && browser.modList["row" + i] != undefined && KnowYourSkyrim.modsFound[KnowYourSkyrim.modFirst + i].name != parts[1])
      {
         i++;
      }
      clip = browser.modList["row" + i];
   }
   else if(key == "link")
   {
      spot = KnowYourSkyrim.browserLinkSpot(parts.slice(1).join(","));
      if(spot != undefined)
      {
         share = KnowYourSkyrim.stageShare(spot.x, spot.y);
         KnowYourSkyrim.aimedX = share.x;
         KnowYourSkyrim.aimedY = share.y;
      }
      return undefined;
   }
   if(clip == undefined)
   {
      return undefined;
   }
   bounds = clip.getBounds(_root);
   share = KnowYourSkyrim.stageShare((bounds.xMin + bounds.xMax) / 2, (bounds.yMin + bounds.yMax) / 2);
   KnowYourSkyrim.aimedX = share.x;
   KnowYourSkyrim.aimedY = share.y;
   KnowYourSkyrim.aimedTop = String(Mouse.getTopMostEntity((bounds.xMin + bounds.xMax) / 2, (bounds.yMin + bounds.yMax) / 2, true));
};

// For the test stand, the middle of the row of an item in SkyUI's list, if the list shows it now, as aimedX, aimedY
KnowYourSkyrim.aimListRow = function(formId)
{
   var list = KnowYourSkyrim.menu.inventoryLists.itemList;
   var entries = list.entryList;
   var i = 0;
   var row;
   var point;
   KnowYourSkyrim.aimedX = KnowYourSkyrim.aimedY = -1;
   while(i < entries.length && entries[i].formId != Number(formId))
   {
      i++;
   }
   row = i < entries.length && entries[i].clipIndex != undefined ? list.getClipByIndex(entries[i].clipIndex) : undefined;
   if(row == undefined)
   {
      return undefined;
   }
   point = {x:row._x + row._width * 0.3, y:row._y + list.layout.entryHeight / 2};
   row._parent.localToGlobal(point);
   point = KnowYourSkyrim.stageShare(point.x, point.y);
   KnowYourSkyrim.aimedX = point.x;
   KnowYourSkyrim.aimedY = point.y;
};

// For the test stand, the middle of the second letter of a link with these words, or of the link of that number counted
// from 0 when the words are "#3", or of the first link that opens or shuts a list for "(opens)" and "(shuts)", in the
// coordinates of the root
KnowYourSkyrim.browserLinkSpot = function(words)
{
   var details = KnowYourSkyrim.browser.details;
   var holders = [details.head, details.page.lines];
   var counted = words.charAt(0) == "#" && !isNaN(Number(words.substr(1))) ? Number(words.substr(1)) : -1;
   var seen = 0;
   var h = 0;
   var f;
   var l;
   var field;
   var box;
   var point;
   while(h < holders.length)
   {
      f = 0;
      while(holders[h].linkFields != undefined && f < holders[h].linkFields.length)
      {
         field = holders[h].linkFields[f];
         l = 0;
         while(l < field.links.length)
         {
            seen++;
            if(counted >= 0 ? seen - 1 == counted : (field.links[l].link.text == words || words == "(opens)" && field.links[l].link.open != undefined || words == "(shuts)" && field.links[l].link.shut != undefined))
            {
               box = field.getCharBoundaries(field.links[l].from + 1);
               point = {x:field._x + box.x + box.width / 2, y:field._y + box.y + box.height / 2};
               field._parent.localToGlobal(point);
               return point;
            }
            l++;
         }
         f++;
      }
      h++;
   }
   return undefined;
};

KnowYourSkyrim.linkGoal = function(link)
{
   if(link.open != undefined || link.shut != undefined)
   {
      return link.open != undefined ? " (opens " + link.open + ")" : " (shuts " + link.shut + ")";
   }
   if(link.marker != undefined)
   {
      return " (map " + KnowYourSkyrim.formHex(link.marker) + ")";
   }
   if(link.pick != undefined)
   {
      return " (mod " + link.pick + ")";
   }
   return link.search != undefined ? " (search " + link.search + ")" : " (" + KnowYourSkyrim.formHex(link.id) + ")";
};

// For the test stand, every link of the details in order, with what it leads to
KnowYourSkyrim.linksReport = function(note)
{
   var details = KnowYourSkyrim.browser.details;
   var holders = [details.head, details.page.lines];
   var found = [];
   var h = 0;
   var f;
   var l;
   var link;
   while(h < holders.length)
   {
      f = 0;
      while(holders[h].linkFields != undefined && f < holders[h].linkFields.length)
      {
         l = 0;
         while(l < holders[h].linkFields[f].links.length)
         {
            link = holders[h].linkFields[f].links[l].link;
            found.push("#" + found.length + " " + link.text + KnowYourSkyrim.linkGoal(link));
            l++;
         }
         f++;
      }
      h++;
   }
   KnowYourSkyrim.standLog("PANELTEST links " + note + ": " + found.join(", "));
};

// For the test stand, the page that starts at this row, and every row of it in the log. A name with a letter past plain
// Latin is written with the numbers of those letters, a font may have no picture for them
KnowYourSkyrim.browserJump = function(first)
{
   KnowYourSkyrim.findBrowser(Number(first), false);
};

KnowYourSkyrim.pageReport = function(note)
{
   var page = KnowYourSkyrim.browserPage;
   var rows = [];
   var i = 0;
   var name;
   var odd;
   var c;
   while(page != undefined && i < page.rows.length)
   {
      name = page.rows[i].name;
      odd = [];
      c = 0;
      while(c < name.length)
      {
         if(name.charCodeAt(c) > 126)
         {
            odd.push(name.charCodeAt(c));
         }
         c++;
      }
      rows.push(name + (odd.length > 0 ? " {" + odd.join(" ") + "}" : "") + " [" + page.rows[i].mod + "]");
      i++;
   }
   KnowYourSkyrim.standLog("PANELTEST page " + note + ": from " + (page != undefined ? page.first : "-") + " of " + (page != undefined ? page.total : "-") + ": " + rows.join(" | "));
};

// words searched as if typed, a link of the page searches this way too
KnowYourSkyrim.searchBrowser = function(words)
{
   KnowYourSkyrim.browser.search.field.text = String(words);
   KnowYourSkyrim.browserSearchNow();
};

// For the test stand, the page scrolled so that the section with these words for its head starts at the top. Papyrus
// hands strings over in the case it met first, the head is matched in any case
KnowYourSkyrim.pageToSection = function(words)
{
   var page = KnowYourSkyrim.browser.details.page;
   var heads = page.lines.heads;
   var asked = String(words).toLowerCase();
   var i = 0;
   while(heads != undefined && i < heads.length)
   {
      if(String(heads[i].head).toLowerCase() == asked)
      {
         page.first = Math.floor(heads[i].y / KnowYourSkyrim.BROWSER_PAGE_STEP);
         KnowYourSkyrim.scrollBrowserPage(0);
         return undefined;
      }
      i++;
   }
};

// For the test stand, the state of the browser in one line of the log
KnowYourSkyrim.browserReport = function(note)
{
   var state = KnowYourSkyrim.browserState;
   var page = KnowYourSkyrim.browserPage;
   var told = KnowYourSkyrim.browserItem;
   var rows = [];
   var where = "-";
   var i = 0;
   while(page != undefined && i < page.rows.length && i < 5)
   {
      rows.push(page.rows[i].name + (page.rows[i].variants > 0 ? " +" + page.rows[i].variants : "") + " [" + page.rows[i].mod + "]");
      i++;
   }
   if(told != undefined && KnowYourSkyrim.browserSources != undefined)
   {
      where = KnowYourSkyrim.sectionsText(KnowYourSkyrim.pageSections(told, KnowYourSkyrim.browserSources));
   }
   KnowYourSkyrim.standLog("PANELTEST browser " + note + ": open " + KnowYourSkyrim.browsing + ", words \"" + state.words + "\", mod \"" + state.mod + "\", family " + state.family + ", chips " + KnowYourSkyrim.chosenBrowserChips().join("/") + ", total " + (page != undefined ? page.total : "-") + ", first " + state.first + ", rows " + rows.join(", ") + "; selected " + (told != undefined ? told.name + (told.favorite == true ? " (starred)" : "") : "-") + ", back " + KnowYourSkyrim.browserHistory.length + "; page " + where + "; " + KnowYourSkyrim.besideText() + ", typing " + KnowYourSkyrim.browserTyping);
};

// For the test stand, what the menu lets through now, whether it faded for the game's zoom, whether SkyUI's lists
// answer, and the area the game turns its own preview in
KnowYourSkyrim.menuReport = function(note)
{
   var menu = KnowYourSkyrim.menu;
   var lists = menu.inventoryLists;
   var rect = menu.mouseRotationRect;
   KnowYourSkyrim.standLog("PANELTEST menu " + note + ": faded in " + menu.bFadedIn + ", lists answer " + KnowYourSkyrim.listsAnswer() + " (items input " + !lists.itemList.disableInput + ", items selection " + !lists.itemList.disableSelection + ", categories " + !lists.categoryList.disableInput + "), inspecting " + KnowYourSkyrim.inspecting + ", browsing " + KnowYourSkyrim.browsing + ", rotation area " + (rect != undefined ? Math.round(rect._xscale) + "%" : "none") + ", pad " + KnowYourSkyrim.padFocus + ", family " + KnowYourSkyrim.browserState.family + ", selected " + (KnowYourSkyrim.browserItem != undefined ? KnowYourSkyrim.browserItem.name : "-"));
};

// For the test stand, the menu told a gamepad (1) or the keyboard (0) took over, the way the game tells it
KnowYourSkyrim.padPlatformAs = function(platform)
{
   KnowYourSkyrim.menu.SetPlatform(Number(platform), false);
};

// For the test stand, where the gamepad's input is, what the stick does, the lit link, the selected row of SkyUI's list
// and what the bottom bar says
KnowYourSkyrim.padReport = function(note)
{
   var menu = KnowYourSkyrim.menu;
   var panel = menu.navPanel;
   var list = menu.inventoryLists.itemList;
   var lit = KnowYourSkyrim.browserLinkLit;
   var page = KnowYourSkyrim.browserLaid ? KnowYourSkyrim.browser.details.page : undefined;
   var words = [];
   var i = 0;
   while(panel != undefined && i < panel.buttons.length)
   {
      if(panel.buttons[i]._visible && panel.buttons[i].label.length > 0)
      {
         words.push(panel.buttons[i].label);
      }
      i++;
   }
   KnowYourSkyrim.standLog("PANELTEST pad " + note + ": platform " + menu._platform + ", input " + (KnowYourSkyrim.padFocus != undefined ? KnowYourSkyrim.padFocus : "list") + (_global.Equipment != undefined && _global.Equipment.padFocus != undefined ? " (the panel's on " + _global.Equipment.padFocus.kind + ")" : "") + ", browsing " + KnowYourSkyrim.browsing + ", stick " + ["nothing", "page", "item"][KnowYourSkyrim.padTurned] + ", inspect " + KnowYourSkyrim.inspecting + ", family " + KnowYourSkyrim.browserState.family + ", chips " + KnowYourSkyrim.chosenBrowserChips().join("+") + ", selected " + (KnowYourSkyrim.browserItem != undefined ? KnowYourSkyrim.browserItem.name : "-") + ", link " + (lit != undefined ? lit.link.text + KnowYourSkyrim.linkGoal(lit.link) : "-") + ", page line " + (page != undefined ? page.first : "-") + ", things " + (KnowYourSkyrim.belongings != undefined ? (KnowYourSkyrim.belongings.player ? "yours" : "theirs") : "shut") + ", list row " + list.selectedIndex + (list.selectedEntry != undefined ? " " + list.selectedEntry.text : "") + ", zoom sign " + menu.inventoryLists.zoomButtonHolder._visible + ", bar " + words.join(" | "));
};

// For the test stand, no row of SkyUI's list picked, a pointer thrown across the list picks one for the pictures
KnowYourSkyrim.unpickList = function()
{
   var list = KnowYourSkyrim.menu.inventoryLists.itemList;
   list.selectedIndex = -1;
   KnowYourSkyrim.menu.updateBottomBar(false);
};

// For the test stand, the entry shown starred (1) or not (0) whatever it was, a press of the star only turns it over
KnowYourSkyrim.starBrowser = function(on)
{
   var told = KnowYourSkyrim.browserItem;
   if(told != undefined && (told.favorite == true) != (Number(on) == 1))
   {
      KnowYourSkyrim.toggleFavorite();
   }
};

// For the test stand, a part of the list of someone's things, none while the list is shut
KnowYourSkyrim.belongingsPart = function(part, which)
{
   var clip = KnowYourSkyrim.belongingsClip;
   if(clip == undefined)
   {
      return undefined;
   }
   if(part == "row")
   {
      return clip.rows["row" + Number(which)]._visible ? clip.rows["row" + Number(which)] : undefined;
   }
   if(part == "tab")
   {
      return clip.tabs["tab" + Number(which)];
   }
   if(part == "one" || part == "all")
   {
      return KnowYourSkyrim.contextLine(part);
   }
   return part == "cross" ? clip.cross : undefined;
};

// For the test stand, the line of the open menu with this key, none while no menu is open
KnowYourSkyrim.contextLine = function(key)
{
   var context = KnowYourSkyrim.context;
   for(var name in context)
   {
      if(context[name] != undefined && context[name].what != undefined && context[name].what.key == String(key))
      {
         return context[name];
      }
   }
   return undefined;
};

// For the test stand, the words of the lines of the open menu
KnowYourSkyrim.contextText = function()
{
   var context = KnowYourSkyrim.context;
   var words = [];
   var i = 0;
   while(context != undefined && i < 40)
   {
      if(context["line" + i] != undefined)
      {
         words.push(context["line" + i].words.text + (context["line" + i].what.chosen == true ? " (chosen)" : ""));
      }
      i++;
   }
   return context == undefined ? "shut" : words.join(" | ");
};

// For the test stand, the list of someone's things in one line of the log, where it stands and what SkyUI's list does
KnowYourSkyrim.belongingsReport = function(note)
{
   var state = KnowYourSkyrim.belongings;
   var clip = KnowYourSkyrim.belongingsClip;
   var lists = KnowYourSkyrim.menu.inventoryLists;
   var rows = [];
   var bounds;
   var i = 0;
   if(state == undefined)
   {
      KnowYourSkyrim.standLog("PANELTEST things " + note + ": shut, SkyUI's list shown " + lists._visible + ", hints shown " + KnowYourSkyrim.menu.bottomBar.buttonPanel._visible);
      return undefined;
   }
   while(i < state.told.rows.length && i < 12)
   {
      rows.push(state.told.rows[i].name + " x" + state.told.rows[i].count + (state.told.rows[i].worn == true ? " worn" : "") + (state.told.rows[i].id == state.selected ? " (picked)" : ""));
      i++;
   }
   bounds = clip.getBounds(_root);
   KnowYourSkyrim.standLog("PANELTEST things " + note + ": of " + state.told.name + (state.told.dead == true ? " (dead)" : "") + ", tab " + (state.player ? "player" : "theirs") + ", " + state.told.rows.length + " things: " + rows.join(", ") + "; at " + Math.round(bounds.xMin) + ".." + Math.round(bounds.xMax) + " x " + Math.round(bounds.yMin) + ".." + Math.round(bounds.yMax) + ", SkyUI's list shown " + lists._visible + ", foot " + clip.foot.hint.text + ", menu " + KnowYourSkyrim.contextText());
};

// what stands beside the browser: the panel of Inventory Reforged and whether it shows, or the item card of SkyUI
KnowYourSkyrim.besideText = function()
{
   var panel = KnowYourSkyrim.panel;
   if(panel != undefined)
   {
      return "with the panel, panel shown " + panel.panel._visible + ", panel browsing " + panel.browsing;
   }
   return "alone, item card shown " + KnowYourSkyrim.menu.itemCard._parent._visible;
};

// For the test stand, where the browser, its room and its eye button stand, in the units of the stage
KnowYourSkyrim.roomReport = function(note)
{
   var browser = KnowYourSkyrim.browser;
   var button = KnowYourSkyrim.browseButton;
   var at = function(clip)
   {
      var bounds = clip != undefined ? clip.getBounds(_root) : undefined;
      return bounds != undefined ? Math.round(bounds.xMin) + ".." + Math.round(bounds.xMax) + " x " + Math.round(bounds.yMin) + ".." + Math.round(bounds.yMax) : "-";
   };
   var cheats = [];
   var names = ["take", "kill", "raise"];
   var i = 0;
   while(KnowYourSkyrim.browserLaid && i < names.length)
   {
      if(browser.details.cheats._visible && browser.details.cheats[names[i]]._visible)
      {
         cheats.push(names[i]);
      }
      i++;
   }
   KnowYourSkyrim.standLog("PANELTEST room " + note + ": " + KnowYourSkyrim.besideText() + ", browser " + (KnowYourSkyrim.browserLaid && browser._visible ? at(browser) : "shut") + ", eye " + at(button) + (button.gold ? " gold" : " plain") + ", inspecting " + KnowYourSkyrim.inspecting + ", cheats " + (cheats.length > 0 ? cheats.join("/") : "-") + ", count " + KnowYourSkyrim.cheatCount + ", squash " + Math.round(KnowYourSkyrim.squash * 100) / 100 + ", paper " + KnowYourSkyrim.paper + ", says \"" + (KnowYourSkyrim.browserLaid ? browser.count.text : "") + "\"");
};

// For the test stand, a long line goes into the log in pieces, SKSE's log takes the game down on the whole text of a
// page as tall as Steel Ingot's
KnowYourSkyrim.standLog = function(text)
{
   var most = 1000;
   var at = 0;
   while(at < text.length)
   {
      skse.Log((at > 0 ? "PANELTEST ... " : "") + text.substr(at, most));
      at += most;
   }
};

KnowYourSkyrim.sectionsText = function(sections)
{
   var parts = [];
   var s = 0;
   var i;
   while(s < sections.length)
   {
      i = 0;
      while(i < sections[s].lines.length)
      {
         parts.push(sections[s].head + ": " + KnowYourSkyrim.lineText(sections[s].lines[i]));
         i++;
      }
      s++;
   }
   return parts.join(" | ");
};
