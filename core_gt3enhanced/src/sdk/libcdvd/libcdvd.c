/* libcdvd.c - Sony EE libcdvd client "PsIIlibcdvd 2240",
 * 0x00270628..0x00271A60.  Same machine code as the bootstrap's
 * sdk/libcdvd/libcdvd.c (0x01012FD8..0x01014410): its source with the core
 * addresses.
 *
 * Only the core of the library is linked: init / exit, the semaphores and
 * the EE callback thread, the power-off notification, the N-command gate
 * (+ one N-command, DISKREADY 0x0E), sceCdSync, sceCdSearchFile, the
 * S-command gate, sceCdDiskReady and sceCdMmode.  In the main executable
 * only sceCdInit and sceCdMmode have callers (one each, outside the unit).
 *
 * IOP RPC servers (init bound by every sceCdInit, the others lazily; bind
 * retry loops are endless except _sceCdPOffCbBind's):
 *   0x80000592 CD init        client @0x004AB7A8   (sceCdInit)
 *   0x80000593 S-commands     client @0x00305E08   (_sceCdCheckSCmd)
 *   0x80000595 N-commands     client @0x00305590   (_sceCdCheckNCmd)
 *   0x80000596 power-off CB   client @0x004AB5B8   (_sceCdPOffCbBind)
 *   0x80000597 search file    client @0x004AB780   (sceCdSearchFile)
 *   0x8000059A disk ready     client @0x004AB7D0   (sceCdDiskReady)
 * SIF command 0x80000012 = power-off notification (_sceCdPOffCmdHandler).
 *
 * Debug output: sce_printf_deci2 (kernel TTY printf, runtime/newlib/G007_stdio.h) gated
 * by s_cd_debug > 0, which only becomes 1 if the IOP's sceCdInit reply
 * carries the 0xFE marker.
 */
/* libcdvd was built against Sony's eekernel.h, which declares its kernel TTY
 * printf (here sce_printf_deci2) as returning void: after each debug print
 * the retail code treats $v0 as free (the register choice of the following
 * s_cd_debug checks shows it; the bootstrap's libcdvd.c does the same with
 * scePrintf).  runtime/newlib/G007_stdio.h declares the function's real int result, so
 * that declaration is renamed away here.  Same function; libcdvd never uses
 * the result.                                                               */
#define sce_printf_deci2 _sce_printf_deci2_int_decl
#include "sdk/libcdvd/G004_libcdvd.h"
#include "sdk/libcdvd/libcdvd_internal.h"
#undef sce_printf_deci2
void sce_printf_deci2(const char *fmt, ...);   /* 0x00287B80 */

// DATA_START 0x003043C0

/* ======================================================================== */
/* Library state (file statics, as the bootstrap's libcdvd.c)               */
/* ======================================================================== */

/* ---- .data 0x003043C0..0x00305E30 --------------------------------------- */
/* Everything down to s_scmd_client is in the retail .data, zeros included:
 * egcs puts a C variable in .data only when it has an initialiser ("= 0"
 * included; one without is .bss) and lays .data out in definition order, so
 * every one of them is initialised explicitly.  s_ncmd_rbuf's aligned(64)
 * makes the section 64-aligned: the zeros before 0x003043C0 (after u128's
 * frame info) and 0x0030441C..0x00304440 are that padding in the original
 * too.  (GCC 14 moves the zero ones to .bss; the values are the same.)    */

/* Library version tag, exactly 16 chars, no NUL (the next word is s_cd_debug);
 * nothing references it.                                                    */
static BOOT_USED char s_libcdvd_version[16] = "PsIIlibcdvd 2240";  /* 0x003043C0 */

static int  s_cd_debug        = 0;          /* 0x003043D0 debug verbosity (>0 prints)    */
static int  s_cd_cb_thread_id = 0;          /* 0x003043D4 EE callback thread id, 0 = none */
static int  s_scmd_keep       = 0;          /* 0x003043D8 keep_cmd of the last S-command */
static int  s_ncmd_keep       = 0;          /* 0x003043DC keep_cmd of the last N-command */
static int  s_cd_cb_sema      = -1;         /* 0x003043E0 wakes the callback thread       */
static CD_ISR_VOLATILE int s_cd_init_busy = 0;  /* 0x003043E4 1 while sceCdInit / POff bind runs */
static int  s_ncmd_sema       = -1;         /* 0x003043E8 N-command mutex                 */
static int  s_scmd_sema       = -1;         /* 0x003043EC S-command mutex                 */
static CD_ISR_VOLATILE int s_cd_cb_busy = 0;    /* 0x003043F0 completion/callback pending */
static int  g_003043F4        = 0;          /* 0x003043F4 only cleared (sceCdInit)        */
static int  s_ncmd_bind       = -1;         /* 0x003043F8 <0: N-cmd server not bound      */
static int  s_poff_bind       = -1;         /* 0x003043FC <0: POff server not bound       */
static int  s_search_bind     = -1;         /* 0x00304400 <0: search server not bound     */
static int  s_diskready_bind  = -1;         /* 0x00304404 <0: disk-ready server not bound */
static int  s_scmd_bind       = -1;         /* 0x00304408 <0: S-cmd server not bound      */
static int  s_init_bind       = -1;         /* 0x0030440C <0: init server not bound       */
static int  s_init_count      = 0;          /* 0x00304410 number of sceCdInit calls       */
static CD_ISR_VOLATILE int s_cd_cb_num = 0;     /* 0x00304414 last completion reason; -1 = thread exit request */
static CD_ISR_VOLATILE int s_cd_cb_num_user = 0;    /* 0x00304418 reason passed to the user callback */

