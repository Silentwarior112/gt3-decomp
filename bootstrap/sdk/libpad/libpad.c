/* libpad.c - Sony EE libpad "PsIIlibpad  2200" (SDK 2.2.x) as linked into the
 * GT3 US bootstrap (BOOTSTRAP_SCUS-97102), 0x01014410..0x010157B8.
 *
 * EE-side client of padman.irx (module version 4.x).  Two RPC servers are
 * bound (0x80000100 and 0x80000101) but every command in this build goes
 * through the first client; the second one is bound and never used.  All
 * commands share ONE 0x80-byte in/out buffer (s_padRpcBuf @0x010488C0),
 * function number 1, mode 0, no end function:
 *
 *   word 0      command   1 OPEN, 6 SETMAINMODE, 8 SETACTALIGN,
 *                         0xA SETBUTTONINFO, 0xB SETVREFPARAM, 0xC GETPORTMAX,
 *                         0xD GETSLOTMAX, 0xE CLOSE, 0xF END, 0x10 INIT,
 *                         0x12 GETMODVERSION, 0x14 (unknown)
 *   word 1      port      word 2  slot     words 3.. arguments / result
 *
 * padman DMAs the pad state into the caller's 256-byte, 64-byte aligned
 * "padArea" as two ping-pong 0x80-byte halves (PadDmaHalf); readers pick the
 * half whose frame counter is larger.  Actuator (vibration) values are not
 * sent by RPC but by a direct SIF DMA of a 0x20-byte block (PadActDma) into
 * a double-buffered IOP buffer whose address the OPEN reply returns.
 *
 * Debug output uses the shared printf (Polyphony's pdistd printf, libc.h),
 * gated by s_padWarning (@0x0103BE44, initialised 1, never written here).
 *
 * The boot path only uses scePadInit(0), scePadPortOpen(0, 0, 0x01042080),
 * scePadGetState(0, 0), scePadRead(0, 0, buf) and scePadEnd() (main unit).
 * Nothing in this unit touches core.gt3 or its signature check.
 *
 * Toolchain artefacts in the listing, not reproduced: the 4 nops inside the
 * bind-retry spin loops are the assembler's R5900 short-loop padding; the
 * `mult1`/`mult3` lines are the R5900 3-operand multiplies computing
 * port*0x70 / slot*0x1C (row/column strides of s_padCtl).
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "sif.h"
#include "libpad.h"

/* ======================================================================== */
/* Structures                                                               */
/* ======================================================================== */

/* One 0x80-byte half of the padArea handed to scePadPortOpen, written by
 * padman over SIF DMA.  Offsets are the ones this libpad reads/writes;
 * names in "ps2sdk:" are the open-source libpad's names for the same bytes. */
typedef struct {
    u8  data[0x20];       /* +0x00 received pad data; scePadRead copies `length` bytes */
    u8  unk20[0x10];      /* +0x20 not touched here (ps2sdk: actDirData/actAlignData) */
    u8  actInfo[4][4];    /* +0x30 scePadInfoAct: 4 bytes per actuator, term 1..4 = byte 0..3 */
    u8  actComb[4][4];    /* +0x40 scePadInfoComb: 4 bytes per list = {count, act0, act1, act2} */
    u16 modeTable[4];     /* +0x50 extended mode-ID table (scePadInfoMode) */
    int frame;            /* +0x58 frame counter; compared SIGNED (slt) between halves */
    int unk5C;            /* +0x5C (ps2sdk: findPadRetries) */
    int length;           /* +0x60 valid bytes at +0x00 */
    u8  modeConfig;       /* +0x64 info stage: >=2 act/comb info valid, 1 = no mode table */
    u8  modeCurId;        /* +0x65 current mode id byte (id in the high nibble, 0xF3 = config) */
    u8  model;            /* +0x66 >=2 required by scePadGetButtonMask (ps2sdk: model) */
    u8  unk67;            /* +0x67 cleared by scePadPortOpen (ps2sdk: buttonDataReady) */
    u8  nrOfModes;        /* +0x68 entries in modeTable */
    u8  modeCurOffs;      /* +0x69 current index into modeTable */
    u8  nrOfActuators;    /* +0x6A */
    u8  numActComb;       /* +0x6B */
    u8  unk6C[4];         /* +0x6C */
    u8  state;            /* +0x70 scePadState* (Discon/FindPad/FindCTP1/ExecCmd/Stable/Error) */
    u8  reqState;         /* +0x71 scePadReqState* (Complete/Faild/Busy) */
    u8  ok;               /* +0x72 ==1: pad information valid */
    u8  unk73[6];         /* +0x73 */
    u8  buttonMask[4];    /* +0x79 32-bit button mask, little-endian bytes */
    u8  unk7D[3];         /* +0x7D */
} PadDmaHalf;             /* 0x80 */
_Static_assert(sizeof(PadDmaHalf) == 0x80, "PadDmaHalf");
_Static_assert(offsetof(PadDmaHalf, frame) == 0x58, "frame");
_Static_assert(offsetof(PadDmaHalf, length) == 0x60, "length");
_Static_assert(offsetof(PadDmaHalf, state) == 0x70, "state");
_Static_assert(offsetof(PadDmaHalf, buttonMask) == 0x79, "buttonMask");

