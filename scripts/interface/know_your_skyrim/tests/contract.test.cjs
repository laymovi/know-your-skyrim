// Runs Know your Skyrim and the panel of Inventory Reforged in one menu outside the game, the way the two movies share
// _global there: the handshake whichever starts first, the browser in the panel's room and alone, the item card
// getting its place back, the panel's own look button
//   node scripts/interface/know_your_skyrim/tests/contract.test.cjs

const {browserParts, panelPart, clip, inventory, world, run, browserPlugin, said, done} = require("./movie.cjs");

const browserMovie = browserParts();
const panelMovie = ["10-core.as", "26-draw.as", "38-look.as", "39-browser-room.as"].map(panelPart);

// the panel's own plugin, with what it was asked
function panelPlugin() {
   const calls = [];
   const keep = (name, value) => (...args) => {
      calls.push(name + (args.length ? " " + JSON.stringify(args) : ""));
      return value;
   };
   return {calls, Words: keep("Words", {}), Settings: keep("Settings", {}), HidePlayer: keep("HidePlayer"), ShowSlots: keep("ShowSlots", []), ShowPreview: keep("ShowPreview")};
}

// A menu with SkyUI's inventory, the browser's plugin and, when asked, the panel's plugin and its movie on its way
function menu(kept, withPanel) {
   const {root, menu} = inventory();
   const plugins = {KnowYourSkyrim: browserPlugin(kept)};
   if (withPanel) {
      plugins.EquipmentPanel = panelPlugin();
      root.createEmptyMovieClip("equipmentPanelScript", 7);
   }
   return {root, menu, plugins, made: world(root, plugins)};
}

// The panel joins as its parts would: laid out, the room for the browser first, the look button after
function startPanel(setup) {
   const Equipment = run(setup.made.context(), panelMovie)._global.Equipment;
   Equipment.ICONS = {};
   Equipment.menu = setup.menu;
   Equipment.frame = {left: 600, top: 40, right: 1240, bottom: 600};
   Equipment.LABEL_HEIGHT = 18;
   Equipment.settings = {heroStats: true};
   Equipment.panel = clip("equipmentPanel");
   Equipment.hero = clip("hero");
   Equipment.shown = [];
   for (const name of ["closePopup", "closeContext", "closePieces", "showModels", "refreshHero", "listsAnswer", "showWindow"]) {
      Equipment[name] = () => {
         Equipment.shown.push(name);
         return true;
      };
   }
   // the panel's own item window keeps SkyUI's card and shows no preview of the game's under the browser, the card
   // isn't in the character's room
   Equipment.leavesCard = () => false;
   Equipment.cardInRoom = () => false;
   Equipment.previewShown = () => false;
   // the panel draws in two layers of the menu, the browser's room lies in the lower one
   Equipment.squash = 1;
   Equipment.under = setup.menu.createEmptyMovieClip("equipmentUnder", setup.menu.getNextHighestDepth());
   Equipment.over = setup.menu.createEmptyMovieClip("equipmentOver", setup.menu.getNextHighestDepth());
   Equipment.buildBrowserRoom();
   Equipment.buildLook();
   return Equipment;
}

function startBrowser(setup) {
   return run(setup.made.context(), browserMovie)._global.KnowYourSkyrim;
}

function frame(clipWithFrame) {
   if (clipWithFrame.onEnterFrame) {
      clipWithFrame.onEnterFrame();
   }
}

// the panel starts first, the browser meets it as it starts
{
   const setup = menu({}, true);
   const Equipment = startPanel(setup);
   said("the panel waits for no browser left shut", Equipment.browsing, false);
   frame(Equipment.browserRoom);
   said("the room is ready on the next frame", Equipment.browserRoomReady, true);
   const KnowYourSkyrim = startBrowser(setup);
   said("the browser meets the panel laid out before it", KnowYourSkyrim.panel === Equipment, true);
   frame(KnowYourSkyrim.browseButton);
   const button = KnowYourSkyrim.browseButton;
   said("the eye after SkyUI's hints", [button._x, button._y, button.wide], [20 + 160 + 24, Math.round((26 - 22) / 2), 32]);
   said("the look button right after the eye", Equipment.lookButton._x, button._x + button.wide + Equipment.LOOK_BAR_GAP);

   KnowYourSkyrim.toggleBrowser();
   said("open, the panel makes way", [KnowYourSkyrim.browsing, Equipment.browsing, Equipment.panel._visible, Equipment.hero._visible], [true, true, false, false]);
   said("the panel takes its doll and slot models away", setup.plugins.EquipmentPanel.calls.filter((call) => call.startsWith("HidePlayer") || call.startsWith("ShowSlots")), ["HidePlayer", "ShowSlots [[]]"]);
   said("the browser stands in the panel's room", [KnowYourSkyrim.browser._parent === Equipment.browserRoom, KnowYourSkyrim.browser._x, KnowYourSkyrim.browser.high], [true, 600, 600 + 18 - 40]);
   said("the item card stays the panel's business", setup.menu.itemCard._parent._visible, true);
   said("with the panel too the eye is gold while the browser is open", button.gold, true);

   Equipment.frame.right = 1200;
   Equipment.browserRoomChanged();
   said("a narrower list moves the browser with the room", KnowYourSkyrim.browser._x, 1200 - KnowYourSkyrim.browser.wide);

   KnowYourSkyrim.toggleBrowser();
   said("shut, the panel comes back", [KnowYourSkyrim.browsing, Equipment.browsing, Equipment.panel._visible, Equipment.shown.includes("showModels")], [false, false, true, true]);
   said("the tile is taken away", setup.plugins.KnowYourSkyrim.calls.some((call) => call[0] == "ShowTile" && call.length == 1), true);
}

// the browser starts first and was left open, it waits for the panel on its way
{
   const setup = menu({open: true, words: "iron"}, true);
   const KnowYourSkyrim = startBrowser(setup);
   frame(KnowYourSkyrim.browseButton);
   said("a browser left open waits for the panel on its way", [KnowYourSkyrim.browsing, KnowYourSkyrim.panelComing()], [false, true]);
   const Equipment = startPanel(setup);
   said("the panel makes no doll for a browser left open", Equipment.browsing, true);
   frame(Equipment.browserRoom);
   said("the panel meets the browser, it opens in the room", [KnowYourSkyrim.panel === Equipment, KnowYourSkyrim.browsing, KnowYourSkyrim.browser._parent === Equipment.browserRoom], [true, true, true]);
   said("the room stops waiting once met", Equipment.browserRoom.onEnterFrame, undefined);
   said("the words left in it come back", KnowYourSkyrim.browser.search.field.text, "iron");
}

// a browser left open whose movie never came gives the room back to the panel
{
   const setup = menu({open: true}, true);
   const Equipment = startPanel(setup);
   for (let i = 0; i <= Equipment.BROWSER_WAIT + 1; i++) {
      frame(Equipment.browserRoom);
   }
   said("no browser, the panel shows itself again", [Equipment.browsing, Equipment.panel._visible, Equipment.shown.includes("showModels")], [false, true, true]);
}

