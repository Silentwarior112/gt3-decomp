/* libmc.c - Sony EE libmc "PsIIlibmc   2240" (SDK 2.2.x) as linked into the
 * GT3 US bootstrap (BOOTSTRAP_SCUS-97102), 0x010157B8..0x01016F70.
 *
 * EE-side client of mcserv.irx (SIF RPC server 0x80000400, client
 * s_mcClient @0x01048940).  Every entry point follows the same pattern:
 *
 *   if (PollSema(s_mcSema) < 0)       return -200;   another request in flight
 *   if (s_mcClient.serve == NULL)     { SignalSema(s_mcSema); return -100; }
 *   [name == NULL || *name == 0]      { SignalSema(s_mcSema); return -210; }
 *   fill s_mcDesc (0x30 bytes) or s_mcName (0x414 bytes)
 *   r = sceSifCallRpc(&s_mcClient, fno, SIF_RPCM_NOWAIT, send, ssize,
 *                     s_mcRdata, 4, end_func, end_para);
 *   if (r == 0) s_mcFuncNo = <sceMcFuncNo*>;  else SignalSema(s_mcSema);
 *   return r;
 *
 * i.e. the library semaphore stays taken while the asynchronous request is
 * pending and sceMcSync() releases it when the RPC has completed, handing
 * back the mcserv result word s_mcRdata[0].  Exceptions: sceMcInit (fno 0xFE,
 * synchronous handshake, checks mcserv/mcman versions) and sceMcGetSlotMax
 * (fno 0x15, synchronous, returns the result directly).
 *
 * RPC function numbers equal the sceMcSync command codes, except sceMcMkdir
 * (sent as Open fno 2 with mode 0x40, code 0xB) and sceMcRename (sent as
 * SetFileInfo fno 0xE with valid 0x10, code 0x13).
 *
 * The boot path (io unit, src/mcard_io.cpp) uses sceMcInit, sceMcGetInfo,
 * sceMcGetDir, sceMcOpen, sceMcRead, sceMcClose and sceMcSync to read the
 * GAMEDATA/core.gt3 candidates from memory card.  Nothing in this unit
 * touches the core.gt3 signature check.
 *
 * Toolchain artefacts in the listing, not reproduced: the nops inside
 * sceMcInit's spin loops are the assembler's R5900 short-loop padding and
 * the lone nops after some `b`/`jr` are loop-alignment fill.
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "sif.h"
#include "libmc.h"

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

#define MCSERV_RPC_ID      0x80000400   /* mcserv.irx SIF RPC server id        */
#define MC_RPC_INIT        0xFE         /* sceMcInit handshake function number */

#define MC_ERR_SEMA        (-200)       /* PollSema failed: request pending    */
#define MC_ERR_NOT_BOUND   (-100)       /* client not bound (no sceMcInit)     */
#define MC_ERR_NAME        (-210)       /* NULL or empty path argument         */
#define MC_ERR_OLD_MCSERV  (-120)       /* mcserv.irx version < 0x20A          */
#define MC_ERR_OLD_MCMAN   (-121)       /* mcman.irx version < 0x20E           */

#define MC_MIN_MCSERV_VER  0x20A
#define MC_MIN_MCMAN_VER   0x20E

/* sceMcRename's "valid" word: the EntryName field of the info block is valid.
 * sceMcSetFileInfo masks its own valid argument with 7, so only Rename can
 * send this bit.                                                            */
#define MC_FILEINFO_NAME   0x10

/* ======================================================================== */
/* Structures (mcserv wire formats; ps2sdk libmc-common.h has the same      */
/* layouts as mcDescParam_t / mcNameParam_t / mcEndParam2_t)                */
/* ======================================================================== */

