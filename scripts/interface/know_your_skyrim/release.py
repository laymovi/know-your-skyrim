"""Lay Know your Skyrim out the way a player gets it, a folder with an installer for Mod Organizer 2 and Vortex, and
its archive

    python -m interface.know_your_skyrim.release [--no-archive]

--no-archive lays out the folder only, for a local build between releases

Takes the package python -m interface.know_your_skyrim.build writes and lays out out/know-your-skyrim/release/installer
  core       the SKSE plugin, the browser's movie and its words, the light plugin and scripts of its menu in SkyUI,
             installed always
  fomod      the installer, one page with nothing to choose (an installer with no page took MO2 down), and its picture
  source     what the plugin and the movie are built from, the same files as the public repository without the readme's
             pictures. The plugin links CommonLibSSE-NG, which is under the GPL 3, so its source goes with every copy
  readme.txt, compatibilities.md
The archive goes to out/know-your-skyrim/KnowYourSkyrim-<version>.zip
"""

import argparse
import re
import shutil
from pathlib import Path

from PIL import Image

from common.paths import PROJECT
from interface.know_your_skyrim import public_repo
from interface.know_your_skyrim.build import PACKAGE, WORK
from common import fomod

NAME = "Know your Skyrim"
FILES = "KnowYourSkyrim"
RELEASE = WORK / "release" / "installer"
PLUGIN_FOLDER = PROJECT / "plugins" / "know-your-skyrim"
ARCHIVE_TEXTS = PROJECT / "scripts" / "interface" / "know_your_skyrim" / "archive"
VERSION_LINE = re.compile(rf"project\({FILES} VERSION ([\d.]+)")
PICTURE_IN_INSTALLER = "fomod/main.jpg"
PICTURE_WIDTH = 960
MAIN_PICTURE = "pictures/01-browser.jpg"


def version() -> str:
    return VERSION_LINE.search((PLUGIN_FOLDER / "CMakeLists.txt").read_text(encoding="utf-8")).group(1)


def installer() -> fomod.Installer:
    everything = fomod.Group("What goes in", (
        fomod.Option(
            NAME,
            "The browser in SkyUI's inventory, its SKSE plugin and the menu in Mod Configuration, a light plugin that "
            "takes no place in your load order. Nothing to choose, press Install.",
            required=True,
            image=PICTURE_IN_INSTALLER,
        ),
    ), kind=fomod.ALL_OF)
    return fomod.Installer(
        name=NAME,
        author="laymovi",
        version=version(),
        website="",
        description="Every item, person, quest and place of your load order in SkyUI's inventory, with where to get each and the mod it comes from.",
        folders=(fomod.Folder("core"),),
        steps=(fomod.Step("Install", (everything,)),),
        image=PICTURE_IN_INSTALLER,
    )


def lay_out() -> None:
    shutil.rmtree(RELEASE, ignore_errors=True)
    shutil.copytree(PACKAGE, RELEASE / "core")
    readme = (ARCHIVE_TEXTS / "readme.txt").read_text(encoding="utf-8")
    if version() not in readme.splitlines()[0]:
        raise SystemExit(f"the first line of the archive's readme doesn't name {version()}")
    (RELEASE / "readme.txt").write_text(readme, encoding="utf-8")
    wanted = public_repo.wanted()
    public_repo.laid(wanted["compatibilities.md"], RELEASE / "compatibilities.md")
    for name, source in wanted.items():
        if not name.startswith("pictures/"):
            public_repo.laid(source, RELEASE / "source" / name)
    shown = RELEASE / PICTURE_IN_INSTALLER
    shown.parent.mkdir(parents=True, exist_ok=True)
    with Image.open(wanted[MAIN_PICTURE]) as picture:
        height = round(picture.height * PICTURE_WIDTH / picture.width)
        picture.convert("RGB").resize((PICTURE_WIDTH, height), Image.LANCZOS).save(shown, quality=88)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--no-archive", action="store_true", help="the folder only")
    arguments = parser.parse_args()
    lay_out()
    told = installer()
    fomod.write(RELEASE, told)
    if missing := fomod.check(RELEASE):
        raise SystemExit(f"the installer names what the release does not have: {missing}")
    print(f"{sum(1 for file in RELEASE.rglob('*') if file.is_file())} files in {RELEASE}")
    if arguments.no_archive:
        return
    archive = shutil.make_archive(str(WORK / f"{FILES}-{told.version}"), "zip", RELEASE)
    print(f"{archive}: {Path(archive).stat().st_size} bytes")


if __name__ == "__main__":
    main()
