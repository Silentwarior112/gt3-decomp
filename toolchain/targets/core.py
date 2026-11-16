"""GT3 US main executable, CORE.GT3 (core/).  Wraps core/tools/build.py; each
option builds in core/build/builder-<option>/.

The fast and period-accurate options compile the decompiled C and relink the
split of your CORE.GT3 byte for byte. The period-accurate build then links
the compiled C over it (core/hybrid.txt says which units): the original data
and the code left out stay at their addresses. The fast build's GCC 14 C++
does not mix with the original's yet, so its CORE.GT3 is the original code.
The all-C option links the program from the C alone (core/tools/fulllink.py):
no original code, and the C's data at the original addresses; only alignment
padding and the few bytes of code the C does not match yet come from your
CORE.GT3."""
import os

from builder import REPO_DIR
from builder.model import GameFile, Option, Target

PROJECT = os.path.join(REPO_DIR, "core")
BUILD_PY = os.path.join(PROJECT, "tools", "build.py")
BUILD_COMPILER = {"fast": "gcc14", "period": "991111b", "allc": "991111b"}     # build.py --compiler
BUILD_LINK = {"allc": "full"}                                                   # build.py --link
RETAIL_SHA256 = "FDCEA2AA43231873592ADEE0FB96499F2E40979E0269191AF1EAFCF95DA12B0B"


class Core(Target):
    id = "core"
    name = "GT3 US main executable (CORE.GT3)"
    short_name = "Core"
    description = ("The game itself, which the bootstrap loads. Period-accurate: the decompiled C linked "
                   "over the original data (core/hybrid.txt). Fast: the C is checked, CORE.GT3 is the original code.")
    project = PROJECT
    output_name = "CORE.GT3"
    output_magic = b"\x01\x01"          # loadFlags 0x0101: a disc / memory card copy
    options = [
        Option("fast", "Fast", "Modern GCC checks the C (about 25 seconds); CORE.GT3 is the original code for now.",
               "ps2sdk", also=["splat"]),
        Option("period", "Period-accurate",
               "The original's compiler, SCE ee-gcc 2.9-ee-991111b: CORE.GT3 runs the decompiled C. About 45 seconds.",
               "ee-gcc-2.9-991111b", also=["ps2sdk", "splat"]),    # assembler and ld: the EE toolchain
        Option("allc", "All C",
               "The original's compiler with the matching flags, and only the decompiled C linked: no original "
               "code, every variable the C's at its original address. About 3 minutes.",
               "ee-gcc-2.9-991111b", also=["ps2sdk", "splat"]),
    ]
    # the split (and so the link) comes from it; the C needs no game file
    game_files = [GameFile("CORE.GT3", "the main executable of Gran Turismo 3 A-Spec (US, SCUS-97102)",
                           "CORE.GT3", RETAIL_SHA256, os.path.join(PROJECT, "baserom", "CORE.GT3"))]
    originals = {RETAIL_SHA256: "the retail US CORE.GT3"}

    def problems(self):
        return [] if os.path.isfile(BUILD_PY) else ["missing %s" % BUILD_PY]

    def build_dir(self, option_id):
        return os.path.join(PROJECT, "build", "builder-" + option_id)

    def command(self, option_id, ctx):
        argv = [ctx.python, "-u", BUILD_PY,
                "--compiler", BUILD_COMPILER[option_id],
                "--toolchain", ctx.status["ps2sdk"].location,
                "--build-dir", self.build_dir(option_id),
                "--jobs", str(ctx.jobs), "--progress"]
        if option_id in BUILD_LINK:
            argv += ["--link", BUILD_LINK[option_id]]
        return argv, PROJECT

    def artifact(self, option_id):
        return os.path.join(self.build_dir(option_id), "CORE.GT3")


TARGET = Core()
