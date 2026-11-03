// The tabs of the categories with how many entries the words leave in each, the chosen one gold and underlined. Too
// narrow a browser keeps the signs and drops the counts
KnowYourSkyrim.drawBrowserTabs = function(page)
{
   var browser = KnowYourSkyrim.browser;
   var tabs = browser.tabs;
   var families = KnowYourSkyrim.BROWSER_FAMILIES;
   var room = browser.wide - 2 * KnowYourSkyrim.BROWSER_PAD;
   var withCounts = true;
   var x;
   var i;
   var tab;
   var family;
   var count;
   while(true)
   {
      i = 0;
      x = KnowYourSkyrim.BROWSER_PAD;
      while(i < families.length)
      {
         family = families[i];
         tab = tabs.createEmptyMovieClip("tab" + i, i);
         tab.family = family.key;
         tab._x = x;
         tab.createTextField("touch", 0, 0, 0, 10, KnowYourSkyrim.BROWSER_STRIP);
         tab.touch.selectable = false;
         tab.createEmptyMovieClip("sign", 1);
         count = page != undefined ? (family.key == "" ? page.all : page.families[family.key]) : undefined;
         KnowYourSkyrim.writeLine(tab, "count", 2, withCounts && count != undefined ? String(count) : "", KnowYourSkyrim.BROWSER_SMALL, KnowYourSkyrim.WHITE);
         tab.count._x = KnowYourSkyrim.CHIP_ICON + 4;
         tab.count._y = Math.round((KnowYourSkyrim.BROWSER_STRIP - tab.count._height) / 2);
         tab.wide = withCounts ? tab.count._x + tab.count._width : KnowYourSkyrim.CHIP_ICON;
         tab.empty = count == 0;
         tab.touch._width = tab.wide;
         tab.onRollOver = function()
         {
            this.lit = true;
            KnowYourSkyrim.drawBrowserTab(this);
         };
         tab.onRollOut = tab.onReleaseOutside = function()
         {
            this.lit = false;
            KnowYourSkyrim.drawBrowserTab(this);
         };
         tab.onPress = function()
         {
            KnowYourSkyrim.chooseBrowserFamily(this.family);
         };
         KnowYourSkyrim.drawBrowserTab(tab);
         x += tab.wide + KnowYourSkyrim.BROWSER_TAB_GAP;
         i++;
      }
      if(!withCounts || x - KnowYourSkyrim.BROWSER_TAB_GAP <= KnowYourSkyrim.BROWSER_PAD + room)
      {
         return undefined;
      }
      withCounts = false;
   }
};

KnowYourSkyrim.drawBrowserTab = function(tab)
{
   var chosen = tab.family == KnowYourSkyrim.browserState.family;
   var color = chosen ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE;
   var alpha = chosen ? 100 : (tab.lit ? KnowYourSkyrim.CHIP_LIT : (tab.empty ? 22 : KnowYourSkyrim.CHIP_DIM));
   var family = KnowYourSkyrim.browserFamily(tab.family);
   tab.sign.clear();
   tab.sign.beginFill(0, 0);
   KnowYourSkyrim.roundBox(tab.sign, 0, 0, tab.wide, KnowYourSkyrim.BROWSER_STRIP, 0);
   tab.sign.endFill();
   KnowYourSkyrim.drawIcon(tab.sign, family.icon, 0, (KnowYourSkyrim.BROWSER_STRIP - KnowYourSkyrim.CHIP_ICON) / 2, KnowYourSkyrim.CHIP_ICON, color, alpha);
   tab.count.textColor = color;
   tab.count._alpha = alpha;
   if(chosen)
   {
      tab.sign.lineStyle(2, KnowYourSkyrim.GOLD, 85, true);
      tab.sign.moveTo(0, KnowYourSkyrim.BROWSER_STRIP - 2);
      tab.sign.lineTo(tab.wide, KnowYourSkyrim.BROWSER_STRIP - 2);
      tab.sign.lineStyle();
   }
};

