// Runs the rules of the item browser outside the game: what a search turns into, how the details and the ways
// to get an item read
//   node scripts/interface/know_your_skyrim/tests/browser.test.cjs

const fs = require("fs");
const path = require("path");
const {browserParts, inventory, world, run, clip, said, done} = require("./movie.cjs");

const source = path.join(__dirname, "..", "source");
const parts = browserParts();
// every part but the start, which looks for the menu
const movie = run(world(inventory().root).context(), parts.filter((part) => part.name != "40-start.as"));
const KnowYourSkyrim = movie._global.KnowYourSkyrim;
// arrays made in the context of the parts compare by their contents here
const plain = (value) => JSON.parse(JSON.stringify(value));

// what the player types becomes terms for the plugin
said("two words are two names", plain(KnowYourSkyrim.browserTerms("iron sword")), [["name", "iron", false], ["name", "sword", false]]);
said("signs of mod, keyword and effect, a minus leaves out",
   plain(KnowYourSkyrim.browserTerms("@Dawnguard #ArmorHeavy $fire -steel")),
   [["mod", "Dawnguard", false], ["keyword", "ArmorHeavy", false], ["effect", "fire", false], ["name", "steel", true]]);
said("quotes keep words together, a minus before a sign", plain(KnowYourSkyrim.browserTerms("\"ebony mail\" -@update")), [["name", "ebony mail", false], ["mod", "update", true]]);
said("spaces alone are nothing", plain(KnowYourSkyrim.browserTerms("   ")), []);
said("a sign alone is nothing", plain(KnowYourSkyrim.browserTerms("@ #")), []);
said("a quote left open runs to the end", plain(KnowYourSkyrim.browserTerms("-\"glass bow")), [["name", "glass bow", true]]);

// the chips a player left come back in their groups
said("armor chips into their groups", plain(KnowYourSkyrim.keptChips("armor", "heavy,head")), ["heavy", "head"]);
said("a slot chip alone keeps the first group empty", plain(KnowYourSkyrim.keptChips("armor", "feet")), [null, "feet"]);
said("a chip of another category is dropped", plain(KnowYourSkyrim.keptChips("weapons", "heavy")), []);
said("a category with no chips", plain(KnowYourSkyrim.keptChips("keys", "")), []);

// the lines beside the tile
said("kind of a heavy cuirass", KnowYourSkyrim.browserKind({family: "armor", chips: ["heavy", "body"]}), "Apparel, Heavy, Body");
said("kind of a key", KnowYourSkyrim.browserKind({family: "keys", chips: []}), "Keys");
said("numbers of a weapon", KnowYourSkyrim.browserNumbers({damage: 7, weight: 9, value: 25}), "Damage 7   Weight 9   Value 25");
said("numbers of a cuirass with its slot", KnowYourSkyrim.browserNumbers({armor: 25.4, weight: 30, value: 125, partMask: 4}), "Armor 25   Weight 30   Value 125   Slot 32");
said("a helmet takes two slots", KnowYourSkyrim.browserNumbers({armor: 15, partMask: 3}), "Armor 15   Slots 30, 31");
// a line is a list of pieces, words or links, and reads as one text
const texts = (lines) => plain(lines).map((line) => KnowYourSkyrim.lineText(line));

said("origin with the plugins that change it", KnowYourSkyrim.lineText(KnowYourSkyrim.browserOrigin({id: 0x12E46, mod: "Skyrim.esm", edits: ["Update.esm", "USSEP.esp"]})), "Skyrim.esm, changed by Update.esm, USSEP.esp   00012E46");
{
   const many = {id: 0xA2C94, mod: "Skyrim", edits: ["USSEP", "Bijin Warmaidens", "CK_Lydia_Replacer", "IFDL"]};
   said("many plugins that change it are counted", KnowYourSkyrim.lineText(KnowYourSkyrim.browserOrigin(many)), "Skyrim, changed by USSEP, Bijin Warmaidens and 2 more   000A2C94");
   said("the count opens all of them", plain(KnowYourSkyrim.browserOrigin(many).filter((piece) => typeof piece == "object")), [{text: "2 more", open: "edits"}]);
   KnowYourSkyrim.browserOpen = {edits: true};
   said("all of them with a press that shuts them", texts([[many.edits.join(", ")].concat(KnowYourSkyrim.cutTail(KnowYourSkyrim.editsCut(many), "   ", ""))]),
      ["USSEP, Bijin Warmaidens, CK_Lydia_Replacer, IFDL   show less"]);
   KnowYourSkyrim.browserOpen = {};
}
said("origin of a light plugin's form", KnowYourSkyrim.lineText(KnowYourSkyrim.browserOrigin({id: 0xFE001802, mod: "Lantern.esp", edits: []})), "Lantern.esp   FE001802");

// effects, the name of an effect searches for it
said("effects in the game's words, the unknown ones counted on one line after them", texts(KnowYourSkyrim.browserEffectLines([
   {name: "Fortify Smithing", words: "Weapons and armor can be improved 15% better.", known: true},
   {name: "", words: "", known: false},
   {name: "Flames", words: "", known: true},
   {name: "", words: "", known: false},
])), ["Fortify Smithing: Weapons and armor can be improved 15% better.", "Flames", "2 effects you haven't found yet"]);
said("an ingredient with nothing found", texts(KnowYourSkyrim.browserEffectLines([{name: "", words: "", known: false}])), ["One effect you haven't found yet"]);
said("the name of an effect is a search for it", plain(KnowYourSkyrim.browserEffectLines([{name: "Fortify Smithing", words: "Better.", known: true}])[0][0]),
   {text: "Fortify Smithing", search: "$\"Fortify Smithing\""});
said("keywords search for themselves", plain(KnowYourSkyrim.keywordPieces(["ArmorHeavy", "VendorItemArmor"])),
   [{text: "ArmorHeavy", search: "#ArmorHeavy"}, ", ", {text: "VendorItemArmor", search: "#VendorItemArmor"}]);

// pieces in the words of the language
said("values by number in their own order", plain(KnowYourSkyrim.sayPieces("#2 then #1", "a", "b")), ["b", " then ", "a"]);
said("a link among the words", plain(KnowYourSkyrim.sayPieces("Sold by #", [{text: "Belethor", id: 0x1A67C}])), ["Sold by ", {text: "Belethor", id: 0x1A67C}]);
said("a name with no entry is plain words", plain(KnowYourSkyrim.linkPiece("Forge", 0)), "Forge");
const whole = (names, more) => KnowYourSkyrim.cutList(names, more, undefined, "names");
said("names that have an entry are links", plain(KnowYourSkyrim.namesPieces(whole([{name: "Belethor", level: 1, id: 0x1A67C}, {name: "Riverwood Trader", level: 3, id: 0}], 0))),
   [{text: "Belethor", id: 0x1A67C}, ", ", "Riverwood Trader", " (from level ", "3", ")"]);
said("what a recipe takes links each thing", plain(KnowYourSkyrim.takesPieces([{name: "Iron Ingot", count: 2, id: 0x5ACE4}, {name: "Leather Strips", count: 1, id: 0x800E4}])),
   ["2", " ", {text: "Iron Ingot", id: 0x5ACE4}, ", ", {text: "Leather Strips", id: 0x800E4}]);

// where to get it
const none = {made: [], improved: [], uses: [], usesMore: 0, sold: [], soldMore: 0, carried: [], carriedMore: 0, dropped: [], droppedMore: 0,
   harvested: [], harvestedMore: 0, chests: [], chestsMore: 0, lies: [], liesMore: 0, common: 0, lists: 0};
const sources = (more) => Object.assign({}, none, more);

