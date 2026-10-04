/* main_iop.cpp - IOP / pad start-up object of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), .text 0x01000790..0x01000A4F (unit "main").
 *
 * A translation unit of its own, not part of main.cxx: it has its own
 * .eh_frame CIE (0x01026E44, no "eh" augmentation = no exception table; the
 * FDEs cover exactly 0x01000790..0x01000A4C) and its own data right after
 * main.cxx's:
 *   .rodata 0x0103DEA0  "cdrom0:\\"
 *           0x0103DEB0  "cdrom0:\\IOPRP224.IMG;1"
 *           0x0103DEC8.. "sio2man.irx" "mtapman.irx" "mcman.irx" "mcserv.irx"
 *                        "padman.irx" (16-byte slots)
 *   .sdata  0x01041DC0  g_skipMemoryCardBoot (explicitly initialised: .sdata,
 *                       not .sbss; main.cxx reads it through $gp)
 *           0x01041DC8  ";1"
 *   .bss    0x01042080  s_padDmaBuffer (64-byte aligned, 0x100 bytes)
 * It has an .eh_frame at all, so it was compiled as C++ (GCC 2.9x emits
 * .eh_frame only with exceptions enabled); function names are C++.
 *
 * The file name is this decompilation's; the original's is unknown.
 */
#include "types.h"
#include "libc.h"
#include "sif.h"
#include "sifdev.h"
#include "libcdvd.h"
#include "libpad.h"
#include "boot_io.h"
#include "gs_driver.h"
#include "main.h"

/* ------------------------------------------------------------------------ */
/* Globals                                                                  */
/* ------------------------------------------------------------------------ */

/* Set to 1 by boot_iop_reboot_and_pad_init when START is held; then
 * select_boot_source (main.cxx) probes only the disc.                      */
int g_skipMemoryCardBoot = 0;                   /* @0x01041DC0 (.sdata) */

/* padman DMA area for port 0 / slot 0 (scePadPortOpen). */
static u128 s_padDmaBuffer[scePadDmaBufferMax]  /* @0x01042080 (.bss) */
    __attribute__((aligned(64)));

/* ------------------------------------------------------------------------ */
/* Functions                                                                */
/* ------------------------------------------------------------------------ */

/* 'a'..'z' -> 'A'..'Z'.  An inline function returning char: its result is
 * a QImode value, so egcs neither sign-extends the "- 0x20" arm nor turns
 * the if/else into a conditional move (no movqicc) - the retail code is a
 * plain bnez over "daddu" with the subtraction in the delay slot.  (The
 * same rule in DiskFileName::GetPath is written inline and does sign-extend.) */
static inline char irx_toupper(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - 0x20;
    return c;
}

// FUNC 0x01000790 load_irx_module
//   Only caller: boot_iop_reboot_and_pad_init.  Builds
//   "cdrom0:\" + name with 'a'..'z' upper-cased + ";1" in an alloca'd stack
//   buffer ($fp frame; the alloca size is (len + 11 + 15) & ~15, i.e.
//   8 + len + 3 rounded up to 16) and calls sceSifLoadModule(path, 0, NULL)
//   (0x01022558) until it returns >= 0 - forever if the module cannot be
//   loaded.  len is strlen (0x0100F538), the loop index is unsigned (sltu),
//   the characters are signed (lb); the ";1" copy includes the NUL (3 bytes).
//   Same path rule as DiskFileName::GetPath, written out again here.
//   Both copies are real memcpy calls (0x0101B010): the destinations are
//   void * (no known alignment), so egcs does not expand the builtin inline.
//   The tail pointer is path + 8, then + len (retail computes path + 8
//   before the loop and adds len after it).
void load_irx_module(const char *name)
{
    size_t len = strlen(name);
    void *path = __builtin_alloca(8 + len + 3);
    void *tail;

    memcpy(path, "cdrom0:\\", 8);
    for (size_t i = 0; i < len; i++)
        ((char *)path)[8 + i] = irx_toupper(name[i]);
    tail = (char *)path + 8;
    tail = (char *)tail + len;
    memcpy(tail, ";1", 3);

    while (sceSifLoadModule((char *)path, 0, NULL) < 0)
        ;
}