/* SIF RPC buffers: keep the alignments (the IOP DMAs into them) and the
 * addresses.  N-command receive buffer (only the first word is used here);
 * the rest of the 0x1150-byte span 0x00304440..0x00305590 is zero-filled
 * .data no linked code references (? the buffers of Sony N-commands such as
 * sceCdRead that were not linked in): s_ncmd_unused keeps that span, so the
 * following variables sit at their original offsets.                        */
static u32  s_ncmd_rbuf[0x10] __attribute__((aligned(64))) = { 0 };   /* 0x00304440 */
static BOOT_USED u8 s_ncmd_unused[0x1150 - 0x40] = { 0 };             /* 0x00304480 unreferenced */
static sceSifClientData s_ncmd_client = { 0 };                        /* 0x00305590 server 0x80000595 */
/* S-command / init / disk-ready receive buffer (0x440 bytes to 0x00305A00;
 * sceCdInit uses 16 bytes, the others 4).                                   */
static u32  s_scmd_rbuf[0x110] __attribute__((aligned(64))) = { 0 };  /* 0x003055C0 */
/* S-command send buffer: 0x408 bytes up to s_scmd_client (only word 0 used). */
static u32  s_scmd_sbuf[0x102] __attribute__((aligned(64))) = { 0 };  /* 0x00305A00 */
static sceSifClientData s_scmd_client = { 0 };                        /* 0x00305E08 server 0x80000593 */

/* ---- .bss 0x004AB540..0x004AB814 (statics without an initialiser: egcs
 * writes them at the end of the output in definition order) -------------- */
static sceCdCBFunc s_cd_user_cb;            /* 0x004AB540 user completion callback */
static void (*s_poff_func)(void *);         /* 0x004AB544 power-off callback */
static void *s_poff_arg;                    /* 0x004AB548 its argument */
static int  g_004AB54C;                     /* 0x004AB54C only cleared (callback thread exit) */
static int  s_cd_thread_id;                 /* 0x004AB550 thread that called sceCdInit */
static struct ThreadParam s_cd_thread_status;   /* 0x004AB558 ReferThreadStatus scratch */
static struct ThreadParam s_cd_cb_thread_param; /* 0x004AB588 callback thread CreateThread block */
static sceSifClientData s_poff_client;      /* 0x004AB5B8 server 0x80000596 */
static int  s_poff_sbuf;                    /* 0x004AB5E0 written 0x0B, never sent */
static _sceCdSearchSbuf s_search_sbuf __attribute__((aligned(64)));   /* 0x004AB600 */
static int  s_search_rbuf[0x10] __attribute__((aligned(64)));         /* 0x004AB740 (4 bytes used) */
static sceSifClientData s_search_client;    /* 0x004AB780 server 0x80000597 */
static sceSifClientData s_init_client;      /* 0x004AB7A8 server 0x80000592 */
static sceSifClientData s_diskready_client; /* 0x004AB7D0 server 0x8000059A */
static int  s_init_sbuf __attribute__((aligned(64)));                 /* 0x004AB800 init_mode send word */
static int  s_diskready_sbuf __attribute__((aligned(16)));            /* 0x004AB810 disk-ready mode send word */

/* Sony's nopdelay(): the 0x100000-constant busy loop inlined after every
 * failed bind (v0 = 0x100000; loop { v0--; nop x4 } while (v0 != -1), i.e.
 * 0x100001 passes).  The loop is empty in the source: the four nops are the
 * old compiler's padding of a loop shorter than seven instructions (R5900
 * short-loop erratum), placed around the decrement.  The GCC 14 build writes
 * them out, so each pass costs what it did before; the asm also keeps GCC 14
 * from deleting the loop.  An inline function, not a macro, as in the
 * bootstrap.                                                                */
#ifdef BOOT_OLD_GCC
#define CD_NOPDELAY_PASS()  __asm__ __volatile__("")
#else
#define CD_NOPDELAY_PASS()  __asm__ __volatile__("nop\n\tnop\n\tnop\n\tnop")
#endif
static __inline__ void _sceCdNopDelay(void)
{
    int i = 0x100000;

    do
        CD_NOPDELAY_PASS();
    while (i-- != 0);
}