said("names with levels and how many more", KnowYourSkyrim.lineText(KnowYourSkyrim.namesPieces(whole([{name: "Adrianne Avenicci", level: 1}, {name: "Eorlund Gray-Mane", level: 12}], 2))),
   "Adrianne Avenicci, Eorlund Gray-Mane (from level 12), 2 more");
said("the count of the rest opens the list", plain(KnowYourSkyrim.namesPieces(whole([{name: "Adrianne Avenicci", level: 1}], 2)).slice(-1)[0]), {text: "2 more", open: "names"});

// a list opened shows what the plugin sent whole and a press that shuts it, shut again it shows the first few
{
   const smiths = {sold: [{name: "Adrianne", level: 1}, {name: "Eorlund", level: 1}, {name: "Balimund", level: 1}, {name: "Ghorza", level: 1}], soldMore: 2, soldShown: 2};
   said("a shut list shows its first few", KnowYourSkyrim.lineText(KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(smiths, "sold"))), "Adrianne, Eorlund, 4 more");
   KnowYourSkyrim.browserOpen = {sold: true};
   const open = KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut(smiths, "sold"));
   said("an open list shows all it has and what the cap left out", KnowYourSkyrim.lineText(open), "Adrianne, Eorlund, Balimund, Ghorza, 2 more   show less");
   said("the last press shuts it", plain(open.slice(-1)[0]), {text: "show less", shut: "sold"});
   const short = KnowYourSkyrim.namesPieces(KnowYourSkyrim.sourceCut({sold: [{name: "Adrianne", level: 1}], soldMore: 0, soldShown: 2}, "sold"));
   said("a list too short to cut has nothing to shut", KnowYourSkyrim.lineText(short), "Adrianne");
   KnowYourSkyrim.browserOpen = {};
   said("no list open", KnowYourSkyrim.anyListOpen(), false);
   KnowYourSkyrim.browserOpen = {lies: true};
   said("a list open", KnowYourSkyrim.anyListOpen(), true);
   said("places opened end in a line that shuts them", texts(KnowYourSkyrim.placeLines(KnowYourSkyrim.say("Lies in #"), undefined, KnowYourSkyrim.sourceCut({lies: [{name: "A"}, {name: "B"}], liesMore: 0, liesShown: 1}, "lies"))),
      ["Lies in A", "Lies in B", "show less"]);
   KnowYourSkyrim.browserOpen = {};
}

// the chips of a category, one of a group, a shield is no part of the body
{
   const rows = KnowYourSkyrim.CHIP_ROWS.armor;
   const chosen = [];
   KnowYourSkyrim.pickChip(chosen, rows, 1, "head");
   KnowYourSkyrim.pickChip(chosen, rows, 0, "heavy");
   said("heavy and head add up", plain(chosen), ["heavy", "head"]);
   KnowYourSkyrim.pickChip(chosen, rows, 0, "shield");
   said("a shield lets the parts of the body go", plain(chosen), ["shield", null]);
   KnowYourSkyrim.pickChip(chosen, rows, 1, "feet");
   said("a part of the body lets the shield go", plain(chosen), [null, "feet"]);
   KnowYourSkyrim.pickChip(chosen, rows, 1, "feet");
   said("the same chip again lets its group go", plain(chosen), [null, null]);
   const living = [];
   KnowYourSkyrim.pickChip(living, KnowYourSkyrim.CHIP_ROWS.living, 0, "dragons");
   KnowYourSkyrim.pickChip(living, KnowYourSkyrim.CHIP_ROWS.living, 1, "merchants");
   said("one chip of Living at a time, Merchants lets Dragons go", plain(living), [null, "merchants"]);
   KnowYourSkyrim.pickChip(living, KnowYourSkyrim.CHIP_ROWS.living, 0, "people");
   said("and People lets Merchants go", plain(living), ["people", null]);
}
const perk = (name) => ({kind: "perk", name, number: 0});
said("a forge recipe, a merchant and the grindstone", texts(KnowYourSkyrim.sourceLines(sources({
   made: [{bench: "Forge", count: 1, takes: [{name: "Iron Ingot", count: 2}, {name: "Leather Strips", count: 1}], needs: [[perk("Steel Smithing")]], more: 0}],
   improved: [{bench: "Grindstone", count: 1, takes: [{name: "Iron Ingot", count: 1}], needs: [[{kind: "plain", name: "", number: 0}, perk("Arcane Blacksmith")]], more: 1}],
   sold: [{name: "Adrianne Avenicci", level: 1}],
}))), [
   "Made at Forge from 2 Iron Ingot, Leather Strips, needs Steel Smithing",
   "Sold by Adrianne Avenicci",
   "Improved at Grindstone with Iron Ingot, needs Arcane Blacksmith for an enchanted one, one more condition",
]);
said("conditions joined by or, the same perk of two ranks once, a book to carry, a quest done", texts(KnowYourSkyrim.recipeLines([{bench: "Forge", count: 1,
   takes: [{name: "Gold Ingot", count: 2}], needs: [[perk("Alchemy Mastery"), perk("Alchemy Mastery")], [perk("Herbalist")],
      [{kind: "carried", name: "Cloaks of Skyrim", id: 7, number: 1}], [{kind: "stage", name: "Dragon Rising", id: 8, number: 10}, {kind: "quest", name: "The Golden Claw", id: 9, number: 0}]],
   more: 0}])),
   ["Made at Forge from 2 Gold Ingot, needs Alchemy Mastery, needs Herbalist, carrying Cloaks of Skyrim, after stage 10 of Dragon Rising or after The Golden Claw"]);
