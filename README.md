# Gran Turismo 3 Decompilation
<p align="center">
  <img src="https://github.com/Silentwarior112/gt3-decomp/blob/main/logo.png">
</p>



Decompiled source code of the Gran Turismo 3 family of executables, and the **GT3
Decomp Builder**, which streamlines the process of compiling the source code. Choose what to build and with which
compiler. The builder downloads that compiler and writes the finished ELF
wherever you want it, such as your extracted disc.

| Executable | Source Code Folder | State |
|---|---|---|
| GT3 US bootstrap, `SCUS_971.02` | [bootstrap/](bootstrap/README.md) | All 881 functions in C/C++. 851 are byte-exact when built with Sony's compiler (99.4% of the code bytes). Builds and boots to the game. |
| GT3 US core, `core.gt3` | [core/](core/README.md) | 10,959 of 11,415 functions exact, Builds and runs. |
| GT3 US (Enhanced Engine), `CORE.GT3` | [core_gt3enhanced/](core_gt3enhanced/README.md) | GT3 + GTC engine features: The frontier engine for modding purposes |

# Gran Turismo 3 core executable status
| Group | What it is | Functions | How it differs from the original | Severity |
|---|---|---|---|---|
| Menus, game flow, save data 									              | Game | 61 (41 same size) | Register numbering and instruction order. In 20, a few extra or missing instructions (spills, a reload, branch layout) | None |
| Car physics and dynamics 										                | Game | 42 (25 same size) | Registers and scheduling; 17 with spill or reload differences. Same float operations in the same order | Low: a few cycles; no known effect on results |
| Rendering and effects (cars, smoke, flares, tyres, track) 	| Game | 36 (24 same size) | Registers and scheduling; some loops with different hoisting or spills | None to low |
| Race HUD and displays 										                  | Game | 29 (22 same size) | Mostly registers; a few branch-layout differences | None |
| Race logic, AI, organisation 									              | Game | 17 (11 same size) | Registers and spills; one sound function lacks the original's dead address code and larger stack frame | None to low |
| Cameras 														                        | Game | 9 (6 same size) | Registers and scheduling | None |
| Debug logger 													                      | Game | 8 | Registers and scheduling | None |
| Utilities 													                        | Game support, not PS2-specific | 7 | SAFER's code has the original's shape but a different round schedule (36–41% of bytes) | None: same output |
| Sound logic 													                      | Game | 4 | Registers and scheduling; one with different loads | Low |
| Graphics library 												                    | PS2 | 57 (43 same size) | Registers and scheduling; some originals were hand-written assembly | None to low: same results, at most a few cycles |
| PlayStation 2 I/O 											                    | PS2 | 14 | Registers, spills, block layout | Low: same calls, same order |
| Media drivers: (MPEG/IPU player, SPU2 sound) 					      | PS2 | 14 | Registers; the stream parser's block layout | Low: the intro movie plays the same |
| Sony SDK and i.LINK RPC 										                | PS2 | 5 | Registers and spills | Low |
| C runtime: libm, libgcc, newlib, printf 						        | PS2 toolchain runtime | 12 (10 same size) | Registers. Several are exact with the other Sony compiler build (2.9-ee-991111-01) | None |
| Hand-written assembly: crt0, syscall stubs, `strlen` 			  | PS2 | 141 | Not scored, because the scorer only compares C; the bytes are the original's | None |

**No game data is included.** Building needs your own copy of Gran Turismo 3
A-Spec (US, SCUS-97102). The builder asks once for its `SCUS_971.02`, or for
the disc image it is on.

## Quick start

