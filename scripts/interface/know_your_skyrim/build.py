"""Build Know your Skyrim, the item browser for SkyUI's inventory

    python -m interface.know_your_skyrim.build
    python -m interface.know_your_skyrim.build --parts     the joined parts of the movie as JSON, for the tests

The browser is a script movie of its own, Interface/KnowYourSkyrim/browser.swf. Its plugin loads it into SkyUI's
inventory as it opens, no file of SkyUI is shipped or replaced. The parts in source run one after another in its first
frame. Right after 10-core come the helpers taken by name from the panel of Inventory Reforged (borrowed.py), so both
mods draw and word things from one source. Beside the movie go the plugin from plugins/know-your-skyrim (built with
plugins/build.ps1 know-your-skyrim), its ini with the switch of the cheats, the English of every word under each
language of the game (KnowYourSkyrim_<LANGUAGE>.txt, keys "$KYSK_<words>"), and the mod's menu in SkyUI's own Mod
Configuration made from menu/config.json the way Inventory Reforged makes its own: a light plugin with the quest of
the menu and its scripts. The menu is in English with no keys: keys need the game's translator, and touching it took
the game down at its start for players whose mods replace it (1.0.1)
"""

import configparser
import json
import re
import shutil
import sys
from pathlib import Path

from common.paths import BUILT, PROJECT
from interface.equipment import translations
from interface.know_your_skyrim import borrowed
from interface.swf import script_movie

SOURCE = Path(__file__).parent / "source"
NAME = borrowed.NAME
STEM = "KnowYourSkyrim"
PREFIX = "$KYSK_"
WORK = BUILT / "know-your-skyrim"
PACKAGE = WORK / "package" / "Know your Skyrim"
PLUGIN_FOLDER = PROJECT / "plugins" / "know-your-skyrim"
PLUGIN = PLUGIN_FOLDER / "build" / f"{STEM}.dll"
# the ini with the switch of the cheats, off as it ships
SETTINGS = Path("SKSE") / "Plugins" / f"{STEM}.ini"
SHIPPED = PLUGIN_FOLDER / "package"
MOVIE = Path("Interface") / STEM / "browser.swf"
MENU = Path(__file__).parent / "menu"
MENU_PAGES = MENU / "config.json"
MENU_SCRIPT = f"{STEM}Menu"
MENU_PLUGIN = f"{STEM}.esp"
NATIVE_SCRIPT = STEM
MENU_QUEST = 0x800
AUTHOR = "laymovi"

# the parts after the namespace and the helpers taken from the panel
FIRST = "10-core.as"
PARTS = [
    "20-families.as",
    "25-button.as",
    "30-browser.as",
    "31-search.as",
    "32-rows.as",
    "33-details.as",
    "33-find.as",
    "33-journal.as",
    "34-pages.as",
    "34-perks.as",
    "35-inspect.as",
    "36-cheats.as",
    "36-context.as",
    "37-belongings.as",
    "38-stand.as",
    "39-pad.as",
    "40-start.as",
]


def own_scripts() -> list[str]:
    return [(SOURCE / part).read_text(encoding="utf-8") for part in [FIRST, *PARTS]]


def parts() -> list[tuple[str, str]]:
    """Every part of the movie in the order it runs, as (name, script)"""
    own = own_scripts()
    _, taken = borrowed.taken(own)
    return [(FIRST, own[0]), ("borrowed", taken), *zip(PARTS, own[1:])]


def unknown(scripts: list[str]) -> list[str]:
    """Names called as KnowYourSkyrim.name that no part ever sets: Scaleform calls a missing function silently"""
    text = "\n".join(scripts)
    set_names = set(re.findall(rf"{NAME}\.(\w+)\s*=(?!=)", text))
    used = set(re.findall(rf"{NAME}\.(\w+)\s*\(", text))
    return sorted(used - set_names)


def placed(source: Path, target: Path) -> None:
    """A file of the package is hard linked elsewhere, so it's replaced and never written through"""
    if not source.exists():
        raise FileNotFoundError(source)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.unlink(missing_ok=True)
    shutil.copy2(source, target)


def starts() -> dict[str, float]:
    """What every control of the pages starts from, as the shipped ini says"""
    from interface import mcm
    told = configparser.ConfigParser(comment_prefixes=(";",))
    told.optionxform = str
    told.read(SHIPPED / SETTINGS, encoding="utf-8")
    found: dict[str, float] = {}
    for control in mcm.controls(MENU_PAGES):
        key, section = control.split(":")
        if not told.has_option(section, key):
            raise ValueError(f"the menu has {control}, the ini has no such setting")
        found[control] = float(told[section][key])
    return found


def menu_version() -> int:
    """The mod's version as SkyUI counts a menu's, 1.3.0 as 10300: a save made before gets the pages of this one"""
    told = re.search(rf"project\({STEM} VERSION (\d+)\.(\d+)\.(\d+)", (PLUGIN_FOLDER / "CMakeLists.txt").read_text(encoding="utf-8"))
    major, minor, patch = (int(part) for part in told.groups())
    return major * 10000 + minor * 100 + patch


def menu(target: Path) -> list[str]:
    """The menu in SkyUI's own Mod Configuration: its script made from the pages, the plugin with its quest. The
    builders of menus come in here only, the parts the tests ask for need none of what they bring"""
    from interface import mcm, skyui_menu
    defaults = starts()
    shown = json.loads(MENU_PAGES.read_text(encoding="utf-8"))["displayName"]
    source = WORK / "build" / "menu"
    source.mkdir(parents=True, exist_ok=True)
    (source / f"{MENU_SCRIPT}.psc").write_text(skyui_menu.script(MENU_PAGES, MENU_SCRIPT, shown, NATIVE_SCRIPT, defaults, menu_version()), encoding="utf-8")
    mcm.compiled(MENU, NATIVE_SCRIPT, target)
    script = mcm.compiled(source, MENU_SCRIPT, target, also=(MENU,))
    esp = mcm.plugin(target / MENU_PLUGIN, MENU_PAGES, MENU_SCRIPT, MENU_QUEST, AUTHOR, shown)
    return [f"menu: {len(defaults)} settings in SkyUI's Mod Configuration, {script.name}, {esp.name}"]


def main() -> None:
    # the package starts empty, a file left from an older name would be installed beside the new one
    shutil.rmtree(PACKAGE, ignore_errors=True)
    joined = parts()
    scripts = [script for _, script in joined]
    missing = unknown(scripts)
    if missing:
        raise ValueError(f"the scripts call functions nobody defines: {missing}")
    names, _ = borrowed.taken(own_scripts())
    print(f"taken from the panel: {len(names)} names")
    movie = PACKAGE / MOVIE
    script_movie(movie, scripts, WORK / "build" / "browser")
    print(f"movie: {movie} {movie.stat().st_size} bytes")

    placed(PLUGIN, PACKAGE / "SKSE" / "Plugins" / PLUGIN.name)
    placed(SHIPPED / SETTINGS, PACKAGE / SETTINGS)
    print(f"plugin: {PLUGIN.stat().st_size} bytes")

    entries = translations.lines(translations.panel_words(scripts, NAME), {}, PREFIX)
    files = translations.write(PACKAGE, entries, stem=STEM)
    print(f"translations: {len(entries)} lines in {len(files)} languages, {files[0].name}")
    for line in menu(PACKAGE):
        print(line)


if __name__ == "__main__":
    if "--parts" in sys.argv:
        sys.stdout.reconfigure(encoding="utf-8")
        print(json.dumps([{"name": name, "script": script} for name, script in parts()], ensure_ascii=False))
    else:
        main()
