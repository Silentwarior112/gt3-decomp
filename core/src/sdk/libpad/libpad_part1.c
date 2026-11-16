/* libpad_part1.c - Sony EE libpad "PsIIlibpad  2200", first part:
 * 0x00271A60..0x00272B40 (pad_act_dma_transfer .. scePadEnterPressMode; the
 * rest, from scePadExitPressMode, is in u130).  Same machine code as the
 * bootstrap's sdk/libpad/libpad.c (0x01014410..0x010154F0): its source with
 * the core addresses.
 *
 * EE-side client of padman.irx (module version 4.x).  Two RPC servers are
 * bound (0x80000100 and 0x80000101) but every command goes through the
 * first client.  All commands share ONE 0x80-byte in/out buffer
 * (s_padRpcBuf @0x004ABB80), function number 1, mode 0, no end function:
 *   word 0  command (PAD_RPCCMD_*: 1 OPEN, 6 SET_MMODE, 8 SET_ACTALIGN,
 *           0xA SET_BTNINFO, 0xE CLOSE, 0xF END, 0x10 INIT, ...), word 1 port,
 *           word 2 slot, words 3.. arguments / result.
 * The control table s_padCtl, the clients and the buffer are declared in
 * sdk/libcdvd/G004_libcdvd.h (u130's part of libpad uses them too).
 * padman DMAs the pad state into the caller's 256-byte, 64-byte aligned
 * padArea as two ping-pong 0x80-byte halves (PadDmaHalf); readers pick the
 * half whose frame counter is larger.  Actuator values go by a direct SIF
 * DMA of a 0x20-byte block (PadActDma) into a double-buffered IOP buffer.
 * Debug output: printf (pdistd, engine/pdistd/T533_printf.h) gated by s_padWarning
 * (= 1, never written).
 *
 * In the main executable the callers (outside the unit) are 0x0022F328 /
 * 0x0022F938 (scePadRead, scePadGetState, scePadGetReqState, scePadInfoAct,
 * scePadInfoMode, ...) and the pad init / shutdown code.
 */
#include "sdk/libcdvd/G004_libcdvd.h"
#include "sdk/libcdvd/libcdvd_internal.h"

// DATA_START 0x00305E30

/* ======================================================================== */
/* Data (as the bootstrap's libpad.c)                                       */
/* ======================================================================== */

/* .data 0x00305E30..0x00305E78, in definition order: the version string, the
 * two flags, then the name tables (defined further down, before
 * scePadStateIntToStr: where their strings belong in .rodata).  Not const:
 * all of it is in the original's writable .data.                           */
/* "PsIIlibpad  2200": the libpad.a id, exactly 16 chars, no NUL;
 * nothing references it.                                                    */
static BOOT_USED char s_padVersion[16] = "PsIIlibpad  2200";   /* 0x00305E30 */
static int s_padInitialized = 0;    /* 0x00305E40 set by scePadInit, cleared by scePadEnd; never read */
static int s_padWarning     = 1;    /* 0x00305E44 debug-print enable; never written */

/* .bss.  The control table, the clients and the RPC buffer are libpad
 * statics in the original (its .bss 0x004AB840..0x004ABC00), but u130's half
 * of the library uses them too, so here they are C commons (the full link
 * puts a common at its declared address; sdk/libcdvd/G004_libcdvd.h keeps the
 * externs).  The actuator blocks are only this file's: a file static, alone
 * in the object's .bss.  Keep the alignments: padman DMAs into the buffer
 * and the actuator blocks are DMA sources.                                 */
sceSifClientData s_padClient[2];                                    /* 0x004AB840 padman servers 0x80000100, 0x80000101 */
PadPortCtl       s_padCtl[2][4];                                    /* 0x004AB890 */
static PadActDma s_padActDma[2][4] __attribute__((aligned(64)));    /* 0x004AB980 */
u32              s_padRpcBuf[32] __attribute__((aligned(64)));      /* 0x004ABB80 shared 0x80-byte RPC in / out buffer */

static void        pad_act_dma_transfer(int port, int slot);
static int         pad_init_rpc(int mode);
static PadDmaHalf *pad_get_recv_buf(int port, int slot);