/* fd / port-slot request, 0x30 bytes (s_mcDesc @0x010489C0). */
typedef struct {
    int   fd;          /* +0x00 Close/Seek/Read/Write/Flush                        */
    int   port;        /* +0x04 GetInfo/Format/Unformat/GetSlotMax                  */
    int   slot;        /* +0x08                                                    */
    int   size;        /* +0x0C Read: size; Write: aligned-part size; GetInfo: format wanted */
    int   offset;      /* +0x10 Seek: offset; GetInfo: free wanted                  */
    int   origin;      /* +0x14 Seek: whence; Write: inline byte count; GetInfo: type wanted;
                                ChangeThreadPriority: level                         */
    void *buffer;      /* +0x18 Read: user buffer; Write: 16-byte aligned remainder */
    void *param;       /* +0x1C Read/GetInfo: &s_mcEndPara                          */
    u8    data[16];    /* +0x20 Write: unaligned head / small write, inline         */
} McDescParam;         /* 0x30 */
_Static_assert(sizeof(McDescParam) == 0x30, "McDescParam");

/* path request, 0x414 bytes (s_mcName @0x010489F0). */
typedef struct {
    int   port;        /* +0x000 */
    int   slot;        /* +0x004 */
    int   flags;       /* +0x008 Open: mode; GetDir: mode; SetFileInfo: valid; Delete: 0 */
    int   maxent;      /* +0x00C GetDir */
    void *table;       /* +0x010 GetDir: user table; Chdir: s_mcPwdBuf; SetFileInfo/Rename: &s_mcFileInfo */
    char  name[0x400]; /* +0x014 strncpy'd, 0x3FF chars max, [0x3FF] forced to 0 */
} McNameParam;         /* 0x414 */
_Static_assert(sizeof(McNameParam) == 0x414, "McNameParam");

/* End-parameter area that mcserv writes back (s_mcEndPara @0x01048E40,
 * 0xC0 bytes, read through the uncached mirror by the end callbacks).
 * Read: unaligned head/tail fragments of the user buffer, copied by the EE.
 * GetInfo: the three out-values (type in the size1 word, free in size2).
 * (No anonymous unions: the old compiler's C does not have them.)           */
typedef struct {
    int   size1;                      /* +0x00 Read: head bytes;  GetInfo: type */
    int   size2;                      /* +0x04 Read: tail bytes;  GetInfo: free */
    u8   *dest1;                      /* +0x08 */
    u8   *dest2;                      /* +0x0C */
    u8    src1[64];                   /* +0x10 */
    u8    src2[64];                   /* +0x50 */
    int   formatted;                  /* +0x90 GetInfo: format */
    u8    pad94[0xC0 - 0x94];         /* +0x94 */
} McEndParam;                         /* 0xC0 */
_Static_assert(sizeof(McEndParam) == 0xC0, "McEndParam");
_Static_assert(offsetof(McEndParam, src2) == 0x50, "src2");
_Static_assert(offsetof(McEndParam, formatted) == 0x90, "formatted");

/* ======================================================================== */
/* Data                                                                     */
/* ======================================================================== */

/* .data 0x0103BE78: library id string (16 chars, no NUL of its own).  Not
 * const: it lives in writable .data, not .rodata, in the binary.            */
static char s_mcVersion[16] BOOT_USED = "PsIIlibmc   2240"; /* @0x0103BE78 */

/* sceMcSync command code of the pending request (0 = idle).  Written only
 * after a successful issue; cleared by sceMcSync when the request is done. */
static int s_mcFuncNo = 0;                            /* @0x0103BE88 (.data) */
/* Library semaphore (max 1); created lazily by sceMcInit.                   */
static int s_mcSema = -1;                             /* @0x0103BE8C (.data) */

/* .bss */
static sceSifClientData s_mcClient __attribute__((aligned(64)));  /* @0x01048940 (0x28) */
static int *s_mcInfoType;                             /* @0x01048968 sceMcGetInfo *type   */
static int *s_mcInfoFree;                             /* @0x0104896C sceMcGetInfo *free   */
static int *s_mcInfoFormat;                           /* @0x01048970 sceMcGetInfo *format */
static sceMcTblGetDir s_mcFileInfo;                   /* @0x01048980 (0x40) SetFileInfo/Rename info block */
static McDescParam s_mcDesc __attribute__((aligned(64)));         /* @0x010489C0 (0x30)  */
static McNameParam s_mcName;                          /* @0x010489F0 (0x414) */
static McEndParam s_mcEndPara __attribute__((aligned(64)));       /* @0x01048E40 (0xC0)  */
static char s_mcPwdBuf[0x400] __attribute__((aligned(64)));       /* @0x01048F00 Chdir result */
/* 0x01049300..0x01049EFF, between the Chdir buffer and s_mcRdata, is libmc
 * .bss referenced by no code in the binary (the linker's _gp, 0x01049CF0 =
 * .lit4 + 0x7FF0, points into it, but every $gp access in the binary uses a
 * negative offset).  Kept as an unreferenced array so that s_mcRdata sits at
 * its retail offset in the object (the old compiler lays out .bss in
 * declaration order).                                                       */