// the browser alone takes the place of the item card
{
   const setup = menu({open: true}, false);
   const KnowYourSkyrim = startBrowser(setup);
   frame(KnowYourSkyrim.browseButton);
   const browser = KnowYourSkyrim.browser;
   said("alone, a browser left open opens at once in a layer of the menu", [KnowYourSkyrim.browsing, browser._parent === setup.menu.knowYourSkyrimLayer, setup.menu.knowYourSkyrimLayer._xscale], [true, true, 100]);
   said("alone, right of the list to the edge of the screen", [browser._x, browser._y, browser.wide, browser.high], [10 + 530 + 25 + 7, 20 + 14, 1280 - 20 - 14 - 572, 680 - 14 - 34]);
   said("the item card and the game's model step out", [setup.menu.itemCard._parent._visible, setup.plugins.KnowYourSkyrim.calls.filter((call) => call[0] == "ShowPreview").map((call) => call[1])], [false, [false]]);
   said("the eye is gold while the browser is open", KnowYourSkyrim.browseButton.gold, true);
   const lists = setup.menu.inventoryLists;
   const previews = () => setup.plugins.KnowYourSkyrim.calls.filter((call) => call[0] == "ShowPreview").map((call) => call[1]);
   const kept = () => setup.plugins.KnowYourSkyrim.calls.filter((call) => call[0] == "KeepBrowser").pop()[1];
   setup.menu.itemCard.ShowQuantityMenu(5);
   said("a card that asks how many gets its place back", [KnowYourSkyrim.browsing, setup.menu.itemCard._parent._visible, setup.menu.quantityAsked], [false, true, 5]);
   said("the browser stays open for the next inventory meanwhile", [KnowYourSkyrim.browserState.open, kept()], [true, true]);
   lists.itemList.disableInput = true;
   frame(browser);
   said("it waits while the card asks", KnowYourSkyrim.browsing, false);
   lists.itemList.disableInput = false;
   frame(browser);
   said("and comes back once the card is answered", [KnowYourSkyrim.browsing, setup.menu.itemCard._parent._visible], [true, false]);
   setup.menu.AttemptChargeItem();
   said("so does one that charges with a soul gem", [KnowYourSkyrim.browsing, setup.menu.chargeAsked], [false, true]);
   for (let i = 0; i <= KnowYourSkyrim.CARD_WAIT; i++) {
      frame(browser);
   }
   said("no soul gem list came, the browser is back after a while", KnowYourSkyrim.browsing, true);
   said("the game's model comes back while the card asks", previews(), [false, true, false, true, false]);
   const tiles = () => setup.plugins.KnowYourSkyrim.calls.filter((call) => call[0] == "ShowTile");
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   setup.menu.ToggleMenuFade();
   said("the game's own zoom fades the menu, the browser stays under it", [KnowYourSkyrim.browsing, setup.menu.bFadedIn, KnowYourSkyrim.browserState.open], [true, false, true]);
   said("the tile steps out and the zoom gets the game's model", [tiles().pop().length, previews().pop()], [1, true]);
   setup.menu.ToggleMenuFade();
   said("back from the zoom the tile comes again", [tiles().pop().length, previews().pop()], [5, false]);
   KnowYourSkyrim.toggleBrowser();
   said("the button shuts it for the next inventory too, its eye no longer gold", [KnowYourSkyrim.browsing, kept(), KnowYourSkyrim.browseButton.gold], [false, false, false]);
}

// the next inventory gives back what was looked at before, the lists of the page opened and how far it was scrolled
{
   const setup = menu({open: true, selected: 0x12EB7, history: "77496,77497", lists: "holders,sellers", page: 3}, false);
   const KnowYourSkyrim = startBrowser(setup);
   frame(KnowYourSkyrim.browseButton);
   said("the state comes back as it was left", [KnowYourSkyrim.browsing, KnowYourSkyrim.browserState.selected, KnowYourSkyrim.browserHistory, KnowYourSkyrim.openListKeys(), KnowYourSkyrim.browser.details.page.first], [true, 0x12EB7, [77496, 77497], ["holders", "sellers"], 3]);
   const kept = setup.plugins.KnowYourSkyrim.calls.filter((call) => call[0] == "KeepBrowser").pop();
   said("and is kept again for the one after", kept.slice(8), ["77496,77497", "holders,sellers", 3, "names"]);
   KnowYourSkyrim.goBrowserItem(0x12EB8);
   said("another item opens with its lists shut", [KnowYourSkyrim.openListKeys(), KnowYourSkyrim.browserHistory.length], [[], 3]);
}

// the files of the game itself are told apart from mods in the list of plugins
{
   const KnowYourSkyrim = startBrowser(menu({}, false));
   const files = ["Skyrim.esm", "Update.esm", "Dawnguard.esm", "ccBGSSSE001-Fish.esm", "_ResourcePack.esl", "CCOR.esp", "Lux.esp"];
   said("base game, DLC, Creation Club and mods", files.map((file) => KnowYourSkyrim.modLabel(file)), ["Skyrim (base game)", "Update (base game)", "Dawnguard (DLC)", "ccBGSSSE001-Fish (Creation Club)", "_ResourcePack (Creation Club)", "CCOR", "Lux"]);
}

// SkyUI closes the inventory with a fade, a browser open then is open in the next inventory
{
   const setup = menu({}, false);
   const KnowYourSkyrim = startBrowser(setup);
   KnowYourSkyrim.toggleBrowser();
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   const calls = setup.plugins.KnowYourSkyrim.calls;
   const before = calls.filter((call) => call[0] == "ShowPreview").length;
   setup.menu.startMenuFade();
   const kept = calls.filter((call) => call[0] == "KeepBrowser").pop();
   said("closing keeps the browser open", [KnowYourSkyrim.browserState.open, kept[1], kept[6]], [true, true, 0x12EB7]);
   said("the tile goes and no model of the game's shows while the menu fades", [calls.filter((call) => call[0] == "ShowTile").pop().length, calls.filter((call) => call[0] == "ShowPreview").length], [1, before]);
}

// the panel comes while the browser was opened alone in the first frames, the browser moves into the room
{
   const setup = menu({}, true);
   const KnowYourSkyrim = startBrowser(setup);
   KnowYourSkyrim.toggleBrowser();
   const alone = KnowYourSkyrim.browser;
   const Equipment = startPanel(setup);
   frame(Equipment.browserRoom);
   said("moved into the panel's room", [alone.removed, KnowYourSkyrim.browser._parent === Equipment.browserRoom, KnowYourSkyrim.browsing, setup.menu.itemCard._parent._visible], [true, true, true, true]);
}

// the inspect view of the browser, over everything, and the keys that close it
{
   const setup = menu({}, false);
   const KnowYourSkyrim = startBrowser(setup);
   KnowYourSkyrim.toggleBrowser();
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   KnowYourSkyrim.inspectBrowserItem();
   said("inspecting the entry", [KnowYourSkyrim.inspecting, KnowYourSkyrim.viewer._visible, KnowYourSkyrim.viewer.title.text], [true, true, "Iron Sword"]);
   const lists = setup.menu.inventoryLists;
   said("the lists don't answer meanwhile, R can't drop the row behind it", [KnowYourSkyrim.listsAnswer(), lists.itemList.disableInput, lists.categoryList.disableInput], [false, true, true]);
   said("a key of the game goes nowhere", KnowYourSkyrim.inspectKey({value: "keyDown", navEquivalent: "up"}), true);
   KnowYourSkyrim.inspectKey({value: "keyDown", navEquivalent: "escape"});
   said("Esc closes it", [KnowYourSkyrim.inspecting, KnowYourSkyrim.viewer._visible, setup.plugins.KnowYourSkyrim.calls.some((call) => call[0] == "InspectEnd")], [false, false, true]);
   said("the lists answer again", [KnowYourSkyrim.listsAnswer(), lists.itemList.disableInput], [true, false]);
}