/* forward declarations (address order) */
static void _sceCdAlarmHandler(int alarm_id, u16 time, void *arg);
static void _sceCdDelay(u16 hsync);
static void _sceCdCbDispatch(int *funcnum);
static void _sceCdCallbackThread(void *arg);
static void _sceCdAlignReadCb(void *data);
static void _sceCdSemaInit(void);
static void _sceCdEeCbExit(void);
static void _sceCdPOffCmdHandler(void *pkt, void *harg);
static int  _sceCdPOffCbBind(void);
static int  _sceCdCheckNCmd(int cur_cmd);
static int  _sceCdSyncS(int mode);
static int  _sceCdCheckSCmd(int cur_cmd);

// FUNC 0x00270628 _sceCdAlarmHandler
// SetAlarm handler of _sceCdDelay: iSignalSema(semaphore passed as the alarm
// argument), then ExitHandler() (`sync.l; ei`).  $v0 is left = iSignalSema's
// result (the kernel does not use the handler's return value).
static void _sceCdAlarmHandler(int alarm_id, u16 time, void *arg)
{
    iSignalSema((int)arg);
    ExitHandler();
}

// FUNC 0x00270650 _sceCdDelay
// Sleeps for `hsync` H-SYNC periods: CreateSema{max 1, init 0}, SetAlarm with
// _sceCdAlarmHandler, WaitSema, DeleteSema.  Only maxCount / initCount /
// option of the SemaParam are written (the rest is stack garbage, as in the
// binary).  No error checks.  Callers: sceCdSync / _sceCdSyncS with 0x3C
// (CD_SYNC_DELAY_HSYNC).
static void _sceCdDelay(u16 hsync)
{
    struct SemaParam param;         /* sp+0x00 */
    int wake_sema;

    param.maxCount  = 1;            /* sp+0x04 */
    param.initCount = 0;            /* sp+0x08 */
    param.option    = 0;            /* sp+0x14 */
    wake_sema = CreateSema(&param);
    SetAlarm(hsync, _sceCdAlarmHandler, (void *)wake_sema);
    WaitSema(wake_sema);
    DeleteSema(wake_sema);
}

// FUNC 0x002706B8 sceCdCallback
// Installs the user completion callback and returns the previous one; while
// an N-command is in flight (sceCdSync(1) != 0) it returns NULL without
// changing anything.  The swap is done with interrupts disabled.  No caller.
sceCdCBFunc sceCdCallback(sceCdCBFunc func)
{
    sceCdCBFunc old_func;

    if (sceCdSync(1) != 0)
        return NULL;

    DIntr();
    old_func = s_cd_user_cb;
    s_cd_user_cb = func;
    EIntr();
    return old_func;
}

// FUNC 0x00270710 _sceCdCbDispatch
// Generic N-command completion end-function (interrupt context; no static
// reference - only reached by the tail jump from _sceCdAlignReadCb).
// Records the completion reason, releases the N-command semaphore and wakes
// the callback thread if there is one and a user callback is set; otherwise
// clears the busy flag at once.  Reason 0x0B is swallowed without releasing
// the semaphore.  s_cd_cb_num is re-read after each store (volatile).
static void _sceCdCbDispatch(int *funcnum)
{
    CD_VI(s_cd_cb_num) = *funcnum;
    CD_VI(s_cd_cb_num_user) = CD_VI(s_cd_cb_num);

    if (CD_VI(s_cd_cb_num) == CDVD_CBNUM_POFF) {
        CD_VI(s_cd_cb_num) = 0;
        CD_VI(s_cd_cb_busy) = 0;
        return;
    }

    iSignalSema(NCMD_SEMA_V);
    if (s_cd_cb_thread_id != 0 && s_cd_user_cb != NULL)
        iSignalSema(CD_VI(s_cd_cb_sema));   /* the thread clears s_cd_cb_busy */
    else
        CD_VI(s_cd_cb_busy) = 0;
    CD_VI(s_cd_cb_num) = 0;
}

// FUNC 0x002707B0 _sceCdCallbackThread
// Entry of the EE callback thread created by sceCdInitEeCB (its address is
// taken there, 0x002708DC).  Waits on s_cd_cb_sema; on the exit request
// (s_cd_cb_num == -1, set by _sceCdEeCbExit) clears its state and
// ExitDeleteThread()s.  Otherwise prints the debug line, calls the user
// callback with the reason (only if both callback and reason are non-zero)
// and clears the busy flag.  The debug print passes the callback *pointer*
// as the first %d.  `arg` is StartThread's NULL.  The thread loop never
// returns, so sceCdInitEeCB follows the loop directly (0x00270870,
// ref/extra_starts.txt).
static void _sceCdCallbackThread(void *arg)
{
    for (;;) {
        WaitSema(s_cd_cb_sema);
        if (s_cd_cb_num == -1) {
            s_cd_cb_busy = 0;
            s_cd_cb_num = 0;
            s_cd_cb_thread_id = 0;
            g_004AB54C = 0;
            ExitDeleteThread();             /* does not return */
        }
        if (s_cd_debug > 0)
            sce_printf_deci2("sceCdCbfunc= %d sceCdCbfunc_num= %d\n",
                             (int)s_cd_user_cb, s_cd_cb_num_user);
        if (s_cd_user_cb != NULL && CD_VI(s_cd_cb_num_user) != 0)
            s_cd_user_cb(s_cd_cb_num_user);
        CD_VI(s_cd_cb_busy) = 0;
    }
}

