// The entry of the details looked at from all sides: one item big in the middle of a dark screen, the way the panel
// of Inventory Reforged shows it, with its words and its cross. A drag turns it, the wheel brings it closer, a right
// click, Esc, Tab or the cross close the view. The plugin draws it, every other model of the menu steps out meanwhile
KnowYourSkyrim.buildInspect = function()
{
   KnowYourSkyrim.inspecting = false;
   KnowYourSkyrim.inspectWheel = {};
   KnowYourSkyrim.inspectWheel.onMouseWheel = function(delta)
   {
      if(KnowYourSkyrim.inspecting)
      {
         KnowYourSkyrim.plugin().InspectZoom(delta > 0 ? KnowYourSkyrim.INSPECT_ZOOM : 1 / KnowYourSkyrim.INSPECT_ZOOM);
      }
   };
   Mouse.addListener(KnowYourSkyrim.inspectWheel);
};

// The lists don't answer while the view is up, as while a search is typed, because R or E of the game would drop or
// equip the row selected behind it. The panel's keys keep still too
KnowYourSkyrim.inspectBrowserItem = function()
{
   var told = KnowYourSkyrim.browserItem;
   var lists = KnowYourSkyrim.menu.inventoryLists;
   var viewer;
   KnowYourSkyrim.endBrowserTyping(false);
   if(told == undefined || told.living == true || KnowYourSkyrim.signOnly(told) || !KnowYourSkyrim.listsAnswer())
   {
      return undefined;
   }
   if(KnowYourSkyrim.plugin().Inspect(told.id, 0.5, KnowYourSkyrim.INSPECT_MIDDLE_Y, KnowYourSkyrim.INSPECT_SIZE) != true)
   {
      return undefined;
   }
   lists.itemList.disableInput = lists.itemList.disableSelection = lists.categoryList.disableInput = true;
   if(KnowYourSkyrim.viewer == undefined)
   {
      KnowYourSkyrim.buildViewer();
   }
   viewer = KnowYourSkyrim.viewer;
   // over everything of the menu, the panel's windows made after the viewer too
   viewer.swapDepths(KnowYourSkyrim.menu.getNextHighestDepth());
   KnowYourSkyrim.inspecting = true;
   KnowYourSkyrim.tellPointerArea();
   viewer.title.text = KnowYourSkyrim.gameName(told.name);
   viewer.title._x = (KnowYourSkyrim.screenRect().width - viewer.title._width) / 2 + KnowYourSkyrim.screenRect().x;
   viewer._visible = true;
   KnowYourSkyrim.padViewerHint();
   KnowYourSkyrim.padTurns();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.leaveInspect = function()
{
   var lists = KnowYourSkyrim.menu.inventoryLists;
   if(!KnowYourSkyrim.inspecting)
   {
      return undefined;
   }
   lists.itemList.disableInput = lists.itemList.disableSelection = lists.categoryList.disableInput = false;
   KnowYourSkyrim.inspecting = false;
   KnowYourSkyrim.tellPointerArea();
   KnowYourSkyrim.inspectTurning = false;
   KnowYourSkyrim.viewer._visible = false;
   KnowYourSkyrim.plugin().InspectEnd();
   KnowYourSkyrim.padTurns();
   KnowYourSkyrim.padHintsNow();
};

// Tab, Esc and Shift+Tab close the view, nothing else takes a key while it's up
KnowYourSkyrim.inspectKey = function(details)
{
   var code = details.navEquivalent;
   if(!KnowYourSkyrim.inspecting)
   {
      return false;
   }
   if(Shared.GlobalFunc.IsKeyPressed(details) && (code == gfx.ui.NavigationCode.TAB || code == gfx.ui.NavigationCode.SHIFT_TAB || code == gfx.ui.NavigationCode.ESCAPE))
   {
      KnowYourSkyrim.leaveInspect();
   }
   return true;
};

KnowYourSkyrim.buildViewer = function()
{
   var menu = KnowYourSkyrim.menu;
   var screen = KnowYourSkyrim.screenRect();
   var safe = KnowYourSkyrim.safeInset();
   var viewer = menu.createEmptyMovieClip("knowYourSkyrimViewer", menu.getNextHighestDepth());
   var hint;
   var close;
   viewer._visible = false;
   // on a stage the game stretches the view is stretched back, the same as the browser's layer
   viewer._xscale = 100 / KnowYourSkyrim.squash;
   viewer.createTextField("touch", 0, screen.x, screen.y, screen.width, screen.height);
   viewer.touch.selectable = false;
   viewer.beginFill(0, KnowYourSkyrim.INSPECT_SHADE);
   viewer.moveTo(screen.x, screen.y);
   viewer.lineTo(screen.x + screen.width, screen.y);
   viewer.lineTo(screen.x + screen.width, screen.y + screen.height);
   viewer.lineTo(screen.x, screen.y + screen.height);
   viewer.lineTo(screen.x, screen.y);
   viewer.endFill();
   KnowYourSkyrim.writeLine(viewer, "title", 1, "", KnowYourSkyrim.INSPECT_TITLE, KnowYourSkyrim.WHITE);
   viewer.title._y = screen.y + safe.y + 24;
   hint = KnowYourSkyrim.writeLine(viewer, "hint", 2, KnowYourSkyrim.INSPECT_HINT, 15, KnowYourSkyrim.PARCHMENT);
   hint._x = screen.x + (screen.width - hint._width) / 2;
   hint._y = screen.y + screen.height - safe.y - 54;
   hint._alpha = 80;
   close = viewer.createEmptyMovieClip("close", 3);
   close.createTextField("touch", 0, 0, 0, KnowYourSkyrim.CLOSE_SIZE, KnowYourSkyrim.CLOSE_SIZE);
   close.touch.selectable = false;
   close._x = screen.x + screen.width - safe.x - KnowYourSkyrim.CLOSE_SIZE - 24;
   close._y = screen.y + safe.y + 20;
   KnowYourSkyrim.drawClose(close, false);
   // the viewer takes the presses of the cross inside it, so it looks whether a press landed on the cross
   viewer.onPress = function()
   {
      if(this.close.hitTest(_root._xmouse, _root._ymouse, true))
      {
         KnowYourSkyrim.leaveInspect();
         return undefined;
      }
      KnowYourSkyrim.inspectTurning = true;
      KnowYourSkyrim.inspectFrom = {x:_root._xmouse, y:_root._ymouse};
   };
   viewer.onRelease = viewer.onReleaseOutside = function()
   {
      KnowYourSkyrim.inspectTurning = false;
   };
   viewer.onPressAux = function()
   {
      KnowYourSkyrim.leaveInspect();
   };
   viewer.onMouseMove = function()
   {
      var from = KnowYourSkyrim.inspectFrom;
      var lit;
      if(!KnowYourSkyrim.inspecting)
      {
         return undefined;
      }
      lit = this.close.hitTest(_root._xmouse, _root._ymouse, true);
      if(lit != this.close.lit)
      {
         this.close.lit = lit;
         KnowYourSkyrim.drawClose(this.close, lit);
      }
      if(!KnowYourSkyrim.inspectTurning)
      {
         return undefined;
      }
      KnowYourSkyrim.plugin().InspectTurn((_root._xmouse - from.x) * KnowYourSkyrim.squash * KnowYourSkyrim.INSPECT_TURN, (_root._ymouse - from.y) * KnowYourSkyrim.INSPECT_TURN);
      KnowYourSkyrim.inspectFrom = {x:_root._xmouse, y:_root._ymouse};
   };
   KnowYourSkyrim.viewer = viewer;
};
