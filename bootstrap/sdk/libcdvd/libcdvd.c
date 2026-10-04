/* libcdvd.c - Sony EE libcdvd client ("PsIIlibcdvd 2240") as linked into the
 * GT3 US bootstrap (BOOTSTRAP_SCUS-97102), .text 0x01012FD8..0x01014410.
 *
 * Only the core of the library is linked: init / exit, the semaphores and
 * the EE callback thread, the power-off notification, the N-command gate
 * (+ one N-command, DISKREADY 0x0E), sceCdSync, sceCdSearchFile, the
 * S-command gate, sceCdDiskReady and sceCdMmode.  sceCdRead & co. are not in
 * the binary (the bootstrap reads the disc through fileio "cdrom0:").  The
 * boot init 0x01000880 (called from main 0x01000538) calls
 * sceCdInit(SCECdINIT) twice (before and after the IOPRP224 reboot),
 * sceCdMmode(SCECdDVD) and sceCdDiskReady(1); 0x010001C8 (also called from
 * main) calls sceCdDiskReady(0).
 *
 * IOP RPC servers (init bound by every sceCdInit, the others lazily; bind
 * retry loops are endless except _sceCdPOffCbBind's, see there):
 *   0x80000592 CD init        client @0x010484E8   (sceCdInit)
 *   0x80000593 S-commands     client @0x0103BE08   (_sceCdCheckSCmd)
 *   0x80000595 N-commands     client @0x0103B590   (_sceCdCheckNCmd)
 *   0x80000596 power-off CB   client @0x010482F8   (_sceCdPOffCbBind)
 *   0x80000597 search file    client @0x010484C0   (sceCdSearchFile)
 *   0x8000059A disk ready     client @0x01048510   (sceCdDiskReady)
 * SIF command 0x80000012 = power-off notification (handler 0x01013520).
 *
 * Debug output: scePrintf (libkernl kernel-TTY printf, 0x0101D2A8) gated by
 * s_cd_debug > 0; s_cd_debug only becomes 1 if the IOP's sceCdInit reply
 * carries the 0xFE marker.
 *
 * Nothing in the binary registers the completion end-functions
 * (0x010130C0 / 0x010132F8) or the callback thread (sceCdInitEeCB is
 * unreferenced), so s_cd_cb_busy is never set: sceCdSync only waits on the
 * N-command RPC status.
 *
 * Reachability (callgraph + address-taken scan): only sceCdInit, sceCdMmode
 * and sceCdDiskReady are called from game code.  sceCdCallback,
 * sceCdInitEeCB, sceCdPOffCallback, sceCdSearchFile and sceCdNCmdDiskReady
 * have no callers and no data/code address references; _sceCdCbDispatch is
 * reached only by the tail jump from the likewise unreferenced
 * _sceCdAlignReadCb.  _sceCdPOffCmdHandler is live (address taken by
 * _sceCdPOffCbBind, which sceCdInit calls).
 */
#include "types.h"
/* libcdvd was built against Sony's eekernel.h, which declares scePrintf as
 * returning void: after each debug print the retail code treats $v0 as free
 * (the register choice of the following s_cd_debug checks shows it).  The
 * shared header declares the function's real int result, so that
 * declaration is renamed away here.  Same function; the result is never
 * used by libcdvd.                                                          */
#define scePrintf _scePrintf_int_decl
#include "eekernel.h"
#undef scePrintf
void scePrintf(const char *fmt, ...);
#include "sif.h"
#include "libcdvd.h"

/* ======================================================================== */
/* Local helpers / types                                                    */
/* ======================================================================== */

/* Linker-provided small-data base (= 0x01049CF0), stored as the callback
 * thread's gp.                                                              */
extern char _gp[];

/* Sony's nopdelay(): the 0x100000-constant busy loop found inlined after
 * every failed bind.  Binary: v0 = 0x100000; loop { v0--; nop x4; }
 * while (v0 != -1) (+ the bne delay-slot nop) = 0x100001 passes.  The loop
 * is empty in the source: the four nops are the old compiler's padding of a
 * loop shorter than seven instructions (R5900 short-loop erratum).  The GCC 14
 * build writes them out, so each pass costs what it did before; the asm also
 * keeps GCC 14 from deleting the loop.  Must stay an inline function (not a
 * macro): that is what gives the retail register allocation.               */
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

/* Completion reason that is not forwarded to the user callback by
 * _sceCdCbDispatch (0x010130C0).  _sceCdPOffCbBind stores the same value at
 * 0x01048320 before enabling the IOP power-off notification.               */