said("a smelter melts down a piece not worn", texts(KnowYourSkyrim.recipeLines([{bench: "Smelter", count: 2, takes: [{name: "Steel Armor", count: 1}],
   needs: [[perk("Basic Smithing")], [{kind: "unworn", name: "", number: 0}]], more: 0}])),
   ["Made at Smelter from Steel Armor, needs Basic Smithing, not the one you wear, makes 2"]);
{
   const melt = (name, id) => ({bench: "Smelter", count: 2, takes: [{name, count: 1, id}], needs: [[perk("Basic Smithing")]], more: 0});
   const melted = texts(KnowYourSkyrim.recipeLines([melt("Steel Armor", 1), melt("Steel Boots", 2), melt("Steel Gauntlets", 3), melt("Steel Helmet", 4),
      {bench: "Forge", count: 1, takes: [{name: "Iron Ore", count: 2}], needs: [], more: 0}]));
   said("recipes of one bench from one thing each go on one line", melted,
      ["Made at Smelter from any of Steel Armor, Steel Boots, Steel Gauntlets, Steel Helmet, needs Basic Smithing, makes 2", "Made at Forge from 2 Iron Ore"]);
   said("two such recipes stay apart", texts(KnowYourSkyrim.recipeLines([melt("Steel Armor", 1), melt("Steel Boots", 2)])).length, 2);
}
{
   const page = KnowYourSkyrim.livingSections({carries: [], drops: [], sells: [], stands: [], encounters: 0, encounterLevel: 0,
      relations: [{name: "Ulfberth War-Bear", id: 5, rank: 4, role: "Husband"}, {name: "Proventus Avenicci", id: 6, rank: 3, role: "Father"},
         {name: "Olfina Gray-Mane", rank: 3}, {name: "Nazeem", rank: -1, secret: true}],
      playerRank: 1, aggression: 1, assistance: 2, hostile: false, reaction: "friend"});
   const at = (head) => page.find((section) => section.head == head);
   said("relationships a line for each rank, who they are to the person, a secret one marked", texts(at("Relationships").lines),
      ["Lover: Ulfberth War-Bear (husband)", "Ally: Proventus Avenicci (father), Olfina Gray-Mane", "Rival: Nazeem (secret)"]);
   said("the people of a relationship lead to their pages", plain(at("Relationships").lines[0][1]), {text: "Ulfberth War-Bear", id: 5});
   said("what it is to you, its factions and its temper", texts(at("Toward you").lines),
      ["Your friend", "Its factions take you for a friend", "Attacks its enemies on sight, helps its allies and friends in a fight"]);
   const hostile = KnowYourSkyrim.livingSections({carries: [], drops: [], sells: [], stands: [], encounters: 0, encounterLevel: 0, playerRank: 0,
      aggression: 3, assistance: 0, hostile: true, reaction: "enemy"});
   said("an acquaintance hostile now", texts(hostile.find((section) => section.head == "Toward you").lines),
      ["An acquaintance, no closer bond with you", "Hostile to you now", "Attacks anyone on sight, helps no one in a fight"]);
   said("no relationships, no section", hostile.some((section) => section.head == "Relationships"), false);
}
{
   const hint = (words, room) => {
      const box = clip("hintTest");
      KnowYourSkyrim.writeLine(box, "hint", 2, words, 12, 0);
      KnowYourSkyrim.fitHint(box.hint, room);
      return [box.hint.text, box.hint._width <= room];
   };
   const words = "Search: words, @mod, #keyword, $effect, -leave out";
   said("a hint that fits stays whole", hint(words, 400), [words, true]);
   said("a narrow box loses the last parts of the hint's list", hint(words, 220), ["Search: words, @mod, #keyword", true]);
   said("a box narrower than the first part cuts it with dots", hint(words, 70), ["Search...", true]);
}
{
   // the progression mode tells what the character hasn't met by what it is
   const own = KnowYourSkyrim.plugin;
   const masks = {0x1A2B: {family: "places", chips: ["dungeons"], around: "Whiterun Hold"}, 0x2B: {family: "living", chips: ["people", "merchants"]},
      0x3C: {family: "quests", chips: ["mainQuests"]}, 0x4D: {family: "weapons", chips: []}, 0x6F: {family: "living", chips: ["animals"]}};
   KnowYourSkyrim.plugin = () => ({BrowserMask: (id) => masks[id]});
   said("a place not found says where it lies", KnowYourSkyrim.linkPiece("Bleak Falls Barrow", 0x1A2B), "a place you haven't found (Whiterun Hold)");
   said("someone not met says what they are", KnowYourSkyrim.linkPiece("Belethor", 0x2B), "someone you haven't met (People, Merchants)");
   said("a creature not met", KnowYourSkyrim.linkPiece("Wolf", 0x6F), "a creature you haven't met (Animals)");
   said("a quest not begun says its kind", KnowYourSkyrim.linkPiece("Dragon Rising", 0x3C), "a quest you haven't begun (Main)");
   said("a thing not found says its category", KnowYourSkyrim.linkPiece("Iron Sword", 0x4D), "something you haven't found (Weapons)");
   said("an entry met stays a link", plain(KnowYourSkyrim.linkPiece("Lydia", 0x5E)), {text: "Lydia", id: 0x5E});
   said("a place not found has no map to it", texts([KnowYourSkyrim.spotPieces({name: "Bleak Falls Barrow", id: 0x1A2B, marker: 0x99})]), ["a place you haven't found (Whiterun Hold)"]);
   said("a place met keeps its map", texts([KnowYourSkyrim.spotPieces({name: "Riverwood", id: 0x5E, marker: 0x99})]), ["Riverwood   on the map"]);
   said("things not found go after the names met, counted by their kind",
      texts([KnowYourSkyrim.namesPieces(whole([{name: "Boots", id: 0x4D}, {name: "Iron Dagger", id: 0x5E}, {name: "Gloves", id: 0x4D}, {name: "Wolf", id: 0x6F}], 0))]),
      ["Iron Dagger, 2 things you haven't found (Weapons), a creature you haven't met (Animals)"]);
   said("people not met counted too", texts([KnowYourSkyrim.peoplePieces(whole([{name: "Belethor", id: 0x2B}, {name: "Adrianne", id: 0x2B}], 0))]),
      ["2 people you haven't met (People, Merchants)"]);
   KnowYourSkyrim.plugin = own;
}
{
   const x = (told) => KnowYourSkyrim.padCheatWords(KnowYourSkyrim.cheatDeeds(told)[0]);
   said("X on a quest not begun starts it", x({quest: true, state: 0}), "Start");
   said("X on a quest in progress completes it", x({quest: true, state: 2}), "Complete");
   said("X words of every cheat", ["take", "kill", "raise"].map(KnowYourSkyrim.padCheatWords), ["Take", "Kill", "Resurrect"]);
}
said("one list of encounters",texts(KnowYourSkyrim.livingSections({carries: [], drops: [], sells: [], stands: [], encounters: 1, encounterLevel: 0}).slice(-1)[0].lines), ["Comes in one list of encounters"]);
said("common loot of one kind", texts(KnowYourSkyrim.sourceLines(sources({common: 1}))), ["Common loot of one kind of chest or person"]);
said("an effect under conditions says so", texts(KnowYourSkyrim.browserEffectLines([{name: "Fortify Magicka Rate", words: "Magicka regenerates 100% faster.", known: true, conditional: true}])),
   ["Fortify Magicka Rate: Magicka regenerates 100% faster. (under some conditions)"]);
said("a place of another world names it and has no map link", plain(KnowYourSkyrim.spotPieces({name: "Fort Valus", near: true, world: "Wyrmstooth", marker: 0x1})),
   ["near ", "Fort Valus", " (in Wyrmstooth)"]);
said("a line of places starts with a capital", texts(KnowYourSkyrim.placeLines(undefined, undefined, KnowYourSkyrim.cutList([{name: "Riverwood", near: true}], 0, 4, "places"))), ["Near Riverwood"]);
said("form ids of light plugins in hex", [KnowYourSkyrim.formHex(0xFE001802), KnowYourSkyrim.formHex(0x80000000), KnowYourSkyrim.formHex(0x12E46), KnowYourSkyrim.formHex(0xFFFFFFFF)],
   ["FE001802", "80000000", "00012E46", "FFFFFFFF"]);
said("a chip named as its tab says nothing", KnowYourSkyrim.browserKind({family: "food", chips: ["meal"]}), "Food");
said("arrows made by the dozen", texts(KnowYourSkyrim.sourceLines(sources({made: [{bench: "Forge", count: 24, takes: [{name: "Firewood", count: 1}], needs: [], more: 0}]}))),
   ["Made at Forge from Firewood, makes 24"]);
said("people, creatures, plants and places", texts(KnowYourSkyrim.sourceLines(sources({
   carried: [{name: "Bandit Chief", level: 1}], dropped: [{name: "Skeever", level: 1}], harvested: [{name: "Nirnroot", level: 1}],
   chests: [{name: "Bleak Falls Barrow", thing: "Boss Chest", level: 1, marker: 0x1A2B}], lies: [{name: "Dragonsreach", level: 1}, {name: "Riverwood", near: true, level: 1}], liesMore: 3, common: 4,
}))), [
   "Carried or worn by Bandit Chief",
   "Dropped by Skeever",
   "Grows on Nirnroot",
   "In Boss Chest, Bleak Falls Barrow   on the map",
   "Lies in Dragonsreach",
   "Lies near Riverwood",
   "3 more places",
   "Common loot of 4 kinds of chests and people",
]);
// what mods hand out while the game runs names its framework and its chance, the records give the rest for sure
said("a name SPID gives by chance", KnowYourSkyrim.lineText(KnowYourSkyrim.namesPieces(whole([{name: "Thalmor Soldier", level: 1, by: "SPID", chance: 20}, {name: "Elenwen", level: 1}], 0))),
   "Thalmor Soldier (SPID, 20%), Elenwen");
said("the level of a list and the chance in one bracket", KnowYourSkyrim.lineText(KnowYourSkyrim.namesPieces(whole([{name: "Hagraven", level: 22, by: "SPID", chance: 4}], 0))),
   "Hagraven (from level 22, SPID, 4%)");
