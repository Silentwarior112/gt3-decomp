/* T519 - pdicdvd.cxx.  0x0025B490..0x0025BF38.
 *
 * Client of the pdicdvd IOP module: two RPC objects in the file's anonymous
 * namespace (_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7):
 *   RPC  @0x004A6B00, server 'PCDV' (0x50434456): disc reads (fno 3), the
 *        disc identity (fno 2), fno 1 (sent when an iso9660 lookup fails);
 *   RPC2 @0x004A6A80, server 'Pcdv' (0x50636476): the clock (fno 2), fno 1.
 * Reads are serialised by the semaphore g_003530E8 and end asynchronously:
 * RPC::onCallEnd signals doneSema, pdicdvd_read_wait takes it.
 *
 * The small methods are inline in the class (egcs emits their out-of-line
 * copies at the end of the file, after the static ctor/dtor); the free
 * functions below inline them. */
#include "engine/pdi/T519_pdicdvd.h"
#include "engine/pdi/pdi_internal.h"

#define PDICDVD_SERVER_PCDV 0x50434456  /* 'PCDV' */
#define PDICDVD_SERVER_PCDV_CLOCK 0x50636476 /* 'Pcdv' */

/* The anonymous namespace's name, "_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7" in the
 * original (type names @0x00345830 / 0x00345860): egcs names `namespace {'
 * after the first public symbol already assembled in the file, else after
 * the file cc1plus reads plus six random characters.  For the retail
 * compiler only, a zero-size public object with that assembler name comes
 * first, so egcs derives the original's name (u125's T533_printf.cpp has the
 * details); it emits no bytes.  It also names the static initialiser and
 * destructor (_GLOBAL_$I$ / $D$ .._.._pdicdvd.cxxs1Kop7; retail's are named
 * after RPC::RPC).  GCC 14 does not need it. */
#ifdef BOOT_OLD_GCC
extern const char pdicdvd_cxx_namespace_anchor[] __asm__(".._.._pdicdvd.cxxs1Kop7") = {};
#endif

namespace {   /* _GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7 */

/* Class order: RPC, then RPC2.  egcs emits the vtables, the typeinfo names
 * and the out-of-line inline members in reverse definition order, and
 * retail has RPC2's first (vtable 0x00345800, name 0x00345830, getTime
 * 0x0025BC08 before RPC::isPending).  Within a class too: each class's
 * inline members come last first (F_0025BEC8 .. isPending gives retail's
 * isPending .. F_0025BEC8; the frame info in .data shows the order). */
/* RPC - vtable 0x00345818 (slot 1 = RPC::onCallEnd); 0xC0 bytes. */
class RPC : public PDIClientRpc {
public:
    int pending;                /* 0x3C a read is in flight */
    int doneSema;               /* 0x40 create_mutex_sema(0), signalled when a read ends */
    u8  pad_44[0xC];            /* 0x44 */
    union {                     /* 0x50 sendBuf (sendSize 0x10) */
        u32 w[4];
        struct {
            u32   lsn;          /* 0x50 */
            u32   size;         /* 0x54 bytes */
            void *buf;          /* 0x58 */
            int   mode;         /* 0x5C */
        } read;                 /* fno 3 */
        PdiDiscId discId;       /* fno 2 */
    } send;
    u8  pad_60[0x20];           /* 0x60 */
    u32 recv[0x10];             /* 0x80 answer of the reads: recv[0] == 0 = OK */

    RPC();                                                  /* 0x0025B490 */
    virtual ~RPC();                                         /* [0] +0x0C 0x0025B510 */
    virtual void onCallEnd();                               /* [1] +0x14 0x0025B538 */

    // FUNC 0x0025BEC8 (pdicdvd)::RPC::F_0025BEC8
    void F_0025BEC8()
    {
        lockedCall(1, SIF_RPC_M_NOWAIT);
    }

    /* No callers.  fno 0, answer read into a 64-byte aligned stack buffer. */
    // FUNC 0x0025BE70 (pdicdvd)::RPC::F_0025BE70
    int F_0025BE70()
    {
        u8 area[0x80];
        u32 *answer = (u32 *)RPC_ALIGN64(area);

        WaitSema(sema);
        call(0, 0, answer, 0x40);
        return answer[0];
    }

    // FUNC 0x0025BE10 (pdicdvd)::RPC::setDiscId
    void setDiscId(const PdiDiscId *id)
    {
        WaitSema(sema);
        send.discId = *id;
        call(2, SIF_RPC_M_NOWAIT, NULL, 0);
    }

