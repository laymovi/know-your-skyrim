import os
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[2]
TOOLS = Path(os.environ.get("KYSK_TOOLS", PROJECT / "tools"))
BUILT = PROJECT / "out"
OUT = BUILT