said("a chance under conditions",KnowYourSkyrim.lineText(KnowYourSkyrim.namesPieces(whole([{name: "Lucan Valerius", level: 1, by: "CID", chance: 33.3, some: true}], 0))),
   "Lucan Valerius (CID, 33.3%, under some conditions)");
said("hand outs among the ways", texts(KnowYourSkyrim.sourceLines(sources({
   carried: [{name: "Thalmor Soldier", level: 1, by: "SPID", chance: 20}],
   told: [{by: "SPID", filters: "", death: false, chance: 5}, {by: "SPID", filters: "*Necro | 5/20", death: true}],
   chests: [{name: "Riften", thing: "Barrel", level: 1, by: "CID"}],
   swappedFor: [{name: "Goblet", level: 1, by: "BOS", chance: 50}], swappedAt: [{name: "Whiterun", level: 1, by: "BOS"}], swappedBy: [{name: "Silver Goblet", level: 1, by: "BOS"}],
}))), [
   "Carried or worn by Thalmor Soldier (SPID, 20%)",
   "SPID gives it to everyone (5%)",
   "SPID gives it to those that match *Necro | 5/20 when they die",
   "In Barrel, Riften   (CID)",
   "Stands in place of Goblet (BOS, 50%)",
   "Stands in Whiterun   (BOS)",
   "Silver Goblet (BOS) may stand in its place",
]);
said("a line SPID gives to nobody named is the only way", texts(KnowYourSkyrim.sourceLines(sources({told: [{by: "SPID", filters: "MS09NorthwatchFaction", death: false}]}))),
   ["SPID gives it to those that match MS09NorthwatchFaction"]);
said("a place on the map is a link to its marker", plain(KnowYourSkyrim.spotPieces({name: "Bleak Falls Barrow", thing: "Boss Chest", marker: 0x1A2B}).slice(-1)[0]),
   {text: "on the map", marker: 0x1A2B});
said("a place with no marker has no link", KnowYourSkyrim.spotPieces({name: "Katariah"}).length, 1);
said("while the plugin files are read", texts(KnowYourSkyrim.sourceLines(sources({reading: true}))), ["Reading the plugin files for more places..."]);
said("loot lists only when nothing else tells", texts(KnowYourSkyrim.sourceLines(sources({lists: 3}))), ["In 3 loot lists no record hands out, a script may add them to the game's loot"]);
said("loot lists aren't told beside a merchant", texts(KnowYourSkyrim.sourceLines(sources({lists: 3, sold: [{name: "Belethor", level: 1}]}))), ["Sold by Belethor"]);
said("nothing in memory tells", KnowYourSkyrim.sourceLines(sources({})).length, 1);
said("what it goes into", texts(KnowYourSkyrim.browserUseLines(sources({uses: [{name: "Steel Sword (Forge)", level: 1}], usesMore: 12}))), ["Steel Sword (Forge), 12 more"]);
said("a recipe that takes nothing", plain(KnowYourSkyrim.takesPieces([])), ["nothing"]);

// the page waits for the sources a frame
{
   const sections = KnowYourSkyrim.browserSections({effects: [], keywords: ["ArmorHeavy", "VendorItemArmor"]}, undefined);
   said("sections before the sources come", plain(sections.map((one) => one.head)), ["Where to get", "Keywords"]);
   said("the sources are being read", KnowYourSkyrim.lineText(sections[0].lines[0]), "Reading the records...");
   const later = KnowYourSkyrim.browserSections({effects: [{name: "Flames", words: "", known: true}], keywords: []}, sources({uses: [{name: "Iron Dagger", level: 1}]}));
   said("sections with sources and uses", plain(later.map((one) => one.head)), ["Effects", "Where to get", "Used for"]);
}

// an original lists its variants, a variant says what it is a variant of
{
   const original = {effects: [], keywords: [], variants: [{name: "Iron Sword of Embers", id: 0x10A0}, {name: "Iron Sword of Frost", id: 0x10A1}], variantsMore: 3};
   const lines = KnowYourSkyrim.variantLines(original);
   said("variants as one line of links", KnowYourSkyrim.lineText(lines[0]), "Iron Sword of Embers, Iron Sword of Frost, 3 more");
   said("each variant is a link", plain(lines[0][0]), {text: "Iron Sword of Embers", id: 0x10A0});
   said("the variants have a section after the uses", plain(KnowYourSkyrim.browserSections(original, sources({})).map((one) => one.head)), ["Where to get", "Variants"]);
   said("no variants, no section", KnowYourSkyrim.variantLines({variants: []}).length, 0);
   const head = KnowYourSkyrim.browserHeadLines({family: "weapons", chips: ["oneHanded"], id: 0x10A0, mod: "Skyrim.esm", edits: [], original: {name: "Iron Sword", id: 0x12EB7}});
   said("a variant names its original in the head", KnowYourSkyrim.lineText(head[1].pieces), "Variant of Iron Sword");
   said("the original is a link", plain(head[1].pieces[1]), {text: "Iron Sword", id: 0x12EB7});
}

// people and creatures
{
   const bandit = {living: true, family: "living", chips: ["people"], races: ["Nord", "Imperial"], levelLow: 1, levelHigh: 9, records: 40};
   said("a living thing's kind and races", KnowYourSkyrim.livingKind(bandit), "People, Nord, Imperial");
   said("levels set in the records", KnowYourSkyrim.livingNumbers(bandit), "Levels 1 to 9   40 records");
   said("a level that follows the player", KnowYourSkyrim.livingNumbers({shareLow: 1, shareHigh: 1, unique: true, merchant: true, records: 1}),
      "Level follows yours x1.00   Unique   Trades");
   const head = (told) => KnowYourSkyrim.browserHeadLines(Object.assign({living: true, family: "living", chips: ["people"], races: [], records: 1, mod: "Skyrim", edits: [], id: 0x13BBD}, told));
   const guarded = head({guard: 2});
   said("one the game keeps alive has a line of its own with a shield", [KnowYourSkyrim.lineText(guarded[1].pieces), guarded[1].sign], ["Essential: the game keeps it alive", "ward"]);
   said("one only the player can kill", KnowYourSkyrim.lineText(head({guard: 1})[1].pieces), "Protected: only you can kill it");
   said("a name kept alive in some records", KnowYourSkyrim.lineText(head({guard: 0, guardedRecords: 2, records: 5}).find((line) => line.sign == "ward").pieces), "Kept alive in 2 of 5 records");
   said("one who can die has no such line", head({guard: 0}).length, 2);
   said("a follower's levels have a floor and a top", KnowYourSkyrim.livingNumbers({shareLow: 1, shareHigh: 1, followFrom: 6, followTo: 40, records: 1}), "Level follows yours x1.00 (6 to 40)");
   said("no top", KnowYourSkyrim.livingNumbers({shareLow: 1.5, shareHigh: 1.5, followFrom: 10, followTo: 0, records: 1}), "Level follows yours x1.5 (from 10)");
   const things = {carries: [{name: "Iron Sword", level: 1, id: 0x12EB7}], carriesMore: 0, drops: [], dropsMore: 0, sells: [], sellsMore: 0,
      stands: [{name: "Riverwood", level: 1, id: 0}], standsMore: 0, encounters: 12, encounterLevel: 6};
   const sections = KnowYourSkyrim.livingSections(things);
   said("a living thing's sections", plain(sections.map((one) => one.head)), ["Carries", "Where to meet"]);
   said("where to meet it", sections[1].lines.map((line) => KnowYourSkyrim.lineText(line)), ["Stands in Riverwood", "Comes in 12 lists of encounters, from level 6"]);
   said("what it carries links the things", plain(sections[0].lines[0][0]), {text: "Iron Sword", id: 0x12EB7});
   said("before the records come", KnowYourSkyrim.livingSections(undefined).length, 1);
   said("nothing known of where", KnowYourSkyrim.livingSections({carries: [], drops: [], sells: [], stands: [], encounters: 0}).length, 1);
}

