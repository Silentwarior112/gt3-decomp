/* kernel.c - Sony libkernl (PsIIlibkernl2240) part 1, C side,
 * 0x00286408..0x002869E7, in link (object) order:
 *   cache.o    0x00286408  _SyncDCache / SyncDCache / iSyncDCache,
 *                          _InvalidDCache / InvalidDCache / iInvalidDCache
 *   intr.o     0x00286688  DisableIntc / EnableIntc / DisableDmac / EnableDmac
 *                          and the interrupt-context i-forms
 *   alarm.o    0x002868A8  private SetSyscall/Copy/GetEntryAddress stubs, the
 *                          kernel-mode Copy service, InitAlarm
 * thread.o (topThread, 0x002869E8..) follows in the next unit.
 *
 * The same machine code as the bootstrap's sdk/libkernl/kernel.c
 * (0x0101C050..0x0101C62F), reused.  alarm.o's .data (0x00307A18..
 * 0x003081B8) is defined before InitAlarm, as in the bootstrap source and
 * under its names: the patch blob (_alarm_patch_code, 0x00307A18), the
 * callback trampoline (_alarm_patch_stub, 0x00308158) and the install table
 * (_alarm_syscalls, 0x00308178), with the original's words.  cache.o and
 * intr.o have no data.
 *
 * index.txt calls the alarm.o private stubs at 0x002868A8 / 0x00286900
 * "SetSyscall" / "GetEntryAddress", the names of the table stubs at
 * 0x00285FE0 / 0x00285E10; they keep the bootstrap's unique names
 * _alarm_SetSyscall / _alarm_GetEntryAddress.
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part3_internal.h"

/* ======================================================================== */
/* Hardware registers used here                                             */
/* ======================================================================== */

#define T3_MODE      0x10001810   /* EE timer 3 mode register                  */
#define T3_MODE_CMPE 0x100        /* compare-interrupt enable (set by the alarm patch: 0x583) */

/* alarm.o: where InitAlarm puts the patch (bootstrap kernel.c). */
#define ALARM_PATCH_ADDR    0x80076000  /* kseg0 kernel RAM: _alarm_patch_code */
#define ALARM_PATCH_SIZE    0x740
#define ALARM_STUB_ADDR     0x00082000  /* user RAM: _alarm_patch_stub (callback trampoline) */
#define ALARM_STUB_SIZE     0x20

/* ======================================================================== */
/* cache.o                                                                  */
/* ======================================================================== */

/* R5900 D-cache: 64-byte lines, 64 sets (index 0..0xFC0), 2 ways (way = bit 0
 * of the cache-op address).  COP0 $28 = TagLo, bits 31..12 = physical tag.
 *   cache 0x10 DXLTG  index load tag      cache 0x14 DXWBIN index writeback-invalidate
 *   cache 0x16 DXIN   index invalidate
 * Every op is bracketed by sync.l exactly as in the binary.                  */
#define DCACHE_LINE        0x40         /* bytes per line; also the index step */
#define DCACHE_WAY_SIZE    0x1000       /* 64 sets x 64 bytes: the index range */
#define TAGLO_PTAG         0xFFFFF000   /* TagLo physical tag (address bits 31..12) */
#define DCACHE_LINE_MASK   (~(DCACHE_LINE - 1))   /* 0xFFFFFFC0 */
#define DCACHE_LOAD_TAG(idx, way, tag)                                        \
    __asm__ __volatile__("sync.l\n\tcache 0x10, " #way "(%1)\n\tsync.l\n\tmfc0 %0, $28" \
                         : "=r"(tag) : "r"(idx) : "memory")
#define DCACHE_INDEX_OP(op, idx, way)                                         \
    __asm__ __volatile__("sync.l\n\tcache " #op ", " #way "(%0)\n\tsync.l"   \
                         : : "r"(idx) : "memory")
#define SYNC_L() __asm__ __volatile__("sync.l" : : : "memory")

// FUNC 0x00286408 _SyncDCache
//   Index-walks both ways of all 64 D-cache sets; for every line whose
//   (TagLo & TAGLO_PTAG) + index lies in [start, end] (unsigned, inclusive)
//   does DXWBIN (write back + invalidate).  The valid/dirty bits are not
//   looked at.  Callers pass 64-byte-aligned bounds.
static void _SyncDCache(void *start, void *end)
{
    int idx;
    u32 tag, addr;

    for (idx = 0; idx < DCACHE_WAY_SIZE; idx += DCACHE_LINE) {
        DCACHE_LOAD_TAG(idx, 0, tag);
        addr = (tag & TAGLO_PTAG) + idx;
        if (!(addr < (u32)start) && !((u32)end < addr))
            DCACHE_INDEX_OP(0x14, idx, 0);
        DCACHE_LOAD_TAG(idx, 1, tag);
        addr = (tag & TAGLO_PTAG) + idx;
        if (!(addr < (u32)start) && !((u32)end < addr))
            DCACHE_INDEX_OP(0x14, idx, 1);
        SYNC_L();
    }
}

