/* thread_util.c - interrupt / semaphore / thread helpers
 * (0x00269B00..0x00269D80, and busy_wait_delay at 0x00269F88 after the
 * hostfs wrappers, as in the bootstrap's link order).
 *
 * intr_suspend, create_mutex_sema, yield_low_priority, signal_sema_helper and
 * busy_wait_delay are the same machine code as the bootstrap's
 * (src/pdistd/pdistd_misc.cpp, src/thread_util.cpp): their source is reused.
 * The worker-thread helpers (thread_start_retry .. worker_thread_exit) are
 * new in the main executable.
 */
#define G002_WORKER_JOIN_INT            /* worker_thread_join returns int here (see the header) */
#include "engine/media/G002_mpeg_frameq.h"
#include "engine/system/system_util_internal.h"

// FUNC 0x00269B00 intr_suspend
// Polyphony's own DIntr: reads COP0 Status, then the R5900 di-errata loop
// (`di; sync.p; mfc0 Status` until Status.EIE (bit 16) reads back clear),
// and returns the EIE bit of the Status value read BEFORE disabling
// (1 = interrupts were enabled).  Unlike libkernl DIntr it executes `di`
// unconditionally.  Callers pair it with `if (r) ei` (116 callers here:
// the allocator, the sound code, GameObjectController, ...).
// Bootstrap 0x01012F38: the retail function is hand-scheduled assembly
// (status >> 16 computed inside the loop, empty delay slot, $a1/$a0), so the
// egcs build assembles the retail instructions verbatim; the modern build
// keeps the equivalent C.
//   mfc0 $a1,$12; lui $a0,1; 1: di; sync.p; mfc0 $v0,$12; srl $v1,$a1,16;
//   and $v0,$v0,$a0; bnez $v0,1b; nop; jr $ra; andi $v0,$v1,1
#ifdef BOOT_OLD_GCC
__asm__("\t.text\n"
        "\t.p2align 3\n"
        "\t.globl\tintr_suspend\n"
        "\t.ent\tintr_suspend\n"
        "\t.type\tintr_suspend,@function\n"
        "intr_suspend:\n"
        "\t.set\tnoreorder\n"
        "\tmfc0\t$5, $12\n"
        "\tlui\t$4, 1\n"
        "1:\tdi\n"
        "\tsync.p\n"
        "\tmfc0\t$2, $12\n"
        "\tsrl\t$3, $5, 16\n"
        "\tand\t$2, $2, $4\n"
        "\tbne\t$2, $0, 1b\n"
        "\tnop\n"
        "\tjr\t$31\n"
        "\tandi\t$2, $3, 1\n"
        "\t.set\treorder\n"
        "\t.end\tintr_suspend\n"
        "\t.size\tintr_suspend, . - intr_suspend\n"
        "\t.previous\n");
#else
int intr_suspend(void)
{
    u32 status = _ee_get_status();      /* mfc0 $a1, $12 */

    DI();                               /* 1: di; sync.p; mfc0; and 0x10000; bnez 1b */
    return (status >> 16) & 1;
}
#endif

// FUNC 0x00269B30 create_mutex_sema
// Creates a binary semaphore (maxCount 1) with the given initial count.
// Only maxCount, initCount and option are written; currentCount,
// numWaitThreads and attr (+0x00/+0x0C/+0x10) are left as stack garbage,
// exactly as in the binary.  CreateSema == -1 (no free semaphore) -> yield and
// retry forever.  Quirk: an id of 0 is rejected by recursing once more,
// which leaks semaphore 0.  16 callers (global_lock_acquire, vol_init,
// worker_thread_start, ...).
int create_mutex_sema(int initCount)
{
    struct SemaParam param;       /* sp+0x00, 0x18 bytes */
    int sid;

    param.maxCount  = 1;          /* sp+0x04 */
    param.initCount = initCount;  /* sp+0x08 */
    param.option    = 0;          /* sp+0x14 */

    while ((sid = CreateSema(&param)) == -1)
        yield_low_priority();

    if (sid == 0)
        return create_mutex_sema(initCount);   /* real recursive jal at 0x00269B80 */
    return sid;
}

