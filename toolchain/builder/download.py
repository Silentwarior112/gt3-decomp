"""Getting what a build needs: downloading and unpacking SDKs, installing a
Python package with pip, and taking a game file out of a disc image.

Everything an SDK installs goes to sdk/<home>, through sdk/<home>.partial: a
download that fails or is cancelled leaves nothing half-installed.
"""
import hashlib
import os
import shutil
import struct
import subprocess
import tarfile
import threading
import urllib.request
import zipfile

from . import NO_WINDOW, SDK_DIR, python_exe

USER_AGENT = "GT3DecompBuilder/1 (+python urllib)"


class Cancelled(Exception):
    pass


class SetupError(Exception):
    pass


def _long(path):
    """path in Windows' extended-length form: the toolchain has files deep
    enough to pass the 260-character limit in a deep checkout."""
    if os.name == "nt":
        path = os.path.abspath(path)
        if not path.startswith("\\\\"):
            return "\\\\?\\" + path
    return path


def _inside(root, rel):
    """root/rel, refusing a path that would leave root."""
    path = os.path.normpath(os.path.join(root, rel))
    if os.path.commonpath([os.path.abspath(root), os.path.abspath(path)]) != os.path.abspath(root):
        raise SetupError("an archive member points outside its folder: %s" % rel)
    return path


def _write(src, dest):
    os.makedirs(_long(os.path.dirname(dest)), exist_ok=True)
    with open(_long(dest), "wb") as f:
        shutil.copyfileobj(src, f, 1 << 20)


def _rmtree(path):
    if os.path.exists(_long(path)):
        shutil.rmtree(_long(path))


def unpack_tar(archive, staging, ctx, members=None, prefix=""):
    """Unpack a .tar.gz / .tar.xz.  members: {member name: path under
    staging} to take only those (a missing one is an error); otherwise every
    regular file under `prefix`, written without it.  Leading "./" in member
    names is ignored."""
    want = {k.lstrip("./"): v for k, v in members.items()} if members else None
    done = set()
    with tarfile.open(archive) as t:
        for m in t:
            ctx.check()
            name = m.name
            while name.startswith("./"):
                name = name[2:]
            if want is not None:
                if name not in want:
                    continue
                if not m.isfile():
                    raise SetupError("%s: %s is not a plain file" % (os.path.basename(archive), name))
                _write(t.extractfile(m), _inside(staging, want[name]))
                done.add(name)
            elif m.isfile() and name.startswith(prefix):
                _write(t.extractfile(m), _inside(staging, name[len(prefix):]))
    if want is not None and done != set(want):
        raise SetupError("%s lacks %s" % (os.path.basename(archive), ", ".join(sorted(set(want) - done))))


def unpack_zip(archive, staging, ctx, prefix):
    """Unpack every file of a .zip under `prefix` (written without it)."""
    with zipfile.ZipFile(archive) as z:
        names = [n for n in z.namelist() if n.startswith(prefix) and not n.endswith("/")]
        if not names:
            raise SetupError("%s has nothing under %s" % (os.path.basename(archive), prefix))
        for i, n in enumerate(names):
            ctx.check()
            if i % 50 == 0:
                ctx.progress("Unpacking %s" % os.path.basename(archive), i, len(names))
            with z.open(n) as src:
                _write(src, _inside(staging, n[len(prefix):]))


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def pip_install(package, ctx):
    """pip install `package` into the Python that runs the builder (with
    --user when the plain install is not allowed)."""
    for extra in ([], ["--user"]):
        cmd = [python_exe(), "-m", "pip", "install", "--disable-pip-version-check"] + extra + [package]
        ctx.log("> " + subprocess.list2cmdline(cmd))
        r = subprocess.run(cmd, capture_output=True, text=True, creationflags=NO_WINDOW)
        for line in (r.stdout + r.stderr).splitlines()[-6:]:
            ctx.log("  " + line)
        if r.returncode == 0:
            return
    raise SetupError("pip could not install %s (see the log above)" % package)