// FUNC 0x002864B0 SyncDCache
//   Thread-context wrapper: if COP0 Status.EIE was set, DIntr() before and
//   EIntr() (tail call) after; bounds are rounded down to 64 bytes.
//   10 callers: the pdi* IOP RPC clients before a SIF transfer
//   (sif_call_rpc, pdicdvd_read_start, pdispu2/pdimpeg/pdipipe ...) and
//   libpad (pad_act_dma_transfer, pad_get_recv_buf).
void SyncDCache(void *start, void *end)
{
    u32 eie;

    __asm__ __volatile__("mfc0 %0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DIntr();
    _SyncDCache((void *)((u32)start & DCACHE_LINE_MASK), (void *)((u32)end & DCACHE_LINE_MASK));
    if (eie)
        EIntr();
}

// FUNC 0x00286530 iSyncDCache
//   Interrupt-context form: align both bounds to 64 bytes and tail-jump into
//   _SyncDCache without touching the interrupt state.
void iSyncDCache(void *start, void *end)
{
    _SyncDCache((void *)((u32)start & DCACHE_LINE_MASK), (void *)((u32)end & DCACHE_LINE_MASK));
}

// FUNC 0x00286548 _InvalidDCache
//   Same tag walk as _SyncDCache but the per-line op is DXIN (cache 0x16):
//   invalidate WITHOUT write-back.
static void _InvalidDCache(void *start, void *end)
{
    int idx;
    u32 tag, addr;

    for (idx = 0; idx < DCACHE_WAY_SIZE; idx += DCACHE_LINE) {
        DCACHE_LOAD_TAG(idx, 0, tag);
        addr = (tag & TAGLO_PTAG) + idx;
        if (!(addr < (u32)start) && !((u32)end < addr))
            DCACHE_INDEX_OP(0x16, idx, 0);
        DCACHE_LOAD_TAG(idx, 1, tag);
        addr = (tag & TAGLO_PTAG) + idx;
        if (!(addr < (u32)start) && !((u32)end < addr))
            DCACHE_INDEX_OP(0x16, idx, 1);
        SYNC_L();
    }
}

// FUNC 0x002865F0 InvalidDCache
//   DIntr/EIntr-guarded wrapper of _InvalidDCache (same shape as SyncDCache).
//   No callers.
void InvalidDCache(void *start, void *end)
{
    u32 eie;

    __asm__ __volatile__("mfc0 %0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DIntr();
    _InvalidDCache((void *)((u32)start & DCACHE_LINE_MASK), (void *)((u32)end & DCACHE_LINE_MASK));
    if (eie)
        EIntr();
}

// FUNC 0x00286670 iInvalidDCache
//   Interrupt-context form: align and tail-jump into _InvalidDCache.  Unused.
void iInvalidDCache(void *start, void *end)
{
    _InvalidDCache((void *)((u32)start & DCACHE_LINE_MASK), (void *)((u32)end & DCACHE_LINE_MASK));
}

/* ======================================================================== */
/* intr.o - INTC / DMAC mask wrappers                                       */
/* ======================================================================== */

// FUNC 0x00286688 DisableIntc
//   Status.EIE sampled; DIntr() if set; _DisableIntc (#0x15); sync.l;
//   EIntr() if EIE was set; returns the kernel's result.
//   Callers: sceGsResetGraph (0x0028E5D0), music_term, ipu_sync_init /
//   ipu_sync_term, ipu_decode_image.
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

// FUNC 0x002866F0 EnableIntc
//   As DisableIntc with _EnableIntc (#0x14).  Callers: gfx_init (0x00250820),
//   music_init, ipu_sync_arm, ipu_decode_image, usbd_init.
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

// FUNC 0x00286758 DisableDmac
//   As DisableIntc with _DisableDmac (#0x17).  Caller: sceSifExitCmd
//   (0x00287ED8).
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

// FUNC 0x002867C0 EnableDmac
//   As DisableIntc with _EnableDmac (#0x16).  Callers: gfx_dma_init
//   (0x00250E68), sceSifInitCmd.
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

// FUNC 0x00286828 iEnableIntc
//   _iEnableIntc (#-0x1A) then sync.l; returns the kernel's result.  Unused.
int iEnableIntc(int cause)
{
    int ret = _iEnableIntc(cause);
    SYNC_L();
    return ret;
}

// FUNC 0x00286848 iDisableIntc
//   _iDisableIntc (#-0x1B) then sync.l.  Callers: the IPU interrupt handlers
//   ipu_sync_intc_handler (0x00264AA8) and ipu_intc_handler (0x002654E8).
int iDisableIntc(int cause)
{
    int ret = _iDisableIntc(cause);
    SYNC_L();
    return ret;
}

// FUNC 0x00286868 iEnableDmac
//   _iEnableDmac (#-0x1C) then sync.l.  Unused.
int iEnableDmac(int channel)
{
    int ret = _iEnableDmac(channel);
    SYNC_L();
    return ret;
}

// FUNC 0x00286888 iDisableDmac
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

// FUNC 0x002868A8 _alarm_SetSyscall
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

// FUNC 0x002868B8 _alarm_Copy
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

// FUNC 0x002868C8 _kCopy_handler
//   Kernel-mode handler that InitAlarm installs as syscall 0x5A (table word
//   @0x0030817C, its only reference).  Copies nbytes>>2 words (logical
//   shift, unsigned count), returns 0.  static in the bootstrap source; here
//   global, as runtime/newlib/G007_stdio.h declares it (the symbol's binding changes
//   nothing in the code or the table's word).
int _kCopy_handler(u32 *dst, const u32 *src, int nbytes)
{
    unsigned int n = (unsigned int)nbytes >> 2;
    unsigned int i;

    for (i = 0; i < n; i++)
        *dst++ = *src++;
    return 0;
}

// FUNC 0x00286900 _alarm_GetEntryAddress
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

/* alarm.o's .data, 0x00307A18..0x003081B8 (file statics, as in the
 * bootstrap's kernel.c; only InitAlarm uses them).  Every word is the
 * original's; the patch and the trampoline are machine code for the kernel
 * (instruction words and kernel addresses, no pointers into this program).
 *
 * The alarm kernel patch, 0x740 bytes of EE kernel-mode code, copied to
 * 0x80076000.  Layout (kernel addresses):
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
 * The same words as the bootstrap's _alarm_patch_code (0x0103D288). */
static u32 _alarm_patch_code[ALARM_PATCH_SIZE / 4] = {             /* 0x00307A18 */
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

/* The alarm callback trampoline, copied to 0x00082000:
 *   lui $sp,8; jalr $v1; addiu $sp,$sp,0x1FC0 (sp = 0x81FC0); li $v1,-8; syscall; 0; 0; 0
 * i.e. callback(a0, a1, a2) on a private stack, then `syscall -8' back into
 * the patch's service 0x08. */
static u32 _alarm_patch_stub[ALARM_STUB_SIZE / 4] = {              /* 0x00308158 */
    0x3C1D0008, 0x0060F809, 0x27BD1FC0, 0x2403FFF8,
    0x0000000C, 0x00000000, 0x00000000, 0x00000000,
};

/* InitAlarm's install table: the syscall slots it points at the patch. */
static struct _alarm_syscall _alarm_syscalls[8] = {                     /* 0x00308178 */
    { 0x5A,  (void *)_kCopy_handler },          /* kernel-mode Copy                     */
    { 0x5B,  (void *)ALARM_PATCH_ADDR },        /* patch+0x000: GetEntryAddress service */
    { 0xFC,  0 },                               /* SetAlarm                             */
    { 0xFE,  0 },                               /* iSetAlarm (-0xFE)                    */
    { 0xFD,  0 },                               /* ReleaseAlarm                         */
    { 0xFF,  0 },                               /* iReleaseAlarm (-0xFF)                */
    { 0x12C, 0 },                               /* T3 compare interrupt handler slot    */
    { 0x08,  0 },                               /* return from callback (syscall -8)    */
};

// FUNC 0x00286910 InitAlarm
//   Installs the SDK 2.2 "new alarm" kernel patch.  Skipped when T3_MODE.CMPE
//   is already set (patch already active).  Otherwise: point syscall 0x5A at
//   _kCopy_handler, Copy the 0x740-byte patch (_alarm_patch_code) to
//   0x80076000 and the 0x20-byte trampoline (_alarm_patch_stub) to
//   0x00082000, FlushCache(0) + FlushCache(2), point 0x5B at the patch's
//   lookup routine, then for services 0xFC,0xFE,0xFD,0xFF,0x12C,0x08
//   SetSyscall(n, GetEntryAddress(n)) (entries 2..7 of _alarm_syscalls).
//   Called by _InitSys (0x0028D750).
void InitAlarm(void)
{
    unsigned int i;

    if (REG32(T3_MODE) & T3_MODE_CMPE)
        return;

    _alarm_SetSyscall(_alarm_syscalls[0].num, _alarm_syscalls[0].handler);
    _alarm_Copy((void *)ALARM_PATCH_ADDR, _alarm_patch_code, ALARM_PATCH_SIZE);
    _alarm_Copy((void *)ALARM_STUB_ADDR, _alarm_patch_stub, ALARM_STUB_SIZE);
    FlushCache(WRITEBACK_DCACHE);
    FlushCache(INVALIDATE_ICACHE);
    _alarm_SetSyscall(_alarm_syscalls[1].num, _alarm_syscalls[1].handler);
    for (i = 2; i < sizeof(_alarm_syscalls) / sizeof(_alarm_syscalls[0]); i++)
        _alarm_SetSyscall(_alarm_syscalls[i].num,
                          _alarm_GetEntryAddress(_alarm_syscalls[i].num));
}
