/* thread_util.cpp - semaphore / thread helpers of the bootstrap's I/O layer
 * (unit "io").  Functions at 0x0100F5D0..0x0100F6D0 and 0x0100F8D8..0x0100F918.
 *
 * Link order in .text: mcard_io (0x0100F130..) | strlen (0x0100F538, 128-bit
 * EE version, libc unit) | create_mutex_sema, yield_low_priority, signal_sema_helper |
 * hostfs_io (0x0100F6D0..) | busy_wait_delay.
 */
#include "types.h"
#include "eekernel.h"
#include "boot_io.h"

// FUNC 0x0100F5D0 create_mutex_sema
// Creates a binary semaphore (maxCount 1) with the given initial count.
// Only maxCount, initCount and option are written; currentCount,
// numWaitThreads and attr (+0x00/+0x0C/+0x10) are left as stack garbage,
// exactly as in the binary.  CreateSema == -1 (no free semaphore) -> yield and
// retry forever.  Quirk: an id of 0 is rejected by recursing once more,
// which leaks semaphore 0.  Callers: mc_init_once (0x0100F340),
// global_lock_acquire (0x01012F68, allocator unit).
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
        return create_mutex_sema(initCount);   /* real recursive jal at 0x0100F620 */
    return sid;
}

// FUNC 0x0100F640 yield_low_priority
// Cooperative yield used by every polling loop of the boot loader (MC
// wrappers, CreateSema retry, boot code at 0x010001C8): lower the calling
// thread to priority 0x7E, then 100 times spin busy_wait_delay(10) and rotate
// the 0x7E ready queue so lower/equal-priority threads (SIF/IOP callbacks)
// get CPU time, then restore the priority read by ReferThreadStatus
// (ThreadParam.currentPriority, +0x18).  Return values are ignored.
void yield_low_priority(void)
{
    struct ThreadParam status;    /* sp+0x00, 0x30 bytes */
    int tid;
    int i;

    tid = GetThreadId();
    ReferThreadStatus(tid, &status);
    ChangeThreadPriority(tid, 0x7E);

    /* binary counts s1 = 99 down to 0 (bgez after decrement): 100 passes */
    for (i = 99; i >= 0; i--) {
        busy_wait_delay(10);
        RotateThreadReadyQueue(0x7E);
    }

    ChangeThreadPriority(tid, status.currentPriority);
}

// FUNC 0x0100F6B0 signal_sema_helper
// Shared unlock tail of the MC lock stubs (0x0100F198/288/338/3A0/440/4D0)
// and of the pdistd global lock release (0x01012FA0); only reached by `j`.
// Quirks: it reads COP0 Status into $v0 and never uses it (vestigial - maybe a
// removed "SignalSema vs iSignalSema" selection), and it calls iSignalSema
// (syscall -0x43, stub 0x0101BC10) unconditionally even though every caller
// runs in thread context.  The kernel still signals the semaphore/releases a
// waiter but does not reschedule immediately.  Returns iSignalSema's result.
int signal_sema_helper(int semaId)
{
    (void)_ee_get_status();       /* mfc0 $v0, $12 - result discarded */
    return iSignalSema(semaId);
}

// FUNC 0x0100F8D8 busy_wait_delay
// Calibrated spin: count = n * 75 computed with 64-bit ops (dsll/daddu/dsll/
// dsubu = ((n*5) << 4) - n*5), then a loop of six nops closed by
// `bgez count, loop` with `daddiu count, count, -1` in the delay slot, i.e.
// 75*n + 2 passes (count = 75n .. 0 taken, one final pass at -1).  n < 0
// still runs one pass.  The binary copies the count into $at (`move $at,$v0`)
// and loops on $at - GCC never allocates $at itself, so the original was an
// inline asm block that moves its 64-bit input into $at; the shift/add
// multiply is egcs's synthesis of a 64-bit `n * 75`, which needs a 64-bit
// parameter (an int one would add a sign-extending move): `long` in ee-gcc,
// written s64 here so that both compilers use 64 bits.  Inline asm is kept
// because the pass count / instruction timing is the whole behaviour.
// Only caller: yield_low_priority (n = 10 -> 752 passes).
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