static u8  s_mcUnused[0xC00];                                     /* @0x01049300 */
/* RPC receive area: [0] result, [1] mcserv version, [2] mcman version (Init
 * receives 12 bytes, everything else 4); [15] is written only by
 * mc_get_rpc_state.  libgcc2's .bss follows at 0x01049F40.                   */
static int s_mcRdata[16] __attribute__((aligned(64)));            /* @0x01049F00 */

/* ======================================================================== */
/* Functions                                                                */
/* ======================================================================== */

static void mc_read_end_callback(void *end_para);
static void mc_getinfo_end_callback(void *end_para);
static void mc_chdir_end_callback(void *end_para);
sceSifClientData *mc_get_rpc_state(int **rdata, int **func_no);

// FUNC 0x010157B8 sceMcInit
// Creates the library semaphore on first use (init 1, max 1), waits for any
// pending request (sceMcSync(0, NULL, NULL)), takes the semaphore, binds the
// mcserv client (spinning 0x100000 iterations between bind polls; a negative
// sceSifBindRpc prints "bind error libmc \n" and hangs forever), then runs
// the synchronous handshake RPC 0xFE (send: s_mcDesc as-is, 0x30 bytes;
// receive 12 bytes) and releases the semaphore.  Returns:
//   RPC error r < 0      -> r - 100, client unbound
//   mcserv ver < 0x20A   -> -120, client unbound (message printed)
//   mcman  ver < 0x20E   -> -121, client unbound (message printed)
//   else                 -> s_mcRdata[0] (mcserv init result).
// Version compares are signed (slti).  Caller: io unit mc_init (0x0100F340).
int sceMcInit(void)
{
    struct SemaParam sp;
    int i;
    int r;

    if (s_mcSema < 0) {
        sp.option    = 0;
        sp.initCount = 1;
        sp.maxCount  = 1;
        s_mcSema = CreateSema(&sp);
    }
    sceMcSync(0, NULL, NULL);
    WaitSema(s_mcSema);
    sceSifInitRpc(0);

    for (;;) {
        if (sceSifBindRpc(&s_mcClient, MCSERV_RPC_ID, 0) < 0) {
            printf("bind error libmc \n");
            for (;;)
                ;                                  /* hang */
        }
        if (s_mcClient.serve != NULL)
            break;
        for (i = 0x100000; i != 0; i--)
            __asm__ __volatile__("");              /* busy delay */
    }

    r = sceSifCallRpc(&s_mcClient, MC_RPC_INIT, 0, &s_mcDesc, 0x30,
                      s_mcRdata, 12, NULL, NULL);
    SignalSema(s_mcSema);

    if (r < 0) {
        s_mcClient.serve = NULL;
        return r - 100;
    }
    if (s_mcRdata[1] < MC_MIN_MCSERV_VER) {
        printf("libmc: too old release of mcserv.irx\n");
        s_mcClient.serve = NULL;
        return MC_ERR_OLD_MCSERV;
    }
    if (s_mcRdata[2] < MC_MIN_MCMAN_VER) {
        printf("libmc: too old release of mcman.irx\n");
        s_mcClient.serve = NULL;
        return MC_ERR_OLD_MCMAN;
    }
    return s_mcRdata[0];
}

