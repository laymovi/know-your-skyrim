"""The helpers Know your Skyrim takes from the panel of Inventory Reforged, by name

    python -m interface.know_your_skyrim.borrowed      prints what is taken

The browser draws and words things the way the panel does: the same signs, boxes, chips and wrapped lines, the same
spot in the bottom bar. The panel keeps those helpers in its parts as "Equipment.<name> = ..." at the start of a line
and every name the browser's parts read or call and never set themselves is taken from the panel's parts listed in
LIBRARY, with everything it uses in turn, and goes into the browser's movie as KnowYourSkyrim.<name>. So both mods
keep one source of them, a fix in the panel reaches the browser. A name neither of them defines stops the build, and
so does a taken helper that touches the panel's own global. A name the browser sets itself is never taken
"""

import re
from pathlib import Path

from interface.equipment import icons

PANEL = Path(__file__).parents[1] / "equipment" / "source"
# the parts of the panel with what both mods share, in the order the panel's movie runs them
LIBRARY = ["10-core.as", "24-subcategories.as", "26-draw.as", icons, "30-panel.as", "35-models.as", "37-inspect.as", "38-stats.as", "38-hero.as",
           "39-browser-room.as"]
PANEL_NAME = "Equipment"
NAME = "KnowYourSkyrim"

STARTS = re.compile(rf"^{PANEL_NAME}\.(\w+)")


def statements(text: str) -> list[tuple[str, str]]:
    """The statements at the top of a part that set a name of the panel, as (name, text). A statement runs from
    its first line to the ; that closes it outside brackets, strings and comments"""
    found = []
    lines = text.splitlines()
    at = 0
    while at < len(lines):
        start = STARTS.match(lines[at])
        if not start:
            at += 1
            continue
        depth = 0
        end = at
        while end < len(lines):
            depth = depth_after(lines[end], depth)
            if depth == 0 and lines[end].rstrip().endswith(";"):
                break
            end += 1
        found.append((start.group(1), "\n".join(lines[at:end + 1])))
        at = end + 1
    return found


def depth_after(line: str, depth: int) -> int:
    quote = None
    i = 0
    while i < len(line):
        letter = line[i]
        if quote:
            if letter == "\\":
                i += 1
            elif letter == quote:
                quote = None
        elif letter in "\"'":
            quote = letter
        elif line.startswith("//", i):
            break
        elif letter in "{[(":
            depth += 1
        elif letter in "}])":
            depth -= 1
        i += 1
    return depth


def library() -> dict[str, list[tuple[int, str]]]:
    """Every name the library sets with the statements that set it, each with its place in the library"""
    found: dict[str, list[tuple[int, str]]] = {}
    places: dict[str, set[str]] = {}
    order = 0
    for part in LIBRARY:
        text = part.script() if part is icons else (PANEL / part).read_text(encoding="utf-8")
        label = "icons" if part is icons else part
        for name, statement in statements(text):
            found.setdefault(name, []).append((order, statement))
            places.setdefault(name, set()).add(label)
            order += 1
    # a stub one part sets and another fills later can't be told apart, such a name is never taken
    for name, parts in places.items():
        if len(parts) > 1:
            found[name] = []
    return found


def uncommented(text: str) -> str:
    """The code without its // comments, a file name in a comment is no name of the movie"""
    kept = []
    for line in text.splitlines():
        quote = None
        i = 0
        while i < len(line):
            letter = line[i]
            if quote:
                if letter == "\\":
                    i += 1
                elif letter == quote:
                    quote = None
            elif letter in "\"'":
                quote = letter
            elif line.startswith("//", i):
                line = line[:i]
                break
            i += 1
        kept.append(line)
    return "\n".join(kept)


def names_used(text: str, name: str) -> set[str]:
    return set(re.findall(rf"\b{name}\.(\w+)", uncommented(text)))


def names_set(text: str, name: str) -> set[str]:
    return set(re.findall(rf"\b{name}\.(\w+)\s*=(?!=)", uncommented(text)))


def taken(own_parts: list[str]) -> tuple[list[str], str]:
    """The names taken for the browser's parts and the script that sets them under the browser's name"""
    own = "\n".join(own_parts)
    have = names_set(own, NAME)
    shared = library()
    wanted = sorted(names_used(own, NAME) - have)
    chosen: dict[str, list[tuple[int, str]]] = {}
    missing = []
    while wanted:
        name = wanted.pop()
        if name in chosen or name in have:
            continue
        if not shared.get(name):
            missing.append(name)
            continue
        chosen[name] = shared[name]
        for statement in (text for _, text in shared[name]):
            wanted.extend(sorted(names_used(statement, PANEL_NAME) - have - chosen.keys()))
    if missing:
        raise ValueError(f"the browser uses names neither it nor the panel's library sets: {sorted(set(missing))}")
    ordered = sorted((place, text) for statements_of in chosen.values() for place, text in statements_of)
    script = "\n".join(re.sub(rf"\b{PANEL_NAME}\.", f"{NAME}.", text) for _, text in ordered) + "\n"
    leaking = re.findall(rf"\b{PANEL_NAME}\b", script)
    if leaking:
        raise ValueError(f"a helper taken from the panel touches the panel's own global: {len(leaking)} times")
    return sorted(chosen), script


if __name__ == "__main__":
    from interface.know_your_skyrim import build
    names, _ = taken(build.own_scripts())
    print(f"{len(names)} names taken from the panel: {', '.join(names)}")
