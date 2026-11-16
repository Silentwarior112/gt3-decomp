"""Build the GT3 US main executable (core.gt3) from the decomp.

    python tools/build.py                        # GCC 14, into build/gcc14/
    python tools/build.py --compiler 991111b     # the original compiler, into build/991111b/

The output is <build>/CORE.GT3: the program packed and signed as the
bootstrap loads it (tools/gt3image.py), for the disc or a memory card.

What goes into it, for now: the decompiled C is compiled and checked, but the
program is still linked from the original code, the splat split of
baserom/BASE_SCUS-97102 relinked byte for byte (tools/relink.py). Linking the
compiled units in place of their assembly is the next step of the roadmap:
it needs the data, which the C does not define yet (ref/pass2/after_phase2.md).

--compiler gcc14 (default): GCC 14, the EE toolchain's own, with the flags of
  tools/cc.sh (the ones the source tree is checked with).
--compiler 991111b: SCE ee-gcc 2.9-ee-991111b, the compiler the original was
  built with (<repo>/sdk/ee-gcc-2.9-991111b), with each file's flags of ref/compile_flags.json
  except -G0 for all (the hybrid cannot put the C's small data near $gp), paths relative to
  core/ (its output depends on their length). The .S files go through the EE
  toolchain's assembler in both builds.
--toolchain DIR picks the EE toolchain: its bin folder, or a folder that has
  ee/bin. Default: tools/paths.py (<repo>/sdk/ps2sdk, ps2sdk-main next to the
  repository, $PS2SDK_HOME, $PS2DEV). Both builds use its assembler and ld.
--link full (with --compiler 991111b): the program from the C alone,
  tools/fulllink.py (the matching flags of ref/compile_flags.json, -G8
  included; only alignment padding and the frame info of code that does not
  match yet come from the original): stages 0 and 4 (the relink, for the
  split's symbols), a full tools/score.py (the objects' frame info and the
  bases their code's data references imply, build/score/, which the layout
  reads), then fulllink's own compile, link and pack. --link hybrid (the default): the
  split with the C units over it, the stages below.
--build-dir DIR: where everything goes (default build/<compiler>/).
--jobs N compiles N files at once (default: one per CPU). --progress prints a
"[done/total] file" line as each file finishes (the GT3 Decomp Builder reads it).
--keep-going compiles the other files after a failure.
--compile-only stops after the report (no game file needed).

Stages:
  0. base      baserom/BASE_SCUS-97102, the program as an ELF, made from your
               baserom/CORE.GT3 when it is missing (the GT3 Decomp Builder puts
               the disc's CORE.GT3 there); then the split in ref/splat/
               (tools/gen_splat.py and splat) when it is missing or older than
               what it is made from
  1. compile   every .c/.cpp/.cc/.S under src/ into <build>/obj/
  2. combine   ld -r of all objects into <build>/core.o: two definitions of
               one symbol fail here
  3. report    the symbols core.o uses but does not define, in
               <build>/undefined.txt: the data and runtime the link still needs
  4. link      the split assembled and linked at the original addresses in
               <build>/link/; the image must equal BASE_SCUS-97102
  5. pack      <build>/CORE.GT3, signed with the decomp's key (the same input
               gives the same file)

Exit status 0 only when every stage worked and CORE.GT3 was written.
"""
import argparse
import concurrent.futures as cf
import hashlib
import json
import os
import re
import subprocess
import sys
import time

import funcmap
import gt3image
import hybrid
import mcc
import paths
import relink

ROOT = paths.ROOT
SRC = os.path.join(ROOT, "src")
EGCS = paths.sdk("ee-gcc-2.9-991111b")
EGCS_CC = os.path.join(EGCS, "bin", "ee-gcc" + paths.EXE)
COMPILERS = ("gcc14", "991111b")

# tools/cc.sh's flags: -fno-builtin-printf keeps printf("...\n") a printf call;
# -fno-sized-deallocation gives operator delete(void *) as egcs calls it.
GCC14_FLAGS = ["-O2", "-G0", "-I" + os.path.join(ROOT, "include"), "-I" + os.path.join(ROOT, "src"), "-Wall", "-Wno-unused",
               "-Wno-unused-parameter", "-Wno-attributes", "-Wno-unknown-pragmas", "-fno-builtin-printf"]
GCC14_C = ["-std=gnu99"]
GCC14_CXX = ["-fno-sized-deallocation", "-Wno-pmf-conversions", "-std=gnu++14", "-fpermissive"]
# -ffast-math: the original has max.s / min.s for clamps and sqrt.s without
# sqrtf's errno call, which egcs emits only with it
EGCS_FLAGS = ["-O2", "-G0", "-ffast-math"]

TC_BIN = None