/* EE-side actuator block, one per port/slot (0x40 stride, 64-byte aligned);
 * the first 0x20 bytes are sent to the IOP by pad_act_dma_transfer.        */
typedef struct {
    int frame;            /* +0x00 incremented per transfer; bit 0 picks the IOP half */
    int valid;            /* +0x04 set to 1 by scePadSetActDirect */
    int length;           /* +0x08 set to 6 by scePadSetActDirect */
    u8  act[6];           /* +0x0C actuator values */
    u8  pad12[0x40 - 0x12];
} PadActDma;              /* 0x40 */
_Static_assert(sizeof(PadActDma) == 0x40, "PadActDma");

/* Per port/slot control entry (0x1C bytes, [2 ports][4 slots]).            */
typedef struct {
    PadDmaHalf *padArea;  /* +0x00 user DMA area (two halves) */
    PadActDma  *act;      /* +0x04 &s_padActDma[port][slot] */
    u32         iopAct;   /* +0x08 IOP actuator buffer (OPEN reply word 5), 2 x 0x20 bytes */
    u32         dmaId;    /* +0x0C id of the last actuator sceSifSetDma */
    int         open;     /* +0x10 1 = port/slot open */
    int         unk14;    /* +0x14 only cleared (pad_init_rpc) */
    int         unk18;    /* +0x18 only cleared (pad_init_rpc) */
} PadPortCtl;             /* 0x1C */
_Static_assert(sizeof(PadPortCtl) == 0x1C, "PadPortCtl");

/* ======================================================================== */
/* Data                                                                     */
/* ======================================================================== */

/* .data 0x0103BE30: "PsIIlibpad  2200" version string (the libpad.a id). */
/* Not const: it lives in writable .data, not .rodata, in the binary (as do
 * the two name tables below: their pointer arrays are in .data).         */
static char s_padVersion[16] BOOT_USED = "PsIIlibpad  2200"; /* @0x0103BE30, no NUL */

static int s_padInitialized = 0;  /* @0x0103BE40 set by scePadInit, cleared by scePadEnd; never read */
static int s_padWarning     = 1;  /* @0x0103BE44 debug-print enable; never written in this binary */

/* s_padStateStr (@0x0103BE48) and s_padReqStr (@0x0103BE68), the last of
 * libpad's .data, are defined further down, before scePadStateIntToStr.   */

/* .bss */
static sceSifClientData s_padClient[2];                        /* @0x01048580 (0x80000100, 0x80000101) */
static PadPortCtl       s_padCtl[2][4];                        /* @0x010485D0 */
static PadActDma        s_padActDma[2][4] __attribute__((aligned(64)));  /* @0x010486C0 */
static u32              s_padRpcBuf[32]   __attribute__((aligned(64)));  /* @0x010488C0, 0x80 bytes */

#define PAD_RPC_SERVER0   0x80000100
#define PAD_RPC_SERVER1   0x80000101

