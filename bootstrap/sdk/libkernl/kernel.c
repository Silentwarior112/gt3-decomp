/* kernel.c - Sony libkernl (PsIIlibkernl2240) part 1, C side, as linked into
 * the GT3 US bootstrap (BOOTSTRAP_SCUS-97102).  Unit kernl_sys.
 *
 * Address range 0x0101C050..0x0101D307, in link (object) order:
 *   cache.o    0x0101C050  _SyncDCache / SyncDCache / iSyncDCache,
 *                          _InvalidDCache / InvalidDCache / iInvalidDCache
 *   intr.o     0x0101C2D0  DisableIntc / EnableIntc / DisableDmac / EnableDmac
 *                          and the interrupt-context i-forms
 *   alarm.o    0x0101C4F0  private SetSyscall/Copy/GetEntryAddress stubs, the
 *                          kernel-mode Copy service, InitAlarm (+ .data blob)
 *   thread.o   0x0101C630  "top" thread server, InitThread, iWakeupThread,
 *                          iRotateThreadReadyQueue, iSuspendThread
 *   sio/printf 0x0101C990  EE SIO putc, DECI2 kputs line buffer, the kernel
 *                          printf engine (_printf / scePrintf)
 * The syscall stub table (0x0101B800..) is in syscalls.S, the SIF command /
 * first half of SIF RPC (0x0101D308..0x0101E0BF) in sifcmd.c.
 *
 * Nothing here is related to the core.gt3 signature check: InitAlarm's copy of
 * 0x740 bytes into kernel RAM is Sony's standard SDK 2.2 "new alarm" patch.
 */
#include "types.h"
#include "eekernel.h"
#include <stdarg.h>

/* ======================================================================== */
/* Hardware registers used here                                             */
/* ======================================================================== */

#define T3_MODE      0x10001810   /* EE timer 3 mode register                  */
#define T3_MODE_CMPE 0x100        /* compare-interrupt enable (set by the alarm patch: 0x583) */
#define SIO_ISR      0x1000F130   /* EE SIO (debug UART) status                 */
#define SIO_TXFIFO   0x1000F180   /* EE SIO transmit FIFO (byte)                */

/* Linker-defined small-data base (0x01049CF0), passed as the new thread's gp. */
extern char _gp[];

/* ======================================================================== */
/* cache.o                                                                  */
/* ======================================================================== */

/* R5900 D-cache: 64-byte lines, 64 sets (index 0..0xFC0), 2 ways (way = bit 0
 * of the cache-op address).  COP0 $28 = TagLo, bits 31..12 = physical tag.
 *   cache 0x10 DXLTG  index load tag      cache 0x14 DXWBIN index writeback-invalidate
 *   cache 0x16 DXIN   index invalidate    cache 0x18 DHWBIN hit writeback-invalidate
 * Every op is bracketed by sync.l exactly as in the binary.                  */
#define DCACHE_LOAD_TAG(idx, way, tag)                                        \
    __asm__ __volatile__("sync.l\n\tcache 0x10, " #way "(%1)\n\tsync.l\n\tmfc0 %0, $28" \
                         : "=r"(tag) : "r"(idx) : "memory")
#define DCACHE_INDEX_OP(op, idx, way)                                         \
    __asm__ __volatile__("sync.l\n\tcache " #op ", " #way "(%0)\n\tsync.l"   \
                         : : "r"(idx) : "memory")
#define SYNC_L() __asm__ __volatile__("sync.l" : : : "memory")

// FUNC 0x0101C050 _SyncDCache
//   Index-walks both ways of all 64 D-cache sets; for every line whose
//   (TagLo & 0xFFFFF000) + index lies in [start, end] (unsigned, inclusive)
//   does DXWBIN (write back + invalidate).  The valid/dirty bits are not
//   looked at.  Callers pass 64-byte-aligned bounds.
static void _SyncDCache(void *start, void *end)
{
    int idx;
    u32 tag, addr;

    for (idx = 0; idx < 0x1000; idx += 0x40) {
        DCACHE_LOAD_TAG(idx, 0, tag);
        addr = (tag & 0xFFFFF000) + idx;
        if (!(addr < (u32)start) && !((u32)end < addr))
            DCACHE_INDEX_OP(0x14, idx, 0);
        DCACHE_LOAD_TAG(idx, 1, tag);
        addr = (tag & 0xFFFFF000) + idx;
        if (!(addr < (u32)start) && !((u32)end < addr))
            DCACHE_INDEX_OP(0x14, idx, 1);
        SYNC_L();
    }
}

