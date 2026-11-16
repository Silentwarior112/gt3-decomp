"""GT3 enhanced engine, CORE.GT3 (core_gt3enhanced/): the GT3 US decompilation
with the game-logic updates of Lupo 2 (GT Concept) added (ref/ENHANCED.md).
Wraps core_gt3enhanced/tools/build.py; builds in
core_gt3enhanced/build/builder-<option>/.

Only the all-C link: the enhanced code no longer matches the original's object
layouts, so it cannot be linked over the original code (the period-accurate
hybrid) or stand in for it (the fast build's original code)."""
import os

from builder import REPO_DIR
from builder.model import GameFile, Option, Target

PROJECT = os.path.join(REPO_DIR, "core_gt3enhanced")
BUILD_PY = os.path.join(PROJECT, "tools", "build.py")
RETAIL_SHA256 = "FDCEA2AA43231873592ADEE0FB96499F2E40979E0269191AF1EAFCF95DA12B0B"


class CoreEnhanced(Target):
    id = "core_gt3enhanced"
    name = "GT3 enhanced engine (CORE.GT3)"
    short_name = "Core enhanced"
    description = ("GT3's main executable from the decompiled C, with the game-logic updates of "
                   "Lupo 2 (GT Concept): gearbox types, the Toyota Pod and more.")
    project = PROJECT
    output_name = "CORE.GT3"
    output_magic = b"\x01\x01"          # loadFlags 0x0101: a disc / memory card copy
    options = [
        Option("allc", "All C",
               "The original's compiler with the matching flags, the enhanced C linked alone. About 3 minutes.",
               "ee-gcc-2.9-991111b", also=["ps2sdk", "splat"]),
    ]
    # the alignment padding and the frame info's reference bytes come from it
    game_files = [GameFile("CORE.GT3", "the main executable of Gran Turismo 3 A-Spec (US, SCUS-97102)",
                           "CORE.GT3", RETAIL_SHA256, os.path.join(PROJECT, "baserom", "CORE.GT3"))]
    originals = {RETAIL_SHA256: "the retail US CORE.GT3"}

    def problems(self):
        return [] if os.path.isfile(BUILD_PY) else ["missing %s" % BUILD_PY]

    def build_dir(self, option_id):
        return os.path.join(PROJECT, "build", "builder-" + option_id)

    def command(self, option_id, ctx):
        argv = [ctx.python, "-u", BUILD_PY,
                "--compiler", "991111b", "--link", "full",
                "--toolchain", ctx.status["ps2sdk"].location,
                "--build-dir", self.build_dir(option_id),
                "--jobs", str(ctx.jobs), "--progress"]
        return argv, PROJECT

    def artifact(self, option_id):
        return os.path.join(self.build_dir(option_id), "CORE.GT3")


TARGET = CoreEnhanced()
