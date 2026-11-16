/* T522 - pdimpeg.cxx.  0x0025CE00..0x0025D348.
 *
 * Client of the pdimpeg IOP module (movie streams): an array of two objects
 * of the anonymous-namespace class RPC (_GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE::RPC)
 * at 0x004A73A0, servers 'MPG1' (0x4D504731) and 'MPG2' (0x4D504732).
 * MPG1: 1 open (mode, lsn, sectors, host name), 2 feed (reads the next bytes
 * of the stream into an EE buffer), 4 close, all blocking; MPG2: 3 with two
 * doublewords (NOWAIT).  Users: mpegif::preload / mpegif::feed (T529,
 * 0x0025E710, 0x0025EF00), MenuBgMovie::update / stopAndWait (T253, 0x001CD1E0,
 * 0x001CD558). */
#include "engine/pdi/T522_pdimpeg.h"
#include "engine/pdi/pdi_internal.h"

#define PDIMPEG_SERVER_STREAM 0x4D504731    /* 'MPG1' */
#define PDIMPEG_SERVER_TIME   0x4D504732    /* 'MPG2' */

/* The anonymous namespace's name, "_GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE" in the
 * original (type name @0x00345938): a zero-size public object with that
 * assembler name comes first, for the retail compiler only, so egcs derives
 * it instead of six random characters (see T519 / u125's T533_printf.cpp). */
#ifdef BOOT_OLD_GCC
extern const char pdimpeg_cxx_namespace_anchor[] __asm__(".._.._pdimpeg.cxxIPKYuE") = {};
#endif

namespace {   /* _GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE */

/* The out-of-line copies of RPC's inline members come in reverse definition
 * order (T519): defined last first, so F_0025D178 .. close as retail. */

/* RPC - vtable 0x00345920 (slot 1 = PDIClientRpc::onCallEnd); 0xC0 bytes. */
class RPC : public PDIClientRpc {
public:
    u32 pad_3C;                 /* 0x3C */
    union {                     /* 0x40 sendBuf (sendSize 0x80) */
        u32 w[0x20];
        s64 d[0x10];            /* fno 3: {lo, hi} */
        struct {
            int  mode;          /* 0x40 1 preload, 2 play with sound, 3 without */
            int  lsn;           /* 0x44 */
            int  sectors;       /* 0x48 */
            char hostName[0x74]; /* 0x4C "" = the disc */
        } open;                 /* fno 1 */
        struct {
            void *buf;          /* 0x40 */
            int   size;         /* 0x44 */
        } feed;                 /* fno 2 */
    } send;

    RPC();                                                  /* 0x0025CE00 */
    virtual ~RPC();                                         /* [0] +0x0C 0x0025CE48 */

    // FUNC 0x0025D2D8 (pdimpeg)::RPC::close
    void close()
    {
        lockedCall(4, 0);
    }

    // FUNC 0x0025D248 (pdimpeg)::RPC::open
    void open(int mode, int lsn, int sectors, const char *hostName)
    {
        WaitSema(sema);
        send.open.mode    = mode;
        send.open.lsn     = lsn;
        send.open.sectors = sectors;
        send.open.hostName[0] = 0;
        if (hostName != NULL)
            strcpy(send.open.hostName, hostName);
        call(1, 0, NULL, 0);
    }

    // FUNC 0x0025D1D8 (pdimpeg)::RPC::feed
    void feed(void *buf, int size)
    {
        WaitSema(sema);
        send.feed.buf  = buf;
        send.feed.size = size;
        SyncDCache(buf, (u8 *)buf + size);
        call(2, 0, NULL, 0);
    }