// the quests things and people take part in, and the page of a quest
{
   const withQuests = KnowYourSkyrim.livingSections({carries: [], drops: [], sells: [], stands: [{name: "Katariah", level: 1, marker: 0x2C3}], encounters: 0,
      quests: [{name: "Hail Sithis!", id: 0x3C57A, state: 2}], questsMore: 0});
   said("a living thing's quests", plain(withQuests.map((one) => one.head)), ["Where to meet", "Quests"]);
   said("where to meet with the map", KnowYourSkyrim.lineText(withQuests[0].lines[0]), "Stands in Katariah   on the map");
   said("a quest line links the quest and says how far", plain(withQuests[1].lines[0]), [{text: "Hail Sithis!", id: 0x3C57A}, "  In progress"]);
   said("no places while the files are read", texts(KnowYourSkyrim.livingSections({carries: [], drops: [], sells: [], stands: [], encounters: 0, reading: true})[0].lines),
      ["Reading the plugin files for more places..."]);
   const quest = {quest: true, family: "quests", chips: ["sideQuests"], state: 3, stage: 200,
      objectives: [{text: "Find the Sapphire Dragon Claw", state: 1}, {text: "Return to Ysolda", state: 0}]};
   said("the kind of a quest", KnowYourSkyrim.browserKind(quest), "Quests, Side");
   said("how far a quest is", KnowYourSkyrim.questNumbers(quest), "Done   Stage 200");
   said("a quest not begun has no stage", KnowYourSkyrim.questNumbers({state: 0, stage: 0}), "Not begun");
   const before = KnowYourSkyrim.questSections(quest, undefined);
   said("the objectives come before the records", plain(before.map((one) => one.head)), ["Objectives", "Rewards"]);
   said("objectives done and open", texts(before[0].lines), ["Find the Sapphire Dragon Claw (done)", "Return to Ysolda"]);
   const after = KnowYourSkyrim.questSections({objectives: []}, {people: [{name: "Ysolda", id: 0x13BAB, level: 1}], peopleMore: 0, things: [], thingsMore: 0,
      places: [{name: "Shroud Hearth Barrow", level: 1, marker: 0x3F1}], placesMore: 0});
   said("a quest's sections", plain(after.map((one) => one.head)), ["Objectives", "People", "Places"]);
   said("a quest of no objectives says so", KnowYourSkyrim.lineText(after[0].lines[0]), "It has no objectives");
   said("a quest's places with the map", KnowYourSkyrim.lineText(after[2].lines[0]), "Shroud Hearth Barrow   on the map");
   said("a quest row says how far it is", KnowYourSkyrim.rowMore({variants: 0, state: 3}), "  Done");
   said("a quest not begun says nothing in its row", KnowYourSkyrim.rowMore({variants: 0, state: 0}), "");
   said("a quest going is gold", KnowYourSkyrim.questActive({state: 2}), true);
   said("pages of the three kinds", [KnowYourSkyrim.pageSections({living: true}, undefined).length, KnowYourSkyrim.pageSections({quest: true}, undefined)[0].head], [1, "Objectives"]);
   said("an objective not given yet says so", texts(KnowYourSkyrim.objectiveLines([{text: "Kill Grelod", state: 3}, {text: "Escape", state: 2}])), ["Kill Grelod (not yet)", "Escape (failed)"]);
   said("a done quest's objectives never given say nothing of it", texts(KnowYourSkyrim.objectiveLines([{text: "Kill Grelod", state: 3}], true)), ["Kill Grelod"]);

   // what the quest's scripts give, its chains and stages, once the plugin read the quests' files
   const read = {people: [], peopleMore: 0, things: [], thingsMore: 0, places: [], placesMore: 0, questsRead: true,
      rewards: [{kind: "item", name: "Gold", id: 0xF, count: 500, stage: 200, dialogue: false},
         {kind: "item", options: [{name: "Steel Sword", id: 0x13989}, {name: "Steel War Axe", id: 0x13990}], optionsMore: 3, optionsShown: 6, count: 1, stage: -1, dialogue: true},
         {kind: "word", name: "Fus", id: 0x13E22, means: "Force", count: 0, stage: 90, dialogue: false},
         {kind: "skill", name: "One-handed", count: 2, stage: -1, dialogue: false}],
      before: [[{name: "Unbound", id: 0x3372B, state: 3}, {name: "Before the Storm", id: 0x3372C, state: 3}]], beforeMore: 0, beforeShown: 4,
      after: [[{name: "The Way of the Voice", id: 0x242BA, state: 0}]], afterMore: 2, afterShown: 4,
      steps: [{stage: 10, text: "Talk to the Jarl", done: true, current: false, finishes: false, fails: false},
         {stage: 160, text: "The Jarl rewarded me", done: false, current: true, finishes: true, fails: false}],
      locations: [{name: "Dragonsreach", id: 0x16EF2, count: 1}], locationsMore: 0, locationsShown: 8};
   const full = KnowYourSkyrim.questSections({objectives: [{text: "Talk to the Jarl", state: 1}]}, read);
   said("a read quest's sections", plain(full.map((one) => one.head)), ["Objectives", "Rewards", "Quest chain", "Stages", "Places"]);
   said("rewards with their stage or dialogue", texts(full[1].lines), ["Gold (500)  (stage 200)", "One of Steel Sword, Steel War Axe, 3 more  (in dialogue)", "Word of power: Fus (Force)  (stage 90)", "One-handed +2"]);
   said("a reward is a link", plain(full[1].lines[0][0]), {text: "Gold", id: 0xF});
   said("chains before and after", texts(full[2].lines), ["Before: Unbound > Before the Storm", "After: The Way of the Voice", "2 more chains"]);
   said("a quest of a chain is a link", plain(full[2].lines[0][1]), {text: "Unbound", id: 0x3372B});
   said("stages with how far they are", texts(full[3].lines), ["10: Talk to the Jarl (done)", "160: The Jarl rewarded me (now, finishes the quest)"]);
   said("the places of a quest link to them", plain(full[4].lines[0][0]), {text: "Dragonsreach", id: 0x16EF2});
   said("no reward of its own says so", texts(KnowYourSkyrim.questSections({objectives: []}, Object.assign({}, read, {rewards: []}))[1].lines), ["Its scripts give you nothing of their own"]);
   said("while the quests' files are read", texts(KnowYourSkyrim.questSections({objectives: []}, Object.assign({}, read, {questsRead: false}))[1].lines), ["Reading the quests' scripts..."]);
   said("a stage carries its mark and bare words for the looks", [full[3].lines[0].mark, full[3].lines[1].mark, full[3].lines[1].stage, texts([full[3].lines[1].bare])[0]],
      ["done", "now", 160, "The Jarl rewarded me  (finishes the quest)"]);
   const way = {name: "The Way of the Voice", id: 1}, blade = {name: "A Blade in the Dark", id: 2}, fallen = {name: "The Fallen", id: 3}, horn = {name: "The Horn", id: 4}, eyrie = {name: "Eyrie", id: 5}, paarthurnax = {name: "Paarthurnax", id: 6};
   const forks = KnowYourSkyrim.chainForks([[way, blade, fallen, eyrie], [way, blade, fallen, paarthurnax], [way, horn]]);
   said("chains after a quest fork where they part", [forks.kids.length, forks.kids[0].kids.length, forks.kids[0].kids[0].kids[0].kids.length], [1, 2, 2]);
}

