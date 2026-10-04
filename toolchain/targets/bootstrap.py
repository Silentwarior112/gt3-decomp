"""GT3 US bootstrap, SCUS_971.02 (bootstrap/).  Wraps the project's own build,
bootstrap/tools/build.py; each option builds in bootstrap/build/builder-<option>/."""
import os

from builder import REPO_DIR
from builder.model import GameFile, Option, Target

PROJECT = os.path.join(REPO_DIR, "bootstrap")
BUILD_PY = os.path.join(PROJECT, "tools", "build.py")
RETAIL_SHA256 = "E60A8821D01051CECE0EDEDCD600BCA0A79FB460BAA2A5BABEDD72B3D18E5518"
BUILD_COMPILER = {"fast": "gcc14", "period": "991111-01"}       # build.py --compiler


class Bootstrap(Target):
    id = "bootstrap"
    name = "GT3 US bootstrap (SCUS_971.02)"
    short_name = "Bootstrap"
    description = "The disc's boot ELF: it loads, checks and starts core.gt3."
    project = PROJECT
    output_name = "SCUS_971.02"
    options = [
        Option("fast", "Fast", "Modern GCC: builds in about 2 seconds. For modding.", "ps2sdk"),
        Option("period", "Period-accurate",
               "The compiler the original was built with: close to the retail code, about 25 seconds.",
               "ee-gcc-2.9-991111-01", also=["ps2sdk"]),          # it links with the EE toolchain's ld
    ]
    # the retail file: the three data blobs of the build come from it
    game_files = [GameFile("SCUS_971.02", "the boot file of Gran Turismo 3 A-Spec (US, SCUS-97102)",
                           "SCUS_971.02", RETAIL_SHA256, os.path.join(PROJECT, "baserom", "SCUS_971.02"))]
    originals = {RETAIL_SHA256: "the retail US SCUS_971.02"}

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
        return argv, PROJECT

    def artifact(self, option_id):
        return os.path.join(self.build_dir(option_id), "scus_971.02")


TARGET = Bootstrap()