1. Install [**Python 3.9 or newer**](https://www.python.org/downloads/)
   with the installer's default options.
2. Double-click **`GT3Builder.pyw`**.
3. The **Set up** window opens. Pick the executable under **What to build**
   It lists what that executable needs.
   All necessary dependencies / compilers are linked into the builder,
   and only requires one click to download what's needed. <br>
   Click **Download**, and locate the ELF. <br>
   You can set up profiles for different executables and different configurations
   of the compiled output. When ready, click **Build**.

Windows 10 or 11. **Keep this folder's path under 100 characters.** <br>
GCC cannot open files past Windows' 260-character path
limit, and the toolchain's own header paths are long. The first run creates
one profile per compiler option of each executable.

## The builder

### Profiles

The list on the left holds the profiles. The right side edits the selected
one, and every change is saved at once.

| Field | What it is |
|---|---|
| **Name** | Anything unique. |
| **What to build** | The executable. |
| **Compiler** | Which purpose for building: Modified code, or source/bytes matching. There are 2 compiler options: **Fast** (modern GCC, about 2 s, for modding) or **Period-accurate** (the compiler the original was built with, slow, close to the retail code). |
| **Game files** | The ELF executable the build needs |
| **Output ELF** | Where the compiled ELF is outputted |

- **Managing profiles:** **New**, **Copy**, **Delete** and the arrows manage
  the list. Right-click a profile for the same actions, plus Build and the Go
  to buttons.
- **Set up:** whenever the selected profile cannot build, the builder opens
  **Set up** on its executable. **Tools > Set up...** opens it any time;
  its **What to build** dropdown lists every executable, and switching it
  shows what that one needs.

### Building

**Build** runs the build and shows its output under **Build log**:
- the progress bar follows the files;
- **Stop** ends a build;
- **Go to source** opens the project folder;
- **Go to output** shows the ELF in Explorer.

When a build fails, the log jumps to the first compiler error. Double-click an
error to open the file at that line, in VS Code if it is installed.

Safeguards:
- **Originals.** The first time a build would replace an original file (the
  retail `SCUS_971.02`, recognised by its SHA-256), that file is copied to
  `backups/` first.
- **Partial copies.** The ELF is written under a temporary name first, so a
  failed copy never leaves a half-written file.

### Downloads

The builder asks before it downloads anything. You can untick items you do
not want. Each archive is checked against the SHA-256 in its plug-in
(`toolchain/sdks/`). Everything goes to `sdk/` in this folder, except Unicorn,
which pip installs into your Python.

| Download | Needed for | Size | From | Licence |
|---|---|---|---|---|
| EE GCC 14.2, the ps2dev toolchain (its `ee/` folder) | Fast & Accurate (the period-accurate one links with it) | about 310 MB | [ThirteenAG/ps2sdk](https://github.com/ThirteenAG/ps2sdk), a standalone Windows build of [ps2dev](https://github.com/ps2dev/ps2dev), pinned to commit `057829f` | GPL (GCC, binutils), newlib's |
| Sony ee-gcc 2.9-ee-991111-01, with the glibc 2.3.6 it runs on | period-accurate | 7 MB | [decomp.me's compiler archive](https://github.com/decompme/compilers/releases/tag/compilers) | GPL; glibc LGPL |
| SCE ee-gcc 2.9-ee-991111b: driver, preprocessor, assembler, `libgcc.a` | period-accurate | 16 MB | decomp.me's compiler archive; three C headers from [gcc-2.95.2](https://ftp.gnu.org/gnu/gcc/) | GPL |
| Unicorn, the CPU emulator that runs Sony's Linux compiler on Windows | period-accurate | 16 MB | PyPI, with pip | GPL 2 |

Sony's compilers are GCC, and decomp.me redistributes them under its licence.
Nothing from them is stored in this repository.

**Using a toolchain you already have.** Pick its folder (the one that holds
`ee\bin`) in **Tools > SDKs > Choose folder**. The builder also finds one
named `ps2sdk-main` next to this folder, or one in the `PS2SDK_HOME` or
`PS2DEV` environment variables. The official ps2dev release (GCC 15) needs
MSYS2's MinGW DLLs on the PATH and is not tested.

### Game files

You must provide your own PlayStation 2 ELF. <br>
In **Set up**:
- **Choose file or disc image...** takes the file itself, or a `.iso` (the
  builder reads the file out of it).
- It is checked against the retail SHA-256 and kept in `bootstrap/baserom/`.
- The build extracts its three data blobs from it, and the decomp's analysis
  tools read it there.

### Command line

```sh
python gt3build.py                    # the profiles, and whether each can build
python gt3build.py bootstrap-fast     # build a profile: its name, short form or unique start
python gt3build.py bootstrap-fast --yes --game-file "D:\GT3 (USA).iso"   # set up without asking
python gt3build.py --sdks             # what was found for each SDK
```

A profile that cannot build yet is set up first. The builder asks before it
downloads (`--yes` answers for you) and asks for a game file it needs
(`--game-file`). The exit status is 0 only when the build worked.

### What the builder writes

| Path | Content |
|---|---|
| `sdk/` | the downloads |
| `output/` | where new profiles put their ELF |
| `backups/` | originals a build replaced |
| `toolchain/profiles.json`, `toolchain/settings.json` | your profiles; this PC's choices (a chosen toolchain folder, the window size) |
| `bootstrap/baserom/`, `bootstrap/data/` | your game file, and the data extracted from it |
| `bootstrap/build/` | build files |

## Repository layout

| Path | Content |
|---|---|
| `GT3Builder.pyw`, `gt3build.py` | The builder: window and command line |
| `toolchain/builder/` | Its code: profiles (`config.py`), plug-ins (`registry.py`, `model.py`), downloads (`download.py`), builds (`runner.py`), the window (`gui.py`) |
| `toolchain/targets/` | One plug-in per executable |
| `toolchain/sdks/` | One plug-in per compiler or toolchain |
| `bootstrap/` | The bootstrap decomp. [README](bootstrap/README.md): what the bootstrap does and how it was decompiled. [MATCHING.md](bootstrap/MATCHING.md): the byte-matching work, its tools and compiler notes. |

## Adding an executable or a compiler

Executables and compilers are plug-ins: one Python file each, loaded when the
builder starts.

1. Copy `toolchain/targets/_template.py` or `toolchain/sdks/_template.py`
   under a new name. Files starting with `_` are not loaded.
2. Fill it in and restart the builder.

A new executable then appears under **What to build**, with its own compiler
options.

**A target** (`targets/*.py`) defines:
- `options`: the compiler choices. Each is an `Option` naming the SDK whose
  name it shows, plus any other SDKs that build uses. An SDK's own
  `requires` are added automatically.
- `game_files`: files from the user's disc, each with its SHA-256 and its
  path on the disc.
- `command()`: the build command, which runs in the project folder.
- `artifact()`: where that command leaves the ELF.

The build command can be anything: a project's own `build.py`, `make`, a
batch file. It must exit with 0 only when the ELF is written. Lines like
`[12/53] src/file.cpp` move the progress bar, and `== link` names a stage.
Lines with `error:` or `warning:` are coloured.

**An SDK** (`sdks/*.py`) defines:
- `detect()`: whether it is installed, and where;
- `downloads`: each URL with its SHA-256;
- `unpack()`: what goes where under `sdk/<home>`. `builder/download.py` has
  the zip and tar helpers.

Installs go through a staging folder, so a failed or cancelled download
leaves nothing half-installed.

## Legal

This project is not affiliated with Sony Interactive Entertainment or
Polyphony Digital. The repository holds no data from the game: the
decompiled code is new source written to behave like the original, and the
build takes its data from your own copy of the game. Compilers and
toolchains are downloaded from their public sources when you ask for them.