// the page of a place: who is there now by the places inside it, who lives there, what lies inside
{
   const place = {place: true, family: "places", chips: ["towns"], around: {name: "Whiterun Hold", id: 0x16772}, marker: 0x2C3, here: true, keywords: ["LocTypeCity"]};
   const head = KnowYourSkyrim.placeHeadLines(place);
   said("the place around and the map", KnowYourSkyrim.lineText(head[0].pieces), "In Whiterun Hold   on the map");
   said("the place around is a link", plain(head[0].pieces[1]), {text: "Whiterun Hold", id: 0x16772});
   said("the player is here", [KnowYourSkyrim.lineText(head[1].pieces), head[1].sign], ["You are here", "pin"]);
   said("the kind of a place", KnowYourSkyrim.browserKind(place), "Places, Towns");
   const sources = {
      here: [{people: [{name: "Whiterun Guard", id: 0x2BF9F, count: 4}, {name: "Heimskr", id: 0x13BA9, count: 1}], peopleMore: 0, peopleShown: 12},
         {place: "Dragonsreach", id: 0x16EF2, people: [{name: "Balgruuf the Greater", id: 0x13BBD, count: 1}], peopleMore: 0, peopleShown: 12}],
      hereMore: 0, hereShown: 6, deadHere: [], deadHereMore: 0, deadHereShown: 6,
      lives: [{name: "Heimskr", id: 0x13BA9, count: 1}], livesMore: 0, livesShown: 12,
      inside: [{name: "Breezehome", id: 0x16774, count: 1}, {name: "Dragonsreach", id: 0x16EF2, count: 1}], insideMore: 0, insideShown: 12};
   const sections = KnowYourSkyrim.placeSections(place, sources);
   said("a place's sections", plain(sections.map((one) => one.head)), ["Here now", "Live here", "Places inside", "Keywords"]);
   const questsThere = KnowYourSkyrim.placeSections(place, Object.assign({}, sources, {questsRead: true,
      questGroups: [{quests: [{name: "Dragon Rising", id: 0x2610C, state: 3}], questsMore: 0, questsShown: 8},
         {place: "Dragonsreach", id: 0x16EF2, quests: [{name: "The Golden Claw", id: 0x2B1DA, state: 0}], questsMore: 0, questsShown: 8}],
      questGroupsMore: 0, questGroupsShown: 6}));
   said("a place's quests before its keywords", plain(questsThere.map((one) => one.head)), ["Here now", "Live here", "Places inside", "Quests", "Keywords"]);
   said("the quests of the place, then by the places inside", texts(questsThere[3].lines), ["Dragon Rising (Done)", "In Dragonsreach: The Golden Claw (Not begun)"]);
   said("who is right there, then each place inside", texts(sections[0].lines), ["Whiterun Guard (4), Heimskr", "In Dragonsreach: Balgruuf the Greater"]);
   said("the place inside is a link", plain(sections[0].lines[1][1]), {text: "Dragonsreach", id: 0x16EF2});
   said("no one there says so", texts(KnowYourSkyrim.placeSections(place, Object.assign({}, sources, {here: []}))[0].lines), ["No one is here now"]);
   said("before the records come", KnowYourSkyrim.placeSections(place, undefined).length, 1);
   said("a place of a person's page leads to its page", plain(KnowYourSkyrim.spotPieces({name: "Breezehome, Whiterun", id: 0x16774})), [{text: "Breezehome, Whiterun", id: 0x16774}]);
   said("a place has nothing to inspect or cheat", plain(KnowYourSkyrim.cheatDeeds(place)), []);
   said("a place row says what it lies in", KnowYourSkyrim.rowMore({variants: 0, around: "Whiterun Hold"}), "  in Whiterun Hold");
}

// the trees of Perks: a line for each school and one for the other trees, one tree at a time
{
   const keys = KnowYourSkyrim.BROWSER_FAMILIES.map((family) => family.key);
   said("the tab of Perks comes after Places", keys.indexOf("perks"), keys.indexOf("places") + 1);
   const rows = KnowYourSkyrim.CHIP_ROWS.perks;
   said("the warrior's, the thief's and the mage's skills, then the vampire's, the werewolf's and any other tree", plain(rows.map((group) => group.length)), [6, 6, 6, 3]);
   said("the skills in the game's order", [rows[0][0].key, rows[1][0].key, rows[2][5].key, rows[3][2].key], ["oneHandedPerks", "lightArmorPerks", "enchantingPerks", "otherPerks"]);
   said("every tree is a chip alone", rows.every((group) => group.every((chip) => chip.alone == true)), true);
   const picked = [];
   KnowYourSkyrim.pickChip(picked, rows, 0, "oneHandedPerks");
   KnowYourSkyrim.pickChip(picked, rows, 2, "illusionPerks");
   said("one tree at a time, Illusion lets One-handed go", plain(picked), [null, null, "illusionPerks", null]);
}