// the game's zoom key while the browser inspects, alone the view goes before the browser makes way
{
   const setup = menu({}, false);
   const KnowYourSkyrim = startBrowser(setup);
   KnowYourSkyrim.toggleBrowser();
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   KnowYourSkyrim.inspectBrowserItem();
   setup.menu.ToggleMenuFade();
   said("alone, the game's zoom ends the inspect view, the browser stays", [KnowYourSkyrim.inspecting, KnowYourSkyrim.viewer._visible, KnowYourSkyrim.browsing, setup.menu.inventoryLists.itemList.disableInput], [false, false, true, false]);
}

// with the panel the zoom key is the panel's, it closes the browser's inspect view as its own
{
   const setup = menu({}, true);
   const Equipment = startPanel(setup);
   frame(Equipment.browserRoom);
   const KnowYourSkyrim = startBrowser(setup);
   said("no inspect view of the browser, the key goes on", Equipment.browserInspectKey(), false);
   KnowYourSkyrim.toggleBrowser();
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   KnowYourSkyrim.inspectBrowserItem();
   said("the panel takes the key and closes the browser's view", [Equipment.browserInspectKey(), KnowYourSkyrim.inspecting, KnowYourSkyrim.browsing], [true, false, true]);
}

// an Inventory Reforged from before the browser was its own mod has no room for it, a browser left open opens alone
{
   const setup = menu({open: true}, true);
   setup.made.shared._global.Equipment = {};
   const KnowYourSkyrim = startBrowser(setup);
   frame(KnowYourSkyrim.browseButton);
   said("an old panel isn't waited for", [KnowYourSkyrim.panelComing(), KnowYourSkyrim.browsing, KnowYourSkyrim.browser._parent === setup.menu.knowYourSkyrimLayer], [false, true, true]);
}

// a panel whose movie never runs is waited for a while only
{
   const setup = menu({open: true}, true);
   const KnowYourSkyrim = startBrowser(setup);
   let frames = 0;
   while (KnowYourSkyrim.browseButton.onEnterFrame && frames < 1000) {
      frame(KnowYourSkyrim.browseButton);
      frames++;
   }
   said("the browser opens alone once the panel is late", [frames, KnowYourSkyrim.browsing, KnowYourSkyrim.panelComing()], [KnowYourSkyrim.PANEL_WAIT + 1, true, false]);
}

// the panel's inspect view keeps the browser still too
{
   const setup = menu({}, true);
   const Equipment = startPanel(setup);
   frame(Equipment.browserRoom);
   const KnowYourSkyrim = startBrowser(setup);
   Equipment.inspecting = true;
   said("the browser won't open under the panel's inspect view", [KnowYourSkyrim.listsAnswer(), KnowYourSkyrim.anyInspect()], [false, true]);
}

// the cheats: off as shipped, then a button to take an item and to kill or bring back the nearest one of a person
{
   const setup = menu({}, false);
   const plugin = setup.plugins.KnowYourSkyrim;
   const KnowYourSkyrim = startBrowser(setup);
   KnowYourSkyrim.toggleBrowser();
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   const details = KnowYourSkyrim.browser.details;
   said("no cheats while the switch is off", details.cheats._visible, false);
   plugin.cheatsOn = true;
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   said("an item can be taken", [details.cheats._visible, details.cheats.take._visible, details.cheats.kill._visible], [true, true, false]);
   said("TAKE stands after INSPECT on its line", [details.cheats._x, details.cheats._y], [details.inspect._x + details.inspect.wide + KnowYourSkyrim.CHEAT_GAP, details.inspect._y]);
   KnowYourSkyrim.cheat("take");
   said("taken, the browser says so", [plugin.calls.some((call) => call[0] == "CheatTake" && call[1] == 0x12EB7), KnowYourSkyrim.browser.count.text], [true, "Taken: Iron Sword"]);
   const count = details.cheats.count;
   said("a box for how many right after TAKE, one at first", [count._visible, count._x, count.field.text, plugin.calls.filter((call) => call[0] == "CheatTake").pop()[2]], [true, details.cheats.take.wide + KnowYourSkyrim.CHEAT_COUNT_GAP, "1", 1]);
   KnowYourSkyrim.typeInBrowser("count", "25");
   said("typing a count keeps the lists still", [KnowYourSkyrim.listsAnswer(), KnowYourSkyrim.browserTyping], [false, "count"]);
   KnowYourSkyrim.cheat("take");
   said("TAKE ends the typing and takes that many", [KnowYourSkyrim.browserTyping, plugin.calls.filter((call) => call[0] == "CheatTake").pop().slice(1), KnowYourSkyrim.browser.count.text], [undefined, [0x12EB7, 25], "Taken: Iron Sword (25)"]);
   KnowYourSkyrim.typeInBrowser("count", "");
   KnowYourSkyrim.endBrowserTyping(false);
   said("an empty box gives one", [KnowYourSkyrim.cheatCount, count.field.text], [1, "1"]);
   KnowYourSkyrim.typeInBrowser("count", "7");
   KnowYourSkyrim.browserKey({value: "keyDown", navEquivalent: "escape"});
   said("Esc leaves the count as it was", [KnowYourSkyrim.cheatCount, count.field.text, KnowYourSkyrim.listsAnswer()], [1, "1", true]);

   plugin.item = {id: 0xA2C94, name: "Lydia", icon: "hero", family: "living", living: true, mod: "Skyrim", chips: ["people"], edits: [], keywords: [], effects: [], races: [], records: 1};
   plugin.targets = {alive: 1, dead: 0};
   KnowYourSkyrim.showBrowserItem(0xA2C94);
   said("a living one can be killed, nothing to inspect", [details.cheats.kill._visible, details.cheats.raise._visible, details.cheats.take._visible, details.cheats._x], [true, false, false, details.inspect._x]);
   said("the page goes under the line of buttons", details.view._y >= details.cheats._y + KnowYourSkyrim.BROWSER_INSPECT_HIGH, true);
   plugin.killed = {name: "Lydia", place: "Breezehome, Whiterun"};
   KnowYourSkyrim.cheat("kill");
   said("killed, where", KnowYourSkyrim.browser.count.text, "Lydia killed, Breezehome, Whiterun");
   plugin.targets = {alive: 0, dead: 1};
   said("the buttons stay until the game did it", [details.cheats.kill._visible, details.cheats.raise._visible], [true, false]);
   KnowYourSkyrim.deedDone();
   said("once the plugin says it's done she can be brought back", [details.cheats.kill._visible, details.cheats.raise._visible], [false, true]);
   plugin.raised = {name: "Lydia", place: ""};
   KnowYourSkyrim.cheat("raise");
   said("brought back", KnowYourSkyrim.browser.count.text, "Lydia brought back");
   plugin.killed = {name: "Balgruuf the Greater", place: "Dragonsreach"};
   KnowYourSkyrim.cheat("kill");
   said("one the game keeps alive dies too", KnowYourSkyrim.browser.count.text, "Balgruuf the Greater killed, Dragonsreach");
   plugin.targets = {alive: 0, dead: 0};
   KnowYourSkyrim.showBrowserItem(0xA2C94);
   said("no one of them about, no buttons", details.cheats._visible, false);
   // a quest not begun starts or finishes, one going finishes, a done one offers nothing
   plugin.item = {id: 0x3C57A, name: "Hail Sithis!", quest: true, family: "quests", mod: "Skyrim", chips: [], edits: [], keywords: [], effects: [], state: 0};
   KnowYourSkyrim.showBrowserItem(0x3C57A);
   said("a quest not begun starts and finishes", [details.cheats.start._visible, details.cheats.finish._visible, details.cheats.take._visible], [true, true, false]);
   plugin.questDone = {stage: 200};
   KnowYourSkyrim.cheat("finish");
   said("finishing sets the stage that finishes it", [plugin.calls.filter((call) => call[0] == "CheatQuest").pop().slice(1), KnowYourSkyrim.browser.count.text], [[0x3C57A, true], "Hail Sithis! set to stage 200"]);
   plugin.questDone = {};
   KnowYourSkyrim.cheat("start");
   said("starting tells so", KnowYourSkyrim.browser.count.text, "Started: Hail Sithis!");
   plugin.item = Object.assign({}, plugin.item, {state: 1});
   KnowYourSkyrim.showBrowserItem(0x3C57A);
   said("a quest waiting for the player with no journal stage only finishes", [details.cheats.start._visible, details.cheats.finish._visible], [false, true]);
   plugin.item = Object.assign({}, plugin.item, {state: 1, beginsAt: 10});
   KnowYourSkyrim.showBrowserItem(0x3C57A);
   said("a quest waiting for the player starts and finishes", [details.cheats.start._visible, details.cheats.finish._visible], [true, true]);
   said("the field to find words goes across the card under the buttons", [details.find._x, details.find.wide == details.wide, details.find._y > details.cheats._y], [0, true, true]);
   plugin.questDone = {stage: 10};
   KnowYourSkyrim.cheat("start");
   said("a quest waiting starts by its first journal stage", KnowYourSkyrim.browser.count.text, "Hail Sithis! set to stage 10");
   plugin.item = Object.assign({}, plugin.item, {state: 2});
   KnowYourSkyrim.showBrowserItem(0x3C57A);
   said("a quest going only finishes", [details.cheats.start._visible, details.cheats.finish._visible], [false, true]);
   // the quest's page in the journal's look, its chains as a tree once the quests' files are in
   plugin.item = Object.assign({}, plugin.item, {objectives: [{text: "Kill Grelod", state: 1}, {text: "Escape", state: 3}]});
   KnowYourSkyrim.showBrowserItem(0x3C57A);
   KnowYourSkyrim.browserSources = {people: [], peopleMore: 0, things: [], thingsMore: 0, places: [], placesMore: 0, questsRead: true, rewards: [],
      before: [[{name: "Unbound", id: 1, state: 3}]], beforeMore: 0, beforeShown: 4, after: [[{name: "Way", id: 2, state: 0}, {name: "Blade", id: 3, state: 0}], [{name: "Way", id: 2, state: 0}, {name: "Horn", id: 4, state: 0}]],
      afterMore: 0, afterShown: 4, steps: [{stage: 10, text: "Talk", done: true, current: false}], locations: [], locationsMore: 0, locationsShown: 8};
   KnowYourSkyrim.drawBrowserPage(false);
   said("the quest's page draws its sections", details.page.lines.heads.length, 4);
   // objectives 2, rewards 1, stages 1, the tree: before, the quest, the trunk and two forks
   said("the chains go as a tree of their forks", details.page.lines.fields.length, 2 + 1 + 5 + 1);
   said("a quest's tile is small, an item's whole", [KnowYourSkyrim.tileOf(plugin.item), KnowYourSkyrim.tileOf({id: 1})], [KnowYourSkyrim.SMALL_TILE, KnowYourSkyrim.BROWSER_TILE]);
   plugin.item = Object.assign({}, plugin.item, {state: 3});
   KnowYourSkyrim.showBrowserItem(0x3C57A);
   said("a quest done offers nothing", details.cheats._visible, false);
}

