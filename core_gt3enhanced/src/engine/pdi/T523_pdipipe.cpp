/* T523 - pdipipe.cxx.  0x0025D348..0x0025DA00.
 *
 * Client of the pdipipe IOP module (streamed file reads): one object of the
 * anonymous-namespace class RPC (_GLOBAL_$N$.._.._pdipipe.cxxIVS7RR::RPC) at
 * 0x004A7520, server 'PDIP' (0x50444950).  Functions (all blocking):
 * 1 close, 2 / 5 open (answer word 0 = the handle), 3 / 6 read into an EE
 * buffer, 4 read into SPU2 memory (buf = the SPU address), 7 / 8 with a name
 * and a two-word answer.  Users: GZIP::InflateLoader::fetchInput (0x0023E1E0),
 * vol_thread_main (0x002411E8), file_read_sound_bank (0x002418D0). */
#include "engine/pdi/T523_pdipipe.h"
#include "engine/pdi/pdi_internal.h"

#define PDIPIPE_SERVER 0x50444950       /* 'PDIP' */

/* The anonymous namespace's name, "_GLOBAL_$N$.._.._pdipipe.cxxIVS7RR" in the
 * original (type name @0x00345980): a zero-size public object with that
 * assembler name comes first, for the retail compiler only, so egcs derives
 * it instead of six random characters (see T519 / u125's T533_printf.cpp). */
#ifdef BOOT_OLD_GCC
extern const char pdipipe_cxx_namespace_anchor[] __asm__(".._.._pdipipe.cxxIVS7RR") = {};
#endif

namespace {   /* _GLOBAL_$N$.._.._pdipipe.cxxIVS7RR */

/* The out-of-line copies of RPC's inline members come in reverse definition
 * order (T519): defined last first, so F_0025D7A8 .. F_0025D940 as retail. */

/* RPC - vtable 0x00345968 (slot 1 = PDIClientRpc::onCallEnd); 0xC0 bytes. */
class RPC : public PDIClientRpc {
public:
    u32 pad_3C;                 /* 0x3C */
    union {                     /* 0x40 sendBuf (sendSize 0x80) */
        u32  w[0x20];           /* fno 1: {h}; 3 / 4 / 6: {h, buf, size} */
        char name[0x80];        /* fno 7 / 8 */
        struct {
            int  lsn;           /* 0x40 sector, or the byte offset in a host: file */
            int  size;          /* 0x44 bytes */
            int  bufSize;       /* 0x48 */
            u32  unk_4C[2];     /* 0x4C */
            char hostPath[0x6C]; /* 0x54 "" = the disc */
        } open;                 /* fno 2 / 5 */
    } send;

    RPC();                                                  /* 0x0025D348 */
    virtual ~RPC();                                         /* [0] +0x0C 0x0025D390 */
    int  open(u32 fno, int lsn, int size, int bufSize, const char *hostPath); /* 0x0025D3B8 */
    void transfer(u32 fno, int h, void *buf, int size);     /* 0x0025D480 */

    /* No callers (alloca).  fno 0, returns the first answer word. */
    // FUNC 0x0025D940 (pdipipe)::RPC::F_0025D940
    int F_0025D940()
    {
        int *answer = (int *)RPC_ALIGN64(__builtin_alloca(0x80));

        WaitSema(sema);
        call(0, 0, answer, 0x40);
        return answer[0];
    }

    /* {a, b, c} without D-cache write-backs. */
    // FUNC 0x0025D8C0 (pdipipe)::RPC::request
    void request(u32 fno, int a, int b, int c)
    {
        WaitSema(sema);
        send.w[0] = a;
        send.w[1] = b;
        send.w[2] = c;
        call(fno, 0, NULL, 0);
    }

    // FUNC 0x0025D870 (pdipipe)::RPC::close
    void close(int h)
    {
        WaitSema(sema);
        send.w[0] = h;
        call(1, 0, NULL, 0);
    }

    /* Not inlined (alloca).  Caller: F_0025D720.  Returns -1 when the first
     * answer word is -1, else 0.
     * Not exact (89.5%, the same with 991111-01): retail tests *out0 with a
     * branch (`bne v1,a0(-1)', v0 = 0 / -1), egcs if-converts every form
     * tried into nor / movn: ?:, if / return, a result variable set in one
     * or both arms, -(x == -1), a goto, the answer word instead of *out0. */
    // FUNC 0x0025D7A8 (pdipipe)::RPC::F_0025D7A8
    int F_0025D7A8(u32 fno, const char *name, int *out0, int *out1)
    {
        int *answer = (int *)RPC_ALIGN64(__builtin_alloca(0x80));

        WaitSema(sema);
        strcpy(send.name, name);
        call(fno, 0, answer, 0x40);
        *out0 = answer[0];
        *out1 = answer[1];
        return *out0 == -1 ? -1 : 0;
    }
};
static_assert(sizeof(RPC) == 0xC0, "pdipipe RPC");

static RPC g_004A7520;          /* 0x004A7520 'PDIP' (.bss, see T519) */

} /* namespace */

extern u32 g_004B2B00[3];       /* 0x004B2B00 __tiQ234_GLOBAL_$N$.._.._pdipipe.cxxIVS7RR3RPC */

// FUNC 0x0025D348 (pdipipe)::RPC::RPC
RPC::RPC()
{
    /* implicit: PDIClientRpc (the base) */
    sendBuf  = &send;
    sendSize = sizeof(send);    /* 0x80 */
}