// the page of a perk: one of a single rank tells its description and what it asks, one of several a line for each rank
// with both, then its neighbours in the tree, what it unlocks and who has it
{
   const one = {perk: true, family: "perks", chips: ["oneHandedPerks"], ranks: 1, rank: 0, id: 0x3FFFA, mod: "Skyrim", edits: []};
   said("the kind of a perk is its tree", KnowYourSkyrim.browserKind(one), "Perks, One-handed");
   said("a perk the player lacks says so beside the tile", texts(KnowYourSkyrim.browserHeadLines(one).map((line) => line.pieces)), ["Perks, One-handed", "You don't have it", "Skyrim   0003FFFA"]);
   const many = KnowYourSkyrim.browserHeadLines(Object.assign({}, one, {ranks: 5, rank: 2}));
   said("a perk of ranks tells how many and the one the player has, with a tick", [texts(many.map((line) => line.pieces)), many[2].sign],
      [["Perks, One-handed", "5 ranks", "You have rank 2 of 5", "Skyrim   0003FFFA"], "check"]);
   said("a perk of one rank the player has", KnowYourSkyrim.lineText(KnowYourSkyrim.perkHeadLine({ranks: 1, rank: 1}).pieces), "You have it");

   const sources = {description: "Power attacks with one-handed weapons cost 25% less stamina.", reading: false,
      ranks: [{rank: 1, has: false, description: "Power attacks with one-handed weapons cost 25% less stamina.",
         needs: [[{kind: "perk", name: "Armsman", id: 0xBABE4, number: 0}], [{kind: "skill", name: "One-handed", number: 20}]], more: 1}],
      parents: [{name: "Armsman", id: 0xBABE4}], parentsMore: 0, parentsShown: 8,
      children: [{name: "Critical Charge", id: 0x3AF81}, {name: "Savage Strike", id: 0x52D50}], childrenMore: 0, childrenShown: 8,
      recipes: [{name: "Steel Sword (Forge)", id: 0x13989}], recipesMore: 0, recipesShown: 12, improving: [], improvingMore: 0, improvingShown: 12,
      neededBy: [{name: "Critical Charge", id: 0x3AF81}], neededByMore: 0, neededByShown: 12,
      people: [{name: "Bandit Chief", id: 0x1E7D6}, {name: "Ulfric Stormcloak", id: 0x1414D}], peopleMore: 3, peopleShown: 12};
   const sections = KnowYourSkyrim.perkSections(sources);
   const at = (head) => sections.find((section) => section.head == head);
   said("a perk of one rank's sections", plain(sections.map((section) => section.head)), ["Description", "Requirements", "In the tree", "Unlocks", "Who has it"]);
   said("its description in the game's words", texts(at("Description").lines), ["Power attacks with one-handed weapons cost 25% less stamina."]);
   said("what it asks a group to a line, the rest counted", texts(at("Requirements").lines), ["Needs Armsman", "One-handed 20", "One more condition"]);
   said("a perk it asks for leads to its page", plain(at("Requirements").lines[0][1]), {text: "Armsman", id: 0xBABE4});
   said("its neighbours in the tree", texts(at("In the tree").lines), ["Comes after Armsman", "Leads to Critical Charge, Savage Strike"]);
   said("a neighbour leads to its page", plain(at("In the tree").lines[1][1]), {text: "Critical Charge", id: 0x3AF81});
   said("the recipes and the perks that ask for it", texts(at("Unlocks").lines), ["Recipes: Steel Sword (Forge)", "Perks that need it: Critical Charge"]);
   said("who has it, the rest a press away", texts(at("Who has it").lines), ["Bandit Chief, Ulfric Stormcloak, 3 more"]);
   said("the count of the rest opens the list", plain(at("Who has it").lines[0].slice(-1)[0]), {text: "3 more", open: "people"});

   const ranks = {reading: false, ranks: [
      {rank: 1, has: true, description: "One-handed weapons do 20% more damage.", needs: [], more: 0},
      {rank: 2, has: false, description: "One-handed weapons do 40% more damage.", needs: [[{kind: "skill", name: "One-handed", number: 20}]], more: 0},
      {rank: 3, has: false, description: "", needs: [[{kind: "skill", name: "One-handed", number: 40}]], more: 0}],
      parents: [], children: [{name: "Fighting Stance", id: 0x52D52}], childrenMore: 0, childrenShown: 8, recipes: [], improving: [], neededBy: [], people: []};
   const ranked = KnowYourSkyrim.perkSections(ranks);
   said("a perk of ranks' sections", plain(ranked.map((section) => section.head)), ["Ranks", "In the tree"]);
   said("each rank with its description and what it asks, the player's said", texts(ranked[0].lines),
      ["Rank 1 of 3: One-handed weapons do 20% more damage. (yours)", "Rank 2 of 3: One-handed weapons do 40% more damage. (One-handed 20)", "Rank 3 of 3 (One-handed 40)"]);
   said("a rank had is ticked, the next one to take open, the later ones faint", plain(ranked[0].lines.map((line) => line.mark)), ["done", "open", "later"]);
   said("the box of a rank stands for the word of it", texts([ranked[0].lines[0].bare]), ["Rank 1 of 3: One-handed weapons do 20% more damage."]);

   said("before the records come", plain(KnowYourSkyrim.pageSections({perk: true}, undefined).map((section) => section.head)), ["Description"]);
   said("while the descriptions are read", texts(KnowYourSkyrim.perkSections({reading: true, ranks: [{rank: 1, has: false, needs: [], more: 0}]})[0].lines), ["Reading the descriptions..."]);
   said("nothing to inspect on a perk", KnowYourSkyrim.signOnly({perk: true}), true);

   said("a perk the player lacks can be added", plain(KnowYourSkyrim.cheatDeeds({perk: true, rank: 0, ranks: 1})), ["add"]);
   said("one the player has whole can be removed", plain(KnowYourSkyrim.cheatDeeds({perk: true, rank: 1, ranks: 1})), ["remove"]);
   said("a rank of several both", plain(KnowYourSkyrim.cheatDeeds({perk: true, rank: 2, ranks: 5})), ["add", "remove"]);
   said("X words of a perk's cheats", ["add", "remove"].map(KnowYourSkyrim.padCheatWords), ["Add", "Remove"]);
   said("the words told of a rank added or taken", [KnowYourSkyrim.perkDeedText("add", {name: "Armsman"}, {rank: 2, ranks: 5}), KnowYourSkyrim.perkDeedText("remove", {name: "Fighting Stance"}, {rank: 1, ranks: 1})],
      ["Added: Armsman, rank 2 of 5", "Removed: Fighting Stance"]);
   said("a perk row says the rank the player has", [KnowYourSkyrim.rowMore({variants: 0, rank: 2, ranks: 5}), KnowYourSkyrim.rowMore({variants: 0, rank: 1, ranks: 1}), KnowYourSkyrim.rowMore({variants: 0, rank: 0, ranks: 3})],
      ["  2/5", "  yours", ""]);
   said("the sign of a perk the player has is gold", [KnowYourSkyrim.rowLit({rank: 1}), KnowYourSkyrim.rowLit({rank: 0})], [true, false]);
}

// mods that only change textures: named apart in the list of mods, a line of the page that leads to what each changes
said("a mod of textures in the list of mods", KnowYourSkyrim.modLabel("textures:Noble Skyrim"), "Noble Skyrim (textures)");
{
   const head = KnowYourSkyrim.browserHeadLines({family: "armor", chips: ["heavy"], mod: "Skyrim", edits: [], id: 0x12E46, retextured: ["Noble Skyrim", "Iron Armor HD"]});
   const line = head.find((one) => KnowYourSkyrim.lineText(one.pieces).indexOf("Textures") == 0);
   said("the mods that change its textures", KnowYourSkyrim.lineText(line.pieces), "Textures changed by Noble Skyrim, Iron Armor HD");
   said("each picks its mod", plain(line.pieces[1]), {text: "Noble Skyrim", pick: "textures:Noble Skyrim"});
}

// mods read without their file endings
said("a plugin without .esp", KnowYourSkyrim.modStem("Inventory Reforged.esp"), "Inventory Reforged");
said("an esm and an esl", [KnowYourSkyrim.modStem("Skyrim.esm"), KnowYourSkyrim.modStem("ccBGSSSE001-Fish.ESL")], ["Skyrim", "ccBGSSSE001-Fish"]);
said("words with no ending stay", KnowYourSkyrim.modStem("Dawnguard"), "Dawnguard");

// A name defined in two parts loses the earlier one without a word. The helpers taken from the panel count as a part of
// their own, the browser never sets one of them
{
   const defined = {};
   for (const part of parts) {
      for (const match of part.script.matchAll(/^KnowYourSkyrim\.(\w+) = /gm)) {
         (defined[match[1]] = defined[match[1]] || []).push(part.name);
      }
   }
   said("no name is defined by two parts", Object.keys(defined).filter((name) => defined[name].length > 1), []);
   said("every part of the source is in the movie", fs.readdirSync(source).filter((name) => name.endsWith(".as") && !parts.some((part) => part.name == name)), []);
}

// rows of one name and one mod say what tells them apart
{
   const rows = [{id: 0xF6E1A, name: "Gauldur Blackbow", mod: "Skyrim", number: 13, numberKind: "damage"}, {id: 0xF6E1B, name: "Gauldur Blackbow", mod: "Skyrim", number: 16, numberKind: "damage"},
      {id: 0x401CD87, name: "Dunmer Outfit", mod: "Dragonborn", number: 25, numberKind: "value"}, {id: 0x401CD88, name: "Dunmer Outfit", mod: "Dragonborn", number: 25, numberKind: "value"},
      {id: 0x13950, name: "Steel Armor", mod: "Skyrim", number: 31, numberKind: "armor"}, {id: 0x9013950, name: "Steel Armor", mod: "AVExpansion", number: 31, numberKind: "armor"}];
   const tags = KnowYourSkyrim.twinTags(rows);
   said("twins by their numbers, by their ids", [tags[0], tags[1], tags[2], tags[3]], ["Damage 13", "Damage 16", "0401CD87", "0401CD88"]);
   said("one name in two mods needs no tag, the mod stands in the row", [tags[4], tags[5]], [undefined, undefined]);
   const places = KnowYourSkyrim.twinTags([{id: 0x16772, name: "Whiterun", mod: "Skyrim", placeKind: "holds"}, {id: 0x18A56, name: "Whiterun", mod: "Skyrim", placeKind: "towns"}]);
   said("a hold and its city by their kinds", [places[0], places[1]], ["hold", "town"]);
}