#define CDVD_CBNUM_POFF   0x0B      /* ? name: meaning inferred */

/* sceCdSearchFile send buffer (0x128 bytes @0x01048340, 64-byte aligned).
 * The IOP writes the sceCdlFILE result back over the head of the buffer,
 * using .addr (the EE address of the buffer itself) as the destination.   */
typedef struct {
    sceCdlFILE  file;           /* +0x000 result (written by the IOP)   */
    char        name[256];      /* +0x024 path to search                */
    void       *addr;           /* +0x124 EE address of this buffer     */
} _sceCdSearchSbuf;             /* 0x128 */

/* The 0x24-byte sceCdlFILE as raw bytes: sceCdSearchFile copies the result
 * out with alignment-1 moves (ldl/ldr x4 + lwl/lwr, sdl/sdr x4 + swl/swr), as
 * a byte-aligned block copy compiles; same bytes as a struct assignment.   */
typedef struct {
    u8 b[sizeof(sceCdlFILE)];
} _sceCdFileImage;
_Static_assert(sizeof(_sceCdSearchSbuf) == 0x128, "_sceCdSearchSbuf");

/* Unaligned-read tail data sent by the IOP (cdvdfsv) when a read buffer is
 * not 64-byte aligned: the head/tail bytes that could not be DMAed directly.
 * Only read by the (unreferenced) end-function 0x010132F8.                 */
typedef struct {
    int   size1;                /* +0x00 */
    int   size2;                /* +0x04 */
    u8   *dest1;                /* +0x08 */
    u8   *dest2;                /* +0x0C */
    u8    buf1[0x40];           /* +0x10 */
    u8    buf2[0x40];           /* +0x50 ? size: only the offset is used */
} _sceCdAlignData;

/* ======================================================================== */
/* Globals                                                                  */
/* ======================================================================== */

/* Flags shared with interrupt-context code (SIF command handler, RPC end
 * functions).  Sony's objects declared them plain: the retail code puts their
 * stores in delay slots, where egcs never puts a volatile access, and reuses
 * loaded values.  The GCC 14 build keeps them volatile so that its optimiser
 * cannot drop or move an access the retail code performs.                  */
#ifdef BOOT_OLD_GCC
#define CD_ISR_VOLATILE
#else
#define CD_ISR_VOLATILE volatile
#endif
/* A volatile access to an int, where the retail code shows one (the load is
 * repeated / kept out of delay slots and not reordered).                    */
#define CD_VI(x)  (*(volatile int *)&(x))
/* Posting a completion reason as a statement macro (do { } while (0)): the
 * retail _sceCdEeCbExit keeps the store ahead of the SignalSema argument
 * load (the load fills the call's delay slot), which is what egcs makes of
 * a plain store inside a do-while(0) block - its loop notes are a
 * scheduling barrier.  A plain store outside one is scheduled after the
 * load.  Same single store either way.                                     */
#define CD_SET_CBNUM(n)  do { s_cd_cb_num = (n); } while (0)

/* ---- .data (0x0103A3C0..0x0103BE30) ------------------------------------- */
/* Everything down to s_scmd_client is in the retail .data, zeros included:
 * the old compiler puts a static in .data only when it has an initialiser
 * ("= 0" included) and lays .data out in declaration order, so every one of
 * them is initialised explicitly.  (GCC 14 moves the zero ones to .bss; the
 * values are the same.)                                                     */

/* Library version tag, exactly 16 chars, no NUL (the next word is s_cd_debug). */
static char s_libcdvd_version[16] BOOT_USED = "PsIIlibcdvd 2240"; /* @0x0103A3C0 */

static int  s_cd_debug        = 0;          /* @0x0103A3D0 CdDebug (verbosity; >0 prints)  */
static int  s_cd_cb_thread_id = 0;          /* @0x0103A3D4 EE callback thread id, 0 = none */
static int  s_scmd_keep       = 0;          /* @0x0103A3D8 keep_cmd of the last S-command  */
static int  s_ncmd_keep       = 0;          /* @0x0103A3DC keep_cmd of the last N-command  */
static int  s_cd_cb_sema      = -1;         /* @0x0103A3E0 wakes the callback thread       */
static CD_ISR_VOLATILE int s_cd_init_busy = 0; /* @0x0103A3E4 1 while sceCdInit / POff bind runs;
                                             *   the POff SIF handler ignores notifications */
