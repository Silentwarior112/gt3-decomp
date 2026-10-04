"""SCE ee-gcc 2.9-ee-991111b, Sony's Windows package of the 2000 SDK compiler.
The period-accurate bootstrap build takes its driver, preprocessor, assembler
and libgcc.a; its C headers are gcc-2.95.2's own, from GNU."""
import os

from builder import SDK_DIR
from builder.download import unpack_tar
from builder.model import Download, Sdk, Status

DECOMPME = "https://github.com/decompme/compilers/releases/download/compilers/"
LIB = "lib/gcc-lib/ee/2.9-ee-991111/"
FILES = ["bin/ee-gcc.exe", LIB + "cpp.exe", LIB + "cc1.exe", LIB + "cc1plus.exe", LIB + "libgcc.a",
         "ee/bin/as.exe", "include/stdarg.h", "include/stddef.h", "include/va-mips.h"]
HEADERS = ("stdarg.h", "stddef.h", "va-mips.h", "varargs.h")


class SceEeGcc991111b(Sdk):
    id = "ee-gcc-2.9-991111b"
    name = "SCE ee-gcc 2.9-ee-991111b"
    summary = "Sony's Windows package of the same compiler: preprocessor, assembler and libgcc"
    source = "decomp.me compiler archive; headers from gcc-2.95.2 (ftp.gnu.org)"
    home = "ee-gcc-2.9-991111b"
    downloads = [
        Download(DECOMPME + "ee-gcc2.9-991111.tar.gz",
                 "9EBBB042EB1E5D33D772293C5E9286A54DBEC29865A3D7FBB398C97436F33583", 3494403),
        Download("https://ftp.gnu.org/gnu/gcc/gcc-2.95.2.tar.gz",
                 "064E1CB06EA5D2F4A07EC46C1C64D771F74D04F404B6A6766BCA2477F7D72482", 12885815),
    ]

    def can_install(self):
        return os.name == "nt"          # Windows programs

    def detect(self, settings):
        # the matching tools' patched copy (tools/patch_divs.py) does as well
        for home in (self.home, self.home + "-divs"):
            if all(os.path.isfile(os.path.join(SDK_DIR, home, f)) for f in FILES):
                return Status(True, "2.9-ee-991111b", os.path.join(SDK_DIR, home))
        missing = [f for f in FILES if not os.path.isfile(self.path(f))]
        return Status(False, problems=["not installed" if len(missing) == len(FILES) else
                                       "incomplete: %s missing" % ", ".join(missing)])

    def unpack(self, files, staging, ctx):
        unpack_tar(files[0], staging, ctx)
        unpack_tar(files[1], staging, ctx, members={"gcc-2.95.2/gcc/ginclude/" + h: "include/" + h for h in HEADERS})
        ctx.verify(os.path.join(staging, LIB, "cc1.exe"),
                   "124F7C6870B7FACBAECBC51F0B87C0AAD46DB5F6ED491C21638E1DA991E45BE5")
        ctx.verify(os.path.join(staging, LIB, "cc1plus.exe"),
                   "65B5134F24377544BB2E5D1D2DD3436807CC50CE2CE77D759E4FCA1ADD18F9A4")


SDK = SceEeGcc991111b()
