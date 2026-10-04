/* main.cpp - main.cxx of the GT3 US bootstrap (BOOTSTRAP_SCUS-97102),
 * .text 0x01000140..0x010006AF (unit "main").
 *
 * The boot-source records, select_boot_source and main.  The same object
 * also holds the MCardSource_<1,0> / <0,0> typeinfo functions and
 * destructors at 0x010006B0..0x0100078F (template instantiations emitted
 * here, marked at the end of this file).
 *
 * Object data (all verified in the ELF):
 *   .rodata 0x0103DE00  "core.gt3"
 *           0x0103DE10 / 0x0103DE30  vtables of MCardSource_<1,0> / <0,0>
 *           0x0103DE50 / 0x0103DE78  their RTTI names
 *   .data   0x01026CC8  g_bootSourceNames[4] (unreferenced)
 *           0x01026CD8  .eh_frame ("eh" CIE -> except table 0x01041000)
 *   .sdata  0x01041D88 / 8C / 90  g_hostCoreFile / g_diskCoreFile /
 *                                 g_mcardCoreFile (all -> "core.gt3")
 *           0x01041D98..0x01041DB7  "DISK", "MCARD 0", "MCARD 1", "HOST"
 *           0x01041DB8  "hot"
 *   .bss    0x01042060  s_execArgv[3]
 *
 * Exceptions: the except-table entries 0x01041000..0x0104106B of this object
 * are all cleanups (destroy the partially built record array, operator
 * delete a source whose constructor threw, destroy the ImageLoader) that end
 * in __throw (0x01017CA8), plus terminate (0x010180D8) guards around the
 * cleanup code itself.  There is no catch clause; plain RAII scopes below
 * reproduce them.
 */
#include "types.h"
#include "libc.h"
#include "cxxrt.h"
#include "eekernel.h"
#include "libcdvd.h"
#include "boot_io.h"
#include "gs_driver.h"
#include "imageloader.h"
#include "startup_screen.h"
#include "main.h"

/* ------------------------------------------------------------------------ */
/* Globals                                                                  */
/* ------------------------------------------------------------------------ */

/* The bare file name each record probes for.  Three separate (non-const)
 * pointer variables, all to the one "core.gt3" literal (.rodata 0x0103DE00):
 * the code loads each from .sdata instead of folding the address.          */
const char *g_hostCoreFile  = "core.gt3";      /* @0x01041D88 (.sdata) */
const char *g_diskCoreFile  = "core.gt3";      /* @0x01041D8C (.sdata) */
const char *g_mcardCoreFile = "core.gt3";      /* @0x01041D90 (.sdata) */

/* Display names of the candidates, in record order.  Nothing references
 * this array (probably left over from debug output that was compiled out). */
const char *g_bootSourceNames[BOOT_SOURCE_COUNT] = {   /* @0x01026CC8 (.data) */
    "DISK", "MCARD 0", "MCARD 1", "HOST"
};

/* argv handed to core.gt3: {device path of the boot source, "hot", NULL}.
 * argv[0] points into main's stack frame.                                  */
static char *s_execArgv[3];                    /* @0x01042060 (.bss) */

/* ------------------------------------------------------------------------ */
/* SourceRecord                                                             */
/* ------------------------------------------------------------------------ */

// FUNC 0x01000140 SourceRecord::~SourceRecord
//   (was source_record_dtor)  `delete source`: if non-NULL, virtual
//   destructor through the Source vtable (vptr at source+0xC, slot 1:
//   lh delta @vt+8, lw pfn @vt+0xC) with flags 3.  Then, GCC 2.9x hidden
//   flags: operator delete(this) (0x01019910) if flags & 1.
//   select_boot_source destroys its records with flags 0.
SourceRecord::~SourceRecord()
{
    delete source;
}

