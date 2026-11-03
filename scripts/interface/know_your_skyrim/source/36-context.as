KnowYourSkyrim.CONTEXT_WIDE = 150;
KnowYourSkyrim.CONTEXT_LINE = 26;
KnowYourSkyrim.CONTEXT_PAD = 4;
KnowYourSkyrim.CONTEXT_ICON = 14;
KnowYourSkyrim.CONTEXT_DIVIDER = 9;
// the press that opened a menu reaches the watch of the mouse a moment later, it closes nothing
KnowYourSkyrim.CONTEXT_SETTLE = 250;

// A small menu at a point of the stage, the way the panel of Inventory Reforged opens one on a right click. A line is
// {key, words, icon, run}, {divider:true} is a thin line between two groups, chosen:true draws a line gold. A press
// on a line closes the menu and runs it, a press anywhere else, Esc or Tab close it
KnowYourSkyrim.openContext = function(lines, x, y)
{
   var layer = KnowYourSkyrim.aloneLayer();
   var pad = KnowYourSkyrim.CONTEXT_PAD;
   var wide = KnowYourSkyrim.CONTEXT_WIDE;
   var point = {x:x, y:y};
   var bottom = {x:0, y:KnowYourSkyrim.menu.bottomBar._y};
   var context;
   var made = [];
   var line;
   var y = pad;
   var i = 0;
   KnowYourSkyrim.closeContext();
   context = layer.createEmptyMovieClip("knowYourSkyrimContext", layer.getNextHighestDepth());
   while(i < lines.length)
   {
      if(lines[i].divider == true)
      {
         made.push({divider:y + KnowYourSkyrim.CONTEXT_DIVIDER / 2});
         y += KnowYourSkyrim.CONTEXT_DIVIDER;
         i++;
         continue;
      }
      line = context.createEmptyMovieClip("line" + i, i + 1);
      line._x = pad;
      line._y = y;
      line.what = lines[i];
      line.createTextField("touch", 0, 0, 0, 10, KnowYourSkyrim.CONTEXT_LINE);
      line.touch.selectable = false;
      line.createEmptyMovieClip("look", 1);
      KnowYourSkyrim.writeLine(line, "words", 2, lines[i].words, 14, KnowYourSkyrim.WHITE);
      line.words._x = 2 * pad + KnowYourSkyrim.CONTEXT_ICON + 8;
      line.words._y = (KnowYourSkyrim.CONTEXT_LINE - line.words._height) / 2;
      wide = Math.max(wide, line.words._x + line.words._width + 4 * pad);
      made.push(line);
      y += KnowYourSkyrim.CONTEXT_LINE;
      i++;
   }
   context.wide = wide;
   context.high = y + pad;
   KnowYourSkyrim.panelBox(context, wide, context.high, 100, 40);
   i = 0;
   while(i < made.length)
   {
      line = made[i];
      i++;
      if(line.divider != undefined)
      {
         context.lineStyle(1, KnowYourSkyrim.WHITE, 20, true);
         context.moveTo(2 * pad, line.divider);
         context.lineTo(wide - 2 * pad, line.divider);
         context.lineStyle();
         continue;
      }
      line.touch._width = wide - 2 * pad;
      KnowYourSkyrim.drawContextLine(line, false);
      line.onRollOver = function()
      {
         KnowYourSkyrim.drawContextLine(this, true);
      };
      line.onRollOut = line.onReleaseOutside = function()
      {
         KnowYourSkyrim.drawContextLine(this, false);
      };
      line.onPress = function()
      {
         KnowYourSkyrim.runContext(this.what);
      };
   }
   // kept on the screen above the bottom bar
   layer.globalToLocal(point);
   KnowYourSkyrim.menu.localToGlobal(bottom);
   layer.globalToLocal(bottom);
   context._x = Math.max(0, point.x);
   context._y = Math.max(0, Math.min(point.y, bottom.y - context.high));
   KnowYourSkyrim.context = context;
   KnowYourSkyrim.contextOpenedAt = getTimer();
   KnowYourSkyrim.watchContext();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.watchContext = function()
{
   if(KnowYourSkyrim.contextCatcher != undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.contextCatcher = {};
   KnowYourSkyrim.contextCatcher.onMouseDown = function()
   {
      var context = KnowYourSkyrim.context;
      if(context == undefined || getTimer() - KnowYourSkyrim.contextOpenedAt < KnowYourSkyrim.CONTEXT_SETTLE)
      {
         return undefined;
      }
      if(!context.hitTest(_root._xmouse, _root._ymouse, true))
      {
         KnowYourSkyrim.closeContext();
      }
   };
   Mouse.addListener(KnowYourSkyrim.contextCatcher);
};

KnowYourSkyrim.drawContextLine = function(line, lit)
{
   var look = line.look;
   var size = KnowYourSkyrim.CONTEXT_ICON;
   var wide = line._parent.wide - 2 * KnowYourSkyrim.CONTEXT_PAD;
   var gold = lit || line.what.chosen == true;
   look.clear();
   look.beginFill(lit ? KnowYourSkyrim.GOLD : 0, lit ? 20 : 0);
   KnowYourSkyrim.roundBox(look, 0, 0, wide, KnowYourSkyrim.CONTEXT_LINE, 3);
   look.endFill();
   if(lit)
   {
      look.beginFill(KnowYourSkyrim.GOLD, 100);
      KnowYourSkyrim.roundBox(look, 0, 4, 2, KnowYourSkyrim.CONTEXT_LINE - 8, 1);
      look.endFill();
   }
   if(line.what.icon != undefined)
   {
      KnowYourSkyrim.drawIcon(look, line.what.icon, KnowYourSkyrim.CONTEXT_PAD + 4, (KnowYourSkyrim.CONTEXT_LINE - size) / 2, size, gold ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 100);
   }
   line.words.textColor = gold ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE;
};

KnowYourSkyrim.runContext = function(what)
{
   KnowYourSkyrim.closeContext();
   if(what != undefined && what.run != undefined)
   {
      what.run();
   }
};

KnowYourSkyrim.closeContext = function()
{
   if(KnowYourSkyrim.context == undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.context.removeMovieClip();
   KnowYourSkyrim.context = undefined;
};

// While a menu is open the keys are its own: Esc and Tab close it, nothing else goes on to the browser or SkyUI
KnowYourSkyrim.contextKey = function(details)
{
   var code = details.navEquivalent;
   if(KnowYourSkyrim.context == undefined)
   {
      return false;
   }
   if(Shared.GlobalFunc.IsKeyPressed(details) && (code == gfx.ui.NavigationCode.ESCAPE || code == gfx.ui.NavigationCode.TAB))
   {
      KnowYourSkyrim.closeContext();
   }
   return true;
};

// For the test stand, the line of the open menu with this key is pressed
KnowYourSkyrim.pressContext = function(key)
{
   var context = KnowYourSkyrim.context;
   var found;
   for(var name in context)
   {
      if(context[name] != undefined && context[name].what != undefined && context[name].what.key == String(key))
      {
         found = context[name].what;
      }
   }
   KnowYourSkyrim.runContext(found);
};