/* The one call shape used by every command:
 * sceSifCallRpc(&client0, fno 1, mode 0, buf, 0x80, buf, 0x80, NULL, NULL). */
#define PAD_CALL_RPC() \
    sceSifCallRpc(&s_padClient[0], 1, 0, s_padRpcBuf, 0x80, s_padRpcBuf, 0x80, NULL, NULL)

static void        pad_act_dma_transfer(int port, int slot);
static int         pad_init_rpc(int mode);
static PadDmaHalf *pad_get_recv_buf(int port, int slot);
int                pad_rpc_cmd_0x14(int arg);

/* ======================================================================== */
/* Code                                                                     */
/* ======================================================================== */

// FUNC 0x01014410 pad_act_dma_transfer
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
    u32           id;

    if (sceSifDmaStat(s_padCtl[port][slot].dmaId) >= 0) {
        if (s_padWarning)
            printf("libpad: sceSifSetDma faild\n");
        return;
    }

    {
        u32 iop = s_padCtl[port][slot].iopAct;
        u32 dest;
        act->frame++;
        dest = iop + ((act->frame & 1) << 5);
        SyncDCache(act, (u8 *)act + 0x20);
        dma[0].data = (u32)act;
        dma[0].addr = dest;
        dma[0].size = 0x20;
        dma[0].mode = 0;
    }
    id = sceSifSetDma(&dma[0], 1);
    if (id == 0) {
        if (s_padWarning)
            printf("libpad: sceSifSetDma faild\n");
    }
    s_padCtl[port][slot].dmaId = id;
}

/* Not in the binary as code.  The retail .rodata holds, between the strings
 * of pad_act_dma_transfer and scePadInit, "libpad: tPadDma Structure
 * Invalid\n" (@0x0103FF10) that no code references: the trace of an unused
 * inline helper of Sony's libpad (the old compiler emits the string literals
 * of a static inline function even when it is never called).  Its real body
 * is unknown; this stand-in only reproduces the string and its position.
 * No caller - it generates no code in either build. */
static __inline__ void pad_dma_struct_invalid(void)
{
    if (s_padWarning)
        printf("libpad: tPadDma Structure Invalid\n");
}

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

// FUNC 0x01014540 scePadInit
/* Sets s_padInitialized, binds both padman servers (retrying with a
 * 0x10000-iteration spin until client.serve != NULL; the bind return value
 * is ignored), then checks padman's module version: major (version >> 8,
 * arithmetic) must be 4, else prints the mismatch message
 * "[libpad.a = 4.0, padman.irx = major.minor]" and returns 0.  On success
 * returns pad_init_rpc(mode).  Caller: main's pad/IRX init (0x01000880). */
int scePadInit(int mode)
{
    int ver, major;

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

    ver   = scePadGetModVersion();
    major = ver >> 8;
    if (major != 4) {
        if (s_padWarning) {
            printf("libpad: Module version mismatch ");
            printf("[libpad.a = %d.%d, padman.irx = %d.%d]\n", 4, 0, major, ver & 0xFF);
        }
        return 0;
    }
    return pad_init_rpc(mode);
}

// FUNC 0x01014680 pad_init_rpc
/* Tail of scePadInit: clears open/+0x14/+0x18 of all 2x4 control entries,
 * then RPC INIT (0x10) with word 4 = 0.  The `mode` argument scePadInit
 * passes is ignored ($a0 is overwritten before use).  Returns reply word 3,
 * or 0 if the RPC call fails. */
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

    s_padRpcBuf[0] = 0x10;          /* INIT */
    s_padRpcBuf[4] = 0;             /* ? mode is not forwarded */
    if (PAD_CALL_RPC() < 0)
        return 0;
    return s_padRpcBuf[3];
}

// FUNC 0x01014720 scePadEnd
/* RPC END (0xF).  Returns 0 if the call fails, else reply word 3; if that
 * is 1, s_padInitialized is cleared.  Caller: main's shutdown (0x010009D8). */
int scePadEnd(void)
{
    int ret;

    s_padRpcBuf[0] = 0xF;           /* END */
    if (PAD_CALL_RPC() < 0)
        return 0;
    ret = s_padRpcBuf[3];
    if (ret == 1)
        s_padInitialized = 0;
    return ret;
}

