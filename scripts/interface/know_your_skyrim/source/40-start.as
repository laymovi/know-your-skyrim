// The plugin loads this movie into SkyUI's inventory while the inventory sets itself up, so the menu may not be there
// on the first frame. The browser starts once the menu has its settings, its lists are laid out then
KnowYourSkyrim.findMenu = function()
{
   var menu = _root.Menu_mc;
   if(menu == undefined || menu.inventoryLists == undefined || menu.inventoryLists.itemList == undefined)
   {
      return false;
   }
   KnowYourSkyrim.attach(menu);
   return true;
};

KnowYourSkyrim.attach = function(menu)
{
   if(menu._config != undefined)
   {
      KnowYourSkyrim.start(menu);
      return undefined;
   }
   menu.knowYourSkyrimSetConfig = menu.setConfig;
   menu.setConfig = function(config)
   {
      this.knowYourSkyrimSetConfig(config);
      KnowYourSkyrim.start(this);
   };
};

// An inventory not made by SkyUI never gets the lists, the search gives up after a while there
if(!KnowYourSkyrim.findMenu())
{
   this.menuWait = KnowYourSkyrim.MENU_WAIT;
   this.onEnterFrame = function()
   {
      if(KnowYourSkyrim.findMenu() || --this.menuWait <= 0)
      {
         delete this.onEnterFrame;
      }
   };
}