static int  s_ncmd_sema       = -1;         /* @0x0103A3E8 N-command mutex                 */
static int  s_scmd_sema       = -1;         /* @0x0103A3EC S-command mutex                 */
/* Volatile views of the two mutex ids.  Sony's libcdvd is several objects, and
 * the retail code shows that the gates (_sceCdCheckNCmd / _sceCdCheckSCmd,
 * sceCdSearchFile, sceCdDiskReady, _sceCdCbDispatch) saw them as volatile - the
 * loads are never moved into a delay slot (`lw a0; jal PollSema; nop`) - while
 * the commands and the exit path (sceCdNCmdDiskReady, sceCdMmode,
 * _sceCdEeCbExit) did not.  Same values either way.                        */
#define NCMD_SEMA_V  CD_VI(s_ncmd_sema)
#define SCMD_SEMA_V  CD_VI(s_scmd_sema)
static CD_ISR_VOLATILE int s_cd_cb_busy = 0; /* @0x0103A3F0 completion/callback pending      */
static int  g_0103A3F4        = 0;          /* @0x0103A3F4 only cleared (sceCdInit)         */
static int  s_ncmd_bind       = -1;         /* @0x0103A3F8 <0: N-cmd server not bound       */
static int  s_poff_bind       = -1;         /* @0x0103A3FC <0: POff server not bound        */
static int  s_search_bind     = -1;         /* @0x0103A400 <0: search server not bound      */
static int  s_diskready_bind  = -1;         /* @0x0103A404 <0: disk-ready server not bound  */
static int  s_scmd_bind       = -1;         /* @0x0103A408 <0: S-cmd server not bound       */
static int  s_init_bind       = -1;         /* @0x0103A40C <0: init server not bound        */
static int  s_init_count      = 0;          /* @0x0103A410 number of sceCdInit calls        */
static CD_ISR_VOLATILE int s_cd_cb_num = 0; /* @0x0103A414 CdCallbackNum: last completion
                                             *   reason; -1 = callback-thread exit request   */
static CD_ISR_VOLATILE int s_cd_cb_num_user = 0; /* @0x0103A418 sceCdCbfunc_num (user cb arg)    */

/* N-command receive buffer (only the first word is used here).  The rest of
 * the 0x1150-byte span 0x0103A440..0x0103B590 is zero-filled .data that no
 * linked code references (? presumably the buffers of Sony N-commands such
 * as sceCdRead that were not linked in); s_ncmd_unused keeps that span so
 * the following objects sit at their retail offsets.                        */
static u32  s_ncmd_rbuf[0x10] __attribute__((aligned(64))) = { 0 };   /* @0x0103A440 */
static u8   s_ncmd_unused[0x1150 - 0x40] = { 0 };                     /* @0x0103A480 unreferenced */
static sceSifClientData s_ncmd_client = { 0 };                        /* @0x0103B590 */
/* S-command / init / disk-ready receive buffer (0x440 bytes to 0x0103BA00;
 * sceCdInit uses 16 bytes, the others 4).                                   */
static u32  s_scmd_rbuf[0x110] __attribute__((aligned(64))) = { 0 };  /* @0x0103B5C0 */
/* S-command send buffer: 0x408 bytes up to s_scmd_client (only word 0 used). */
static u32  s_scmd_sbuf[0x102] __attribute__((aligned(64))) = { 0 };  /* @0x0103BA00 */
static sceSifClientData s_scmd_client = { 0 };                        /* @0x0103BE08 */

/* ---- .bss (0x01048280..0x01048560) -------------------------------------- */

static sceCdCBFunc s_cd_user_cb;            /* @0x01048280 sceCdCbfunc (user callback)      */
static void (*s_poff_func)(void *);         /* @0x01048284 power-off callback               */
static void *s_poff_arg;                    /* @0x01048288 its argument                     */
static int  g_0104828C;                     /* @0x0104828C only cleared (callback thread exit) */
static int  s_cd_thread_id;                 /* @0x01048290 thread that called sceCdInit     */
static struct ThreadParam s_cd_thread_status;   /* @0x01048298 ReferThreadStatus scratch    */
static struct ThreadParam s_cd_cb_thread_param; /* @0x010482C8 callback thread CreateThread */
static sceSifClientData s_poff_client;      /* @0x010482F8 */
static int  s_poff_sbuf;                    /* @0x01048320 written 0x0B, never sent         */
static _sceCdSearchSbuf s_search_sbuf __attribute__((aligned(64))); /* @0x01048340 */
static int  s_search_rbuf[0x10] __attribute__((aligned(64)));       /* @0x01048480 (4 used) */
static sceSifClientData s_search_client;    /* @0x010484C0 */
static sceSifClientData s_init_client;      /* @0x010484E8 */
static sceSifClientData s_diskready_client; /* @0x01048510 */
static int  s_init_sbuf __attribute__((aligned(64)));      /* @0x01048540 init_mode    */
static int  s_diskready_sbuf __attribute__((aligned(16))); /* @0x01048550 disk-ready mode */

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