// Words found on the page: every list opens, each place they stand counts, Enter walks to the next, Esc lets go
{
   const setup = menu({open: true}, false);
   const KnowYourSkyrim = startBrowser(setup);
   frame(KnowYourSkyrim.browseButton);
   const plugin = setup.plugins.KnowYourSkyrim;
   plugin.sources = {made: [], sold: [{name: "Belethor", id: 0x1A67C, level: 1}, {name: "Adrianne Avenicci", id: 0x13BA2, level: 1}], soldMore: 0, soldShown: 1,
      carried: [], dropped: [], harvested: [], chests: [], lies: [], common: 0, lists: 0};
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   frame(KnowYourSkyrim.browser);
   const details = KnowYourSkyrim.browser.details;
   said("the field to find words stands on every page", details.find._visible, true);
   said("a long list is cut before any words", KnowYourSkyrim.lineText(KnowYourSkyrim.sourceLines(KnowYourSkyrim.browserSources)[0]), "Sold by Belethor, 1 more");
   KnowYourSkyrim.typeInBrowser("find", "a");
   // the page is drawn anew a moment after the last key, the test's timer never runs by itself
   KnowYourSkyrim.findInPage();
   said("words open every list and ask for all of it", [KnowYourSkyrim.findAll, plugin.calls.filter((call) => call[0] == "BrowserSources").pop()[2]], [true, true]);
   said("every list stands open", KnowYourSkyrim.lineText(KnowYourSkyrim.sourceLines(KnowYourSkyrim.browserSources)[0]), "Sold by Belethor, Adrianne Avenicci");
   const places = KnowYourSkyrim.foundPlaces().length;
   said("each place the words stand is counted", details.find.count.text, "1/" + places);
   KnowYourSkyrim.findNext(1);
   said("Enter walks to the next", details.find.count.text, "2/" + places);
   KnowYourSkyrim.findNext(places - 1);
   said("and from the last back to the first", details.find.count.text, "1/" + places);
   KnowYourSkyrim.typeInBrowser("find", "Adrianne Avenicci");
   KnowYourSkyrim.findInPage();
   said("the case doesn't count", KnowYourSkyrim.foundPlaces().length, 1);
   KnowYourSkyrim.browserKey({navEquivalent: "escape", value: "keyDown"});
   said("Esc empties the field and shuts the lists again", [details.find.field.text, KnowYourSkyrim.findAll, details.find.count.text], ["", false, ""]);
}

// the search looks in the names, the descriptions or both, the button before the field takes the next
{
   const setup = menu({open: true, within: "texts"}, false);
   const KnowYourSkyrim = startBrowser(setup);
   frame(KnowYourSkyrim.browseButton);
   const plugin = setup.plugins.KnowYourSkyrim;
   const asked = () => plugin.calls.filter((call) => call[0] == "BrowserFind").pop()[1].within;
   said("the kind of search comes back as it was left", [KnowYourSkyrim.browserState.within, asked()], ["texts", "texts"]);
   said("the button says it", KnowYourSkyrim.browser.within.label.text, "DESCRIPTIONS");
   KnowYourSkyrim.nextSearchWithin();
   said("a press takes the next", [KnowYourSkyrim.browserState.within, asked(), KnowYourSkyrim.browser.within.label.text], ["both", "both", "BOTH"]);
   KnowYourSkyrim.nextSearchWithin();
   said("and from the last to names", asked(), "names");
   said("a search of an unknown kind is by names", KnowYourSkyrim.keptWithin("nonsense"), "names");
}