def iso_extract(iso, disc_path):
    """The bytes of disc_path (e.g. "SCUS_971.02") from an ISO 9660 disc
    image with 2048-byte sectors (a PS2 DVD image), or None."""
    with open(iso, "rb") as f:
        f.seek(16 * 2048)
        pvd = f.read(2048)
        if pvd[1:6] != b"CD001" or pvd[0] != 1:
            return None
        extent, length = struct.unpack_from("<I", pvd, 156 + 2)[0], struct.unpack_from("<I", pvd, 156 + 10)[0]
        parts = [p for p in disc_path.replace("\\", "/").split("/") if p]
        for i, part in enumerate(parts):
            f.seek(extent * 2048)
            data = f.read(length)
            pos, found = 0, None
            while pos < len(data):
                n = data[pos]
                if n == 0:                       # records do not cross sectors
                    pos = (pos // 2048 + 1) * 2048
                    continue
                name = data[pos + 33:pos + 33 + data[pos + 32]].decode("ascii", "replace")
                if name.split(";")[0].upper() == part.upper():
                    found = (struct.unpack_from("<I", data, pos + 2)[0], struct.unpack_from("<I", data, pos + 10)[0],
                             data[pos + 25] & 2)
                    break
                pos += n
            if found is None:
                return None
            extent, length, is_dir = found
            if bool(is_dir) != (i < len(parts) - 1):
                return None
        f.seek(extent * 2048)
        return f.read(length)


def provide_game_file(game_file, chosen):
    """Check the file the user chose (the game file itself, or a disc image
    that has it) and keep it at game_file.dest.  Returns None, or why not."""
    try:
        with open(chosen, "rb") as f:
            data = f.read(64 << 20) if os.path.getsize(chosen) <= (64 << 20) else None
        if data is None or hashlib.sha256(data).hexdigest().upper() != game_file.sha256:
            from_iso = iso_extract(chosen, game_file.disc_path)
            if from_iso is None:
                if data is not None:
                    return ("%s is not the right file: this needs %s (SHA-256 %s...)."
                            % (os.path.basename(chosen), game_file.disc, game_file.sha256[:16]))
                return "%s is not a disc image with %s on it." % (os.path.basename(chosen), game_file.disc_path)
            data = from_iso
            if hashlib.sha256(data).hexdigest().upper() != game_file.sha256:
                return ("the %s on that disc is not %s (a different region or version?)."
                        % (game_file.disc_path, game_file.disc))
    except OSError as e:
        return "cannot read %s: %s" % (chosen, e.strerror)
    os.makedirs(os.path.dirname(game_file.dest), exist_ok=True)
    tmp = game_file.dest + ".tmp"
    with open(tmp, "wb") as f:
        f.write(data)
    os.replace(tmp, game_file.dest)
    return None


class InstallContext:
    """What Sdk.install() works with.  log(text) and progress(text, done,
    total) report; cancel is a threading.Event."""

    def __init__(self, log, progress, cancel=None, sdk_dir=SDK_DIR):
        self.log = log
        self.progress = progress
        self.cancel = cancel or threading.Event()
        self.sdk_dir = sdk_dir
        self.downloads = os.path.join(sdk_dir, ".downloads")

    def check(self):
        if self.cancel.is_set():
            raise Cancelled()

    def fetch(self, d):
        """Download d (a Download) into sdk/.downloads; returns the path.  A
        file already there with the right SHA-256 is used as it is."""
        os.makedirs(self.downloads, exist_ok=True)
        name = d.url.rstrip("/").split("/")[-1]
        dest = os.path.join(self.downloads, name)
        if d.sha256 and os.path.isfile(dest) and sha256_of(dest) == d.sha256.upper():
            self.log("Already downloaded: %s" % name)
            return dest
        self.log("Downloading %s" % d.url)
        part = dest + ".part"
        h = hashlib.sha256()
        req = urllib.request.Request(d.url, headers={"User-Agent": USER_AGENT})
        try:
            with urllib.request.urlopen(req, timeout=60) as r, open(part, "wb") as f:
                total = int(r.headers.get("Content-Length") or 0) or d.size
                done = 0
                while True:
                    self.check()
                    chunk = r.read(1 << 16)
                    if not chunk:
                        break
                    f.write(chunk)
                    h.update(chunk)
                    done += len(chunk)
                    self.progress("Downloading %s" % name, done, total)
        except Cancelled:
            os.remove(part)
            raise
        except OSError as e:
            if os.path.exists(part):
                os.remove(part)
            raise SetupError("downloading %s failed: %s" % (d.url, getattr(e, "reason", e)))
        if d.sha256 and h.hexdigest().upper() != d.sha256.upper():
            os.remove(part)
            raise SetupError("%s did not download correctly (SHA-256 mismatch); try again" % name)
        os.replace(part, dest)
        return dest

    def staging(self, home):
        """An empty folder to unpack into: sdk/<home>.partial."""
        path = os.path.join(self.sdk_dir, home + ".partial")
        _rmtree(path)
        os.makedirs(path)
        return path

    def verify(self, path, sha256):
        if sha256_of(_long(path)) != sha256.upper():
            raise SetupError("%s is not the expected file (SHA-256 mismatch)" % os.path.basename(path))

    def commit(self, staging, home):
        """Move the unpacked staging folder into place as sdk/<home>."""
        final = os.path.join(self.sdk_dir, home)
        _rmtree(final)
        os.replace(_long(staging), _long(final))

    def cleanup(self, home):
        try:
            _rmtree(os.path.join(self.sdk_dir, home + ".partial"))
        except OSError:
            pass

    def forget_downloads(self, sdk):
        """Delete sdk's downloaded archives once it is installed."""
        for d in sdk.downloads:
            p = os.path.join(self.downloads, d.url.rstrip("/").split("/")[-1])
            if os.path.exists(p):
                os.remove(p)


def install_all(sdks, ctx):
    """Install each SDK in order.  Returns [(sdk, None or error text)]."""
    results = []
    for i, sdk in enumerate(sdks):
        ctx.log("== %s (%d of %d)" % (sdk.name, i + 1, len(sdks)))
        try:
            sdk.install(ctx)
            ctx.forget_downloads(sdk)
            ctx.log("Installed %s" % sdk.name)
            results.append((sdk, None))
        except Cancelled:
            if sdk.home:
                ctx.cleanup(sdk.home)
            results.append((sdk, "cancelled"))
            break
        except (SetupError, OSError, tarfile.TarError, zipfile.BadZipFile) as e:
            if sdk.home:
                ctx.cleanup(sdk.home)
            ctx.log("Could not install %s: %s" % (sdk.name, e))
            results.append((sdk, str(e)))
    return results