/* ======================================================================== */
/* Functions (address order)                                                */
/* ======================================================================== */

// FUNC 0x01012FD8 _sceCdAlarmHandler
// SetAlarm handler of _sceCdDelay: iSignalSema(semaphore passed as the alarm
// argument), then ExitHandler() (`sync.l; ei`).  $v0 is left = iSignalSema's
// result (handler return value is not used by the kernel).
static void _sceCdAlarmHandler(int alarm_id, u16 time, void *arg)
{
    iSignalSema((int)arg);
    ExitHandler();
}

// FUNC 0x01013000 _sceCdDelay
// Sleeps for `hsync` H-SYNC periods: CreateSema{max 1, init 0}, SetAlarm with
// _sceCdAlarmHandler, WaitSema, DeleteSema.  Only maxCount / initCount /
// option of the SemaParam are written (the rest is stack garbage, as in the
// binary).  No error checks.  Callers: sceCdSync / _sceCdSyncS with 0x3C.
static void _sceCdDelay(u16 hsync)
{
    struct SemaParam param;         /* sp+0x00 */
    int sid;

    param.maxCount  = 1;            /* sp+0x04 */
    param.initCount = 0;            /* sp+0x08 */
    param.option    = 0;            /* sp+0x14 */
    sid = CreateSema(&param);
    SetAlarm(hsync, _sceCdAlarmHandler, (void *)sid);
    WaitSema(sid);
    DeleteSema(sid);
}

// FUNC 0x01013068 sceCdCallback
// Installs the user completion callback and returns the previous one; while
// an N-command is in flight (sceCdSync(1) != 0) it returns NULL without
// changing anything.  The swap is done with interrupts disabled.
// Unreferenced in this binary.
sceCdCBFunc sceCdCallback(sceCdCBFunc func)
{
    sceCdCBFunc old;

    if (sceCdSync(1) != 0)
        return NULL;

    DIntr();
    old = s_cd_user_cb;
    s_cd_user_cb = func;
    EIntr();
    return old;
}

// FUNC 0x010130C0 _sceCdCbDispatch
// Generic N-command completion end-function (interrupt context; no static
// reference in this binary - only reached by the tail jump from 0x010132F8).
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
        iSignalSema(CD_VI(s_cd_cb_sema));   /* thread clears s_cd_cb_busy */
    else
        CD_VI(s_cd_cb_busy) = 0;
    CD_VI(s_cd_cb_num) = 0;
}

// FUNC 0x01013160 _sceCdCallbackThread
// Entry of the EE callback thread created by sceCdInitEeCB.  Waits on
// s_cd_cb_sema; on the exit request (s_cd_cb_num == -1, set by
// _sceCdEeCbExit) clears its state and ExitDeleteThread()s.  Otherwise
// prints the debug line, calls the user callback with the reason (only if
// both callback and reason are non-zero) and clears the busy flag.
// Note the debug print passes the callback *pointer* as the first %d.
static void _sceCdCallbackThread(void *arg)
{
    for (;;) {
        WaitSema(s_cd_cb_sema);
        if (s_cd_cb_num == -1) {
            s_cd_cb_busy = 0;
            s_cd_cb_num = 0;
            s_cd_cb_thread_id = 0;
            g_0104828C = 0;
            ExitDeleteThread();             /* does not return */
        }
        if (s_cd_debug > 0)
            scePrintf("sceCdCbfunc= %d sceCdCbfunc_num= %d\n",
                      (int)s_cd_user_cb, s_cd_cb_num_user);
        if (s_cd_user_cb != NULL && CD_VI(s_cd_cb_num_user) != 0)
            s_cd_user_cb(s_cd_cb_num_user);
        CD_VI(s_cd_cb_busy) = 0;
    }
}

