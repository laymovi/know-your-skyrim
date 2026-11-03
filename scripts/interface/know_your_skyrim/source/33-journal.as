// The page of an entry in the way of the game's journal, each head in the middle between two rules, each section on a
// panel of its own, the journal's entries and a book's words in the letters of the game's books. Objectives, stages and
// quests carry a box ticked, crossed or with a dot for the one the quest stands at, a done one dimmer. Quest chains go
// as a tree of their forks. A quest and a place have no model to show, their tile shrinks to a small sign
KnowYourSkyrim.SMALL_TILE = 60;
KnowYourSkyrim.MARK_WIDE = 18;
KnowYourSkyrim.FORK_STEP = 14;
KnowYourSkyrim.PANEL_PAD = 8;
KnowYourSkyrim.SECTION_GAP = 12;
KnowYourSkyrim.FAILED = 0xC8644B;
KnowYourSkyrim.BOOK_FONT = "$SkyrimBooks";
// the sections whose words are a journal's or a book's
KnowYourSkyrim.BOOK_SECTIONS = ["Description", "Stages"];

KnowYourSkyrim.isSection = function(section, words)
{
   return section.head == KnowYourSkyrim.say(words);
};

KnowYourSkyrim.bookSection = function(section)
{
   var i = 0;
   while(i < KnowYourSkyrim.BOOK_SECTIONS.length)
   {
      if(KnowYourSkyrim.isSection(section, KnowYourSkyrim.BOOK_SECTIONS[i]))
      {
         return true;
      }
      i++;
   }
   return false;
};

// a quest, a place and a perk have no model to show or inspect, their tile is a sign
KnowYourSkyrim.signOnly = function(told)
{
   return told != undefined && (told.quest == true || told.place == true || told.perk == true);
};

// the tile of the head: a model or a portrait takes the whole, the sign of a quest, a place or a perk a small one
KnowYourSkyrim.tileOf = function(told)
{
   return KnowYourSkyrim.signOnly(told) ? KnowYourSkyrim.SMALL_TILE : KnowYourSkyrim.BROWSER_TILE;
};

KnowYourSkyrim.drawTileBox = function(details, tile)
{
   details.tile.clear();
   details.tile.lineStyle(1, KnowYourSkyrim.WHITE, 30, true);
   details.tile.beginFill(0, 55);
   KnowYourSkyrim.roundBox(details.tile, 0, 0, tile, tile, 5);
   details.tile.endFill();
   details.tile.lineStyle();
};

// a head of a section in the middle between two rules ending in small diamonds, how high it came out
KnowYourSkyrim.drawSectionHead = function(lines, section, y, wide)
{
   var field = KnowYourSkyrim.heroText(lines, section.head.toUpperCase(), wide / 2, y, 20, 13, KnowYourSkyrim.PARCHMENT, "center");
   var middle = Math.round(y + 10) + 0.5;
   lines.lineStyle(1, KnowYourSkyrim.GOLD, 40, true);
   lines.moveTo(4, middle);
   lines.lineTo(field._x - 12, middle);
   lines.moveTo(field._x + field._width + 12, middle);
   lines.lineTo(wide - 4, middle);
   lines.lineStyle();
   KnowYourSkyrim.diamond(lines, field._x - 8, middle, 3, KnowYourSkyrim.GOLD, 70);
   KnowYourSkyrim.diamond(lines, field._x + field._width + 8, middle, 3, KnowYourSkyrim.GOLD, 70);
   return 26;
};

// One line of a section. A line that carries a mark (an objective done, the stage the quest stands at, how far a quest
// is) shows its box with the words in place of the mark after them, a done one dimmer. A stage tells its number in a
// narrow column. The words of a journal or a book are in the books' letters
KnowYourSkyrim.drawSectionLine = function(lines, section, line, x, y, wide)
{
   var pieces = line;
   var color = section.color;
   var indent = 0;
   var field;
   var format;
   if(line.bare != undefined)
   {
      KnowYourSkyrim.drawMark(lines, line.mark, x + 1, y + 3, 11);
      indent = KnowYourSkyrim.MARK_WIDE;
      pieces = line.bare;
      color = line.mark == "done" ? KnowYourSkyrim.HERO_DIM : (line.mark == "later" ? KnowYourSkyrim.HERO_FAINT : (line.mark == "now" ? KnowYourSkyrim.WHITE : color));
      if(line.stage != undefined)
      {
         KnowYourSkyrim.heroText(lines, String(line.stage), x + indent, y + 1, 16, 11, line.mark == "now" ? KnowYourSkyrim.GOLD : KnowYourSkyrim.HERO_FAINT, "left");
         indent += 28;
      }
   }
   field = KnowYourSkyrim.richLine(lines, pieces, x + indent, y, wide - indent, section.size, color);
   // the books' letters stand wide and high, a size less and the lines drawn closer keep them to the others
   if(KnowYourSkyrim.bookSection(section))
   {
      format = new TextFormat();
      format.font = KnowYourSkyrim.BOOK_FONT;
      format.size = section.size - 1;
      format.leading = -3;
      field.setTextFormat(format);
   }
   return field;
};

