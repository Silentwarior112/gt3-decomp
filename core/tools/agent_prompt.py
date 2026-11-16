"""The task for the agent that decompiles one work unit.

    python tools/agent_prompt.py <unit>        print it
    python tools/agent_prompt.py --write       write it to every ref/units/<unit>/TASK.md
"""
import json
import os
import sys

import paths

ROOT = paths.ROOT.replace("\\", "/")


def prompt(u):
    units = json.load(open(os.path.join(paths.REF, "units.json")))
    s = units[u]
    lo, hi = s["ranges"][0]
    pk = os.path.join(paths.REF, "units", u)
    has_boot = os.path.exists(os.path.join(pk, "boot.txt"))
    split = (" Translation units only partly in this unit: %s (name those files with the _%s suffix)."
             % (", ".join(s["split_tus"]), u)) if s["split_tus"] else ""
    cls = (" Main classes: %s." % ", ".join(s["classes"][:6])) if s["classes"] else ""
    boot = ("\n- boot.txt: bootstrap source of the %d functions marked BOOT= in index.txt. It is the same machine "
            "code: reuse it (new FUNC address, your names)." % s["boot"]) if has_boot else ""
    return f"""You are decompiling one work unit of the Gran Turismo 3 A-Spec US main executable (PlayStation 2 EE, MIPS R5900) into C/C++. This is the first, fast pass: semantically faithful source for every function, quickly.

Project root: {ROOT}  (use the Bash tool; start every command with: cd "{ROOT}" && ...)
Read these completely first: ref/CONTEXT.md, ref/DECOMP_RULES.md. Follow DECOMP_RULES.md exactly.

YOUR UNIT: {u}, addresses {lo}..{hi}: {s['nfuncs']} functions, {s['instrs']} instructions, translation units {', '.join(s['tus'])}.{split}{cls}
Your packet, ref/units/{u}/:
- index.txt: the function list (address, size, translation unit, known name, class, callers). Read it all.
- m2c.c: machine drafts of nearly every function. Start each function from its draft.
- listing.txt: the annotated disassembly of exactly your functions: the ground truth. Read the parts you need to check a draft, and the functions m2c missed.
- classes.txt: the classes your unit owns, with vtables.{boot}
Write only under src/{u}/ (create it). If it already exists, an earlier run was interrupted: run the coverage check first, keep the functions already written, and finish the missing ones.

Work through the functions in address order, translation unit by translation unit. Write the source in pieces as you go (create each file early, then append with Edit), so no work is lost, and compile each file as soon as it has its first functions: sh tools/cc.sh --unit {u}
Keep to the first pass: do not build emulators, test harnesses or extra tools, and for classes owned by other units declare only what your code uses. Your checks are reading the listing, compiling and the coverage check.
Before finishing, both must pass:
  sh tools/cc.sh --unit {u}                      (every file OK)
  python tools/check_coverage.py --unit {u}      (missing 0, duplicated 0, unknown 0)

Final answer, at most 15 lines: files written; functions claimed / total; compile and coverage results; up to 5 notable findings about what this code does; the addresses of functions you are unsure about."""


def launcher(u):
    """The short message that starts an agent on a unit."""
    return ("Decompile work unit %s of the GT3 main executable. Your complete task is in "
            "%s/ref/units/%s/TASK.md: read it first and follow it exactly." % (u, ROOT, u))


if __name__ == "__main__":
    if sys.argv[1] == "--write":
        units = json.load(open(os.path.join(paths.REF, "units.json")))
        for u in units:
            open(os.path.join(paths.REF, "units", u, "TASK.md"), "w", encoding="utf-8", newline=chr(10)).write(prompt(u) + chr(10))
        print("wrote TASK.md for %d units" % len(units))
    else:
        print(prompt(sys.argv[1]))
