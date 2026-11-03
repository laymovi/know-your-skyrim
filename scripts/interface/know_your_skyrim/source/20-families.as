// The categories of the browser, the words SkyUI has for them where it has them. Favorites holds the starred entries
// of every category
KnowYourSkyrim.BROWSER_FAMILIES = [
   {key:"", words:"$All", fallback:"All", icon:"browser"},
   {key:"favorites", words:"$Favorites", fallback:"Favorites", icon:"star"},
   {key:"weapons", words:"$Weapons", fallback:"Weapons", icon:"sword"},
   {key:"armor", words:"$Apparel", fallback:"Apparel", icon:"cuirass"},
   {key:"potions", words:"$Potions", fallback:"Potions", icon:"potion"},
   {key:"scrolls", words:"$Scrolls", fallback:"Scrolls", icon:"scroll"},
   {key:"food", words:"$Food", fallback:"Food", icon:"food"},
   {key:"ingredients", words:"$Ingredients", fallback:"Ingredients", icon:"ingredient"},
   {key:"books", words:"$Books", fallback:"Books", icon:"book"},
   {key:"keys", words:"$Keys", fallback:"Keys", icon:"key"},
   {key:"misc", words:"$Misc", fallback:"Misc", icon:"sack"},
   {key:"living", words:"$Living", fallback:"Living", icon:"living"},
   {key:"quests", words:"$Quests", fallback:"Quests", icon:"quest"},
   {key:"places", words:"$Places", fallback:"Places", icon:"pin"},
   {key:"perks", words:"$Perks", fallback:"Perks", icon:"tome"},
   {key:"pseudo", words:"$Pseudo-items", fallback:"Pseudo-items", icon:"ghost"}
];

// The chips of the item categories are the ones of the panel's strip over the list, the same words and signs. People
// and creatures go by the kind of their race, the plugin sorts them by its keywords. Then what people do for the
// player, by the factions of the game every follower mod uses too. One chip of the two lines at a time, because a role
// is people's already and Dragons with Merchants found no one and looked like a chip that wouldn't let go
KnowYourSkyrim.CHIP_ROWS.living = [
   [
      {key:"people", words:"$People", fallback:"People", icon:"hero", alone:true},
      {key:"animals", words:"$Animals", fallback:"Animals", icon:"paw", alone:true},
      {key:"undead", words:"$Undead", fallback:"Undead", icon:"skull", alone:true},
      {key:"daedra", words:"$Daedra", fallback:"Daedra", icon:"horns", alone:true},
      {key:"dragons", words:"$Dragons", fallback:"Dragons", icon:"dragon", alone:true},
      {key:"automatons", words:"$Automatons", fallback:"Automatons", icon:"cog", alone:true},
      {key:"otherLiving", words:"$Other", fallback:"Other", icon:"living", alone:true}
   ],
   [
      {key:"merchants", words:"$Merchants", fallback:"Merchants", icon:"coin", alone:true},
      {key:"hirelings", words:"$Hirelings", fallback:"Hirelings", icon:"sword", alone:true},
      {key:"followers", words:"$Followers", fallback:"Followers", icon:"people", alone:true},
      {key:"trainers", words:"$Trainers", fallback:"Trainers", icon:"book", alone:true},
      {key:"story", words:"$Story", fallback:"Story", icon:"star", alone:true}
   ]
];

// Quests by the kind the journal sorts them into, then by how far the player is with them: not done (not begun,
// waiting for the player or stopped), in progress, done. A kind and a progress go together
KnowYourSkyrim.CHIP_ROWS.quests = [
   [
      {key:"mainQuests", words:"$Main", fallback:"Main", icon:"star"},
      {key:"factionQuests", words:"$Factions", fallback:"Factions", icon:"people"},
      {key:"daedricQuests", words:"$Daedric", fallback:"Daedric", icon:"horns"},
      {key:"sideQuests", words:"$Side", fallback:"Side", icon:"scroll"},
      {key:"miscQuests", words:"$Miscellaneous", fallback:"Miscellaneous", icon:"note"},
      {key:"otherQuests", words:"$Other", fallback:"Other", icon:"quest"}
   ],
   [
      {key:"unfinishedQuests", words:"$Unfinished", fallback:"Unfinished", icon:"cross"},
      {key:"activeQuests", words:"$In progress", fallback:"In progress", icon:"hourglass"},
      {key:"doneQuests", words:"$Done", fallback:"Done", icon:"check"}
   ]
];