// FUNC 0x00270870 sceCdInitEeCB
/* Creates and starts the EE callback thread (entry _sceCdCallbackThread,
 * caller-supplied stack, gp = _gp, given priority) and returns 1; if the
 * thread already exists only changes its priority and returns 0.  Also
 * records the calling thread (GetThreadId / ReferThreadStatus).  Only the
 * entry/stack/stackSize/gpReg/initPriority fields of the param block are
 * written.  No caller.                                                       */
int sceCdInitEeCB(int priority, void *stack, int stack_size)
{
    struct ThreadParam *param;
    int caller_tid;
    int created = 1;

    if (s_cd_cb_thread_id == 0) {
        caller_tid = GetThreadId();
        s_cd_thread_id = caller_tid;
        ReferThreadStatus(caller_tid, &s_cd_thread_status);

        param = &s_cd_cb_thread_param;
        param->stackSize    = stack_size;           /* +0x0C */
        param->gpReg        = _gp;                  /* +0x10 */
        param->entry        = _sceCdCallbackThread; /* +0x04 */
        param->stack        = stack;                /* +0x08 */
        param->initPriority = priority;             /* +0x14 */
        s_cd_cb_thread_id = CreateThread(param);
        StartThread(s_cd_cb_thread_id, NULL);
    } else {
        ChangeThreadPriority(s_cd_cb_thread_id, priority);
        created = 0;
    }
    return created;
}

// FUNC 0x00270948 _sceCdAlignReadCb
// End-function for reads into buffers that are not 64-byte aligned: through
// the uncached view of the IOP-written _sceCdAlignData, copies size1 bytes
// of buf1 to dest1 and size2 bytes of buf2 to dest2 (byte loops, sizes
// re-read every pass, dest loaded once; no NULL checks), then tail-jumps to
// _sceCdCbDispatch(&s_cd_cb_num).  No static reference.
static void _sceCdAlignReadCb(void *data)
{
    _sceCdAlignData *align = (_sceCdAlignData *)UNCACHED(data);
    u8 *dst;
    int i;

    if (align->size1 > 0) {
        dst = align->dest1;
        for (i = 0; i < align->size1; i++)
            dst[i] = align->buf1[i];
    }
    if (align->size2 > 0) {
        dst = align->dest2;
        for (i = 0; i < align->size2; i++)
            dst[i] = align->buf2[i];
    }
    _sceCdCbDispatch((int *)&s_cd_cb_num);   /* j 0x00270710 */
}

// FUNC 0x002709E8 _sceCdSemaInit
// Creates the library semaphores unless both the N- and S-command mutexes
// already exist: N-cmd mutex {max 1, init 1}, S-cmd mutex {1, 1}, callback
// wake-up {max 1, init 0}; clears the busy flag.  Same partial SemaParam
// initialisation as _sceCdDelay.  Quirk: if only one of the two mutex ids
// is -1, all three semaphores are created again (the surviving ones leak).
static void _sceCdSemaInit(void)
{
    struct SemaParam param;         /* sp+0x00 */

    if (s_ncmd_sema != -1 && s_scmd_sema != -1)
        return;

    param.option    = 0;            /* sp+0x14 */
    param.initCount = 1;            /* sp+0x08 */
    param.maxCount  = 1;            /* sp+0x04 */
    NCMD_SEMA_V = CreateSema(&param);
    SCMD_SEMA_V = CreateSema(&param);
    param.initCount = 0;
    CD_VI(s_cd_cb_sema) = CreateSema(&param);
    CD_VI(s_cd_cb_busy) = 0;
}

// FUNC 0x00270A80 _sceCdEeCbExit
// Exit path of sceCdInit(SCECdEXIT): if the callback thread exists, posts the
// exit request (s_cd_cb_num = -1) and signals it; deletes the three
// semaphores; removes the power-off SIF command handler with interrupts
// disabled (EIntr is a tail call).  The ids are reset to -1 by the caller.
static void _sceCdEeCbExit(void)
{
    if (s_cd_cb_thread_id != 0) {
        CD_SET_CBNUM(-1);           /* exit request for the callback thread */
        SignalSema(s_cd_cb_sema);
    }
    DeleteSema(s_ncmd_sema);
    DeleteSema(s_scmd_sema);
    DeleteSema(s_cd_cb_sema);
    DIntr();
    sceSifRemoveCmdHandler(SIF_CMDC_CDVD_POFF);
    EIntr();
}