/* Sony's nopdelay() spin after a bind that found no server: v0 = 0x10000;
 * loop { v0--; } while (v0 != -1), i.e. 0x10001 passes.  The loop body is
 * empty in the source; the four nops in it are the old compiler's R5900
 * short-loop padding.  The GCC 14 build writes them out (so each pass costs
 * the same) and the asm keeps GCC 14 from deleting the loop.  (Same helper
 * as libcdvd's _sceCdNopDelay, with the smaller count.)                    */
#ifdef BOOT_OLD_GCC
#define PAD_NOPDELAY_PASS()  __asm__ __volatile__("")
#else
#define PAD_NOPDELAY_PASS()  __asm__ __volatile__("nop\n\tnop\n\tnop\n\tnop")
#endif
static __inline__ void pad_nopdelay(void)
{
    int i = 0x10000;

    do
        PAD_NOPDELAY_PASS();
    while (i-- != 0);
}

// FUNC 0x00271A60 pad_act_dma_transfer
/* Sends the port/slot actuator block (0x20 bytes) to the IOP by SIF DMA.
 * If the previous transfer is still pending (sceSifDmaStat >= 0) it only
 * prints "sceSifSetDma faild" and returns.  Otherwise it bumps the block's
 * frame counter, writes back the cache lines and DMAs the block to
 * iopAct + (frame & 1) * 0x20; the new DMA id (0 on failure, which also
 * prints the message) is stored in dmaId.  Only caller: scePadSetActDirect.
 * The frame reserves 0x100 bytes of locals; only the first 0x10 (the DMA
 * descriptor) are used. */
static void pad_act_dma_transfer(int port, int slot)
{
    PadActDma    *act = s_padCtl[port][slot].act;
    sceSifDmaData dma[16];          /* only dma[0] is used */
    u32           dma_id;

    if (sceSifDmaStat(s_padCtl[port][slot].dmaId) >= 0) {
        if (s_padWarning)
            printf("libpad: sceSifSetDma faild\n");
        return;
    }

    {
        u32 iop_buf = s_padCtl[port][slot].iopAct;
        u32 iop_dest;
        act->frame++;
        iop_dest = iop_buf + (act->frame & 1) * PAD_ACT_DMA_SIZE;   /* sll 5 */
        SyncDCache(act, (u8 *)act + PAD_ACT_DMA_SIZE);
        dma[0].data = (u32)act;
        dma[0].addr = iop_dest;
        dma[0].size = PAD_ACT_DMA_SIZE;
        dma[0].mode = 0;
    }
    dma_id = sceSifSetDma(&dma[0], 1);
    if (dma_id == 0) {
        if (s_padWarning)
            printf("libpad: sceSifSetDma faild\n");
    }
    s_padCtl[port][slot].dmaId = dma_id;
}

/* Not in the binary as code.  The retail .rodata holds, between the strings
 * of pad_act_dma_transfer and scePadInit, "libpad: tPadDma Structure
 * Invalid\n" (0x00347A08) that no code references: the trace of an unused
 * inline helper of Sony's libpad (the old compiler emits the string literals
 * of a static inline function even when it is never called).  Its real body
 * is unknown; this stand-in (the bootstrap's) only reproduces the string and
 * its position.  No caller - it generates no code in either build.          */
static __inline__ void pad_dma_struct_invalid(void)
{
    if (s_padWarning)
        printf("libpad: tPadDma Structure Invalid\n");
}

// FUNC 0x00271B90 scePadInit
/* Sets s_padInitialized, binds both padman servers (retrying with a
 * 0x10000-iteration spin until client.serve != NULL; the bind return value
 * is ignored), then checks padman's module version: major (version >> 8,
 * arithmetic) must be 4, else prints the mismatch message
 * "[libpad.a = 4.0, padman.irx = major.minor]" and returns 0.  On success
 * returns pad_init_rpc(mode).  One caller outside the unit. */
