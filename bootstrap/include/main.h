/* main.h - the bootstrap's main program (unit "main") of the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102).  C++ only.
 *
 * Files of the unit and the original objects they stand for (object
 * boundaries from the .eh_frame CIEs, see src/crtbegin.c):
 *
 *   src/crt0.S      0x01000000..0x010000C7  Sony crt0.o: _start, _exit, _root
 *   src/crtbegin.c  0x010000C8..0x0100013F  collect2's generated ctor-table object:
 *                                           reg_frame / dereg_frame, __CTOR_LIST__,
 *                                           __DTOR_LIST__, frame_table, __main_reference
 *   src/main.cpp    0x01000140..0x0100078F  main.cxx ("eh" CIE 0x01026CD8): the
 *                                           boot-source records, select_boot_source,
 *                                           main.  (0x010006B0..0x0100078F, the
 *                                           MCardSource_<> typeinfo functions and 
 *                                           dtors instantiated in main.cxx, belong
 *                                           to the imageloader unit.)
 *   src/main_iop.cpp 0x01000790..0x01000A4F a separate C++ object (its own CIE
 *                                           0x01026E44, no EH table): IRX loading,
 *                                           IOP reboot, pad init / START check,
 *                                           IOP shutdown.  Its .rodata starts at
 *                                           0x0103DEA0, its .sdata at 0x01041DC0,
 *                                           its .bss at 0x01042080.
 *
 * Boot flow (main, 0x01000538):
 *   __main (global ctors) -> gfx_init_default(NTSC) -> startup_screen_start
 *   -> boot_iop_reboot_and_pad_init -> select_boot_source -> copy the source's
 *   device path -> ImageLoader(src).load() (inflate + RSA/SHA-512 verify +
 *   copy sections; returns the entry point, 0 = rejected) -> destroy loader
 *   and source -> wait for the startup screen to finish -> shut down MC lock,
 *   pad, SIF cmd -> gfx_shutdown -> entry == 0: hang forever; else
 *   ExecPS2(entry, 2, {path, "hot", NULL}).
 *
 * Boot-source selection (select_boot_source):
 *   Candidates DISK ("cdrom0:\CORE.GT3;1"), MCARD 0 and MCARD 1
 *   ("/BASCUS-97102GAMEDATA/core.gt3" on port 0 / port 1, slot 0), HOST (never
 *   probed).  Each probe reads the u16 loadSourceFlags at offset 0 of that
 *   core.gt3 (0 = absent).  The candidate with the HIGHEST flags wins; on a
 *   tie the later one wins (order DISK < MCARD 0 < MCARD 1 < HOST), so a
 *   memory-card core.gt3 with flags >= the disc's is booted instead of the
 *   disc's.  The memory cards are probed by DEFAULT; holding START during the
 *   pad poll at boot sets g_skipMemoryCardBoot and restricts the choice to the
 *   disc.  (The mapping pass read the bnel the other way round.)
 */
#ifndef BOOT_MAIN_H
#define BOOT_MAIN_H

#include "types.h"
#include "imageloader.h"

/* ======================================================================== */
/* main.cxx                                                                 */
/* ======================================================================== */

/* Index of a candidate in select_boot_source's record array (and in
 * g_bootSourceNames).                                                      */
enum BootSourceIndex {
    BOOT_SOURCE_DISK   = 0,
    BOOT_SOURCE_MCARD0 = 1,
    BOOT_SOURCE_MCARD1 = 2,
    BOOT_SOURCE_HOST   = 3,
    BOOT_SOURCE_COUNT  = 4
};

/* One boot-source candidate (12 bytes, 4 of them in an array on
 * select_boot_source's stack at sp+0, stride 0xC).  No vtable.  The first
 * three members are out-of-line functions of main.cxx; open() / close() are
 * inline (select_boot_source recomputes &rec[best] for each of them, and the
 * open result is materialised as a value before it is tested).            */
class SourceRecord {
public:
    SourceRecord();                         /* 0x010001A0 zero all fields        */
    ~SourceRecord();                        /* 0x01000140 delete source          */
    ImageLoader::Source *release();         /* 0x010001B8 take ownership of source */

    /* source->open(fileName) (vtable slot 3) if there is a source, else false. */
    bool open()  { return source != NULL ? source->open(fileName) : false; }
    /* source->close() (vtable slot 2) if there is a source. */
    void close() { if (source != NULL) source->close(); }

    u16                  loadFlags;         /* +0x00 u16 at offset 0 of the file,
                                             *       0 = not found / not probed    */
    const char          *fileName;          /* +0x04 bare name ("core.gt3")         */
    ImageLoader::Source *source;            /* +0x08 created on the first hit       */
};
static_assert(sizeof(SourceRecord) == 0xC, "SourceRecord: array stride 0xC");
static_assert(offsetof(SourceRecord, fileName) == 0x4, "SourceRecord layout");
static_assert(offsetof(SourceRecord, source) == 0x8, "SourceRecord layout");

/* Probe until some candidate has a core.gt3, open the best one; on open
 * failure close it, yield and probe again (forever).  Returns the opened
 * source (ownership passes to the caller); the other candidates' sources are
 * deleted.                                                                 */
ImageLoader::Source *select_boot_source(void);          /* 0x010001C8 */

int main(int argc, char **argv);                        /* 0x01000538 */

/* main.cxx globals */
extern const char *g_hostCoreFile;      /* @0x01041D88 .sdata = "core.gt3" (HOST record)     */
extern const char *g_diskCoreFile;      /* @0x01041D8C .sdata = "core.gt3" (DISK record)     */
extern const char *g_mcardCoreFile;     /* @0x01041D90 .sdata = "core.gt3" (both MC records) */
extern const char *g_bootSourceNames[BOOT_SOURCE_COUNT];
                                        /* @0x01026CC8 .data {"DISK","MCARD 0","MCARD 1",
                                         * "HOST"} (strings in .sdata 0x01041D98..); no
                                         * code references it                            */

/* ======================================================================== */
/* main_iop.cpp (separate object, 0x01000790..0x01000A4F)                   */
/* ======================================================================== */

/* sceSifLoadModule("cdrom0:\" + UPPERCASE(name) + ";1", 0, NULL), retried
 * until it returns >= 0.                                                   */
void load_irx_module(const char *name);                 /* 0x01000790 */

/* SIF/CD init, IOP reboot with cdrom0:\IOPRP224.IMG;1, CD/fileio re-init,
 * load sio2man / mtapman / mcman / mcserv / padman, pad port 0 open,
 * mc_init_once, then poll the pad for up to 16 frames and set
 * g_skipMemoryCardBoot if START is held.                                   */
void boot_iop_reboot_and_pad_init(void);                /* 0x01000880 */

/* mc_shutdown(); scePadEnd(); sceSifExitCmd().  Called by main right before
 * the launch.                                                              */
void shutdown_iop_services(void);                       /* 0x010009D8 */

/* scePadRead(0, 0, buf): 0 if it returned 0, else the active-high digital
 * button mask 0xFFFF ^ (buf[2] << 8 | buf[3]) (SCE_PAD* bits).             */
u32  read_pad_buttons(void);                            /* 0x01000A08 */

/* 1 = START was held at boot: select_boot_source skips both memory cards. */
extern int g_skipMemoryCardBoot;                        /* @0x01041DC0 .sdata, init 0 */

#endif /* BOOT_MAIN_H */