// FUNC 0x00270B00 sceCdPOffCallback
// Installs the power-off callback (func, addr) and returns the previous
// function.  Binds the power-off server first if that has not been done
// (s_poff_bind < 0; normally done by sceCdInit).  The pair is swapped with
// interrupts disabled, argument stored before the function.  No caller, so
// s_poff_func stays NULL and the power-off handler never calls anything.
void *sceCdPOffCallback(void (*func)(void *), void *addr)
{
    void (*old_func)(void *);

    if (s_poff_bind < 0)
        _sceCdPOffCbBind();

    DIntr();
    old_func = s_poff_func;
    s_poff_arg = addr;
    s_poff_func = func;
    EIntr();
    return (void *)old_func;
}

// FUNC 0x00270B70 _sceCdPOffCmdHandler
// SIF command handler for 0x80000012 (IOP power-off notification), installed
// by _sceCdPOffCbBind: calls the user power-off callback with its argument,
// unless none is set or sceCdInit / the power-off bind is in progress.
static void _sceCdPOffCmdHandler(void *pkt, void *harg)
{
    if (s_poff_func != NULL && s_cd_init_busy == 0)
        s_poff_func(s_poff_arg);
}

// FUNC 0x00270BB0 _sceCdPOffCbBind
// sceSifInitRpc(0); marks init busy; installs _sceCdPOffCmdHandler for SIF
// command 0x80000012 (interrupts disabled); if not yet bound, binds RPC
// server 0x80000596 (bind errors: debug print + nopdelay, retried forever;
// bound-but-no-server: nopdelay, retried - the 18th such attempt gives up
// and returns 0; the counter is never reset, bind errors in between do not
// count).  Then stores 0x0B in s_poff_sbuf (never passed anywhere) and sends
// fno 1 with SIF_RPCM_NOWAIT and no data to enable the IOP-side
// notification.  Returns 1 if that call was queued, else 0; init busy is
// cleared on both paths.  Callers: sceCdInit, sceCdPOffCallback.
static int _sceCdPOffCbBind(void)
{
    int no_server_tries;

    sceSifInitRpc(0);
    s_cd_init_busy = 1;
    DIntr();
    sceSifAddCmdHandler(SIF_CMDC_CDVD_POFF, _sceCdPOffCmdHandler, NULL);
    EIntr();

    if (s_poff_bind < 0) {
        no_server_tries = 0;
        while (1) {
            if (sceSifBindRpc(&s_poff_client, CD_SERVER_POFF, 0) < 0) {
                if (s_cd_debug > 0)
                    sce_printf_deci2("Libcdvd bind err PowerOffCB\n");
                _sceCdNopDelay();
                continue;
            }
            if (s_poff_client.serve != NULL) {
                s_poff_bind = 0;
                break;
            }
            _sceCdNopDelay();
            if (no_server_tries++ >= 17) {
                s_cd_init_busy = 0;
                return 0;
            }
        }
    }

    s_poff_sbuf = CDVD_CBNUM_POFF;
    if (sceSifCallRpc(&s_poff_client, 1, SIF_RPCM_NOWAIT, NULL, 0, NULL, 0, NULL, NULL) < 0) {
        s_cd_init_busy = 0;
        return 0;
    }
    CD_VI(s_cd_init_busy) = 0;
    return 1;
}

// FUNC 0x00270D38 sceCdSearchFile
// Looks a path up in the ISO9660 directory via RPC server 0x80000597.
// Takes the N-command mutex with PollSema (busy -> 0, no debug print),
// keep_cmd = 1, refreshes the caller's thread status, fails (releasing the
// mutex) if an N-command is still in flight, binds the server on first use
// (endless retry).  The name is copied into the send buffer (at most 256
// bytes; if no NUL is among them, [255] is forced to NUL), the buffer's own
// address is stored at +0x124, the buffer is written back and sent (fno 0,
// 0x128 bytes, 4-byte reply).  The 0x24-byte sceCdlFILE the IOP wrote over
// the head of the buffer is copied to *fp through the uncached view.
// Returns the reply word (1 = found), 0 on any failure.  No caller.
int sceCdSearchFile(sceCdlFILE *fp, const char *name)
{
    int i;
    int found;

    _sceCdSemaInit();
    if (NCMD_SEMA_V != PollSema(NCMD_SEMA_V))
        return 0;
    s_ncmd_keep = CD_KEEP_SEARCHFILE;
    ReferThreadStatus(s_cd_thread_id, &s_cd_thread_status);
    if (sceCdSync(1) != 0) {
        SignalSema(NCMD_SEMA_V);
        return 0;
    }

    sceSifInitRpc(0);
    if (s_search_bind < 0) {
        while (1) {
            if (sceSifBindRpc(&s_search_client, CD_SERVER_SEARCHFILE, 0) < 0) {
                if (s_cd_debug > 0)
                    sce_printf_deci2("Libcdvd bind err CdSearchFile\n");
                _sceCdNopDelay();
                continue;
            }
            if (s_search_client.serve != NULL) {
                s_search_bind = 0;
                break;
            }
            _sceCdNopDelay();
        }
    }

    /* copy the name: stop after the NUL or after 256 characters */
    for (i = 0; i < (int)sizeof(s_search_sbuf.name); i++) {
        if ((s_search_sbuf.name[i] = name[i]) == '\0')
            break;
    }
    if (i == (int)sizeof(s_search_sbuf.name))
        s_search_sbuf.name[sizeof(s_search_sbuf.name) - 1] = '\0';
    s_search_sbuf.addr = &s_search_sbuf;

    if (s_cd_debug > 0)
        sce_printf_deci2("ee call cmd search %s\n", s_search_sbuf.name);
    sceSifWriteBackDCache(&s_search_sbuf, sizeof(s_search_sbuf));
    if (sceSifCallRpc(&s_search_client, 0, 0, &s_search_sbuf, sizeof(s_search_sbuf),
                      s_search_rbuf, 4, NULL, NULL) < 0) {
        SignalSema(NCMD_SEMA_V);
        return 0;
    }

    *(_sceCdFileImage *)fp = *(_sceCdFileImage *)UNCACHED(&s_search_sbuf.file);
    if (s_cd_debug > 0)
        sce_printf_deci2("search name %s\n", fp->name);
    if (s_cd_debug > 0)
        sce_printf_deci2("search size %d\n", fp->size);
    if (s_cd_debug > 0)
        sce_printf_deci2("search loc lbn %d\n", fp->lsn);

    found = *(int *)UNCACHED(s_search_rbuf);    /* read before SignalSema */
    SignalSema(NCMD_SEMA_V);
    return found;
}

