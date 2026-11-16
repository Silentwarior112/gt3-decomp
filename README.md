# Gran Turismo 3 Decompilation

Decompiled source code of the Gran Turismo 3 family of executables, and the **GT3
Decomp Builder**, which streamlines the process of compiling the source code. Choose what to build and with which
compiler. The builder downloads that compiler and writes the finished ELF
wherever you want it, such as your extracted disc.

| Executable | Source Code Folder | State |
|---|---|---|
| GT3 US bootstrap, `SCUS_971.02` | [bootstrap/](bootstrap/README.md) | All 881 functions in C/C++. 851 are byte-exact when built with Sony's compiler (99.4% of the code bytes). Builds and boots to the game. |
| GT3 US main executable, `CORE.GT3` | [core/](core/README.md) | All 11,411 functions in C/C++. Compiles with modern GCC and with the original compiler. Built with the original compiler, the C is linked over the original data into a signed `CORE.GT3` that boots to the game's intro; the modern-GCC build checks the C and outputs the original code. |

**No game data is included.** Building needs your own copy of Gran Turismo 3
A-Spec (US, SCUS-97102). The builder asks once for its `SCUS_971.02` and
`CORE.GT3`, or for the disc image they are on.

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
| splat, spimdisasm and rabbitizer, which split the main executable into assembly for its relink | the main executable | 8 MB | PyPI, with pip | MIT |

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
- It is checked against the retail SHA-256 and kept in `bootstrap/baserom/`
  (`SCUS_971.02`) or `core/baserom/` (`CORE.GT3`).
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
| `core/baserom/`, `core/ref/splat/` | your `CORE.GT3`, the program made from it, and its split into assembly |
| `bootstrap/build/`, `core/build/` | build files |

## Repository layout

| Path | Content |
|---|---|
| `GT3Builder.pyw`, `gt3build.py` | The builder: window and command line |
| `toolchain/builder/` | Its code: profiles (`config.py`), plug-ins (`registry.py`, `model.py`), downloads (`download.py`), builds (`runner.py`), the window (`gui.py`) |
| `toolchain/targets/` | One plug-in per executable |
| `toolchain/sdks/` | One plug-in per compiler or toolchain |
| `bootstrap/` | The bootstrap decomp. [README](bootstrap/README.md): what the bootstrap does and how it was decompiled. [MATCHING.md](bootstrap/MATCHING.md): the byte-matching work, its tools and compiler notes. |
| `core/` | The main executable's decomp. [README](core/README.md): how it is cut up, its passes and tools. |

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