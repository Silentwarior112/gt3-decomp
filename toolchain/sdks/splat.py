"""splat and spimdisasm, the disassembler that splits the main executable into
assembly the build relinks (core/tools/gen_splat.py, core/tools/relink.py):
Python packages, installed with pip into the Python that runs the builder.
Pinned to the versions whose output relinks byte-identically."""
import importlib.metadata
import importlib.util

from builder.download import pip_install
from builder.model import Sdk, Status

PACKAGES = [("splat", "splat64", "0.50.0"), ("spimdisasm", "spimdisasm", "1.42.4"),
            ("rabbitizer", "rabbitizer", "1.16.2")]            # (module, distribution, version)


class Splat(Sdk):
    id = "splat"
    name = "splat and spimdisasm (Python packages)"
    summary = "the disassembler that splits the main executable into assembly for the relink"
    source = "PyPI, with pip"

    def can_install(self):
        return True

    def download_size(self):
        return 8 << 20

    def detect(self, settings):
        problems, versions = [], []
        for module, dist, want in PACKAGES:
            if importlib.util.find_spec(module) is None:
                problems.append("%s is not installed" % dist)
                continue
            try:
                have = importlib.metadata.version(dist)
            except importlib.metadata.PackageNotFoundError:
                have = ""
            if have != want:
                problems.append("%s %s is installed; the build needs %s" % (dist, have or "?", want))
            versions.append("%s %s" % (dist, have))
        return Status(not problems, ", ".join(versions), "Python packages", problems)

    def label(self, status=None):
        return "splat %s" % PACKAGES[0][2]

    def install(self, ctx):
        for _, dist, version in PACKAGES:
            pip_install("%s==%s" % (dist, version), ctx)


SDK = Splat()