// FUNC 0x010147A0 scePadPortOpen
/* Opens port/slot with the caller's 256-byte DMA area.  Rejects (return 0,
 * message if s_padWarning) an area not 64-byte aligned and a port/slot whose
 * open flag is exactly 1.  Resets both halves (frame 0, state ExecCmd(5),
 * reqState Busy(2), +0x67 = 0, data[0..0x1F] = 0xFF, length 0), sends RPC
 * OPEN (1) with word 4 = area; on RPC failure returns 0.  Otherwise marks
 * the entry open (regardless of the reply), resets the actuator block frame,
 * records the IOP actuator buffer (reply word 5) and returns reply word 3. */
int scePadPortOpen(int port, int slot, u128 *addr)
{
    PadDmaHalf *half;
    PadPortCtl *ctl;
    PadActDma  *act;
    u32         iopAct;
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
        memset(half, 0xFF, 0x20);
        half->length   = 0;
    }

    s_padRpcBuf[0] = 1;             /* OPEN */
    s_padRpcBuf[1] = port;
    s_padRpcBuf[2] = slot;
    s_padRpcBuf[4] = (u32)addr;
    if (PAD_CALL_RPC() < 0)
        return 0;

    s_padCtl[port][slot].open    = 1;
    act = &s_padActDma[port][slot];
    act->frame                   = 0;
    s_padCtl[port][slot].dmaId   = 0;
    s_padCtl[port][slot].iopAct  = s_padRpcBuf[5];
    s_padCtl[port][slot].padArea = (PadDmaHalf *)addr;
    s_padCtl[port][slot].act     = act;
    return s_padRpcBuf[3];
}

// FUNC 0x01014984 F_01014984 (alignment padding, not a function)
/* Not a function: the single `nop` padding scePadPortOpen's end
 * (0x01014980) to the 8-byte-aligned start of scePadPortClose.  Split off
 * by the boundary fix; never executed. */

// FUNC 0x01014988 scePadPortClose
/* Returns 0 if port/slot is not open.  Otherwise RPC CLOSE (0xE) with
 * word 4 = 1; on RPC failure returns 0, else clears the open flag (whatever
 * the reply says) and returns reply word 3. */
int scePadPortClose(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;

    s_padRpcBuf[0] = 0xE;           /* CLOSE */
    s_padRpcBuf[1] = port;
    s_padRpcBuf[2] = slot;
    s_padRpcBuf[4] = 1;
    if (PAD_CALL_RPC() < 0)
        return 0;
    s_padCtl[port][slot].open = 0;
    return s_padRpcBuf[3];
}

// FUNC 0x01014A40 pad_get_recv_buf
/* SyncDCache over the whole 256-byte padArea, then returns the half with the
 * larger frame counter (signed compare; half 0 on a tie).  Does NOT check
 * the open flag - callers do, except scePadSetActDirect. */
static PadDmaHalf *pad_get_recv_buf(int port, int slot)
{
    PadDmaHalf *half = s_padCtl[port][slot].padArea;
    int idx;

    SyncDCache(half, (u8 *)half + 0x100);
    idx = (half[0].frame < half[1].frame) ? 1 : 0;
    return &half[idx];
}

// FUNC 0x01014AA0 scePadGetFrameCount
/* 0 if not open, else the newest half's frame counter.  (Name by analogy
 * with later Sony SDKs; no caller in this binary.) */
int scePadGetFrameCount(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return pad_get_recv_buf(port, slot)->frame;
}

// FUNC 0x01014AF0 scePadRead
/* 0 if not open; else copies `length` bytes of the newest half's data to
 * rdata and returns length (re-read after the copy).  Caller: main's
 * button poll (0x01000A08) with a stack buffer. */
int scePadRead(int port, int slot, unsigned char *rdata)
{
    PadDmaHalf *half;

    if (s_padCtl[port][slot].open == 0)
        return 0;
    half = pad_get_recv_buf(port, slot);
    memcpy(rdata, half, half->length);
    return half->length;
}

