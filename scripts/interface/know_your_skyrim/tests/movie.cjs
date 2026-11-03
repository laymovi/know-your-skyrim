// What the tests of Know your Skyrim run on: the parts of the movie as the build joins them, the panel's own parts, and
// stand-ins for the clips, text fields and objects of Flash and SkyUI that the parts touch
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const {execFileSync} = require("child_process");

const scripts = path.join(__dirname, "..", "..", "..");
const panelSource = path.join(scripts, "interface", "equipment", "source");

// The parts as the movie runs them, the helpers taken from the panel among them. The build tells them, so the tests
// see what the game sees
function browserParts() {
   const out = execFileSync("python", ["-m", "interface.know_your_skyrim.build", "--parts"], {cwd: scripts, encoding: "utf8", maxBuffer: 64 << 20});
   return JSON.parse(out);
}

function panelPart(name) {
   return {name, script: fs.readFileSync(path.join(panelSource, name), "utf8")};
}

// A text field: as wide as it was made, its words seven units a letter
function field(name, parent, x, y, wide, high) {
   const made = {_name: name, _parent: parent, _x: x || 0, _y: y || 0, _width: wide || 10, _height: high || 10, _visible: true, _alpha: 100, text: "", textColor: 0, links: []};
   Object.defineProperty(made, "textWidth", {get: () => String(made.text).length * 7});
   // a field that sizes itself to its words is as wide as they are, the way Flash keeps it
   let fixed = made._width;
   delete made._width;
   Object.defineProperty(made, "_width", {get: () => (made.autoSize != undefined && made.autoSize != "none" ? made.textWidth + 4 : fixed), set: (value) => { fixed = value; }, enumerable: true});
   Object.defineProperty(made, "textHeight", {get: () => 16});
   for (const noop of ["setNewTextFormat", "setTextFormat"]) {
      made[noop] = () => undefined;
   }
   made.getNewTextFormat = () => ({});
   made.getCharIndexAtPoint = () => -1;
   made.getCharBoundaries = () => ({x: 0, y: 0, width: 7, height: 16});
   made.getLineMetrics = () => ({x: 0});
   return made;
}

// A movie clip: children by name, places added up its parents for the stage, drawing does nothing
function clip(name, parent, depth) {
   const made = {_name: name, _parent: parent, _x: 0, _y: 0, _visible: true, _alpha: 100, _xscale: 100, _yscale: 100, _width: 100, _height: 100, depth, children: {}};
   made.createEmptyMovieClip = (child, at) => {
      const one = clip(child, made, at);
      made[child] = one;
      made.children[at] = one;
      return one;
   };
   made.createTextField = (child, at, x, y, wide, high) => {
      const one = field(child, made, x, y, wide, high);
      made[child] = one;
      return one;
   };
   made.getNextHighestDepth = () => Math.max(-1, ...Object.keys(made.children).map(Number)) + 1;
   made.removeMovieClip = () => {
      made.removed = true;
      if (parent && parent[name] === made) {
         delete parent[name];
      }
   };
   made.swapDepths = (at) => {
      made.depth = at;
   };
   for (const noop of ["clear", "lineStyle", "beginFill", "beginGradientFill", "endFill", "moveTo", "lineTo", "curveTo", "setMask"]) {
      made[noop] = () => undefined;
   }
   const offset = () => {
      let x = 0;
      let y = 0;
      for (let at = made; at; at = at._parent) {
         x += at._x || 0;
         y += at._y || 0;
      }
      return {x, y};
   };
   made.localToGlobal = (point) => {
      const by = offset();
      point.x += by.x;
      point.y += by.y;
   };
   made.globalToLocal = (point) => {
      const by = offset();
      point.x -= by.x;
      point.y -= by.y;
   };
   made.getBounds = () => {
      const by = offset();
      return {xMin: by.x, yMin: by.y, xMax: by.x + (made.wide || made._width), yMax: by.y + (made.high || made._height)};
   };
   made.hitTest = () => false;
   return made;
}

