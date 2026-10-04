"""Template for a new SDK (a compiler, a toolchain, a package).  Copy it to
sdks/<name>.py (files that start with "_" are not loaded), fill it in, and
name its id in a target's Option.  README.md, "Adding an executable or a
compiler", explains the parts."""
import os

from builder.download import unpack_tar
from builder.model import Download, Sdk, Status

FILES = ["bin/ee-gcc", "lib/gcc-lib/ee/2.96-ee-001003-1/cc1"]    # what detect() checks


class SonyEeGcc296(Sdk):
    id = "ee-gcc-2.96"                  # what targets name; do not change it later
    name = "Sony ee-gcc 2.96-ee-001003-1"
    summary = "the compiler the main executable was built with"
    source = "decomp.me compiler archive"
    home = "ee-gcc-2.96"                # installed as sdk/ee-gcc-2.96
    requires = []                       # other SDK ids it cannot work without
    downloads = [Download("https://github.com/decompme/compilers/releases/download/compilers/ee-gcc2.96.tar.xz",
                          "<SHA-256 of the archive>", 2754768)]

    def detect(self, settings):
        missing = [f for f in FILES if not os.path.isfile(self.path(f))]
        return Status(not missing, "2.96-ee-001003-1", self.path(),
                      ["not installed"] if missing else [])

    def unpack(self, files, staging, ctx):
        unpack_tar(files[0], staging, ctx)      # every file; or members={...} for some


SDK = SonyEeGcc296()