// how far a quest is as a mark: done, in progress, waiting for the player, not begun or stopped
KnowYourSkyrim.questMark = function(state)
{
   switch(Number(state))
   {
      case 3:
         return "done";
      case 2:
         return "now";
      case 1:
         return "open";
   }
   return "later";
};

// The quests of a place a line each with its mark, under the place inside they are of. How high it came out
KnowYourSkyrim.drawQuestGroup = function(lines, section, line, x, y, wide)
{
   var top = y;
   var indent = 0;
   var quest;
   var row;
   var field;
   var tail;
   var i = 0;
   if(line.group != undefined)
   {
      field = KnowYourSkyrim.richLine(lines, line.group, x, y, wide, section.size, KnowYourSkyrim.HERO_DIM);
      lines.fields.push(field);
      y += field.textHeight + 3;
      indent = 12;
   }
   while(i < line.quests.items.length)
   {
      quest = line.quests.items[i];
      row = [KnowYourSkyrim.linkPiece(quest.name, quest.id), "  " + KnowYourSkyrim.questState(quest.state)];
      row.mark = KnowYourSkyrim.questMark(quest.state);
      row.bare = row;
      field = KnowYourSkyrim.drawSectionLine(lines, section, row, x + indent, y, wide - indent);
      lines.fields.push(field);
      y += field.textHeight + 3;
      i++;
   }
   tail = KnowYourSkyrim.cutTail(line.quests, "", KnowYourSkyrim.say("# more", line.quests.more));
   if(tail.length > 0)
   {
      field = KnowYourSkyrim.richLine(lines, tail, x + indent + KnowYourSkyrim.MARK_WIDE, y, wide - indent - KnowYourSkyrim.MARK_WIDE, section.size, KnowYourSkyrim.HERO_DIM);
      lines.fields.push(field);
      y += field.textHeight + 3;
   }
   return y - top + 1;
};

// the box before a line: ticked done, crossed failed, a dot in it for the stage the quest stands at, empty for one
// open or not given yet, that one fainter
KnowYourSkyrim.drawMark = function(clip, mark, x, y, size)
{
   var color = mark == "failed" ? KnowYourSkyrim.FAILED : (mark == "later" ? KnowYourSkyrim.HERO_FAINT : KnowYourSkyrim.GOLD);
   var alpha = mark == "done" ? 60 : 90;
   clip.lineStyle(1, color, alpha, true);
   KnowYourSkyrim.roundBox(clip, x, y, size, size, 2);
   clip.lineStyle();
   if(mark == "done")
   {
      KnowYourSkyrim.drawIcon(clip, "check", x + 1, y + 1, size - 2, color, alpha);
   }
   else if(mark == "failed")
   {
      KnowYourSkyrim.drawIcon(clip, "cross", x + 2, y + 2, size - 4, color, alpha);
   }
   else if(mark == "now")
   {
      KnowYourSkyrim.dot(clip, x + size / 2, y + size / 2, size / 4, color, alpha);
   }
};

// a filled circle drawn in eight bends
KnowYourSkyrim.dot = function(clip, x, y, r, color, alpha)
{
   var reach = r / Math.cos(Math.PI / 8);
   var i = 0;
   var angle;
   clip.lineStyle();
   clip.beginFill(color, alpha);
   clip.moveTo(x + r, y);
   while(i < 8)
   {
      angle = (i + 1) * Math.PI / 4;
      clip.curveTo(x + reach * Math.cos(angle - Math.PI / 8), y + reach * Math.sin(angle - Math.PI / 8), x + r * Math.cos(angle), y + r * Math.sin(angle));
      i++;
   }
   clip.endFill();
};

KnowYourSkyrim.diamond = function(clip, x, y, r, color, alpha)
{
   clip.lineStyle();
   clip.beginFill(color, alpha);
   clip.moveTo(x, y - r);
   clip.lineTo(x + r, y);
   clip.lineTo(x, y + r);
   clip.lineTo(x - r, y);
   clip.lineTo(x, y - r);
   clip.endFill();
};