// FUNC 0x01015970 mc_get_rpc_state
// Internal accessor with no callers in this binary (it sits unreferenced in
// sceMcInit's code block): *rdata = s_mcRdata, *func_no = &s_mcFuncNo,
// stores the semaphore id into s_mcRdata[15] and returns &s_mcClient.
// Sony-internal name unknown.
sceSifClientData *mc_get_rpc_state(int **rdata, int **func_no)
{
    *rdata = s_mcRdata;
    *func_no = &s_mcFuncNo;
    s_mcRdata[15] = s_mcSema;
    return &s_mcClient;
}

// FUNC 0x010159A0 sceMcChangeThreadPriority
// Asynchronous RPC 0x14 with level at s_mcDesc.origin (+0x14); pending code
// sceMcFuncChgPrior (0x14).
int sceMcChangeThreadPriority(int level)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.origin = level;
    r = sceSifCallRpc(&s_mcClient, sceMcFuncChgPrior, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncChgPrior;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01015A58 sceMcGetSlotMax
// SYNCHRONOUS RPC 0x15 (mode 0) with port at s_mcDesc.port; releases the
// semaphore itself and returns the RPC error, or s_mcRdata[0] (slot count)
// on success.  Does not set s_mcFuncNo.
int sceMcGetSlotMax(int port)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.port = port;
    r = sceSifCallRpc(&s_mcClient, sceMcFuncSlotMax, 0,
                      &s_mcDesc, 0x30, s_mcRdata, 4, NULL, NULL);
    if (r != 0) {
        SignalSema(s_mcSema);
        return r;
    }
    SignalSema(s_mcSema);
    return s_mcRdata[0];
}

// FUNC 0x01015B18 sceMcOpen
// Asynchronous RPC 2: s_mcName {port, slot, flags = mode, name}, ssize 0x414;
// pending code sceMcFuncNoOpen.  sceMcSync's result is the fd.  Also the
// engine of sceMcMkdir (mode 0x40).  Caller: io unit mc_open_sync
// (0x0100F3A8).
int sceMcOpen(int port, int slot, const char *name, int mode)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    if (name == NULL || *name == '\0') {
        SignalSema(s_mcSema);
        return MC_ERR_NAME;
    }
    strncpy(s_mcName.name, name, 0x3FF);
    s_mcName.port  = port;
    s_mcName.flags = mode;
    s_mcName.slot  = slot;
    s_mcName.name[0x3FF] = '\0';
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoOpen, SIF_RPCM_NOWAIT,
                      &s_mcName, 0x414, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoOpen;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01015C50 sceMcMkdir
// sceMcOpen(port, slot, name, sceMcFileCreateDir); on a successful issue the
// pending code is overwritten with sceMcFuncNoMkdir (0xB).
int sceMcMkdir(int port, int slot, const char *name)
{
    int r = sceMcOpen(port, slot, name, sceMcFileCreateDir);

    if (r == 0)
        s_mcFuncNo = sceMcFuncNoMkdir;
    return r;
}

// FUNC 0x01015C88 sceMcClose
// Asynchronous RPC 3 with fd at s_mcDesc.fd; pending code sceMcFuncNoClose.
// Caller: io unit mc_close_sync (0x0100F130).
int sceMcClose(int fd)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd = fd;
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoClose, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoClose;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01015D40 sceMcSeek
// Asynchronous RPC 4: fd, offset (+0x10), origin/whence (+0x14); pending
// code sceMcFuncNoSeek.  Unused by the bootstrap.
int sceMcSeek(int fd, int offset, int origin)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd     = fd;
    s_mcDesc.offset = offset;
    s_mcDesc.origin = origin;
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoSeek, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoSeek;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01015E18 mc_read_end_callback
// sceMcRead's RPC end function (runs from the SIF RPC-end handler with
// end_para = &s_mcEndPara).  mcserv DMAs only the cache-line (64-byte)
// aligned middle of the user buffer directly (the fragments are 64-byte
// src1/src2 arrays, mcEndParam2_t layout); the unaligned head (size1 bytes
// from src1 to dest1) and tail (size2 bytes from src2 to dest2) come back in
// the end parameter and are copied here byte by byte, read through the uncached
// mirror (| 0x20000000).  Counts are re-read every iteration, compared
// signed (slt).
static void mc_read_end_callback(void *end_para)
{
    McEndParam *p = (McEndParam *)UNCACHED(end_para);
    u8 *dst;
    int i;

    if (p->size1 != 0) {
        dst = p->dest1;
        for (i = 0; i < p->size1; i++)
            *dst++ = p->src1[i];
    }
    if (p->size2 != 0) {
        dst = p->dest2;
        for (i = 0; i < p->size2; i++)
            *dst++ = p->src2[i];
    }
}