// the star of the details puts an entry among the favorites, the tabs count again
{
   const setup = menu({}, false);
   const plugin = setup.plugins.KnowYourSkyrim;
   const KnowYourSkyrim = startBrowser(setup);
   KnowYourSkyrim.toggleBrowser();
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   const details = KnowYourSkyrim.browser.details;
   said("a star in the corner of the details", [details.star._visible, details.star._x], [true, details.wide - KnowYourSkyrim.BROWSER_STAR]);
   const searched = plugin.calls.filter((call) => call[0] == "BrowserFind").length;
   details.star.onPress();
   said("starred, the search runs again", [plugin.starred[0x12EB7], KnowYourSkyrim.browserItem.favorite, plugin.calls.filter((call) => call[0] == "BrowserFind").length], [true, true, searched + 1]);
   details.star.onPress();
   said("pressed again, the star goes", [plugin.starred[0x12EB7], KnowYourSkyrim.browserItem.favorite], [false, false]);
   said("a tab of the favorites after All", [KnowYourSkyrim.browser.tabs.tab1.family], ["favorites"]);
   KnowYourSkyrim.chooseBrowserFamily("favorites");
   said("the favorites are asked for by name", plugin.calls.filter((call) => call[0] == "BrowserFind").pop()[1].family, "favorites");
}

// the things of a person in a list of the browser's own where SkyUI's list stood, taken and given with the cheats
{
   const setup = menu({}, false);
   const plugin = setup.plugins.KnowYourSkyrim;
   // SkyUI's lists as big as they stand on the screen
   setup.menu.inventoryLists._width = 530;
   setup.menu.inventoryLists._height = 600;
   const KnowYourSkyrim = startBrowser(setup);
   KnowYourSkyrim.toggleBrowser();
   plugin.item = {id: 0xA2C94, name: "Lydia", icon: "hero", family: "living", living: true, mod: "Skyrim", chips: ["people"], edits: [], keywords: [], effects: [], races: [], records: 1, about: 1};
   plugin.things = {name: "Lydia", rows: [{id: 0x13950, name: "Steel Armor", count: 1, icon: "cuirass", worn: true, entry: 0x13950}, {id: 0xF, name: "Gold", count: 30, icon: "coin", worn: false, entry: 0xF}]};
   plugin.mine = [{id: 0x12EB7, name: "Iron Sword", count: 2, icon: "sword", worn: false, entry: 0x12EB7}];
   KnowYourSkyrim.showBrowserItem(0xA2C94);
   const details = KnowYourSkyrim.browser.details;
   said("INVENTORY of one about, at the start of the line", [details.things._visible, details.things._x], [true, details.inspect._x]);
   details.things.onPress();
   const list = KnowYourSkyrim.belongingsClip;
   said("SkyUI's list and hints step out", [setup.menu.inventoryLists._visible, setup.menu.bottomBar.buttonPanel._visible], [false, false]);
   said("her things, worn first as the plugin sends them", [list.rows.row0.thing.name, list.rows.row1.thing.name, list.rows.row1.count.text], ["Steel Armor", "Gold", "30"]);
   said("without the cheats she has one tab and the foot is to look only", [list.tabs.tab1, list.foot.hint.text], [undefined, "Look only, the cheats let you take and give"]);
   list.rows.row1.onPressAux();
   said("without the cheats a right click opens nothing", KnowYourSkyrim.context, undefined);
   said("SkyUI's list gets no key meanwhile", KnowYourSkyrim.belongingsKey({value: "keyDown", navEquivalent: "enter"}), true);
   list.rows.row1.onPress();
   said("a thing picked shows its page", [KnowYourSkyrim.belongings.selected, KnowYourSkyrim.browserState.selected], [0xF, 0xF]);
   KnowYourSkyrim.belongingsKey({value: "keyDown", navEquivalent: "escape"});
   said("Esc puts SkyUI's list back", [KnowYourSkyrim.belongings, setup.menu.inventoryLists._visible, setup.menu.bottomBar.buttonPanel._visible], [undefined, true, true]);

   plugin.cheatsOn = true;
   KnowYourSkyrim.openBelongings(0xA2C94);
   const open = KnowYourSkyrim.belongingsClip;
   said("with the cheats a tab of the player's things, the foot says where taking is", [open.tabs.tab1 != undefined, open.foot.hint.text], [true, "Right click a thing to take it"]);
   open.rows.row1.onPressAux();
   said("a right click picks the thing and opens what can be done with it", [KnowYourSkyrim.belongings.selected, KnowYourSkyrim.contextText()], [0xF, "Take one | Take all (30)"]);
   said("a key while the menu is open is the menu's, Esc closes it and leaves the list open", [KnowYourSkyrim.contextKey({value: "keyDown", navEquivalent: "escape"}), KnowYourSkyrim.context, KnowYourSkyrim.belongings != undefined], [true, undefined, true]);
   open.rows.row1.onPressAux();
   KnowYourSkyrim.pressContext("all");
   said("all of the gold taken, the menu shut", [plugin.calls.filter((call) => call[0] == "BelongingsMove").pop().slice(1), KnowYourSkyrim.browser.count.text, KnowYourSkyrim.context], [[0xA2C94, 0xF, true, false], "Taken: Gold (30)", undefined]);
   open.rows.row0.onPressAux();
   said("one of a kind has no line for all", KnowYourSkyrim.contextText(), "Take one");
   KnowYourSkyrim.closeContext();
   open.tabs.tab1.onPress();
   said("the player's tab gives", [KnowYourSkyrim.belongings.player, open.rows.row0.thing.name, open.foot.hint.text], [true, "Iron Sword", "Right click a thing of yours to give it"]);
   open.rows.row0.onPressAux();
   said("the menu of the player's tab", KnowYourSkyrim.contextText(), "Give one | Give all (2)");
   KnowYourSkyrim.pressContext("one");
   said("one sword given", plugin.calls.filter((call) => call[0] == "BelongingsMove").pop().slice(1), [0xA2C94, 0x12EB7, false, true]);
   plugin.mine = [];
   KnowYourSkyrim.deedDone();
   said("the list asked again once the game moved it", [open.rows.row0._visible, open.rows.none._visible], [false, true]);
   KnowYourSkyrim.closeBrowser();
   said("shutting the browser shuts the list", [KnowYourSkyrim.belongings, setup.menu.inventoryLists._visible], [undefined, true]);
}

// Alone the browser keeps SkyUI's model area off while it's open, because the wheel over it zoomed the hidden model
{
   const setup = menu({}, false);
   const KnowYourSkyrim = startBrowser(setup);
   const area = setup.menu.mouseRotationRect;
   KnowYourSkyrim.toggleBrowser();
   said("open, the area takes nothing", [area._xscale, area._yscale], [0, 0]);
   KnowYourSkyrim.toggleBrowser();
   said("shut, the area is back as SkyUI laid it", [area._xscale, area._yscale], [522, 410]);
}
{
   const setup = menu({}, true);
   const Equipment = startPanel(setup);
   frame(Equipment.browserRoom);
   setup.menu.mouseRotationRect._xscale = 0;
   const KnowYourSkyrim = startBrowser(setup);
   KnowYourSkyrim.toggleBrowser();
   KnowYourSkyrim.toggleBrowser();
   said("with the panel the area stays the panel's", setup.menu.mouseRotationRect._xscale, 0);
}