// FUNC 0x00271038 _sceCdCheckNCmd
// Common prologue of the N-commands: creates the semaphores if needed, takes
// the N-command mutex with PollSema (busy -> debug print, return 0), records
// cur_cmd as keep_cmd, refreshes the caller's thread status, returns 0
// (mutex released) if an N-command is still in flight, and binds RPC server
// 0x80000595 on first use (endless retry).  Returns 1 with the mutex held.
// Caller: sceCdNCmdDiskReady.
static int _sceCdCheckNCmd(int cur_cmd)
{
    _sceCdSemaInit();
    if (NCMD_SEMA_V != PollSema(NCMD_SEMA_V)) {
        if (s_cd_debug > 0)
            sce_printf_deci2("Ncmd fail sema cur_cmd:%d keep_cmd:%d\n", cur_cmd, s_ncmd_keep);
        return 0;
    }
    s_ncmd_keep = cur_cmd;
    ReferThreadStatus(s_cd_thread_id, &s_cd_thread_status);
    if (sceCdSync(1) != 0) {
        SignalSema(NCMD_SEMA_V);
        return 0;
    }

    sceSifInitRpc(0);
    if (s_ncmd_bind >= 0)
        return 1;
    while (1) {
        if (sceSifBindRpc(&s_ncmd_client, CD_SERVER_NCMD, 0) < 0) {
            if (s_cd_debug > 0)
                sce_printf_deci2("Libcdvd bind err N CMD\n");
            _sceCdNopDelay();
            continue;
        }
        if (s_ncmd_client.serve != NULL)
            break;
        _sceCdNopDelay();
    }
    s_ncmd_bind = 0;
    return 1;
}

// FUNC 0x002711A8 sceCdNCmdDiskReady
// N-command DISKREADY (fno 0x0E; keep_cmd 2): no send data, 4-byte reply in
// the N-command receive buffer.  Returns the reply (read uncached before the
// mutex is released) or 0 if the gate or the RPC failed.  No caller.
int sceCdNCmdDiskReady(void)
{
    int ready;

    if (_sceCdCheckNCmd(CD_KEEP_NCMD_DISKREADY) == 0)
        return 0;
    if (sceSifCallRpc(&s_ncmd_client, CD_NCMD_DISKREADY, 0, NULL, 0,
                      s_ncmd_rbuf, 4, NULL, NULL) < 0) {
        SignalSema(s_ncmd_sema);
        return 0;
    }
    ready = *(int *)UNCACHED(s_ncmd_rbuf);
    SignalSema(s_ncmd_sema);
    return ready;
}

// FUNC 0x00271240 sceCdSync
// N-command completion.  mode 0: debug "N cmd wait", then while the callback
// is pending or the N-command RPC is in flight, _sceCdDelay(CD_SYNC_DELAY_HSYNC); returns 0.
// mode != 0: returns 1 if either is still busy, else 0.
int sceCdSync(int mode)
{
    if (mode == 0) {
        if (s_cd_debug > 0)
            sce_printf_deci2("N cmd wait\n");
        while (s_cd_cb_busy != 0 || sceSifCheckStatRpc(&s_ncmd_client.rpcd) != 0)
            _sceCdDelay(CD_SYNC_DELAY_HSYNC);
        return 0;
    }

    if (s_cd_cb_busy != 0 || sceSifCheckStatRpc(&s_ncmd_client.rpcd) != 0)
        return 1;
    return 0;
}