// FUNC 0x01015EA8 sceMcRead
// Asynchronous RPC 5: fd, param = &s_mcEndPara, buffer = buf, size.  Writes
// back the D-cache of the user buffer (size bytes) and of the 0xC0-byte end
// parameter first; end function mc_read_end_callback(&s_mcEndPara).  Pending
// code sceMcFuncNoRead; sceMcSync's result is the byte count.  Caller: io
// unit mc_read_sync (0x0100F448).
int sceMcRead(int fd, void *buf, int size)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd     = fd;
    s_mcDesc.param  = &s_mcEndPara;
    s_mcDesc.buffer = buf;
    s_mcDesc.size   = size;
    sceSifWriteBackDCache(buf, size);
    sceSifWriteBackDCache(&s_mcEndPara, 0xC0);
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoRead, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4,
                      mc_read_end_callback, &s_mcEndPara);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoRead;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01015FC8 sceMcWrite
// Asynchronous RPC 6.  size <= 16 (signed compare): all bytes go inline in
// s_mcDesc.data (origin = size, buffer = NULL, size field 0).  Otherwise the
// head up to the next 16-byte boundary (0..15 bytes, computed as
// ((buf-1) & ~0xF) - (buf-0x10)) goes inline and the aligned remainder is
// passed by address (buffer = buf+head, size = size-head, origin = head).
// Then FlushCache(0) (write back the whole D-cache) and the call.  Pending
// code sceMcFuncNoWrite.  Quirk: the inline copy loop compares unsigned
// (sltu), so a negative size would overrun s_mcDesc.data.  Unused by the
// bootstrap.
int sceMcWrite(int fd, const void *buf, int size)
{
    unsigned int i;
    u32 end, start;
    int head;
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd = fd;
    if (size < 0x11) {
        s_mcDesc.origin = size;
        s_mcDesc.size   = 0;
        s_mcDesc.buffer = NULL;
    } else {
        end   = ((u32)buf - 1) & 0xFFFFFFF0;   /* last 16-byte boundary below buf */
        start = (u32)buf - 0x10;
        head  = end - start;
        s_mcDesc.size   = size - head;
        s_mcDesc.origin = head;
        s_mcDesc.buffer = (u8 *)buf + head;
    }
    for (i = 0; i < (unsigned int)s_mcDesc.origin; i++)
        s_mcDesc.data[i] = ((const u8 *)buf)[i];
    FlushCache(WRITEBACK_DCACHE);
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoWrite, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoWrite;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01016148 mc_alarm_handler
// SetAlarm callback of mc_delay_thread (interrupt context): wakes the thread
// whose id was passed as the alarm argument (iWakeupThread wrapper
// 0x0101C7E0), then the usual handler epilogue `sync.l; ei`.
static void mc_alarm_handler(int alarm_id, u16 time, void *arg)
{
    iWakeupThread((int)arg);
    ExitHandler();
}

// FUNC 0x01016170 mc_delay_thread
// Sleeps the calling thread for `ticks` H-sync units: SetAlarm(ticks & 0xFFFF,
// mc_alarm_handler, GetThreadId()) then SleepThread() as a `j` tail call (so
// $v0 on return is SleepThread's result; the only caller, sceMcSync's wait
// loop with ticks 0x3C, ignores it).
static void mc_delay_thread(int ticks)
{
    SetAlarm((u16)ticks, mc_alarm_handler, (void *)GetThreadId());
    SleepThread();
}

// FUNC 0x010161B8 sceMcSync
// Returns -1 if nothing is pending (s_mcFuncNo == 0).  Otherwise polls
// sceSifCheckStatRpc(&s_mcClient); with mode == 0 and the call still in
// flight it loops {sceSifCheckStatRpc; mc_delay_thread(0x3C)} until done.
// *cmd (if non-NULL) always receives the pending code.  Still running
// (mode != 0) -> 0.  Finished -> clears s_mcFuncNo, *result (if non-NULL) =
// s_mcRdata[0], SignalSema(s_mcSema), returns 1.  Callers: sceMcInit, io
// unit mc_sync_wait (0x0100F4D8).
int sceMcSync(int mode, int *cmd, int *result)
{
    int busy;

    if (s_mcFuncNo == 0)
        return sceMcExecIdle;

    busy = sceSifCheckStatRpc(&s_mcClient.rpcd);
    if (mode == 0 && busy != 0) {
        while (sceSifCheckStatRpc(&s_mcClient.rpcd) != 0)
            mc_delay_thread(0x3C);
        busy = 0;
    }
    busy = !busy;                   /* now: 1 = finished */
    if (cmd != NULL)
        *cmd = s_mcFuncNo;
    if (busy) {
        s_mcFuncNo = 0;
        if (result != NULL)
            *result = s_mcRdata[0];
        SignalSema(s_mcSema);
    }
    return busy;
}

// FUNC 0x01016298 mc_getinfo_end_callback
// sceMcGetInfo's RPC end function: through the uncached mirror of the end
// parameter, copies +0x00 -> *type, +0x04 -> *free, +0x90 -> *format for
// each user pointer that sceMcGetInfo saved as non-NULL.
static void mc_getinfo_end_callback(void *end_para)
{
    McEndParam *p = (McEndParam *)UNCACHED(end_para);

    if (s_mcInfoType != NULL)
        *s_mcInfoType = p->size1;           /* type */
    if (s_mcInfoFree != NULL)
        *s_mcInfoFree = p->size2;           /* free */
    if (s_mcInfoFormat != NULL)
        *s_mcInfoFormat = p->formatted;
}

// FUNC 0x010162F0 sceMcGetInfo
// Asynchronous RPC 1: port, slot, param = &s_mcEndPara and three request
// flags (origin = type wanted, offset = free wanted, size = format wanted,
// each 1/0).  Saves the three user pointers for mc_getinfo_end_callback,
// writes back the end parameter's D-cache, pending code sceMcFuncNoCardInfo.
// Caller: io unit mc_getinfo_sync (0x0100F290).
int sceMcGetInfo(int port, int slot, int *type, int *free, int *format)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.port   = port;
    s_mcDesc.slot   = slot;
    s_mcDesc.param  = &s_mcEndPara;
    if (type != NULL)
        s_mcDesc.origin = 1;
    else
        s_mcDesc.origin = 0;
    if (free != NULL)
        s_mcDesc.offset = 1;
    else
        s_mcDesc.offset = 0;
    if (format != NULL)
        s_mcDesc.size = 1;
    else
        s_mcDesc.size = 0;
    s_mcInfoType   = type;
    s_mcInfoFree   = free;
    s_mcInfoFormat = format;
    sceSifWriteBackDCache(&s_mcEndPara, 0xC0);
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoCardInfo, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4,
                      mc_getinfo_end_callback, &s_mcEndPara);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoCardInfo;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01016478 sceMcGetDir