// A gamepad alone: the click of the right stick opens the browser with the input in it, the cross walks rows and
// categories, A inspects, B steps back, the triggers walk the links, the game's calls for the list's row act on nothing
{
   const setup = menu({}, false);
   const KnowYourSkyrim = startBrowser(setup);
   const plugin = setup.plugins.KnowYourSkyrim;
   const lists = setup.menu.inventoryLists;
   const pad = (nav, code, value) => setup.menu.handleInput({value: value || "keyDown", navEquivalent: nav, skseKeycode: code}, []);
   const turns = () => plugin.calls.filter((call) => call[0] == "BrowserPadTurns").map((call) => call[1]);
   KnowYourSkyrim.padStick(false);
   said("with the keyboard the click does nothing", [KnowYourSkyrim.browsing, KnowYourSkyrim.padFocus], [false, undefined]);
   said("with the keyboard the zoom key is the game's while the browser is shut", KnowYourSkyrim.zoomKey(), false);
   KnowYourSkyrim.toggleBrowser();
   said("while it's open with no entry the key is kept from the game's zoom", [KnowYourSkyrim.zoomKey(), KnowYourSkyrim.inspecting], [true, false]);
   KnowYourSkyrim.showBrowserItem(0x12EB7);
   said("with an entry the key inspects it", [KnowYourSkyrim.zoomKey(), KnowYourSkyrim.inspecting], [true, true]);
   said("and closes the view again", [KnowYourSkyrim.zoomKey(), KnowYourSkyrim.inspecting], [true, false]);
   KnowYourSkyrim.toggleBrowser();
   setup.menu.SetPlatform(1);
   said("a gamepad takes over: SkyUI's zoom sign goes, the bar shows the way into the browser", [lists.zoomButtonHolder._visible, setup.menu.navPanel.shown], [false, ["Equip", "Browser"]]);
   said("on a gamepad the zoom key never zooms the game", KnowYourSkyrim.zoomKey(), true);
   KnowYourSkyrim.padStick(false);
   said("the click opens the browser with the input in it, no row of the list selected", [KnowYourSkyrim.browsing, KnowYourSkyrim.padFocus, lists.itemList.selectedIndex, KnowYourSkyrim.browserItem.name], [true, "browser", -1, "Iron Sword"]);
   said("the stick scrolls the page", turns(), [1]);
   said("the bar says what the buttons do", setup.menu.navPanel.shown.slice(0, 2), ["Inspect", "Favorite"]);
   pad("down");
   said("down picks the next row", KnowYourSkyrim.browserState.selected, 0x12EB8);
   KnowYourSkyrim.chooseBrowserFamily("weapons");
   pad("tab", 275);
   pad("tab", 275, "keyUp");
   said("RB as the game sends it, with a Tab of its own, is a subcategory and keeps the input", [KnowYourSkyrim.padFocus, KnowYourSkyrim.chosenBrowserChips()], ["browser", ["oneHanded"]]);
   pad("shifttab", 274);
   pad("shifttab", 274, "keyUp");
   said("LB before the first subcategory is all of the category", KnowYourSkyrim.chosenBrowserChips().length, 0);
   pad("shifttab", 274);
   pad("shifttab", 274, "keyUp");
   said("LB round to the last subcategory", KnowYourSkyrim.chosenBrowserChips().length, 1);
   KnowYourSkyrim.chooseBrowserFamily("");
   pad("left");
   said("left goes to the category before", KnowYourSkyrim.browserState.family, KnowYourSkyrim.BROWSER_FAMILIES[KnowYourSkyrim.BROWSER_FAMILIES.length - 1].key);
   pad("right");
   said("right comes back round", KnowYourSkyrim.browserState.family, KnowYourSkyrim.BROWSER_FAMILIES[0].key);
   setup.menu.AttemptEquip(0);
   setup.menu.DropItem();
   setup.menu.AttemptChargeItem();
   said("the game's calls for the list's row act on nothing, RB's charge doesn't make the browser step aside for the card", [setup.menu.equipAsked, setup.menu.dropAsked, setup.menu.chargeAsked, KnowYourSkyrim.browsing, KnowYourSkyrim.padFocus], [undefined, undefined, undefined, true, "browser"]);
   pad("enter", 276);
   said("A inspects the entry, the stick turns it", [KnowYourSkyrim.inspecting, turns().pop()], [true, 2]);
   pad("enter", 276, "keyUp");
   pad("right");
   said("right turns it by a step", plugin.calls.filter((call) => call[0] == "InspectTurn").pop(), ["InspectTurn", 15, 0]);
   pad("tab", 277);
   said("B closes the view, the input stays in the browser", [KnowYourSkyrim.inspecting, KnowYourSkyrim.padFocus, turns().pop()], [false, "browser", 1]);
   said("the B held on is kept from SkyUI", pad("tab", 277, "keyHold"), true);
   pad("tab", 277, "keyUp");
   pad(undefined, 279);
   pad(undefined, 279, "keyUp");
   said("Y stars the entry", plugin.starred[KnowYourSkyrim.browserItem.id], true);
   plugin.item = {id: 0x12EB9, name: "Iron Sword of Frost", icon: "sword", family: "weapons", mod: "Skyrim", chips: ["oneHanded"], original: {name: "Iron Sword", id: 0x12EB7}, edits: [], keywords: [], effects: [], variants: [], variantsMore: 0};
   KnowYourSkyrim.showBrowserItem(0x12EB9);
   plugin.item = undefined;
   said("an entry with links says so in the bar", setup.menu.navPanel.shown.indexOf("Links") >= 0, true);
   setup.menu.AttemptEquip(0);
   said("RT lights the first link", [KnowYourSkyrim.browserLinkLit.link.text, setup.menu.navPanel.shown[0]], ["Iron Sword", "Open link"]);
   setup.menu.AttemptEquip(1);
   said("LT goes back before the first, no link lit", KnowYourSkyrim.browserLinkLit, undefined);
   setup.menu.AttemptEquip(1);
   said("LT round to the last", KnowYourSkyrim.browserLinkLit != undefined, true);
   pad("tab", 277);
   pad("tab", 277, "keyUp");
   said("B puts the link out first", [KnowYourSkyrim.browserLinkLit, KnowYourSkyrim.padFocus], [undefined, "browser"]);
   setup.menu.AttemptEquip(0);
   pad("enter", 276);
   pad("enter", 276, "keyUp");
   said("A follows the lit link", KnowYourSkyrim.browserState.selected, 0x12EB7);
   pad("tab", 277);
   pad("tab", 277, "keyUp");
   said("B walks back what was looked at", KnowYourSkyrim.browserState.selected, 0x12EB9);
   while (KnowYourSkyrim.browserHistory.length > 0) {
      KnowYourSkyrim.browserHistory.pop();
   }
   pad("tab", 277);
   said("B with nothing to walk back gives the input to the list and its row back", [KnowYourSkyrim.padFocus, lists.itemList.selectedIndex, KnowYourSkyrim.browsing, turns().pop()], [undefined, 3, true, 0]);
   said("the B held on doesn't close the menu", pad("tab", 277, "keyHold"), true);
   pad("tab", 277, "keyUp");
   KnowYourSkyrim.padStick(false);
   said("a click goes back into the open browser", KnowYourSkyrim.padFocus, "browser");
   KnowYourSkyrim.padStick(true);
   said("a hold shuts it", [KnowYourSkyrim.browsing, KnowYourSkyrim.browserState.open, KnowYourSkyrim.padFocus, lists.itemList.selectedIndex], [false, false, undefined, 3]);
   KnowYourSkyrim.padStick(false);
   setup.menu.SetPlatform(0);
   said("the mouse takes over, the list has the input again", [KnowYourSkyrim.padFocus, lists.zoomButtonHolder._visible, turns().pop()], [undefined, true, 0]);
}

