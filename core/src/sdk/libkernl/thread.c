/* thread.c - Sony libkernl 2240 thread.o, core
 * 0x002869E8..0x00286D47: the priority-0 "top" server thread that performs
 * WakeupThread / RotateThreadReadyQueue / SuspendThread on behalf of
 * interrupt handlers (the kernel's i-forms misbehave on the current thread;
 * ps2sdk kernel.h: "the defect within iWakeupThread() that prevents the
 * running thread from being woken up").
 * Same machine code as decomp/bootstrap/sdk/libkernl/kernel.c (thread.o part).
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part1_internal.h"

/* thread.o's data (names as in ps2sdk's _thread_internals.o: topArg, topId,
 * topSema; its `stack' is topStack here).  File statics: the .bss block sits
 * between frame.o's and tty.o's statics in the original (C commons would be
 * after every object's .bss), in this order; topId is initialised data. */
static u8              topStack[0x400] __attribute__((aligned(16))); /* 0x004AD260 */
static int             topSema;         /* 0x004AD660 */
static struct _top_arg topArg;          /* 0x004AD668 (requestIn @0x004AD66C) */
static int             topId = 0;       /* 0x003081B8 */

/* Inline interrupt-context GetThreadId (`addiu $v1,$0,-0x2F; syscall`), used
 * by iWakeupThread / iSuspendThread (ps2sdk: _iGetThreadId, "actually
 * GetThreadId(), used for a hack by SCE").  $a0 survives the syscall.     */
static __inline__ int _iGetThreadId(void)
{
    register int v0 __asm__("$2");

    __asm__ __volatile__("addiu\t$3, $0, -0x2F\n\tsyscall" : "=r"(v0) : : "$3", "memory");
    return v0;
}

// FUNC 0x002869E8 topThread
//   Priority-0 server thread (started by InitThread with arg = &topArg):
//   forever WaitSema(topSema), pop one {mode,data} request and perform it in
//   thread context: TOP_REQ_WAKEUP WakeupThread, TOP_REQ_ROTATE
//   RotateThreadReadyQueue, TOP_REQ_SUSPEND SuspendThread, anything else
//   prints "## internel error in libkernl.a!\n" (sic) through the SIO
//   printf.  Never returns.
static void topThread(void *arg)
{
    struct _top_arg *top = (struct _top_arg *)arg;
    unsigned int slot;

    for (;;) {
        WaitSema(topSema);
        slot = top->requestOut & TOP_QUEUE_MASK;
        top->requestOut = slot + 1;
        switch (top->request[slot].mode) {
        case TOP_REQ_WAKEUP:
            WakeupThread(top->request[slot].data);
            break;
        case TOP_REQ_ROTATE:
            RotateThreadReadyQueue(top->request[slot].data);
            break;
        case TOP_REQ_SUSPEND:
            SuspendThread(top->request[slot].data);
            break;
        default:
            sce_printf_sio("## internel error in libkernl.a!\n");
            break;
        }
    }
}

// FUNC 0x00286AC0 InitThread
//   Thread-server init (_InitSys tail-jumps here).  Returns -1 if topId > 0
//   already, if CreateSema fails, or if CreateThread fails (then the sema is
//   deleted; topId keeps the negative id).  Otherwise creates sema {max 0xFF,
//   init 0}, thread {topThread, topStack, 0x400, gp, prio 0}, clears the
//   ring, StartThread(topId, &topArg), drops the caller to priority 1 and
//   returns topId.  Only +0x04/+0x08 of the SemaParam and +0x04..+0x14 of the
//   ThreadParam are written (the rest is stack garbage).
int InitThread(void)
{
    struct ThreadParam thread_param;    /* sp+0x00 */
    struct SemaParam   sema_param;      /* sp+0x30 */

    if (topId > 0)
        return -1;

    sema_param.maxCount = 0xFF;
    sema_param.initCount = 0;
    topSema = CreateSema(&sema_param);
    if (topSema < 0)
        return -1;

    thread_param.entry = topThread;
    thread_param.stack = topStack;
    thread_param.stackSize = sizeof(topStack);      /* 0x400 */
    thread_param.gpReg = _gp;
    thread_param.initPriority = 0;
    topId = CreateThread(&thread_param);
    if (topId < 0) {
        DeleteSema(topSema);
        return -1;
    }

    topArg.requestOut = 0;
    topArg.requestIn = 0;
    StartThread(topId, &topArg);
    ChangeThreadPriority(GetThreadId(), 1);
    return topId;
}

// FUNC 0x00286B98 iWakeupThread
//   Interrupt-context wakeup with the kernel-bug workaround: if tid is not
//   the current thread, plain _iWakeupThread(tid) (#-0x34) and its result.
//   If it IS the current thread: fail (-1) when tid >= MAX_THREADS (unsigned)
//   or the server was never created (topId == 0); else queue {WAKEUP, tid}
//   for topThread, iSignalSema(topSema) and return tid.  8 callers outside
//   the unit (GS driver VBlank/DMAC handlers, libmc alarm) plus
//   _request_call's tail call.
int iWakeupThread(int tid)
{
    int self = _iGetThreadId();
    unsigned int slot;

    if (self != tid)
        return _iWakeupThread(tid);

    if (!((unsigned int)self < MAX_THREADS) || topId == 0)
        return -1;

    slot = topArg.requestIn & TOP_QUEUE_MASK;
    topArg.requestIn = slot + 1;
    topArg.request[slot].mode = TOP_REQ_WAKEUP;
    topArg.request[slot].data = (u8)self;
    iSignalSema(topSema);
    return self;
}

// FUNC 0x00286C30 iRotateThreadReadyQueue
//   Always goes through the server: fails (-1) if prio >= MAX_PRIORITY
//   (unsigned) or topId == 0, else queues {ROTATE, prio}, iSignalSema(topSema),
//   returns prio.  _iRotateThreadReadyQueue (#-0x2C) is never used.  No
//   callers.
int iRotateThreadReadyQueue(int prio)
{
    unsigned int slot;

    if (!((unsigned int)prio < MAX_PRIORITY) || topId == 0)
        return -1;

    slot = topArg.requestIn & TOP_QUEUE_MASK;
    topArg.requestIn = slot + 1;
    topArg.request[slot].mode = TOP_REQ_ROTATE;
    topArg.request[slot].data = (u8)prio;
    iSignalSema(topSema);
    return prio;
}

// FUNC 0x00286CB0 iSuspendThread
//   Same shape as iWakeupThread with _iSuspendThread (#-0x38) and
//   TOP_REQ_SUSPEND.  No callers.
int iSuspendThread(int tid)
{
    int self = _iGetThreadId();
    unsigned int slot;

    if (self != tid)
        return _iSuspendThread(tid);

    if (!((unsigned int)self < MAX_THREADS) || topId == 0)
        return -1;

    slot = topArg.requestIn & TOP_QUEUE_MASK;
    topArg.requestIn = slot + 1;
    topArg.request[slot].mode = TOP_REQ_SUSPEND;
    topArg.request[slot].data = (u8)self;
    iSignalSema(topSema);
    return self;
}