/* index.txt: _GLOBAL_$N$.._.._pdipipe.cxxIVS7RR::RPC::virtual_0. */
// FUNC 0x0025D390 (pdipipe)::RPC::~RPC
RPC::~RPC()
{
    /* implicit: ~PDIClientRpc (the base) */
}

/* Uses alloca for the 64-byte aligned answer buffer; returns the handle.
 * Callers: pdipipe_open (fno 2), F_0025D580 (fno 5). */
// FUNC 0x0025D3B8 (pdipipe)::RPC::open
int RPC::open(u32 fno, int lsn, int size, int bufSize, const char *hostPath)
{
    int *answer = (int *)RPC_ALIGN64(__builtin_alloca(0x80));

    WaitSema(sema);
    send.open.lsn     = lsn;
    send.open.size    = size;
    send.open.bufSize = bufSize;
    send.open.hostPath[0] = 0;
    if (hostPath != NULL)
        strcpy(send.open.hostPath, hostPath);
    call(fno, 0, answer, 0x40);
    return answer[0];
}

/* {h, buf, size}; buf is written back before and after the call.
 * Callers: pdipipe_read / pdipipe_read2 (fno 3), F_0025D690 (fno 6). */
// FUNC 0x0025D480 (pdipipe)::RPC::transfer
void RPC::transfer(u32 fno, int h, void *buf, int size)
{
    WaitSema(sema);
    send.w[0] = h;
    send.w[1] = (u32)buf;
    send.w[2] = size;
    SyncDCache(buf, (u8 *)buf + size);
    call(fno, 0, NULL, 0);
    SyncDCache(buf, (u8 *)buf + size);
}

/* Caller: RaceIOPLoadModules::cmdBindClients. */
// FUNC 0x0025D520 pdipipe_init
void pdipipe_init(void)
{
    g_004A7520.bind(PDIPIPE_SERVER);
}

/* Callers: vol_thread_main (0x002411E8), file_read_sound_bank (0x002418D0). */
// FUNC 0x0025D548 pdipipe_open
int pdipipe_open(int lsn, int size, int bufSize, const char *hostPath)
{
    return g_004A7520.open(2, lsn, size, bufSize, hostPath);   /* implicit: F_0025D3B8 (RPC::open) */
}

/* No callers. */
// FUNC 0x0025D580 F_0025D580
int F_0025D580(int lsn, int size, int bufSize, const char *hostPath)
{
    return g_004A7520.open(5, lsn, size, bufSize, hostPath);   /* implicit: F_0025D3B8 (RPC::open) */
}

/* Callers: GZIP::InflateLoader::fetchInput (0x0023E1E0), vol_thread_main. */
// FUNC 0x0025D5B8 pdipipe_read
void pdipipe_read(int h, void *buf, int size)
{
    g_004A7520.transfer(3, h, buf, size);      /* implicit: F_0025D480 (RPC::transfer) */
}

/* Caller: file_read_sound_bank: toSpu 1 for the samples, buf = their SPU2
 * address.  toSpu != 0: fno 4 without the D-cache write-backs. */
// FUNC 0x0025D5F0 pdipipe_read2
void pdipipe_read2(int h, void *buf, int size, int toSpu)
{
    if (toSpu == 0) {
        g_004A7520.transfer(3, h, buf, size);  /* implicit: F_0025D480 (RPC::transfer) */
        return;
    }
    g_004A7520.request(4, h, (int)buf, size);  /* implicit: WaitSema, call (inline request) */
}

/* No callers. */
// FUNC 0x0025D690 F_0025D690
void F_0025D690(int h, void *buf, int size)
{
    g_004A7520.transfer(6, h, buf, size);      /* implicit: F_0025D480 (RPC::transfer) */
}

/* Callers: vol_thread_main, file_read_sound_bank. */
// FUNC 0x0025D6C8 pdipipe_close
void pdipipe_close(int h)
{
    g_004A7520.close(h);                       /* implicit: WaitSema, call (inline close) */
}

/* No callers. */
// FUNC 0x0025D720 F_0025D720
int F_0025D720(const char *name, int *out0, int *out1, int flag)
{
    return g_004A7520.F_0025D7A8(flag == 0 ? 7 : 8, name, out0, out1);
}

/* Compiler-generated: egcs's static destructor and initialiser of the
 * object (retail's named after the unit's first global, RPC::RPC:
 * _GLOBAL_$D$__Q234_GLOBAL_$N$.._.._pdipipe.cxxIVS7RR3RPC; ours after the
 * namespace anchor). */
// FUNC 0x0025D760 T523_static_fini (_GLOBAL_$D$.._.._pdipipe.cxxIVS7RR)
//   (.dtors entry 0x0028F29C) ~RPC (flags 2).
/* implicit: ~RPC */
// FUNC 0x0025D788 T523_static_init (_GLOBAL_$I$.._.._pdipipe.cxxIVS7RR)
//   (.ctors entry 0x0028F1CC) RPC().
/* implicit: RPC */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0025D9B0 __tfQ234_GLOBAL_$N$.._.._pdipipe.cxxIVS7RR3RPC
extern "C" void *tf_pdipipe_RPC(void)
{
    if (g_004B2B00[0] == 0) {
        __tf12PDIClientRpc();
        __rtti_si(g_004B2B00, "Q234_GLOBAL_$N$.._.._pdipipe.cxxIVS7RR3RPC",
                  (const type_info *)__ti12PDIClientRpc);
    }
    return g_004B2B00;
}
#endif