// FUNC 0x010001A0 SourceRecord::SourceRecord
//   (was source_record_init)  loadFlags = 0 (sh), fileName = NULL,
//   source = NULL; returns this (GCC 2.9x constructor convention).  Called
//   4 times by select_boot_source's array construction.
SourceRecord::SourceRecord()
    : loadFlags(0), fileName(NULL), source(NULL)
{
}

// FUNC 0x010001B8 SourceRecord::release
//   (was source_record_release_source)  Returns source and sets it to NULL,
//   so the record's destructor no longer deletes it.
ImageLoader::Source *SourceRecord::release()
{
    ImageLoader::Source *s = source;
    source = NULL;
    return s;
}

/* ------------------------------------------------------------------------ */
/* select_boot_source                                                       */
/* ------------------------------------------------------------------------ */

// FUNC 0x010001C8 select_boot_source
//   Only caller: main.  Builds SourceRecord rec[4] on its stack (sp+0; GCC
//   array construction counting 3..0 with an EH cleanup that destroys the
//   records built so far), names the records, calls sceCdDiskReady(0)
//   (blocking wait for the drive; result ignored) and then loops:
//
//   probe (repeated, without any delay, until some probe hits):
//     unless g_skipMemoryCardBoot (set when START was held at boot):
//       MCARD 0: MCardSource::getLoadFlags(name, 0, 0) (0x01005748); on a
//                non-zero result store it and, if the record has no source
//                yet, new MCardSource_<0,0> (operator new(0x18),
//                MCardSource(0, 0) 0x01005428, vptr 0x0103DE30);
//       MCARD 1: same with port 1: MCardSource_<1,0>, vptr 0x0103DE10;
//     DISK: DiskSource::getLoadFlags(name) (0x010051F8); on a hit store it
//           and create a DiskSource (operator new(0x10), ctor 0x01004EF0).
//     HOST is never probed (its record keeps flags 0 and no source).
//   choose: best = argmax of loadFlags over the 4 records with an unsigned
//     `>=` test, so on equal flags the LATER record wins (MCARD 1 > MCARD 0 >
//     DISK).
//   open: if the best record has a source, call its open(fileName) (vtable
//     slot 3).  On success release() it, destroy all 4 records (deleting
//     the other sources) and return it.  On failure (or no source) call
//     close() (slot 2) if there is a source, yield_low_priority(), and
//     probe again.
//
//   Quirks: a record's loadFlags is only written on a hit, never cleared, so
//   a stale value from an earlier pass (card removed, failed open) still
//   takes part in the next choice as long as any probe hits.  A source whose
//   open() failed is kept and reused.  The loop never gives up.
//   Exception regions: operator delete of a source whose constructor threw
//   (landing pads 0x010004BC / 0x010004CC / 0x010004DC), destruction of the
//   whole array (0x010004EC, 0x0100040C region -> 0x0100050C), all ending in
//   __throw; terminate (0x01000514) guards the cleanups.
//
//   Source shape (reconstructed from the retail code, see MATCHING.md):
//   - each probe is its own `do { ... } while (0)` block: the retail code
//     aligns the first label inside the MCARD 1 and DISK probes to 8 bytes
//     (the nops at 0x010002BC / 0x0100032C), which egcs does for loops only,
//     so the original was probably a macro per probe;
//   - `r` lives in $s2: MCARD 0 reads its file name through it, MCARD 1
//     reads rec[2].fileName directly (sp-relative, loaded before the
//     aligned label);
//   - the MC results are masked (andi 0xffff) but the disk result is not.
//     /* ? */ MCardSource::getLoadFlags probably returned a wider type (int)
//     in the original; with the u16 declaration of imageloader.h those two
//     andi are the only differences.  The value is < 0x10000 either way.
ImageLoader::Source *select_boot_source(void)
{
    SourceRecord rec[BOOT_SOURCE_COUNT];
    bool found;

    rec[BOOT_SOURCE_HOST].fileName   = g_hostCoreFile;
    rec[BOOT_SOURCE_MCARD0].fileName = g_mcardCoreFile;
    rec[BOOT_SOURCE_MCARD1].fileName = g_mcardCoreFile;
    rec[BOOT_SOURCE_DISK].fileName   = g_diskCoreFile;

    sceCdDiskReady(0);

    found = false;
    for (;;) {
        do {
            if (!g_skipMemoryCardBoot) {
                do {
                    SourceRecord *r = &rec[BOOT_SOURCE_MCARD0];
                    u16 flags = ImageLoader::MCardSource::getLoadFlags(r->fileName, 0, 0);
                    if (flags != 0) {
                        found = true;
                        r->loadFlags = flags;
                        if (r->source == NULL)
                            r->source = new ImageLoader::MCardSource_<0u, 0u>();
                    }
                } while (0);
                do {
                    SourceRecord *r = &rec[BOOT_SOURCE_MCARD1];
                    u16 flags = ImageLoader::MCardSource::getLoadFlags(
                                    rec[BOOT_SOURCE_MCARD1].fileName, 1, 0);
                    if (flags != 0) {
                        found = true;
                        r->loadFlags = flags;
                        if (r->source == NULL)
                            r->source = new ImageLoader::MCardSource_<1u, 0u>();
                    }
                } while (0);
            }
            do {
                SourceRecord *r = &rec[BOOT_SOURCE_DISK];
                u16 flags = ImageLoader::DiskSource::getLoadFlags(r->fileName);
                if (flags != 0) {
                    found = true;
                    r->loadFlags = flags;
                    if (r->source == NULL)
                        r->source = new ImageLoader::DiskSource();
                }
            } while (0);
        } while (!found);

        /* sltu + movz: best = i unless rec[i] < rec[best] (ties -> later). */
        unsigned int best = 0;
        for (unsigned int i = 1; i < BOOT_SOURCE_COUNT; i++)
            if (!(rec[i].loadFlags < rec[best].loadFlags))
                best = i;

        if (rec[best].open())
            return rec[best].release();     /* ~rec[3..0] delete the losers */
        rec[best].close();
        yield_low_priority();
        found = false;
    }
}

