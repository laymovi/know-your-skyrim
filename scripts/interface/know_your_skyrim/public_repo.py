"""Lay Know your Skyrim out as its own repository, only what the mod is built from, in the same relative places

    python -m interface.know_your_skyrim.public_repo [folder]

Files come from here, public/ adds the readme, its pictures and the few files that differ outside this project. The
plugin and the movie also build from Inventory Reforged's files, those come along by what CMakeLists.txt and borrowed.py
name. A file with edits not committed stops it, another's work under way never goes out. So does a file edited by hand in
the folder, see common.laying. Nothing in the folder is ever removed, a file that no longer belongs is only named. The words that shouldn't go public are looked for in every name
and every file of text, the lines are named and it's up to a person what happens to them
"""

import argparse
import re
import subprocess
from pathlib import Path

from common import laying
from common.paths import PROJECT
from interface.know_your_skyrim import borrowed

TARGET = PROJECT.parents[1] / "know-your-skyrim"
BROWSER = "scripts/interface/know_your_skyrim"
PANEL_MOVIE = "scripts/interface/equipment"
PLUGIN = "plugins/know-your-skyrim"
HERE = PROJECT / BROWSER / "public"
# whole folders
FOLDERS = [
    f"{PLUGIN}/source",
    f"{PLUGIN}/package",
    "plugins/cmake/ports",
    "scripts/interface/papyrus-headers",
    f"{BROWSER}/source",
    f"{BROWSER}/menu",
    f"{BROWSER}/tests",
]
FILES = [
    "plugins/build.ps1",
    f"{PLUGIN}/CMakeLists.txt",
    f"{PLUGIN}/CMakePresets.json",
    f"{PLUGIN}/vcpkg.json",
    "scripts/common/__init__.py",
    "scripts/common/dll.py",
    "scripts/common/pex.py",
    "scripts/common/fomod.py",
    "scripts/common/laying.py",
    "scripts/interface/swf.py",
    "scripts/interface/mcm.py",
    "scripts/interface/skyui_menu.py",
    *(f"{BROWSER}/{name}.py" for name in ("borrowed", "build", "public_repo", "release")),
    f"{BROWSER}/archive/readme.txt",
    f"{PANEL_MOVIE}/icons.py",
    f"{PANEL_MOVIE}/translations.py",
]
# the panel's part the tests of both movies in one menu run, the movie itself takes none of it
PANEL_PART_FOR_TESTS = "38-look.as"
SKIPPED = {"__pycache__"}
ADDED = {
    "README.md": "README.md",
    "README.uk.md": "README.uk.md",
    "compatibilities.md": "compatibilities.md",
    "LICENSE": "LICENSE",
    "gitignore": ".gitignore",
    "gitattributes": ".gitattributes",
    "editorconfig": ".editorconfig",
    "paths.py": "scripts/common/paths.py",
    "papyrus.py": "scripts/interface/papyrus.py",
}
BINARY = {".jpg", ".png", ".gif", ".dds", ".swf", ".pex", ".esp"}
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)
# one pattern a line, this file stays in the project and never goes out itself
PRIVATE_WORDS = PROJECT / "scripts" / "interface" / "equipment" / "public" / "private-words.txt"


def laid(source: Path, target: Path) -> None:
    """A file with the same bytes stays as it is, so laying the tree again doesn't make a build there start over"""
    target.parent.mkdir(parents=True, exist_ok=True)
    data = source.read_bytes()
    if source.suffix.lower() not in BINARY:
        data = data.replace(b"\r\n", b"\n")
    if not target.exists() or target.read_bytes() != data:
        target.write_bytes(data)


