"""Finds the plug-ins: every toolchain/sdks/*.py defines SDK and every
toolchain/targets/*.py defines TARGET.  Files starting with "_" are skipped."""
import importlib
import importlib.util
import os
import traceback

from . import TOOLCHAIN_DIR
from .model import Sdk, Status, Target


def _load(folder, attr, base):
    found, errors = [], []
    if not os.path.isdir(folder):
        return found, errors
    for fn in sorted(os.listdir(folder)):
        if not fn.endswith(".py") or fn.startswith("_"):
            continue
        path = os.path.join(folder, fn)
        try:
            spec = importlib.util.spec_from_file_location(
                "gt3builder_%s_%s" % (os.path.basename(folder), fn[:-3]), path)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            obj = getattr(mod, attr, None)
            if not isinstance(obj, base):
                raise TypeError("the file must define %s = <a %s subclass>()" % (attr, base.__name__))
            found.append(obj)
        except Exception:
            errors.append("%s did not load:\n%s" % (path, traceback.format_exc(limit=4).rstrip()))
    return found, errors


class Registry:
    def __init__(self, root=TOOLCHAIN_DIR):
        self.sdks, self.targets, self.errors = {}, {}, []
        sdks, errs = _load(os.path.join(root, "sdks"), "SDK", Sdk)
        self.errors += errs
        for s in sdks:
            if s.id in self.sdks:
                self.errors.append("two SDKs have the id %r" % s.id)
            self.sdks[s.id] = s
        targets, errs = _load(os.path.join(root, "targets"), "TARGET", Target)
        self.errors += errs
        for t in targets:
            if t.id in self.targets:
                self.errors.append("two targets have the id %r" % t.id)
            if not t.options:
                self.errors.append("target %r has no options" % t.id)
            for o in t.options:
                for sid in [o.sdk] + o.also:
                    if sid not in self.sdks:
                        self.errors.append("target %r, option %r: unknown SDK %r" % (t.id, o.id, sid))
            self.targets[t.id] = t

    def needs(self, target, option_id):
        """Every SDK id a build of target with option_id uses: the option's own
        SDK first, then the others, each with what it requires."""
        o = target.option(option_id)
        if o is None:
            return []
        out = []

        def add(sid):
            if sid in out:
                return
            out.append(sid)
            s = self.sdks.get(sid)
            for r in (s.requires if s else []):
                add(r)
        for sid in [o.sdk] + o.also:
            add(sid)
        return out

    def detect(self, sdk_id, settings):
        """detect() of one SDK, never raising."""
        s = self.sdks.get(sdk_id)
        if s is None:
            return Status(False, problems=["no SDK plug-in has the id %r" % sdk_id])
        try:
            importlib.invalidate_caches()         # a package pip just installed
            st = s.detect(settings)
            if not isinstance(st, Status):
                raise TypeError("detect() returned %r, not a Status" % (st,))
            return st
        except Exception as e:
            return Status(False, problems=["detecting %s failed: %s: %s" % (s.name, type(e).__name__, e)])

    def detect_all(self, settings):
        return {sid: self.detect(sid, settings) for sid in self.sdks}

    def missing(self, target, option_id, status):
        """(SDK ids that are not ready, game files that are absent) for one option."""
        sdks = [sid for sid in self.needs(target, option_id)
                if not (status.get(sid) and status[sid].ready)]
        files = [g for g in target.game_files if not g.present()]
        return sdks, files

    def readiness(self, target, option_id, status):
        """(ready, problems) of building target with option_id."""
        if target.option(option_id) is None:
            return False, ["%s has no option %r" % (target.name, option_id)]
        problems = list(target.problems())
        sdks, files = self.missing(target, option_id, status)
        for sid in sdks:
            s = self.sdks.get(sid)
            st = status.get(sid)
            why = "; ".join(st.problems) if st and st.problems else "not installed"
            problems.append("%s: %s" % (s.name if s else sid, why))
        for g in files:
            problems.append("your %s (%s) is not in place" % (g.name, g.disc))
        return not problems, problems