    /* Starts reading `size' bytes at `lsn' into buf (NOWAIT). */
    // FUNC 0x0025BD68 (pdicdvd)::RPC::read
    void read(void *buf, u32 lsn, u32 size, int mode)
    {
        WaitSema(g_003530E8);
        WaitSema(sema);
        pending = 1;
        send.read.lsn  = lsn;
        send.read.size = size;
        send.read.buf  = buf;
        send.read.mode = mode;
        SyncDCache(buf, (u8 *)buf + size);
        call(3, SIF_RPC_M_NOWAIT, recv, 0x40);
        signal_sema_helper(g_003530E8);
    }

    // FUNC 0x0025BD38 (pdicdvd)::RPC::waitRead
    int waitRead()
    {
        WaitSema(doneSema);
        return recv[0] == 0;
    }

    // FUNC 0x0025BCF0 (pdicdvd)::RPC::isPending
    int isPending()
    {
        WaitSema(g_003530E8);
        int inFlight = pending;
        signal_sema_helper(g_003530E8);
        return inFlight;
    }
};
static_assert(sizeof(RPC) == 0xC0, "pdicdvd RPC");

/* RPC2 - vtable 0x00345800 (slot 1 = PDIClientRpc::onCallEnd); 0x80 bytes. */
class RPC2 : public PDIClientRpc {
public:
    u32 pad_3C;                 /* 0x3C */
    u32 send[0x10];             /* 0x40 sendBuf (sendSize 4, never filled) */

    RPC2();                                                 /* 0x0025B568 */
    virtual ~RPC2();                                        /* [0] +0x0C 0x0025B5B0 */

    // FUNC 0x0025BC80 (pdicdvd)::RPC2::F_0025BC80
    void F_0025BC80()
    {
        lockedCall(1, SIF_RPC_M_NOWAIT);
    }

    /* Not inlined (alloca): the clock, {seconds since 2000-01-01, stat}. */
    // FUNC 0x0025BC08 (pdicdvd)::RPC2::getTime
    u64 getTime()
    {
        u32 *answer = (u32 *)RPC_ALIGN64(__builtin_alloca(0x80));

        WaitSema(sema);
        call(2, 0, answer, 0x40);
        return ((u64)answer[1] << 32) | answer[0];
    }
};
static_assert(sizeof(RPC2) == 0x80, "pdicdvd RPC2");

/* static: egcs puts them in .bss at the end of the unit, in reverse order
 * (RPC2 below RPC, as retail); public objects with a constructor would go
 * to .data.  The static initialiser / destructor below are egcs's. */
static RPC  g_004A6B00;         /* 0x004A6B00 'PCDV' */
static RPC2 g_004A6A80;         /* 0x004A6A80 'Pcdv' */

} /* namespace */

/* type_info storage of the two classes (__si_type_info, 3 words) */
extern u32 g_004B2AB0[3];       /* 0x004B2AB0 __tiQ234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop74RPC2 */
extern u32 g_004B2AC0[3];       /* 0x004B2AC0 __tiQ234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop73RPC */

/* An exception from create_mutex_sema destroys the PDIClientRpc base again
 * (landing pad 0x0025B4E4). */
// FUNC 0x0025B490 (pdicdvd)::RPC::RPC
RPC::RPC()
{
    /* implicit: PDIClientRpc (the base), ~PDIClientRpc (landing pad) */
    pending  = 0;
    sendBuf  = &send;
    sendSize = 0x10;
    doneSema = create_mutex_sema(0);
}

/* index.txt: _GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7::RPC::virtual_0. */
// FUNC 0x0025B510 (pdicdvd)::RPC::~RPC
RPC::~RPC()
{
    /* implicit: ~PDIClientRpc (the base) */
}

/* index.txt: _GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7::RPC::virtual_1. */
// FUNC 0x0025B538 (pdicdvd)::RPC::onCallEnd
void RPC::onCallEnd()
{
    if (pending) {
        pending = 0;
        signal_sema_helper(doneSema);
    }
}

// FUNC 0x0025B568 (pdicdvd)::RPC2::RPC2
RPC2::RPC2()
{
    /* implicit: PDIClientRpc (the base) */
    sendBuf  = send;
    sendSize = 4;
}

/* index.txt: _GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7::RPC2::virtual_0. */
// FUNC 0x0025B5B0 (pdicdvd)::RPC2::~RPC2
RPC2::~RPC2()
{
    /* implicit: ~PDIClientRpc (the base) */
}

