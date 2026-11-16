/* T520 - pdispu2.cxx.  0x0025BF38..0x0025C830.
 *
 * Client of the pdispu2 IOP module: two objects of the anonymous-namespace
 * class RPC (_GLOBAL_$N$.._.._pdispu2.cxx9KEP7e::RPC):
 *   @0x004A6F50, server 'SPUP' (0x53505550): the SPU2 state (SpuState, fno 4),
 *        a status request (fno 3), fno 5 with three words; answers land in
 *        the 0x40-byte buffer g_002FCA00;
 *   @0x004A6BC0, server 'SPUV' (0x53505556): transfers (fno 1 / 2) of an EE
 *        buffer (? to / from SPU2 memory: {spuAddr, buf, size}).
 * The SpuState helpers fill a state block the caller owns (the G002 sound
 * code's 0x00302698).  engine/pdi/T520_pdispu2.h is a C header: the free functions
 * have C linkage. */
#include "engine/pdi/T520_pdispu2.h"          /* C linkage */
#include "engine/pdi/pdi_internal.h"

#define PDISPU2_SERVER_STATE    0x53505550  /* 'SPUP' */
#define PDISPU2_SERVER_TRANSFER 0x53505556  /* 'SPUV' */

/* The anonymous namespace's name, "_GLOBAL_$N$.._.._pdispu2.cxx9KEP7e" in the
 * original (type name @0x003458A8): a zero-size public object with that
 * assembler name comes first, for the retail compiler only, so egcs derives
 * it instead of six random characters (see T519 / u125's T533_printf.cpp). */
#ifdef BOOT_OLD_GCC
extern const char pdispu2_cxx_namespace_anchor[] __asm__(".._.._pdispu2.cxx9KEP7e") = {};
#endif

namespace {   /* _GLOBAL_$N$.._.._pdispu2.cxx9KEP7e */

/* The out-of-line copies of RPC's inline members come in reverse definition
 * order (T519): defined last first, so F_0025C598 .. F_0025C788 as retail. */

/* RPC - vtable 0x00345890 (slot 1 = PDIClientRpc::onCallEnd); 0x390 bytes. */
class RPC : public PDIClientRpc {
public:
    u32 pad_3C;                 /* 0x3C */
    union {                     /* 0x40 sendBuf (sendSize 0x350) */
        u32      w[0xD4];       /* fno 5: {a, b, c} */
        SpuState state;         /* fno 4 */
        struct {
            u32   spuAddr;      /* 0x40 */
            void *buf;          /* 0x44 */
            int   size;         /* 0x48 */
        } transfer;             /* 'SPUV' fno 1 / 2 */
    } send;

    RPC();                                                  /* 0x0025BF38 */
    virtual ~RPC();                                         /* [0] +0x0C 0x0025BF80 */
    void transfer(u32 fno, void *buf, u32 spuAddr, int size); /* 0x0025BFA8 */

    /* No callers.  fno 0, answer read into a 64-byte aligned stack buffer. */
    // FUNC 0x0025C788 (pdispu2)::RPC::F_0025C788
    int F_0025C788()
    {
        u8 area[0x80];
        u32 *answer = (u32 *)RPC_ALIGN64(area);

        WaitSema(sema);
        call(0, 0, answer, 0x40);
        return answer[0];
    }

    // FUNC 0x0025C740 (pdispu2)::RPC::requestStatus
    void requestStatus()
    {
        WaitSema(sema);
        call(3, 0, g_002FCA00, 0x40);
    }

    /* Copies the state with interrupts off and clears its dirty flag, then
     * sends it (NOWAIT, the answer goes to g_002FCA00). */
    // FUNC 0x0025C608 (pdispu2)::RPC::sendState
    void sendState(SpuState *st)
    {
        WaitSema(sema);
        int wasEnabled = intr_suspend();
        send.state = *st;
        st->dirty = 0;
        intr_resume(wasEnabled);
        call(4, SIF_RPC_M_NOWAIT, g_002FCA00, 0x40);
    }