def tool(name):
    return os.path.join(TC_BIN, paths.EE_PREFIX + name + paths.EXE)


def run(cmd, cwd=ROOT):
    # SCE ee-gcc's driver gets its own environment (paths.egcs_env)
    env = dict(paths.egcs_env(EGCS_CC) if cmd[0] == EGCS_CC else os.environ)
    env["PATH"] = TC_BIN + os.pathsep + env.get("PATH", "")
    return subprocess.run(cmd, capture_output=True, text=True, errors="replace", env=env, cwd=cwd)


def sources():
    out = []
    for dp, _, fns in os.walk(SRC):
        for fn in fns:
            if fn.endswith((".c", ".cpp", ".cc", ".S", ".s")):
                out.append(os.path.relpath(os.path.join(dp, fn), ROOT).replace("\\", "/"))
    return sorted(out)


def compile_one(rel, compiler, obj_dir):
    """(ok, messages) of compiling src/<unit>/<file> into obj_dir."""
    o = os.path.join(obj_dir, rel.replace("/", "__") + ".o")
    if rel.endswith((".S", ".s")):
        cmd = [tool("gcc"), "-I" + os.path.join(ROOT, "include"), "-I" + os.path.join(ROOT, "src"), "-c", rel, "-o", o]
    elif compiler == "991111b":
        rel_o = os.path.relpath(o, ROOT).replace("\\", "/")
        inc = os.path.relpath(os.path.join(EGCS, "include"), ROOT).replace("\\", "/")
        cxx = rel.endswith((".cpp", ".cc"))
        # Preprocessed into the build folder first: egcs names anonymous
        # namespaces (and their typeinfo name strings in the program) after the
        # file cc1plus reads, which would otherwise be a temporary file under
        # the user's profile.  The line markers keep the source paths relative.
        rel_i = rel_o[:-2] + (".ii" if cxx else ".i")
        r = run([EGCS_CC, "-x", "c++" if cxx else "c", "-E", "-Iinclude", "-Isrc", "-I" + os.path.dirname(rel), "-I" + inc,
                 "-o", rel_i, rel])
        if r.returncode:
            return False, (r.stdout + r.stderr).strip(), o
        # the matching flags of ref/compile_flags.json, except small data: the
        # hybrid cannot place the C's own .sdata near the original's $gp
        flags = [("-G0" if f.startswith("-G") else f) for f in mcc.flags_for(rel)]
        base = [EGCS_CC, "-x", "c++-cpp-output" if cxx else "cpp-output"] + flags
        r = run(base + ["-c", "-o", rel_o, rel_i])
        if r.returncode:
            return False, (r.stdout + r.stderr).strip(), o
        # a second pass, to assembly with -g, only for its stabs: where each
        # function starts (tools/funcmap.py ties them to the FUNC markers).  The
        # object comes from the first: egcs's -g changes the code of a few functions.
        rel_s = rel_o[:-2] + ".g.s"
        r2 = run(base + ["-S", "-g", "-o", rel_s, rel_i])
        s_path = os.path.join(ROOT, rel_s)
        if r2.returncode or not os.path.isfile(s_path):
            return False, (r2.stdout + r2.stderr).strip(), o
        with open(o[:-2] + ".funcs.json", "w", encoding="utf-8") as f:
            json.dump(funcmap.stab_functions(open(s_path, encoding="utf-8", errors="replace").read()), f)
        for p in (s_path, os.path.join(ROOT, rel_i)):
            os.remove(p)
        return os.path.isfile(o), (r.stdout + r.stderr).strip(), o
    elif rel.endswith((".cpp", ".cc")):
        cmd = [tool("g++")] + GCC14_FLAGS + GCC14_CXX + ["-c", rel, "-o", o]
    else:
        cmd = [tool("gcc")] + GCC14_FLAGS + GCC14_C + ["-c", rel, "-o", o]
    r = run(cmd)
    return r.returncode == 0 and os.path.isfile(o), (r.stdout + r.stderr).strip(), o


def ensure_base():
    """baserom/BASE_SCUS-97102 in place (made from baserom/CORE.GT3 if need be).
    True when it is the retail program."""
    elf = paths.RETAIL_ELF
    if os.path.isfile(elf) and hashlib.sha256(open(elf, "rb").read()).hexdigest().upper() == paths.RETAIL_SHA256:
        return True
    core = os.path.join(ROOT, "baserom", "CORE.GT3")
    if not os.path.isfile(core):
        print("error: neither %s nor %s is there: the GT3 Decomp Builder copies CORE.GT3 there from "
              "your disc or disc image" % (elf, core))
        return False
    print("== base: %s from %s" % (elf, core), flush=True)
    data = gt3image.Image.read(open(core, "rb").read()).to_elf()
    if hashlib.sha256(data).hexdigest().upper() != paths.RETAIL_SHA256:
        print("error: %s is not the retail US CORE.GT3 (its program differs)" % core)
        return False
    with open(elf, "wb") as f:
        f.write(data)
    return True