// places by the kind the game's keywords give their locations
KnowYourSkyrim.CHIP_ROWS.places = [
   [
      {key:"hereNow", words:"$Here", fallback:"Here", icon:"pin", page:true},
      {key:"holds", words:"$Holds", fallback:"Holds", icon:"map"},
      {key:"towns", words:"$Towns", fallback:"Towns", icon:"people"},
      {key:"shops", words:"$Shops", fallback:"Shops", icon:"coin"},
      {key:"homes", words:"$Homes", fallback:"Homes", icon:"key"},
      {key:"dungeons", words:"$Dungeons", fallback:"Dungeons", icon:"skull"},
      {key:"otherPlaces", words:"$Other", fallback:"Other", icon:"pin"}
   ]
];

// Perks by the tree they stand in, one tree at a time: the skills of the warrior, the thief and the mage in the game's
// order, a line each, then the vampire's and the werewolf's trees and any other. The words of a skill are the game's
// own once Perks first shows (nameTreeChips), these stand in until then
KnowYourSkyrim.CHIP_ROWS.perks = [
   [
      {key:"oneHandedPerks", words:"", fallback:"One-handed", icon:"sword", alone:true},
      {key:"twoHandedPerks", words:"", fallback:"Two-handed", icon:"battleaxe", alone:true},
      {key:"archeryPerks", words:"", fallback:"Archery", icon:"bow", alone:true},
      {key:"blockPerks", words:"", fallback:"Block", icon:"shield", alone:true},
      {key:"smithingPerks", words:"", fallback:"Smithing", icon:"ingot", alone:true},
      {key:"heavyArmorPerks", words:"", fallback:"Heavy Armor", icon:"plate", alone:true}
   ],
   [
      {key:"lightArmorPerks", words:"", fallback:"Light Armor", icon:"jerkin", alone:true},
      {key:"pickpocketPerks", words:"", fallback:"Pickpocket", icon:"sack", alone:true},
      {key:"lockpickingPerks", words:"", fallback:"Lockpicking", icon:"key", alone:true},
      {key:"sneakPerks", words:"", fallback:"Sneak", icon:"eyeShut", alone:true},
      {key:"alchemyPerks", words:"", fallback:"Alchemy", icon:"potion", alone:true},
      {key:"speechPerks", words:"", fallback:"Speech", icon:"coin", alone:true}
   ],
   [
      {key:"alterationPerks", words:"", fallback:"Alteration", icon:"ward", alone:true},
      {key:"conjurationPerks", words:"", fallback:"Conjuration", icon:"horns", alone:true},
      {key:"destructionPerks", words:"", fallback:"Destruction", icon:"staff", alone:true},
      {key:"illusionPerks", words:"", fallback:"Illusion", icon:"eye", alone:true},
      {key:"restorationPerks", words:"", fallback:"Restoration", icon:"heart", alone:true},
      {key:"enchantingPerks", words:"", fallback:"Enchanting", icon:"soulgem", alone:true}
   ],
   [
      {key:"vampirePerks", words:"", fallback:"Vampire", icon:"dragon", alone:true},
      {key:"werewolfPerks", words:"", fallback:"Werewolf", icon:"paw", alone:true},
      {key:"otherPerks", words:"$Other", fallback:"Other", icon:"star", alone:true}
   ]
];

// The chips of the trees take the names the game gives the actor values of its trees, asked of the plugin once, the
// first time the chips of Perks are laid or told. A tree the game names nothing keeps its words
KnowYourSkyrim.nameTreeChips = function()
{
   var plugin = KnowYourSkyrim.plugin();
   var trees;
   var rows = KnowYourSkyrim.CHIP_ROWS.perks;
   var t = 0;
   var g;
   var c;
   if(KnowYourSkyrim.treesNamed)
   {
      return undefined;
   }
   KnowYourSkyrim.treesNamed = true;
   trees = plugin != undefined && plugin.BrowserPerkTrees != undefined ? plugin.BrowserPerkTrees() : undefined;
   while(trees != undefined && t < trees.length)
   {
      g = 0;
      while(g < rows.length)
      {
         c = 0;
         while(c < rows[g].length)
         {
            if(rows[g][c].key == trees[t].key && trees[t].name != undefined && trees[t].name != "")
            {
               rows[g][c].fallback = String(trees[t].name);
               rows[g][c].words = "";
            }
            c++;
         }
         g++;
      }
      t++;
   }
};

// the chips of a category to lay or to tell, the trees of Perks in the game's words
KnowYourSkyrim.nameChipsOf = function(key)
{
   if(key == "perks")
   {
      KnowYourSkyrim.nameTreeChips();
   }
   return KnowYourSkyrim.CHIP_ROWS[key];
};

// how far the player is with a quest, as the plugin counts it
KnowYourSkyrim.questState = function(state)
{
   switch(Number(state))
   {
      case 1:
         return KnowYourSkyrim.say("Waiting for you");
      case 2:
         return KnowYourSkyrim.say("In progress");
      case 3:
         return KnowYourSkyrim.say("Done");
      case 4:
         return KnowYourSkyrim.say("Stopped");
   }
   return KnowYourSkyrim.say("Not begun");
};
