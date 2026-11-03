// The page of a perk of a skill tree. The plugin sends every rank with its description and what it asks, the perks
// right before and after it in the tree, the recipes and perks that ask for any rank of it, and the people whose
// records have it. Beside the tile stand how many ranks it has and the one the player has

KnowYourSkyrim.perkNumbers = function(told)
{
   return told.ranks > 1 ? KnowYourSkyrim.say("# ranks", told.ranks) : "";
};

// whether the player has a perk and which rank, gold with a tick when they do
KnowYourSkyrim.perkHeadLine = function(told)
{
   var rank = KnowYourSkyrim.number(told.rank);
   if(rank <= 0)
   {
      return {pieces:[KnowYourSkyrim.say("You don't have it")], size:13, color:KnowYourSkyrim.HERO_DIM};
   }
   return {pieces:[told.ranks > 1 ? KnowYourSkyrim.say("You have rank #1 of #2", rank, told.ranks) : KnowYourSkyrim.say("You have it")], size:13, color:KnowYourSkyrim.GOLD, sign:"check", alpha:90};
};

// A perk of one rank tells its description and what it asks, one of several a line for each rank with both. Then its
// neighbours in the tree, what it unlocks and who has it
KnowYourSkyrim.perkSections = function(sources)
{
   var sections = [];
   var ranks;
   var lines;
   if(sources == undefined)
   {
      return [{head:KnowYourSkyrim.say("Description"), lines:[[KnowYourSkyrim.say("Reading the records...")]], size:13, color:KnowYourSkyrim.HERO_DIM}];
   }
   ranks = sources.ranks != undefined ? sources.ranks : [];
   if(ranks.length > 1)
   {
      sections.push({head:KnowYourSkyrim.say("Ranks"), lines:KnowYourSkyrim.perkRankLines(ranks), size:13, color:KnowYourSkyrim.WHITE});
   }
   else
   {
      if(sources.description != undefined && sources.description != "")
      {
         sections.push({head:KnowYourSkyrim.say("Description"), lines:KnowYourSkyrim.descriptionLines(sources.description), size:13, color:KnowYourSkyrim.WHITE});
      }
      else if(sources.reading == true)
      {
         sections.push({head:KnowYourSkyrim.say("Description"), lines:[[KnowYourSkyrim.say("Reading the descriptions...")]], size:13, color:KnowYourSkyrim.HERO_DIM});
      }
      lines = ranks.length > 0 ? KnowYourSkyrim.perkNeedLines(ranks[0]) : [];
      if(lines.length > 0)
      {
         sections.push({head:KnowYourSkyrim.say("Requirements"), lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
      }
   }
   lines = KnowYourSkyrim.perkTreeLines(sources);
   if(lines.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("In the tree"), lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   lines = KnowYourSkyrim.perkUnlockLines(sources);
   if(lines.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Unlocks"), lines:lines, size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   if(sources.people != undefined && sources.people.length > 0)
   {
      sections.push({head:KnowYourSkyrim.say("Who has it"), lines:[KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "people"))], size:13, color:KnowYourSkyrim.PARCHMENT});
   }
   return sections;
};

// Each rank with its description and in brackets what it asks besides the rank before it. The ranks the player has
// are ticked, the first one they lack is the one to take next
KnowYourSkyrim.perkRankLines = function(ranks)
{
   var lines = [];
   var next = true;
   var rank;
   var bare;
   var needs;
   var line;
   var i = 0;
   while(i < ranks.length)
   {
      rank = ranks[i];
      bare = rank.description != undefined && rank.description != "" ? KnowYourSkyrim.sayPieces("Rank #1 of #2: #3", rank.rank, ranks.length, rank.description) : KnowYourSkyrim.sayPieces("Rank #1 of #2", rank.rank, ranks.length);
      needs = KnowYourSkyrim.recipeNeeds(rank);
      if(needs.length > 0)
      {
         bare = bare.concat([" ("], needs.slice(1), [")"]);
      }
      line = rank.has == true ? bare.concat([" " + KnowYourSkyrim.say("(yours)")]) : bare;
      line.mark = rank.has == true ? "done" : (next ? "open" : "later");
      line.bare = bare;
      next = next && rank.has == true;
      lines.push(line);
      i++;
   }
   return lines;
};

// What a rank asks, a group of conditions the game joins by "or" to a line, the ones the page can't word counted
KnowYourSkyrim.perkNeedLines = function(rank)
{
   var lines = [];
   var pieces;
   var g = 0;
   while(rank.needs != undefined && g < rank.needs.length)
   {
      pieces = KnowYourSkyrim.needGroupPieces(rank.needs[g]);
      if(pieces.length > 0)
      {
         lines.push(KnowYourSkyrim.capitalFirst(pieces));
      }
      g++;
   }
   if(rank.more > 0)
   {
      lines.push(KnowYourSkyrim.capitalFirst([rank.more == 1 ? KnowYourSkyrim.say("one more condition") : KnowYourSkyrim.say("# more conditions", rank.more)]));
   }
   return lines;
};

// the perks right before it in the tree and right after it, each a link
KnowYourSkyrim.perkTreeLines = function(sources)
{
   var lines = [];
   if(sources.parents != undefined && sources.parents.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Comes after #", KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "parents"))));
   }
   if(sources.children != undefined && sources.children.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Leads to #", KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "children"))));
   }
   return lines;
};

// What asks for any rank of it: the things recipes make with their bench, the things it lets be improved, the perks
// that need it. Each leads to its page
KnowYourSkyrim.perkUnlockLines = function(sources)
{
   var lines = [];
   if(sources.recipes != undefined && sources.recipes.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Recipes: #", KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "recipes"))));
   }
   if(sources.improving != undefined && sources.improving.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Improving: #", KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "improving"))));
   }
   if(sources.neededBy != undefined && sources.neededBy.length > 0)
   {
      lines.push(KnowYourSkyrim.sayPieces("Perks that need it: #", KnowYourSkyrim.peoplePieces(KnowYourSkyrim.sourceCut(sources, "neededBy"))));
   }
   return lines;
};