// FUNC 0x01013220 sceCdInitEeCB
// Creates and starts the EE callback thread (entry _sceCdCallbackThread,
// caller-supplied stack, gp = _gp, given priority) and returns 1; if the
// thread already exists only changes its priority and returns 0.  Also
// records the calling thread (GetThreadId / ReferThreadStatus).  Only the
// entry/stack/stackSize/gpReg/initPriority fields of the param block are
// written.  Unreferenced in this binary.
int sceCdInitEeCB(int priority, void *stack, int stack_size)
{
    struct ThreadParam *p;
    int tid;
    int ret = 1;

    if (s_cd_cb_thread_id == 0) {
        tid = GetThreadId();
        s_cd_thread_id = tid;
        ReferThreadStatus(tid, &s_cd_thread_status);

        p = &s_cd_cb_thread_param;
        p->stackSize    = stack_size;           /* +0x0C */
        p->gpReg        = _gp;                  /* +0x10 */
        p->entry        = _sceCdCallbackThread; /* +0x04 */
        p->stack        = stack;                /* +0x08 */
        p->initPriority = priority;             /* +0x14 */
        s_cd_cb_thread_id = CreateThread(p);
        StartThread(s_cd_cb_thread_id, NULL);
    } else {
        ChangeThreadPriority(s_cd_cb_thread_id, priority);
        ret = 0;
    }
    return ret;
}

// FUNC 0x010132F8 _sceCdAlignReadCb
// End-function for reads into buffers that are not 64-byte aligned: through
// the uncached view of the IOP-written _sceCdAlignData, copies size1 bytes
// of buf1 to dest1 and size2 bytes of buf2 to dest2 (byte loops, sizes
// re-read every pass, dest loaded once; no NULL checks), then tail-jumps to
// _sceCdCbDispatch(&s_cd_cb_num).  No static reference in this binary.
static void _sceCdAlignReadCb(void *data)
{
    _sceCdAlignData *p = (_sceCdAlignData *)UNCACHED(data);
    u8 *dst;
    int i;

    if (p->size1 > 0) {
        dst = p->dest1;
        for (i = 0; i < p->size1; i++)
            dst[i] = p->buf1[i];
    }
    if (p->size2 > 0) {
        dst = p->dest2;
        for (i = 0; i < p->size2; i++)
            dst[i] = p->buf2[i];
    }
    _sceCdCbDispatch((int *)&s_cd_cb_num);   /* j 0x010130C0 */
}

// FUNC 0x01013398 _sceCdSemaInit
// Creates the library semaphores unless both the N- and S-command mutexes
// already exist: N-cmd mutex {max 1, init 1}, S-cmd mutex {1, 1}, callback
// wake-up {max 1, init 0}; clears the busy flag.  Same partial SemaParam
// initialisation as _sceCdDelay.  Quirk: if only one of the two mutex ids
// is -1, all three semaphores are created again (the surviving ones leak).
// Callers: sceCdSearchFile, the N/S gates, sceCdInit, sceCdDiskReady.
static void _sceCdSemaInit(void)
{
    struct SemaParam param;         /* sp+0x00 */

    if (s_ncmd_sema != -1 && s_scmd_sema != -1)
        return;

    param.option    = 0;            /* sp+0x14 */
    param.initCount = 1;            /* sp+0x08 */
    param.maxCount  = 1;            /* sp+0x04 */
    NCMD_SEMA_V = CreateSema(&param);    /* volatile stores: see NCMD_SEMA_V */
    SCMD_SEMA_V = CreateSema(&param);
    param.initCount = 0;
    CD_VI(s_cd_cb_sema) = CreateSema(&param);
    CD_VI(s_cd_cb_busy) = 0;
}

// FUNC 0x01013430 _sceCdEeCbExit
// Exit path of sceCdInit(SCECdEXIT): if the callback thread exists, posts the
// exit request (s_cd_cb_num = -1) and signals it; deletes the three
// semaphores; removes the power-off SIF command handler with interrupts
// disabled (EIntr is a tail call).  The semaphore ids are reset to -1 by
// the caller.
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

// FUNC 0x010134B0 sceCdPOffCallback
// Installs the power-off callback (func, addr) and returns the previous
// function.  Binds the power-off server first if that has not been done
// (s_poff_bind < 0; normally done by sceCdInit).  The pair is swapped with
// interrupts disabled, argument stored before the function.
// Unreferenced in this binary, so s_poff_func stays NULL and the power-off
// handler never calls anything.
void *sceCdPOffCallback(void (*func)(void *), void *addr)
{
    void (*old)(void *);

    if (s_poff_bind < 0)
        _sceCdPOffCbBind();

    DIntr();
    old = s_poff_func;
    s_poff_arg = addr;
    s_poff_func = func;
    EIntr();
    return (void *)old;
}

// FUNC 0x01013520 _sceCdPOffCmdHandler
// SIF command handler for 0x80000012 (IOP power-off notification), installed
// by _sceCdPOffCbBind: calls the user power-off callback with its argument,
// unless none is set or sceCdInit / the power-off bind is in progress.
static void _sceCdPOffCmdHandler(void *pkt, void *harg)
{
    if (s_poff_func != NULL && s_cd_init_busy == 0)
        s_poff_func(s_poff_arg);
}