// FUNC 0x01014B70 scePadGetState
/* 99 if not open.  Else the newest half's state byte, except that Stable (6)
 * with reqState Busy (2) is reported as ExecCmd (5).  Caller: main (polls
 * port 0/slot 0 up to 16 frames until 0, 2 or 6). */
int scePadGetState(int port, int slot)
{
    PadDmaHalf *half;

    if (s_padCtl[port][slot].open == 0)
        return 99;
    half = pad_get_recv_buf(port, slot);
    if (half->state == scePadStateStable) {
        if (half->reqState == scePadReqStateBusy)
            return scePadStateExecCmd;
    }
    return half->state;
}

/* The two name tables are defined here rather than with the other data:
 * their strings follow scePadPortOpen's in the retail .rodata
 * (0x0103FFE8..0x01040040), which is where the old compiler emits them when
 * the tables are defined at this point of the file.  Their pointer arrays
 * are writable .data after s_padWarning, as in the binary.                */

/* @0x0103BE48: state names.  Entries 1, 3 and 4 all point at the same empty
 * string @0x01040010 (NOT "FINDPAD" in this build).                        */
static const char *s_padStateStr[8] = {
    "DISCONNECT", /* 0 @0x01040018 */
    "",           /* 1 @0x01040010 */
    "FINDCTP1",   /* 2 @0x01040000 */
    "",           /* 3 @0x01040010 */
    "",           /* 4 @0x01040010 */
    "EXECCMD",    /* 5 @0x0103FFF8 */
    "STABLE",     /* 6 @0x0103FFF0 */
    "ERROR",      /* 7 @0x0103FFE8 */
};

/* @0x0103BE68: request-state names.  Only 3 real entries; the 4th word
 * (@0x0103BE74) is the zero padding before libmc's .data, but
 * scePadReqIntToStr accepts state < 4 and so reads it (NULL).              */
static const char *s_padReqStr[4] = {
    "COMPLETE",   /* 0 @0x01040038 */
    "FAILED",     /* 1 @0x01040030 */
    "BUSY",       /* 2 @0x01040028 */
    NULL,         /* 3 @0x0103BE74 = 0 (padding word) */
};

// FUNC 0x01014BE8 scePadStateIntToStr
/* strcpy(str, s_padStateStr[state]) for (unsigned)state < 8 (tail call),
 * else writes the empty string: the out-of-range path is an inlined
 * strcpy(str, "") that copies the NUL byte of the "" literal @0x01040010.
 * __builtin_strcpy for that one: egcs expands strcpy(str, "") the same way
 * either form, and the GCC 14 build (-fno-builtin) then also stores the byte
 * instead of calling strcpy, as the binary does. */
void scePadStateIntToStr(int state, unsigned char *str)
{
    if ((unsigned int)state < 8) {
        strcpy((char *)str, s_padStateStr[state]);
        return;
    }
    __builtin_strcpy((char *)str, "");
}

// FUNC 0x01014C20 scePadSetReqState
/* 0 if not open; else stores (u8)state into the newest half's reqState and
 * returns 1.  Used by the RPC setters to mark a request Busy (2). */
int scePadSetReqState(int port, int slot, int state)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    pad_get_recv_buf(port, slot)->reqState = (u8)state;
    return 1;
}

// FUNC 0x01014C88 scePadGetReqState
/* 0 if not open; else the newest half's reqState byte (0 Complete,
 * 1 Faild, 2 Busy). */
int scePadGetReqState(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return pad_get_recv_buf(port, slot)->reqState;
}

// FUNC 0x01014CD8 scePadReqIntToStr
/* strcpy(str, s_padReqStr[state]) for (unsigned)state < 4, else
 * strcpy(str, "") (inlined NUL copy, as in scePadStateIntToStr).
 * QUIRK: the bound is 4 but the table has 3 names; state 3 reads the zero
 * padding word @0x0103BE74 and calls strcpy(str, NULL). */
void scePadReqIntToStr(int state, unsigned char *str)
{
    if ((unsigned int)state < 4) {
        strcpy((char *)str, s_padReqStr[state]);   /* state 3: src == NULL */
        return;
    }
    __builtin_strcpy((char *)str, "");
}