SPLIT_INPUTS = ([os.path.join(paths.REF, n) for n in ("funcs.json", "units.json", "symbols.csv", "data_symbols.csv")] +
                [os.path.join(paths.TOOLS, "gen_splat.py"), paths.RETAIL_ELF])


def ensure_split():
    """ref/splat/ made from the base, unless it is there and newer than its inputs."""
    ld = os.path.join(relink.SPLAT, "core.ld")
    if os.path.isfile(ld) and all(os.path.getmtime(ld) >= os.path.getmtime(p) for p in SPLIT_INPUTS if os.path.exists(p)):
        return True
    print("== split: tools/gen_splat.py, then splat (ref/splat/)", flush=True)
    for cmd, cwd in (([sys.executable, os.path.join(paths.TOOLS, "gen_splat.py")], paths.TOOLS),
                     ([sys.executable, "-m", "splat", "split", "core.yaml"], relink.SPLAT)):
        r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", cwd=cwd)
        if r.returncode:
            print((r.stdout + r.stderr).strip()[-3000:])
            print("error: %s failed" % " ".join(cmd[1:]))
            return False
    os.utime(ld)                        # splat keeps files it did not change
    return True


def compile_all(rels, compiler, obj_dir, jobs, progress, keep_going):
    fails, warned, objs, done = [], 0, [], 0
    with cf.ThreadPoolExecutor(max_workers=jobs) as ex:
        futs = {ex.submit(compile_one, rel, compiler, obj_dir): rel for rel in rels}
        for fut in cf.as_completed(futs):
            if fut.cancelled():
                continue
            rel = futs[fut]
            ok, msg, o = fut.result()
            done += 1
            if progress:
                print("[%d/%d] %s" % (done, len(rels), rel), flush=True)
            if not ok:
                fails.append(rel)
                print("FAIL compile %s\n%s" % (rel, msg[-3000:]), flush=True)
                if not keep_going:
                    for f in futs:
                        f.cancel()
            else:
                objs.append(o)
                n = msg.count("warning:")
                if n:
                    warned += 1
                    if compiler == "gcc14":
                        print("warn compile %s (%d warnings)" % (rel, n), flush=True)
    return fails, warned, sorted(objs)