int scePadInit(int mode)
{
    int mod_version, major;

    s_padInitialized = 1;

    for (;;) {
        sceSifBindRpc(&s_padClient[0], PAD_RPC_SERVER0, 0);
        if (s_padClient[0].serve != NULL)
            break;
        pad_nopdelay();
    }
    for (;;) {
        sceSifBindRpc(&s_padClient[1], PAD_RPC_SERVER1, 0);
        if (s_padClient[1].serve != NULL)
            break;
        pad_nopdelay();
    }

    mod_version = scePadGetModVersion();    /* 0x00272D38 (u130) */
    major       = mod_version >> 8;
    if (major != 4) {
        if (s_padWarning) {
            printf("libpad: Module version mismatch ");
            printf("[libpad.a = %d.%d, padman.irx = %d.%d]\n", 4, 0, major,
                   mod_version & 0xFF);
        }
        return 0;
    }
    return pad_init_rpc(mode);
}

// FUNC 0x00271CD0 pad_init_rpc
/* Tail of scePadInit: clears open/+0x14/+0x18 of all 2x4 control entries
 * (both ports per pass, slot 0..3), then RPC INIT (0x10) with word 4 = 0.
 * The `mode` argument scePadInit passes is not used: $a0 is reloaded with a
 * data address before anything reads it and word 4 is stored from $zero, so
 * the source wrote the constant (the bootstrap's identical, byte-matched
 * copy does the same).  Returns reply word 3, or 0 if the RPC call fails. */
static int pad_init_rpc(int mode)
{
    int slot;

    for (slot = 0; slot < 4; slot++) {
        s_padCtl[0][slot].open  = 0;
        s_padCtl[0][slot].unk18 = 0;
        s_padCtl[0][slot].unk14 = 0;
        s_padCtl[1][slot].open  = 0;
        s_padCtl[1][slot].unk18 = 0;
        s_padCtl[1][slot].unk14 = 0;
    }

    s_padRpcBuf[0] = PAD_RPCCMD_INIT;
    s_padRpcBuf[4] = 0;             /* constant 0: `mode` is not forwarded */
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    return s_padRpcBuf[3];
}

// FUNC 0x00271D70 scePadEnd
/* RPC END (0xF).  Returns 0 if the call fails, else reply word 3; if that
 * is 1, s_padInitialized is cleared.  One caller outside the unit. */
int scePadEnd(void)
{
    int result;

    s_padRpcBuf[0] = PAD_RPCCMD_END;
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    result = s_padRpcBuf[3];
    if (result == 1)
        s_padInitialized = 0;
    return result;
}

// FUNC 0x00271DF0 scePadPortOpen
/* Opens port/slot with the caller's 256-byte DMA area.  Rejects (return 0,
 * message if s_padWarning) an area not 64-byte aligned and a port/slot whose
 * open flag is exactly 1.  Resets both halves (frame 0, state ExecCmd(5),
 * reqState Busy(2), +0x67 = 0, data[0..0x1F] = 0xFF, length 0), sends RPC
 * OPEN (1) with word 4 = area; on RPC failure returns 0.  Otherwise marks
 * the entry open (regardless of the reply), resets the actuator block frame,
 * records the IOP actuator buffer (reply word 5) and returns reply word 3.
 * (The range ends with the alignment nop at 0x00271FD4.) */
int scePadPortOpen(int port, int slot, u128 *addr)
{
    PadDmaHalf *half;
    PadActDma  *act;
    int         i;

    if (((u32)addr & 0x3F) != 0) {
        if (s_padWarning)
            printf("libpad: buffer addr is not 64 byte align. %08x\n", (u32)addr);
        return 0;
    }
    if (s_padCtl[port][slot].open == 1) {
        if (s_padWarning)
            printf("libpad: pad port is already open [%d][%d]\n", port, slot);
        return 0;
    }

    half = (PadDmaHalf *)addr;
    for (i = 0; i < 2; i++, half++) {
        half->frame    = 0;
        half->state    = scePadStateExecCmd;     /* 5 */
        half->reqState = scePadReqStateBusy;     /* 2 */
        half->unk67    = 0;
        memset(half->data, 0xFF, sizeof(half->data));
        half->length   = 0;
    }

    s_padRpcBuf[0] = PAD_RPCCMD_OPEN;
    s_padRpcBuf[1] = port;
    s_padRpcBuf[2] = slot;
    s_padRpcBuf[4] = (u32)addr;     /* the padArea padman DMAs into */
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;

    /* Matching (open, 95.0%): this order of the independent stores (the
     * best of the 360 orders searched; the old order, open / frame / dmaId /
     * iopAct / padArea / act, gave 89.5%).  Retail keeps the frame store
     * second and the reply-word load first: as if act's base were unknown
     * to the alias analysis and the load not int-typed; reading word 5 as a
     * pointer gives 96.3% (not kept: a cast nothing else supports). */
    act = &s_padActDma[port][slot];
    act->frame                   = 0;
    s_padCtl[port][slot].open    = 1;
    s_padCtl[port][slot].padArea = (PadDmaHalf *)addr;
    s_padCtl[port][slot].iopAct  = s_padRpcBuf[5];
    s_padCtl[port][slot].dmaId   = 0;
    s_padCtl[port][slot].act     = act;
    return s_padRpcBuf[3];
}