/* Lock of the disc reads (the RPC methods above), a semaphore made by
 * pdicdvd_init; -1 before.  .sdata (-G8). */
int g_003530E8 = -1;            /* 0x003530E8 */

/* Caller: RaceIOPLoadModules::cmdBindClients, after the pdi modules are
 * loaded. */
// FUNC 0x0025B5D8 pdicdvd_init
void pdicdvd_init(void)
{
    g_003530E8 = create_mutex_sema(1);
    g_004A6B00.bind(PDICDVD_SERVER_PCDV);
    g_004A6A80.bind(PDICDVD_SERVER_PCDV_CLOCK);
}

/* Caller: iso9660_find, when the lookup fails. */
// FUNC 0x0025B620 F_0025B620
void F_0025B620(void)
{
    g_004A6B00.F_0025BEC8();                    /* implicit: lockedCall (inline) */
}

/* Caller: iso9660_read_pvd. */
// FUNC 0x0025B648 pdicdvd_set_disc_id
void pdicdvd_set_disc_id(const PdiDiscId *id)
{
    g_004A6B00.setDiscId(id);                   /* implicit: WaitSema, call (inline setDiscId) */
}

/* Callers: vol_mount (0x0023DE10), sector_cache_read.  Blocking read;
 * returns 1 when it succeeded. */
// FUNC 0x0025B6A8 pdicdvd_read
int pdicdvd_read(void *buf, u32 lsn, u32 size, int mode)
{
    pdicdvd_read_start(buf, lsn, size, mode);
    return pdicdvd_read_wait();
}

// FUNC 0x0025B6D0 pdicdvd_read_start
void pdicdvd_read_start(void *buf, u32 lsn, u32 size, int mode)
{
    g_004A6B00.read(buf, lsn, size, mode);      /* implicit: WaitSema, SyncDCache, call, signal_sema_helper (inline read) */
}

// FUNC 0x0025B780 pdicdvd_read_wait
int pdicdvd_read_wait(void)
{
    return g_004A6B00.waitRead();               /* implicit: WaitSema (inline waitRead) */
}

/* No callers. */
// FUNC 0x0025B7B8 pdicdvd_read_pending
int pdicdvd_read_pending(void)
{
    return g_004A6B00.isPending();              /* implicit: WaitSema, signal_sema_helper (inline isPending) */
}

/* No callers. */
// FUNC 0x0025B800 F_0025B800
void F_0025B800(void)
{
    g_004A6A80.F_0025BC80();                    /* implicit: lockedCall (inline) */
}

/* Caller: clock_update (0x0022BA48). */
// FUNC 0x0025B828 pdicdvd_get_time
u64 pdicdvd_get_time(void)
{
    return g_004A6A80.getTime();                /* implicit: F_0025BC08 (RPC2::getTime, out of line) */
}

#define SECONDS_PER_DAY  86400
#define DAYS_PER_4_YEARS 1461           /* 3 x 365 + 366 */

/* Two BCD digits.  The tens digit is a signed `% 10' of the unsigned
 * quotient (div after divu in the listing). */
#define PDI_BCD(v) ((u8)((((int)((v) / 10) % 10) << 4) | ((v) % 10)))

/* First day of each month: [0] normal, [1] leap year.  Not const: retail
 * has it in .data (a const table would be in .rodata). */
u16 g_002FC4B0[2][12] = {       /* 0x002FC4B0 */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 },
};

/* First day of each year of a 4-year group (2000 is a leap year).  .sdata
 * (-G8), after g_003530E8. */
const u16 g_003530F0[4] = { 0, 366, 731, 1096 };   /* 0x003530F0 */

/* Converts t (low word: seconds since 2000-01-01 00:00; high word: the stat
 * byte) into a CdClock and returns its stat.  Callers: clock_update
 * (0x0022BA48), pdicdvd_time_to_clock_gmt / _local.
 * Not exact (81.8%, the same with the reference compiler 991111-01): the
 * register assignment differs from the first division on (retail's seconds
 * in $a2, the 60 / 1461 divisor copies in other registers), and the year
 * loop's `yearInGroup = next' sits at the loop label instead of in a
 * `beql' delay slot (loop.c calls that loop phony).  Tried: the year and
 * month loops as for / while / do / break / goto forms, an explicit `next',
 * int counters, a table pointer, the BCD stores with day / month computed
 * in place or month++ first (worse: 52-74%). */