// the cheats and someone's things on a gamepad
{
   const setup = menu({}, false);
   const KnowYourSkyrim = startBrowser(setup);
   const plugin = setup.plugins.KnowYourSkyrim;
   const pad = (nav, code) => {
      setup.menu.handleInput({value: "keyDown", navEquivalent: nav, skseKeycode: code}, []);
      setup.menu.handleInput({value: "keyUp", navEquivalent: nav, skseKeycode: code}, []);
   };
   plugin.cheatsOn = true;
   setup.menu.SetPlatform(1);
   KnowYourSkyrim.padStick(false);
   said("X is the cheat in the bar", setup.menu.navPanel.shown.slice(0, 2), ["Inspect", "Take"]);
   pad(undefined, 278);
   said("X takes the entry", plugin.calls.some((call) => call[0] == "CheatTake"), true);
   plugin.item = {id: 0xA2C94, name: "Lydia", icon: "person", family: "people", mod: "Skyrim", living: true, about: 1, edits: [], keywords: [], effects: [], variants: [], variantsMore: 0};
   plugin.targets = {alive: 1, dead: 0};
   plugin.things = {name: "Lydia", rows: [{id: 0xF, name: "Gold", count: 30}]};
   plugin.mine = [{id: 0x12EB7, name: "Iron Sword", count: 1}];
   KnowYourSkyrim.showBrowserItem(0xA2C94);
   said("on someone about A shows their things, X kills", setup.menu.navPanel.shown.slice(0, 2), ["Inventory", "Kill"]);
   pad("enter", 276);
   said("A opens the list of their things with its own bar", [KnowYourSkyrim.belongings != undefined, setup.menu.navPanel.shown, setup.menu.bottomBar.buttonPanel._visible], [true, ["Take one", "Take all", "You", "Close"], true]);
   pad("down");
   pad(undefined, 278);
   said("X takes one of what's picked", plugin.calls.filter((call) => call[0] == "BelongingsMove").pop().slice(3), [false, false]);
   pad(undefined, 275);
   said("RB goes to the player's things", KnowYourSkyrim.belongings.player, true);
   pad("tab", 277);
   said("B shuts the list, the input stays in the browser", [KnowYourSkyrim.belongings, KnowYourSkyrim.padFocus, setup.menu.navPanel.shown[0]], [undefined, "browser", "Inventory"]);
}

// With the panel of Inventory Reforged the click walks the list, the panel and the browser in turn, one of the two
// decides on it as it stood before the click
{
   const setup = menu({}, true);
   const Equipment = startPanel(setup);
   const told = [];
   Equipment.padStick = function(held) {
      told.push(held ? "hold" : this.padFocus == undefined && !this.browsing ? "into the panel" : "out of the panel");
      this.padFocus = this.padFocus == undefined && !this.browsing ? {kind: "slot"} : undefined;
   };
   Equipment.leavePad = function() {
      told.push("left");
      this.padFocus = undefined;
   };
   Equipment.padMark = Equipment.padTurns = () => undefined;
   Equipment.browserShown = (shown) => {
      Equipment.browsing = shown;
   };
   frame(Equipment.browserRoom);
   const KnowYourSkyrim = startBrowser(setup);
   setup.menu.SetPlatform(1);
   KnowYourSkyrim.padStick(false);
   said("the browser's own word of the click is let go, the panel's decides", told, []);
   said("no way into the browser in the bar while it's shut, the click is the panel's", setup.menu.navPanel.shown, ["Equip"]);
   Equipment.padStick(false);
   said("from the list the click goes to the panel", [told, KnowYourSkyrim.browsing], [["into the panel"], false]);
   Equipment.padStick(false);
   said("from the panel it opens the browser with the input", [told, KnowYourSkyrim.browsing, KnowYourSkyrim.padFocus, Equipment.padFocus], [["into the panel", "left"], true, "browser", undefined]);
   Equipment.padStick(false);
   said("from the browser to the list", [KnowYourSkyrim.padFocus, KnowYourSkyrim.browsing], [undefined, true]);
   Equipment.padStick(false);
   said("from the list back into the open browser", [KnowYourSkyrim.padFocus, told.length], ["browser", 2]);
   Equipment.padStick(true);
   said("a hold in the browser shuts it, the bar no longer offers it, the click is the panel's again", [KnowYourSkyrim.browsing, KnowYourSkyrim.padFocus, setup.menu.navPanel.shown], [false, undefined, ["Equip"]]);
   Equipment.padStick(true);
   said("a hold on the list is the panel's", told.pop(), "hold");
}

// Living's twelve chips in a line of their own for each group, one chosen at a time, the skill of trainers a chip of
// its own while Trainers is chosen
{
   const setup = menu({}, false);
   const plugin = setup.plugins.KnowYourSkyrim;
   const KnowYourSkyrim = startBrowser(setup);
   const finds = () => plugin.calls.filter((call) => call[0] == "BrowserFind");
   const chipOf = (key) => {
      const holder = KnowYourSkyrim.browser.chips.row;
      for (let i = 1; holder["chip" + i]; i++) {
         if (holder["chip" + i].key == key) {
            return holder["chip" + i];
         }
      }
      return undefined;
   };
   KnowYourSkyrim.toggleBrowser();
   const first = KnowYourSkyrim.browser;
   said("All takes one line of chips", first.chipLines, 1);
   KnowYourSkyrim.chooseBrowserFamily("living");
   const laid = KnowYourSkyrim.browser;
   said("Living is laid again with two lines, the rows start lower", [laid !== first, laid.chipLines, laid.rows._y - first.rows._y], [true, 2, KnowYourSkyrim.BROWSER_STRIP]);
   said("the kinds on the first line, the roles under them from where the kinds start", [chipOf("dragons")._y, chipOf("merchants")._y, chipOf("merchants")._x], [0, KnowYourSkyrim.BROWSER_STRIP, chipOf("people")._x]);
   chipOf("dragons").onPress();
   chipOf("merchants").onPress();
   said("Merchants after Dragons lets Dragons go", finds().pop()[1].chips, ["merchants"]);
   said("no skill chip without Trainers", chipOf("skill"), undefined);
   chipOf("trainers").onPress();
   const skill = chipOf("skill");
   said("Trainers brings the chip of the skill after it", [skill != undefined, skill.words.text, skill._y], [true, "ANY SKILL", KnowYourSkyrim.BROWSER_STRIP]);
   skill.onPress();
   said("its menu: any skill, then the skills someone trains by school", KnowYourSkyrim.contextText(), "Any skill (chosen) | Archery (3) | Block (1) | Alteration (2)");
   KnowYourSkyrim.pressContext("skill8");
   said("a skill picked narrows the trainers and names the chip", [finds().pop()[1].chips, chipOf("skill").words.text, chipOf("skill").chosen], [["trainers", "skill:8"], "ARCHERY", true]);
   said("the skill is kept with the chips for the next inventory", plugin.calls.filter((call) => call[0] == "KeepBrowser").pop()[4], "trainers,skill:8");
   chipOf("story").onPress();
   said("another chip lets Trainers and its skill go", [finds().pop()[1].chips, KnowYourSkyrim.browserState.skill], [["story"], undefined]);
   KnowYourSkyrim.chooseBrowserFamily("weapons");
   said("Weapons is laid back with one line", KnowYourSkyrim.browser.chipLines, 1);
}
{
   const setup = menu({open: true, family: "living", chips: "trainers,skill:18"}, false);
   const KnowYourSkyrim = startBrowser(setup);
   frame(KnowYourSkyrim.browseButton);
   for (let i = 0; i < KnowYourSkyrim.PANEL_WAIT + 2; i++) {
      frame(KnowYourSkyrim.browseButton);
   }
   said("a kept skill comes back with Trainers, Living laid with two lines from the start", [KnowYourSkyrim.browserState.skill, KnowYourSkyrim.chosenBrowserChips(), KnowYourSkyrim.browser.chipLines], [18, ["trainers", "skill:18"], 2]);
}