// FUNC 0x00271FD8 scePadPortClose
/* Returns 0 if port/slot is not open.  Otherwise RPC CLOSE (0xE) with
 * word 4 = 1; on RPC failure returns 0, else clears the open flag (whatever
 * the reply says) and returns reply word 3.  No caller. */
int scePadPortClose(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;

    s_padRpcBuf[0] = PAD_RPCCMD_CLOSE;
    s_padRpcBuf[1] = port;
    s_padRpcBuf[2] = slot;
    s_padRpcBuf[4] = 1;             /* close mode (ps2sdk: padCloseArgs.mode = 1) */
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    s_padCtl[port][slot].open = 0;
    return s_padRpcBuf[3];
}

// FUNC 0x00272090 pad_get_recv_buf
/* SyncDCache over the whole 256-byte padArea, then returns the half with the
 * larger frame counter (signed compare; half 0 on a tie).  Does NOT check
 * the open flag - callers do, except scePadSetActDirect.
 * (bootstrap 0x01014A40.) */
static PadDmaHalf *pad_get_recv_buf(int port, int slot)
{
    PadDmaHalf *halves = s_padCtl[port][slot].padArea;
    int newest;

    SyncDCache(halves, (u8 *)halves + PAD_DMA_AREA_SIZE);
    newest = (halves[0].frame < halves[1].frame) ? 1 : 0;
    return &halves[newest];
}

// FUNC 0x002720F0 scePadGetFrameCount
/* (bootstrap 0x01014AA0.)  0 if not open, else the newest half's frame counter.  (Name by analogy
 * with later Sony SDKs.)  No caller. */
int scePadGetFrameCount(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return pad_get_recv_buf(port, slot)->frame;
}

// FUNC 0x00272140 scePadRead
/* 0 if not open; else copies `length` bytes of the newest half's data to
 * rdata and returns length (re-read after the copy).  Callers 0x0022F328,
 * 0x0022F938. */
int scePadRead(int port, int slot, unsigned char *rdata)
{
    PadDmaHalf *half;

    if (s_padCtl[port][slot].open == 0)
        return 0;
    half = pad_get_recv_buf(port, slot);
    memcpy(rdata, half, half->length);
    return half->length;
}

// FUNC 0x002721C0 scePadGetState
/* 99 if not open.  Else the newest half's state byte, except that Stable (6)
 * with reqState Busy (2) is reported as ExecCmd (5).  Caller 0x0022F328.
 * (bootstrap 0x01014B70.) */
int scePadGetState(int port, int slot)
{
    PadDmaHalf *half;

    if (s_padCtl[port][slot].open == 0)
        return PAD_STATE_NOT_OPEN;
    half = pad_get_recv_buf(port, slot);
    if (half->state == scePadStateStable) {
        if (half->reqState == scePadReqStateBusy)
            return scePadStateExecCmd;
    }
    return half->state;
}

/* The two name tables are defined here, at their original addresses
 * (ref/MATCHING.md, "Defining data in C"): their strings follow
 * scePadPortOpen's in the retail .rodata (0x00347AD8..0x00347B38), which is
 * where the old compiler emits them when the tables are defined at this
 * point of the file, as in the bootstrap's libpad.c.  The pointer arrays are
 * writable .data after s_padWarning, as in the binary.                      */