// FUNC 0x01014D10 scePadInfoAct
/* Actuator information.  0 unless open, ok == 1 and modeConfig >= 2.
 * actno >= nrOfActuators (signed) -> 0; actno == -1 -> nrOfActuators;
 * term 1..4 (InfoActFunc/Sub/Size/Curr) -> byte term-1 of the 4-byte entry
 * actInfo[actno]; any other term -> 0. */
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

// FUNC 0x01014E30 scePadInfoComb
/* Actuator combination lists.  0 unless open, ok == 1 and modeConfig >= 2.
 * listno == -1 -> numActComb (checked BEFORE the range test, unlike
 * scePadInfoAct); listno >= numActComb (signed) -> 0; offs -1 -> number of
 * actuators in the list, offs 0..2 -> actuator number; entry is the 4 bytes
 * actComb[listno]; any other offs -> 0. */
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

// FUNC 0x01014F50 scePadInfoMode
/* Mode information.  0 unless open, ok == 1 and reqState != Busy.
 *   term 1 (InfoModeCurID):     0 if modeCurId == 0xF3 (config mode), else
 *                               modeCurId >> 4
 *   term 2 (InfoModeCurExID):   0 if modeConfig == 1, else
 *                               modeTable[modeCurOffs]
 *   term 3 (InfoModeCurExOffs): 0 if modeConfig == 1, else modeCurOffs
 *   term 4 (InfoModeIdTable):   0 if modeConfig == 1; offs == -1 ->
 *                               nrOfModes; offs >= nrOfModes (signed) -> 0;
 *                               else modeTable[offs]
 *   other terms: 0. */
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
        if (half->modeCurId == 0xF3)
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

// FUNC 0x01015088 scePadSetMainMode
/* RPC SETMAINMODE (6): word 3 = offs, word 4 = lock.  0 on RPC failure;
 * else if reply word 5 == 1 sets reqState Busy and returns word 5 (re-read),
 * otherwise returns word 5. */
int scePadSetMainMode(int port, int slot, int offs, int lock)
{
    s_padRpcBuf[3] = offs;
    s_padRpcBuf[4] = lock;
    s_padRpcBuf[0] = 6;             /* SETMAINMODE */
    s_padRpcBuf[1] = port;
    s_padRpcBuf[2] = slot;
    if (PAD_CALL_RPC() < 0)
        return 0;
    if (s_padRpcBuf[5] == 1)
        scePadSetReqState(port, slot, scePadReqStateBusy);
    return s_padRpcBuf[5];
}

// FUNC 0x01015140 scePadSetActDirect
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
    for (i = 0; i < 6; i++)
        act->act[i] = data[i];
    act->length = 6;
    act->valid  = 1;
    pad_act_dma_transfer(port, slot);
    return 1;
}

// FUNC 0x010151F8 scePadSetActAlign
/* RPC SETACTALIGN (8) with the 6 align bytes at buffer bytes 0x0C..0x11.
 * 0 on RPC failure; else if reply word 5 == 1 sets reqState Busy; returns
 * word 5.  No open check. */
int scePadSetActAlign(int port, int slot, const unsigned char *data)
{
    u32 *buf = s_padRpcBuf;
    u8 *dst;
    int i;

    buf[0] = 8;                     /* SETACTALIGN */
    buf[1] = port;
    buf[2] = slot;
    dst = (u8 *)&buf[3];
    for (i = 0; i < 6; i++)
        dst[i] = data[i];
    if (sceSifCallRpc(&s_padClient[0], 1, 0, buf, 0x80, buf, 0x80, NULL, NULL) < 0)
        return 0;
    if (buf[5] == 1)
        scePadSetReqState(port, slot, scePadReqStateBusy);
    return buf[5];
}

// FUNC 0x010152D0 scePadGetButtonMask
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

// FUNC 0x01015388 scePadSetButtonInfo
/* RPC SETBUTTONINFO (0xA): word 3 = info.  0 on RPC failure; else if reply
 * word 4 == 1 sets reqState Busy; returns word 4.  No open check. */