// Asynchronous RPC 0xD: s_mcName {port, slot, flags = mode, maxent,
// table, name}, ssize 0x414.  Writes back maxent * 0x40 bytes of the table's
// D-cache when maxent >= 0 (signed test).  Pending code sceMcFuncNoGetDir;
// sceMcSync's result is the number of entries.  Caller: io unit
// mc_getdir_sync (0x0100F1D0).
int sceMcGetDir(int port, int slot, const char *name, unsigned int mode,
                int maxent, sceMcTblGetDir *table)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    if (name == NULL || *name == '\0') {
        SignalSema(s_mcSema);
        return MC_ERR_NAME;
    }
    s_mcName.port   = port;
    s_mcName.slot   = slot;
    s_mcName.flags  = mode;
    s_mcName.maxent = maxent;
    s_mcName.table  = table;
    strncpy(s_mcName.name, name, 0x3FF);
    s_mcName.name[0x3FF] = '\0';
    if (maxent >= 0)
        sceSifWriteBackDCache(table, maxent << 6);
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoGetDir, SIF_RPCM_NOWAIT,
                      &s_mcName, 0x414, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoGetDir;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x010165D0 mc_chdir_end_callback
// sceMcChdir's RPC end function (end_para = the caller's pwd buffer): if pwd
// is non-NULL, copies the previous directory string that mcserv left in
// s_mcPwdBuf (read via the uncached mirror) into pwd, clamped to 0x3FF
// chars, and NUL-terminates.  strlen is called twice when the length is
// < 0x400 (unsigned compare).  (Mapping-pass name: _sceMcChdirEndFunc.)
static void mc_chdir_end_callback(void *end_para)
{
    char *pwd = (char *)end_para;
    size_t len;

    if (pwd == NULL)
        return;
    if (strlen((const char *)UNCACHED(s_mcPwdBuf)) < 0x400)
        len = strlen((const char *)UNCACHED(s_mcPwdBuf));
    else
        len = 0x3FF;
    memcpy(pwd, UNCACHED(s_mcPwdBuf), len);
    pwd[len] = '\0';
}