/* State names.  Entries 1, 3 and 4 all point at the same empty string
 * (0x00347B00; not "FINDPAD" in this build).                                */
static const char *s_padStateStr[8] = {     /* 0x00305E48 */
    "DISCONNECT",   /* 0 0x00347B08 */
    "",             /* 1 0x00347B00 */
    "FINDCTP1",     /* 2 0x00347AF0 */
    "",             /* 3 0x00347B00 */
    "",             /* 4 0x00347B00 */
    "EXECCMD",      /* 5 0x00347AE8 */
    "STABLE",       /* 6 0x00347AE0 */
    "ERROR",        /* 7 0x00347AD8 */
};

/* Request-state names.  Only 3 real entries; the 4th word (0x00305E74) is
 * the zero padding before libmc's .data, but scePadReqIntToStr accepts
 * state < 4 and so reads it (NULL).                                         */
static const char *s_padReqStr[4] = {       /* 0x00305E68 */
    "COMPLETE",     /* 0 0x00347B28 */
    "FAILED",       /* 1 0x00347B20 */
    "BUSY",         /* 2 0x00347B18 */
    NULL,           /* 3 = 0 (padding word) */
};

// FUNC 0x00272238 scePadStateIntToStr
/* (bootstrap 0x01014BE8.)
 * strcpy(str, s_padStateStr[state]) for (unsigned)state < 8 (tail call),
 * else an inlined strcpy(str, ""): stores the NUL byte of the "" literal
 * @0x00347B00.  No caller. */
void scePadStateIntToStr(int state, unsigned char *str)
{
    if ((unsigned int)state < 8) {
        strcpy((char *)str, s_padStateStr[state]);
        return;
    }
    __builtin_strcpy((char *)str, "");
}

// FUNC 0x00272270 scePadSetReqState
/* 0 if not open; else stores (u8)state into the newest half's reqState and
 * returns 1.  Used by the RPC setters to mark a request Busy (2). */
int scePadSetReqState(int port, int slot, int state)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    pad_get_recv_buf(port, slot)->reqState = (u8)state;
    return 1;
}

// FUNC 0x002722D8 scePadGetReqState
/* 0 if not open; else the newest half's reqState byte (0 Complete,
 * 1 Faild, 2 Busy).  Caller 0x0022F328.
 * (bootstrap 0x01014C88.) */
int scePadGetReqState(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return pad_get_recv_buf(port, slot)->reqState;
}

// FUNC 0x00272328 scePadReqIntToStr
/* (bootstrap 0x01014CD8.)
 * strcpy(str, s_padReqStr[state]) for (unsigned)state < 4, else
 * strcpy(str, "") (inlined NUL copy, as in scePadStateIntToStr).
 * QUIRK: the bound is 4 but the table has 3 names; state 3 reads the zero
 * word after the table and calls strcpy(str, NULL).  No caller. */
void scePadReqIntToStr(int state, unsigned char *str)
{
    if ((unsigned int)state < 4) {
        strcpy((char *)str, s_padReqStr[state]);   /* state 3: src == NULL */
        return;
    }
    __builtin_strcpy((char *)str, "");
}

// FUNC 0x00272360 scePadInfoAct
/* Actuator information.  0 unless open, ok == 1 and modeConfig >= 2.
 * actno >= nrOfActuators (signed) -> 0; actno == -1 -> nrOfActuators;
 * term 1..4 (InfoActFunc/Sub/Size/Curr) -> byte term-1 of the 4-byte entry
 * actInfo[actno]; any other term -> 0.  Caller 0x0022F328.
 * (bootstrap 0x01014D10.) */
int scePadInfoAct(int port, int slot, int actno, int term)
{
    PadDmaHalf *half;

    if (s_padCtl[port][slot].open == 0)
        return 0;
    half = pad_get_recv_buf(port, slot);
    if (half->ok != 1)
        return 0;
    if (half->modeConfig < 2)
        return 0;
    if (actno >= half->nrOfActuators)
        return 0;
    if (actno == -1)
        return half->nrOfActuators;

    switch (term) {
    case InfoActFunc: return half->actInfo[actno][0];
    case InfoActSub:  return half->actInfo[actno][1];
    case InfoActSize: return half->actInfo[actno][2];
    case InfoActCurr: return half->actInfo[actno][3];
    default:          return 0;
    }
}

