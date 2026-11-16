/* T521 - pdibgm.cxx.  0x0025C830..0x0025CE00.
 *
 * Client of the pdibgm IOP module (streamed background music): one object
 * of the anonymous-namespace class RPC (_GLOBAL_$N$.._.._pdibgm.cxxpLWmzt::RPC)
 * at 0x004A72E0, server 'PBGM' (0x5042474D).  Functions: 1 start, 2 stop,
 * 3 volume, 4 pause, 5 resume, 6 status (copied to g_002FCE00 by
 * pdibgm_update, read by pdibgm_get_status).  Users: RaceBGMPS2 (T130),
 * ads_play / ads_set_volume / ads_stop / music_thread (T466, 0x0022AC98 ..
 * 0x0022AFF8), mpegif::feed (0x0025EF00). */
#include "engine/pdi/T521_pdibgm.h"
#include "engine/pdi/pdi_internal.h"

#define PDIBGM_SERVER 0x5042474D        /* 'PBGM' */

/* x clamped to [lo, hi]: max.s then min.s, volatile asm statements as every
 * max.s / min.s of the image behaves (u018's T055, u032's T077); an inline
 * clamp, as the listing loads both bounds before the max.s.  GCC 14: the C
 * ternaries (max.s / min.s differ only for -0.0 against 0.0). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline f32 bgm_maxf(f32 a, f32 b)
{
    f32 r;
    __asm__ __volatile__("max.s %0,%1,%2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}
static inline f32 bgm_minf(f32 a, f32 b)
{
    f32 r;
    __asm__ __volatile__("min.s %0,%1,%2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}
#else
static inline f32 bgm_maxf(f32 a, f32 b) { return a > b ? a : b; }
static inline f32 bgm_minf(f32 a, f32 b) { return a < b ? a : b; }
#endif
static inline f32 bgm_clampf(f32 x, f32 lo, f32 hi)
{
    return bgm_minf(bgm_maxf(x, lo), hi);
}

/* The anonymous namespace's name, "_GLOBAL_$N$.._.._pdibgm.cxxpLWmzt" in the
 * original (type name @0x003458F0): a zero-size public object with that
 * assembler name comes first, for the retail compiler only, so egcs derives
 * it instead of six random characters (see T519 / u125's T533_printf.cpp). */
#ifdef BOOT_OLD_GCC
extern const char pdibgm_cxx_namespace_anchor[] __asm__(".._.._pdibgm.cxxpLWmzt") = {};
#endif

namespace {   /* _GLOBAL_$N$.._.._pdibgm.cxxpLWmzt */

/* The out-of-line copies of RPC's inline members come in reverse definition
 * order (T519): defined last first, so updateStatus .. pause as retail. */

/* RPC - vtable 0x003458D8 (slot 1 = PDIClientRpc::onCallEnd); 0xC0 bytes. */
class RPC : public PDIClientRpc {
public:
    u32 pad_3C;                 /* 0x3C */
    union {                     /* 0x40 sendBuf (sendSize 0x80) */
        u32 w[0x20];            /* fno 3: {volume} */
        struct {
            int  lsn;           /* 0x40 */
            int  sectors;       /* 0x44 */
            int  arg;           /* 0x48 */
            int  bufSize;       /* 0x4C */
            char hostName[0x70]; /* 0x50 "" = the disc */
        } start;                /* fno 1 */
    } send;

    RPC();                                                  /* 0x0025C830 */
    virtual ~RPC();                                         /* [0] +0x0C 0x0025C878 */

    // FUNC 0x0025CD90 (pdibgm)::RPC::pause
    void pause()
    {
        lockedCall(4, SIF_RPC_M_NOWAIT);
    }

    // FUNC 0x0025CD70 (pdibgm)::RPC::resume
    void resume()
    {
        lockedCall(5, SIF_RPC_M_NOWAIT);
    }

    // FUNC 0x0025CD50 (pdibgm)::RPC::stop
    void stop()
    {
        lockedCall(2, SIF_RPC_M_NOWAIT);
    }

    // FUNC 0x0025CC98 (pdibgm)::RPC::start
    void start(int lsn, int sectors, int arg, int bufSize, const char *hostName)
    {
        WaitSema(sema);
        g_002FCE00.time  = -1;
        g_002FCE00.ended = 0;
        send.start.lsn     = lsn;
        send.start.sectors = sectors;
        send.start.arg     = arg;
        send.start.bufSize = bufSize;
        send.start.hostName[0] = 0;
        if (hostName != NULL)
            strcpy(send.start.hostName, hostName);
        call(1, SIF_RPC_M_NOWAIT, NULL, 0);
    }

    // FUNC 0x0025CC48 (pdibgm)::RPC::setVolume
    void setVolume(int volume)
    {
        WaitSema(sema);
        send.w[0] = volume;
        call(3, SIF_RPC_M_NOWAIT, NULL, 0);
    }

    /* Not inlined (alloca).  Caller: pdibgm_update. */
    // FUNC 0x0025CB88 (pdibgm)::RPC::updateStatus
    void updateStatus()
    {
        BgmStatus *answer = (BgmStatus *)RPC_ALIGN64(__builtin_alloca(0x80));

        WaitSema(sema);
        call(6, 0, answer, sizeof(BgmStatus));
        int wasEnabled = intr_suspend();
        g_002FCE00 = *answer;
        intr_resume(wasEnabled);
    }
};
static_assert(sizeof(RPC) == 0xC0, "pdibgm RPC");

static RPC g_004A72E0;          /* 0x004A72E0 'PBGM' (.bss, see T519) */

} /* namespace */