// the name of a row keeps its room, the words after it give way first
{
   const field = {_width: 340, text: ""};
   Object.defineProperty(field, "textWidth", {get: () => String(field.text).length * 7});
   const tail = KnowYourSkyrim.fitRow(field, "Gather Some Uncommon Ingredients", "  2E0133BE  Waiting for you", 10);
   said("the tail is cut, the name stays whole", [field.text.indexOf("Gather Some Uncommon Ingredients") == 0, tail.indexOf("  2E0133BE") == 0, field.textWidth <= 336], [true, true, true]);
   const short = KnowYourSkyrim.fitRow(field, "Iron Sword", "  +12", 0);
   said("a row that fits stays as it is", [field.text, short], ["Iron Sword  +12", "  +12"]);
}

said("rows that fit a room", KnowYourSkyrim.browserRowsFit(100), 4);
said("at least one row", KnowYourSkyrim.browserRowsFit(10), 1);

// the words of the browser are its own, the helpers of the panel speak them too
{
   KnowYourSkyrim.said = {};
   KnowYourSkyrim.fileWords = {"$KYSK_All items": "Усі речі", "$KYSK_Heavy": "Важкі"};
   said("words from the browser's own file", KnowYourSkyrim.say("All items"), "Усі речі");
   said("a chip of the panel falls back on the browser's words", KnowYourSkyrim.translated({words: "$Heavy", fallback: "Heavy"}), "Важкі");
   const translate = movie.skyui.util.Translator.translate;
   movie.skyui.util.Translator.translate = (key) => key == "$Heavy" ? "Heavy" : key;
   KnowYourSkyrim.said = {};
   said("the browser's own translation wins over another mod's English key", KnowYourSkyrim.translated({words: "$Heavy", fallback: "Heavy"}), "Важкі");
   movie.skyui.util.Translator.translate = translate;
   said("what a word brings in isn't looked at again", KnowYourSkyrim.say("#1 (#2)", "Note #2", 3), "Note #2 (3)");
   said("the numbers may swap", KnowYourSkyrim.say("#2 and #1", "a", "b"), "b and a");
   said("words with no value keep the sign", KnowYourSkyrim.say("# more"), "# more");
   said("a word the file lacks stays English", KnowYourSkyrim.say("# found", 3), "3 found");
   KnowYourSkyrim.said = {};
   KnowYourSkyrim.fileWords = undefined;
}

// what stays shut around the player and what opens it
{
   const gate = {thing: "", far: 36, side: 1, up: 1.2, count: 1, openers: [{name: "Chain", far: 5.8, side: 6, up: 9.7, off: false, away: false}]};
   const texts = (lines) => plain(lines).map((line) => line.map((piece) => typeof piece == "string" ? piece : piece.text).join(""));
   const section = (sources) => {
      const sections = [];
      KnowYourSkyrim.pushBlockers(sections, sources);
      return sections.length > 0 ? texts(sections[0].lines) : [];
   };
   KnowYourSkyrim.browserOpen = {};
   said("the gate and the chain from where the player looks", section({blockers: [gate], blockersMore: 0, blockersShown: 3, blockersNoParent: 0}),
      ["A shut gate, 36 m ahead on the right, at your level", "   Opened by Chain, 6 m on the left, 10 m above you"]);
   said("what a quest brings has no spot", section({blockers: [Object.assign({}, gate, {openers: [{name: "Chain", far: 5, side: 0, up: 0, off: true}]})], blockersMore: 0, blockersShown: 3, blockersNoParent: 0})[1],
      "   What opens it isn't here yet, it depends on a quest");
   said("a bridge is told as one", section({blockers: [Object.assign({}, gate, {raised: true})], blockersMore: 0, blockersShown: 3, blockersNoParent: 0})[0], "A raised bridge, 36 m ahead on the right, at your level");
   said("a lock tells its key",section({blockers: [{thing: "Door", far: 12, side: 6, up: 0, key: "Rusty Key", carried: false, openers: []}], blockersMore: 0, blockersShown: 3, blockersNoParent: 0}),
      ["Door, 12 m on the left, at your level: needs Rusty Key"]);
   said("the progression mode holds it behind a press", section({blockers: [gate], blockersMore: 0, blockersShown: 3, blockersNoParent: 1, blockersHidden: true}),
      ["2 ways stay shut here   show what opens them"]);
   KnowYourSkyrim.browserOpen = {blockers: true};
   said("asked, it tells all of it", section({blockers: [gate], blockersMore: 0, blockersShown: 3, blockersNoParent: 0, blockersHidden: true}).length, 2);
   KnowYourSkyrim.browserOpen = {};
   said("nothing shut, no section", section({blockers: [], blockersMore: 0, blockersShown: 3, blockersNoParent: 0}), []);
   said("a page of another place has none", section({}), []);
}

// the plugin is told where the browser lies, the whole screen under the view of an item, nothing while it's shut
{
   const plugin = KnowYourSkyrim.plugin;
   const browsing = KnowYourSkyrim.browsing;
   const browser = KnowYourSkyrim.browser;
   const box = KnowYourSkyrim.shareBox;
   const told = [];
   KnowYourSkyrim.plugin = () => ({BrowserArea: (...area) => told.push(area)});
   KnowYourSkyrim.shareBox = () => ({from: {x: 0.5, y: 0.1}, to: {x: 0.9, y: 0.8}});
   KnowYourSkyrim.browser = {};
   KnowYourSkyrim.browsing = true;
   KnowYourSkyrim.tellPointerArea();
   KnowYourSkyrim.inspecting = true;
   KnowYourSkyrim.tellPointerArea();
   KnowYourSkyrim.inspecting = false;
   KnowYourSkyrim.browsing = false;
   KnowYourSkyrim.tellPointerArea();
   said("where the browser lies", plain(told), [[0.5, 0.1, 0.9, 0.8], [0, 0, 1, 1], [0, 0, 0, 0]]);
   KnowYourSkyrim.plugin = plugin;
   KnowYourSkyrim.browsing = browsing;
   KnowYourSkyrim.browser = browser;
   KnowYourSkyrim.shareBox = box;
}

// the chip of the place the player stands in opens its page and picks nothing
{
   said("the gamepad walks past it", plain(KnowYourSkyrim.CHIP_ROWS.places[0].filter((chip) => chip.page == true).map((chip) => chip.key)), ["hereNow"]);
   const plugin = KnowYourSkyrim.plugin;
   const go = KnowYourSkyrim.goBrowserItem;
   const tell = KnowYourSkyrim.tellBrowser;
   const seen = [];
   KnowYourSkyrim.goBrowserItem = (id) => seen.push(id);
   KnowYourSkyrim.tellBrowser = (words) => seen.push(words);
   KnowYourSkyrim.plugin = () => ({BrowserHere: () => 77});
   KnowYourSkyrim.pressBrowserChip("hereNow", 0);
   KnowYourSkyrim.plugin = () => ({BrowserHere: () => 0});
   KnowYourSkyrim.pressBrowserChip("herenow", 0);
   said("the page, or a word when it has none", plain(seen), [77, "This place has no page"]);
   KnowYourSkyrim.plugin = plugin;
   KnowYourSkyrim.goBrowserItem = go;
   KnowYourSkyrim.tellBrowser = tell;
}

// the helpers taken from the panel are there under the browser's name, and the browser's own chips added to them
said("the panel's signs", typeof KnowYourSkyrim.drawIcon == "function" && KnowYourSkyrim.ICONS.lens != undefined, true);
said("the panel's chips with the browser's", [KnowYourSkyrim.CHIP_ROWS.armor[1][0].key, KnowYourSkyrim.CHIP_ROWS.living[0][0].key, KnowYourSkyrim.CHIP_ROWS.quests[0][0].key], ["head", "people", "mainQuests"]);
said("the panel's spot after the hints", typeof KnowYourSkyrim.afterHints, "function");
said("the helpers taken don't touch the panel's global", /\bEquipment\b/.test(parts.find((part) => part.name == "borrowed").script), false);

done();