// FUNC 0x00272480 scePadInfoComb
/* (bootstrap 0x01014E30.)
 * Actuator combination lists.  0 unless open, ok == 1 and modeConfig >= 2.
 * listno == -1 -> numActComb (checked BEFORE the range test, unlike
 * scePadInfoAct); listno >= numActComb (signed) -> 0; offs -1 -> number of
 * actuators in the list, offs 0..2 -> actuator number; entry is the 4 bytes
 * actComb[listno]; any other offs -> 0.  No caller. */
int scePadInfoComb(int port, int slot, int listno, int offs)
{
    PadDmaHalf *half;

    if (s_padCtl[port][slot].open == 0)
        return 0;
    half = pad_get_recv_buf(port, slot);
    if (half->ok != 1)
        return 0;
    if (half->modeConfig < 2)
        return 0;
    if (listno == -1)
        return half->numActComb;
    if (listno >= half->numActComb)
        return 0;

    switch (offs) {
    case -1: return half->actComb[listno][0];
    case 0:  return half->actComb[listno][1];
    case 1:  return half->actComb[listno][2];
    case 2:  return half->actComb[listno][3];
    default: return 0;
    }
}

// FUNC 0x002725A0 scePadInfoMode
/* Mode information.  0 unless open, ok == 1 and reqState != Busy.
 *   term 1 (InfoModeCurID):     0 if modeCurId == 0xF3 (config mode), else
 *                               modeCurId >> 4
 *   term 2 (InfoModeCurExID):   0 if modeConfig == 1, else
 *                               modeTable[modeCurOffs]
 *   term 3 (InfoModeCurExOffs): 0 if modeConfig == 1, else modeCurOffs
 *   term 4 (InfoModeIdTable):   0 if modeConfig == 1; offs == -1 ->
 *                               nrOfModes; offs >= nrOfModes (signed) -> 0;
 *                               else modeTable[offs]
 *   other terms: 0.  Caller 0x0022F328. */
int scePadInfoMode(int port, int slot, int term, int offs)
{
    PadDmaHalf *half;

    if (s_padCtl[port][slot].open == 0)
        return 0;
    half = pad_get_recv_buf(port, slot);
    if (half->ok != 1)
        return 0;
    if (half->reqState == scePadReqStateBusy)
        return 0;

    switch (term) {
    case InfoModeCurID:
        if (half->modeCurId == PAD_MODE_ID_CONFIG)
            return 0;
        return half->modeCurId >> 4;
    case InfoModeCurExID:
        if (half->modeConfig == 1)
            return 0;
        return half->modeTable[half->modeCurOffs];
    case InfoModeCurExOffs:
        if (half->modeConfig == 1)
            return 0;
        return half->modeCurOffs;
    case InfoModeIdTable:
        if (half->modeConfig == 1)
            return 0;
        if (offs == -1)
            return half->nrOfModes;
        if (offs >= half->nrOfModes)
            return 0;
        return half->modeTable[offs];
    default:
        return 0;
    }
}

// FUNC 0x002726D8 scePadSetMainMode
/* RPC SETMAINMODE (6): word 3 = offs, word 4 = lock.  0 on RPC failure;
 * else if reply word 5 == 1 sets reqState Busy and returns word 5 (re-read),
 * otherwise returns word 5. */
int scePadSetMainMode(int port, int slot, int offs, int lock)
{
    s_padRpcBuf[3] = offs;
    s_padRpcBuf[4] = lock;
    s_padRpcBuf[0] = PAD_RPCCMD_SET_MMODE;
    s_padRpcBuf[1] = port;
    s_padRpcBuf[2] = slot;
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    if (s_padRpcBuf[5] == 1)
        scePadSetReqState(port, slot, scePadReqStateBusy);
    return s_padRpcBuf[5];
}

