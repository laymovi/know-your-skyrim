// what stands beside the tile: the kind of the entry, the numbers or levels, what it is a variant of, whether the
// player has a perk, the mod it comes from and its form id
KnowYourSkyrim.browserHeadLines = function(told)
{
   var lines = [];
   var kind = told.living == true ? KnowYourSkyrim.livingKind(told) : KnowYourSkyrim.browserKind(told);
   var numbers = told.living == true ? KnowYourSkyrim.livingNumbers(told) : (told.quest == true ? KnowYourSkyrim.questNumbers(told) : (told.perk == true ? KnowYourSkyrim.perkNumbers(told) : KnowYourSkyrim.browserNumbers(told)));
   if(kind != "")
   {
      lines.push({pieces:[kind], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(told.place == true)
   {
      lines = lines.concat(KnowYourSkyrim.placeHeadLines(told));
   }
   if(numbers != "")
   {
      lines.push({pieces:[numbers], size:13, color:KnowYourSkyrim.WHITE});
   }
   if(told.trains != undefined)
   {
      lines.push({pieces:[KnowYourSkyrim.say("Trains #1 up to level #2", told.trains, told.trainsTo)], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(told.guard > 0)
   {
      lines.push({pieces:[told.guard == KnowYourSkyrim.ESSENTIAL ? KnowYourSkyrim.say("Essential: the game keeps it alive") : KnowYourSkyrim.say("Protected: only you can kill it")], size:13, color:KnowYourSkyrim.BROWSER_GUARD, sign:"ward", alpha:told.guard == KnowYourSkyrim.ESSENTIAL ? 95 : 45});
   }
   else if(told.guardedRecords > 0)
   {
      lines.push({pieces:[KnowYourSkyrim.say("Kept alive in #1 of #2 records", told.guardedRecords, told.records)], size:13, color:KnowYourSkyrim.BROWSER_GUARD, sign:"ward", alpha:45});
   }
   if(told.perk == true)
   {
      lines.push(KnowYourSkyrim.perkHeadLine(told));
   }
   if(told.original != undefined)
   {
      lines.push({pieces:KnowYourSkyrim.sayPieces("Variant of #", KnowYourSkyrim.linkPiece(told.original.name, told.original.id)), size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(told.retextured != undefined && told.retextured.length > 0)
   {
      lines.push({pieces:KnowYourSkyrim.sayPieces("Textures changed by #", KnowYourSkyrim.retexturedPieces(told.retextured)), size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   lines.push({pieces:KnowYourSkyrim.browserOrigin(told), size:KnowYourSkyrim.BROWSER_SMALL, color:KnowYourSkyrim.HERO_DIM});
   return lines;
};

// the mods that only change textures and change the thing's, each a press that lists what the mod changes
KnowYourSkyrim.retexturedPieces = function(names)
{
   var pieces = [];
   var i = 0;
   while(i < names.length)
   {
      if(i > 0)
      {
         pieces.push(", ");
      }
      pieces.push({text:String(names[i]), pick:KnowYourSkyrim.TEXTURES + names[i]});
      i++;
   }
   return pieces;
};

// the category and the chips that hold the item, in the words the chips have
KnowYourSkyrim.browserKind = function(told)
{
   var words = [KnowYourSkyrim.translated(KnowYourSkyrim.browserFamily(told.family))];
   var rows = KnowYourSkyrim.nameChipsOf(told.family);
   var g = 0;
   var c;
   while(rows != undefined && g < rows.length)
   {
      c = 0;
      while(c < rows[g].length)
      {
         // a chip named as its tab, Food in Food, says nothing new
         if(KnowYourSkyrim.has(told.chips, rows[g][c].key) && KnowYourSkyrim.translated(rows[g][c]) != words[0])
         {
            words.push(KnowYourSkyrim.translated(rows[g][c]));
         }
         c++;
      }
      g++;
   }
   return words.join(", ");
};

// the kind of a living thing and the races its records have
KnowYourSkyrim.livingKind = function(told)
{
   var words = KnowYourSkyrim.browserKind(told).split(", ").slice(1);
   var i = 0;
   while(told.races != undefined && i < told.races.length)
   {
      words.push(told.races[i]);
      i++;
   }
   return words.join(", ");
};

// The levels a living thing comes at: a set level or a range of them, a share of the player's level for those that
// follow the player, and what marks it out
KnowYourSkyrim.livingNumbers = function(told)
{
   var parts = [];
   if(told.levelLow != undefined)
   {
      parts.push(told.levelLow == told.levelHigh ? KnowYourSkyrim.say("Level #", told.levelLow) : KnowYourSkyrim.say("Levels #1 to #2", told.levelLow, told.levelHigh));
   }
   if(told.shareLow != undefined)
   {
      parts.push((told.shareLow == told.shareHigh ? KnowYourSkyrim.say("Level follows yours x#", KnowYourSkyrim.fixed(told.shareLow, 2)) : KnowYourSkyrim.say("Level follows yours x#1 to x#2", KnowYourSkyrim.fixed(told.shareLow, 2), KnowYourSkyrim.fixed(told.shareHigh, 2))) + KnowYourSkyrim.followRange(told));
   }
   if(told.unique == true)
   {
      parts.push(KnowYourSkyrim.say("Unique"));
   }
   if(told.merchant == true)
   {
      parts.push(KnowYourSkyrim.say("Trades"));
   }
   if(told.records > 1)
   {
      parts.push(KnowYourSkyrim.say("# records", told.records));
   }
   return parts.join("   ");
};

// how far the player is with a quest and the stage it stands at
KnowYourSkyrim.questNumbers = function(told)
{
   var parts = [KnowYourSkyrim.questState(told.state)];
   if(told.stage > 0)
   {
      parts.push(KnowYourSkyrim.say("Stage #", told.stage));
   }
   return parts.join("   ");
};

// the lowest and the highest level a living thing that follows the player reaches, no top when it has none
KnowYourSkyrim.followRange = function(told)
{
   var from = KnowYourSkyrim.number(told.followFrom);
   var to = KnowYourSkyrim.number(told.followTo);
   if(to > 0)
   {
      return " " + KnowYourSkyrim.say("(#1 to #2)", Math.max(from, 1), to);
   }
   return from > 1 ? " " + KnowYourSkyrim.say("(from #)", from) : "";
};

KnowYourSkyrim.browserNumbers = function(told)
{
   var parts = [];
   var slots;
   if(told.damage != undefined)
   {
      parts.push(KnowYourSkyrim.say("Damage #", KnowYourSkyrim.fixed(told.damage, 0)));
   }
   if(told.armor != undefined)
   {
      parts.push(KnowYourSkyrim.say("Armor #", KnowYourSkyrim.fixed(told.armor, 0)));
   }
   if(told.weight != undefined)
   {
      parts.push(KnowYourSkyrim.say("Weight #", KnowYourSkyrim.weightText(told.weight)));
   }
   if(told.value != undefined)
   {
      parts.push(KnowYourSkyrim.say("Value #", told.value));
   }
   slots = told.partMask != undefined ? KnowYourSkyrim.bodySlots(told.partMask) : [];
   if(slots.length > 0)
   {
      parts.push(slots.length == 1 ? KnowYourSkyrim.say("Slot #", slots[0]) : KnowYourSkyrim.say("Slots #", slots.join(", ")));
   }
   return parts.join("   ");
};

// A form id in eight hex digits as the console takes it. toString(16) works on 32 bit signed numbers in this Flash,
// every id from 80000000 on would come out wrong, so the halves go one by one
KnowYourSkyrim.formHex = function(value)
{
   var id = Number(value);
   var high = Math.floor(id / 65536).toString(16).toUpperCase();
   var low = (id % 65536).toString(16).toUpperCase();
   while(high.length < 4)
   {
      high = "0" + high;
   }
   while(low.length < 4)
   {
      low = "0" + low;
   }
   return high + low;
};

// the mod an item comes from, the ones that change it after, and its form id as the console takes it
KnowYourSkyrim.browserOrigin = function(told)
{
   var id = KnowYourSkyrim.formHex(told.id);
   // a record many mods touch names the first two, the count of the rest stays beside the tile and opens all of them
   // at the top of the page
   if(told.edits != undefined && told.edits.length > KnowYourSkyrim.BROWSER_EDITS_NAMED)
   {
      return KnowYourSkyrim.sayPieces("#1, changed by #2 and #3", told.mod, told.edits.slice(0, KnowYourSkyrim.BROWSER_EDITS_NAMED).join(", "), {text:KnowYourSkyrim.say("# more", told.edits.length - KnowYourSkyrim.BROWSER_EDITS_NAMED), open:"edits"}).concat(["   " + id]);
   }
   if(told.edits != undefined && told.edits.length > 0)
   {
      return [KnowYourSkyrim.say("#1, changed by #2", told.mod, told.edits.join(", ")) + "   " + id];
   }
   return [told.mod + "   " + id];
};

// The place around a place and the map that leads there, and a line when the player is in it
KnowYourSkyrim.placeHeadLines = function(told)
{
   var lines = [];
   var pieces = [];
   if(told.around != undefined)
   {
      pieces = KnowYourSkyrim.sayPieces("In #", KnowYourSkyrim.linkPiece(told.around.name, told.around.id));
   }
   if(told.world != undefined && told.world != "")
   {
      pieces.push((pieces.length > 0 ? " " : "") + KnowYourSkyrim.say("(in #)", told.world));
   }
   else if(told.marker != undefined && told.marker > 0)
   {
      if(pieces.length > 0)
      {
         pieces.push("   ");
      }
      pieces.push({text:KnowYourSkyrim.say("on the map"), marker:told.marker});
   }
   if(pieces.length > 0)
   {
      lines.push({pieces:pieces, size:13, color:KnowYourSkyrim.WHITE});
   }
   if(told.here == true)
   {
      lines.push({pieces:[KnowYourSkyrim.say("You are here")], size:13, color:KnowYourSkyrim.GOLD, sign:"pin", alpha:90});
   }
   return lines;
};

KnowYourSkyrim.pageSections = function(told, sources)
{
   if(told.living == true)
   {
      return KnowYourSkyrim.livingSections(sources);
   }
   if(told.place == true)
   {
      return KnowYourSkyrim.placeSections(told, sources);
   }
   if(told.perk == true)
   {
      return KnowYourSkyrim.perkSections(sources);
   }
   return told.quest == true ? KnowYourSkyrim.questSections(told, sources) : KnowYourSkyrim.browserSections(told, sources);
};

// What the page of an item shows, as sections of a head and lines, each line a list of pieces
KnowYourSkyrim.browserSections = function(told, sources)
{
   var sections = [];
   var effects = KnowYourSkyrim.browserEffectLines(told.effects);
   var uses;
   var variants;
   if(effects.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Effects"), lines:effects, size:13, color:KnowYourSkyrim.WHITE});
   }
   if(sources != undefined && sources.description != undefined)
   {
      sections.push({head:KnowYourSkyrim.say("Description"), lines:KnowYourSkyrim.descriptionLines(sources.description), size:13, color:KnowYourSkyrim.WHITE});
   }
   if(sources == undefined)
   {
      sections.push({head:KnowYourSkyrim.say("Where to get"), lines:[[KnowYourSkyrim.say("Reading the records...")]], size:13, color:KnowYourSkyrim.HERO_DIM});
   }
   else
   {
      sections.push({head:KnowYourSkyrim.say("Where to get"), lines:KnowYourSkyrim.sourceLines(sources), size:13, color:KnowYourSkyrim.PARCHMENT});
      uses = KnowYourSkyrim.browserUseLines(sources);
      if(uses.length > 0)
      {
         sections.push({head:KnowYourSkyrim.say("Used for"), lines:uses, size:13, color:KnowYourSkyrim.PARCHMENT});
      }
      KnowYourSkyrim.pushQuestSection(sections, sources);
   }
   variants = KnowYourSkyrim.variantLines(told);
   if(variants.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Variants"), lines:variants, size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(told.keywords != undefined && told.keywords.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Keywords"), lines:[KnowYourSkyrim.keywordPieces(told.keywords)], size:KnowYourSkyrim.BROWSER_SMALL, color:KnowYourSkyrim.HERO_DIM});
   }
   return sections;
};

// The page of a living thing: what it carries and wears, what it drops when it dies, what it sells, where it stands
// for the whole game and how many lists of encounters can bring it, its quests, then the people it's close to or at odds
// with and how it meets the player
KnowYourSkyrim.livingSections = function(sources)
{
   var sections = [];
   var where = [];
   var relations;
   var toward;
   if(sources == undefined)
   {
      return [{head:KnowYourSkyrim.say("Carries"), lines:[[KnowYourSkyrim.say("Reading the records...")]], size:13, color:KnowYourSkyrim.HERO_DIM}];
   }
   if(sources.carries.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Carries"), lines:[KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "carries"))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(sources.drops.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Drops when killed"), lines:[KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "drops"))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(sources.sells.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Sells"), lines:[KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "sells"))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   // where the one there is of it is now, before where the plugin files put it
   where = KnowYourSkyrim.placeLines(KnowYourSkyrim.say("Now in #"), KnowYourSkyrim.say("Now near #"), KnowYourSkyrim.sourceCut(sources, "now"));
   where = where.concat(KnowYourSkyrim.placeLines(KnowYourSkyrim.say("Lies dead in #"), KnowYourSkyrim.say("Lies dead near #"), KnowYourSkyrim.sourceCut(sources, "deadNow")));
   where = where.concat(KnowYourSkyrim.placeLines(KnowYourSkyrim.say("Stands in #"), KnowYourSkyrim.say("Stands near #"), KnowYourSkyrim.sourceCut(sources, "stands")));
   if(sources.encounters == 1)
   {
      where.push(sources.encounterLevel > 1 ? [KnowYourSkyrim.say("Comes in one list of encounters, from level #", sources.encounterLevel)] : [KnowYourSkyrim.say("Comes in one list of encounters")]);
   }
   else if(sources.encounters > 0)
   {
      where.push(sources.encounterLevel > 1 ? [KnowYourSkyrim.say("Comes in #1 lists of encounters, from level #2", sources.encounters, sources.encounterLevel)] : [KnowYourSkyrim.say("Comes in # lists of encounters", sources.encounters)]);
   }
   if(sources.reading == true)
   {
      where.push([KnowYourSkyrim.say("Reading the plugin files for more places...")]);
   }
   else if(where.length == 0)
   {
      where.push([KnowYourSkyrim.say("Stands nowhere in the plugin files, a quest or a script brings it")]);
   }
   sections.push({head:KnowYourSkyrim.say("Where to meet"), lines:where, size:13, color:KnowYourSkyrim.PARCHMENT});
   KnowYourSkyrim.pushQuestSection(sections, sources);
   relations = KnowYourSkyrim.relationLines(sources.relations);
   if(relations.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Relationships"), lines:relations, size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   toward = KnowYourSkyrim.towardLines(sources);
   if(toward.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Toward you"), lines:toward, size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   return sections;
};

// a rank of a relationship as Papyrus counts it, 4 a lover down to -4 an archnemesis, as a word, or what the player is
// to the one of that rank
KnowYourSkyrim.rankWord = function(rank, toYou)
{
   switch(Number(rank))
   {
      case 4:
         return toYou ? KnowYourSkyrim.say("Your lover") : KnowYourSkyrim.say("Lover");
      case 3:
         return toYou ? KnowYourSkyrim.say("Your ally") : KnowYourSkyrim.say("Ally");
      case 2:
         return toYou ? KnowYourSkyrim.say("Your confidant") : KnowYourSkyrim.say("Confidant");
      case 1:
         return toYou ? KnowYourSkyrim.say("Your friend") : KnowYourSkyrim.say("Friend");
      case -1:
         return toYou ? KnowYourSkyrim.say("Your rival") : KnowYourSkyrim.say("Rival");
      case -2:
         return toYou ? KnowYourSkyrim.say("Your foe") : KnowYourSkyrim.say("Foe");
      case -3:
         return toYou ? KnowYourSkyrim.say("Your enemy") : KnowYourSkyrim.say("Enemy");
      case -4:
         return toYou ? KnowYourSkyrim.say("Your archnemesis") : KnowYourSkyrim.say("Archnemesis");
   }
   return toYou ? KnowYourSkyrim.say("An acquaintance, no closer bond with you") : KnowYourSkyrim.say("Acquaintance");
};

// The people a person is more to than an acquaintance, a line for each rank from lovers down to archnemeses, each
// with who they are to the person ("husband", "daughter") and a secret one so marked. The plugin sends them by rank
KnowYourSkyrim.relationLines = function(relations)
{
   var lines = [];
   var line;
   var rank;
   var one;
   var name;
   var i = 0;
   while(relations != undefined && i < relations.length)
   {
      one = relations[i];
      if(rank !== one.rank)
      {
         rank = one.rank;
         line = [KnowYourSkyrim.rankWord(rank, false) + ": "];
         lines.push(line);
      }
      else
      {
         line.push(", ");
      }
      name = KnowYourSkyrim.linkPiece(one.name, one.id);
      line.push(name);
      if(one.role != undefined && one.role != "")
      {
         line.push(" (" + String(one.role).toLowerCase() + ")");
      }
      if(one.secret == true)
      {
         line.push(" " + KnowYourSkyrim.say("(secret)"));
      }
      i++;
   }
   return lines;
};

// What the player is to it, whether it's hostile now and its factions' word on the player (the one there is of a
// unique one), how it starts fights and whom it helps
KnowYourSkyrim.towardLines = function(sources)
{
   var lines = [];
   var rank = sources.playerRank;
   var temper;
   var help;
   if(rank != undefined)
   {
      lines.push([KnowYourSkyrim.rankWord(rank, true) + (sources.playerRole != undefined && sources.playerRole != "" ? " (" + String(sources.playerRole).toLowerCase() + ")" : "")]);
   }
   if(sources.hostile == true)
   {
      lines.push([KnowYourSkyrim.say("Hostile to you now")]);
   }
   else if(sources.reaction == "enemy")
   {
      lines.push([KnowYourSkyrim.say("Its factions take you for an enemy")]);
   }
   else if(sources.reaction == "ally")
   {
      lines.push([KnowYourSkyrim.say("Its factions take you for an ally")]);
   }
   else if(sources.reaction == "friend")
   {
      lines.push([KnowYourSkyrim.say("Its factions take you for a friend")]);
   }
   switch(Number(sources.aggression))
   {
      case 0:
         temper = KnowYourSkyrim.say("Never starts a fight");
         break;
      case 1:
         temper = KnowYourSkyrim.say("Attacks its enemies on sight");
         break;
      case 2:
         temper = KnowYourSkyrim.say("Attacks its enemies and strangers on sight");
         break;
      case 3:
         temper = KnowYourSkyrim.say("Attacks anyone on sight");
         break;
   }
   switch(Number(sources.assistance))
   {
      case 0:
         help = KnowYourSkyrim.say("helps no one in a fight");
         break;
      case 1:
         help = KnowYourSkyrim.say("helps its allies in a fight");
         break;
      case 2:
         help = KnowYourSkyrim.say("helps its allies and friends in a fight");
         break;
   }
   if(temper != undefined)
   {
      lines.push([temper + (help != undefined ? ", " + help : "")]);
   }
   return lines;
};

// The page of a quest: its objectives, what it gives, the chains of quests before and after it, its stages with their
// journal entries, then the people and things its aliases and scripts hold and the places it touches
KnowYourSkyrim.questSections = function(told, sources)
{
   var sections = [{head:KnowYourSkyrim.say("Objectives"), lines:KnowYourSkyrim.objectiveLines(told.objectives, Number(told.state) == 3), size:13, color:KnowYourSkyrim.WHITE}];
   var lines;
   if(sources == undefined)
   {
      sections.push({head:KnowYourSkyrim.say("Rewards"), lines:[[KnowYourSkyrim.say("Reading the records...")]], size:13, color:KnowYourSkyrim.HERO_DIM});
      return sections;
   }
   // the plugin tells whether the quests' files are in, a page of an older plugin has nothing of them
   if(sources.questsRead == false)
   {
      sections.push({head:KnowYourSkyrim.say("Rewards"), lines:[[KnowYourSkyrim.say("Reading the quests' scripts...")]], size:13, color:KnowYourSkyrim.HERO_DIM});
   }
   else if(sources.questsRead == true)
   {
      lines = KnowYourSkyrim.rewardLines(sources.rewards);
      sections.push({head:KnowYourSkyrim.say("Rewards"), lines:lines.length > 0 ? lines : [[KnowYourSkyrim.say("Its scripts give you nothing of their own")]], size:13, color:lines.length > 0 ? KnowYourSkyrim.PARCHMENT : KnowYourSkyrim.HERO_DIM});
      lines = KnowYourSkyrim.chainLines(sources);
      if(lines.length > 0)
      {
         sections.push({head:KnowYourSkyrim.say("Quest chain"), lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
      }
      lines = KnowYourSkyrim.stepLines(sources);
      if(lines.length > 0)
      {
         sections.push({head:KnowYourSkyrim.say("Stages"), lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
      }
   }
   if(sources.people.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("People"), lines:[KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "people"))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(sources.things.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Things"), lines:[KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "things"))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   KnowYourSkyrim.pushBlockers(sections, sources);
   lines = sources.locations != undefined && sources.locations.length > 0 ? [KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "locations"))] : [];
   lines = lines.concat(KnowYourSkyrim.placeLines(undefined, undefined, KnowYourSkyrim.sourceCut(sources, "places")));
   if(lines.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Places"), lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   return sections;
};

// Every objective of the quest in the journal's order, how each stands after it. Those the journal hasn't given yet are
// said to be so, while the quest isn't done. Each line also carries its mark and its bare words for the page that draws
// the mark as a box (33-journal.as)
KnowYourSkyrim.objectiveLines = function(objectives, done)
{
   var lines = [];
   var objective;
   var line;
   var i = 0;
   while(objectives != undefined && i < objectives.length)
   {
      objective = objectives[i];
      switch(Number(objective.state))
      {
         case 1:
            line = [KnowYourSkyrim.say("# (done)", objective.text)];
            line.mark = "done";
            break;
         case 2:
            line = [KnowYourSkyrim.say("# (failed)", objective.text)];
            line.mark = "failed";
            break;
         case 3:
            line = done ? [objective.text] : [KnowYourSkyrim.say("# (not yet)", objective.text)];
            line.mark = "later";
            break;
         default:
            line = [objective.text];
            line.mark = "open";
      }
      line.bare = [objective.text];
      lines.push(line);
      i++;
   }
   return lines.length > 0 ? lines : [[KnowYourSkyrim.say("It has no objectives")]];
};

// What the quest's scripts put into your hands, with the stage that gives it or the dialogue it comes in
KnowYourSkyrim.rewardLines = function(rewards)
{
   var lines = [];
   var reward;
   var pieces;
   var when;
   var i = 0;
   while(rewards != undefined && i < rewards.length)
   {
      reward = rewards[i];
      pieces = KnowYourSkyrim.rewardPieces(reward, "reward" + i);
      when = reward.stage >= 0 ? KnowYourSkyrim.say("(stage #)", reward.stage) : (reward.dialogue == true ? KnowYourSkyrim.say("(in dialogue)") : "");
      if(pieces.length > 0)
      {
         lines.push(when != "" ? pieces.concat(["  " + when]) : pieces);
      }
      i++;
   }
   return lines;
};

KnowYourSkyrim.rewardPieces = function(reward, key)
{
   var name = reward.name != undefined ? KnowYourSkyrim.linkPiece(reward.name, reward.id) : "";
   var count = KnowYourSkyrim.number(reward.count);
   var most = KnowYourSkyrim.number(reward.countHigh);
   switch(String(reward.kind))
   {
      case "spell":
         return KnowYourSkyrim.sayPieces("Spell: #", name);
      case "perk":
         return KnowYourSkyrim.sayPieces("Perk: #", name);
      case "shout":
         return KnowYourSkyrim.sayPieces("Shout: #", name);
      case "word":
         return reward.means != undefined ? KnowYourSkyrim.sayPieces("Word of power: #1 (#2)", name, reward.means) : KnowYourSkyrim.sayPieces("Word of power: #", name);
      case "skill":
         return [count > 0 ? KnowYourSkyrim.say("#1 +#2", reward.name, count) : KnowYourSkyrim.say("Training in #", reward.name)];
      case "perkPoints":
         return [count == 1 ? KnowYourSkyrim.say("A perk point") : KnowYourSkyrim.say("# perk points", count)];
      case "souls":
         return [count == 1 ? KnowYourSkyrim.say("A dragon soul") : KnowYourSkyrim.say("# dragon souls", count)];
   }
   // a thing picked from a list names what it may be
   if(reward.options != undefined)
   {
      return KnowYourSkyrim.sayPieces(count > 1 ? "#1 of #2" : "One of #2", count, KnowYourSkyrim.peoplePieces(KnowYourSkyrim.cutList(reward.options, reward.optionsMore, reward.optionsShown, key)));
   }
   // a list of one thing gives as many as the level picks
   if(most > count)
   {
      return KnowYourSkyrim.sayPieces("#1 (#2 to #3)", name, count, most);
   }
   return count > 1 ? KnowYourSkyrim.sayPieces("#1 (#2)", name, count) : [name];
};

// The quests before this one, from the first of a chain, and the quests after it, the longest chain first. A quest
// with no chain says nothing
KnowYourSkyrim.chainLines = function(sources)
{
   var lines = [];
   var sides = [{key:"before", words:"Before: #"}, {key:"after", words:"After: #"}];
   var cut;
   var tail;
   var s = 0;
   var i;
   while(s < sides.length)
   {
      cut = KnowYourSkyrim.sourceCut(sources, sides[s].key);
      i = 0;
      while(i < cut.items.length)
      {
         lines.push(KnowYourSkyrim.sayPieces(sides[s].words, KnowYourSkyrim.chainPieces(cut.items[i])));
         i++;
      }
      tail = KnowYourSkyrim.cutTail(cut, "", KnowYourSkyrim.say("# more chains", cut.more));
      if(tail.length > 0)
      {
         lines.push(tail);
      }
      s++;
   }
   return lines;
};

KnowYourSkyrim.chainPieces = function(chain)
{
   var pieces = [];
   var i = 0;
   while(i < chain.length)
   {
      if(i > 0)
      {
         pieces.push(" > ");
      }
      pieces.push(KnowYourSkyrim.linkPiece(chain[i].name, chain[i].id));
      i++;
   }
   return pieces;
};

// The stages the journal tells, each with what it says: done, the one the quest stands at now, and the ones that
// finish or fail it
KnowYourSkyrim.stepLines = function(sources)
{
   var lines = [];
   var cut = KnowYourSkyrim.cutList(sources.steps, 0, KnowYourSkyrim.BROWSER_STEPS_SHOWN, "steps");
   var tail = KnowYourSkyrim.cutTail(cut, "", KnowYourSkyrim.say("# more stages", cut.more));
   var step;
   var marks;
   var ends;
   var line;
   var i = 0;
   while(i < cut.items.length)
   {
      step = cut.items[i];
      marks = [];
      ends = [];
      if(step.current == true)
      {
         marks.push(KnowYourSkyrim.say("now"));
      }
      else if(step.done == true)
      {
         marks.push(KnowYourSkyrim.say("done"));
      }
      if(step.finishes == true)
      {
         ends.push(KnowYourSkyrim.say("finishes the quest"));
      }
      if(step.fails == true)
      {
         ends.push(KnowYourSkyrim.say("fails the quest"));
      }
      marks = marks.concat(ends);
      // the journal's words go in as a piece, a # in them stays as it is
      line = KnowYourSkyrim.sayPieces("#1: #2", step.stage, step.text).concat(marks.length > 0 ? [" (" + marks.join(", ") + ")"] : []);
      line.mark = step.current == true ? "now" : (step.done == true ? "done" : "later");
      line.stage = step.stage;
      line.bare = [step.text].concat(ends.length > 0 ? ["  (" + ends.join(", ") + ")"] : []);
      lines.push(line);
      i++;
   }
   if(tail.length > 0)
   {
      lines.push(tail);
   }
   return lines;
};

// The quests that touch a place: the place itself, then each place right inside it with the quests of everything
// inside that one. Each quest says how far you are with it
KnowYourSkyrim.placeQuestLines = function(sources)
{
   var lines = [];
   var groups = KnowYourSkyrim.sourceCut(sources, "questGroups");
   var tail = KnowYourSkyrim.cutTail(groups, "", KnowYourSkyrim.say("# more places", groups.more));
   var group;
   var quests;
   var cut;
   var line;
   var i = 0;
   while(i < groups.items.length)
   {
      group = groups.items[i];
      cut = KnowYourSkyrim.cutList(group.quests, group.questsMore, group.questsShown, "quests" + i);
      quests = KnowYourSkyrim.questPieces(cut);
      line = group.place != undefined ? KnowYourSkyrim.sayPieces("In #1: #2", KnowYourSkyrim.linkPiece(group.place, group.id), quests) : quests;
      // the page draws each quest with its mark on a line of its own (33-journal.as)
      line.quests = cut;
      line.group = group.place != undefined ? KnowYourSkyrim.sayPieces("In #", KnowYourSkyrim.linkPiece(group.place, group.id)) : undefined;
      lines.push(line);
      i++;
   }
   if(tail.length > 0)
   {
      lines.push(tail);
   }
   return lines;
};

KnowYourSkyrim.questPieces = function(cut)
{
   var pieces = [];
   var i = 0;
   while(i < cut.items.length)
   {
      if(i > 0)
      {
         pieces.push(", ");
      }
      pieces = pieces.concat(KnowYourSkyrim.sayPieces("#1 (#2)", KnowYourSkyrim.linkPiece(cut.items[i].name, cut.items[i].id), KnowYourSkyrim.questState(cut.items[i].state)));
      i++;
   }
   return pieces.concat(KnowYourSkyrim.cutTail(cut, ", ", KnowYourSkyrim.say("# more", cut.more)));
};

// Where a thing stands from the player: how far, on which side, how much higher. The plugin counts the sides clockwise
// from where the player looks
KnowYourSkyrim.spotWords = function(spot)
{
   var sides = ["ahead", "ahead on the right", "on the right", "behind on the right", "behind", "behind on the left", "on the left", "ahead on the left"];
   var up = Math.round(Number(spot.up));
   var far = KnowYourSkyrim.say("# m", Math.max(1, Math.round(Number(spot.far))));
   var height = KnowYourSkyrim.say("at your level");
   if(Math.abs(up) >= 2)
   {
      height = KnowYourSkyrim.say(up > 0 ? "# m above you" : "# m below you", Math.abs(up));
   }
   return far + " " + KnowYourSkyrim.say(sides[Number(spot.side)]) + ", " + height;
};

// What a shut thing in the player's cell needs: the key its lock names, or what its activate parents name
KnowYourSkyrim.blockerLines = function(shut)
{
   var thing = shut.thing != undefined && shut.thing != "" ? shut.thing : KnowYourSkyrim.say(shut.raised == true ? "A raised bridge" : "A shut gate");
   var lines = [];
   var opener;
   var who;
   var i = 0;
   if(shut.key != undefined)
   {
      lines.push([KnowYourSkyrim.say(shut.carried == true ? "#1, #2: needs #3, you carry it" : "#1, #2: needs #3", thing, KnowYourSkyrim.spotWords(shut), shut.key)]);
      return lines;
   }
   lines.push([thing + ", " + KnowYourSkyrim.spotWords(shut)]);
   while(i < shut.openers.length)
   {
      opener = shut.openers[i];
      who = opener.name != "" ? opener.name : KnowYourSkyrim.say("something with no name");
      if(opener.off == true)
      {
         lines.push(["   " + KnowYourSkyrim.say("What opens it isn't here yet, it depends on a quest")]);
      }
      else if(opener.away == true)
      {
         lines.push(["   " + KnowYourSkyrim.say("Opened by # in another place", who)]);
      }
      else
      {
         lines.push(["   " + KnowYourSkyrim.say("Opened by #1, #2", who, KnowYourSkyrim.spotWords(opener))]);
      }
      i++;
   }
   return lines;
};

// A section of what stays shut around the player, for the page of the place the player is in and for a quest with a
// target in this cell. The progression mode keeps it behind a press until the player asks
KnowYourSkyrim.pushBlockers = function(sections, sources)
{
   var lines = [];
   var head = KnowYourSkyrim.say("Shut around you");
   var cut;
   var total;
   var tail;
   var i = 0;
   if(sources.blockers == undefined)
   {
      return;
   }
   total = sources.blockers.length + KnowYourSkyrim.number(sources.blockersMore);
   if(total == 0 && KnowYourSkyrim.number(sources.blockersNoParent) == 0)
   {
      return;
   }
   if(sources.blockersHidden == true && KnowYourSkyrim.browserOpen.blockers != true)
   {
      lines.push([KnowYourSkyrim.say("# ways stay shut here", total + KnowYourSkyrim.number(sources.blockersNoParent)) + "   ", {text:KnowYourSkyrim.say("show what opens them"), open:"blockers"}]);
      sections.push({head:head, lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
      return;
   }
   cut = KnowYourSkyrim.sourceCut(sources, "blockers");
   while(i < cut.items.length)
   {
      lines = lines.concat(KnowYourSkyrim.blockerLines(cut.items[i]));
      i++;
   }
   if(KnowYourSkyrim.number(sources.blockersNoParent) > 0)
   {
      lines.push([KnowYourSkyrim.say("# more stay shut, nothing here says what opens them", sources.blockersNoParent)]);
   }
   tail = KnowYourSkyrim.cutTail(cut, "", KnowYourSkyrim.say("# more", cut.more));
   if(tail.length > 0)
   {
      lines.push(tail);
   }
   sections.push({head:head, lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
};

// The page of a place: who is in it now, by the places right inside it, the dead lying there, who lives there by the
// records of the place, and the places inside it. Keywords last
KnowYourSkyrim.placeSections = function(told, sources)
{
   var sections = [];
   var here;
   var dead;
   if(sources == undefined)
   {
      return [{head:KnowYourSkyrim.say("Here now"), lines:[[KnowYourSkyrim.say("Reading the records...")]], size:13, color:KnowYourSkyrim.HERO_DIM}];
   }
   here = KnowYourSkyrim.crowdLines(sources, "here");
   KnowYourSkyrim.pushBlockers(sections, sources);
   sections.push({head:KnowYourSkyrim.say("Here now"), lines:here.length > 0 ? here : [[KnowYourSkyrim.say("No one is here now")]], size:13, color:KnowYourSkyrim.PARCHMENT});
   dead = KnowYourSkyrim.crowdLines(sources, "deadHere");
   if(dead.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Lie dead here"), lines:dead, size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(sources.lives != undefined && sources.lives.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Live here"), lines:[KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "lives"))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(sources.inside != undefined && sources.inside.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Places inside"), lines:[KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "inside"))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(sources.questsRead == false)
   {
      sections.push({head:KnowYourSkyrim.say("Quests"), lines:[[KnowYourSkyrim.say("Reading the quests' scripts...")]], size:13, color:KnowYourSkyrim.HERO_DIM});
   }
   else if(sources.questGroups != undefined && sources.questGroups.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Quests"), lines:KnowYourSkyrim.placeQuestLines(sources), size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(told.keywords != undefined && told.keywords.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Keywords"), lines:[KnowYourSkyrim.keywordPieces(told.keywords)], size:KnowYourSkyrim.BROWSER_SMALL, color:KnowYourSkyrim.HERO_DIM});
   }
   return sections;
};

// A line for each group of a list of a place: the ones right there first, then each place inside it with the ones in
// it. One press opens every group whole
KnowYourSkyrim.crowdLines = function(sources, key)
{
   var lines = [];
   var groups = KnowYourSkyrim.sourceCut(sources, key);
   var tail = KnowYourSkyrim.cutTail(groups, "", KnowYourSkyrim.say("# more places", groups.more));
   var group;
   var people;
   var i = 0;
   while(i < groups.items.length)
   {
      group = groups.items[i];
      people = KnowYourSkyrim.peoplePieces(KnowYourSkyrim.cutList(group.people, group.peopleMore, group.peopleShown, key));
      lines.push(group.place != undefined ? KnowYourSkyrim.sayPieces("In #1: #2", KnowYourSkyrim.linkPiece(group.place, group.id), people) : people);
      i++;
   }
   if(tail.length > 0)
   {
      lines.push(tail);
   }
   return lines;
};

// names with how many there are of each, a name the browser has an entry for is a link to it
KnowYourSkyrim.peoplePieces = function(cut)
{
   var pieces = [];
   var apart = KnowYourSkyrim.maskedApart(cut.items);
   var people = apart.known;
   var i = 0;
   var name;
   while(i < people.length)
   {
      if(i > 0)
      {
         pieces.push(", ");
      }
      name = KnowYourSkyrim.linkPiece(people[i].name, people[i].id);
      pieces = pieces.concat(people[i].count > 1 ? KnowYourSkyrim.sayPieces("#1 (#2)", name, people[i].count) : [name]);
      i++;
   }
   return KnowYourSkyrim.maskPieces(pieces, apart.masks).concat(KnowYourSkyrim.cutTail(cut, ", ", KnowYourSkyrim.say("# more", cut.more)));
};

// The quests an item or a living thing takes part in, each a link with how far the player is with it
KnowYourSkyrim.pushQuestSection = function(sections, sources)
{
   var lines = [];
   var cut = KnowYourSkyrim.sourceCut(sources, "quests");
   var tail = KnowYourSkyrim.cutTail(cut, "", KnowYourSkyrim.say("# more", cut.more));
   var quest;
   var line;
   var i = 0;
   while(i < cut.items.length)
   {
      quest = cut.items[i];
      line = [KnowYourSkyrim.linkPiece(quest.name, quest.id), "  " + KnowYourSkyrim.questState(quest.state)];
      line.mark = KnowYourSkyrim.questMark(quest.state);
      line.bare = line;
      lines.push(line);
      i++;
   }
   if(tail.length > 0)
   {
      lines.push(tail);
   }
   if(lines.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Quests"), lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
   }
};

// A place is its name, or the marker of the map it lies near, with a chest that stands there before it. A place the
// map can show ends in a link that opens the map there. nearSaid is true when the words around it say near already
KnowYourSkyrim.spotPieces = function(place, nearSaid)
{
   var pieces = [];
   // a place the character hasn't found has no map to it either, a place of a list with no link of its own is
   // masked by the place it lies in
   var mask = KnowYourSkyrim.maskOf(place.id != undefined && place.id > 0 ? place.id : place.place);
   if(place.thing != undefined && place.thing != "")
   {
      pieces.push(place.thing + ", ");
   }
   // a place the browser has an entry for leads to its page
   pieces = pieces.concat(place.near == true && nearSaid != true ? KnowYourSkyrim.sayPieces("near #", mask != undefined ? mask : place.name) : [KnowYourSkyrim.linkPiece(place.name, place.id)]);
   // the map shows the world the player is in, a place of another world says which world instead of a link
   if(mask == undefined && place.world != undefined && place.world != "")
   {
      pieces.push(" " + KnowYourSkyrim.say("(in #)", place.world));
   }
   else if(mask == undefined && place.marker != undefined && place.marker > 0)
   {
      pieces.push("   ");
      pieces.push({text:KnowYourSkyrim.say("on the map"), marker:place.marker});
   }
   if(place.by != undefined)
   {
      pieces.push("   " + KnowYourSkyrim.handoutWords(place, true));
   }
   return pieces;
};

// a line that starts with words in small letters, "near Riverwood", starts with a capital
KnowYourSkyrim.capitalFirst = function(pieces)
{
   var first = pieces.length > 0 && typeof(pieces[0]) != "object" ? String(pieces[0]) : "";
   if(first.length > 0)
   {
      pieces[0] = first.charAt(0).toUpperCase() + first.substr(1);
   }
   return pieces;
};

// One line for each place, the words around it say what stands there, in a place or near a marker of the map
KnowYourSkyrim.placeLines = function(words, nearWords, cut)
{
   var lines = [];
   var tail = KnowYourSkyrim.cutTail(cut, "", KnowYourSkyrim.say("# more places", cut.more));
   var i = 0;
   var place;
   var near;
   var pieces;
   while(i < cut.items.length)
   {
      place = cut.items[i];
      near = place.near == true && (place.thing == undefined || place.thing == "") && nearWords != undefined;
      pieces = KnowYourSkyrim.spotPieces(place, near);
      lines.push(words != undefined ? KnowYourSkyrim.sayPieces(near ? nearWords : words, pieces) : KnowYourSkyrim.capitalFirst(pieces));
      i++;
   }
   if(tail.length > 0)
   {
      lines.push(tail);
   }
   return lines;
};

// An effect in the game's words, its name where it has none. The effects of an ingredient not found yet are one line
// that counts them. The name of an effect searches for everything that has it
KnowYourSkyrim.browserEffectLines = function(effects)
{
   var lines = [];
   var unknown = 0;
   var i = 0;
   var effect;
   while(effects != undefined && i < effects.length)
   {
      effect = effects[i];
      if(!effect.known)
      {
         unknown++;
         i++;
         continue;
      }
      if(effect.words != "" && effect.name != "")
      {
         lines.push([{text:effect.name, search:"$\"" + effect.name + "\""}, ": " + effect.words]);
      }
      else if(effect.name != "")
      {
         lines.push([{text:effect.name, search:"$\"" + effect.name + "\""}]);
      }
      else
      {
         lines.push([effect.words]);
      }
      // an effect with conditions works only while they hold, several of them are steps of one and don't add up
      if(effect.conditional == true)
      {
         lines[lines.length - 1].push(" " + KnowYourSkyrim.say("(under some conditions)"));
      }
      i++;
   }
   if(unknown > 0)
   {
      lines.push([unknown == 1 ? KnowYourSkyrim.say("One effect you haven't found yet") : KnowYourSkyrim.say("# effects you haven't found yet", unknown)]);
   }
   return lines;
};

// A description a paragraph to a line. A long one, the text of a book, shows its start and opens whole on a press
KnowYourSkyrim.descriptionLines = function(text)
{
   var whole = String(text);
   var open = KnowYourSkyrim.browserOpen.description == true || KnowYourSkyrim.findAll;
   var cut = whole.length > KnowYourSkyrim.DESCRIPTION_SHOWN;
   var shown = whole;
   var lines = [];
   var paragraphs;
   var end;
   var i = 0;
   if(cut && !open)
   {
      end = whole.lastIndexOf(" ", KnowYourSkyrim.DESCRIPTION_SHOWN);
      shown = whole.substr(0, end > 0 ? end : KnowYourSkyrim.DESCRIPTION_SHOWN) + "...";
   }
   // the empty lines between the paragraphs of a book would leave every other line of the page blank
   paragraphs = shown.split("\n");
   while(i < paragraphs.length)
   {
      if(paragraphs[i].length > 0)
      {
         lines.push([paragraphs[i]]);
      }
      i++;
   }
   if(cut && !open)
   {
      lines.push([{text:KnowYourSkyrim.say("show all"), open:"description"}]);
   }
   else if(cut && !KnowYourSkyrim.findAll)
   {
      lines.push([{text:KnowYourSkyrim.say("show less"), shut:"description"}]);
   }
   return lines;
};

// the keywords of an entry, each searches for everything that has it
KnowYourSkyrim.keywordPieces = function(keywords)
{
   var pieces = [];
   var i = 0;
   while(i < keywords.length)
   {
      if(i > 0)
      {
         pieces.push(", ");
      }
      pieces.push({text:keywords[i], search:"#" + keywords[i]});
      i++;
   }
   return pieces;
};

// The variants of an original, one line of links. Variants of one name, a bow blessed at two levels, are one link
// with how many there are, the list of rows tells them apart
KnowYourSkyrim.variantLines = function(told)
{
   var cut = KnowYourSkyrim.cutList(told.variants, told.variantsMore, KnowYourSkyrim.BROWSER_VARIANTS_SHOWN, "variants");
   var named = [];
   var at = {};
   var pieces;
   var i = 0;
   while(i < cut.items.length)
   {
      if(at[cut.items[i].name] == undefined)
      {
         at[cut.items[i].name] = named.length;
         named.push({name:cut.items[i].name, id:cut.items[i].id, count:0});
      }
      named[at[cut.items[i].name]].count++;
      i++;
   }
   pieces = KnowYourSkyrim.peoplePieces({items:named, more:cut.more, open:cut.open, shuts:cut.shuts, key:cut.key});
   return pieces.length > 0 ? [pieces] : [];
};

// Where an item can be got, each way on a line of pieces. The places of loot the game sets only when a cell loads are
// known from the plugin files alone, so an item with nothing found says so
KnowYourSkyrim.sourceLines = function(sources)
{
   var lines = KnowYourSkyrim.recipeLines(sources.made);
   var i = 0;
   var recipe;
   if(sources.sold.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Sold by #", KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "sold"))));
   }
   if(sources.carried.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Carried or worn by #", KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "carried"))));
   }
   if(sources.dropped.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Dropped by #", KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "dropped"))));
   }
   if(sources.rewardOf != undefined && sources.rewardOf.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("A reward of #", KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "rewardOf"))));
   }
   if(sources.rewardPickedOf != undefined && sources.rewardPickedOf.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("May be picked as a reward of #", KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "rewardPickedOf"))));
   }
   lines = lines.concat(KnowYourSkyrim.toldLines(sources.told));
   if(sources.harvested.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Grows on #", KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "harvested"))));
   }
   lines = lines.concat(KnowYourSkyrim.placeLines(KnowYourSkyrim.say("In #"), undefined, KnowYourSkyrim.sourceCut(sources, "chests")));
   lines = lines.concat(KnowYourSkyrim.placeLines(KnowYourSkyrim.say("Lies in #"), KnowYourSkyrim.say("Lies near #"), KnowYourSkyrim.sourceCut(sources, "lies")));
   // what Base Object Swapper puts it in place of where things stand, and what it may put in its place
   if(sources.swappedFor != undefined && sources.swappedFor.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Stands in place of #", KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "swappedFor"))));
   }
   lines = lines.concat(KnowYourSkyrim.placeLines(KnowYourSkyrim.say("Stands in #"), KnowYourSkyrim.say("Stands near #"), KnowYourSkyrim.sourceCut(sources, "swappedAt")));
   if(sources.swappedBy != undefined && sources.swappedBy.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("# may stand in its place", KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "swappedBy"))));
   }
   if(sources.common > 0)
   {
      lines.push([sources.common == 1 ? KnowYourSkyrim.say("Common loot of one kind of chest or person") : KnowYourSkyrim.say("Common loot of # kinds of chests and people", sources.common)]);
   }
   if(lines.length == 0 && sources.lists > 0)
   {
      // no record hands the list out, a mod's script adds it to the game's own loot while the game runs (Mysticism)
      lines.push([sources.lists == 1 ? KnowYourSkyrim.say("In one loot list no record hands out, a script may add it to the game's loot") : KnowYourSkyrim.say("In # loot lists no record hands out, a script may add them to the game's loot", sources.lists)]);
   }
   if(sources.reading == true)
   {
      lines.push([KnowYourSkyrim.say("Reading the plugin files for more places...")]);
   }
   else if(lines.length == 0)
   {
      lines.push([KnowYourSkyrim.say("Nothing in the records gives it, a quest or a script hands it out")]);
   }
   i = 0;
   while(sources.improved != undefined && i < sources.improved.length)
   {
      recipe = sources.improved[i];
      lines.push(KnowYourSkyrim.sayPieces("Improved at #1 with #2", recipe.bench, KnowYourSkyrim.takesPieces(recipe.takes)).concat(KnowYourSkyrim.recipeNeeds(recipe)));
      i++;
   }
   return lines;
};

// Each recipe on a line. Recipes of one bench with the same needs that each take a single thing, a smelter that
// melts down thirty pieces of armor, go on one line of the things any of which gives it
KnowYourSkyrim.recipeLines = function(recipes)
{
   var lines = [];
   var groups = [];
   var byKey = {};
   var recipe;
   var key;
   var names;
   var g;
   var r;
   var i = 0;
   while(recipes != undefined && i < recipes.length)
   {
      recipe = recipes[i];
      key = recipe.takes != undefined && recipe.takes.length == 1 && recipe.takes[0].count == 1 ? recipe.bench + "|" + KnowYourSkyrim.lineText(KnowYourSkyrim.recipeNeeds(recipe)) : undefined;
      if(key != undefined && byKey[key] != undefined)
      {
         groups[byKey[key]].push(recipe);
      }
      else
      {
         if(key != undefined)
         {
            byKey[key] = groups.length;
         }
         groups.push([recipe]);
      }
      i++;
   }
   g = 0;
   while(g < groups.length)
   {
      if(groups[g].length < KnowYourSkyrim.BROWSER_RECIPES_JOINED)
      {
         r = 0;
         while(r < groups[g].length)
         {
            recipe = groups[g][r];
            lines.push(KnowYourSkyrim.sayPieces("Made at #1 from #2", recipe.bench, KnowYourSkyrim.takesPieces(recipe.takes)).concat(KnowYourSkyrim.recipeNeeds(recipe)));
            r++;
         }
      }
      else
      {
         names = [];
         r = 0;
         while(r < groups[g].length)
         {
            names.push({name:groups[g][r].takes[0].name, id:groups[g][r].takes[0].id, level:1});
            r++;
         }
         recipe = groups[g][0];
         lines.push(KnowYourSkyrim.sayPieces("Made at #1 from any of #2", recipe.bench, KnowYourSkyrim.namesPieces(KnowYourSkyrim.cutList(names, 0, KnowYourSkyrim.BROWSER_NAMES_SHOWN, "made" + g))).concat(KnowYourSkyrim.recipeNeeds(recipe)));
      }
      g++;
   }
   return lines;
};

KnowYourSkyrim.browserUseLines = function(sources)
{
   return sources.uses != undefined && sources.uses.length > 0 ? [KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(sources, "uses"))] : [];
};

// What a recipe asks besides its materials as pieces after them: each group of conditions the game joins by "or" (the
// perks of one group under one "needs"), how many it makes, the conditions the page can't tell counted. Improving asks
// Arcane Blacksmith of an enchanted thing only, the plain one of the group says so
KnowYourSkyrim.recipeNeeds = function(recipe)
{
   var parts = [];
   var seen = {};
   var g = 0;
   var pieces;
   var text;
   var i;
   while(recipe.needs != undefined && g < recipe.needs.length)
   {
      pieces = KnowYourSkyrim.needGroupPieces(recipe.needs[g]);
      text = KnowYourSkyrim.lineText(pieces);
      if(pieces.length > 0 && seen[text] != true)
      {
         seen[text] = true;
         parts.push(pieces);
      }
      g++;
   }
   if(recipe.count > 1)
   {
      parts.push([KnowYourSkyrim.say("makes #", recipe.count)]);
   }
   if(recipe.more > 0)
   {
      parts.push([recipe.more == 1 ? KnowYourSkyrim.say("one more condition") : KnowYourSkyrim.say("# more conditions", recipe.more)]);
   }
   pieces = [];
   i = 0;
   while(i < parts.length)
   {
      pieces = pieces.concat([", "], parts[i]);
      i++;
   }
   return pieces;
};

KnowYourSkyrim.needGroupPieces = function(group)
{
   var perks = [];
   var others = [];
   var plain = false;
   var names = {};
   var pieces = [];
   var i = 0;
   var need;
   var name;
   while(i < group.length)
   {
      need = group[i];
      name = KnowYourSkyrim.linkPiece(need.name, need.id);
      if(need.kind == "plain")
      {
         plain = true;
      }
      else if(need.kind == "perk")
      {
         if(names[need.name] != true)
         {
            names[need.name] = true;
            perks.push(name);
         }
      }
      else
      {
         others.push(KnowYourSkyrim.needPieces(need, name));
      }
      i++;
   }
   if(perks.length > 0)
   {
      others.unshift(KnowYourSkyrim.sayPieces("needs #", KnowYourSkyrim.joinedPieces(perks, " " + KnowYourSkyrim.say("or") + " ")));
   }
   pieces = KnowYourSkyrim.joinedPieces(others, " " + KnowYourSkyrim.say("or") + " ");
   if(plain)
   {
      return pieces.length > 0 ? KnowYourSkyrim.sayPieces("#1 for an enchanted one", pieces) : [KnowYourSkyrim.say("an unenchanted one only")];
   }
   return pieces;
};

KnowYourSkyrim.needPieces = function(need, name)
{
   switch(String(need.kind))
   {
      case "spell":
         return KnowYourSkyrim.sayPieces("knowing #", name);
      case "worn":
         return KnowYourSkyrim.sayPieces("wearing #", name);
      case "notWearing":
         return KnowYourSkyrim.sayPieces("not wearing #", name);
      case "unworn":
         return [KnowYourSkyrim.say("not the one you wear")];
      case "carried":
         return need.number > 1 ? KnowYourSkyrim.sayPieces("carrying #1 #2", need.number, name) : KnowYourSkyrim.sayPieces("carrying #", name);
      case "quest":
         return KnowYourSkyrim.sayPieces("after #", name);
      case "stage":
         return KnowYourSkyrim.sayPieces("after stage #1 of #2", need.number, name);
      case "before":
         return KnowYourSkyrim.sayPieces("before stage #1 of #2", need.number, name);
      case "skill":
         return KnowYourSkyrim.sayPieces("#1 #2", name, need.number);
      case "enchanted":
         return [KnowYourSkyrim.say("an enchanted one only")];
   }
   return [];
};

KnowYourSkyrim.joinedPieces = function(lines, between)
{
   var pieces = [];
   var i = 0;
   while(i < lines.length)
   {
      if(i > 0)
      {
         pieces.push(between);
      }
      pieces = pieces.concat(KnowYourSkyrim.isPieces(lines[i]) ? lines[i] : [lines[i]]);
      i++;
   }
   return pieces;
};

KnowYourSkyrim.takesPieces = function(takes)
{
   var pieces = [];
   var i = 0;
   var name;
   while(takes != undefined && i < takes.length)
   {
      if(i > 0)
      {
         pieces.push(", ");
      }
      name = KnowYourSkyrim.linkPiece(takes[i].name, takes[i].id);
      pieces = pieces.concat(takes[i].count > 1 ? KnowYourSkyrim.sayPieces("#1 #2", takes[i].count, name) : [name]);
      i++;
   }
   return pieces.length > 0 ? pieces : [KnowYourSkyrim.say("nothing")];
};

// names with the level of the player each comes at from, and how many more there are. A name the browser has an
// entry for is a link to it
KnowYourSkyrim.namesPieces = function(cut)
{
   var pieces = [];
   var apart = KnowYourSkyrim.maskedApart(cut.items);
   var names = apart.known;
   var i = 0;
   var name;
   while(i < names.length)
   {
      if(i > 0)
      {
         pieces.push(", ");
      }
      name = KnowYourSkyrim.linkPiece(names[i].name, names[i].id);
      if(names[i].by != undefined)
      {
         pieces = pieces.concat([name, " " + KnowYourSkyrim.handoutWords(names[i], true)]);
      }
      else
      {
         pieces = pieces.concat(names[i].level > 1 ? KnowYourSkyrim.sayPieces("#1 (from level #2)", name, names[i].level) : [name]);
      }
      i++;
   }
   return KnowYourSkyrim.maskPieces(pieces, apart.masks).concat(KnowYourSkyrim.cutTail(cut, ", ", KnowYourSkyrim.say("# more", cut.more)));
};

// The mod's framework that hands a thing out while the game runs and how sure it is, "(SPID, 20%)", after the
// level a list gives it from. The records give the rest for sure and say nothing
KnowYourSkyrim.handoutWords = function(entry, withBy)
{
   var words = [];
   if(withBy && entry.level > 1)
   {
      words.push(KnowYourSkyrim.say("from level #", entry.level));
   }
   if(withBy)
   {
      words.push(entry.by);
   }
   if(entry.chance != undefined)
   {
      words.push(KnowYourSkyrim.say("#%", entry.chance));
   }
   if(entry.some == true)
   {
      words.push(KnowYourSkyrim.say("under some conditions"));
   }
   return words.length > 0 ? "(" + words.join(", ") + ")" : "";
};

// A SPID line the browser can name nobody for, by its filters as the mod's file writes them: a line for everyone, or
// one that asks for a place or a single person in the world
KnowYourSkyrim.toldLines = function(told)
{
   var lines = [];
   var i = 0;
   var one;
   var who;
   var chance;
   while(told != undefined && i < told.length)
   {
      one = told[i];
      who = one.filters == "" ? KnowYourSkyrim.say("everyone") : KnowYourSkyrim.say("those that match #", one.filters);
      chance = KnowYourSkyrim.handoutWords(one, false);
      lines.push([KnowYourSkyrim.say(one.death == true ? "#1 gives it to #2 when they die" : "#1 gives it to #2", one.by, who) + (chance != "" ? " " + chance : "")]);
      i++;
   }
   return lines;
};

// A list of the page shows its first few, a press on the count of the rest opens it whole and "show less" shuts it
// again. The plugin sends the first few of every list until one is opened, then every list up to its cap
KnowYourSkyrim.cutList = function(list, more, shown, key)
{
   var items = list != undefined ? list : [];
   var total = items.length + KnowYourSkyrim.number(more);
   // words to find on the page open every list, and none of them shuts meanwhile
   var open = KnowYourSkyrim.browserOpen[key] == true || KnowYourSkyrim.findAll;
   var cut = shown != undefined && items.length > shown;
   if(!open && cut)
   {
      items = items.slice(0, shown);
   }
   return {items:items, more:total - items.length, open:open, shuts:open && cut && !KnowYourSkyrim.findAll, key:key};
};

KnowYourSkyrim.sourceCut = function(sources, key)
{
   return KnowYourSkyrim.cutList(sources[key], sources[key + "More"], sources[key + "Shown"], key);
};

// What follows the last name of a list: the count of the rest as a press that opens it, or the press that shuts it. An
// open list past the plugin's cap tells how many are still left out. counted is the words of that count
KnowYourSkyrim.cutTail = function(cut, before, counted)
{
   var tail = [];
   if(!cut.open)
   {
      return cut.more > 0 ? [before, {text:counted, open:cut.key}] : [];
   }
   if(cut.more > 0)
   {
      tail.push(before + counted);
   }
   if(cut.shuts)
   {
      tail.push(tail.length > 0 ? "   " : before);
      tail.push({text:KnowYourSkyrim.say("show less"), shut:cut.key});
   }
   return tail;
};

KnowYourSkyrim.anyListOpen = function()
{
   if(KnowYourSkyrim.findAll)
   {
      return true;
   }
   for(var key in KnowYourSkyrim.browserOpen)
   {
      if(KnowYourSkyrim.browserOpen[key] == true)
      {
         return true;
      }
   }
   return false;
};

KnowYourSkyrim.openListKeys = function()
{
   var keys = [];
   for(var key in KnowYourSkyrim.browserOpen)
   {
      if(KnowYourSkyrim.browserOpen[key] == true)
      {
         keys.push(key);
      }
   }
   return keys;
};

// a list opened asks the plugin for every list whole once, the page stays where it was
KnowYourSkyrim.openBrowserList = function(key, open)
{
   var told = KnowYourSkyrim.browserItem;
   KnowYourSkyrim.browserOpen[key] = open;
   if(open && KnowYourSkyrim.browserSources != undefined && KnowYourSkyrim.browserSources.whole != true)
   {
      KnowYourSkyrim.browserSources = KnowYourSkyrim.plugin().BrowserSources(told.id, true);
   }
   KnowYourSkyrim.drawBrowserPage(true);
   KnowYourSkyrim.keepBrowser();
   gfx.io.GameDelegate.call("PlaySound", ["UIMenuFocus"]);
};

// the mods that change an entry after the one it comes from, two named beside the tile, all of them on the page
KnowYourSkyrim.editsCut = function(told)
{
   return KnowYourSkyrim.cutList(told.edits, 0, KnowYourSkyrim.BROWSER_EDITS_NAMED, "edits");
};

// A piece of a line is words, or words with the id of an entry or with words to search for. A line is a list of them
KnowYourSkyrim.linkPiece = function(text, id)
{
   var mask;
   if(id == undefined || id <= 0)
   {
      return String(text);
   }
   mask = KnowYourSkyrim.maskOf(id);
   return mask != undefined ? mask : {text:String(text), id:id};
};

// In the progression mode a name of an entry the character hasn't met says what it is and leaves its name and its
// link out: "a place you haven't found (Whiterun Hold)", "someone you haven't met (People, Merchants)". Nothing for
// an entry met, and for every entry while the mode is off
KnowYourSkyrim.maskOf = function(id)
{
   var info = KnowYourSkyrim.maskInfo(id);
   return info != undefined ? KnowYourSkyrim.maskWords(info, 1) : undefined;
};

// what an entry not met is, a place, someone, a creature, a quest or a thing, and the words of its kind after it
KnowYourSkyrim.maskInfo = function(id)
{
   var plugin = KnowYourSkyrim.plugin();
   var told = id != undefined && id > 0 && plugin != undefined && plugin.BrowserMask != undefined ? plugin.BrowserMask(Number(id)) : undefined;
   var kind;
   if(told == undefined)
   {
      return undefined;
   }
   kind = KnowYourSkyrim.browserKind({family:String(told.family), chips:told.chips != undefined ? told.chips : []}).split(", ");
   switch(String(told.family))
   {
      case "places":
         return {what:"place", hint:told.around != undefined && String(told.around) != "" ? String(told.around) : kind.slice(1).join(", ")};
      case "living":
         return {what:KnowYourSkyrim.has(told.chips, "people") ? "someone" : "creature", hint:kind.slice(1).join(", ")};
      case "quests":
         return {what:"quest", hint:kind.slice(1).join(", ")};
   }
   return {what:"thing", hint:kind.join(", ")};
};

// "a place you haven't found (Whiterun Hold)", several of a kind "3 places you haven't found (Whiterun Hold)"
KnowYourSkyrim.maskWords = function(info, count)
{
   var words;
   switch(info.what)
   {
      case "place":
         words = count > 1 ? KnowYourSkyrim.say("# places you haven't found", count) : KnowYourSkyrim.say("a place you haven't found");
         break;
      case "someone":
         words = count > 1 ? KnowYourSkyrim.say("# people you haven't met", count) : KnowYourSkyrim.say("someone you haven't met");
         break;
      case "creature":
         words = count > 1 ? KnowYourSkyrim.say("# creatures you haven't met", count) : KnowYourSkyrim.say("a creature you haven't met");
         break;
      case "quest":
         words = count > 1 ? KnowYourSkyrim.say("# quests you haven't begun", count) : KnowYourSkyrim.say("a quest you haven't begun");
         break;
      default:
         words = count > 1 ? KnowYourSkyrim.say("# things you haven't found", count) : KnowYourSkyrim.say("something you haven't found");
   }
   return info.hint != "" ? words + " (" + info.hint + ")" : words;
};

// The names of a list the character met keep their order, the ones not met are counted by their kind for after them:
// a merchant's stock read as a wall of "something you haven't found (Apparel, Clothing, Body)"
KnowYourSkyrim.maskedApart = function(items)
{
   var known = [];
   var masks = [];
   var at = {};
   var info;
   var key;
   var i = 0;
   while(i < items.length)
   {
      info = KnowYourSkyrim.maskInfo(items[i].id);
      if(info == undefined)
      {
         known.push(items[i]);
      }
      else
      {
         key = info.what + "|" + info.hint;
         if(at[key] == undefined)
         {
            at[key] = masks.length;
            masks.push({info:info, count:0});
         }
         masks[at[key]].count++;
      }
      i++;
   }
   return {known:known, masks:masks};
};

// the counted kinds after the names, "Iron Dagger, Gold, 6 things you haven't found (Apparel, Clothing, Body)"
KnowYourSkyrim.maskPieces = function(pieces, masks)
{
   var i = 0;
   while(i < masks.length)
   {
      if(pieces.length > 0)
      {
         pieces.push(", ");
      }
      pieces.push(KnowYourSkyrim.maskWords(masks[i].info, masks[i].count));
      i++;
   }
   return pieces;
};

KnowYourSkyrim.isPieces = function(value)
{
   return value != undefined && typeof(value) == "object" && value.length != undefined && value.text == undefined;
};

// say() with values that may be pieces, the words of the language around them
KnowYourSkyrim.sayPieces = function(words)
{
   var pattern = KnowYourSkyrim.say(words);
   var pieces = [];
   var next = 1;
   var i = 0;
   var at;
   var number;
   var value;
   var k;
   while(i < pattern.length)
   {
      at = pattern.indexOf("#", i);
      if(at < 0)
      {
         pieces.push(pattern.substring(i));
         break;
      }
      if(at > i)
      {
         pieces.push(pattern.substring(i, at));
      }
      number = Number(pattern.charAt(at + 1));
      if(pattern.charAt(at + 1) >= "1" && pattern.charAt(at + 1) <= "9")
      {
         value = arguments[number];
         i = at + 2;
      }
      else
      {
         value = arguments[next];
         next++;
         i = at + 1;
      }
      if(KnowYourSkyrim.isPieces(value))
      {
         k = 0;
         while(k < value.length)
         {
            pieces.push(value[k]);
            k++;
         }
      }
      else
      {
         pieces.push(value != undefined && typeof(value) == "object" ? value : String(value));
      }
   }
   return pieces;
};

KnowYourSkyrim.lineText = function(pieces)
{
   var text = "";
   var i = 0;
   if(!KnowYourSkyrim.isPieces(pieces))
   {
      return String(pieces);
   }
   while(i < pieces.length)
   {
      text += typeof(pieces[i]) == "object" ? pieces[i].text : String(pieces[i]);
      i++;
   }
   return text;
};
