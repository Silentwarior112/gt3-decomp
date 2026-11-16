/* libmc.c - Sony EE libmc "PsIIlibmc   2240" (0x00272E08..0x002745C0).
 *
 * Same machine code as the bootstrap's sdk/libmc/libmc.c
 * (0x010157B8..0x01016F70).  EE-side client of mcserv.irx (SIF RPC server
 * 0x80000400).  Every entry point follows the same pattern:
 *
 *   if (PollSema(s_mcSema) < 0)       return -200;   another request in flight
 *   if (s_mcClient.serve == NULL)     { SignalSema(s_mcSema); return -100; }
 *   [name == NULL || *name == 0]      { SignalSema(s_mcSema); return -210; }
 *   fill s_mcDesc (0x30 bytes) or s_mcName (0x414 bytes)
 *   ret = sceSifCallRpc(&s_mcClient, fno, SIF_RPCM_NOWAIT, send, ssize,
 *                       s_mcRdata, 4, end_func, end_para);
 *   if (ret == 0) s_mcFuncNo = <sceMcFuncNo*>;  else SignalSema(s_mcSema);
 *   return ret;
 *
 * i.e. the library semaphore stays taken while the asynchronous request is
 * pending and sceMcSync() releases it when the RPC has completed, handing
 * back the mcserv result word s_mcRdata[0].  Exceptions: sceMcInit (fno 0xFE,
 * synchronous handshake, checks mcserv/mcman versions) and sceMcGetSlotMax
 * (fno 0x15, synchronous, returns the result directly).
 *
 * The game's blocking wrappers (mc_init_once 0x002674E0, mc_sync_wait
 * 0x002678E8, mc_write_sync 0x002679C8, ...) are in u123.
 */
#include "sdk/libmc/libmc_internal.h"

/* ======================================================================== */
/* Data (file statics, as the bootstrap's sdk/libmc/libmc.c)                */
/* ======================================================================== */

/* .data 0x00305E78..0x00305E90.  The library id string opens the object's
 * .data (16 chars, no NUL of its own; referenced by no code).  Not const: it
 * lives in writable .data, not .rodata, in the binary. */
// DATA_START 0x00305E78
static BOOT_USED char s_mcVersion[16] = "PsIIlibmc   2240"; /* 0x00305E78 */
/* sceMcSync command code of the pending request (0 = idle).  Written only
 * after a successful issue; cleared by sceMcSync when the request is done. */
static int s_mcFuncNo = 0;                            /* 0x00305E88 */
/* Library semaphore (max 1); created lazily by sceMcInit. */
static int s_mcSema = -1;                             /* 0x00305E8C */

/* .bss 0x004ABC00..0x004AD200, right after libpad's (s_padRpcBuf ends at
 * 0x004ABC00) and before libgcc2's (0x004AD200): file statics in definition
 * order.  The SIF RPC buffers keep their 64-byte alignment (the IOP DMAs
 * into them; mcserv writes s_mcEndPara / s_mcPwdBuf back, read through the
 * uncached mirror). */
static sceSifClientData s_mcClient __attribute__((aligned(64)));  /* 0x004ABC00 (0x28) mcserv client (serve at 0x004ABC24) */
static int *s_mcInfoType;                             /* 0x004ABC28 sceMcGetInfo *type */
static int *s_mcInfoFree;                             /* 0x004ABC2C sceMcGetInfo *free */
static int *s_mcInfoFormat;                           /* 0x004ABC30 sceMcGetInfo *format */
static sceMcTblGetDir s_mcFileInfo;                   /* 0x004ABC40 (0x40) SetFileInfo/Rename info block */
static McDescParam s_mcDesc __attribute__((aligned(64)));         /* 0x004ABC80 (0x30) */
static McNameParam s_mcName;                          /* 0x004ABCB0 (0x414) */
static McEndParam s_mcEndPara __attribute__((aligned(64)));       /* 0x004AC100 (0xC0) */
static char s_mcPwdBuf[0x400] __attribute__((aligned(64)));       /* 0x004AC1C0 Chdir result */
/* 0x004AC5C0..0x004AD1C0, between the Chdir buffer and s_mcRdata, is libmc
 * .bss that no code references (as in the bootstrap, 0x01049300): kept as an
 * unreferenced array so that s_mcRdata sits at its original offset. */
