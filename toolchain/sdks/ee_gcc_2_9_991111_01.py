"""Sony ee-gcc 2.9-ee-991111-01: the compiler the retail bootstrap was most
likely built with.  Its cc1 and cc1plus are i386 Linux programs; the
bootstrap's tools/linuxcc.py runs them under Unicorn, with the glibc 2.3.6
they were packaged with."""
import os

from builder.download import unpack_tar
from builder.model import Download, Sdk, Status

DECOMPME = "https://github.com/decompme/compilers/releases/download/compilers/"
CC1_DIR = "lib/gcc-lib/ee/2.9-ee-991111-01/"
FILES = {   # path under sdk/ee-gcc-2.9-991111-01: SHA-256
    CC1_DIR + "cc1": "B9AEF69F93EFB949F15EA58189E8EEF4A002B9FE4DE5D3FCF89C34E1244EC026",
    CC1_DIR + "cc1plus": "DA6EF339898E721FA6393F608F7C5DE975B9510841D4C2967F318D6108F62512",
    "rootfs/lib/ld-linux.so.2": "D5A16AEE4A04DB7FA27A5C071A0BEF62002EE679BF5ECCE51E383003CF840FD8",
    "rootfs/lib/libc.so.6": "7B091AEE7173E3A7CBDDC6630FA6B0D0106944B5618B0B27C2C058EA9C3D92EB",
}


class SonyEeGcc991111_01(Sdk):
    id = "ee-gcc-2.9-991111-01"
    name = "Sony ee-gcc 2.9-ee-991111-01"
    summary = "the PS2 SDK compiler of 2000 that the retail bootstrap was most likely built with"
    source = "decomp.me compiler archive"
    home = "ee-gcc-2.9-991111-01"
    requires = ["ee-gcc-2.9-991111b", "unicorn"]
    downloads = [
        Download(DECOMPME + "ee-gcc2.9-991111-01.tar.xz",
                 "ED684FD98F89D36B0121CAAB311052089103E3B36241FCEF4338CC9EA41C75B8", 2174148),
        Download(DECOMPME + "ee-gcc2.9-991111-01-dtls13010.tar.gz",
                 "3AB52A7EA962100F2A7DF3E04D3E7A758AAD669EB365251682E769B755D9D586", 5319436),
    ]

    def detect(self, settings):
        missing = [f for f in FILES if not os.path.isfile(self.path(f))]
        if missing:
            return Status(False, problems=["not installed" if len(missing) == len(FILES) else
                                           "incomplete: %s missing" % ", ".join(missing)])
        return Status(True, "2.9-ee-991111-01", self.path())

    def unpack(self, files, staging, ctx):
        unpack_tar(files[0], staging, ctx, members={CC1_DIR + n: CC1_DIR + n for n in ("cc1", "cc1plus")})
        unpack_tar(files[1], staging, ctx, members={"lib/ld-2.3.6.so": "rootfs/lib/ld-linux.so.2",
                                                    "lib/libc-2.3.6.so": "rootfs/lib/libc.so.6"})
        for f, sha in FILES.items():
            ctx.verify(os.path.join(staging, f), sha)


SDK = SonyEeGcc991111_01()
