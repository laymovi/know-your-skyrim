"""A person edits the readme of a laid repository right there, laying it again must never take that away

What each file was when last laid is kept in the repository's out/laid.json. A file that's neither that nor the new text
was changed by hand and stops the laying until the change reaches the source
"""

import hashlib
import json
from pathlib import Path

MEMORY = "out/laid.json"


def fingerprint(data: bytes) -> str:
    return hashlib.md5(data.replace(b"\r\n", b"\n")).hexdigest()


def edited_by_hand(folder: Path, files: dict[str, Path]) -> list[str]:
    memory = folder / MEMORY
    before = json.loads(memory.read_text(encoding="utf-8")) if memory.exists() else {}
    edited = []
    for name, source in sorted(files.items()):
        path = folder / name
        if path.exists() and fingerprint(path.read_bytes()) not in (before.get(name), fingerprint(source.read_bytes())):
            edited.append(name)
    return edited


def stop_on_hand_edits(folder: Path, files: dict[str, Path]) -> None:
    if edited := edited_by_hand(folder, files):
        raise SystemExit(f"nothing was laid, edited by hand in {folder}, carry it over to the source first:\n" + "\n".join(edited))


def remember(folder: Path, files: dict[str, Path]) -> None:
    memory = folder / MEMORY
    memory.parent.mkdir(parents=True, exist_ok=True)
    memory.write_text(json.dumps({name: fingerprint((folder / name).read_bytes()) for name in files}, indent=0), encoding="utf-8")