// Perks: a line of chips for each school of skills and one for the other trees, the skills in the game's words asked
// once Perks shows, one tree at a time. A perk's tile is a sign, its cheats add the next rank and take the highest
{
   const setup = menu({}, false);
   const plugin = setup.plugins.KnowYourSkyrim;
   const KnowYourSkyrim = startBrowser(setup);
   const finds = () => plugin.calls.filter((call) => call[0] == "BrowserFind");
   const asked = (name) => plugin.calls.filter((call) => call[0] == name);
   const chipOf = (key) => {
      const holder = KnowYourSkyrim.browser.chips.row;
      for (let i = 1; holder["chip" + i]; i++) {
         if (holder["chip" + i].key == key) {
            return holder["chip" + i];
         }
      }
      return undefined;
   };
   KnowYourSkyrim.toggleBrowser();
   said("the trees aren't asked for before Perks shows", asked("BrowserPerkTrees").length, 0);
   KnowYourSkyrim.chooseBrowserFamily("perks");
   said("Perks is laid with a line for each group of trees", KnowYourSkyrim.browser.chipLines, 4);
   said("the trees take the game's names, one it names nothing keeps its own", [chipOf("oneHandedPerks").words.text, chipOf("vampirePerks").words.text, chipOf("werewolfPerks").words.text],
      ["ONE-HANDED", "VAMPIRE LORD", "WEREWOLF"]);
   said("the mage's skills on the third line, the other trees on the fourth from where the first starts",
      [chipOf("illusionPerks")._y, chipOf("vampirePerks")._y, chipOf("vampirePerks")._x], [2 * KnowYourSkyrim.BROWSER_STRIP, 3 * KnowYourSkyrim.BROWSER_STRIP, chipOf("oneHandedPerks")._x]);
   chipOf("archeryPerks").onPress();
   said("a tree narrows the perks", finds().pop()[1].chips, ["archeryPerks"]);
   chipOf("restorationPerks").onPress();
   said("a tree of another line lets the first go", finds().pop()[1].chips, ["restorationPerks"]);
   said("the trees are asked for once", asked("BrowserPerkTrees").length, 1);
   said("the tree is kept for the next inventory", asked("KeepBrowser").pop()[4], "restorationPerks");

   plugin.cheatsOn = true;
   plugin.item = {id: 0x79342, name: "Armsman", icon: "tome", family: "perks", perk: true, mod: "Skyrim", chips: ["oneHandedPerks"], edits: [], keywords: [], effects: [], ranks: 5, rank: 1};
   KnowYourSkyrim.showBrowserItem(0x79342);
   const details = KnowYourSkyrim.browser.details;
   said("a perk's tile is a small sign, no model asked, nothing to inspect", [KnowYourSkyrim.tileOf(plugin.item), asked("ShowTile").pop().length, details.inspect._visible], [KnowYourSkyrim.SMALL_TILE, 1, false]);
   said("its head tells the ranks and the player's", details.head.fields.map((field) => field.text).slice(1, 4), ["Perks, One-handed", "5 ranks", "You have rank 1 of 5"]);
   said("a rank to add and one to take away, nothing to take", [details.cheats.add._visible, details.cheats.remove._visible, details.cheats.take._visible, details.cheats._x], [true, true, false, details.inspect._x]);
   plugin.perkDone = {rank: 2, ranks: 5};
   KnowYourSkyrim.cheat("add");
   said("ADD gives the next rank and says which", [asked("CheatPerk").pop().slice(1), KnowYourSkyrim.browser.count.text], [[0x79342, true], "Added: Armsman, rank 2 of 5"]);
   plugin.item = Object.assign({}, plugin.item, {rank: 5});
   const items = asked("BrowserItem").length;
   KnowYourSkyrim.deedDone();
   said("once the game did it the perk is asked anew, with every rank had REMOVE alone stays", [asked("BrowserItem").length, details.cheats.add._visible, details.cheats.remove._visible],
      [items + 1, false, true]);
   {
      const frames = (count) => { for(let i = 0; i < count; i++) { if(details.cheats.onEnterFrame) details.cheats.onEnterFrame(); } };
      const before = asked("BrowserItem").length;
      KnowYourSkyrim.deedDone();
      frames(KnowYourSkyrim.PERK_SETTLE);
      said("a rank the game hasn't moved yet is asked again half a second later", [asked("BrowserItem").length, typeof details.cheats.onEnterFrame], [before + 2, "function"]);
      plugin.item = Object.assign({}, plugin.item, {rank: 4});
      frames(KnowYourSkyrim.PERK_SETTLE);
      said("once it moved the page stops asking", [asked("BrowserItem").length, details.cheats.onEnterFrame], [before + 3, undefined]);
      plugin.item = Object.assign({}, plugin.item, {rank: 5});
      KnowYourSkyrim.entryMoved(0x79342);
   }
   plugin.perkDone = {rank: 5, ranks: 5};
   KnowYourSkyrim.cheat("remove");
   said("REMOVE takes the highest rank", [asked("CheatPerk").pop().slice(1), KnowYourSkyrim.browser.count.text], [[0x79342, false], "Removed: Armsman, rank 5 of 5"]);
   plugin.item = Object.assign({}, plugin.item, {rank: 0});
   setup.menu.SetPlatform(1);
   KnowYourSkyrim.padStick(false);
   KnowYourSkyrim.showBrowserItem(0x79342);
   said("on a gamepad X adds the perk, A has nothing of its own", setup.menu.navPanel.shown.slice(0, 2), ["Add", "Favorite"]);
   plugin.cheatsOn = false;
   KnowYourSkyrim.showBrowserItem(0x79342);
   said("no cheats on a perk while the switch is off", details.cheats._visible, false);
}

// the look button of the panel alone follows SkyUI's hints
{
   const setup = menu({}, true);
   delete setup.plugins.KnowYourSkyrim;
   const Equipment = startPanel(setup);
   frame(Equipment.browserRoom);
   said("the look button after the hints", [Equipment.lookButton._x, Equipment.lookButton._y], [20 + 160 + 24, Math.round((26 - 22) / 2)]);
   said("the panel alone never waits for a browser", Equipment.browsing, false);
}

// an inventory not made by SkyUI has no lists to join, the browser stops looking for them
{
   const root = clip("_root");
   const context = world(root, {KnowYourSkyrim: browserPlugin({})}).context();
   const KnowYourSkyrim = run(context, browserMovie)._global.KnowYourSkyrim;
   let frames = 0;
   while (context.onEnterFrame && frames < 1000) {
      context.onEnterFrame();
      frames++;
   }
   said("the search gives up after its frames", [frames, context.onEnterFrame, KnowYourSkyrim.menu], [KnowYourSkyrim.MENU_WAIT, undefined, undefined]);
}

done();
