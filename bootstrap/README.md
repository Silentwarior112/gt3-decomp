# GT3 US bootstrap decompilation (`SCUS_971.02`)

This tree holds C and C++ for all 881 functions of the GT3 US boot loader,
`SCUS_971.02` on the Gran Turismo 3 A-Spec US disc (SCUS-97102). The target is
PS2 EE (R5900, little-endian): one `PT_LOAD` at `0x01000000`, entry
`0x01000008`, `$gp` = `0x01049CF0`. Building it and the downloads it needs are
in the [repository's README](../README.md); this file is about the decomp.

The decompilation is **semantically faithful, and largely byte-matching**:
- **The retail compiler.** It was identified as SCE ee-gcc 2.9-ee-991111b
  (`sdk/ee-gcc-2.9-991111b`, downloaded by the builder). The matching build
  uses a copy with one retail behaviour restored: `div.s` never goes in a
  delay slot (`tools/patch_divs.py`). Sony's next revision, 2.9-ee-991111-01,
  has that behaviour built in and is most likely the exact retail compiler.
  It is the builder's period-accurate compiler, and it confirms source
  accuracy (`tools/score.py --cc 991111-01`).
- **Matching score.** Compiled with that compiler and the per-file flags in
  `ref/compile_flags.json`:
  - 851 of 881 functions are byte-exact (843 also with every data reference
    exact);
  - 99.37% of the code bytes match;
  - function sizes agree to 99.98%.
  - The remaining 21 compiled functions are listed with what decides each
    in MATCHING.md, "Open functions"; the bootstrap is considered done.
- **The guide.** [MATCHING.md](MATCHING.md) has the tools, the rules and the
  compiler notes.
- **Behaviour first.** Behaviour comes before bytes. Every function must make
  the same calls, constants, stores, signedness and side effects as the machine
  code, and where the source and the disassembly disagree, the disassembly wins.
- **Behaviour testing.** `tools/difftest.py` runs each function in an EE
  interpreter against the original machine code. It covers 723 of the 881
  functions, including hardware-register, DMA and IOP-RPC paths.

**It builds and boots.** The GT3 Decomp Builder, or `python tools/build.py`,
compiles the whole tree with EE GCC 14.2 and links it into a working
`SCUS_971.02`. In PCSX2, against the retail US disc, the rebuilt bootstrap
shows the warning screen, loads and verifies `core.gt3`, and hands over to it;
GT3 runs to its intro. The period-accurate build (Sony's compiler) boots too.
See [Building SCUS_971.02](#building-scus_97102).

**The retail file.** The tools compare against your own retail `SCUS_971.02`,
kept at `baserom/SCUS_971.02` (file offset = va − `0x01000000` + `0x1000`).
The repository includes no part of it: the builder copies it there from your
file or disc image, and the build extracts its three data blobs from it.

## The signature check in short

**The bootstrap does check `core.gt3`'s signature, and it uses the modulus
carried in the file.** The only constant in the check is the exponent, 66001.
A modified `core.gt3` re-signed with a fresh key at e = 66001 (for example
with GT3Hooks' `gt3pack.py repack ... --resign 66001`) passes the check. No
bootstrap patch is needed for the signature. Three independent reviews
confirmed this. One of them ran the bootstrap's own verify machine code in an
emulator. What is still missing is a PCSX2 boot of a re-signed `core.gt3`
through the bootstrap. See [the signature check](#the-coregt3-signature-check)
below.

## Boot flow

`_start` → `main` ([src/main.cpp](src/main.cpp), `0x01000538`):

| Step | Function | Address | File |
|---|---|---|---|
| Clear .sbss/.bss, InitMainThread, InitHeap(`_end`, -1), `_InitSys`, FlushCache(0), `ei`, call `main` | `_start` | `0x01000008` | [src/crt0.S](src/crt0.S) |
| Run static constructors: EH-frame registration, allocator, filename | `__main` → `__do_global_ctors` | `0x010170E0` → `0x01017030` | [runtime/libgcc2.c](runtime/libgcc2.c) |
| GS init (2 = NTSC) | `gfx_init_default(2)` | `0x0100C478` | [src/gs_driver.cpp](src/gs_driver.cpp) |
| Inflate the embedded `startupa.img` and start the warning-screen thread (one image, 151 frames) | `startup_screen_start` | `0x01000A50` | [src/startup_screen.cpp](src/startup_screen.cpp) |
| Reboot the IOP with `cdrom0:\IOPRP224.IMG;1` and load sio2man, mtapman, mcman, mcserv and padman from the disc (each retried forever). Pad init. If START is held, set `g_skipMemoryCardBoot` (`0x01041DC0`) = 1 | `boot_iop_reboot_and_pad_init` | `0x01000880` | [src/main_iop.cpp](src/main_iop.cpp) |
| Pick a source and read the whole `core.gt3` into memory | `select_boot_source` | `0x010001C8` | [src/main.cpp](src/main.cpp) |
| Inflate, verify and copy the sections, then return the entry point (0 = rejected) | `ImageLoader::load` | `0x010049C8` | [src/imageloader.cpp](src/imageloader.cpp) |
| Free the source and buffer. Wait for the warning screen to finish and stop it. Shut down MC/pad/SIF cmd, then the GS | `startup_screen_wait_thread`, `startup_screen_stop`, `shutdown_iop_services`, `gfx_shutdown` | `0x01000E80`, `0x01000EA8`, `0x010009D8`, `0x0100C750` | [src/startup_screen.cpp](src/startup_screen.cpp), [src/main_iop.cpp](src/main_iop.cpp), [src/gs_driver.cpp](src/gs_driver.cpp) |
| entry == 0: endless loop with no message, no retry and no fallback source | `main` | `0x01000608`..`0x01000628` | [src/main.cpp](src/main.cpp) |
| Remove SBUS handlers 0..15, then `ExecPS2(entry, NULL, 2, {path, "hot", NULL})` (syscall 7) | `exec_ps2_with_sbus_cleanup` → `ExecPS2` stub | `0x01016F70` → `0x0101B830` | [src/execps2.c](src/execps2.c), [sdk/libkernl/syscalls.S](sdk/libkernl/syscalls.S) |

Nothing else in the binary calls syscall 7. `LoadExecPS2` (`0x0101B820`) has no
references.

### Boot source selection (`select_boot_source`, `0x010001C8`)

| Record | Probe | File | Probed when |
|---|---|---|---|
| 0 DISK | `DiskSource::getLoadFlags` `0x010051F8` | `cdrom0:\CORE.GT3;1` | always |
| 1 MCARD 0 | `MCardSource::getLoadFlags` `0x01005748` (port 0, slot 0) | `/BASCUS-97102GAMEDATA/core.gt3` | START not held; the card is a formatted PS2 card |
| 2 MCARD 1 | same, port 1 | same | same |
| 3 HOST | never | (`host:/tmp/core.gt3`) | never: `HostSource` is dead code, nothing constructs it |

- **The key.** Each probe reads the file's first two bytes as a u16 LE key.
  GT3Hooks' notes (FINDINGS) call it `loadSourceFlags`; the retail value is
  `0x0101`.
  - The highest key wins. On a tie the later record wins, so the order is
    MCARD 1 > MCARD 0 > DISK.
  - The key is only a priority. It is not per-source flags, and the signature
    does not cover it.
- **Reading and retrying.** The winning source's `open()` reads the whole file
  into its buffer ([src/imageloader_source.cpp](src/imageloader_source.cpp)).
  - If the open fails, the loop probes again and never gives up.
  - A record's key is overwritten only when a probe hits; it is never cleared.
- **argv[0].** `core.gt3` receives the winner's path as argv[0]. The memory-card
  path has no `mc0:` prefix.
- **Derived from the code, not tested:** a `core.gt3` in
  `BASCUS-97102GAMEDATA` on a memory card whose key is ≥ the disc's is loaded
  instead of the disc's copy. Holding START at boot forces the disc. What
  `core.gt3` does with a memory-card argv[0] is outside this decomp.

## The `core.gt3` signature check

Code: `ImageLoader::decompress` `0x01004248`, `sha512_digest_to_bignum`
`0x010043A8`, `ImageLoader::verify` `0x010044A8` and `ImageLoader::load`
`0x010049C8`, all in [src/imageloader.cpp](src/imageloader.cpp).

The bignum library is in [src/bignum.cpp](src/bignum.cpp) and
[src/montgomery.cpp](src/montgomery.cpp). SHA-512 is in
[src/sha512.cpp](src/sha512.cpp). `modpow` `0x0100B238` is in
[src/gzip_montgomery_tail.cpp](src/gzip_montgomery_tail.cpp).

### Container as the bootstrap parses it

| Where | Field | Covered by the hash |
|---|---|---|
| file+0 | u16 LE source key | no |
| file+2 | u32 LE `rawSize`: size of the zero-filled output buffer, and the end of the hashed range | only through the hash length |
| file+6 | raw DEFLATE stream, inflated into the buffer; the result and the inflated length are never checked | — |
| body+0 | u16 LE `modLen` (unsigned) | no |
| body+2 | modulus N, `modLen` bytes, big-endian | no |
| body+2+modLen | u16 LE `sigLen` (unsigned) | no |
| body+4+modLen | signature S, `sigLen` bytes, big-endian | no |
| body+4+modLen+sigLen | payload: u32 nSection, u32 entry, then nSection × {u32 target, u32 size, data} | yes, up to `rawSize` |

### What `verify` computes

```c
decompress();                                    // inflate file+6 into rawSize zeroed bytes (once)
S = BigNumRep(signature, sigLen);                // big-endian import, 0x01007190
E = BigNum(66001);                               // lui 1 / ori 0x1D1 at 0x01004554 / 0x0100455C
N = BigNumRep(modulus, modLen);                  // big-endian import, from the file
M = modpow(S, E, N);                             // 0x0100B238: Montgomery ctor 0x01009128, powmod 0x0100A5D0
D = BigNumRep(SHA512(payload, rawSize - hashStart), 64);   // standard SHA-512, big-endian
return cmp(M, D) == 0;                           // 0x01007408: full signed bignum equality
```

- **Exponent.** The immediate 66001 (`0x101D1`). The `movn` to −66001 at
  `0x01004590` is dead code: the new bignum's sign is always 0 there.
- **Modulus.** Taken from the file's first field and never compared with
  anything. None of the ~125 functions reachable from `load` references a
  key-shaped constant, and the bignum and Montgomery code reads no data at all.
- **Compare.** The whole reduced result must equal the 512-bit digest, so its
  upper bits must be zero. There is no PKCS#1 or other padding check and no
  length check on `modLen` or `sigLen`.
- **On failure.** `load` returns 0 (`0x010049F8`..`0x01004A04`) and `main` spins
  forever. On success, `load` memcpys every section to its raw target address
  (no range checks) and returns the entry point.
- **One path for every source.** The check does not depend on the source.
  `load` and `verify` read only `src->buffer`, and each of `verify`, `load` and
  `modpow` has exactly one caller. The data checked is the data copied, so the
  bytes cannot be swapped between the check and the copy.
- **Other hashes.** The SHA-512 functions have no other callers, so the
  bootstrap does not hash or sign-check the IOPRP image or the IRX modules.

### Verdict and evidence

**A re-signed `core.gt3` with a fresh key and e = 66001 passes the bootstrap's
check.** All three independent reviews returned "confirmed":

| Review | Method | Result |
|---|---|---|
| 1 | Traced the modulus pointer from the file field to the Montgomery context (`0x010042EC` → `0x01004604` → `0x0100B25C` → `0x01009200`). Scanned the verify closure for data references. Searched the binary for the retail US modulus in 4 encodings, the US signature and the EU modulus. Built its own Python model of the machine code, not taken from gt3pack. | No key found in code or bytes. In the model: retail passes; one flipped segment byte fails; the same tampered body re-signed with a fresh 1024-bit key at 66001 passes; the same key signed for 65537 fails; S for D + 2^600 fails (gt3pack's masked check would pass it) |
| 2 | Wrote its own `.text` reference and entropy scanner (435 referenced data addresses; every high-entropy region identified). Ran 230 probes: moduli, signatures, and SHA-512/SHA-256/SHA-1/MD5 of N in several byte orders, across SCUS-97102, SCES-50294 and SCES-50858. | 0 hits. No key material. Static analysis only |
| 3 | Wrote an independent Python verifier and ran it on files built with `gt3pack.py`. Ran the **bootstrap's own** `verify` code (bignum import, SHA-512, Montgomery, compare) in GT3Hooks' EE interpreter (its `docs/gearbox/harness/PDIFF_emu.py`), with only malloc/free/memcpy/memset hooked. | Retail: accepted (126,225,272 instructions over the full `0x2531F3`-byte region). Fresh key at 66001: accepted. One signed byte flipped: rejected. Signature kept but modulus swapped: rejected |

Caveats:

- **Not booted end to end.** No re-signed `core.gt3` has yet been booted in
  PCSX2 through this bootstrap. That is the remaining confirmation.
- **Emulator coverage.** The emulator runs skipped `decompress` (loader +8 was
  pre-set), and the synthetic-key runs hashed a 512-byte region. So the
  bootstrap's inflater (a port of gzip-1.2.4) was not run on a stream produced
  by gt3pack/zlib.
- **Montgomery arithmetic.** Reviews 1 and 2 did not trace `0x01009F68`,
  `0x01006200` or `0x010083D0` instruction by instruction. Review 3 executed
  that code.
- **US only.** The EU bootstrap is not decompiled. The EU core verifies with
  e = 67001 (GT3Hooks' FINDINGS §9, and review 1's model), but the EU immediate was not
  read from the EU bootstrap.
- **Key requirements.**
  - N must be odd, for the Montgomery setup.
  - N must be larger than the digest.
  - 66001 = 13 × 5077, so gcd(66001, p−1) = gcd(66001, q−1) = 1. `gt3pack`'s
    `resign` handles this by retrying until `pow(e, -1, phi)` exists.
  - The code does not fix the key length.
- **Container requirements.** `rawSize` must equal the inflated body length;
  `gt3pack` writes it that way. If `rawSize` is larger, zero bytes get hashed.
  If it is smaller, inflate overruns the heap block. An entry point of 0 counts
  as a rejection.
- **Scope.** This decomp covers the bootstrap only. It says nothing about any
  check inside `core.gt3` itself.
- **Side note, derived from the code and untested.** Nothing requires N to be
  an RSA modulus. For an even digest D, N = D + 1 with S = D passes, because
  (−1)^66001 ≡ −1 (mod N).

Corrections to GT3Hooks' FINDINGS §9 and its `gt3pack.py`:

- The compare is full integer equality, not "low 512 bits". `gt3pack.py`'s
  `verify()` masks with `% (1 << 512)` and can report a pass that the bootstrap
  rejects. Its `resign` output gives exact equality, so real re-signed files are
  unaffected.
- The field lengths are unsigned u16, not i16.
- `loadSourceFlags` is a priority key compared across sources, and it is not
  signed.
- `decompress` runs **before** the check, on unauthenticated data, with no
  output bound. A malformed stream can therefore overrun the heap before
  verification. This was not explored further.

## Module map

Address ranges are end-exclusive and come from the FUNC markers in each file
checked against `ref/funcs.json`. The 23 units in [ref/units.json](ref/units.json)
group these files by address range. Unit names are what `--unit` takes.

| File | Contents | Address range(s) | Functions |
|---|---|---|---|
| [src/crt0.S](src/crt0.S) | Sony SDK crt0: `_start`, `_exit`, `_root` | `01000000..010000C8` | 4 |
| [src/crtbegin.c](src/crtbegin.c) | collect2-generated ctor/dtor lists, `reg_frame` / `dereg_frame` | `010000C8..01000140` | 2 |
| [src/main.cpp](src/main.cpp) | `SourceRecord`, `select_boot_source`, `main` | `01000140..010006B0` | 5 |
| [src/main_iop.cpp](src/main_iop.cpp) | IRX loader, IOP reboot, pad init / START check, IOP shutdown | `01000790..01000A50` | 4 |
| [src/startup_screen.cpp](src/startup_screen.cpp) | warning-screen thread | `01000A50..01000ED8` | 6 |
| [src/imageloader.cpp](src/imageloader.cpp) | `Buffer`, `MCardSource_2<>` instances, `decompress` / `verify` / `load`, digest helper, `bignum_lock_stub` | `010006B0..01000790`, `01000ED8..010010F0`, `010041C0..01004B08`, `01004BB8..01004BF8` | 19 |
| [src/imageloader_source.cpp](src/imageloader_source.cpp) | Host / Disk / MCard sources (probe and `open`), `host:` file helpers | `01004BF8..01005BC0` | 23 |
| [src/sha512.cpp](src/sha512.cpp) | SHA-512 (K table at `0x0103DF18`) | `010010F0..01003570` | 10 |
| [src/filename.cpp](src/filename.cpp) | `FileName_Base`, Host / MCard / Disk path builders, static ctor/dtor | `01003570..010041C0` | 41 |
| [src/bignum.cpp](src/bignum.cpp) | refcounted bignums: BE import, compare, add/sub/mul, division cores, gcd, modInverse | `01005BC0..010085D0` | 26 |
| [src/montgomery.cpp](src/montgomery.cpp) | divmod, shifts, Montgomery ctor / mulmod / powmod | `010085D0..0100B238` | 10 |
| [src/gzip_montgomery_tail.cpp](src/gzip_montgomery_tail.cpp) | `modpow` (RSA driver) and two dead weak copies. Montgomery code, even though it sits in the gzip unit's range | `0100B238..0100B2E8` | 3 |
| [src/tex1.cpp](src/tex1.cpp) | Tex1 texture-set relocate and upload; dead CLUT animation | `0100B2E8..0100BE10` | 10 |
| [src/gs_texture.cpp](src/gs_texture.cpp) | GS TEX0/TEX1 register setup (`gfx_set_texture_regs`); its own original object | `0100BE10..0100BF98` | 1 |
| [src/gs_driver.cpp](src/gs_driver.cpp) | GS / VIF / GIF driver, DMA, vsync, VU microcode upload | `0100BF98..0100EE48` | 70 |
| [src/gs_state.cpp](src/gs_state.cpp) | dirty-register collection, material state; its own original object | `0100EE48..0100F130` | 4 |
| [src/mcard_io.cpp](src/mcard_io.cpp) | blocking libmc wrappers (`mc_*_sync`) | `0100F130..0100F538` | 14 |
| [src/thread_util.cpp](src/thread_util.cpp) | semaphore and thread helpers, busy wait | `0100F5D0..0100F6D0`, `0100F8D8..0100F918` | 4 |
| [src/hostfs_io.cpp](src/hostfs_io.cpp) | lock-protected `host:` fileio wrappers | `0100F6D0..0100F8D8` | 6 |
| [src/gzip_inflate.cpp](src/gzip_inflate.cpp) | `GZIP::InflateBase` (gzip-1.2.4 inflate port), crc32 | `01011700..01012F38` | 13 |
| [src/execps2.c](src/execps2.c) | `exec_ps2_with_sbus_cleanup` | `01016F70..01016FD8` | 1 |
| [src/pdistd/allocator.cpp](src/pdistd/allocator.cpp) | `pdistd::Allocator`, malloc / free / realloc / memalign | `0100F918..01010278`, `010102D8..01010318`, `01011210..010114A8` | 31 |
| [src/pdistd/printformat.cpp](src/pdistd/printformat.cpp) | pdistd `PrintFormat` engine (`PrintFormatTargetBase`) | `01010318..01011210` | 5 |
| [src/pdistd/printf.cpp](src/pdistd/printf.cpp) | printf.cxx: `PrintTarget`, printf / vprintf front ends | `010114A8..01011700` | 7 |
| [src/pdistd/printf_tables.cpp](src/pdistd/printf_tables.cpp) | the engine's power-of-ten and digit tables (a data-only object) | — | 0 |
| [src/pdistd/pdistd_misc.cpp](src/pdistd/pdistd_misc.cpp) | `intr_suspend`, global lock, break stubs, `tty_puts` | `01012F38..01012FD8` | 6 |
| [include/imageloader.h](include/imageloader.h) | class layouts; inline `Source` accessors emitted out of line | `01004B08..01004BB8` | 10 |
| [include/pdistd/allocator.h](include/pdistd/allocator.h) | inline `Allocator` members emitted out of line | `01010278..010102D8` | 2 |
| [sdk/libcdvd/libcdvd.c](sdk/libcdvd/libcdvd.c) | libcdvd 2240 client | `01012FD8..01014410` | 21 |
| [sdk/libpad/libpad.c](sdk/libpad/libpad.c) | libpad 2200 | `01014410..010157B8` | 31 |
| [sdk/libmc/libmc.c](sdk/libmc/libmc.c) | libmc 2240 | `010157B8..01016F70` | 26 |
| [sdk/libkernl/syscalls.S](sdk/libkernl/syscalls.S) | EE syscall stub table (Exit #4 onward, ExecPS2, ...) | `0101B800..0101C050` | 99 |
| [sdk/libkernl/kernel.c](sdk/libkernl/kernel.c) | cache ops, thread server, INTC/DMAC, alarms, kernel printf | `0101C050..0101D308` | 32 |
| [sdk/libkernl/sifcmd.c](sdk/libkernl/sifcmd.c) | SIF command layer, RPC init | `0101D308..0101E0C0` | 27 |
| [sdk/libkernl/sifrpc.c](sdk/libkernl/sifrpc.c) | SIF RPC client (bind / call) | `0101E0C0..0101E9B0` | 11 |
| [sdk/libkernl/fileio.c](sdk/libkernl/fileio.c) | FILEIO RPC client, part 1 | `0101E9B0..01020978` | 23 |
| [sdk/libkernl/filedev.c](sdk/libkernl/filedev.c) | FILEIO RPC client, part 2 | `01020978..01021F20` | 15 |
| [sdk/libkernl/loadfile.c](sdk/libkernl/loadfile.c) | LOADFILE RPC (`sceSifLoadModule`) | `01021F20..010228B8` | 14 |
| [sdk/libkernl/iopreboot.c](sdk/libkernl/iopreboot.c) | `sceSifRebootIop`, `sceSifSyncIop` | `010228B8..01022B68` | 4 |
| [sdk/libkernl/initsys.c](sdk/libkernl/initsys.c) | `_InitSys`, DI/EI, newlib glue | `01022B68..01022F78` | 21 |
| [sdk/libkernl/deci2_tty.c](sdk/libkernl/deci2_tty.c) | DECI2 glue, TTY driver | `01022F78..01023678` | 17 |
| [sdk/libgraph/libgraph.c](sdk/libgraph/libgraph.c) | `sceGsGetGParam`, `sceGsResetGraph` | `010269E8..01026B88` | 2 |
| [runtime/libc.c](runtime/libc.c) | MMI strlen, mem* / str*, abort, `__errno` | `0100F538..0100F5D0`, `0101AFF0..0101B7C0`, `01026940..010269E8` | 11 |
| [runtime/libc_syscall_stubs.S](runtime/libc_syscall_stubs.S) | libkernl syscall stubs #0..#3 | `0101B7C0..0101B800` | 4 |
| [runtime/libgcc2.c](runtime/libgcc2.c) | `__main`, global ctors/dtors, `__muldi3` | `01016FD8..01017100`, `01018070..010180D8` | 5 |
| [runtime/eh.c](runtime/eh.c) | sjlj EH runtime (`__throw`, `__sjthrow`, eh context) | `01017100..01018070` | 24 |
| [runtime/exception.cc](runtime/exception.cc) | terminate / unexpected, `cp_eh_info`, bad_cast / bad_typeid | `010180D8..01018830` | 25 |
| [runtime/frame.c](runtime/frame.c) | DWARF2 unwind registry, CFA interpreter | `01018830..01019860` | 18 |
| [runtime/new.cc](runtime/new.cc) | operator new / delete, `bad_alloc` | `01019860..01019BA0` | 10 |
| [runtime/tinfo.cc](runtime/tinfo.cc) | `type_info`, `__rtti_*` builders | `01019BA0..0101A270` | 27 |
| [runtime/tinfo2.cc](runtime/tinfo2.cc) | RTTI matchers, `__dynamic_cast`, builtin typeinfos | `0101A270..0101AFF0` | 41 |
| [runtime/libgcc_math.c](runtime/libgcc_math.c) | 64-bit div/mod, fp-bit soft float/double | `01023678..01026940` | 36 |
| **Total** | | `01000000..01026B88` | **881** |

Blobs in [data/](data/) are written by `tools/extract_startup_img.py` and
`tools/gs_extract.py`:

- `startupa_img.gz`: the embedded gzip at `0x01027F38`.
- `gs_vu0_microcode.bin`
- `gs_vu1_microcode_scrambled.bin`: exactly as stored in the ELF.
- `startupa_tex1.bin` and `gs_vu1_microcode.bin`: reference-only decoded
  forms, not present in the ELF.

They come from the retail file, so the repository does not include them:
`tools/build.py` runs the two scripts when the blobs are missing.

## Tooling

Run everything from `bootstrap/`. The tools find what is not in the source
tree through [tools/paths.py](tools/paths.py): the retail file in `baserom/`,
the compilers in the repository's `sdk/` folder, and this PC's PCSX2 and disc
image paths in `local_paths.json` at the repository root (not in the
repository).

**Regenerate `ref/`** (Python 3 with `capstone`). `ref/` is generated output
and read-only; regenerating it never touches `src/`. The repository carries
only its tables and notes (`funcs.json`, `symbols.csv`, `units.json`,
`compile_flags.json`, ...); the disassembly listings (`full.txt`, `chunks/`,
`units/*.txt`, `datamap.txt`) are made from your retail file:

1. `python tools/boot_ground.py` writes `full.txt`, `funcs.json` / `funcs.csv`,
   `datamap.txt`, `chunks/` and `boundary_violations.txt` (this must be empty).
   It adds the function starts listed in `ref/extra_starts.txt`.
2. `python tools/gen_callgraph.py` writes `callgraph.txt`.
3. `python tools/merge_map.py <journal.jsonl>` wrote `symbols.csv` and
   `map_notes.md` from the journal of the mapping pass, an agent workflow that
   is not part of the repository. Functions with no mapping result got
   `SPLIT-OFF` (boundary fix) or `UNMAPPED` rows.
4. `python tools/extract_fixes.py` validates the boundary fixes in
   `map_notes.md` against the disassembly and writes `extra_starts.txt`.
   Re-run steps 1–3 after it.
5. `python tools/make_units.py` writes `ref/units/<unit>.txt` and
   `<unit>_symbols.csv` from the hand-maintained `ref/units.json`. It fails if
   any function is in zero units or in two.

`tools/elfinfo.py` dumps the ELF headers.

**Compile check:**

```sh
sh tools/cc.sh --all
sh tools/cc.sh src/imageloader.cpp
```

`cc.sh` uses the EE toolchain (`mips64r5900el-ps2-elf-gcc`/`g++`, GCC 14.2)
from the repository's `sdk/ps2sdk`, or `ps2sdk-main` next to the repository,
or `$PS2SDK_HOME` / `$PS2DEV`. It compiles with
`-O2 -G0 -Wall`: `-std=gnu99` for C, and `-std=gnu++14 -fpermissive
-fno-sized-deallocation` for C++. `-fno-builtin-printf` keeps `printf` from
being turned into `puts`, which this binary does not have. Objects go to
`build/`, which is disposable.

**Coverage check:** `python tools/check_coverage.py`, or add `--unit <name>`.
Every function in `ref/funcs.json` must be claimed exactly once. Coverage is
counted by address range, so a function written in a different file still
counts for the unit whose range holds it.

**Conventions** (full text in [ref/DECOMP_RULES.md](ref/DECOMP_RULES.md), with
background in [ref/CONTEXT.md](ref/CONTEXT.md)):

- One `// FUNC 0x01XXXXXX name` line directly above each function, with the
  address exactly as in `funcs.json`. Compiler-generated `__tf*` functions and
  thunks get the marker plus a comment block.
- `/* ? */` marks an inferred name, type or shape.
- Quirks and bugs are reproduced and commented, not fixed.
- Original vtable addresses and slots, struct offsets and EH landing pads are
  documented in comments.
- The names in `ref/symbols.csv` come from the mapping pass. The FUNC names
  replace them: for example, `0x0100B238` is `modpow`, not
  `GZIP::InflateBase::inflate`.

**Matching and behaviour tools.** [MATCHING.md](MATCHING.md) has the details.

| Tool | Purpose |
|---|---|
| `tools/mcc.py` | Compile one file with the retail compiler, using its rule from `ref/compile_flags.json` and the retail assemblers' passes: `div.s` padding everywhere, and the Sony SDK assembler's delay-slot swap from `tools/sdkas.py` for `sdk/*`. |
| `tools/score.py [files]` | Per-function bytes / size / exact and data-reference checks. With no arguments it writes `build/score/report.md` and `score.json`. `--diff <addr\|name>` shows the C side by side with retail. |
| `tools/score.py --cc 991111-01 [files]` | The same with the reference compiler, Sony's Linux ee-gcc 2.9-ee-991111-01 (`sdk/ee-gcc-2.9-991111-01`), into `build/score-991111-01/`; lists the functions whose result differs from the default compiler's. Confirms source accuracy. Needs `pip install unicorn`. `tools/mcc.py --cc 991111-01` compiles one file. |
| `tools/linuxcc.py` | Runs the reference compiler's i386 Linux `cc1`/`cc1plus` on Windows: Unicorn for the CPU, an ELF loader and the Linux system calls they use, and the glibc 2.3.6 the compiler package came with. Called by `mcc.py`. |
| `tools/patch_divs.py` | Makes `sdk/ee-gcc-2.9-991111b-divs`, the matching compiler, from the pristine package (needs `capstone`). |
| `tools/difftest.py` | Function-level behaviour test against the retail code in `tools/eeemu.py`. Use `--group` / `--fn` to select, `--list` for coverage. Specs live in `tools/difftest_specs/<group>.py`. |
| `tools/layout.py [files]` | Infers the original translation units: retail objects, data-section bases, EH frame blocks. |
| `tools/build_matching.py` | Rebuilds the byte-identical image from the splat split in `matching/`. |
| `tools/build.py --compiler sce` | The behaviour build compiled with the retail compiler (`build/sce/`). `--compiler 991111-01` does the same with the reference compiler (`build/991111-01/`); the GT3 Decomp Builder's period-accurate option runs it. |

## Building SCUS_971.02

```sh
python tools/build.py                       # build, check, link: build/scus_971.02
python tools/build.py --out D:/gt3/SCUS_971.02     # ... and copy it there
python tools/build.py --compiler 991111-01  # Sony's compiler, into build/991111-01/
python tools/build.py --toolchain D:/ps2sdk-main   # another EE toolchain
python tools/boot_test.py                   # boot build/scus_971.02 in PCSX2
python tools/boot_test.py --elf D:/gt3/SCUS_971.02
python tools/boot_test.py --baseline        # the retail bootstrap, for comparison
python tools/boot_test.py --baseline --card ../core/build/packtest/T1_resigned.ps2   # core.gt3 from a card
```

The GT3 Decomp Builder runs the same script from a window with saved
profiles ([README](../README.md)).

[tools/build.py](tools/build.py) runs these stages and stops at the first
failure:

0. **Assets.** When `data/` lacks the three blobs `link/blobs.S` includes, it
   extracts them from the retail file.
1. **Compile** every `.c/.cpp/.cc/.S` under `src/`, `sdk/`, `runtime/` and
   `link/` with the EE toolchain (GCC 14.2, n32 ABI; `--toolchain` picks one,
   else `tools/paths.py` finds it), one file per CPU at a time (`--jobs`). A
   full build takes about 2 s.
   Flags: `-O2 -G0 -ffreestanding -fno-builtin -fno-strict-aliasing
   -fwrapv -fno-math-errno`, and for C++ also `-fno-exceptions -fno-rtti
   -fno-threadsafe-statics -fno-use-cxa-atexit -fno-sized-deallocation`. The
   result needs nothing from the toolchain's libraries.
2. **Check** the objects. It refuses any object with `.ctors`/`.init_array`,
   because the decompiled crtbegin runs the constructor lists explicitly. It
   also refuses a relocation-free `lui`/low-half pair whose value lands in the
   retail image's data or on a retail function start, because that means a
   hard-coded address.
3. **Link** with [link/bootstrap.ld](link/bootstrap.ld): one `PT_LOAD` at
   `0x01000000`, crt0 first so `_start` is `0x01000008` as in retail, `_fbss`
   and `_end` for crt0's `.bss` clear, and the 1 MB stack at `0x01F00000`. The
   map goes to `build/scus_971.02.map`.
4. **Finish.** Strip the ELF, then run `tools/align_segments.py` (from
   GT3Hooks), which aligns the segment for the BIOS loader and replays
   XLOADFILE's load at every buffer size. It also checks the entry point, that
   there is a single loadable segment, and that nothing overlaps `core.gt3`'s
   load area or the stack.
5. **Install**, only with `--out`: the ELF is copied there. The first time
   that replaces the retail file (by SHA-256), it is copied to
   `build/backup/` first. Debug builds (`--trace`, `--opt`, `--opt-file`) are
   never copied.

The GCC 14 build is 322,836 bytes against the retail 274,836, so rebuild the
disc image with its layout adjusted if `SCUS_971.02` no longer fits before
`core.gt3`'s LBA. The SCE build (`--compiler sce`, the retail compiler) is
275,136 bytes and boots too, and so does the reference compiler's
(`--compiler 991111-01`, 275,136 bytes: all six boot stages).

**Build glue** lives in [link/](link/), apart from the decompiled code:

- `blobs.S` `.incbin`s `data/startupa_img.gz`, the scrambled VU1 microcode and
  the VU0 microcode. Each was checked byte-identical to the retail image.
- `glue.c` defines the 36 `eh_frame_*` placeholders as zero words, which
  frame.c reads as "no unwind info".
- `bootstrap.ld` points GCC 14's soft-double libcalls (`__muldf3`, `__ltdf2`
  and so on) at the decompiled GOFAST routines (`dpmul`, `dpcmp`, …). The
  original compiler called those directly. Only comparisons whose NaN
  convention matches fp-bit's `compare` are mapped.

**Source changes needed to build.** Each keeps the original behaviour:

- `runtime/exception.cc` and `runtime/new.cc`: `throw`, `catch` and `typeid`
  are written out as the egcs lowering the machine code performs
  (`__eh_alloc`, `__cp_push_exception`, `__throw`), and `typeid` goes through a
  `__tf()` first virtual, as egcs vtable slot 0 does. There is no modern C++
  ABI in the binary, and none in the build. `__check_eh_spec`'s `catch (...)`
  body is kept as a separate function, reached only through the original's
  exception tables, which a modern build does not have.
- `runtime/libc.c`: the `plzcw` model counts leading zeros with a loop instead
  of `__builtin_clz`, which would pull in libgcc's `__clzdi2`.

**Bugs the boot test found.** There were three, all real. Each is fixed in the
source, and each fix keeps what the machine code does:

- `sdk/libkernl/sifcmd.c` `sceSifGetSreg`: `sceSifInitRpc` polls software
  SIF register 0, which the `set_sreg` handler writes in interrupt context. In
  the binary the getter is out of line. GCC 14 inlined it into the wait loop
  and hoisted the load, so `-O2` builds hung right after the IOP reboot. Now a
  volatile access. Found by bisecting per-file optimisation levels
  (`tools/bisect_opt.py`).
- `sdk/libkernl/fileio.c` `_fio_init`: the `FIO_F_SETUP` RPC sends a local
  variable as its SIF DMA source. DMA needs a 16-byte-aligned source, which it
  got in the binary (`sp+0x10`) only by frame layout. An unaligned build sends
  the IOP a wrong completion-block address and hangs on the first `sceOpen`.
  Now `aligned(16)`.
- `runtime/libgcc2.c` `__muldi3`: the 32×32→64 step was a C widening
  multiply. GCC 14 at `-O0` compiles that as a 64-bit multiply, which is a call
  to `__muldi3` itself, so it recursed forever inside RSA verification. Now one
  `multu` in inline asm, as in the binary. A scan of `libgcc2.c` and
  `libgcc_math.c` at `-O0` and `-O2` finds no other helper that can reach
  itself.

**Boot test** ([tools/boot_test.py](tools/boot_test.py)):

- **Setup.** It runs a private portable copy of PCSX2 (tested with 2.6.3) in
  `build/pcsx2-test/`. The copy is made on first use from your PCSX2 folder
  and takes program files only: no memory cards, save states, cheats, logs or
  texture packs, and the achievements login is removed. It reads the BIOS
  that install uses and gets its own blank memory cards. Your PCSX2 folder
  and disc image go in `local_paths.json` at the repository root:
  `{"pcsx2": "C:/PCSX2", "iso": "D:/Gran Turismo 3 - A-Spec (USA).iso"}`,
  optionally with `"bios"`; `--pcsx2` and `--iso` override them. It needs
  Pillow for the window captures.
- **Run.** It boots the ELF as a boot-ELF override against the retail US ISO,
  so the ELF reads `CORE.GT3`, `IOPRP224.IMG` and the IRX modules from the real
  disc. With `--card`, a copy of that PCSX2 memory card is in slot 2 for the
  run, and the bootstrap loads the `core.gt3` on it instead of the disc's
  (`core/tools/packtest.py` makes such cards).
- **Output.** It saves the emulator log and window captures to
  `build/boottest/<tag>/` and checks six stage markers taken from a retail
  run: ELF executing, IOP rebooted, bootstrap IRX loads, the `ExecPS2` kernel
  restart, `core.gt3`'s own IRX loads, and GT3's per-frame `Timezone=` line.

| Run | Stages reached | `ExecPS2` after ELF start |
|---|---|---|
| retail bootstrap | 6/6 | 5.2 s |
| rebuilt, `-O2` (installed file) | 6/6 | 5.2 s |
| rebuilt, everything `-O0` | 6/6 | 5.2 s |

The `-O0` build is a robustness check. Its frame layouts and inlining differ
completely from `-O2`, so it exposes code that works only by luck. The
fileio and `__muldi3` bugs above were found with it.

The warning-screen captures of the two runs are identical, and GT3 reaches its
Polyphony Digital intro.

**Debug aids:**

- `python tools/build.py --trace` wraps the functions listed in
  `tools/trace/trace_syms.txt` using `ld --wrap`. It logs each entry (first
  three arguments) and return value to the EE serial port, which PCSX2 writes
  to the log.
- `--opt` and `--opt-file` set per-file optimisation levels.
- `tools/bisect_opt.py` bisects which files break a boot at a given level.
- `tools/check_alignment.py` compares every annotated global's alignment with
  its retail address.
- `tools/stack_depth.py` gives worst-case stack depth per thread from GCC's
  call-graph info.

## Status and known gaps

**Done:**

- Coverage: 881 of 881 functions claimed, 0 missing, 0 duplicated.
- Compile: `cc.sh --all` (GCC 14) passes 51 of 51 files, and
  `tools/score.py` compiles all of them with the retail compiler. The 5
  warnings are known and come from reproducing the original faithfully:
  - 4 × `-Wmaybe-uninitialized` in `libgcc_math.c` (`dptofp` / `fptodp`);
  - 1 × `-Wmaybe-uninitialized` in `host_read_file`. `got` is unset only on
    the malloc-failure path, where the result is NULL either way, as in the
    original.
- Integration: a symbol check across all 46 objects found no duplicate strong
  definitions and no name or signature mismatches between units. All headers
  compile together.

**How it was checked:**

- **Byte comparison.** `tools/score.py` compiles every file with the retail
  compiler and compares each function with the retail bytes. Data references
  are checked too. See [MATCHING.md](MATCHING.md).
- **Executed against the original machine code.** `tools/difftest.py` and
  `tools/eeemu.py` run each function in an R5900 interpreter on identical
  inputs and compare results, memory writes, stubbed calls, and hardware
  register and DMA events. It covers 723 of 881 functions, and every spec
  passes on the SCE-compiled build of these sources and on the byte-identical
  rebuild. The runtime units are the toolchain's own `libgcc.a`, so they are
  not tested.
- **Adversarial review.** Every matching edit was reviewed by an independent
  verifier against a snapshot of the sources from before matching began.
- **Boot.** Both builds boot to GT3's main loop in PCSX2.

**Builds and boots** (see [Building SCUS_971.02](#building-scus_97102)). The
link-time gaps an earlier revision of this README listed are closed:

- **`eh_frame` labels.** The 36 are zero-word placeholders in `link/glue.c`.
- **Blobs.** They are linked by `link/blobs.S`.
- **Linker-script symbols.** They come from `link/bootstrap.ld`.
- **Modern C++ ABI.** No C++ is compiled with exceptions or RTTI, and the two
  runtime files that used them now spell out the egcs lowering.
- **Soft-double libcalls.** They are aliased to the decompiled GOFAST routines.
- **`__clzdi2`.** It is no longer referenced.
- **Object layout.** Modern g++ puts the vptr at +0, while egcs puts it after
  the data members (for example `Source` at +0xC). The rebuilt image is
  consistent with itself, and a scan found no code that reaches into
  polymorphic objects by raw offset. The original offsets remain documented in
  comments.

**Exceptions in the rebuilt image.** The bootstrap throws nothing on any path
the boot takes. If something did throw (for example `operator new` failing),
the rebuild would reach `terminate()`, where the original would unwind through
its egcs tables. With no handler present, the original also ends in
`terminate()`.
- **Header and struct duplication.** `runtime/new.h` cannot share a
  translation unit with `<new>`. `struct frame_state` and `struct object` are
  each defined twice; the definitions are identical and checked with
  `static_assert`.

**Open items (none changes boot behaviour):**

- **Unknown identities.** `0x010041C0` and `0x010041C8` are empty functions
  whose identity is unknown. The empty stubs at `0x01007140`, `0x010081B0`,
  `0x01008B90` and `0x0100B2E0` are identified as dead copies of
  `bignum_lock_stub` only from their FDE placement.
- **printf.** `%t` with precision ≥ 20 reads the table's trailing zero word
  (`s_pow10_u64[19]`) and never terminates, as the original does.
- **Malformed input and overruns.** Only the SCE-compiled build reproduces
  these. In the GCC 14 build the frame and object layouts differ:
  - gzip: an overrun past `ll[]` beyond its frame padding, stale `huft_build`
    state, arena overflow into the vptr at +0x3030;
  - kernl_sys: `_sif_cmd_intr_handler` overruns its copy when `psize > 0x70`.
- **GCC 14 build only.**
  - Divide-by-zero traps: GCC 14 emits `teq`, while retail emits `break 7` or,
    for some divisions, no check.
  - Code paths this binary never reaches: VU0 scaling becomes an FPU multiply.
  - `max.s` on -0.0 in `clut_anim_step`.
  These are documented in the function comments, and none is reachable on a
  valid boot.
- **Names.** All `/* ? */` names are inferences, and some Sony-internal static
  names are unknown.
- **Units vs. files.** `0x010041C0`, `0x010041C8` and `0x01005B68` sit in the
  filename unit's range but are written in `imageloader*.cpp`.
  `ref/units.json`'s `files` lists are regenerated from the FUNC markers by
  `tools/sync_unit_files.py`.