// FUNC 0x002712E0 _sceCdSyncS
// S-command completion.  mode 0: debug "S cmd wait", then _sceCdDelay(0x3C)
// while the S-command RPC is in flight; returns 0.  mode != 0: returns
// sceSifCheckStatRpc's raw value (not normalised to 1, unlike sceCdSync).
static int _sceCdSyncS(int mode)
{
    if (mode == 0) {
        if (s_cd_debug > 0)
            sce_printf_deci2("S cmd wait\n");
        while (sceSifCheckStatRpc(&s_scmd_client.rpcd) != 0)
            _sceCdDelay(CD_SYNC_DELAY_HSYNC);
        return 0;
    }
    return sceSifCheckStatRpc(&s_scmd_client.rpcd);
}

// FUNC 0x00271350 _sceCdCheckSCmd
// Common prologue of the S-commands, same shape as _sceCdCheckNCmd: S-command
// mutex via PollSema (busy -> "Scmd fail sema" debug print, 0), keep_cmd,
// ReferThreadStatus, _sceCdSyncS(1) busy -> release + 0, bind RPC server
// 0x80000593 on first use (endless retry).  Returns 1 with the mutex held.
// Caller: sceCdMmode.
static int _sceCdCheckSCmd(int cur_cmd)
{
    _sceCdSemaInit();
    if (SCMD_SEMA_V != PollSema(SCMD_SEMA_V)) {
        if (s_cd_debug > 0)
            sce_printf_deci2("Scmd fail sema cur_cmd:%d keep_cmd:%d\n", cur_cmd, s_scmd_keep);
        return 0;
    }
    s_scmd_keep = cur_cmd;
    ReferThreadStatus(s_cd_thread_id, &s_cd_thread_status);
    if (_sceCdSyncS(1) != 0) {
        SignalSema(SCMD_SEMA_V);
        return 0;
    }

    sceSifInitRpc(0);
    if (s_scmd_bind >= 0)
        return 1;
    while (1) {
        if (sceSifBindRpc(&s_scmd_client, CD_SERVER_SCMD, 0) < 0) {
            if (s_cd_debug > 0)
                sce_printf_deci2("Libcdvd bind err S cmd\n");
            _sceCdNopDelay();
            continue;
        }
        if (s_scmd_client.serve != NULL)
            break;
        _sceCdNopDelay();
    }
    s_scmd_bind = 0;
    return 1;
}

// FUNC 0x002714C0 sceCdInit
// Returns 0 at once if an S-command is in flight.  sceSifInitRpc(0), records
// the calling thread, marks init busy, resets every bind flag to -1 (all
// servers are re-bound lazily), counts the call, binds the init server
// 0x80000592 (endless retry; bind errors print the error and the call count),
// sends init_mode (4 bytes, fno 0) and reads the 16-byte reply uncached:
//   word[3] == 0xFF -> nothing; == 0xFE -> enable debug output (s_cd_debug=1);
//   else result 2 if word[1]/256 < 2 or word[2]/256 < 2 (? old IOP module
//   versions, < 2.00), signed division.
// RPC failure -> init busy cleared, return 0.  Then init_mode SCECdEXIT (5):
// debug "Libcdvd Exit", _sceCdEeCbExit, semaphore ids reset to -1; any other
// mode: _sceCdSemaInit + _sceCdPOffCbBind (result ignored).  Returns 1 (or
// 2, see above).  One caller outside the unit.
int sceCdInit(int init_mode)
{
    int caller_tid;
    int bind_ret;
    int result;
    int reply_flag, reply_ver1, reply_ver2;
    u32 *reply;

    if (_sceCdSyncS(1) != 0)
        return 0;

    sceSifInitRpc(0);
    caller_tid = GetThreadId();
    s_cd_init_busy   = 1;
    s_cd_thread_id   = caller_tid;
    s_poff_bind      = -1;
    s_search_bind    = -1;
    s_ncmd_bind      = -1;
    s_scmd_bind      = -1;
    s_diskready_bind = -1;
    g_003043F4       = 0;
    s_init_count++;
    s_init_bind      = -1;

    while (1) {
        if ((bind_ret = sceSifBindRpc(&s_init_client, CD_SERVER_INIT, 0)) < 0) {
            if (s_cd_debug > 0)
                sce_printf_deci2("Libcdvd bind err %d CD_Init %d\n", bind_ret, s_init_count);
            _sceCdNopDelay();
            continue;
        }
        if (s_init_client.serve != NULL) {
            s_init_sbuf = init_mode;
            do {
                s_init_bind = 0;    /* a statement macro in the original (see bootstrap) */
            } while (0);
            sceSifWriteBackDCache(&s_init_sbuf, 4);
            reply = s_scmd_rbuf;
            if (sceSifCallRpc(&s_init_client, 0, 0, &s_init_sbuf, 4,
                              reply, 0x10, NULL, NULL) < 0) {
                CD_VI(s_cd_init_busy) = 0;
                return 0;
            }
            break;
        }
        _sceCdNopDelay();
    }

    /* Matching (open, 92.9%): the words read in index order (the 3, 1, 2
     * order gave 90.2%).  Left: the first block's store order and
     * registers (retail stores busy first, ours the thread id) and the
     * sceSifWriteBackDCache delay slot (`reply = s_scmd_rbuf` is scheduled
     * above the call): register allocation and scheduling ties. */
    reply_ver1 = *(int *)UNCACHED(&reply[1]);
    reply_ver2 = *(int *)UNCACHED(&reply[2]);
    reply_flag = *(int *)UNCACHED(&reply[3]);
    result = 1;
    if (reply_flag != CD_INIT_REPLY_QUIET) {
        if (reply_flag == CD_INIT_REPLY_DEBUG)
            s_cd_debug = 1;
        else if (reply_ver1 / 256 < 2 || reply_ver2 / 256 < 2)
            result = 2;
    }
    CD_VI(s_cd_init_busy) = 0;

    switch (init_mode) {
    case SCECdEXIT:
        if (s_cd_debug > 0)
            sce_printf_deci2("Libcdvd Exit\n");
        _sceCdEeCbExit();
        NCMD_SEMA_V  = -1;
        SCMD_SEMA_V  = -1;
        CD_VI(s_cd_cb_sema) = -1;
        return result;
    case SCECdINIT:
    case SCECdINoD:
    default:
        _sceCdSemaInit();
        _sceCdPOffCbBind();
        return result;
    }
}