/* ------------------------------------------------------------------------ */
/* main                                                                     */
/* ------------------------------------------------------------------------ */

// FUNC 0x01000538 main
//   Called by _start with (_args.argc, _args.argv); both ignored.
//    1. __main() (0x010170E0): GCC 2.9x inserts this call at the top of
//       main on targets without an .init section; it runs __CTOR_LIST__.
//       The retail compiler (BOOT_OLD_GCC) still inserts it by itself; for
//       GCC 14, which does not, it is written out explicitly.
//    2. gfx_init_default(2) (0x0100C478, 2 = NTSC), startup_screen_start
//       (0x01000A50: warning screen on its own thread),
//       boot_iop_reboot_and_pad_init (0x01000880).
//    3. src = select_boot_source(); strcpy the source's device path
//       (src->fileName->GetPath(), FileName vtable slot 2, i.e. the inline
//       Source::GetPath) into a 128-byte stack buffer (no bounds check).
//    4. ImageLoader at sp+0x80 {src, Buffer{0,0}} (inline ctor);
//       entry = load() (0x010049C8: decompress, verify, copy the sections;
//       0 = rejected); then the inline ~ImageLoader: delete src (virtual,
//       flags 3) and ~Buffer(&image, 2) (0x01000ED8).  An exception from
//       load() runs the same cleanup (landing pad 0x01000660) and __throw.
//    5. startup_screen_wait_thread (0x01000E80: the screen always finishes
//       its full ~151 frames), startup_screen_stop (0x01000EA8),
//       shutdown_iop_services (0x010009D8), gfx_shutdown (0x0100C750).
//    6. entry == 0 (image rejected): spin forever.  The five nops in the
//       loop body are assembler padding for the R5900 short-loop erratum,
//       the source loop is empty.
//    7. argv = {path, "hot", NULL} in .bss 0x01042060, gfx_wait_vsync_poll
//       (0x0100BF98), exec_ps2_with_sbus_cleanup(entry, 2, argv)
//       (0x01016F70: remove SBUS handlers, ExecPS2).  Returns 0 only if that
//       returned.
int main(int argc, char **argv)
{
    char path[0x80];                /* sp+0x00 */
    u32  entry;

#ifndef BOOT_OLD_GCC
    __main();                       /* egcs emits this call itself */
#endif

    gfx_init_default(2);
    startup_screen_start();
    boot_iop_reboot_and_pad_init();

    ImageLoader::Source *src = select_boot_source();
    strcpy(path, src->GetPath());

    {
        ImageLoader loader(src);    /* sp+0x80 */
        entry = loader.load();
    }

    startup_screen_wait_thread();
    startup_screen_stop();
    shutdown_iop_services();
    gfx_shutdown();

    if (entry == 0)
        for (;;)
            ;

    s_execArgv[0] = path;
    s_execArgv[1] = (char *)"hot";  /* .sdata 0x01041DB8 */
    s_execArgv[2] = NULL;
    gfx_wait_vsync_poll();
    exec_ps2_with_sbus_cleanup((void *)entry, 2, s_execArgv);
    return 0;
}