// FUNC 0x0025B848 pdicdvd_time_to_clock
u8 pdicdvd_time_to_clock(CdClock *out, u64 t)
{
    u32 seconds     = (u32)t;
    u32 days        = seconds / SECONDS_PER_DAY;
    u32 secOfDay    = seconds % SECONDS_PER_DAY;
    u32 minOfDay    = secOfDay / 60;
    u32 second      = secOfDay % 60;
    u32 dayInGroup  = days % DAYS_PER_4_YEARS;      /* day in the 4-year group */
    u32 groups      = days / DAYS_PER_4_YEARS;
    u32 hour        = minOfDay / 60;
    u32 minute      = minOfDay % 60;
    u32 yearInGroup, year, dayOfYear, month, day;
    const u16 *monthStart;

    for (yearInGroup = 0;
         yearInGroup < 3 && dayInGroup >= g_003530F0[yearInGroup + 1];
         yearInGroup++)
        ;
    dayOfYear = dayInGroup - g_003530F0[yearInGroup];
    year = yearInGroup + groups * 4;            /* since 2000 */
    /* Quirk: only year 0 (2000) uses the leap-year month table; 2004, 2008...
     * use the normal one although their group year is 0 as well (movz on the
     * full year). */
    monthStart = (year == 0) ? g_002FC4B0[1] : g_002FC4B0[0];
    for (month = 0; month < 11 && dayOfYear >= monthStart[month + 1]; month++)
        ;
    day = dayOfYear - monthStart[month] + 1;

    out->stat   = (u8)(t >> 32);
    out->second = PDI_BCD(second);
    out->minute = PDI_BCD(minute);
    out->pad    = 0;
    out->hour   = PDI_BCD(hour);
    out->day    = PDI_BCD(day);
    out->month  = PDI_BCD(month + 1);
    out->year   = PDI_BCD(year);
    return out->stat;
}

/* Caller: clock_update (0x0022BA48). */
// FUNC 0x0025BB20 pdicdvd_time_to_clock_gmt
u8 pdicdvd_time_to_clock_gmt(CdClock *out, u64 t)
{
    u8 stat = pdicdvd_time_to_clock(out, t);

    scf_clock_to_gmt((sceCdCLOCK *)out);        /* libscf's type, the same layout */
    return stat;
}

/* Callers: gt2k_menu::RecordsButton::virtual_2 (0x00173D20), clock_update
 * (0x0022BA48). */
// FUNC 0x0025BB60 pdicdvd_time_to_clock_local
u8 pdicdvd_time_to_clock_local(CdClock *out, u64 t)
{
    u8 stat = pdicdvd_time_to_clock(out, t);

    scf_clock_to_local((sceCdCLOCK *)out);      /* libscf's type, the same layout */
    return stat;
}

/* Compiler-generated: egcs's static destructor and initialiser of the two
 * objects, emitted here, after the last out-of-line function.  Retail's are
 * named after the unit's first global, RPC::RPC
 * (_GLOBAL_$D$__Q234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop73RPC); ours after the
 * namespace anchor above. */
// FUNC 0x0025BBA0 T519_static_fini (_GLOBAL_$D$.._.._pdicdvd.cxxs1Kop7)
//   (.dtors entry 0x0028F28C) ~RPC2, ~RPC (flags 2).
/* implicit: ~RPC2, ~RPC */
// FUNC 0x0025BBD8 T519_static_init (_GLOBAL_$I$.._.._pdicdvd.cxxs1Kop7)
//   (.ctors entry 0x0028F1BC) RPC(), RPC2().
/* implicit: RPC, RPC2 */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo functions (egcs emits them itself, with the
 * retail namespace name thanks to the anchor at the top). */
// FUNC 0x0025BCA0 __tfQ234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop74RPC2
extern "C" void *tf_pdicdvd_RPC2(void)
{
    if (g_004B2AB0[0] == 0) {
        __tf12PDIClientRpc();
        __rtti_si(g_004B2AB0, "Q234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop74RPC2",
                  (const type_info *)__ti12PDIClientRpc);
    }
    return g_004B2AB0;
}

// FUNC 0x0025BEE8 __tfQ234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop73RPC
extern "C" void *tf_pdicdvd_RPC(void)
{
    if (g_004B2AC0[0] == 0) {
        __tf12PDIClientRpc();
        __rtti_si(g_004B2AC0, "Q234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop73RPC",
                  (const type_info *)__ti12PDIClientRpc);
    }
    return g_004B2AC0;
}
#endif