// FUNC 0x01016658 sceMcChdir
// Asynchronous RPC 0xC: s_mcName {port, slot, table = s_mcPwdBuf, name =
// path} (flags/maxent left stale), ssize 0x414; writes back the 0x400-byte
// result buffer's D-cache; end function mc_chdir_end_callback(pwd).
// Pending code sceMcFuncNoChDir.  Unused by the bootstrap.
int sceMcChdir(int port, int slot, const char *path, char *pwd)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    if (path == NULL || *path == '\0') {
        SignalSema(s_mcSema);
        return MC_ERR_NAME;
    }
    s_mcName.port  = port;
    s_mcName.table = s_mcPwdBuf;
    s_mcName.slot  = slot;
    strncpy(s_mcName.name, path, 0x3FF);
    s_mcName.name[0x3FF] = '\0';
    sceSifWriteBackDCache(s_mcPwdBuf, 0x400);
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoChDir, SIF_RPCM_NOWAIT,
                      &s_mcName, 0x414, s_mcRdata, 4,
                      mc_chdir_end_callback, pwd);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoChDir;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x010167A8 sceMcFormat
// Asynchronous RPC 0x10 with port/slot in s_mcDesc; pending code
// sceMcFuncNoFormat.  Unused by the bootstrap.
int sceMcFormat(int port, int slot)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.slot = slot;
    s_mcDesc.port = port;
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoFormat, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoFormat;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01016870 sceMcDelete
// Asynchronous RPC 0xF: s_mcName {port, slot, flags = 0, name}, ssize 0x414;
// pending code sceMcFuncNoDelete.  Unused by the bootstrap.
int sceMcDelete(int port, int slot, const char *name)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    if (name == NULL || *name == '\0') {
        SignalSema(s_mcSema);
        return MC_ERR_NAME;
    }
    strncpy(s_mcName.name, name, 0x3FF);
    s_mcName.port  = port;
    s_mcName.slot  = slot;
    s_mcName.name[0x3FF] = '\0';
    s_mcName.flags = 0;
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoDelete, SIF_RPCM_NOWAIT,
                      &s_mcName, 0x414, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoDelete;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01016998 sceMcFlush
