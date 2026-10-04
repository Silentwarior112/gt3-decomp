"""The plug-in classes.  A file in toolchain/targets/ defines TARGET = a Target
subclass instance; a file in toolchain/sdks/ defines SDK = an Sdk subclass
instance.  The _template.py file in each folder is a working example."""
import hashlib
import os

from . import OUTPUT_DIR, SDK_DIR


class Status:
    """What an SDK's detect() found.

    ready     True when a build can use it
    version   e.g. "14.2.0"
    location  the folder it was found in
    problems  what is missing, one short sentence each
    """

    def __init__(self, ready, version="", location="", problems=()):
        self.ready = ready
        self.version = version
        self.location = location
        self.problems = list(problems)

    def __repr__(self):
        return "Status(ready=%r, version=%r, location=%r, problems=%r)" % (
            self.ready, self.version, self.location, self.problems)


class Download:
    """One file an SDK downloads: its URL, its SHA-256 (checked when given)
    and its size in bytes (for the progress bar when the server sends none)."""

    def __init__(self, url, sha256=None, size=0):
        self.url = url
        self.sha256 = sha256
        self.size = size


class Sdk:
    """Something a build needs that the builder can find, and usually
    download: a compiler, a toolchain, a Python package.

    id         stable name; targets list it, settings store it
    name       shown in the window
    summary    one line: what it is
    source     where it comes from, shown before downloading
    home       its folder under sdk/ (None: it lives elsewhere, e.g. pip)
    downloads  [Download, ...] that install() fetches
    requires   ids of other SDKs it cannot work without
    setting    settings key of a folder the user may choose instead (or None)
    """

    id = ""
    name = ""
    summary = ""
    source = ""
    home = None
    downloads = []
    requires = []
    setting = None
    setting_label = ""

    def detect(self, settings):
        """Look for it, changing nothing.  Returns a Status."""
        raise NotImplementedError

    def label(self, status=None):
        """The name with the detected version."""
        return self.name

    def download_size(self):
        return sum(d.size for d in self.downloads)

    def can_install(self):
        """Whether install() works on this machine."""
        return bool(self.downloads) and self.home is not None

    def install(self, ctx):
        """Download and unpack: fetch every download, unpack them into an empty
        staging folder, then move that into place as sdk/<home>."""
        files = [ctx.fetch(d) for d in self.downloads]
        staging = ctx.staging(self.home)
        self.unpack(files, staging, ctx)
        ctx.commit(staging, self.home)

    def unpack(self, files, staging, ctx):
        """Put the downloaded files' contents into staging (builder.download
        has the helpers)."""
        raise NotImplementedError

    def path(self, *parts):
        return os.path.join(SDK_DIR, self.home, *parts)


class Option:
    """One way to build a target, offered under "Compiler".

    id       saved in profiles ("fast", "period", ...)
    label    "Fast", "Period-accurate", ...
    summary  one line under it
    sdk      the SDK whose name and version the window shows
    also     other SDKs this build uses (the SDKs' own requirements are added)
    """

    def __init__(self, id, label, summary, sdk, also=()):
        self.id = id
        self.label = label
        self.summary = summary
        self.sdk = sdk
        self.also = list(also)


class GameFile:
    """A file from the user's own copy of the game that a build needs; the
    repository never includes one.  The user picks the file, or the disc
    image it is on.

    name       e.g. "SCUS_971.02"
    disc       what it is, e.g. "the boot file of Gran Turismo 3 A-Spec (US, SCUS-97102)"
    disc_path  its path on the disc, for taking it out of a disc image
    sha256     the exact file (upper case)
    dest       where the builder keeps it
    """

    def __init__(self, name, disc, disc_path, sha256, dest):
        self.name = name
        self.disc = disc
        self.disc_path = disc_path
        self.sha256 = sha256.upper()
        self.dest = dest

    def present(self):
        if not os.path.isfile(self.dest):
            return False
        with open(self.dest, "rb") as f:
            return hashlib.sha256(f.read()).hexdigest().upper() == self.sha256


class BuildContext:
    """What Target.command() gets: python (the interpreter to run scripts
    with), jobs (files to compile at once) and status ({SDK id: Status})."""

    def __init__(self, python, jobs, status):
        self.python = python
        self.jobs = jobs
        self.status = status


class Target:
    """One executable a decomp project builds.

    id            saved in profiles: do not change it later
    name          shown under "What to build"
    short_name    used in the default profile names
    description   one line under the name
    project       the project folder; its build runs there
    output_name   the executable's own file name
    options       [Option, ...]: the compiler choices, the first is the default
    game_files    [GameFile, ...] the build needs
    originals     {SHA-256: description} of original files: an output path
                  holding one is backed up before the first build replaces it
    output_magic  how the executables start: a file that does not is never
                  replaced (an ISO picked by mistake, say)
    """

    id = ""
    name = ""
    short_name = ""
    description = ""
    project = ""
    output_name = ""
    options = []
    game_files = []
    originals = {}
    output_magic = b"\x7fELF"

    def option(self, option_id):
        for o in self.options:
            if o.id == option_id:
                return o
        return None

    def problems(self):
        """What the project itself is missing (not SDKs or game files)."""
        return []

    def command(self, option_id, ctx):
        """(argv, cwd) of the build.  It must exit with 0 only when
        artifact(option_id) has just been written."""
        raise NotImplementedError

    def artifact(self, option_id):
        """Where the build leaves the finished executable."""
        raise NotImplementedError

    def default_output(self, option_id):
        """The output path a new profile starts with: relative, so that it
        follows the repository folder if that moves."""
        return os.path.join(os.path.basename(OUTPUT_DIR), "%s-%s" % (self.id, option_id), self.output_name)