// FUNC 0x002717A0 sceCdDiskReady
// Drive-ready query through its own RPC server 0x8000059A (not an S-command
// RPC, but it shares the S-command mutex and receive buffer).  Debug
// "DiskReady 0"; semaphores created if needed; mutex busy -> return
// SCECdNotReady (6).  If an S-command is in flight or the RPC fails: mutex
// released, return -1 for mode 8, else SCECdNotReady.  Otherwise binds the
// server on first use (endless retry), sends mode (4 bytes, fno 0), debug
// "DiskReady ended", returns the 4-byte reply (SCECdComplete = 2 when ready;
// read uncached before the mutex is released).  No caller in the core.
int sceCdDiskReady(int mode)
{
    int ready;

    if (s_cd_debug > 0)
        sce_printf_deci2("DiskReady 0\n");
    _sceCdSemaInit();
    if (SCMD_SEMA_V != PollSema(SCMD_SEMA_V))
        return SCECdNotReady;

    if (_sceCdSyncS(1) != 0) {                  /* busy */
        SignalSema(SCMD_SEMA_V);
        return (mode != CD_DISKREADY_MODE_8) ? SCECdNotReady : -1;
    }
    sceSifInitRpc(0);
    if (s_diskready_bind < 0) {
        while (1) {
            if (sceSifBindRpc(&s_diskready_client, CD_SERVER_DISKREADY, 0) < 0) {
                if (s_cd_debug > 0)
                    sce_printf_deci2("Libcdvd bind err CdDiskReady\n");
                _sceCdNopDelay();
                continue;
            }
            if (s_diskready_client.serve != NULL) {
                s_diskready_bind = 0;
                break;
            }
            _sceCdNopDelay();
        }
    }

    s_diskready_sbuf = mode;
    sceSifWriteBackDCache(&s_diskready_sbuf, 4);
    if (sceSifCallRpc(&s_diskready_client, 0, 0, &s_diskready_sbuf, 4,
                      s_scmd_rbuf, 4, NULL, NULL) < 0) {
        SignalSema(SCMD_SEMA_V);                /* RPC error (same exit as busy) */
        return (mode != CD_DISKREADY_MODE_8) ? SCECdNotReady : -1;
    }
    if (s_cd_debug > 0)
        sce_printf_deci2("DiskReady ended\n");
    ready = *(int *)UNCACHED(s_scmd_rbuf);
    SignalSema(SCMD_SEMA_V);
    return ready;
}

// FUNC 0x00271998 sceCdMmode
// S-command 0x22 (media mode): gate _sceCdCheckSCmd(0x22), send the media
// word (SCECdCD / SCECdDVD) from the S-command send buffer, 4-byte reply.
// Returns the reply (read uncached before the mutex is released), 0 if the
// gate or the RPC failed.  One caller outside the unit.
int sceCdMmode(int media)
{
    int result;
    u32 *send = s_scmd_sbuf;

    if (_sceCdCheckSCmd(CD_SCMD_MMODE) == 0)
        return 0;
    *send = media;
    sceSifWriteBackDCache(send, 4);
    if (sceSifCallRpc(&s_scmd_client, CD_SCMD_MMODE, 0, send, 4,
                      s_scmd_rbuf, 4, NULL, NULL) < 0) {
        SignalSema(s_scmd_sema);
        return 0;
    }
    result = *(int *)UNCACHED(s_scmd_rbuf);
    SignalSema(s_scmd_sema);
    return result;
}