/* ------------------------------------------------------------------------ */
/* MCardSource_<> template instances (compiler-generated)                   */
/* ------------------------------------------------------------------------ */
/* The typeinfo function and the synthesised virtual destructor of each
 * instance, emitted (weak) with the instance's vtable by the implicit
 * instantiation of `new MCardSource_<0,0>` / `<1,0>` in select_boot_source,
 * after main, as in main.cxx.  The order (<1,0> first) comes from the order
 * of the sizeof checks in imageloader.h.  Their name strings and vtables
 * are this object's .rodata after "core.gt3", as in retail. */

// FUNC 0x010006B0 __tfQ211ImageLoadert12MCardSource_2Ui1Ui0 (compiler-generated)
//   typeinfo function, slot 0 of vtable 0x0103DE10.  If the __si_type_info
//   object at 0x0104DD28 (.bss) is not built yet (first word == 0): call
//   __tfQ211ImageLoader11MCardSource (0x01005900, result ignored), then
//   __rtti_si(&ti, "Q211ImageLoadert12MCardSource_2Ui1Ui0" (.rodata
//   0x0103DE50), &__ti MCardSource @0x0104DD98) (0x01019C38, the tinfo.cc
//   placement builder).  Returns &ti (0x0104DD28).

// FUNC 0x01000700 _$_Q211ImageLoadert12MCardSource_2Ui1Ui0 (compiler-generated)
//   ImageLoader::MCardSource_<1,0>::~MCardSource_, the synthesised virtual
//   dtor (vtable 0x0103DE10 slot 1): forwards (this, flags) unchanged
//   straight to ImageLoader::Source::~Source (0x010041D0) - the implicit
//   (inline) ~MCardSource is folded away - which also does the flags & 1
//   delete.  No vptr store (egcs skips it for an empty destructor).

// FUNC 0x01000720 __tfQ211ImageLoadert12MCardSource_2Ui0Ui0 (compiler-generated)
//   Same as 0x010006B0 for <0,0>: __rtti_si (0x01019C38) on the
//   __si_type_info at 0x0104DD38, name "Q211ImageLoadert12MCardSource_2Ui0Ui0"
//   (.rodata 0x0103DE78), base __ti MCardSource @0x0104DD98 (built first via
//   0x01005900).  Slot 0 of vtable 0x0103DE30.  Returns 0x0104DD38.

// FUNC 0x01000770 _$_Q211ImageLoadert12MCardSource_2Ui0Ui0 (compiler-generated)
//   ImageLoader::MCardSource_<0,0>::~MCardSource_ (vtable 0x0103DE30 slot
//   1): forwards (this, flags) to ImageLoader::Source::~Source (0x010041D0),
//   like 0x01000700.
