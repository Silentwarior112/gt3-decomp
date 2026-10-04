"""Unicorn, the CPU emulator the bootstrap's tools/linuxcc.py runs Sony's
Linux compiler with: a Python package, installed with pip into the Python
that runs the builder."""
import importlib.metadata
import importlib.util

from builder.download import pip_install
from builder.model import Sdk, Status


class Unicorn(Sdk):
    id = "unicorn"
    name = "Unicorn (Python package)"
    summary = "the CPU emulator that runs Sony's Linux compiler on Windows"
    source = "PyPI, with pip"

    def can_install(self):
        return True

    def download_size(self):
        return 16 << 20

    def detect(self, settings):
        if importlib.util.find_spec("unicorn") is None:
            return Status(False, problems=["not installed"])
        try:
            version = importlib.metadata.version("unicorn")
        except importlib.metadata.PackageNotFoundError:
            version = ""
        return Status(True, version, "Python package")

    def label(self, status=None):
        return "Unicorn %s" % status.version if status and status.version else self.name

    def install(self, ctx):
        pip_install("unicorn", ctx)


SDK = Unicorn()