/* The stream status (pdibgm_update copies the server's answer here).  A
 * public C++ global without an initialiser: .data, at the end of the file's
 * variables; aligned (at least 16: the 8 bytes from 0x002FCDF8, after
 * pdispu2's frame info, are padding). */
BgmStatus g_002FCE00 __attribute__((aligned(64)));    /* 0x002FCE00 */

extern u32 g_004B2AE0[3];       /* 0x004B2AE0 __tiQ233_GLOBAL_$N$.._.._pdibgm.cxxpLWmzt3RPC */

// FUNC 0x0025C830 (pdibgm)::RPC::RPC
RPC::RPC()
{
    /* implicit: PDIClientRpc (the base) */
    sendBuf  = &send;
    sendSize = sizeof(send);    /* 0x80 */
}

/* index.txt: _GLOBAL_$N$.._.._pdibgm.cxxpLWmzt::RPC::virtual_0. */
// FUNC 0x0025C878 (pdibgm)::RPC::~RPC
RPC::~RPC()
{
    /* implicit: ~PDIClientRpc (the base) */
}

/* Caller: RaceIOPLoadModules::cmdBindClients. */
// FUNC 0x0025C8A0 pdibgm_init
void pdibgm_init(void)
{
    g_004A72E0.bind(PDIBGM_SERVER);
}

/* Caller: ads_play (0x0022AC98), with bufSize 0x40000.  Resets the status
 * time to -1 and starts the stream (NOWAIT); hostName is copied into the
 * request when given. */
// FUNC 0x0025C8C8 pdibgm_start
void pdibgm_start(int lsn, int sectors, int arg, int bufSize, const char *hostName)
{
    g_004A72E0.start(lsn, sectors, arg, bufSize, hostName); /* implicit: WaitSema, strcpy, call (inline start) */
}

/* Caller: RaceBGMPS2::pause (0x0017E3C0, pause). */
// FUNC 0x0025C980 pdibgm_pause
void pdibgm_pause(void)
{
    g_004A72E0.pause();                         /* implicit: lockedCall (inline pause) */
}

/* Caller: RaceBGMPS2::resume (0x0017E3E0, resume). */
// FUNC 0x0025C9A8 pdibgm_resume
void pdibgm_resume(void)
{
    g_004A72E0.resume();                        /* implicit: lockedCall (inline resume) */
}

/* Caller: ads_stop (0x0022AE18). */
// FUNC 0x0025C9D0 pdibgm_stop
void pdibgm_stop(void)
{
    g_004A72E0.stop();                          /* implicit: lockedCall (inline stop) */
}

/* Callers: RaceBGMPS2::update (0x0017DDF0), ads_set_volume (0x0022AC78),
 * mpegif::feed (0x0025EF00).  volume is clamped to [-1, 1] (max.s / min.s)
 * and sent as volume * 32768 (cvt.w.s truncates). */
// FUNC 0x0025C9F8 pdibgm_set_volume
void pdibgm_set_volume(f32 volume)
{
    volume = bgm_clampf(volume, -1.0f, 1.0f);
    g_004A72E0.setVolume((int)(volume * 32768.0f));    /* implicit: WaitSema, call (inline setVolume) */
}

/* Caller: music_thread (0x0022AFF8), per frame. */
// FUNC 0x0025CA78 pdibgm_update
void pdibgm_update(void)
{
    g_004A72E0.updateStatus();                  /* implicit: F_0025CB88 (RPC::updateStatus, out of line) */
}

/* Caller: RaceBGMPS2::update.  Reads the status with interrupts off and
 * returns its time in seconds (__floatdisf, then / 1e6). */
// FUNC 0x0025CA98 pdibgm_get_status
f32 pdibgm_get_status(int *pos, int *id, int *ended)
{
    int wasEnabled = intr_suspend();
    f32 seconds;

    if (pos != NULL)
        *pos = g_002FCE00.pos;
    if (id != NULL)
        *id = g_002FCE00.id;
    if (ended != NULL)
        *ended = g_002FCE00.ended;
    seconds = (f32)g_002FCE00.time / 1000000.0f;   /* implicit: __floatdisf (s64 -> float) */
    intr_resume(wasEnabled);
    return seconds;
}

/* Compiler-generated: egcs's static destructor and initialiser of the
 * object (retail's named after the unit's first global, RPC::RPC:
 * _GLOBAL_$D$__Q233_GLOBAL_$N$.._.._pdibgm.cxxpLWmzt3RPC; ours after the
 * namespace anchor). */
// FUNC 0x0025CB40 T521_static_fini (_GLOBAL_$D$.._.._pdibgm.cxxpLWmzt)
//   (.dtors entry 0x0028F294) ~RPC (flags 2).
/* implicit: ~RPC */
// FUNC 0x0025CB68 T521_static_init (_GLOBAL_$I$.._.._pdibgm.cxxpLWmzt)
//   (.ctors entry 0x0028F1C4) RPC().
/* implicit: RPC */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0025CDB0 __tfQ233_GLOBAL_$N$.._.._pdibgm.cxxpLWmzt3RPC
extern "C" void *tf_pdibgm_RPC(void)
{
    if (g_004B2AE0[0] == 0) {
        __tf12PDIClientRpc();
        __rtti_si(g_004B2AE0, "Q233_GLOBAL_$N$.._.._pdibgm.cxxpLWmzt3RPC",
                  (const type_info *)__ti12PDIClientRpc);
    }
    return g_004B2AE0;
}
#endif
