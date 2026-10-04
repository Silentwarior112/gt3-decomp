"""Template for a new executable.  Copy it to targets/<name>.py (files that
start with "_" are not loaded), fill it in and restart the builder: it appears
under "What to build", with its own compiler options.  README.md, "Adding an
executable or a compiler", explains the parts."""
import os

from builder import REPO_DIR
from builder.model import GameFile, Option, Target

PROJECT = os.path.join(REPO_DIR, "main")


class MainExecutable(Target):
    id = "main"                         # saved in profiles: do not change it later
    name = "GT3 main executable (CORE.ELF)"
    short_name = "Main"
    description = "One line shown under the name."
    project = PROJECT
    output_name = "CORE.ELF"
    options = [                         # the "Compiler" choices; the first is the default
        Option("fast", "Fast", "Modern GCC: for modding.", "ps2sdk"),
        Option("period", "Period-accurate", "The original's compiler.", "ee-gcc-2.96", also=["ps2sdk"]),
    ]
    game_files = [GameFile("CORE.GT3", "the main executable of Gran Turismo 3 A-Spec (US)", "CORE.GT3",
                           "<SHA-256 of the retail file>", os.path.join(PROJECT, "baserom", "CORE.GT3"))]
    originals = {}                      # {SHA-256: "the retail ..."}: backed up before replacing

    def command(self, option_id, ctx):
        # ctx.python, ctx.jobs, ctx.status[sdk id].location
        argv = [ctx.python, "-u", os.path.join(PROJECT, "tools", "build.py"),
                "--compiler", option_id,
                "--toolchain", ctx.status["ps2sdk"].location,
                "--build-dir", os.path.join(PROJECT, "build", "builder-" + option_id),
                "--jobs", str(ctx.jobs), "--progress"]
        return argv, PROJECT

    def artifact(self, option_id):
        return os.path.join(PROJECT, "build", "builder-" + option_id, "core.elf")


TARGET = MainExecutable()