    /* No callers (the free function F_0025C280 inlines it). */
    // FUNC 0x0025C598 (pdispu2)::RPC::F_0025C598
    void F_0025C598(int a, int b, int c)
    {
        WaitSema(sema);
        send.w[0] = a;
        send.w[1] = b;
        send.w[2] = c;
        call(5, SIF_RPC_M_NOWAIT, NULL, 0);
    }
};
static_assert(sizeof(RPC) == 0x390, "pdispu2 RPC");

/* static: in .bss at the end of the unit, in reverse order (see T519). */
static RPC g_004A6F50;          /* 0x004A6F50 'SPUP' */
static RPC g_004A6BC0;          /* 0x004A6BC0 'SPUV' */

} /* namespace */

/* Answer buffer of the 'SPUP' calls (SIF DMA: 64-byte aligned); doubleword
 * 0 is the voice-active mask.  A public C++ global without an initialiser:
 * egcs puts it in .data, at the end of the file's variables. */
u64 g_002FCA00[8] __attribute__((aligned(64)));    /* 0x002FCA00 */

extern u32 g_004B2AD0[3];       /* 0x004B2AD0 __tiQ234_GLOBAL_$N$.._.._pdispu2.cxx9KEP7e3RPC */

// FUNC 0x0025BF38 (pdispu2)::RPC::RPC
RPC::RPC()
{
    /* implicit: PDIClientRpc (the base) */
    sendBuf  = &send;
    sendSize = sizeof(SpuState);    /* 0x350 */
}

/* index.txt: _GLOBAL_$N$.._.._pdispu2.cxx9KEP7e::RPC::virtual_0. */
// FUNC 0x0025BF80 (pdispu2)::RPC::~RPC
RPC::~RPC()
{
    /* implicit: ~PDIClientRpc (the base) */
}

/* Blocking call {spuAddr, buf, size}; buf is written back before and after.
 * Callers: F_0025C088 (fno 1), F_0025C0C0 (fno 2). */
// FUNC 0x0025BFA8 (pdispu2)::RPC::transfer
void RPC::transfer(u32 fno, void *buf, u32 spuAddr, int size)
{
    WaitSema(sema);
    send.transfer.spuAddr = spuAddr;
    send.transfer.buf     = buf;
    send.transfer.size    = size;
    SyncDCache(buf, (u8 *)buf + size);
    call(fno, 0, NULL, 0);
    SyncDCache(buf, (u8 *)buf + size);
}

/* Caller: RaceIOPLoadModules::cmdBindClients. */
// FUNC 0x0025C048 pdispu2_init
void pdispu2_init(void)
{
    g_004A6F50.bind(PDISPU2_SERVER_STATE);
    g_004A6BC0.bind(PDISPU2_SERVER_TRANSFER);
}

/* No callers. */
// FUNC 0x0025C088 F_0025C088
void F_0025C088(void *buf, u32 spuAddr, int size)
{
    g_004A6BC0.transfer(1, buf, spuAddr, size); /* implicit: F_0025BFA8 (RPC::transfer) */
}

/* No callers. */
// FUNC 0x0025C0C0 F_0025C0C0
void F_0025C0C0(void *buf, u32 spuAddr, int size)
{
    g_004A6BC0.transfer(2, buf, spuAddr, size); /* implicit: F_0025BFA8 (RPC::transfer) */
}

/* No callers. */
// FUNC 0x0025C0F8 pdispu2_request_status
void pdispu2_request_status(void)
{
    g_004A6F50.requestStatus();                 /* implicit: WaitSema, call (inline requestStatus) */
}

/* Caller: snd_flush_voices (0x00269550), a tail jump (not in index.txt's
 * callers). */
// FUNC 0x0025C140 pdispu2_send_state
void pdispu2_send_state(SpuState *st)
{
    g_004A6F50.sendState(st);                   /* implicit: WaitSema, intr_suspend, call (inline sendState) */
}

/* Caller: sound_init (0x0022C900), with (1, 1, 1). */
// FUNC 0x0025C280 F_0025C280
void F_0025C280(int a, int b, int c)
{
    g_004A6F50.F_0025C598(a, b, c);             /* implicit: WaitSema, call (inline) */
}

#define SPU_VOICES 48                   /* 2 cores x 24 */