// FUNC 0x01013560 _sceCdPOffCbBind
// sceSifInitRpc(0); marks init busy; installs _sceCdPOffCmdHandler for SIF
// command 0x80000012 (interrupts disabled); if not yet bound, binds RPC
// server 0x80000596 (bind errors: debug print + nopdelay, retried forever;
// bound-but-no-server: nopdelay, retried - the 18th such attempt gives up
// and returns 0; the counter is never reset, bind errors in between do not
// count).  Then stores 0x0B at 0x01048320 (never passed anywhere)
// and sends fno 1 with SIF_RPCM_NOWAIT and no data to enable the IOP-side
// notification.  Returns 1 if that call was queued, else 0; init busy is
// cleared on both paths.  Callers: sceCdInit, sceCdPOffCallback.
static int _sceCdPOffCbBind(void)
{
    int retry;

    sceSifInitRpc(0);
    s_cd_init_busy = 1;
    DIntr();
    sceSifAddCmdHandler(SIF_CMDC_CDVD_POFF, _sceCdPOffCmdHandler, NULL);
    EIntr();

    if (s_poff_bind < 0) {
        retry = 0;
        while (1) {
            if (sceSifBindRpc(&s_poff_client, 0x80000596, 0) < 0) {
                if (s_cd_debug > 0)
                    scePrintf("Libcdvd bind err PowerOffCB\n");
                _sceCdNopDelay();
                continue;
            }
            if (s_poff_client.serve != NULL) {
                s_poff_bind = 0;
                break;
            }
            _sceCdNopDelay();
            if (retry++ >= 17) {
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

// FUNC 0x010136E8 sceCdSearchFile
// Looks a path up in the ISO9660 directory via RPC server 0x80000597.
// Takes the N-command mutex with PollSema (busy -> 0, no debug print),
// keep_cmd = 1, refreshes the caller's thread status, fails (releasing the
// mutex) if an N-command is still in flight, binds the server on first use
// (endless retry).  The name is copied into the send buffer (at most 256
// bytes; if no NUL is among them, [255] is forced to NUL, i.e. names of 256+
// chars are cut to 255), the buffer's own address
// is stored at +0x124, the buffer is written back and sent (fno 0, 0x128
// bytes, 4-byte reply).  The 0x24-byte sceCdlFILE the IOP wrote over the
// head of the buffer is copied to *fp through the uncached view.  Returns
// the reply word (1 = found), 0 on any failure.  Unreferenced in this binary
// (the bootstrap finds files through fileio "cdrom0:", not libcdvd).
int sceCdSearchFile(sceCdlFILE *fp, const char *name)
{
    int i;
    int result;

    _sceCdSemaInit();
    if (NCMD_SEMA_V != PollSema(NCMD_SEMA_V))
        return 0;
    s_ncmd_keep = 1;
    ReferThreadStatus(s_cd_thread_id, &s_cd_thread_status);
    if (sceCdSync(1) != 0) {
        SignalSema(NCMD_SEMA_V);
        return 0;
    }

    sceSifInitRpc(0);
    if (s_search_bind < 0) {
        while (1) {
            if (sceSifBindRpc(&s_search_client, 0x80000597, 0) < 0) {
                if (s_cd_debug > 0)
                    scePrintf("Libcdvd bind err CdSearchFile\n");
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
    for (i = 0; i < 0x100; i++) {
        if ((s_search_sbuf.name[i] = name[i]) == '\0')
            break;
    }
    if (i == 0x100)
        s_search_sbuf.name[0xFF] = '\0';
    s_search_sbuf.addr = &s_search_sbuf;

    if (s_cd_debug > 0)
        scePrintf("ee call cmd search %s\n", s_search_sbuf.name);
    sceSifWriteBackDCache(&s_search_sbuf, 0x128);
    if (sceSifCallRpc(&s_search_client, 0, 0, &s_search_sbuf, 0x128,
                      s_search_rbuf, 4, NULL, NULL) < 0) {
        SignalSema(NCMD_SEMA_V);
        return 0;
    }

    *(_sceCdFileImage *)fp = *(_sceCdFileImage *)UNCACHED(&s_search_sbuf.file);
    if (s_cd_debug > 0)
        scePrintf("search name %s\n", fp->name);
    if (s_cd_debug > 0)
        scePrintf("search size %d\n", fp->size);
    if (s_cd_debug > 0)
        scePrintf("search loc lbn %d\n", fp->lsn);

    result = *(int *)UNCACHED(s_search_rbuf);   /* read before SignalSema */
    SignalSema(NCMD_SEMA_V);
    return result;
}

// FUNC 0x010139E8 _sceCdCheckNCmd
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
            scePrintf("Ncmd fail sema cur_cmd:%d keep_cmd:%d\n", cur_cmd, s_ncmd_keep);
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
        if (sceSifBindRpc(&s_ncmd_client, 0x80000595, 0) < 0) {
            if (s_cd_debug > 0)
                scePrintf("Libcdvd bind err N CMD\n");
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

// FUNC 0x01013B58 sceCdNCmdDiskReady
// N-command DISKREADY (fno 0x0E; keep_cmd 2): no send data, 4-byte reply in
// the N-command receive buffer.  Returns the reply (read uncached before the
// mutex is released) or 0 if the gate or the RPC failed.  Unreferenced.
int sceCdNCmdDiskReady(void)
{
    int result;

    if (_sceCdCheckNCmd(2) == 0)
        return 0;
    if (sceSifCallRpc(&s_ncmd_client, 0x0E, 0, NULL, 0, s_ncmd_rbuf, 4, NULL, NULL) < 0) {
        SignalSema(s_ncmd_sema);
        return 0;
    }
    result = *(int *)UNCACHED(s_ncmd_rbuf);
    SignalSema(s_ncmd_sema);
    return result;
}

// FUNC 0x01013BF0 sceCdSync
// N-command completion.  mode 0: debug "N cmd wait", then while the callback
// is pending or the N-command RPC is in flight, _sceCdDelay(0x3C); returns 0.
// mode != 0: returns 1 if either is still busy, else 0.
int sceCdSync(int mode)
{
    if (mode == 0) {
        if (s_cd_debug > 0)
            scePrintf("N cmd wait\n");
        while (s_cd_cb_busy != 0 || sceSifCheckStatRpc(&s_ncmd_client.rpcd) != 0)
            _sceCdDelay(0x3C);
        return 0;
    }

    if (s_cd_cb_busy != 0 || sceSifCheckStatRpc(&s_ncmd_client.rpcd) != 0)
        return 1;
    return 0;
}

// FUNC 0x01013C90 _sceCdSyncS
// S-command completion.  mode 0: debug "S cmd wait", then _sceCdDelay(0x3C)
// while the S-command RPC is in flight; returns 0.  mode != 0: returns
// sceSifCheckStatRpc's raw value (not normalised to 1, unlike sceCdSync).
static int _sceCdSyncS(int mode)
{
    if (mode == 0) {
        if (s_cd_debug > 0)
            scePrintf("S cmd wait\n");
        while (sceSifCheckStatRpc(&s_scmd_client.rpcd) != 0)
            _sceCdDelay(0x3C);
        return 0;
    }
    return sceSifCheckStatRpc(&s_scmd_client.rpcd);
}

// FUNC 0x01013D00 _sceCdCheckSCmd
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
            scePrintf("Scmd fail sema cur_cmd:%d keep_cmd:%d\n", cur_cmd, s_scmd_keep);
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
        if (sceSifBindRpc(&s_scmd_client, 0x80000593, 0) < 0) {
            if (s_cd_debug > 0)
                scePrintf("Libcdvd bind err S cmd\n");
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

// FUNC 0x01013E70 sceCdInit
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
// 2, see above).  Caller: 0x01000880 with SCECdINIT (twice, around the IOP
// reboot).
// Matching notes: the store `s_init_bind = 0` sits in a do-while(0) block
// (presumably a statement macro in the original): egcs weights REG_N_REFS by
// loop depth and counts such a block as a loop, and only with that weight
// does the hi part of &s_init_bind outrank the one of &s_init_count for the
// callee-saved registers ($s4 / $s5 as in the binary).  The two `busy = 0`
// stores are volatile accesses (never moved into a delay slot); the one in
// the bind loop then keeps its own hi part, so &s_cd_init_busy loses its
// callee-saved register to &s_scmd_rbuf ($fp), again as in the binary.
// Same stores, same order.
int sceCdInit(int init_mode)
{
    int tid;
    int ret;
    int result;
    int flag, ver1, ver2;
    u32 *rbuf;

    if (_sceCdSyncS(1) != 0)
        return 0;

    sceSifInitRpc(0);
    tid = GetThreadId();
    s_cd_init_busy   = 1;
    s_cd_thread_id   = tid;
    s_poff_bind      = -1;
    s_search_bind    = -1;
    s_ncmd_bind      = -1;
    s_scmd_bind      = -1;
    s_diskready_bind = -1;
    g_0103A3F4       = 0;
    s_init_count++;
    s_init_bind      = -1;

    while (1) {
        if ((ret = sceSifBindRpc(&s_init_client, 0x80000592, 0)) < 0) {
            if (s_cd_debug > 0)
                scePrintf("Libcdvd bind err %d CD_Init %d\n", ret, s_init_count);
            _sceCdNopDelay();
            continue;
        }
        if (s_init_client.serve != NULL) {
            s_init_sbuf = init_mode;
            do {
                s_init_bind = 0;    /* statement block: see the note above */
            } while (0);
            sceSifWriteBackDCache(&s_init_sbuf, 4);
            rbuf = s_scmd_rbuf;
            if (sceSifCallRpc(&s_init_client, 0, 0, &s_init_sbuf, 4,
                              rbuf, 0x10, NULL, NULL) < 0) {
                CD_VI(s_cd_init_busy) = 0;
                return 0;
            }
            break;
        }
        _sceCdNopDelay();
    }

    flag = *(int *)UNCACHED(&rbuf[3]);
    ver1 = *(int *)UNCACHED(&rbuf[1]);
    ver2 = *(int *)UNCACHED(&rbuf[2]);
    result = 1;
    if (flag != 0xFF) {
        if (flag == 0xFE)
            s_cd_debug = 1;
        else if (ver1 / 256 < 2 || ver2 / 256 < 2)
            result = 2;
    }
    CD_VI(s_cd_init_busy) = 0;

    /* compiled as `mode < 0 || mode < 2 || mode != 5` -> default path:
     * a switch whose cases 0/1 share the default block */
    switch (init_mode) {
    case SCECdEXIT:
        if (s_cd_debug > 0)
            scePrintf("Libcdvd Exit\n");
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

// FUNC 0x01014150 sceCdDiskReady
// Drive-ready query through its own RPC server 0x8000059A (not an
// S-command RPC, but it shares the S-command mutex and receive buffer).
// Debug "DiskReady 0"; semaphores created if needed; mutex busy -> return
// SCECdNotReady (6).  If an S-command is in flight or the RPC fails: mutex
// released, return -1 for mode 8, else SCECdNotReady.  Otherwise binds the
// server on first use (endless retry), sends mode (4 bytes, fno 0), debug
// "DiskReady ended", returns the 4-byte reply (SCECdComplete = 2 when ready;
// read uncached before the mutex is released).
// Callers: 0x01000880 (init, mode 1), 0x010001C8 (mode 0).
int sceCdDiskReady(int mode)
{
    int result;

    if (s_cd_debug > 0)
        scePrintf("DiskReady 0\n");
    _sceCdSemaInit();
    if (SCMD_SEMA_V != PollSema(SCMD_SEMA_V))
        return SCECdNotReady;

    if (_sceCdSyncS(1) != 0) {                  /* busy */
        SignalSema(SCMD_SEMA_V);
        return (mode != 8) ? SCECdNotReady : -1;
    }
    sceSifInitRpc(0);
    if (s_diskready_bind < 0) {
        while (1) {
            if (sceSifBindRpc(&s_diskready_client, 0x8000059A, 0) < 0) {
                if (s_cd_debug > 0)
                    scePrintf("Libcdvd bind err CdDiskReady\n");
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
        return (mode != 8) ? SCECdNotReady : -1;
    }
    if (s_cd_debug > 0)
        scePrintf("DiskReady ended\n");
    result = *(int *)UNCACHED(s_scmd_rbuf);
    SignalSema(SCMD_SEMA_V);
    return result;
}

// FUNC 0x01014348 sceCdMmode
// S-command 0x22 (media mode): gate _sceCdCheckSCmd(0x22), send the media
// word (SCECdCD / SCECdDVD) from the S-command send buffer, 4-byte reply.
// Returns the reply (read uncached before the mutex is released), 0 if the
// gate or the RPC failed.  Caller: 0x01000880 with SCECdDVD.
int sceCdMmode(int media)
{
    int result;
    u32 *sbuf = s_scmd_sbuf;

    if (_sceCdCheckSCmd(0x22) == 0)
        return 0;
    *sbuf = media;
    sceSifWriteBackDCache(sbuf, 4);
    if (sceSifCallRpc(&s_scmd_client, 0x22, 0, sbuf, 4, s_scmd_rbuf, 4, NULL, NULL) < 0) {
        SignalSema(s_scmd_sema);
        return 0;
    }
    result = *(int *)UNCACHED(s_scmd_rbuf);
    SignalSema(s_scmd_sema);
    return result;
}
