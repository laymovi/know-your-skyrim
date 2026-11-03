KnowYourSkyrim.FIND_WIDE = 150;
KnowYourSkyrim.FIND_COLOR = 0x8FD6FF;
KnowYourSkyrim.FIND_CURRENT = 0xFFFFFF;
KnowYourSkyrim.findAt = 0;
KnowYourSkyrim.findAll = false;

// Words found on the page of the entry, the way a browser finds them on a page: a field across the card under the tile,
// every place the words stand lit, Enter and the arrows walk from one to the next and the page scrolls there. While
// words are in the field every list of the page stands open, the words are found in all of it. The words stay for the
// next entry
KnowYourSkyrim.buildBrowserFind = function(details)
{
   var high = KnowYourSkyrim.BROWSER_INSPECT_HIGH;
   var wide = KnowYourSkyrim.FIND_WIDE;
   var box = details.createEmptyMovieClip("find", 10);
   var field;
   box.wide = wide;
   box.which = "find";
   box._x = details.wide - wide;
   box._visible = false;
   box.createEmptyMovieClip("look", 0);
   box.createTextField("field", 1, 18, 0, wide - 58, high);
   field = box.field;
   KnowYourSkyrim.styleText(field, KnowYourSkyrim.BROWSER_SMALL, "left", KnowYourSkyrim.WHITE);
   field.type = "input";
   field.selectable = true;
   field.noTranslate = true;
   KnowYourSkyrim.writeLine(box, "hint", 2, KnowYourSkyrim.say("Find on the page"), KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.HERO_FAINT);
   box.hint._x = 20;
   box.hint._y = Math.round((high - box.hint._height) / 2);
   box.hint.hitTestDisable = true;
   KnowYourSkyrim.writeLine(box, "count", 3, "", KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.HERO_DIM);
   box.count.hitTestDisable = true;
   box.createTextField("touch", 4, 0, 0, wide, high);
   KnowYourSkyrim.listenField(box);
   KnowYourSkyrim.drawFindBox(box);
};

// the field as wide as the card it stands in, its count at the right end
KnowYourSkyrim.sizeFindBox = function(box, wide)
{
   if(box.wide == wide)
   {
      return undefined;
   }
   box.wide = wide;
   box.field._width = wide - 58;
   box.touch._width = wide;
   KnowYourSkyrim.drawFindBox(box);
};

KnowYourSkyrim.drawFindBox = function(box)
{
   var high = KnowYourSkyrim.BROWSER_INSPECT_HIGH;
   var typing = KnowYourSkyrim.browserTyping == "find";
   var words = box.field.text.length > 0;
   box.look.clear();
   box.look.lineStyle(1, typing || words ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE, typing ? 90 : (words ? 60 : 30), true);
   box.look.beginFill(0, 55);
   KnowYourSkyrim.roundBox(box.look, 0, 0, box.wide, high, 4);
   box.look.endFill();
   box.look.lineStyle();
   KnowYourSkyrim.drawIcon(box.look, "lens", 3, 2, high - 4, typing || words ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 80);
   box.hint._visible = !typing && !words;
   box.count._x = box.wide - 4 - box.count._width;
   box.count._y = Math.round((high - box.count._height) / 2);
};

KnowYourSkyrim.findWords = function()
{
   var box = KnowYourSkyrim.browser.details.find;
   return box != undefined ? String(box.field.text) : "";
};

// The words changed, so the page is drawn again with every list open, or shut again for no words, the first place they
// stand is lit and the page goes there
KnowYourSkyrim.findInPage = function()
{
   var told = KnowYourSkyrim.browserItem;
   var all = KnowYourSkyrim.findWords().length > 0;
   KnowYourSkyrim.findAt = 0;
   if(all != KnowYourSkyrim.findAll)
   {
      KnowYourSkyrim.findAll = all;
      if(all && told != undefined && KnowYourSkyrim.browserSources != undefined && KnowYourSkyrim.browserSources.whole != true)
      {
         KnowYourSkyrim.browserSources = KnowYourSkyrim.plugin().BrowserSources(told.id, true);
      }
   }
   KnowYourSkyrim.drawBrowserPage(true);
   KnowYourSkyrim.markFound(true);
};

// every place the words stand on the page, in the order of the lines
KnowYourSkyrim.foundPlaces = function()
{
   var lines = KnowYourSkyrim.browser.details.page.lines;
   var words = KnowYourSkyrim.findWords().toLowerCase();
   var places = [];
   var f = 0;
   var text;
   var at;
   while(words.length > 0 && lines.fields != undefined && f < lines.fields.length)
   {
      text = String(lines.fields[f].text).toLowerCase();
      at = text.indexOf(words);
      while(at >= 0)
      {
         places.push({field:lines.fields[f], from:at, to:at + words.length});
         at = text.indexOf(words, at + words.length);
      }
      f++;
   }
   return places;
};

// The places lit, the one walked to brighter and underlined. jump moves the page so that one is in sight
KnowYourSkyrim.markFound = function(jump)
{
   var details = KnowYourSkyrim.browser.details;
   var box = details.find;
   var places = KnowYourSkyrim.foundPlaces();
   var lit = new TextFormat();
   var current = new TextFormat();
   var place;
   var i = 0;
   lit.color = KnowYourSkyrim.FIND_COLOR;
   lit.underline = false;
   current.color = KnowYourSkyrim.FIND_CURRENT;
   current.underline = true;
   if(places.length > 0)
   {
      KnowYourSkyrim.findAt = (KnowYourSkyrim.findAt % places.length + places.length) % places.length;
   }
   while(i < places.length)
   {
      place = places[i];
      place.field.setTextFormat(place.from, place.to, i == KnowYourSkyrim.findAt ? current : lit);
      i++;
   }
   box.count.text = KnowYourSkyrim.findWords().length == 0 ? "" : (places.length > 0 ? KnowYourSkyrim.say("#1/#2", KnowYourSkyrim.findAt + 1, places.length) : "0");
   KnowYourSkyrim.drawFindBox(box);
   place = places[KnowYourSkyrim.findAt];
   if(jump && place != undefined)
   {
      details.page.first = Math.max(0, Math.floor(place.field._y / KnowYourSkyrim.BROWSER_PAGE_STEP) - 1);
      KnowYourSkyrim.scrollBrowserPage(0);
   }
};

// the lit places stay lit, only which one is walked to changes
KnowYourSkyrim.findNext = function(by)
{
   KnowYourSkyrim.findAt += by;
   KnowYourSkyrim.markFound(true);
};

// the page is drawn anew a moment after the last key, because with every list open it can be long
KnowYourSkyrim.findTyped = function()
{
   clearInterval(KnowYourSkyrim.findTimer);
   KnowYourSkyrim.findTimer = setInterval(function()
   {
      clearInterval(KnowYourSkyrim.findTimer);
      KnowYourSkyrim.findInPage();
   }, KnowYourSkyrim.BROWSER_DELAY);
};