// Papyrus hands a key over in the case the game met it first ("Armor" for "armor"), so case doesn't count
KnowYourSkyrim.browserFamily = function(key)
{
   var asked = String(key).toLowerCase();
   var i = 0;
   while(i < KnowYourSkyrim.BROWSER_FAMILIES.length)
   {
      if(KnowYourSkyrim.BROWSER_FAMILIES[i].key.toLowerCase() == asked)
      {
         return KnowYourSkyrim.BROWSER_FAMILIES[i];
      }
      i++;
   }
   return KnowYourSkyrim.BROWSER_FAMILIES[0];
};

KnowYourSkyrim.chooseBrowserFamily = function(told)
{
   var state = KnowYourSkyrim.browserState;
   var key = KnowYourSkyrim.browserFamily(told).key;
   if(state.family == key)
   {
      return undefined;
   }
   state.family = key;
   state.chips = [];
   state.skill = undefined;
   // the browser is laid again when the chips of this category take another number of lines
   if(KnowYourSkyrim.chipLinesOf(key) != KnowYourSkyrim.browser.chipLines)
   {
      KnowYourSkyrim.relayBrowser();
   }
   KnowYourSkyrim.findBrowser(0, true);
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// How many lines the chips of a category take. One while they fit it nearly at full size, a line for each group when
// they would shrink more (the twelve of Living are too small to read on one)
KnowYourSkyrim.chipLinesOf = function(key)
{
   var browser = KnowYourSkyrim.browser;
   var rows = KnowYourSkyrim.nameChipsOf(key);
   var probe;
   var x;
   var g = 0;
   var c;
   if(rows == undefined || rows.length < 2 || browser == undefined)
   {
      return 1;
   }
   probe = browser.createEmptyMovieClip("chipProbe", 90);
   x = KnowYourSkyrim.BROWSER_PAD + KnowYourSkyrim.writeLine(probe, "name", 0, KnowYourSkyrim.translated(KnowYourSkyrim.browserFamily(key)).toUpperCase(), KnowYourSkyrim.CHIP_TEXT, KnowYourSkyrim.GOLD)._width + KnowYourSkyrim.CHIP_GAP;
   while(g < rows.length)
   {
      c = 0;
      while(c < rows[g].length)
      {
         x += KnowYourSkyrim.CHIP_ICON + KnowYourSkyrim.ICON_TO_WORD + KnowYourSkyrim.writeLine(probe, "words", 1, KnowYourSkyrim.translated(rows[g][c]).toUpperCase(), KnowYourSkyrim.CHIP_TEXT, KnowYourSkyrim.WHITE)._width + KnowYourSkyrim.CHIP_GAP;
         c++;
      }
      g++;
   }
   probe.removeMovieClip();
   return (browser.wide - KnowYourSkyrim.BROWSER_PAD) / x < KnowYourSkyrim.CHIP_LINE_FIT ? Math.min(rows.length, KnowYourSkyrim.CHIP_LINES_MOST) : 1;
};

// The name of the category in gold, then its chips as the strip of the list has them, a group is one choice. When the
// browser laid more lines for them every group after the first starts a line of its own, under the first chip. With
// Trainers chosen a chip of the skill follows it
KnowYourSkyrim.drawBrowserChips = function()
{
   var browser = KnowYourSkyrim.browser;
   var holder = browser.chips.createEmptyMovieClip("row", 0);
   var state = KnowYourSkyrim.browserState;
   var family = KnowYourSkyrim.browserFamily(state.family);
   var rows = KnowYourSkyrim.nameChipsOf(family.key);
   var name = KnowYourSkyrim.writeLine(holder, "name", 0, KnowYourSkyrim.translated(family).toUpperCase(), KnowYourSkyrim.CHIP_TEXT, KnowYourSkyrim.GOLD);
   var start = KnowYourSkyrim.BROWSER_PAD + name._width + KnowYourSkyrim.CHIP_GAP;
   var x = start;
   var widest = 0;
   var line = 0;
   var depth = 1;
   var g = 0;
   var c;
   var chip;
   name._x = KnowYourSkyrim.BROWSER_PAD;
   name._y = Math.round((KnowYourSkyrim.BROWSER_STRIP - name._height) / 2);
   while(rows != undefined && g < rows.length)
   {
      if(g > 0 && line < browser.chipLines - 1)
      {
         widest = Math.max(widest, x);
         x = start;
         line++;
      }
      c = 0;
      while(c < rows[g].length)
      {
         chip = KnowYourSkyrim.makeBrowserChip(holder, rows[g][c], g, x, depth);
         chip._y = line * KnowYourSkyrim.BROWSER_STRIP;
         x += chip.wide + KnowYourSkyrim.CHIP_GAP;
         depth++;
         c++;
      }
      g++;
   }
   if(KnowYourSkyrim.has(state.chips, "trainers"))
   {
      chip = KnowYourSkyrim.makeSkillChip(holder, x, depth);
      chip._y = line * KnowYourSkyrim.BROWSER_STRIP;
      x += chip.wide + KnowYourSkyrim.CHIP_GAP;
   }
   widest = Math.max(widest, x);
   // the chips of a family with many get smaller to fit the width of the browser
   holder._xscale = holder._yscale = Math.min(100, (browser.wide - KnowYourSkyrim.BROWSER_PAD) / Math.max(widest, 1) * 100);
};

// The skill of the trainers: Any skill, or the one picked from the menu the chip opens, the menu lists only the skills
// someone of the load order trains
KnowYourSkyrim.makeSkillChip = function(holder, x, depth)
{
   var state = KnowYourSkyrim.browserState;
   var picked = KnowYourSkyrim.trainerSkill(state.skill);
   var chip = KnowYourSkyrim.makeBrowserChip(holder, {key:"skill", words:"", fallback:picked != undefined ? picked.name : KnowYourSkyrim.say("Any skill"), icon:"tome"}, -1, x, depth);
   chip.chosen = picked != undefined;
   KnowYourSkyrim.drawChip(chip, false);
   chip.onPress = function()
   {
      KnowYourSkyrim.openSkillMenu(this);
   };
   return chip;
};

// the skills trainers of the load order train, asked of the plugin once, the index stays for the game
KnowYourSkyrim.trainerSkills = function()
{
   if(KnowYourSkyrim.trainerSkillsKnown == undefined)
   {
      KnowYourSkyrim.trainerSkillsKnown = KnowYourSkyrim.plugin().BrowserTrainerSkills();
   }
   return KnowYourSkyrim.trainerSkillsKnown != undefined ? KnowYourSkyrim.trainerSkillsKnown : [];
};

KnowYourSkyrim.trainerSkill = function(skill)
{
   var skills = KnowYourSkyrim.trainerSkills();
   var i = 0;
   while(skill != undefined && i < skills.length)
   {
      if(skills[i].skill == skill)
      {
         return skills[i];
      }
      i++;
   }
   return undefined;
};

// The menu of the skill chip under it: Any skill, then the skills by the game's three schools, a thin line between
KnowYourSkyrim.openSkillMenu = function(chip)
{
   var skills = KnowYourSkyrim.trainerSkills();
   var state = KnowYourSkyrim.browserState;
   var lines = [{key:"any", words:KnowYourSkyrim.say("Any skill"), icon:"tome", chosen:state.skill == undefined, run:function()
   {
      KnowYourSkyrim.pickTrainerSkill(undefined);
   }}];
   var point = {x:0, y:KnowYourSkyrim.BROWSER_STRIP};
   var school = -1;
   var i = 0;
   while(i < skills.length)
   {
      if(KnowYourSkyrim.skillSchool(skills[i].skill) != school)
      {
         lines.push({divider:true});
         school = KnowYourSkyrim.skillSchool(skills[i].skill);
      }
      lines.push({key:"skill" + skills[i].skill, words:KnowYourSkyrim.say("#1 (#2)", skills[i].name, skills[i].count), chosen:state.skill == skills[i].skill, skill:skills[i].skill, run:function()
      {
         KnowYourSkyrim.pickTrainerSkill(this.skill);
      }});
      i++;
   }
   chip.localToGlobal(point);
   KnowYourSkyrim.openContext(lines, point.x, point.y);
};

// the game keeps the skills in order, six of the warrior, six of the thief, six of the mage
KnowYourSkyrim.skillSchool = function(skill)
{
   return Math.floor((Number(skill) - KnowYourSkyrim.FIRST_SKILL) / KnowYourSkyrim.SKILLS_A_SCHOOL);
};

KnowYourSkyrim.pickTrainerSkill = function(skill)
{
   KnowYourSkyrim.browserState.skill = skill != undefined ? Number(skill) : undefined;
   KnowYourSkyrim.findBrowser(0, true);
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

KnowYourSkyrim.makeBrowserChip = function(holder, told, group, x, depth)
{
   var chip = holder.createEmptyMovieClip("chip" + depth, depth);
   var text = KnowYourSkyrim.writeLine(chip, "words", 1, KnowYourSkyrim.translated(told).toUpperCase(), KnowYourSkyrim.CHIP_TEXT, KnowYourSkyrim.WHITE);
   chip.createEmptyMovieClip("sign", 2);
   KnowYourSkyrim.drawIcon(chip.sign, told.icon, 0, (KnowYourSkyrim.BROWSER_STRIP - KnowYourSkyrim.CHIP_ICON) / 2, KnowYourSkyrim.CHIP_ICON, told.tint != undefined ? told.tint : KnowYourSkyrim.WHITE, 100);
   text._x = KnowYourSkyrim.CHIP_ICON + KnowYourSkyrim.ICON_TO_WORD;
   text._y = (KnowYourSkyrim.BROWSER_STRIP - text._height) / 2;
   chip._x = x;
   chip.wide = text._x + text._width;
   chip.key = told.key;
   chip.group = group;
   chip.tinted = told.tint != undefined;
   chip.color = KnowYourSkyrim.WHITE;
   chip.chosen = KnowYourSkyrim.browserState.chips[group] == told.key;
   KnowYourSkyrim.drawChip(chip, false);
   chip.onRollOver = function()
   {
      KnowYourSkyrim.drawChip(this, true);
   };
   chip.onRollOut = chip.onReleaseOutside = function()
   {
      KnowYourSkyrim.drawChip(this, false);
   };
   chip.onPress = function()
   {
      KnowYourSkyrim.pressBrowserChip(this.key, this.group);
   };
   return chip;
};

// The chip of the place the player stands in filters nothing, it opens that place's page
KnowYourSkyrim.showPlayersPlace = function()
{
   var id = KnowYourSkyrim.plugin().BrowserHere();
   if(id > 0)
   {
      KnowYourSkyrim.goBrowserItem(id);
   }
   else
   {
      KnowYourSkyrim.tellBrowser(KnowYourSkyrim.say("This place has no page"));
      gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
   }
};

KnowYourSkyrim.pressBrowserChip = function(key, group)
{
   var rows;
   if(String(key).toLowerCase() == "herenow")
   {
      KnowYourSkyrim.showPlayersPlace();
      return;
   }
   rows = KnowYourSkyrim.CHIP_ROWS[KnowYourSkyrim.browserFamily(KnowYourSkyrim.browserState.family).key];
   KnowYourSkyrim.pickChip(KnowYourSkyrim.browserState.chips, rows, Number(group), KnowYourSkyrim.groupChipKey(rows, Number(group), key));
   KnowYourSkyrim.keepSkillWithTrainers();
   KnowYourSkyrim.findBrowser(0, true);
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// the key of a chip of the group as the chip has it, a key from the test stand comes in the case the game met it first
KnowYourSkyrim.groupChipKey = function(rows, group, key)
{
   var asked = String(key).toLowerCase();
   var c = 0;
   while(rows != undefined && rows[group] != undefined && c < rows[group].length)
   {
      if(rows[group][c].key.toLowerCase() == asked)
      {
         return rows[group][c].key;
      }
      c++;
   }
   return key;
};

// A row is the sign of its kind, the name with how many variants it has, and the mod it comes from on the
// right. They're made once for the room and filled again for every page
KnowYourSkyrim.buildBrowserRows = function(browser, x, y, wide, high)
{
   var rows = browser.createEmptyMovieClip("rows", 5);
   var count = KnowYourSkyrim.browserRowsFit(high);
   var row;
   var i = 0;
   rows._x = x;
   rows._y = y;
   rows.wide = wide - KnowYourSkyrim.BROWSER_BAR - 6;
   rows.high = high;
   rows.count = count;
   rows.createEmptyMovieClip("view", 0);
   rows.view.beginFill(0, 0);
   KnowYourSkyrim.roundBox(rows.view, 0, 0, wide, high, 0);
   rows.view.endFill();
   while(i < count)
   {
      row = rows.createEmptyMovieClip("row" + i, i + 1);
      KnowYourSkyrim.buildBrowserRow(row, rows.wide);
      row._y = i * KnowYourSkyrim.BROWSER_ROW;
      i++;
   }
   KnowYourSkyrim.writeLine(rows, "none", count + 1, KnowYourSkyrim.say("Nothing found"), KnowYourSkyrim.BROWSER_TEXT, KnowYourSkyrim.HERO_DIM);
   rows.none._x = 6;
   rows.none._y = 4;
   rows.createEmptyMovieClip("bar", count + 2);
   rows.bar._x = wide - KnowYourSkyrim.BROWSER_BAR;
};

KnowYourSkyrim.browserRowsFit = function(high)
{
   return Math.max(1, Math.floor(high / KnowYourSkyrim.BROWSER_ROW));
};

KnowYourSkyrim.buildBrowserRow = function(row, wide)
{
   var high = KnowYourSkyrim.BROWSER_ROW;
   var name;
   var mod;
   var modWide = Math.round(wide * KnowYourSkyrim.BROWSER_MOD_SHARE);
   row.createTextField("touch", 0, 0, 0, wide, high);
   row.touch.selectable = false;
   row.createEmptyMovieClip("light", 1);
   row.createEmptyMovieClip("sign", 2);
   row.createTextField("words", 3, KnowYourSkyrim.BROWSER_ICON + 10, 0, wide - modWide - KnowYourSkyrim.BROWSER_ICON - 16, high);
   row.createEmptyMovieClip("mark", 5);
   name = row.words;
   row.wordsWide = name._width;
   KnowYourSkyrim.styleText(name, KnowYourSkyrim.BROWSER_TEXT, "left", KnowYourSkyrim.WHITE);
   row.createTextField("mod", 4, wide - modWide, 0, modWide - 4, high);
   mod = row.mod;
   KnowYourSkyrim.styleText(mod, KnowYourSkyrim.BROWSER_SMALL, "right", KnowYourSkyrim.HERO_DIM);
   name._y = Math.round((high - 20) / 2);
   mod._y = Math.round((high - 18) / 2);
   row.wide = wide;
   row.onRollOver = function()
   {
      this.lit = true;
      KnowYourSkyrim.drawBrowserRow(this);
   };
   row.onRollOut = row.onReleaseOutside = function()
   {
      this.lit = false;
      KnowYourSkyrim.drawBrowserRow(this);
   };
   row.onPress = function()
   {
      KnowYourSkyrim.pressBrowserRow(this);
   };
   row.onPressAux = function()
   {
      KnowYourSkyrim.browserBack();
   };
};

KnowYourSkyrim.drawBrowserRow = function(row)
{
   var chosen = row.item != undefined && row.item.id == KnowYourSkyrim.browserState.selected;
   var plain = row.words.text.length - (row.more != undefined ? row.more.length : 0);
   var format;
   row.light.clear();
   if(row.item == undefined)
   {
      return undefined;
   }
   if(chosen || row.lit)
   {
      row.light.beginFill(chosen ? KnowYourSkyrim.GOLD : KnowYourSkyrim.WHITE, chosen ? 16 : 8);
      KnowYourSkyrim.roundBox(row.light, 0, 1, row.wide, KnowYourSkyrim.BROWSER_ROW - 2, 3);
      row.light.endFill();
   }
   // a quest behind the player goes dim
   row.words.textColor = chosen ? KnowYourSkyrim.GOLD : (row.item.state == 3 || row.item.state == 4 ? KnowYourSkyrim.HERO_DIM : KnowYourSkyrim.WHITE);
   // the count of variants after the name stays dim
   if(row.more != undefined && row.more.length > 0)
   {
      format = new TextFormat();
      format.color = KnowYourSkyrim.HERO_DIM;
      format.size = KnowYourSkyrim.BROWSER_SMALL;
      row.words.setTextFormat(plain, row.words.text.length, format);
   }
};

KnowYourSkyrim.markGuard = function(row, guard)
{
   row.mark.clear();
   if(guard > 0)
   {
      KnowYourSkyrim.drawIcon(row.mark, "ward", row.words._x + row.words.textWidth + 6, (KnowYourSkyrim.BROWSER_ROW - KnowYourSkyrim.BROWSER_MARK) / 2, KnowYourSkyrim.BROWSER_MARK, KnowYourSkyrim.BROWSER_GUARD, guard == KnowYourSkyrim.ESSENTIAL ? 95 : 45);
   }
};

// what stands dim after the name of a row: how many variants an original has, how far the player is with a quest,
// the place a place lies in, places of one name are told apart by it, the rank of a perk the player has
KnowYourSkyrim.rowMore = function(item)
{
   if(item.variants > 0)
   {
      return "  +" + item.variants;
   }
   if(item.around != undefined && item.around != "")
   {
      return "  " + KnowYourSkyrim.say("in #", item.around);
   }
   if(item.rank > 0)
   {
      return "  " + (item.ranks > 1 ? KnowYourSkyrim.say("#1/#2", item.rank, item.ranks) : KnowYourSkyrim.say("yours"));
   }
   return item.state != undefined && item.state > 0 ? "  " + KnowYourSkyrim.questState(item.state) : "";
};

// Rows of one name and one mod say what tells them apart: their number when it differs (damage, armor, value), the kind
// of a place (a hold and its city share a name), their form id when even that is the same. A mod often has a piece in
// several looks or levels under one name
KnowYourSkyrim.twinTags = function(rows)
{
   var tags = [];
   var i = 0;
   var j;
   var twins;
   var differ;
   var kinds;
   while(i < rows.length)
   {
      twins = 0;
      differ = false;
      kinds = false;
      j = 0;
      while(j < rows.length)
      {
         if(j != i && String(rows[j].name).toLowerCase() == String(rows[i].name).toLowerCase() && rows[j].mod == rows[i].mod)
         {
            twins++;
            differ = differ || rows[j].number != rows[i].number;
            kinds = kinds || rows[j].placeKind != rows[i].placeKind;
         }
         j++;
      }
      if(twins > 0)
      {
         if(differ && rows[i].number != undefined)
         {
            tags[i] = KnowYourSkyrim.numberTag(rows[i]);
         }
         else
         {
            tags[i] = kinds && rows[i].placeKind != undefined ? KnowYourSkyrim.placeWord(rows[i].placeKind) : KnowYourSkyrim.formHex(rows[i].id);
         }
      }
      i++;
   }
   return tags;
};

// one place of a kind, as the chips of the places name the kinds
KnowYourSkyrim.placeWord = function(kind)
{
   switch(String(kind))
   {
      case "holds":
         return KnowYourSkyrim.say("hold");
      case "towns":
         return KnowYourSkyrim.say("town");
      case "shops":
         return KnowYourSkyrim.say("shop");
      case "homes":
         return KnowYourSkyrim.say("home");
      case "dungeons":
         return KnowYourSkyrim.say("dungeon");
   }
   return KnowYourSkyrim.say("place");
};

KnowYourSkyrim.numberTag = function(row)
{
   var shown = KnowYourSkyrim.fixed(row.number, 0);
   if(row.numberKind == "damage")
   {
      return KnowYourSkyrim.say("Damage #", shown);
   }
   return row.numberKind == "armor" ? KnowYourSkyrim.say("Armor #", shown) : KnowYourSkyrim.say("Value #", shown);
};

// a quest the player is in the middle of or that waits for the player, its sign is gold
KnowYourSkyrim.questActive = function(item)
{
   return item.state == 1 || item.state == 2;
};

// the sign of a row is gold for a quest going and for a perk the player has
KnowYourSkyrim.rowLit = function(item)
{
   return KnowYourSkyrim.questActive(item) || item.rank > 0;
};

// A text cut with dots until it fits its field, the whole of it otherwise
KnowYourSkyrim.fitText = function(field, text, after)
{
   var tail = after != undefined ? after : "";
   var keep = text.length;
   field.text = text + tail;
   while(field.textWidth > field._width - 4 && keep > 1)
   {
      keep--;
      field.text = text.substr(0, keep) + "..." + tail;
   }
   return field.text;
};

// A name with dim words after it in one field. The name matters more, so the words after it give way first, down to the
// part kept whole (what tells twins apart), then the name ends in dots too. Tells the words after it as they stand
KnowYourSkyrim.ROW_TAIL_LEAST = 8;
KnowYourSkyrim.fitRow = function(field, name, tail, kept)
{
   var room = field._width - 4;
   var shown = tail;
   var keep = tail.length;
   var least = Math.max(kept, Math.min(tail.length, KnowYourSkyrim.ROW_TAIL_LEAST));
   field.text = name + tail;
   while(field.textWidth > room && keep > least)
   {
      keep--;
      // cut down to the part kept whole it ends there, dots would say more was cut
      shown = keep == kept ? tail.substr(0, keep) : tail.substr(0, keep) + "...";
      field.text = name + shown;
   }
   if(field.textWidth > room)
   {
      KnowYourSkyrim.fitText(field, name, shown);
   }
   return shown;
};

// One page of rows from the plugin. A new search draws the tabs with their counts and the chips again too, a page
// further down the same search only fills the rows
KnowYourSkyrim.findBrowser = function(first, fresh)
{
   var browser = KnowYourSkyrim.browser;
   var state = KnowYourSkyrim.browserState;
   var rows = browser.rows;
   var page = KnowYourSkyrim.plugin().BrowserFind({family:state.family, chips:KnowYourSkyrim.chosenBrowserChips(), terms:KnowYourSkyrim.browserTerms(state.words), mod:state.mod, within:state.within, first:Math.max(0, first), count:rows.count});
   var i = 0;
   var row;
   var item;
   var tags;
   if(page == undefined)
   {
      return undefined;
   }
   KnowYourSkyrim.browserPage = page;
   state.first = page.first;
   tags = KnowYourSkyrim.twinTags(page.rows);
   while(i < rows.count)
   {
      row = rows["row" + i];
      item = page.rows[i];
      row.item = item;
      row._visible = item != undefined;
      if(item != undefined)
      {
         row.words._width = row.wordsWide - (item.guard > 0 ? KnowYourSkyrim.BROWSER_MARK + 6 : 0);
         // what tells twins apart goes first, it is the part of the tail that matters most
         row.more = KnowYourSkyrim.fitRow(row.words, KnowYourSkyrim.gameName(item.name), (tags[i] != undefined ? "  " + tags[i] : "") + KnowYourSkyrim.rowMore(item), tags[i] != undefined ? tags[i].length + 2 : 0);
         KnowYourSkyrim.markGuard(row, item.guard);
         KnowYourSkyrim.fitText(row.mod, item.mod);
         row.sign.clear();
         KnowYourSkyrim.drawIcon(row.sign, item.icon, 4, (KnowYourSkyrim.BROWSER_ROW - KnowYourSkyrim.BROWSER_ICON) / 2, KnowYourSkyrim.BROWSER_ICON, KnowYourSkyrim.rowLit(item) ? KnowYourSkyrim.GOLD : KnowYourSkyrim.PARCHMENT, 85);
         KnowYourSkyrim.drawBrowserRow(row);
      }
      i++;
   }
   rows.none._visible = page.total == 0;
   browser.count.text = page.total > rows.count ? KnowYourSkyrim.say("#1-#2 of #3", page.first + 1, Math.min(page.first + rows.count, page.total), page.total) : KnowYourSkyrim.say("# found", page.total);
   if(page.readingTexts == true)
   {
      browser.count.text = KnowYourSkyrim.say("#1 found, reading the descriptions...", page.total);
      KnowYourSkyrim.findAgainLater();
   }
   if(page.reading == true)
   {
      browser.count.text = KnowYourSkyrim.say("#1 found, reading models #2 of #3", page.total, KnowYourSkyrim.number(page.read), KnowYourSkyrim.number(page.readOf));
      KnowYourSkyrim.findAgainLater();
   }
   browser.count._x = browser.wide - KnowYourSkyrim.BROWSER_PAD - browser.count._width;
   browser.count._y = KnowYourSkyrim.BROWSER_PAD + Math.round((KnowYourSkyrim.BROWSER_HEAD - browser.count._height) / 2);
   if(fresh)
   {
      KnowYourSkyrim.drawBrowserTabs(page);
      KnowYourSkyrim.drawBrowserChips();
   }
   KnowYourSkyrim.drawBrowserBar();
};

// While the plugin reads the models for a mod of textures the rows are asked again every little while, where they are
KnowYourSkyrim.findAgainLater = function()
{
   var rows = KnowYourSkyrim.browser.rows;
   if(rows.onEnterFrame != undefined)
   {
      return undefined;
   }
   rows.wait = KnowYourSkyrim.BROWSER_READ_WAIT;
   rows.onEnterFrame = function()
   {
      if(--this.wait > 0)
      {
         return undefined;
      }
      delete this.onEnterFrame;
      if(KnowYourSkyrim.browsing)
      {
         KnowYourSkyrim.findBrowser(KnowYourSkyrim.browserState.first, true);
      }
   };
};

KnowYourSkyrim.scrollBrowserRows = function(by)
{
   var page = KnowYourSkyrim.browserPage;
   var rows = KnowYourSkyrim.browser.rows;
   var most = Math.max(0, page.total - rows.count);
   var first = Math.max(0, Math.min(most, page.first + by));
   if(first != page.first)
   {
      KnowYourSkyrim.findBrowser(first, false);
      KnowYourSkyrim.keepBrowser();
   }
};

// a thin bar at the side of the rows tells where the page is in all of them, it can be dragged
KnowYourSkyrim.drawBrowserBar = function()
{
   var page = KnowYourSkyrim.browserPage;
   var rows = KnowYourSkyrim.browser.rows;
   var bar = rows.bar;
   var track = rows.high;
   var thumb;
   var at;
   bar.clear();
   bar._visible = page != undefined && page.total > rows.count;
   if(!bar._visible)
   {
      return undefined;
   }
   thumb = Math.max(16, track * rows.count / page.total);
   at = (track - thumb) * page.first / Math.max(1, page.total - rows.count);
   bar.track = track;
   bar.thumb = thumb;
   bar.beginFill(KnowYourSkyrim.WHITE, 12);
   KnowYourSkyrim.roundBox(bar, 0, 0, KnowYourSkyrim.BROWSER_BAR, track, 2);
   bar.endFill();
   bar.beginFill(KnowYourSkyrim.GOLD, KnowYourSkyrim.browserBarHeld ? 95 : 70);
   KnowYourSkyrim.roundBox(bar, 0, at, KnowYourSkyrim.BROWSER_BAR, thumb, 2);
   bar.endFill();
};

// the bar under the pointer with nothing lying over it, the inspect view or a window of the panel
KnowYourSkyrim.onBrowserBar = function()
{
   var bar = KnowYourSkyrim.browser.rows.bar;
   var point = {x:_root._xmouse, y:_root._ymouse};
   var top = Mouse.getTopMostEntity(_root._xmouse, _root._ymouse, true);
   if(!bar._visible || KnowYourSkyrim.anyInspect() || top == undefined || String(top).indexOf(String(KnowYourSkyrim.browser)) != 0)
   {
      return false;
   }
   bar.globalToLocal(point);
   return point.x >= -5 && point.x <= KnowYourSkyrim.BROWSER_BAR + 5 && point.y >= 0 && point.y <= bar.track;
};

KnowYourSkyrim.dragBrowserBar = function()
{
   var page = KnowYourSkyrim.browserPage;
   var rows = KnowYourSkyrim.browser.rows;
   var bar = rows.bar;
   var point = {x:_root._xmouse, y:_root._ymouse};
   var most = Math.max(0, page.total - rows.count);
   var share;
   bar.globalToLocal(point);
   share = (point.y - bar.thumb / 2) / Math.max(1, bar.track - bar.thumb);
   KnowYourSkyrim.scrollBrowserRows(Math.round(Math.max(0, Math.min(1, share)) * most) - page.first);
   KnowYourSkyrim.drawBrowserBar();
};