// The chains of the quest as a tree: the chains before it, the quest itself, then the quests after it, a fork of the
// chains on a line of its own under the quest it forks from. How high it came out
KnowYourSkyrim.drawChainTree = function(lines, section, x, y, wide)
{
   var sources = KnowYourSkyrim.browserSources;
   var told = KnowYourSkyrim.browserItem;
   var before = KnowYourSkyrim.sourceCut(sources, "before");
   var after = KnowYourSkyrim.sourceCut(sources, "after");
   var label = 54;
   var top = y;
   var field;
   var tail;
   var i = 0;
   while(i < before.items.length)
   {
      KnowYourSkyrim.heroText(lines, KnowYourSkyrim.say("Before").toUpperCase(), x, y + 1, 16, 11, KnowYourSkyrim.HERO_DIM, "left");
      field = KnowYourSkyrim.richLine(lines, KnowYourSkyrim.chainPieces(before.items[i]), x + label, y, wide - label, section.size, KnowYourSkyrim.PARCHMENT);
      lines.fields.push(field);
      y += field.textHeight + 4;
      i++;
   }
   tail = KnowYourSkyrim.cutTail(before, "", KnowYourSkyrim.say("# more chains", before.more));
   if(tail.length > 0)
   {
      field = KnowYourSkyrim.richLine(lines, tail, x + label, y, wide - label, section.size, KnowYourSkyrim.HERO_DIM);
      lines.fields.push(field);
      y += field.textHeight + 4;
   }
   // the quest itself between them
   KnowYourSkyrim.dot(lines, x + label - 10, y + 9, 4, KnowYourSkyrim.GOLD, 95);
   field = KnowYourSkyrim.heroWrapped(lines, KnowYourSkyrim.gameName(told.name), x + label, y, wide - label, section.size, KnowYourSkyrim.GOLD);
   lines.fields.push(field);
   y += field.textHeight + 4;
   if(after.items.length > 0)
   {
      KnowYourSkyrim.heroText(lines, KnowYourSkyrim.say("After").toUpperCase(), x, y + 1, 16, 11, KnowYourSkyrim.HERO_DIM, "left");
      y = KnowYourSkyrim.drawForks(lines, section, KnowYourSkyrim.chainForks(after.items), x + label, y, wide - label, 0);
   }
   tail = KnowYourSkyrim.cutTail(after, "", KnowYourSkyrim.say("# more chains", after.more));
   if(tail.length > 0)
   {
      field = KnowYourSkyrim.richLine(lines, tail, x + label, y, wide - label, section.size, KnowYourSkyrim.HERO_DIM);
      lines.fields.push(field);
      y += field.textHeight + 4;
   }
   return y - top;
};

// the chains after a quest as a tree, each quest once with the quests that come after it on one chain or another
KnowYourSkyrim.chainForks = function(chains)
{
   var root = {kids:[]};
   var node;
   var kid;
   var c = 0;
   var q;
   var k;
   while(c < chains.length)
   {
      node = root;
      q = 0;
      while(q < chains[c].length)
      {
         kid = undefined;
         k = 0;
         while(k < node.kids.length && kid == undefined)
         {
            if(node.kids[k].quest.id == chains[c][q].id)
            {
               kid = node.kids[k];
            }
            k++;
         }
         if(kid == undefined)
         {
            kid = {quest:chains[c][q], kids:[]};
            node.kids.push(kid);
         }
         node = kid;
         q++;
      }
      c++;
   }
   return root;
};

// A run of quests that follow each other with no fork goes on one line, each fork under it one step further in with a
// bend drawn from the line above
KnowYourSkyrim.drawForks = function(lines, section, node, x, y, wide, depth)
{
   var step = KnowYourSkyrim.FORK_STEP;
   var at;
   var run;
   var field;
   var left;
   var k = 0;
   while(k < node.kids.length)
   {
      run = [node.kids[k].quest];
      at = node.kids[k];
      while(at.kids.length == 1)
      {
         at = at.kids[0];
         run.push(at.quest);
      }
      left = x + depth * step;
      if(depth > 0)
      {
         lines.lineStyle(1, KnowYourSkyrim.GOLD, 45, true);
         lines.moveTo(left - step + 4, y - 3);
         lines.lineTo(left - step + 4, y + 9);
         lines.lineTo(left - 2, y + 9);
         lines.lineStyle();
      }
      field = KnowYourSkyrim.richLine(lines, KnowYourSkyrim.chainPieces(run), left + (depth > 0 ? 12 : 0), y, wide - depth * step - 12, section.size, KnowYourSkyrim.PARCHMENT);
      lines.fields.push(field);
      y += field.textHeight + 4;
      y = KnowYourSkyrim.drawForks(lines, section, at, x, y, wide, depth + 1);
      k++;
   }
   return y;
};

KnowYourSkyrim.drawSectionPanel = function(lines, top, bottom, wide)
{
   lines.lineStyle(1, KnowYourSkyrim.WHITE, 10, true);
   lines.beginFill(KnowYourSkyrim.WHITE, 4);
   KnowYourSkyrim.roundBox(lines, 0, top, wide, bottom - top, 5);
   lines.endFill();
   lines.lineStyle();
};