def plugin_files_beyond_its_folder() -> set[Path]:
    """What CMakeLists.txt compiles from Inventory Reforged and the shared code, with every header those include"""
    plugin = PROJECT / PLUGIN
    cmake = (plugin / "CMakeLists.txt").read_text(encoding="utf-8")
    panel = (plugin / re.search(r"set\(panel (\S+)\)", cmake).group(1)).resolve()
    shared = (plugin / "../shared/source").resolve()
    roots = [panel, shared, plugin / "source", *(folder for folder in (plugin / "source").iterdir() if folder.is_dir())]
    waiting = [panel / f"{name}.cpp" for name in re.findall(r"\$\{panel\}/(\w+)\.cpp", cmake)]
    waiting += [shared / f"{name}.cpp" for name in re.findall(r"\.\./shared/source/(\w+)\.cpp", cmake)]
    # the plugin's own files name headers there too
    waiting += [path for path in (plugin / "source").rglob("*") if path.is_file()]
    found: set[Path] = set()
    while waiting:
        path = waiting.pop()
        if path in found:
            continue
        found.add(path)
        for name in INCLUDE.findall(path.read_text(encoding="utf-8", errors="replace")):
            near = [folder / name for folder in (path.parent, *roots) if (folder / name).exists()]
            if near:
                waiting.append(near[0].resolve())
    return {path for path in found if not path.is_relative_to(plugin)}


def wanted() -> dict[str, Path]:
    files = {name: PROJECT / name for name in FILES}
    for folder in FOLDERS:
        for path in (PROJECT / folder).rglob("*"):
            if path.is_file() and not SKIPPED.intersection(path.parts):
                files[path.relative_to(PROJECT).as_posix()] = path
    for path in plugin_files_beyond_its_folder():
        files[path.relative_to(PROJECT.resolve()).as_posix()] = path
    for part in (*(part for part in borrowed.LIBRARY if isinstance(part, str)), PANEL_PART_FOR_TESTS):
        files[f"{PANEL_MOVIE}/source/{part}"] = PROJECT / PANEL_MOVIE / "source" / part
    # inside the repository itself these are already in their places, its release.py lays the same list
    added = HERE if HERE.is_dir() else None
    files.update({target: (added / name if added else PROJECT / target) for name, target in ADDED.items()})
    for path in ((added or PROJECT) / "licences").iterdir():
        files[f"licences/{path.name}"] = path
    # only the pictures the readmes show go along
    pictures = (added or PROJECT) / "pictures"
    for readme in ("README.md", "README.uk.md"):
        for name in re.findall(r"\(pictures/([^)]+)\)", files[readme].read_text(encoding="utf-8")):
            files[f"pictures/{name}"] = pictures / name
    return files


def uncommitted(files: dict[str, Path]) -> list[str]:
    told = subprocess.run(["git", "-C", str(PROJECT), "status", "--porcelain", "--", *(str(path) for path in files.values())], check=True, capture_output=True, text=True)
    return told.stdout.splitlines()


def private_lines(folder: Path, files: dict[str, Path]) -> list[str]:
    found = []
    private = re.compile("|".join(line for line in PRIVATE_WORDS.read_text(encoding="utf-8").split("\n") if line), re.IGNORECASE)
    for name in sorted(files):
        if private.search(name):
            found.append(f"{name}: in its name")
        path = folder / name
        if path.suffix.lower() in BINARY:
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").split("\n"), 1):
            if private.search(line):
                found.append(f"{name}:{number}: {line.strip()[:120]}")
    return found


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", nargs="?", type=Path, default=TARGET)
    parser.add_argument("--under-way", action="store_true", help="edits not committed go too, for a try in a scratch folder")
    arguments = parser.parse_args()
    target = arguments.folder
    files = wanted()
    if not arguments.under_way and (under_way := uncommitted(files)):
        raise SystemExit("commit these first, what goes out is what's committed:\n" + "\n".join(under_way))
    laying.stop_on_hand_edits(target, files)
    for name, source in files.items():
        laid(source, target / name)
    laying.remember(target, files)
    print(f"{len(files)} files in {target}")
    for path in sorted(target.rglob("*")):
        relative = path.relative_to(target)
        if path.is_file() and relative.as_posix() not in files and relative.parts[0] not in (".git", "out", "tools") and not {"build", "__pycache__"}.intersection(relative.parts):
            print(f"no longer belongs: {relative.as_posix()}")
    lines = private_lines(target, files)
    print(f"{len(lines)} lines with words that shouldn't go public")
    for line in lines:
        print(f"  {line}")


if __name__ == "__main__":
    main()