// SkyUI's inventory as far as the parts look at it
function inventory() {
   const root = clip("_root");
   const menu = root.createEmptyMovieClip("Menu_mc", 0);
   menu.bFadedIn = true;
   menu._config = {};
   menu.inventoryLists = clip("inventoryLists", menu);
   menu.inventoryLists._x = 10;
   menu.inventoryLists.getContentBounds = () => [0, 0, 530, 600];
   menu.inventoryLists.itemList = {disableInput: false, disableSelection: false, selectedIndex: 3, getListEnumIndex: (at) => (at < 10 ? at : undefined)};
   menu.inventoryLists.categoryList = {disableInput: false};
   menu.inventoryLists.zoomButtonHolder = clip("zoomButtonHolder", menu.inventoryLists);
   // the game tells the menu whether a gamepad (1) or the keyboard and the mouse (0) play
   menu._platform = 0;
   menu.SetPlatform = function(platform) {
      this._platform = platform;
   };
   // the bottom bar's buttons as SkyUI fills them, the words of each in shown
   menu.navPanel = {shown: []};
   menu.navPanel.clearButtons = () => {
      menu.navPanel.shown = [];
   };
   menu.navPanel.addButton = (button) => {
      menu.navPanel.shown.push(button.text);
   };
   menu.navPanel.updateButtons = () => undefined;
   menu.updateBottomBar = function(selected) {
      this.navPanel.clearButtons();
      this.navPanel.addButton({text: selected ? "Equip" : "Exit"});
   };
   // the game's calls for the selected row of the list
   menu.AttemptEquip = function(hand) {
      this.equipAsked = hand;
   };
   menu.DropItem = function() {
      this.dropAsked = true;
   };
   menu.inventoryLists.searchWidget = {_bActive: false};
   // the area SkyUI turns and zooms the game's model in, laid out by width and height
   menu.mouseRotationRect = clip("mouseRotationRect", menu);
   menu.mouseRotationRect._xscale = 522;
   menu.mouseRotationRect._yscale = 410;
   menu.bottomBar = menu.createEmptyMovieClip("bottomBar", 5);
   menu.bottomBar._y = 680;
   const hints = menu.bottomBar.createEmptyMovieClip("buttonPanel", 0);
   hints._x = 20;
   hints.buttons = [{_visible: true, label: "Equip", _x: 0, _width: 80, _height: 26}, {_visible: true, label: "Drop", _x: 90, _width: 70, _height: 26}];
   hints.doUpdateButtons = () => undefined;
   menu.bottomBar.playerInfoCard = {_x: 900};
   const holder = menu.createEmptyMovieClip("itemCardFadeHolder", 1);
   menu.itemCard = holder.createEmptyMovieClip("ItemCard_mc", 0);
   menu.itemCard.ShowQuantityMenu = (count) => {
      menu.quantityAsked = count;
   };
   menu.AttemptChargeItem = () => {
      menu.chargeAsked = true;
   };
   menu.ToggleMenuFade = function() {
      this.bFadedIn = !this.bFadedIn;
   };
   // SkyUI closes the inventory on Tab with a fade
   menu.startMenuFade = function() {
      this.ToggleMenuFade();
   };
   menu.handleInput = () => false;
   root._xmouse = root._ymouse = 0;
   return {root, menu};
}

// The objects of Flash, SkyUI and SKSE the parts call, one set for every movie of a menu, because they share _global
function world(root, plugins) {
   const shared = {_global: {}, skse: {plugins: plugins || {}, AllowTextInput: () => undefined, Log: () => undefined}};
   return {
      shared,
      context: () => ({
         _global: shared._global,
         skse: shared.skse,
         _root: root,
         Math, String, Number, Array, isNaN,
         getTimer: () => Date.now(),
         setInterval: () => 0,
         clearInterval: () => undefined,
         MovieClip: function() {},
         TextField: function() {},
         TextFormat: function() {},
         Stage: {visibleRect: {x: 0, y: 0, width: 1280, height: 720}, safeRect: {x: 20, y: 20}},
         Mouse: {addListener: () => undefined, getTopMostEntity: () => undefined},
         Selection: {setFocus: () => undefined, setSelection: () => undefined},
         flash: {geom: {Matrix: function() {
            this.createGradientBox = () => undefined;
         }}},
         skyui: {util: {Translator: {translate: (key) => key}}},
         gfx: {
            io: {GameDelegate: {call: () => undefined}},
            managers: {FocusHandler: {instance: {getFocus: () => undefined}}},
            ui: {NavigationCode: {UP: "up", DOWN: "down", LEFT: "left", RIGHT: "right", ENTER: "enter", ESCAPE: "escape", TAB: "tab", SHIFT_TAB: "shifttab"}},
         },
         Shared: {GlobalFunc: {IsKeyPressed: (details) => details.value == "keyDown"}},
      }),
   };
}

function run(context, parts) {
   vm.createContext(context);
   for (const part of parts) {
      vm.runInContext(part.script, context, {filename: part.name});
   }
   return context;
}