// FUNC 0x0101C0F8 SyncDCache
//   Thread-context wrapper: if COP0 Status.EIE was set, DIntr() before and
//   EIntr() (tail call) after; bounds are rounded down to 64 bytes.
//   Callers: libpad DMA buffer routines.
void SyncDCache(void *start, void *end)
{
    u32 eie;

    __asm__ __volatile__("mfc0 %0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DIntr();
    _SyncDCache((void *)((u32)start & 0xFFFFFFC0), (void *)((u32)end & 0xFFFFFFC0));
    if (eie)
        EIntr();
}

// FUNC 0x0101C178 iSyncDCache
//   Interrupt-context form: align both bounds to 64 bytes and tail-jump into
//   _SyncDCache without touching the interrupt state.
void iSyncDCache(void *start, void *end)
{
    _SyncDCache((void *)((u32)start & 0xFFFFFFC0), (void *)((u32)end & 0xFFFFFFC0));
}

// FUNC 0x0101C190 _InvalidDCache
//   Same tag walk as _SyncDCache but the per-line op is DXIN (cache 0x16):
//   invalidate WITHOUT write-back.
static void _InvalidDCache(void *start, void *end)
{
    int idx;
    u32 tag, addr;

    for (idx = 0; idx < 0x1000; idx += 0x40) {
        DCACHE_LOAD_TAG(idx, 0, tag);
        addr = (tag & 0xFFFFF000) + idx;
        if (!(addr < (u32)start) && !((u32)end < addr))
            DCACHE_INDEX_OP(0x16, idx, 0);
        DCACHE_LOAD_TAG(idx, 1, tag);
        addr = (tag & 0xFFFFF000) + idx;
        if (!(addr < (u32)start) && !((u32)end < addr))
            DCACHE_INDEX_OP(0x16, idx, 1);
        SYNC_L();
    }
}

// FUNC 0x0101C238 InvalidDCache
//   DIntr/EIntr-guarded wrapper of _InvalidDCache (same shape as SyncDCache).
//   No callers in this binary (linked with cache.o).
void InvalidDCache(void *start, void *end)
{
    u32 eie;

    __asm__ __volatile__("mfc0 %0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DIntr();
    _InvalidDCache((void *)((u32)start & 0xFFFFFFC0), (void *)((u32)end & 0xFFFFFFC0));
    if (eie)
        EIntr();
}

// FUNC 0x0101C2B8 iInvalidDCache
//   Interrupt-context form: align and tail-jump into _InvalidDCache.  Unused.
void iInvalidDCache(void *start, void *end)
{
    _InvalidDCache((void *)((u32)start & 0xFFFFFFC0), (void *)((u32)end & 0xFFFFFFC0));
}

/* ======================================================================== */
/* INTC / DMAC mask wrappers                                                */
/* ======================================================================== */

// FUNC 0x0101C2D0 DisableIntc
//   Status.EIE sampled; DIntr() if set; _DisableIntc (#0x15); sync.l;
//   EIntr() if EIE was set; returns the kernel's result.
//   Callers: sceGsResetGraph, game code.
int DisableIntc(int cause)
{
    u32 eie;
    int ret;

    __asm__ __volatile__("mfc0 %0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DIntr();
    ret = _DisableIntc(cause);
    SYNC_L();
    if (eie)
        EIntr();
    return ret;
}

// FUNC 0x0101C338 EnableIntc
//   As DisableIntc with _EnableIntc (#0x14).  Caller: game VBlank setup 0x0100C490.
int EnableIntc(int cause)
{
    u32 eie;
    int ret;

    __asm__ __volatile__("mfc0 %0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DIntr();
    ret = _EnableIntc(cause);
    SYNC_L();
    if (eie)
        EIntr();
    return ret;
}

// FUNC 0x0101C3A0 DisableDmac
//   As DisableIntc with _DisableDmac (#0x17).  Callers: sceSifExitCmd (ch 5), game.
int DisableDmac(int channel)
{
    u32 eie;
    int ret;

    __asm__ __volatile__("mfc0 %0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DIntr();
    ret = _DisableDmac(channel);
    SYNC_L();
    if (eie)
        EIntr();
    return ret;
}

// FUNC 0x0101C408 EnableDmac
//   As DisableIntc with _EnableDmac (#0x16).  Callers: sceSifInitCmd (ch 5), game.
int EnableDmac(int channel)
{
    u32 eie;
    int ret;

    __asm__ __volatile__("mfc0 %0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DIntr();
    ret = _EnableDmac(channel);
    SYNC_L();
    if (eie)
        EIntr();
    return ret;
}

// FUNC 0x0101C470 iEnableIntc
//   _iEnableIntc (#-0x1A) then sync.l; returns the kernel's result.  Unused.
int iEnableIntc(int cause)
{
    int ret = _iEnableIntc(cause);
    SYNC_L();
    return ret;
}

// FUNC 0x0101C490 iDisableIntc
//   _iDisableIntc (#-0x1B) then sync.l.  Unused.
int iDisableIntc(int cause)
{
    int ret = _iDisableIntc(cause);
    SYNC_L();
    return ret;
}

// FUNC 0x0101C4B0 iEnableDmac
//   _iEnableDmac (#-0x1C) then sync.l.  Unused.
int iEnableDmac(int channel)
{
    int ret = _iEnableDmac(channel);
    SYNC_L();
    return ret;
}

// FUNC 0x0101C4D0 iDisableDmac
//   _iDisableDmac (#-0x1D) then sync.l.  Unused.
int iDisableDmac(int channel)
{
    int ret = _iDisableDmac(channel);
    SYNC_L();
    return ret;
}

/* ======================================================================== */
/* alarm.o - SDK 2.2 "new alarm" kernel patch installer                     */
/* ======================================================================== */

/* alarm.o's private syscall stubs (same 16-byte shape as syscalls.S).       */
void  _alarm_SetSyscall(int syscall_no, void *handler);                 /* 0x0101C4F0 #0x74 */
void *_alarm_Copy(void *dst, const void *src, int nbytes);              /* 0x0101C500 #0x5A */
void *_alarm_GetEntryAddress(int syscall_no);                           /* 0x0101C548 #0x5B */

// FUNC 0x0101C4F0 _alarm_SetSyscall
//   alarm.o's private copy of SetSyscall: `addiu $v1,$0,0x74; syscall; jr $ra; nop`.
__asm__(
    "\t.text\n"
    "\t.globl\t_alarm_SetSyscall\n"
    "\t.type\t_alarm_SetSyscall, @function\n"
    "\t.set\tpush\n"
    "\t.set\tnoreorder\n"
    "_alarm_SetSyscall:\n"
    "\taddiu\t$3, $0, 0x74\n"
    "\tsyscall\n"
    "\tjr\t$31\n"
    "\tnop\n"
    "\t.set\tpop\n"
    "\t.size\t_alarm_SetSyscall, . - _alarm_SetSyscall\n");

// FUNC 0x0101C500 _alarm_Copy
//   alarm.o's private stub for syscall #0x5A.  Slot 0x5A is first re-pointed
//   by InitAlarm to _kCopy_handler below, so this becomes a kernel-mode word
//   copy able to write kseg0 kernel RAM.
__asm__(
    "\t.text\n"
    "\t.globl\t_alarm_Copy\n"
    "\t.type\t_alarm_Copy, @function\n"
    "\t.set\tpush\n"
    "\t.set\tnoreorder\n"
    "_alarm_Copy:\n"
    "\taddiu\t$3, $0, 0x5A\n"
    "\tsyscall\n"
    "\tjr\t$31\n"
    "\tnop\n"
    "\t.set\tpop\n"
    "\t.size\t_alarm_Copy, . - _alarm_Copy\n");

// FUNC 0x0101C510 _kCopy_handler
//   Kernel-mode handler that InitAlarm installs as syscall 0x5A (table word
//   @0x0103D9EC).  Copies nbytes>>2 words (logical shift, unsigned count),
//   returns 0.
static int _kCopy_handler(u32 *dst, const u32 *src, int nbytes)
{
    unsigned int n = (unsigned int)nbytes >> 2;
    unsigned int i;

    for (i = 0; i < n; i++)
        *dst++ = *src++;
    return 0;
}

// FUNC 0x0101C548 _alarm_GetEntryAddress
//   alarm.o's private stub for syscall #0x5B.  After InitAlarm points slot
//   0x5B at 0x80076000 (the blob's lookup routine) it returns the kernel
//   address of a patch service by number.
__asm__(
    "\t.text\n"
    "\t.globl\t_alarm_GetEntryAddress\n"
    "\t.type\t_alarm_GetEntryAddress, @function\n"
    "\t.set\tpush\n"
    "\t.set\tnoreorder\n"
    "_alarm_GetEntryAddress:\n"
    "\taddiu\t$3, $0, 0x5B\n"
    "\tsyscall\n"
    "\tjr\t$31\n"
    "\tnop\n"
    "\t.set\tpop\n"
    "\t.size\t_alarm_GetEntryAddress, . - _alarm_GetEntryAddress\n");

/* "PsIIlibkernl2240" @0x0103D278 (.data, 16 bytes, no NUL) - the library
 * version object at the head of libkernl's .data.  loadfile (kernl_rest)
 * compares the IOP module version against the "2240" at +0x0C (0x0103D284). */
char _libkernl_version[16] = { 'P','s','I','I','l','i','b','k','e','r','n','l','2','2','4','0' };

/* Alarm kernel patch @0x0103D288 (.data), 0x740 bytes of EE kernel-mode code,
 * copied to 0x80076000.  Layout (kernel addresses):
 *   +0x000  service 0x5B "GetEntryAddress": look num up in the table at +0x710
 *   +0x038  time-wrap helper, +0x058 sorted insert into the alarm array
 *   +0x160  set-alarm core (reads T3_COUNT 0xB0001800, max 64 alarms, else -1)
 *   +0x2A0  services 0xFD / 0xFF: release alarm by id (reprograms T3_COMP)
 *   +0x440  services 0xFC / 0xFE: set alarm ((u16)time, callback, arg)
 *   +0x460  write T3_COMP, T3_MODE = 0x583 (H-BLNK clock, CUE, CMPE, EQUF clear)
 *   +0x488  service 0x12C: timer-3 compare interrupt handler (fires expired
 *           alarms, ends `sync; ei`)
 *   +0x680  callback dispatch: saves ra/sp, EPC = 0x82000, eret to user mode
 *   +0x6C0  service 0x08: return from callback (restores Status/ra/sp)
 *   +0x700  alarm count and data, +0x710 service table {0xFC,+0x440} {0xFE,+0x440}
 *           {0xFD,+0x2A0} {0xFF,+0x2A0} {0x12C,+0x488} {0x08,+0x6C0}
 * Values verified by `python tools/kernl_sys_extract.py --check`.           */
static u32 _alarm_patch_code[0x740 / 4] __attribute__((aligned(8))) = {
    0x3C028007, 0x0000282D, 0x24436710, 0x00000000, 0x8C620000, 0x14820003,
    0x24A50001, 0x03E00008, 0x8C620004, 0x2CA20006, 0x1440FFF9, 0x24630008,
    0x03E00008, 0x0000102D, 0x00A4202A, 0x10800003, 0x3C020001, 0x03E00008,
    0x00A21025, 0x03E00008, 0x00A0102D, 0x00000000, 0x27BDFF80, 0xFFB30030,
    0x3C138007, 0xFFB60060, 0xFFB50050, 0x0080B02D, 0xFFB00000, 0x00A0A82D,
    0x8E626700, 0x0000802D, 0xFFBF0070, 0xFFB40040, 0xFFB20020, 0x18400028,
    0xFFB10010, 0x3C148007, 0x24120014, 0x00000000, 0x26916740, 0x02121018,
    0x02C0202D, 0x00511021, 0x0C01D80E, 0x94450000, 0x02A2102A, 0x10400018,
    0x8E626700, 0x2444FFFF, 0x0090182A, 0x14600018, 0x00921018, 0x00511821,
    0x68650007, 0x6C650000, 0x6866000F, 0x6C660008, 0x8C670010, 0xB065001B,
    0xB4650014, 0xB0660023, 0xB466001C, 0xAC670024, 0x2484FFFF, 0x2463FFEC,
    0x0090102A, 0x00000000, 0x1040FFF1, 0x00000000, 0x10000006, 0x0200102D,
    0x26100001, 0x0202102A, 0x1440FFDD, 0x24120014, 0x0200102D, 0xDFBF0070,
    0xDFB60060, 0xDFB50050, 0xDFB40040, 0xDFB30030, 0xDFB20020, 0xDFB10010,
    0xDFB00000, 0x03E00008, 0x27BD0080, 0x00000000, 0x27BDFF70, 0x3C02B000,
    0xFFB70070, 0x34421800, 0xFFB60060, 0x00C0B82D, 0xFFB50050, 0x00A0B02D,
    0xFFB30030, 0x3C158007, 0xFFBF0080, 0x3093FFFF, 0xFFB20020, 0xFFB10010,
    0xFFB00000, 0xFFB40040, 0x8EA36700, 0x8C540000, 0x28630040, 0x14600008,
    0x02749821, 0x1000002E, 0x2402FFFF, 0x0060902D, 0x00621014, 0x00821025,
    0x1000000D, 0xFCA26708, 0x3C058007, 0x0000182D, 0xDCA46708, 0x00641016,
    0x30420001, 0x1040FFF5, 0x24020001, 0x24630001, 0x28620040, 0x1440FFFA,
    0x00641016, 0x2412FFFF, 0x0640001B, 0x0240102D, 0x0380882D, 0x0260282D,
    0x0C01D816, 0x0280202D, 0x24040014, 0x3C088007, 0x00441018, 0x25036740,
    0x8EA56700, 0x24700004, 0x24A50001, 0x00432021, 0x00623821, 0x00508021,
    0xA4940002, 0xA4930000, 0x00E0302D, 0xAE120000, 0x00C0182D, 0xACF10010,
    0x95046740, 0xACD60008, 0xAC77000C, 0x0C01D918, 0xAEA56700, 0x8E020000,
    0xDFBF0080, 0xDFB70070, 0xDFB60060, 0xDFB50050, 0xDFB40040, 0xDFB30030,
    0xDFB20020, 0xDFB10010, 0xDFB00000, 0x03E00008, 0x27BD0090, 0x00000000,
    0x27BDFFD0, 0x3C0C8007, 0xFFB10010, 0x0080682D, 0x8D826700, 0x0180882D,
    0xFFBF0020, 0x2406FFFF, 0x18400058, 0xFFB00000, 0x18400056, 0x0000402D,
    0x3C0B8007, 0x24030014, 0x25656740, 0x01032018, 0x00A41021, 0x8C430004,
    0x15A3004A, 0x8D826700, 0x3C03B000, 0x00852021, 0x34631820, 0x94850000,
    0x8C620000, 0x14A20008, 0x24030014, 0x3C021000, 0x3442F000, 0x8C430000,
    0x30631000, 0x14600043, 0x2402FFFF, 0x24030014, 0x8D896700, 0x01031818,
    0x25646740, 0x2522FFFF, 0x0100382D, 0x0102102A, 0x00641821, 0x10400019,
    0x94700002, 0x3C0A8007, 0x24E30001, 0x24050014, 0x00651018, 0x00E52018,
    0x25666740, 0x0060382D, 0x00462821, 0x00862021, 0x2522FFFF, 0x68A30007,
    0x6CA30000, 0x68A6000F, 0x6CA60008, 0x8CAE0010, 0xB0830007, 0xB4830000,
    0xB086000F, 0xB4860008, 0x00E2102A, 0x1440FFEC, 0xAC8E0010, 0x10000003,
    0x24020001, 0x3C0A8007, 0x24020001, 0x8D846700, 0xDD436708, 0x01A21014,
    0x00021027, 0x2484FFFF, 0x00621824, 0xAD846700, 0x15000003, 0xFD436708,
    0x0C01D918, 0x95646740, 0x8E226700, 0x14400004, 0x24030083, 0x3C02B000,
    0x34421810, 0xAC430000, 0x3C02B000, 0x0200202D, 0x34421800, 0x0C01D80E,
    0x8C450000, 0x10000005, 0x00503023, 0x25080001, 0x0102102A, 0x1440FFAE,
    0x24030014, 0x0000000F, 0x00C0102D, 0xDFBF0020, 0xDFB10010, 0xDFB00000,
    0x03E00008, 0x27BD0030, 0x27BDFFF0, 0xFFBF0000, 0x0C01D858, 0x3084FFFF,
    0x0000000F, 0xDFBF0000, 0x03E00008, 0x27BD0010, 0x3C02B000, 0x34421820,
    0xAC440000, 0x0000000F, 0x3C02B000, 0x24030583, 0x34421810, 0x03E00008,
    0xAC430000, 0x00000000, 0x27BDFF50, 0x0000402D, 0xFFBF00A0, 0xFFB70090,
    0xFFB60080, 0xFFB50070, 0xFFB40060, 0xFFB30050, 0xFFB20040, 0xFFB10030,
    0xFFB00020, 0x3C118007, 0x3C128007, 0x00000000, 0x8E226700, 0x0102102A,
    0x1040000A, 0x24030014, 0x26446740, 0x01031818, 0x96456740, 0x00641821,
    0x94620000, 0x10A2FFF6, 0x25080001, 0x0C01D918, 0x0040202D, 0x3C028007,
    0x0220B02D, 0x24546740, 0x24130014, 0x3C158007, 0x10000004, 0x24170001,
    0x96426740, 0x1462003E, 0x8E226700, 0x8EC26700, 0x0000402D, 0x26466740,
    0x68C30007, 0x6CC30000, 0x68C4000F, 0x6CC40008, 0x8CC50010, 0xB3A30007,
    0xB7A30000, 0xB3A4000F, 0xB7A40008, 0xAFA50010, 0x2442FFFF, 0x1840001A,
    0xAEC26700, 0x8E296700, 0x8FAA0010, 0x8FA60004, 0x97A70000, 0x00000000,
    0x01131818, 0x25020001, 0x0040402D, 0x00742821, 0x00531818, 0x00742021,
    0x688B0007, 0x6C8B0000, 0x688C000F, 0x6C8C0008, 0x8C8D0010, 0xB0AB0007,
    0xB4AB0000, 0xB0AC000F, 0xB4AC0008, 0x0109182A, 0x1460FFEF, 0xACAD0010,
    0x10000004, 0x00000000, 0x8FAA0010, 0x8FA60004, 0x97A70000, 0x0380802D,
    0x0140E02D, 0xDEA36708, 0x00D71014, 0x00021027, 0x3C040008, 0x00621824,
    0x8FA50008, 0x8FA8000C, 0x34842000, 0x0C01D9A0, 0xFEA36708, 0x0200E02D,
    0x8E226700, 0x1C40FFC2, 0x97A30000, 0x8E226700, 0x18400005, 0x24030483,
    0x0C01D918, 0x96446740, 0x10000004, 0x00000000, 0x3C02B000, 0x34421810,
    0xAC430000, 0x0000000F, 0x42000038, 0xDFBF00A0, 0xDFB70090, 0xDFB60080,
    0xDFB50070, 0xDFB40060, 0xDFB30050, 0xDFB20040, 0xDFB10030, 0xDFB00020,
    0x03E00008, 0x27BD00B0, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x3C1A8007, 0xAF5F6C40, 0x3C1A8007, 0xAF5D6C50,
    0x40847000, 0x0000040F, 0x00A0182D, 0x00C0202D, 0x00E0282D, 0x0100302D,
    0x401A6000, 0x375A0012, 0x409A6000, 0x0000040F, 0x42000018, 0x00000000,
    0x40016000, 0x241AFFE4, 0x003A0824, 0x40816000, 0x0000040F, 0x3C1A8007,
    0x8F5F6C40, 0x3C1A8007, 0x03E00008, 0x8F5D6C50, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x000000FC, 0x80076440, 0x000000FE, 0x80076440,
    0x000000FD, 0x800762A0, 0x000000FF, 0x800762A0, 0x0000012C, 0x80076488,
    0x00000008, 0x800766C0,
};

/* Alarm callback trampoline @0x0103D9C8 (.data), copied to 0x00082000:
 *   lui $sp,8; jalr $v1; addiu $sp,$sp,0x1FC0 (sp = 0x81FC0); li $v1,-8; syscall; 0; 0; 0
 * i.e. callback(a0, a1, a2) on a private stack, then `syscall -8` back into
 * the patch's service 0x08.                                                  */
static u32 _alarm_patch_stub[0x20 / 4] = {
    0x3C1D0008, 0x0060F809, 0x27BD1FC0, 0x2403FFF8, 0x0000000C, 0x00000000,
    0x00000000, 0x00000000,
};

/* InitAlarm install table @0x0103D9E8 (.data). */
struct _alarm_syscall {
    int   num;       /* +0x00 */
    void *handler;   /* +0x04 (entries 2..7: 0, resolved via GetEntryAddress) */
};
static struct _alarm_syscall _alarm_syscalls[8] = {
    { 0x5A,  (void *)_kCopy_handler },   /* kernel-mode Copy                     */
    { 0x5B,  (void *)0x80076000 },       /* blob+0x000: GetEntryAddress service  */
    { 0xFC,  0 },                        /* SetAlarm                             */
    { 0xFE,  0 },                        /* iSetAlarm (-0xFE)                    */
    { 0xFD,  0 },                        /* ReleaseAlarm                         */
    { 0xFF,  0 },                        /* iReleaseAlarm (-0xFF)                */
    { 0x12C, 0 },                        /* T3 compare interrupt handler slot  ? */
    { 0x08,  0 },                        /* return-from-callback (syscall -8)    */
};

// FUNC 0x0101C558 InitAlarm
//   Installs the SDK 2.2 "new alarm" kernel patch.  Skipped when T3_MODE.CMPE
//   is already set (patch already active).  Otherwise: point syscall 0x5A at
//   _kCopy_handler, Copy the 0x740-byte patch to 0x80076000 and the 0x20-byte
//   trampoline to 0x00082000, FlushCache(0) + FlushCache(2), point 0x5B at the
//   patch's lookup routine, then for services 0xFC,0xFE,0xFD,0xFF,0x12C,0x08
//   SetSyscall(n, GetEntryAddress(n)).  Called by _InitSys (0x01022C18).
void InitAlarm(void)
{
    unsigned int i;

    if (REG32(T3_MODE) & T3_MODE_CMPE)
        return;

    _alarm_SetSyscall(_alarm_syscalls[0].num, _alarm_syscalls[0].handler);
    _alarm_Copy((void *)0x80076000, _alarm_patch_code, 0x740);
    _alarm_Copy((void *)0x00082000, _alarm_patch_stub, 0x20);
    FlushCache(WRITEBACK_DCACHE);
    FlushCache(INVALIDATE_ICACHE);
    _alarm_SetSyscall(_alarm_syscalls[1].num, _alarm_syscalls[1].handler);
    for (i = 2; i < 8; i++)
        _alarm_SetSyscall(_alarm_syscalls[i].num,
                          _alarm_GetEntryAddress(_alarm_syscalls[i].num));
}

/* ======================================================================== */
/* thread.o - interrupt-safe thread operations via a "top" server thread    */
/* ======================================================================== */

/* Request ring of the top thread @0x0104A378 (.bss).                        */
struct _top_request {
    u8 mode;                        /* +0 0 = Wakeup, 1 = RotateReadyQueue, 2 = Suspend */
    u8 data;                        /* +1 thread id / priority (truncated to 8 bits) */
};
struct _top_arg {
    u32                 requestOut; /* +0x000 read index (masked &0x1FF on use)      */
    u32                 requestIn;  /* +0x004 write index (masked &0x1FF on use)     */
    struct _top_request request[512]; /* +0x008                                     */
};                                  /* 0x408 */
_Static_assert(sizeof(struct _top_arg) == 0x408, "_top_arg");

static u8              topStack[0x400] __attribute__((aligned(16))); /* @0x01049F70 (.bss) */
static int             topSema;                                      /* @0x0104A370 (.bss) */
static struct _top_arg topArg;                                       /* @0x0104A378 (.bss) */
static int             topId = 0;                                    /* @0x0103DA28 (.data) */

/* Inline interrupt-context GetThreadId (`addiu $v1,$0,-0x2F; syscall`), used
 * by iWakeupThread / iSuspendThread (0x0101C7EC, 0x0101C904).  $a0 survives
 * the syscall in the original.  The syscall number load is part of the asm
 * statement: in the binary it follows the prologue's register saves, where
 * egcs never leaves a separate `li` into a register variable (its scheduler
 * hoists that above the saves).                                            */
static __inline__ int _iGetThreadId(void)
{
    register int v0 __asm__("$2");

    __asm__ __volatile__("addiu\t$3, $0, -0x2F\n\tsyscall" : "=r"(v0) : : "$3", "memory");
    return v0;
}

// FUNC 0x0101C630 topThread
//   Priority-0 server thread (started by InitThread with arg = &topArg):
//   forever WaitSema(topSema), pop one {mode,data} request and perform it in
//   thread context: 0 WakeupThread, 1 RotateThreadReadyQueue, 2 SuspendThread,
//   anything else prints "## internel error in libkernl.a!\n" (sic) through
//   the SIO printf.  Never returns.
static void topThread(void *arg)
{
    struct _top_arg *q = (struct _top_arg *)arg;
    unsigned int i;

    for (;;) {
        WaitSema(topSema);
        i = q->requestOut & 0x1FF;
        q->requestOut = i + 1;
        switch (q->request[i].mode) {
        case 0:
            WakeupThread(q->request[i].data);
            break;
        case 1:
            RotateThreadReadyQueue(q->request[i].data);
            break;
        case 2:
            SuspendThread(q->request[i].data);
            break;
        default:
            _printf("## internel error in libkernl.a!\n");
            break;
        }
    }
}

// FUNC 0x0101C708 InitThread
//   Thread-server init (ps2sdk name InitThread; _InitSys tail-jumps here).
//   Returns -1 if topId > 0 already, if CreateSema fails, or if CreateThread
//   fails (then the sema is deleted; topId keeps the negative id).  Otherwise
//   creates sema {max 0xFF, init 0}, thread {topThread, topStack, 0x400, gp,
//   prio 0}, clears the ring, StartThread(topId, &topArg), drops the caller
//   to priority 1 and returns topId.  Only +0x04/+0x08 of the SemaParam and
//   +0x04..+0x14 of the ThreadParam are written (rest is stack garbage).
int InitThread(void)
{
    struct ThreadParam tp;      /* sp+0x00 */
    struct SemaParam   sp;      /* sp+0x30 */

    if (topId > 0)
        return -1;

    sp.maxCount = 0xFF;
    sp.initCount = 0;
    topSema = CreateSema(&sp);
    if (topSema < 0)
        return -1;

    tp.entry = topThread;
    tp.stack = topStack;
    tp.stackSize = 0x400;
    tp.gpReg = _gp;
    tp.initPriority = 0;
    topId = CreateThread(&tp);
    if (topId < 0) {
        DeleteSema(topSema);
        return -1;
    }

    topArg.requestOut = 0;
    topArg.requestIn = 0;
    StartThread(topId, &topArg);
    ChangeThreadPriority(GetThreadId(), 1);
    return topId;
}

// FUNC 0x0101C7E0 iWakeupThread
//   Interrupt-context wakeup with the kernel-bug workaround: if tid is not
//   the current thread, plain _iWakeupThread(tid) (#-0x34) and its result.
//   If it IS the current thread: fail (-1) when tid >= 0x100 (unsigned) or the
//   server was never created (topId == 0); else queue {0, tid} for topThread,
//   iSignalSema(topSema) and return tid.
//   Callers: the game GS driver's VBlank INTC handler (0x0100E630, twice) and
//   DMAC channel handler (0x0100E8E8), libmc's alarm callback (0x01016148),
//   and _request_call (tail call at 0x0101E27C).
int iWakeupThread(int tid)
{
    int self = _iGetThreadId();
    unsigned int i;

    if (self != tid)
        return _iWakeupThread(tid);

    if (!((unsigned int)self < 0x100) || topId == 0)
        return -1;

    i = topArg.requestIn & 0x1FF;
    topArg.requestIn = i + 1;
    topArg.request[i].mode = 0;
    topArg.request[i].data = (u8)self;
    iSignalSema(topSema);
    return self;
}

// FUNC 0x0101C878 iRotateThreadReadyQueue
//   Always goes through the server: fails (-1) if prio >= 0x80 (unsigned) or
//   topId == 0, else queues {1, prio}, iSignalSema(topSema), returns prio.
//   _iRotateThreadReadyQueue (#-0x2C) is never used.  Unused in this binary.
int iRotateThreadReadyQueue(int prio)
{
    unsigned int i;

    if (!((unsigned int)prio < 0x80) || topId == 0)
        return -1;

    i = topArg.requestIn & 0x1FF;
    topArg.requestIn = i + 1;
    topArg.request[i].mode = 1;
    topArg.request[i].data = (u8)prio;
    iSignalSema(topSema);
    return prio;
}

// FUNC 0x0101C8F8 iSuspendThread
//   Same shape as iWakeupThread with _iSuspendThread (#-0x38) and request 2.
//   Unused in this binary.
int iSuspendThread(int tid)
{
    int self = _iGetThreadId();
    unsigned int i;

    if (self != tid)
        return _iSuspendThread(tid);

    if (!((unsigned int)self < 0x100) || topId == 0)
        return -1;

    i = topArg.requestIn & 0x1FF;
    topArg.requestIn = i + 1;
    topArg.request[i].mode = 2;
    topArg.request[i].data = (u8)self;
    iSignalSema(topSema);
    return self;
}

/* ======================================================================== */
/* SIO / kernel printf                                                      */
/* ======================================================================== */

/* Object boundary (thread.o | the printf object, 0x0101C990): in the link
 * the printf object's .rodata starts 16-aligned - it holds _kernel_vprintf's
 * jump table (.align 4) - at 0x01040540, leaving 0x01040532..0x0104053F as
 * padding after thread.o's message.  This file merges both objects, so the
 * alignment is restated here.  Layout only: no code, no data bytes.         */
__asm__("\t.section\t.rodata\n\t.align\t4\n\t.previous\n");

static void _sio_putchar_crlf(int c);
static void _kputs_putc(int c);

/* (Declaration order = .data layout order: the count precedes the hook.)    */
static int  _kputs_count = 0;   /* @0x0103DA2C (.data) chars buffered         */

/* Output hook of the kernel printf engine @0x0103DA30 (.data), initially the
 * SIO CR/LF putchar; scePrintf swaps in the DECI2 line buffer temporarily.   */
static void (*_putc_hook)(int c) = _sio_putchar_crlf;

static char _kputs_buf[0x80];   /* @0x0104A780 (.bss) DECI2 kputs line buffer */

// FUNC 0x0101C990 sio_putc
//   Busy-waits while SIO_ISR bit 15 (TX FIFO full) is set, then writes the
//   byte to SIO_TXFIFO; returns c.  The three nops before the loop branch in
//   the binary are not in the source: the SCE compiler pads loops of <= 6
//   instructions itself (R5900 short-loop erratum).  The (volatile) TX store
//   sits in the `jr` delay slot in the binary although egcs never puts a
//   volatile access there: egcs leaves that `j $31` unfilled and the
//   assembler Sony built the SDK with moved the store into the slot (it fills
//   unfilled delay slots with the preceding non-load instruction; the bundled
//   ee-as never does, so tools/mcc.py output ends `sb; jr; nop` here).
//   GCC 14 does not pad, so there the three nops are written out to keep the
//   binary's poll-loop body.
#ifdef BOOT_OLD_GCC
#define SIO_POLL_PAD() ((void)0)
#else
#define SIO_POLL_PAD() __asm__ __volatile__("nop\n\tnop\n\tnop")
#endif
int sio_putc(int c)
{
    while (REG32(SIO_ISR) & 0x8000)
        SIO_POLL_PAD();
    *(volatile u8 *)SIO_TXFIFO = (u8)c;
    return c;
}
#undef SIO_POLL_PAD

// FUNC 0x0101C9C8 _kputs_putc
//   Line-buffered putc for scePrintf: when 0x7E chars are pending, NUL-
//   terminate at [0x7F] (sic, not at the count) and kPuts the buffer first;
//   '\n' is stored, terminated and the line flushed via kPuts (tail call);
//   any other char is just appended.  ($v0 on return is leftover: kPuts's
//   result or &_kputs_buf[n]; nobody uses it.)
static void _kputs_putc(int c)
{
    int n = _kputs_count;

    if (!(n < 0x7E)) {
        _kputs_count = 0;
        _kputs_buf[0x7F] = '\0';
        kPuts(_kputs_buf);
        n = _kputs_count;
    }
    if (c == '\n') {
        _kputs_count = 0;
        _kputs_buf[n] = (char)c;
        _kputs_buf[n + 1] = '\0';
        kPuts(_kputs_buf);
    } else {
        _kputs_count = n + 1;
        _kputs_buf[n] = (char)c;
    }
}

// FUNC 0x0101CA78 _sio_putchar_crlf
//   Default putc hook (data word @0x0103DA30): '\n' -> sio_putc('\r') then
//   sio_putc('\n'); otherwise sio_putc(c).  Both paths tail-call sio_putc, so
//   $v0 = the character on return (unused).
static void _sio_putchar_crlf(int c)
{
    if (c == '\n') {
        sio_putc('\r');
        sio_putc('\n');
        return;
    }
    sio_putc(c);
}

// FUNC 0x0101CAB0 _dtoi
//   Treats the 64-bit argument as the bit pattern of an IEEE double and
//   converts it to int: e = biased exponent - 1075; e < -53 -> 0; e >= 13 ->
//   9999; else m = mantissa | 1<<52, shifted left by e, or (e < 0) right by
//   (-e - 2) (shift count taken mod 64, so e == -1 shifts by 63) and then by 2
//   more, adding 1 only when both dropped bits are 1 (fraction >= .75, ignoring
//   lower bits).  Returns the low 32 bits.  NB: its only caller passes an
//   INTEGER (see _print_double), not a double bit pattern.
//   The final sign extension sits in the `jr` delay slot in the binary: the
//   SDK's assembler filled unfilled delay slots (see sio_putc).
/* The 64-bit shifts take their count mod 64 (dsrlv/dsllv use its low 6 bits);
 * egcs emits exactly those instructions for a plain shift, which is what Sony
 * wrote.  In ISO C a count of -1 (e == 1) or >= 64 is undefined, so the
 * GCC 14 build masks it explicitly - the same result.                       */
#ifdef BOOT_OLD_GCC
#define DTOI_SHIFT(n) (n)
#else
#define DTOI_SHIFT(n) ((int)(n) & 63)
#endif
static int _dtoi(u64 x)
{
    s64 e;

    e = (x << 1) >> 53;
    e -= 0x433;
    if (e < -0x35)
        return 0;
    if (e >= 13)
        return 9999;
    x = (x << 12) >> 12;
    x |= (u64)1 << 52;
    if (e < 0) {
        e = -e;
        x >>= DTOI_SHIFT(e - 2);
        if ((x & 3) == 3)
            x = (x >> 2) + 1;
        else
            x >>= 2;
    } else {
        x <<= DTOI_SHIFT(e);
    }
    return (int)x;
}
#undef DTOI_SHIFT

// FUNC 0x0101CB40 _print_double
//   %e/%f back end of the kernel printf.  Negative: d = 0.0 - d (__subdf3) and
//   '-' through the putc hook.  Then scales d into [0.1, 1): while d < 0.1
//   d *= 10 (exp--), else while d >= 1.0 d /= 10 (exp++).  Prints
//   _printf("0.%d", _dtoi((u64)(d * 1e6))) and then "e+%d" (exp >= 0) or
//   "e%d".  BUG (faithful): __fixunsdfdi yields an integer < 1e6 that _dtoi
//   decodes as a double bit pattern (exponent field 0 -> returns 0), so every
//   non-zero value prints as "0.0e<exp>".  gcc 2.9x lowers every compare to
//   __cmpdf2 (NaN compares as "greater", so a NaN/Inf argument loops forever
//   in the /10 loop; the C conditions are written as !(d < x) so that modern
//   __ltdf2 reproduces this).  Constants: 0.1 @0x01040558 and @0x01040560, 1e6
//   @0x01040568; 10.0 and 1.0 are built inline.
static void _print_double(double d)
{
    int exp = 0;
    int n;

    if (d < 0.0) {
        d = 0.0 - d;
        _putc_hook('-');
    }
    if (d < 0.1) {
        while (d < 0.1) {
            d = d * 10.0;
            exp--;
        }
    } else if (!(d < 1.0)) {
        /* loop test is `__cmpdf2(d, 1.0) >= 0` (bgez), i.e. !(d < 1.0): a NaN
         * compares as 1 and keeps looping, so this is not `d >= 1.0`. */
        while (!(d < 1.0)) {
            d = d / 10.0;
            exp++;
        }
    }
    n = _dtoi((u64)(d * 1000000.0));
    _printf("0.%d", n);
    if (exp >= 0)
        _printf("e+%d", exp);
    else
        _printf("e%d", exp);
}

// FUNC 0x0101CCA8 _kernel_vprintf
//   The kernel printf engine.  `ap` points at 64-bit argument slots (the
//   callers spill $a1..$t3 with sd; each argument occupies one slot and is
//   read from its low bytes, as the EE's va_arg does - see KARG).  Everything
//   goes out through _putc_hook, one (signed) char at a time.  Conversion
//   switch = 0x49-entry jump table @0x01040570 indexed by (c - '0'):
//     '0'  width: "%0N" / "%0NN" (clamped to 31) pre-fills N '0's to the
//          left of the digit area and remembers where they start; a '0' not
//          followed by a digit is skipped.  fmt[2] is read before fmt[1] is
//          tested (one byte past a NUL right after "%0", as in the binary).
//          Only o/x/d/u use the padding.  (Width digits without a leading
//          '0', e.g. "%5d", are NOT understood: '5' hits the default case and
//          "d" prints literally.)
//     'l'  64-bit argument (ld), 'h' 16-bit (lhu / lh); default 32-bit.
//     'o'/'x'/'u'  unsigned octal / lower-case hex / decimal (__umoddi3,
//          __udivdi3); 'd' signed decimal (__moddi3, __divdi3) with '-'
//          emitted immediately, before any padding.  Each of the four emits
//          its own field (the binary has four copies of the output loop).
//     'e','f'  float slot (lwc1): 0.0f prints "0", else _print_double.
//     's'  prints "(null)" when the FIRST CHAR is NUL (sic - the pointer is
//          dereferenced first, a NULL pointer is not special-cased); no padding.
//     'c'  low byte of the slot, sign-extended.
//     anything else (incl. '%', '1'..'9', 'g', 'p', 'X') is swallowed silently
//          - including a NUL right after '%', 'l', 'h' or "%0": the pointer then
//          steps PAST the terminator and formatting continues in whatever
//          memory follows the string.
//   Returns 0: Sony's function has no return statement, so the binary just
//   leaves $v0 holding the loop test's load of the terminating NUL (scePrintf
//   passes it on).  egcs does exactly that without the return; GCC 14 gets
//   an explicit `return 0`.
#define KARG(ap, T) (*(T *)(void *)((ap += 1) - 1))  /* EE va_arg: 8-byte slots */
static int _kernel_vprintf(const char *fmt, u64 *ap)
{
    char  buf[32];      /* sp+0x00; digits built backwards from buf[30], buf[31] = NUL */
    char *p;            /* $s0 while emitting a field                              */
    char *pad;          /* $s4: first '0' of the "%0N" fill, NULL = none           */
    int   mod;          /* $a3: 0, 'l' or 'h'                                      */
    int   w;
    u64   uv;
    s64   sv;
    char  c;

    for (; *fmt != '\0'; fmt++) {
        c = *fmt;
        pad = NULL;
        mod = 0;
        if (c == '%') {
        again:
            switch (*++fmt) {
            case '0': {
                int c2;

                w = fmt[1] - '0';
                c2 = fmt[2];
                if ((u8)w < 10) {
                    if ((unsigned int)(c2 - '0') < 10) {
                        w = w * 10 + (c2 - '0');
                        fmt += 2;
                        if (!(w < 32))
                            w = 31;
                    } else {
                        fmt += 1;
                    }
                    pad = buf + (31 - w);
                    for (; w > 0; w--)
                        buf[31 - w] = '0';
                }
                goto again;
            }

            case 'l':
                mod = 'l';
                goto again;

            case 'h':
                mod = 'h';
                goto again;

            case 'o':
                if (mod == 'l')
                    uv = KARG(ap, u64);
                else if (mod == 'h')
                    uv = KARG(ap, u16);
                else
                    uv = KARG(ap, u32);
                p = &buf[31];
                *p = '\0';
                if (uv == 0) {
                    *--p = '0';
                } else {
                    while (uv != 0) {
                        *--p = (char)((uv & 7) + '0');
                        uv >>= 3;
                    }
                }
                if (pad != NULL && pad < p)
                    p = pad;
                while (*p != '\0')
                    _putc_hook((s8)*p++);
                break;

            case 'x':
                if (mod == 'l')
                    uv = KARG(ap, u64);
                else if (mod == 'h')
                    uv = KARG(ap, u16);
                else
                    uv = KARG(ap, u32);
                p = &buf[31];
                *p = '\0';
                if (uv == 0) {
                    *--p = '0';
                } else {
                    while (uv != 0) {
                        if ((uv & 0xF) < 10)
                            *--p = (char)((uv & 0xF) + '0');
                        else
                            *--p = (char)((uv & 0xF) + 'a' - 10);
                        uv >>= 4;
                    }
                }
                if (pad != NULL && pad < p)
                    p = pad;
                while (*p != '\0')
                    _putc_hook((s8)*p++);
                break;

            case 'd':
                if (mod == 'l')
                    sv = KARG(ap, s64);
                else if (mod == 'h')
                    sv = KARG(ap, s16);
                else
                    sv = KARG(ap, s32);
                p = &buf[31];
                *p = '\0';
                if (sv == 0) {
                    *--p = '0';
                } else {
                    if (sv < 0) {
                        sv = -sv;
                        _putc_hook('-');
                    }
                    while (sv != 0) {
                        *--p = (char)(sv % 10 + '0');
                        sv = sv / 10;
                    }
                }
                if (pad != NULL && pad < p)
                    p = pad;
                while (*p != '\0')
                    _putc_hook((s8)*p++);
                break;

            case 'u':
                if (mod == 'l')
                    uv = KARG(ap, u64);
                else if (mod == 'h')
                    uv = KARG(ap, u16);
                else
                    uv = KARG(ap, u32);
                p = &buf[31];
                *p = '\0';
                if (uv == 0) {
                    *--p = '0';
                } else {
                    while (uv != 0) {
                        *--p = (char)(uv % 10 + '0');
                        uv = uv / 10;
                    }
                }
                if (pad != NULL && pad < p)
                    p = pad;
                while (*p != '\0')
                    _putc_hook((s8)*p++);
                break;

            case 'e':
            case 'f': {
                float f = KARG(ap, float);

                if (f == 0.0f)
                    _putc_hook('0');
                else
                    _print_double((double)f);   /* __extendsfdf2 */
                break;
            }

            case 's': {
                const char *s = KARG(ap, const char *);

                if (*s == '\0') {
                    _putc_hook('(');
                    _putc_hook('n');
                    _putc_hook('u');
                    _putc_hook('l');
                    _putc_hook('l');
                    _putc_hook(')');
                } else {
                    p = (char *)s;
                    do {
                        _putc_hook((s8)*p);
                        p++;
                    } while (*p != '\0');
                }
                break;
            }

            case 'c':
                sv = KARG(ap, s8);
                _putc_hook((int)sv);
                break;

            default:
                break;
            }
        } else {
            _putc_hook(c);
        }
    }
#ifndef BOOT_OLD_GCC
    return 0;   /* see above: egcs leaves the loop test's 0 in $v0 */
#endif
}
#undef KARG

// FUNC 0x0101D270 _printf
//   Varargs shim: spills $a1..$t3 as 64-bit slots and runs _kernel_vprintf
//   with the CURRENT putc hook (SIO CR/LF unless scePrintf is active).
//   Callers: topThread's error message, _print_double, the DECI2 TTY
//   handler (0x01023200).  The original leaves _kernel_vprintf's result (0)
//   in $v0; no caller reads it, so it is declared void (eekernel.h).
void _printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    _kernel_vprintf(fmt, (u64 *)ap);   /* n32/EE: va_list = pointer to the 8-byte slots */
    va_end(ap);
}

// FUNC 0x0101D2A8 scePrintf
//   Kernel/TTY printf (symbols.csv: "printf"): saves the putc hook, installs
//   _kputs_putc (DECI2 kPuts line buffer), formats, restores the hook and
//   returns _kernel_vprintf's $v0 (0).  A trailing partial line stays in the
//   buffer until the next '\n'.  Callers: libcdvd debug output (9 call
//   sites, 0x01013160..0x01014150), Polyphony tty_puts (0x01012FC8, tail call).
int scePrintf(const char *fmt, ...)
{
    va_list ap;
    void (*saved)(int c);
    int ret;

    saved = _putc_hook;
    _putc_hook = _kputs_putc;
    va_start(ap, fmt);
    ret = _kernel_vprintf(fmt, (u64 *)ap);
    va_end(ap);
    _putc_hook = saved;
    return ret;
}