// FUNC 0x00272790 scePadSetActDirect
/* Direct actuator values, sent by SIF DMA instead of RPC.  QUIRK: no open
 * check - it calls pad_get_recv_buf directly (a never-opened port/slot has
 * padArea == NULL).  0 unless the newest half's ok == 1; else copies 6 bytes
 * to the actuator block, sets valid = 1, length = 6, starts the DMA
 * (pad_act_dma_transfer) and returns 1 - even if the DMA was skipped. */
int scePadSetActDirect(int port, int slot, const unsigned char *data)
{
    PadActDma *act;
    int        i;

    if (pad_get_recv_buf(port, slot)->ok != 1)
        return 0;

    act = s_padCtl[port][slot].act;
    for (i = 0; i < PAD_ACT_BYTES; i++)
        act->act[i] = data[i];
    act->length = PAD_ACT_BYTES;
    act->valid  = 1;
    pad_act_dma_transfer(port, slot);
    return 1;
}

// FUNC 0x00272848 scePadSetActAlign
/* RPC SETACTALIGN (8) with the 6 align bytes at buffer bytes 0x0C..0x11.
 * 0 on RPC failure; else if reply word 5 == 1 sets reqState Busy; returns
 * word 5.  No open check. */
int scePadSetActAlign(int port, int slot, const unsigned char *data)
{
    u32 *buf = s_padRpcBuf;
    u8 *align;
    int i;

    buf[0] = PAD_RPCCMD_SET_ACTALIGN;
    buf[1] = port;
    buf[2] = slot;
    align = (u8 *)&buf[3];
    for (i = 0; i < PAD_ACT_BYTES; i++)
        align[i] = data[i];
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, buf, sizeof(s_padRpcBuf),
                      buf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    if (buf[5] == 1)
        scePadSetReqState(port, slot, scePadReqStateBusy);
    return buf[5];
}

// FUNC 0x00272920 scePadGetButtonMask
/* 0 unless open, ok == 1, modeConfig >= 2 and model >= 2; else the 32-bit
 * mask assembled from buttonMask[0..3] (bytes 0x79..0x7C, little-endian).
 * 0x3FFFF = pressure-sensitive (DualShock 2) - see scePadInfoPressMode. */
int scePadGetButtonMask(int port, int slot)
{
    PadDmaHalf *half;

    if (s_padCtl[port][slot].open == 0)
        return 0;
    half = pad_get_recv_buf(port, slot);
    if (half->ok != 1)
        return 0;
    if (half->modeConfig < 2)
        return 0;
    if (half->model < 2)
        return 0;
    return (int)((u64)half->buttonMask[0]
               + ((u64)half->buttonMask[1] << 8)
               + ((u64)half->buttonMask[2] << 16)
               + ((u64)half->buttonMask[3] << 24));
}

// FUNC 0x002729D8 scePadSetButtonInfo
/* RPC SETBUTTONINFO (0xA): word 3 = info.  0 on RPC failure; else if reply
 * word 4 == 1 sets reqState Busy; returns word 4.  No open check. */
int scePadSetButtonInfo(int port, int slot, int info)
{
    s_padRpcBuf[3] = info;
    s_padRpcBuf[0] = PAD_RPCCMD_SET_BTNINFO;
    s_padRpcBuf[1] = port;
    s_padRpcBuf[2] = slot;
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    if (s_padRpcBuf[4] == 1)
        scePadSetReqState(port, slot, scePadReqStateBusy);
    return s_padRpcBuf[4];
}

// FUNC 0x00272A88 scePadInfoPressMode
/* 0 if not open; else 1 iff scePadGetButtonMask() == 0x3FFFF (pressure
 * mode: all 18 buttons report pressure). */
int scePadInfoPressMode(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return scePadGetButtonMask(port, slot) == PAD_BTNMASK_PRESS;
}

// FUNC 0x00272AE8 scePadEnterPressMode
/* 0 if not open; else scePadSetButtonInfo(port, slot, 0xFFF): turns the
 * pressure-sensitive buttons on (scePadExitPressMode, u130, sends 0). */
int scePadEnterPressMode(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return scePadSetButtonInfo(port, slot, PAD_BTNINFO_PRESS);
}