    // FUNC 0x0025D178 (pdimpeg)::RPC::F_0025D178
    void F_0025D178(s64 lo, s64 hi)
    {
        WaitSema(sema);
        send.d[0] = lo;
        send.d[1] = hi;
        call(3, SIF_RPC_M_NOWAIT, NULL, 0);
    }
};
static_assert(sizeof(RPC) == 0xC0, "pdimpeg RPC");

static RPC g_004A73A0[2];       /* 0x004A73A0 'MPG1', 0x004A7460 'MPG2' (.bss, see T519) */

} /* namespace */

extern u32 g_004B2AF0[3];       /* 0x004B2AF0 __tiQ234_GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE3RPC */

/* Caller: T522_static_init. */
// FUNC 0x0025CE00 (pdimpeg)::RPC::RPC
RPC::RPC()
{
    /* implicit: PDIClientRpc (the base) */
    sendBuf  = &send;
    sendSize = sizeof(send);    /* 0x80 */
}

/* index.txt: _GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE::RPC::virtual_0 (reached only
 * through the vtable, from T522_static_fini). */
// FUNC 0x0025CE48 (pdimpeg)::RPC::~RPC
RPC::~RPC()
{
    /* implicit: ~PDIClientRpc (the base) */
}

/* Caller: RaceIOPLoadModules::cmdBindClients. */
// FUNC 0x0025CE70 pdimpeg_init
void pdimpeg_init(void)
{
    g_004A73A0[0].bind(PDIMPEG_SERVER_STREAM);
    g_004A73A0[1].bind(PDIMPEG_SERVER_TIME);
}

/* Callers: mpegif::preload (mode 1), mpegif::feed (2 / 3). */
// FUNC 0x0025CEB8 pdimpeg_open
void pdimpeg_open(int mode, int lsn, int sectors, const char *hostName)
{
    g_004A73A0[0].open(mode, lsn, sectors, hostName);  /* implicit: WaitSema, strcpy, call (inline open) */
}

/* Callers: mpegif::preload, mpegif::feed. */
// FUNC 0x0025CF50 pdimpeg_feed
void pdimpeg_feed(void *buf, int size)
{
    g_004A73A0[0].feed(buf, size);              /* implicit: WaitSema, SyncDCache, call (inline feed) */
}

/* Callers: MenuBgMovie::update (0x001CD1E0), MenuBgMovie::stopAndWait
 * (0x001CD558). */
// FUNC 0x0025CFC0 F_0025CFC0
void F_0025CFC0(s64 lo, s64 hi)
{
    g_004A73A0[1].F_0025D178(lo, hi);           /* implicit: WaitSema, call (inline) */
}

/* Callers: mpegif::preload, mpegif::feed. */
// FUNC 0x0025D028 pdimpeg_close
void pdimpeg_close(void)
{
    g_004A73A0[0].close();                      /* implicit: lockedCall (inline close) */
}

/* Compiler-generated: egcs's static destructor and initialiser of the
 * array (retail's named after the unit's first global, RPC::RPC:
 * _GLOBAL_$D$__Q234_GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE3RPC; ours after the
 * namespace anchor). */
// FUNC 0x0025D050 T522_static_fini (_GLOBAL_$D$.._.._pdimpeg.cxxIPKYuE)
//   (.dtors entry 0x0028F298) destroys the array from the end, through the
//   vtable (slot 0, flags 0).
/* implicit: ~RPC */
// FUNC 0x0025D0B8 T522_static_init (_GLOBAL_$I$.._.._pdimpeg.cxxIPKYuE)
//   (.ctors entry 0x0028F1C8) RPC() of both; if a constructor throws, the
//   landing pad (0x0025D10C) destroys the elements already built and rethrows.
/* implicit: RPC */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0025D2F8 __tfQ234_GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE3RPC
extern "C" void *tf_pdimpeg_RPC(void)
{
    if (g_004B2AF0[0] == 0) {
        __tf12PDIClientRpc();
        __rtti_si(g_004B2AF0, "Q234_GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE3RPC",
                  (const type_info *)__ti12PDIClientRpc);
    }
    return g_004B2AF0;
}
#endif