// FUNC 0x00269BA0 yield_low_priority
// Cooperative yield used by every polling loop (MC wrappers, CreateSema /
// CreateThread / StartThread retries): lower the calling thread to priority
// 0x7E, then 100 times spin busy_wait_delay(10) and rotate the 0x7E ready
// queue so lower/equal-priority threads (SIF/IOP callbacks) get CPU time,
// then restore the priority read by ReferThreadStatus
// (ThreadParam.currentPriority, +0x18).  Return values are ignored.
void yield_low_priority(void)
{
    struct ThreadParam status;    /* sp+0x00, 0x30 bytes */
    int tid;
    int pass;

    tid = GetThreadId();
    ReferThreadStatus(tid, &status);
    ChangeThreadPriority(tid, YIELD_PRIORITY);

    /* binary counts s1 = 99 down to 0 (bgez after decrement): 100 passes */
    for (pass = YIELD_PASSES - 1; pass >= 0; pass--) {
        busy_wait_delay(YIELD_SPIN);
        RotateThreadReadyQueue(YIELD_PRIORITY);
    }

    ChangeThreadPriority(tid, status.currentPriority);
}

// FUNC 0x00269C10 signal_sema_helper
// Shared unlock helper (43 callers, also global_lock_release).
// Quirks: it reads COP0 Status into $v0 and never uses it (vestigial - maybe a
// removed "SignalSema vs iSignalSema" selection), and it calls iSignalSema
// (syscall -0x43) unconditionally even though callers run in thread context.
// The kernel still signals the semaphore/releases a waiter but does not
// reschedule immediately.  Returns iSignalSema's result.
int signal_sema_helper(int semaId)
{
    (void)_ee_get_status();       /* mfc0 $v0, $12 - result discarded */
    return iSignalSema(semaId);
}

// FUNC 0x00269C30 thread_start_retry
/* Create a thread (thread_create_retry retries CreateThread) and start it
 * with arg, yielding and retrying while StartThread does not return the
 * thread id.  Returns the thread id.  Callers
 * MemorycardFileManager::createThread, F_0022D870 (T473), padStart,
 * worker_thread_start. */
int thread_start_retry(void (*entry)(void *), void *arg, int priority, void *stack, int stackSize)
{
    int tid = thread_create_retry(entry, priority, stack, stackSize);

    while (StartThread(tid, arg) != tid)
        yield_low_priority();
    return tid;
}

// FUNC 0x00269C90 worker_thread_start
/* sema = a fresh binary semaphore at count 0 (created first), then the
 * thread.  Callers logger_record_start, music_init, mc_manager_init,
 * vol_init. */
void worker_thread_start(ThreadCtx *w, void (*entry)(void *), void *arg, int priority,
                         void *stack, int stackSize)
{
    w->sema = create_mutex_sema(0);
    w->thread = thread_start_retry(entry, arg, priority, stack, stackSize);
}

// FUNC 0x00269D18 worker_thread_join
/* Wait for the worker's completion signal, then delete the semaphore and
 * return DeleteSema's result (retail keeps the jal and the frame: the value
 * is returned, not a tail jump; no caller reads it). */
int worker_thread_join(ThreadCtx *w)
{
    WaitSema(w->sema);
    return DeleteSema(w->sema);
}

// FUNC 0x00269D48 worker_thread_exit
/* Called by the worker itself at the end (music_thread, ...): raise its
 * priority to 1 (so it is not preempted between signalling and exiting),
 * signal sema and ExitDeleteThread (tail jump, does not return). */
void worker_thread_exit(ThreadCtx *w)
{
    ChangeThreadPriority(GetThreadId(), WORKER_EXIT_PRIORITY);
    signal_sema_helper(w->sema);
    ExitDeleteThread();
}

// FUNC 0x00269F88 busy_wait_delay
// Calibrated spin: count = n * 75 computed with 64-bit ops (dsll/daddu/dsll/
// dsubu = ((n*5) << 4) - n*5), then a loop of six nops closed by
// `bgez count, loop` with `daddiu count, count, -1` in the delay slot, i.e.
// 75*n + 2 passes (count = 75n .. 0 taken, one final pass at -1).  n < 0
// still runs one pass.  The binary copies the count into $at (`move $at,$v0`)
// and loops on $at - GCC never allocates $at itself, so the original was an
// inline asm block that moves its 64-bit input into $at; the shift/add
// multiply is egcs's synthesis of a 64-bit `n * 75` (a 64-bit parameter:
// `long` in ee-gcc, written s64 here so that both compilers use 64 bits).
// Inline asm is kept because the pass count / instruction timing is the whole
// behaviour.  Only caller: yield_low_priority (n = 10 -> 752 passes).
void busy_wait_delay(s64 n)
{
    s64 count = n * 75;

    __asm__ __volatile__(
        ".set push\n\t"
        ".set noreorder\n\t"
        ".set noat\n\t"
        "move   $1, %0\n"
        "1:\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "bgez   $1, 1b\n\t"
        "daddiu $1, $1, -1\n\t"   /* delay slot: executes on every pass */
        ".set pop"
        : : "r"(count) : "$1");
}