/* Caller: snd_init (0x00268470). */
// FUNC 0x0025C2F8 spu_state_init
void spu_state_init(SpuState *st)
{
    int i;
    SpuVoiceState *v;
    SpuCoreState *core;

    memset(st, 0, sizeof(SpuState));
    /* a walking pointer: the counter alone counts down (indexing let loop.c
     * run the stores backwards) */
    v = st->voice;
    for (i = 0; i < SPU_VOICES; i++) {
        v->flags = 0x50;
        v++;
    }
    core = st->core;
    for (i = 0; i < 2; i++) {
        core->effect.core = i;
        core->unk_0C = i ? 0xFCC : 0xFC0;
        core->unk_00 = 0x3FFF;
        core->unk_02 = 0x3FFF;
        core->unk_04 = 0x7FFF;
        core->unk_06 = 0x7FFF;
        core->unk_08 = 0;
        core->effect.mode = 0;
        core++;
    }
    st->unk_348 = 0x3FFF;
    st->unk_34A = 0x3FFF;
}

/* No callers.  Does not set the dirty flag. */
// FUNC 0x0025C3B0 spu_state_set_unk08
void spu_state_set_unk08(SpuState *st, int core0, int core1)
{
    int wasEnabled = intr_suspend();

    st->core[0].unk_08 = core0;
    st->core[1].unk_08 = core1;
    intr_resume(wasEnabled);
}

/* Caller: snd_set_reverb (0x00268990).  The core fields of the copies are
 * forced to 0 / 1. */
// FUNC 0x0025C400 spu_state_set_effect
void spu_state_set_effect(SpuState *st, const SpuEffectAttr *core0, const SpuEffectAttr *core1)
{
    int wasEnabled = intr_suspend();

    st->dirty = 1;
    st->core[0].effect = *core0;
    st->core[0].effect.core = 0;
    st->core[1].effect = *core1;
    st->core[1].effect.core = 1;
    intr_resume(wasEnabled);
}

/* No callers.  Sets the same pair in both cores; does not set the dirty flag. */
// FUNC 0x0025C4A8 spu_state_set_unk04
void spu_state_set_unk04(SpuState *st, int l, int r)
{
    int wasEnabled = intr_suspend();

    st->core[0].unk_04 = l;
    st->core[0].unk_06 = r;
    st->core[1].unk_04 = l;
    st->core[1].unk_06 = r;
    intr_resume(wasEnabled);
}

/* Caller: snd_flush_voices (0x00269550).  First doubleword of the last
 * 'SPUP' answer: the voice-active mask. */
// FUNC 0x0025C500 pdispu2_status
u64 pdispu2_status(void)
{
    SyncDCache(g_002FCA00, (u8 *)g_002FCA00 + sizeof(g_002FCA00));
    return g_002FCA00[0];
}

/* Compiler-generated: egcs's static destructor and initialiser of the two
 * objects (retail's named after the unit's first global, RPC::RPC:
 * _GLOBAL_$D$__Q234_GLOBAL_$N$.._.._pdispu2.cxx9KEP7e3RPC; ours after the
 * namespace anchor). */
// FUNC 0x0025C530 T520_static_fini (_GLOBAL_$D$.._.._pdispu2.cxx9KEP7e)
//   (.dtors entry 0x0028F290) ~RPC (flags 2) of 'SPUV', then of 'SPUP'.
/* implicit: ~RPC */
// FUNC 0x0025C568 T520_static_init (_GLOBAL_$I$.._.._pdispu2.cxx9KEP7e)
//   (.ctors entry 0x0028F1C0) RPC() of 'SPUP', then of 'SPUV'.
/* implicit: RPC */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0025C7E0 __tfQ234_GLOBAL_$N$.._.._pdispu2.cxx9KEP7e3RPC
extern "C" void *tf_pdispu2_RPC(void)
{
    if (g_004B2AD0[0] == 0) {
        __tf12PDIClientRpc();
        __rtti_si(g_004B2AD0, "Q234_GLOBAL_$N$.._.._pdispu2.cxx9KEP7e3RPC",
                  (const type_info *)__ti12PDIClientRpc);
    }
    return g_004B2AD0;
}
#endif