static u8  s_mcUnused[0xC00];                         /* 0x004AC5C0 */
/* RPC receive area: [0] result, [1] mcserv version, [2] mcman version (Init
 * receives 12 bytes, everything else 4); [15] is written only by
 * mc_get_rpc_state. */
static int s_mcRdata[16] __attribute__((aligned(64)));            /* 0x004AD1C0 */

static void mc_read_end_callback(void *end_para);
static void mc_getinfo_end_callback(void *end_para);
static void mc_chdir_end_callback(void *end_para);

// FUNC 0x00272E08 sceMcInit
// Creates the library semaphore on first use (init 1, max 1), waits for any
// pending request (sceMcSync(0, NULL, NULL)), takes the semaphore, binds the
// mcserv client (spinning 0x100000 iterations between bind polls; a negative
// bind result prints "bind error libmc \n" and hangs forever), then runs
// the synchronous handshake RPC 0xFE (send: s_mcDesc as-is, 0x30 bytes;
// receive 12 bytes) and releases the semaphore.  Returns:
//   RPC error ret < 0    -> ret - 100, client unbound
//   mcserv ver < 0x20A   -> -120, client unbound (message printed)
//   mcman  ver < 0x20E   -> -121, client unbound (message printed)
//   else                 -> s_mcRdata[0] (mcserv init result).
// Version compares are signed (slti).  Caller: mc_init_once (0x002674E0).
int sceMcInit(void)
{
    struct SemaParam sema_param;
    int spin;
    int ret;

    if (s_mcSema < 0) {
        sema_param.option    = 0;
        sema_param.initCount = 1;
        sema_param.maxCount  = 1;
        s_mcSema = CreateSema(&sema_param);
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
        for (spin = MC_BIND_SPIN; spin != 0; spin--)
            __asm__ __volatile__("");              /* busy delay */
    }

    ret = sceSifCallRpc(&s_mcClient, MC_RPC_INIT, 0, &s_mcDesc, sizeof(s_mcDesc),
                        s_mcRdata, MC_INIT_RDATA_SIZE, NULL, NULL);
    SignalSema(s_mcSema);

    if (ret < 0) {
        s_mcClient.serve = NULL;
        return ret - MC_ERR_RPC_BIAS;
    }
    if (s_mcRdata[1] < MC_MIN_MCSERV_VER) {        /* mcserv.irx version */
        printf("libmc: too old release of mcserv.irx\n");
        s_mcClient.serve = NULL;
        return MC_ERR_OLD_MCSERV;
    }
    if (s_mcRdata[2] < MC_MIN_MCMAN_VER) {         /* mcman.irx version */
        printf("libmc: too old release of mcman.irx\n");
        s_mcClient.serve = NULL;
        return MC_ERR_OLD_MCMAN;
    }
    return s_mcRdata[0];
}

// FUNC 0x00272FC0 mc_get_rpc_state
// Internal accessor with no callers (it sits unreferenced in sceMcInit's
// code block): *rdata = s_mcRdata, *func_no = &s_mcFuncNo, stores the
// semaphore id into s_mcRdata[15] and returns &s_mcClient.  Sony-internal
// name unknown.
sceSifClientData *mc_get_rpc_state(int **rdata, int **func_no)
{
    *rdata = s_mcRdata;
    *func_no = &s_mcFuncNo;
    s_mcRdata[15] = s_mcSema;
    return &s_mcClient;
}

// FUNC 0x00272FF0 sceMcChangeThreadPriority
// Asynchronous RPC 0x14 with level at s_mcDesc.origin (+0x14); pending code
// sceMcFuncChgPrior (0x14).
int sceMcChangeThreadPriority(int level)
{
    int ret;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.origin = level;
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncChgPrior, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncChgPrior;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x002730A8 sceMcGetSlotMax
// SYNCHRONOUS RPC 0x15 (mode 0) with port at s_mcDesc.port; releases the
// semaphore itself and returns the RPC error, or s_mcRdata[0] (slot count)
// on success.  Does not set s_mcFuncNo.
int sceMcGetSlotMax(int port)
{
    int ret;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.port = port;
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncSlotMax, 0,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret != 0) {
        SignalSema(s_mcSema);
        return ret;
    }
    SignalSema(s_mcSema);
    return s_mcRdata[0];
}