int scePadSetButtonInfo(int port, int slot, int info)
{
    s_padRpcBuf[3] = info;
    s_padRpcBuf[0] = 0xA;           /* SETBUTTONINFO */
    s_padRpcBuf[1] = port;
    s_padRpcBuf[2] = slot;
    if (PAD_CALL_RPC() < 0)
        return 0;
    if (s_padRpcBuf[4] == 1)
        scePadSetReqState(port, slot, scePadReqStateBusy);
    return s_padRpcBuf[4];
}

// FUNC 0x01015438 scePadInfoPressMode
/* 0 if not open; else 1 iff scePadGetButtonMask() == 0x3FFFF. */
int scePadInfoPressMode(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return scePadGetButtonMask(port, slot) == 0x3FFFF;
}

// FUNC 0x01015498 scePadEnterPressMode
/* 0 if not open; else scePadSetButtonInfo(port, slot, 0xFFF). */
int scePadEnterPressMode(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return scePadSetButtonInfo(port, slot, 0xFFF);
}

// FUNC 0x010154F0 scePadExitPressMode
/* 0 if not open; else scePadSetButtonInfo(port, slot, 0). */
int scePadExitPressMode(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return scePadSetButtonInfo(port, slot, 0);
}

// FUNC 0x01015548 scePadSetVrefParam
/* RPC SETVREFPARAM (0xB) with 12 bytes of data at buffer bytes 0x0C..0x17
 * (inline unaligned 8+4-byte copy, ldl/ldr + lwl/lwr, no memcpy call).
 * 0 on RPC failure; else if reply word 7 == 1 sets reqState Busy; returns
 * word 7.  No open check.  Name from the Sony 2.2 command order (between
 * SETBUTTONINFO 0xA and GETPORTMAX 0xC). */
int scePadSetVrefParam(int port, int slot, const unsigned char *data)
{
    s_padRpcBuf[1] = port;
    s_padRpcBuf[0] = 0xB;           /* SETVREFPARAM */
    s_padRpcBuf[2] = slot;
    __builtin_memcpy(&s_padRpcBuf[3], data, 12);
    if (PAD_CALL_RPC() < 0)
        return 0;
    if (s_padRpcBuf[7] == 1)
        scePadSetReqState(port, slot, scePadReqStateBusy);
    return s_padRpcBuf[7];
}

// FUNC 0x01015618 scePadGetPortMax
/* RPC GETPORTMAX (0xC): reply word 3, or 0 if the call fails. */
int scePadGetPortMax(void)
{
    s_padRpcBuf[0] = 0xC;           /* GETPORTMAX */
    if (PAD_CALL_RPC() < 0)
        return 0;
    return s_padRpcBuf[3];
}

// FUNC 0x01015680 scePadGetSlotMax
/* RPC GETSLOTMAX (0xD) with word 1 = port: reply word 3, or 0 on failure. */
int scePadGetSlotMax(int port)
{
    s_padRpcBuf[1] = port;
    s_padRpcBuf[0] = 0xD;           /* GETSLOTMAX */
    if (PAD_CALL_RPC() < 0)
        return 0;
    return s_padRpcBuf[3];
}

// FUNC 0x010156E8 scePadGetModVersion
/* RPC GETMODVERSION (0x12): reply word 3 = padman version (major << 8 |
 * minor), or 0 on failure.  Caller: scePadInit (requires major 4). */
int scePadGetModVersion(void)
{
    s_padRpcBuf[0] = 0x12;          /* GETMODVERSION */
    if (PAD_CALL_RPC() < 0)
        return 0;
    return s_padRpcBuf[3];
}

// FUNC 0x01015750 pad_rpc_cmd_0x14
/* Unidentified padman command 0x14: word 1 = arg; returns reply word 2
 * (not word 3 like the others), or 0 if the call fails.  No callers in
 * this binary; Sony name unknown. */
int pad_rpc_cmd_0x14(int arg)
{
    s_padRpcBuf[1] = arg;
    s_padRpcBuf[0] = 0x14;
    if (PAD_CALL_RPC() < 0)
        return 0;
    return s_padRpcBuf[2];
}