// Asynchronous RPC 0xA with fd in s_mcDesc; pending code sceMcFuncNoFlush.
// Unused by the bootstrap.
int sceMcFlush(int fd)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd = fd;
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoFlush, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoFlush;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01016A50 sceMcSetFileInfo
// Asynchronous RPC 0xE: valid &= 7 (Create|Modify|Attr), the caller's
// 0x40-byte sceMcTblGetDir is copied into s_mcFileInfo (inlined by the
// compiler as an unaligned ldl/ldr + sdl/sdr doubleword copy - no call to
// memcpy), s_mcName {port, slot, flags = valid, table = &s_mcFileInfo,
// name}, then FlushCache(0) and the call (ssize 0x414).  Pending code
// sceMcFuncNoFileInfo.  Unused by the bootstrap.
int sceMcSetFileInfo(int port, int slot, const char *name, const char *info,
                     unsigned int valid)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    if (name == NULL || *name == '\0') {
        SignalSema(s_mcSema);
        return MC_ERR_NAME;
    }
    valid &= sceMcFileInfoCreate | sceMcFileInfoModify | sceMcFileInfoAttr;
    s_mcName.port  = port;
    s_mcName.slot  = slot;
    s_mcName.flags = valid;
    __builtin_memcpy(&s_mcFileInfo, info, sizeof(sceMcTblGetDir));
    s_mcName.table = &s_mcFileInfo;
    strncpy(s_mcName.name, name, 0x3FF);
    s_mcName.name[0x3FF] = '\0';
    FlushCache(WRITEBACK_DCACHE);
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoFileInfo, SIF_RPCM_NOWAIT,
                      &s_mcName, 0x414, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoFileInfo;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01016C28 sceMcRename
// Implemented as a SetFileInfo request (RPC 0xE) with flags = 0x10 (name
// valid): org goes to s_mcName.name, new_name is strncpy'd (max 0x20) into
// s_mcFileInfo.EntryName and EntryName[31] forced to 0 (the rest of
// s_mcFileInfo is whatever the last sceMcSetFileInfo left there); FlushCache(0).
// Argument check: org NULL/empty or new_name NULL -> -210 (an empty new_name
// is accepted).  Pending code sceMcFuncNoRename (0x13).  Unused by the
// bootstrap.
int sceMcRename(int port, int slot, const char *org, const char *new_name)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    if (org == NULL || *org == '\0' || new_name == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NAME;
    }
    s_mcName.port  = port;
    s_mcName.slot  = slot;
    s_mcName.flags = MC_FILEINFO_NAME;
    strncpy(s_mcName.name, org, 0x3FF);
    s_mcName.name[0x3FF] = '\0';
    strncpy((char *)s_mcFileInfo.EntryName, new_name, 0x20);
    s_mcFileInfo.EntryName[31] = '\0';          /* byte +0x3F of the info block */
    s_mcName.table = &s_mcFileInfo;
    FlushCache(WRITEBACK_DCACHE);
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoFileInfo, SIF_RPCM_NOWAIT,
                      &s_mcName, 0x414, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoRename;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01016D90 sceMcUnformat
// Asynchronous RPC 0x11 with port/slot in s_mcDesc; pending code
// sceMcFuncNoUnformat.  Unused by the bootstrap.
int sceMcUnformat(int port, int slot)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.slot = slot;
    s_mcDesc.port = port;
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoUnformat, SIF_RPCM_NOWAIT,
                      &s_mcDesc, 0x30, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoUnformat;
    else
        SignalSema(s_mcSema);
    return r;
}

// FUNC 0x01016E58 sceMcGetEntSpace
// Asynchronous RPC 0x12: s_mcName {port, slot, name = path}, ssize 0x414;
// pending code sceMcFuncNoEntSpace.  Last libmc function (libgcc follows
// after exec_ps2_with_sbus_cleanup).  Unused by the bootstrap.
int sceMcGetEntSpace(int port, int slot, const char *path)
{
    int r;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    if (path == NULL || *path == '\0') {
        SignalSema(s_mcSema);
        return MC_ERR_NAME;
    }
    s_mcName.port = port;
    s_mcName.slot = slot;
    strncpy(s_mcName.name, path, 0x3FF);
    s_mcName.name[0x3FF] = '\0';
    r = sceSifCallRpc(&s_mcClient, sceMcFuncNoEntSpace, SIF_RPCM_NOWAIT,
                      &s_mcName, 0x414, s_mcRdata, 4, NULL, NULL);
    if (r == 0)
        s_mcFuncNo = sceMcFuncNoEntSpace;
    else
        SignalSema(s_mcSema);
    return r;
}