// FUNC 0x00273168 sceMcOpen
// Asynchronous RPC 2: s_mcName {port, slot, flags = mode, name}, ssize 0x414;
// pending code sceMcFuncNoOpen.  sceMcSync's result is the fd.  Also the
// engine of sceMcMkdir (mode 0x40).
int sceMcOpen(int port, int slot, const char *name, int mode)
{
    int ret;

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
    strncpy(s_mcName.name, name, MC_NAME_MAX);
    s_mcName.port  = port;
    s_mcName.flags = mode;
    s_mcName.slot  = slot;
    s_mcName.name[MC_NAME_MAX] = '\0';
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoOpen, SIF_RPCM_NOWAIT,
                        &s_mcName, sizeof(s_mcName), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoOpen;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x002732A0 sceMcMkdir
// sceMcOpen(port, slot, name, sceMcFileCreateDir); on a successful issue the
// pending code is overwritten with sceMcFuncNoMkdir (0xB).
int sceMcMkdir(int port, int slot, const char *name)
{
    int ret = sceMcOpen(port, slot, name, sceMcFileCreateDir);

    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoMkdir;
    return ret;
}

// FUNC 0x002732D8 sceMcClose
// Asynchronous RPC 3 with fd at s_mcDesc.fd; pending code sceMcFuncNoClose.
int sceMcClose(int fd)
{
    int ret;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd = fd;
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoClose, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoClose;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273390 sceMcSeek
// Asynchronous RPC 4: fd, offset (+0x10), origin/whence (+0x14); pending
// code sceMcFuncNoSeek.
int sceMcSeek(int fd, int offset, int origin)
{
    int ret;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd     = fd;
    s_mcDesc.offset = offset;
    s_mcDesc.origin = origin;
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoSeek, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoSeek;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273468 mc_read_end_callback
// sceMcRead's RPC end function (runs from the SIF RPC-end handler with
// end_para = &s_mcEndPara).  mcserv DMAs only the cache-line (64-byte)
// aligned middle of the user buffer directly; the unaligned head (size1
// bytes from src1 to dest1) and tail (size2 bytes from src2 to dest2) come
// back in the end parameter and are copied here byte by byte, read through
// the uncached mirror (| 0x20000000).  Counts are re-read every iteration,
// compared signed (slt).
static void mc_read_end_callback(void *end_para)
{
    McEndParam *end = (McEndParam *)UNCACHED(end_para);
    u8 *dst;
    int i;

    if (end->size1 != 0) {
        dst = end->dest1;
        for (i = 0; i < end->size1; i++)
            *dst++ = end->src1[i];
    }
    if (end->size2 != 0) {
        dst = end->dest2;
        for (i = 0; i < end->size2; i++)
            *dst++ = end->src2[i];
    }
}

// FUNC 0x002734F8 sceMcRead
// Asynchronous RPC 5: fd, param = &s_mcEndPara, buffer = buf, size.  Writes
// back the D-cache of the user buffer (size bytes) and of the 0xC0-byte end
// parameter first; end function mc_read_end_callback(&s_mcEndPara).  Pending
// code sceMcFuncNoRead; sceMcSync's result is the byte count.
int sceMcRead(int fd, void *buf, int size)
{
    int ret;

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
    sceSifWriteBackDCache(&s_mcEndPara, sizeof(s_mcEndPara));
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoRead, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE,
                        mc_read_end_callback, &s_mcEndPara);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoRead;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273618 sceMcWrite
// Asynchronous RPC 6.  size <= 16 (signed compare, slti 0x11): all bytes go
// inline in s_mcDesc.data (origin = size, buffer = NULL, size field 0).
// Otherwise the head up to the next 16-byte boundary (0..15 bytes, computed
// as ((buf-1) & ~0xF) - (buf-0x10)) goes inline and the aligned remainder is
// passed by address (buffer = buf+head, size = size-head, origin = head).
// Then FlushCache(0) (write back the whole D-cache) and the call.  Pending
// code sceMcFuncNoWrite.  Quirk: the inline copy loop compares unsigned
// (sltu) and re-reads s_mcDesc.origin every iteration, so a negative size
// would overrun s_mcDesc.data.  Same code as the bootstrap's sceMcWrite
// (0x01015FC8).  Caller: mc_write_sync (0x002679C8).
int sceMcWrite(int fd, const void *buf, int size)
{
    unsigned int i;
    u32 below_boundary, buf_minus_16;
    int head;
    int ret;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd = fd;
    if (size <= MC_WRITE_INLINE_MAX) {
        s_mcDesc.origin = size;
        s_mcDesc.size   = 0;
        s_mcDesc.buffer = NULL;
    } else {
        below_boundary = ((u32)buf - 1) & 0xFFFFFFF0;  /* last 16-byte boundary below buf */
        buf_minus_16   = (u32)buf - 0x10;
        head = below_boundary - buf_minus_16;           /* bytes up to the next boundary */
        s_mcDesc.size   = size - head;
        s_mcDesc.origin = head;
        s_mcDesc.buffer = (u8 *)buf + head;
    }
    for (i = 0; i < (unsigned int)s_mcDesc.origin; i++)
        s_mcDesc.data[i] = ((const u8 *)buf)[i];
    FlushCache(WRITEBACK_DCACHE);
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoWrite, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoWrite;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273798 mc_alarm_handler
// SetAlarm callback of mc_delay_thread (interrupt context): wakes the thread
// whose id was passed as the alarm argument (iWakeupThread 0x00286B98),
// then the usual handler epilogue `sync.l; ei`.
static void mc_alarm_handler(int alarm_id, u16 time, void *thread_id)
{
    iWakeupThread((int)thread_id);
    ExitHandler();
}

// FUNC 0x002737C0 mc_delay_thread
// Sleeps the calling thread for `ticks` H-sync units: SetAlarm(ticks & 0xFFFF,
// mc_alarm_handler, GetThreadId()) then SleepThread() as a `j` tail call (so
// $v0 on return is SleepThread's result; the only caller, sceMcSync's wait
// loop with ticks 0x3C, ignores it).
static void mc_delay_thread(int ticks)
{
    SetAlarm((u16)ticks, mc_alarm_handler, (void *)GetThreadId());
    SleepThread();
}

// FUNC 0x00273808 sceMcSync
// Returns -1 if nothing is pending (s_mcFuncNo == 0).  Otherwise polls
// sceSifCheckStatRpc(&s_mcClient); with mode == 0 and the call still in
// flight it loops {sceSifCheckStatRpc; mc_delay_thread(0x3C)} until done.
// *cmd (if non-NULL) always receives the pending code.  Still running
// (mode != 0) -> 0.  Finished -> clears s_mcFuncNo, *result (if non-NULL) =
// s_mcRdata[0], SignalSema(s_mcSema), returns 1.  Same code as the
// bootstrap's sceMcSync (0x010161B8).  Callers: sceMcInit, mc_sync_wait
// (0x002678E8).
int sceMcSync(int mode, int *cmd, int *result)
{
    int busy;

    if (s_mcFuncNo == 0)
        return sceMcExecIdle;

    busy = sceSifCheckStatRpc(&s_mcClient.rpcd);
    if (mode == 0 && busy != 0) {
        while (sceSifCheckStatRpc(&s_mcClient.rpcd) != 0)
            mc_delay_thread(MC_SYNC_DELAY_HSYNC);
        busy = 0;
    }
    busy = !busy;                   /* now 1 = finished (sceMcExecFinish), 0 = sceMcExecRun */
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

// FUNC 0x002738E8 mc_getinfo_end_callback
// sceMcGetInfo's RPC end function (reached only through that pointer): through the uncached mirror of the end
// parameter, copies +0x00 -> *type, +0x04 -> *free, +0x90 -> *format for
// each user pointer that sceMcGetInfo saved as non-NULL.  (Bootstrap
// 0x01016298.)
static void mc_getinfo_end_callback(void *end_para)
{
    McEndParam *end = (McEndParam *)UNCACHED(end_para);

    if (s_mcInfoType != NULL)
        *s_mcInfoType = end->size1;         /* type */
    if (s_mcInfoFree != NULL)
        *s_mcInfoFree = end->size2;         /* free */
    if (s_mcInfoFormat != NULL)
        *s_mcInfoFormat = end->formatted;
}

// FUNC 0x00273940 sceMcGetInfo
// Asynchronous RPC 1: port, slot, param = &s_mcEndPara and three request
// flags (origin = type wanted, offset = free wanted, size = format wanted,
// each 1/0).  Saves the three user pointers for mc_getinfo_end_callback,
// writes back the end parameter's D-cache, pending code sceMcFuncNoCardInfo.
// Caller: mc_getinfo_sync (0x002673C0).
int sceMcGetInfo(int port, int slot, int *type, int *free, int *format)
{
    int ret;

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
    sceSifWriteBackDCache(&s_mcEndPara, sizeof(s_mcEndPara));
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoCardInfo, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE,
                        mc_getinfo_end_callback, &s_mcEndPara);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoCardInfo;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273AC8 sceMcGetDir
// Asynchronous RPC 0xD: s_mcName {port, slot, flags = mode, maxent,
// table, name}, ssize 0x414.  Writes back maxent * 0x40 bytes of the table's
// D-cache when maxent >= 0 (signed test).  Pending code sceMcFuncNoGetDir;
// sceMcSync's result is the number of entries.
int sceMcGetDir(int port, int slot, const char *name, unsigned int mode,
                int maxent, sceMcTblGetDir *table)
{
    int ret;

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
    strncpy(s_mcName.name, name, MC_NAME_MAX);
    s_mcName.name[MC_NAME_MAX] = '\0';
    if (maxent >= 0)
        sceSifWriteBackDCache(table, maxent * (int)sizeof(sceMcTblGetDir));
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoGetDir, SIF_RPCM_NOWAIT,
                        &s_mcName, sizeof(s_mcName), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoGetDir;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273C20 mc_chdir_end_callback
// sceMcChdir's RPC end function (end_para = the caller's pwd buffer): if pwd
// is non-NULL, copies the previous directory string that mcserv left in
// s_mcPwdBuf (read via the uncached mirror) into pwd, clamped to 0x3FF
// chars, and NUL-terminates.  strlen is called twice when the length is
// < 0x400 (unsigned compare).
static void mc_chdir_end_callback(void *end_para)
{
    char *pwd = (char *)end_para;
    size_t len;

    if (pwd == NULL)
        return;
    if (strlen((const char *)UNCACHED(s_mcPwdBuf)) < sizeof(s_mcPwdBuf))
        len = strlen((const char *)UNCACHED(s_mcPwdBuf));
    else
        len = sizeof(s_mcPwdBuf) - 1;
    memcpy(pwd, UNCACHED(s_mcPwdBuf), len);
    pwd[len] = '\0';
}

// FUNC 0x00273CA8 sceMcChdir
// Asynchronous RPC 0xC: s_mcName {port, slot, table = s_mcPwdBuf, name =
// path} (flags/maxent left stale), ssize 0x414; writes back the 0x400-byte
// result buffer's D-cache; end function mc_chdir_end_callback(pwd).
// Pending code sceMcFuncNoChDir.
int sceMcChdir(int port, int slot, const char *path, char *pwd)
{
    int ret;

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
    strncpy(s_mcName.name, path, MC_NAME_MAX);
    s_mcName.name[MC_NAME_MAX] = '\0';
    sceSifWriteBackDCache(s_mcPwdBuf, sizeof(s_mcPwdBuf));
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoChDir, SIF_RPCM_NOWAIT,
                        &s_mcName, sizeof(s_mcName), s_mcRdata, MC_RDATA_SIZE,
                        mc_chdir_end_callback, pwd);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoChDir;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273DF8 sceMcFormat
// Asynchronous RPC 0x10 with port/slot in s_mcDesc; pending code
// sceMcFuncNoFormat.
int sceMcFormat(int port, int slot)
{
    int ret;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.slot = slot;
    s_mcDesc.port = port;
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoFormat, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoFormat;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273EC0 sceMcDelete
// Asynchronous RPC 0xF: s_mcName {port, slot, flags = 0, name}, ssize 0x414;
// pending code sceMcFuncNoDelete.
int sceMcDelete(int port, int slot, const char *name)
{
    int ret;

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
    strncpy(s_mcName.name, name, MC_NAME_MAX);
    s_mcName.port  = port;
    s_mcName.slot  = slot;
    s_mcName.name[MC_NAME_MAX] = '\0';
    s_mcName.flags = 0;
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoDelete, SIF_RPCM_NOWAIT,
                        &s_mcName, sizeof(s_mcName), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoDelete;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00273FE8 sceMcFlush
// Asynchronous RPC 0xA with fd in s_mcDesc; pending code sceMcFuncNoFlush.
int sceMcFlush(int fd)
{
    int ret;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.fd = fd;
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoFlush, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoFlush;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x002740A0 sceMcSetFileInfo
// Asynchronous RPC 0xE: valid &= 7 (Create|Modify|Attr), the caller's
// 0x40-byte sceMcTblGetDir is copied into s_mcFileInfo (inlined by the
// compiler as an unaligned ldl/ldr + sdl/sdr doubleword copy - no call to
// memcpy), s_mcName {port, slot, flags = valid, table = &s_mcFileInfo,
// name}, then FlushCache(0) and the call (ssize 0x414).  Pending code
// sceMcFuncNoFileInfo.
int sceMcSetFileInfo(int port, int slot, const char *name, const char *info,
                     unsigned int valid)
{
    int ret;

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
    strncpy(s_mcName.name, name, MC_NAME_MAX);
    s_mcName.name[MC_NAME_MAX] = '\0';
    FlushCache(WRITEBACK_DCACHE);
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoFileInfo, SIF_RPCM_NOWAIT,
                        &s_mcName, sizeof(s_mcName), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoFileInfo;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x00274278 sceMcRename
// Implemented as a SetFileInfo request (RPC 0xE) with flags = 0x10 (name
// valid): org goes to s_mcName.name, new_name is strncpy'd (max 0x20) into
// s_mcFileInfo.EntryName and EntryName[31] forced to 0 (the rest of
// s_mcFileInfo is whatever the last sceMcSetFileInfo left there); FlushCache(0).
// Argument check: org NULL/empty or new_name NULL -> -210 (an empty new_name
// is accepted).  Pending code sceMcFuncNoRename (0x13).
int sceMcRename(int port, int slot, const char *org, const char *new_name)
{
    int ret;

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
    strncpy(s_mcName.name, org, MC_NAME_MAX);
    s_mcName.name[MC_NAME_MAX] = '\0';
    strncpy((char *)s_mcFileInfo.EntryName, new_name, sizeof(s_mcFileInfo.EntryName));
    s_mcFileInfo.EntryName[sizeof(s_mcFileInfo.EntryName) - 1] = '\0';  /* byte +0x3F of the info block */
    s_mcName.table = &s_mcFileInfo;
    FlushCache(WRITEBACK_DCACHE);
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoFileInfo, SIF_RPCM_NOWAIT,
                        &s_mcName, sizeof(s_mcName), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoRename;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x002743E0 sceMcUnformat
// Asynchronous RPC 0x11 with port/slot in s_mcDesc; pending code
// sceMcFuncNoUnformat.
int sceMcUnformat(int port, int slot)
{
    int ret;

    if (PollSema(s_mcSema) < 0)
        return MC_ERR_SEMA;
    if (s_mcClient.serve == NULL) {
        SignalSema(s_mcSema);
        return MC_ERR_NOT_BOUND;
    }
    s_mcDesc.slot = slot;
    s_mcDesc.port = port;
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoUnformat, SIF_RPCM_NOWAIT,
                        &s_mcDesc, sizeof(s_mcDesc), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoUnformat;
    else
        SignalSema(s_mcSema);
    return ret;
}

// FUNC 0x002744A8 sceMcGetEntSpace
// Asynchronous RPC 0x12: s_mcName {port, slot, name = path}, ssize 0x414;
// pending code sceMcFuncNoEntSpace.  Last libmc function (libscf follows).
int sceMcGetEntSpace(int port, int slot, const char *path)
{
    int ret;

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
    strncpy(s_mcName.name, path, MC_NAME_MAX);
    s_mcName.name[MC_NAME_MAX] = '\0';
    ret = sceSifCallRpc(&s_mcClient, sceMcFuncNoEntSpace, SIF_RPCM_NOWAIT,
                        &s_mcName, sizeof(s_mcName), s_mcRdata, MC_RDATA_SIZE, NULL, NULL);
    if (ret == 0)
        s_mcFuncNo = sceMcFuncNoEntSpace;
    else
        SignalSema(s_mcSema);
    return ret;
}
