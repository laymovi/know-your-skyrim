// The button sits in the bottom bar after the hints of SkyUI and follows them, SkyUI lays them out anew for every
// row. A menu with no such hints gets it at the foot of the browser's room. With Inventory Reforged the panel's look
// button stands right after it
KnowYourSkyrim.buildBrowseButton = function()
{
   var bar = KnowYourSkyrim.menu.bottomBar;
   var hints = bar.buttonPanel;
   // Nordic UI's bottom bar has a class that doesn't come from MovieClip, its own calls are taken from MovieClip then
   var make = bar.createEmptyMovieClip != undefined ? bar.createEmptyMovieClip : MovieClip.prototype.createEmptyMovieClip;
   var depth = bar.getNextHighestDepth != undefined ? bar.getNextHighestDepth() : MovieClip.prototype.getNextHighestDepth.call(bar);
   var button = make.call(bar, "knowYourSkyrimButton", depth);
   button.createTextField("touch", 0, 0, 0, 10, 10);
   button.touch.selectable = false;
   button.createEmptyMovieClip("look", 1);
   button.createEmptyMovieClip("sign", 2);
   button.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawBrowseButton();
   };
   button.onRollOut = button.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawBrowseButton();
   };
   button.onPress = function()
   {
      KnowYourSkyrim.toggleBrowser();
   };
   button.onPressAux = function()
   {
   };
   KnowYourSkyrim.browseButton = button;
   if(hints != undefined && hints.doUpdateButtons != undefined)
   {
      hints.knowYourSkyrimUpdate = hints.doUpdateButtons;
      hints.doUpdateButtons = function()
      {
         this.knowYourSkyrimUpdate();
         KnowYourSkyrim.placeBrowseButton();
      };
   }
   KnowYourSkyrim.drawBrowseButton();
};

// A see-through box with a clear gold edge and the eye that watches everything, gold while the browser is open
KnowYourSkyrim.drawBrowseButton = function()
{
   var button = KnowYourSkyrim.browseButton;
   var high = KnowYourSkyrim.BROWSER_BUTTON_HIGH;
   var wide = KnowYourSkyrim.BROWSER_BUTTON_WIDE;
   var sign = KnowYourSkyrim.BROWSER_BUTTON_SIGN;
   var lit;
   if(button == undefined)
   {
      return undefined;
   }
   lit = button.lit || KnowYourSkyrim.browsing;
   button.gold = lit;
   button.wide = wide;
   button.touch._width = wide;
   button.touch._height = high;
   button.look.clear();
   button.look.lineStyle(1, KnowYourSkyrim.accentInk(), lit ? 100 : 75, true);
   KnowYourSkyrim.buttonFill(button.look, lit ? 62 : 42);
   KnowYourSkyrim.roundBox(button.look, 0, 0, wide, high, 5);
   button.look.endFill();
   button.look.lineStyle();
   button.sign.clear();
   KnowYourSkyrim.drawIcon(button.sign, "watch", Math.round((wide - sign) / 2), Math.round((high - sign) / 2), sign, lit ? KnowYourSkyrim.accentInk() : KnowYourSkyrim.inkOf("sign"), 100);
   KnowYourSkyrim.placeBrowseButton();
};

KnowYourSkyrim.placeBrowseButton = function()
{
   var bar = KnowYourSkyrim.menu.bottomBar;
   var button = KnowYourSkyrim.browseButton;
   var panel = KnowYourSkyrim.panel;
   // the look button of the panel goes right after this one
   var after = panel != undefined ? panel.lookRoom() : 0;
   var spot = KnowYourSkyrim.afterHints(KnowYourSkyrim.BROWSER_BUTTON_HIGH);
   var room;
   var corner;
   if(button == undefined)
   {
      return undefined;
   }
   if(spot == undefined)
   {
      room = KnowYourSkyrim.room();
      corner = {x:room != undefined ? room.right : 0, y:0};
      KnowYourSkyrim.roomClip().localToGlobal(corner);
      (bar.globalToLocal != undefined ? bar.globalToLocal : MovieClip.prototype.globalToLocal).call(bar, corner);
      button._x = Math.round(corner.x - button.wide - after);
      button._y = -KnowYourSkyrim.BROWSER_BUTTON_HIGH - 4;
   }
   else
   {
      button._x = spot.x;
      button._y = spot.y;
   }
   if(panel != undefined)
   {
      panel.followBarButton(button);
   }
};
