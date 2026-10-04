"""The builder's own files in toolchain/ (neither is part of the repository):

profiles.json   the profiles: what to build, with which option, where the ELF goes
settings.json   this PC's choices: a folder picked for an SDK, the window size
"""
import json
import os
import re

from . import TOOLCHAIN_DIR

SETTINGS_FILE = os.path.join(TOOLCHAIN_DIR, "settings.json")
PROFILES_FILE = os.path.join(TOOLCHAIN_DIR, "profiles.json")


def _write_json(path, data):
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    os.replace(tmp, path)


def _read_json(path):
    """(data or None, note): None when the file is missing.  A file that is
    not valid JSON is renamed out of the way, never overwritten."""
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f), ""
    except FileNotFoundError:
        return None, ""
    except (OSError, ValueError) as e:
        n = 1
        while os.path.exists("%s.bad%d" % (path, n)):
            n += 1
        try:
            os.replace(path, "%s.bad%d" % (path, n))
            return None, "%s could not be read (%s); kept it as %s.bad%d" % (path, e, os.path.basename(path), n)
        except OSError:
            return None, "%s could not be read (%s)" % (path, e)


def load_settings():
    data, note = _read_json(SETTINGS_FILE)
    return (data if isinstance(data, dict) else {}), note


def save_settings(settings):
    _write_json(SETTINGS_FILE, settings)


def slug(name):
    return re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")


class Profile:
    """What to build (a target id), with which of its options, and where the
    finished ELF goes."""

    def __init__(self, name, target, option="", output=""):
        self.name = name
        self.target = target
        self.option = option
        self.output = output

    def to_json(self):
        return {"name": self.name, "target": self.target, "option": self.option, "output": self.output}

    @classmethod
    def from_json(cls, d):
        return cls(str(d.get("name", "")).strip(), str(d.get("target", "")),
                   str(d.get("option", d.get("compiler", ""))),       # "compiler": the first version's key
                   str(d.get("output", "")))

    def copy(self):
        return Profile(self.name, self.target, self.option, self.output)


class Profiles:
    def __init__(self, items=(), selected=""):
        self.items = list(items)
        self.selected = selected

    def names(self):
        return [p.name for p in self.items]

    def get(self, name):
        for p in self.items:
            if p.name == name:
                return p
        return None

    def unique_name(self, base):
        names = {p.name.lower() for p in self.items}
        if base.lower() not in names:
            return base
        n = 2
        while ("%s %d" % (base, n)).lower() in names:
            n += 1
        return "%s %d" % (base, n)

    def find(self, query):
        """Profiles matching a command-line name: the exact name, or its short
        form ("bootstrap-fast"), or a unique start of either (any case)."""
        q, qs = query.strip().lower(), slug(query)
        exact = [p for p in self.items if p.name.lower() == q or (qs and slug(p.name) == qs)]
        if exact:
            return exact
        return [p for p in self.items if p.name.lower().startswith(q) or (qs and slug(p.name).startswith(qs))]

    def to_json(self):
        return {"version": 2, "selected": self.selected, "profiles": [p.to_json() for p in self.items]}


def default_profiles(registry):
    """One profile per target and option."""
    items = []
    for t in sorted(registry.targets.values(), key=lambda t: t.name):
        for o in t.options:
            items.append(Profile("%s (%s)" % (t.short_name or t.name, o.label.lower()), t.id, o.id,
                                 t.default_output(o.id)))
    return Profiles(items, items[0].name if items else "")


def load_profiles(registry):
    """(Profiles, note).  The first run starts from default_profiles()."""
    data, note = _read_json(PROFILES_FILE)
    if not isinstance(data, dict) or not isinstance(data.get("profiles"), list):
        return default_profiles(registry), note
    items, seen = [], set()
    for d in data["profiles"]:
        if isinstance(d, dict):
            p = Profile.from_json(d)
            if p.name and p.name.lower() not in seen:
                seen.add(p.name.lower())
                t = registry.targets.get(p.target)
                if t and t.option(p.option) is None and t.options:
                    p.option = t.options[0].id
                items.append(p)
    return Profiles(items, str(data.get("selected", ""))), note


def save_profiles(profiles):
    _write_json(PROFILES_FILE, profiles.to_json())