def main():
    global TC_BIN
    ap = argparse.ArgumentParser(description="Compile the GT3 US main executable from the decompiled sources.")
    ap.add_argument("--compiler", choices=COMPILERS, default="gcc14")
    ap.add_argument("--toolchain", help="the EE toolchain (default: tools/paths.py)")
    ap.add_argument("--build-dir", help="where the build goes (default: build/<compiler>/)")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    ap.add_argument("--progress", action="store_true", help="print [done/total] lines")
    ap.add_argument("--keep-going", action="store_true", help="compile the rest after a failure")
    ap.add_argument("--compile-only", action="store_true", help="stop after the report (no game file needed)")
    ap.add_argument("--link", choices=("hybrid", "full"), default="hybrid",
                    help="991111b: hybrid (the split with the C units over it) or full (the C alone)")
    a = ap.parse_args()
    t0 = time.time()

    TC_BIN = paths.ee_bin(a.toolchain)
    if not TC_BIN:
        print("error: no EE toolchain (GCC 14): pass --toolchain, or see the README (Downloads)")
        return 1
    if a.compiler == "991111b" and not os.path.isfile(EGCS_CC):
        print("error: %s is missing: the GT3 Decomp Builder downloads SCE ee-gcc 2.9-ee-991111b there" % EGCS_CC)
        return 1
    build = os.path.abspath(a.build_dir or os.path.join(ROOT, "build", a.compiler))
    obj_dir = os.path.join(build, "obj")
    os.makedirs(obj_dir, exist_ok=True)
    core_o = os.path.join(build, "core.o")
    out_gt3 = os.path.join(build, "CORE.GT3")
    for p in (core_o, out_gt3):
        if os.path.exists(p):
            os.remove(p)                # exit 0 means fresh files
    if not a.compile_only and not (ensure_base() and ensure_split()):
        return 1

    if a.compiler == "991111b" and a.link == "full" and not a.compile_only:
        # the relink first: its symbols and the split's data relocations tell
        # the full link what the original's leftover data points at
        relink.TC_BIN = TC_BIN
        if relink.relink(os.path.join(build, "link"), a.jobs) is None:
            return 1
        # the layout reads what a full score run writes (build/score/): each
        # object's frame info offset and the base its code's data references
        # imply; a fresh clone has none, and an old run may be stale
        print("== score (tools/score.py: frame info and data references for the layout)", flush=True)
        r = subprocess.run([sys.executable, os.path.join(paths.TOOLS, "score.py"), "--jobs", str(max(1, a.jobs))],
                           cwd=ROOT, capture_output=True, text=True)
        if r.returncode:
            print(r.stdout[-3000:] + r.stderr[-3000:])
            print("error: score.py failed")
            return 1
        print("\n".join("   " + L for L in r.stdout.splitlines() if L.startswith(("functions_exact", "files_failed"))))
        print("== full link (tools/fulllink.py: the C alone)", flush=True)
        cmd = [sys.executable, os.path.join(paths.TOOLS, "fulllink.py"), "--build-dir", os.path.join(build, "full"),
               "--relink", os.path.join(build, "link"), "--pack", out_gt3, "--jobs", str(max(1, a.jobs)),
               "--toolchain", TC_BIN]
        r = subprocess.run(cmd + (["--progress"] if a.progress else []), cwd=ROOT)
        if r.returncode or not os.path.isfile(out_gt3):
            print("error: the full link failed")
            return 1
        print("built %s in %.1f s" % (out_gt3, time.time() - t0))
        return 0

    rels = sources()
    stale = {rel.replace("/", "__") + ".o" for rel in rels}
    for fn in os.listdir(obj_dir):      # objects of files that no longer exist
        if fn.endswith(".o") and fn not in stale:
            os.remove(os.path.join(obj_dir, fn))

    print("== compile (%s, %d files, %d at a time)" % (a.compiler, len(rels), a.jobs), flush=True)
    fails, warned, objs = compile_all(rels, a.compiler, obj_dir, max(1, a.jobs), a.progress, a.keep_going)
    if fails:
        print("error: %d file(s) failed to compile: %s" % (len(fails), ", ".join(sorted(fails)[:10])))
        return 1
    print("compiled %d files%s" % (len(objs), " (%d with warnings)" % warned if warned else ""), flush=True)

    print("== combine (ld -r into %s)" % core_o, flush=True)
    listing = os.path.join(build, "objects.txt")
    with open(listing, "w", encoding="utf-8") as f:
        f.write("\n".join(paths.ld_path(o) for o in objs) + "\n")      # (ld runs in ROOT)
    # the original compiler's objects carry another ABI flag than GCC 14's
    r = run([tool("ld"), "-r", "--no-warn-mismatch", "-o", core_o, "@" + listing])
    if r.returncode or not os.path.isfile(core_o):
        print((r.stdout + r.stderr).strip()[-4000:])
        print("error: combining the objects failed")
        return 1

    print("== report", flush=True)
    r = run([tool("nm"), "-u", core_o])
    undef = sorted({L.split()[-1] for L in r.stdout.splitlines() if L.strip()})
    r = run([tool("nm"), "--defined-only", core_o])
    defined = [L for L in r.stdout.splitlines() if re.match(r"^[0-9a-fA-F]+ [TtWw] ", L)]
    with open(os.path.join(build, "undefined.txt"), "w", encoding="utf-8") as f:
        f.write("# symbols core.o uses but does not define (data, runtime): what the link still needs\n")
        f.write("\n".join(undef) + "\n")
    print("core.o: %d functions defined, %d symbols still undefined (undefined.txt: data and runtime "
          "the link will need)" % (len(defined), len(undef)))
    if a.compile_only:
        print("built %s in %.1f s" % (core_o, time.time() - t0))
        return 0

    relink.TC_BIN = TC_BIN
    image = relink.relink(os.path.join(build, "link"), a.jobs)
    if image is None:
        return 1
    if a.compiler != "991111b":
        # GCC 14's C++ (exceptions, RTTI, vtables) does not mix with the
        # original's, so its objects are not linked in yet
        print("link: the original code only (the GCC 14 objects are compiled and checked, not linked)")
        relink.pack(image, out_gt3)
    else:
        print("== hybrid (hybrid.txt: the units linked from the C)", flush=True)
        hybrid.TC_BIN = TC_BIN
        all_units = [u for u, _, _ in hybrid.unit_ranges()]
        units, keep = hybrid.read_config(hybrid.CONFIG, all_units)
        elf, report = hybrid.link(build, units, out_gt3, keep)
        with open(os.path.join(build, "hybrid", "report.txt"), "w", encoding="utf-8") as f:
            f.write("\n".join(report) + "\n")
        for line in report:
            if not line.startswith("  ") or not elf:
                print(line)
        if elf is None:
            print("error: the hybrid did not link (%s)" % os.path.join(build, "hybrid", "report.txt"))
            return 1
    print("built %s in %.1f s" % (out_gt3, time.time() - t0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