// FUNC 0x01000880 boot_iop_reboot_and_pad_init
//   Called by main after startup_screen_start.  In order:
//    - sceSifInitRpc(0) (0x0101DA50), sceCdInit(SCECdINIT) (0x01013E70);
//    - retry sceSifRebootIop("cdrom0:\IOPRP224.IMG;1") (0x01022A58) while it
//      returns 0, then retry sceSifSyncIop() (0x01022A10) while it returns 0;
//    - sceSifInitRpc(0), sceCdInit(SCECdINIT) again (the reboot killed the
//      IOP side), sceCdMmode(SCECdDVD) (0x01014348), sceCdDiskReady(1)
//      (0x01014150, non-blocking query; result ignored), sceFsReset()
//      (0x0101F180);
//    - load_irx_module: sio2man, mtapman, mcman, mcserv, padman (.irx);
//    - scePadInit(0) (0x01014540), scePadPortOpen(0, 0, s_padDmaBuffer)
//      (0x010147A0), mc_init_once() (0x0100F340);
//    - pad poll, at most 16 tries (signed counter): state = scePadGetState(0,
//      0) (0x01014B70); on state 0 (disconnected), 2 (FindCTP1) or 6 (stable)
//      read the buttons once and, if START (0x800) is held, set
//      g_skipMemoryCardBoot = 1 (bnel: the store only happens when the bit
//      is set), then return; any other state: gfx_vsync(1) (0x0100D9A8) and
//      try again.  After 16 tries it returns without reading the buttons.
//   All results other than the retry conditions are ignored.
void boot_iop_reboot_and_pad_init(void)
{
    int i;

    sceSifInitRpc(0);
    sceCdInit(SCECdINIT);

    while (!sceSifRebootIop("cdrom0:\\IOPRP224.IMG;1"))
        ;
    while (!sceSifSyncIop())
        ;

    sceSifInitRpc(0);
    sceCdInit(SCECdINIT);
    sceCdMmode(SCECdDVD);
    sceCdDiskReady(1);
    sceFsReset();

    load_irx_module("sio2man.irx");
    load_irx_module("mtapman.irx");
    load_irx_module("mcman.irx");
    load_irx_module("mcserv.irx");
    load_irx_module("padman.irx");

    scePadInit(0);
    scePadPortOpen(0, 0, s_padDmaBuffer);
    mc_init_once();

    /* `break`, not `return`: egcs then keeps the retail loop shape (entry
     * jump to the i < 16 test, gfx_vsync / i++ block placed before it). */
    for (i = 0; i < 16; i++) {
        int state = scePadGetState(0, 0);
        if (state == scePadStateDiscon ||
            state == scePadStateFindCTP1 ||
            state == scePadStateStable) {
            if (read_pad_buttons() & SCE_PADstart)
                g_skipMemoryCardBoot = 1;
            break;
        }
        gfx_vsync(1);
    }
}

// FUNC 0x010009D8 shutdown_iop_services
//   Called by main right before the launch: mc_shutdown() (0x0100F1A0: only
//   deletes the MC lock semaphore), scePadEnd() (0x01014720),
//   sceSifExitCmd() (0x0101D600).  The pad port is not closed first and no
//   libmc / libcdvd teardown is done.
void shutdown_iop_services(void)
{
    mc_shutdown();
    scePadEnd();
    sceSifExitCmd();
}

// FUNC 0x01000A08 read_pad_buttons
//   scePadRead(0, 0, rdata) (0x01014AF0) into a 32-byte stack buffer; if it
//   returns 0 the result is 0, otherwise the active-high button mask
//   ((rdata[2] << 8) | rdata[3]) ^ 0xFFFF.  The buffer contents are not
//   otherwise checked (no status byte / mode test).  Only caller:
//   boot_iop_reboot_and_pad_init.  /* ? */ return type: the value always
//   fits in 16 bits.
u32 read_pad_buttons(void)
{
    unsigned char rdata[32];

    if (scePadRead(0, 0, rdata) != 0)
        return ((rdata[2] << 8) | rdata[3]) ^ 0xFFFF;
    return 0;
}