// The browser's plugin as far as the parts call it, what each call was handed is kept in calls
function browserPlugin(kept) {
   const calls = [];
   const page = {total: 2, first: 0, all: 2, families: {}, rows: [{id: 0x12EB7, name: "Iron Sword", mod: "Skyrim", icon: "sword", family: "weapons", variants: 0}, {id: 0x12EB8, name: "Iron Dagger", mod: "Skyrim", icon: "dagger", family: "weapons", variants: 0}]};
   const plugin = {calls};
   const keep = (name, value) => (...args) => {
      calls.push([name, ...args]);
      return typeof value == "function" ? value(...args) : value;
   };
   Object.assign(plugin, {
      BrowserKept: keep("BrowserKept", () => Object.assign({open: false, words: "", family: "", chips: "", first: 0, selected: 0, mod: ""}, kept)),
      KeepBrowser: keep("KeepBrowser"),
      BrowserFind: keep("BrowserFind", page),
      BrowserItem: keep("BrowserItem", (id) => plugin.item || {id, name: "Iron Sword", icon: "sword", family: "weapons", mod: "Skyrim", chips: ["oneHanded"], edits: [], keywords: [], effects: [], variants: [], variantsMore: 0}),
      BrowserSources: keep("BrowserSources", (id, whole) => plugin.sources == undefined ? undefined : Object.assign({}, plugin.sources, {whole: whole == true})),
      BrowserMods: keep("BrowserMods", []),
      BrowserMap: keep("BrowserMap", true),
      BrowserTyping: keep("BrowserTyping"),
      ShowTile: keep("ShowTile", true),
      Cover: keep("Cover"),
      ShowPreview: keep("ShowPreview"),
      Inspect: keep("Inspect", true),
      InspectTurn: keep("InspectTurn"),
      InspectZoom: keep("InspectZoom"),
      InspectEnd: keep("InspectEnd"),
      BrowserPadTurns: keep("BrowserPadTurns"),
      BrowserTrainerSkills: keep("BrowserTrainerSkills", () => [{skill: 8, name: "Archery", count: 3}, {skill: 9, name: "Block", count: 1}, {skill: 18, name: "Alteration", count: 2}]),
      // the game names the skills' trees and the vampire's, the werewolf's has no name
      BrowserPerkTrees: keep("BrowserPerkTrees", () => [{key: "oneHandedPerks", name: "One-handed"}, {key: "archeryPerks", name: "Archery"}, {key: "vampirePerks", name: "Vampire Lord"}]),
      Words: keep("Words", {}),
      // the cheats answer what the test put into plugin.cheatsOn, targets, killed and raised
      Cheats: keep("Cheats", () => plugin.cheatsOn == true),
      CheatTargets: keep("CheatTargets", () => plugin.targets),
      CheatKill: keep("CheatKill", () => plugin.killed),
      CheatRaise: keep("CheatRaise", () => plugin.raised),
      CheatTake: keep("CheatTake", () => plugin.item == undefined || plugin.item.living != true),
      CheatQuest: keep("CheatQuest", () => plugin.questDone),
      CheatPerk: keep("CheatPerk", () => plugin.perkDone),
      // the entries the character hasn't met, by id, put into plugin.masks
      BrowserMask: keep("BrowserMask", (id) => plugin.masks == undefined ? undefined : plugin.masks[id]),
      // someone's things and the player's are what the test put into plugin.things and plugin.mine
      Belongings: keep("Belongings", (id, player) => plugin.things == undefined ? undefined : {name: plugin.things.name, dead: false, rows: player ? plugin.mine : plugin.things.rows}),
      BelongingsMove: keep("BelongingsMove", () => plugin.cheatsOn == true),
      // the stars the test put on, by id
      BrowserFavorite: keep("BrowserFavorite", (id, on) => {
         plugin.starred = Object.assign({}, plugin.starred, {[id]: on});
         return on;
      }),
   });
   return plugin;
}

let failed = 0;
let passed = 0;
function said(name, got, wanted) {
   const shown = JSON.stringify(got);
   if (shown === JSON.stringify(wanted)) {
      passed++;
   } else {
      failed++;
      console.log(`FAIL ${name}: got ${shown}, wanted ${JSON.stringify(wanted)}`);
   }
}

function done() {
   console.log(`${passed} passed, ${failed} failed`);
   process.exit(failed ? 1 : 0);
}

module.exports = {browserParts, panelPart, panelSource, clip, inventory, world, run, browserPlugin, said, done};
