/* pdistd_misc.c - the start of the no-FDE stretch G003,
 * .text 0x0026E310..0x0026E3E0: a CreateThread-with-retry helper, then the
 * small pdistd helpers that are the same machine code as the bootstrap's
 * src/pdistd/pdistd_misc.cpp (0x01012F68..0x01012FD8).  The bootstrap's
 * object starts with intr_suspend; here thread_create_retry takes that
 * place (intr_suspend is in u124).
 *
 * .sdata of this object: s_globalLockSema (0x003531D8, the lock semaphore
 * id, = -1), the "%s" literal at 0x003531E0; the .sdata words pointing at
 * the break stubs are 0x003531C0 / 0x003531C4 (no code reads them; another
 * object's data: u125's T532_heap.cpp defines them, g_break_hook_0 / _1).
 */
#include "engine/pdi/G003_pdi1394_rpc.h"
#include "engine/pdi/pdi1394_rpc_internal.h"

/* Semaphore id of the global lock; -1 until the first global_lock_acquire.
 * The object's .sdata: this word, then the "%s" literal (8-aligned) of
 * printf_to_deci2. */
static int s_globalLockSema = -1;               /* 0x003531D8 */

// FUNC 0x0026E310 thread_create_retry
// Fills a ThreadParam on the stack (entry +0x04, stack +0x08, stackSize
// +0x0C, gpReg = &_gp +0x10, initPriority +0x14, option +0x20 = 0; status
// and attr are left as the stack has them) and calls CreateThread until it
// does not return -1, yielding (yield_low_priority) between tries.  Returns
// the thread id.  Only caller: thread_start_retry (0x00269C30), which then
// starts the thread the same way.
int thread_create_retry(void (*entry)(void *), int priority, void *stack,
                        int stackSize)
{
    struct ThreadParam param;
    int tid;

    param.entry = entry;
    param.stack = stack;
    param.stackSize = stackSize;
    param.initPriority = priority;
    param.gpReg = _gp;
    param.option = 0;
    while ((tid = CreateThread(&param)) == -1)
        yield_low_priority();
    return tid;
}

// FUNC 0x0026E370 global_lock_acquire
// (bootstrap 0x01012F68.)  If the lock semaphore does not exist yet
// (id == -1), creates it with create_mutex_sema(1) (binary semaphore,
// initially available) and stores the id; then WaitSema(id), whose result
// is returned (jal, not a tail jump).  The lazy creation is not protected
// against a concurrent first use.  Callers: the "host:" fileio wrappers
// host_open, host_lseek, host_read, host_write, host_close (u124), which
// ignore the result.
int global_lock_acquire(void)
{
    int sema = s_globalLockSema;

    if (sema == -1) {
        sema = create_mutex_sema(1);
        s_globalLockSema = sema;
    }
    return WaitSema(sema);
}

// FUNC 0x0026E3A8 global_lock_release
// (bootstrap 0x01012FA0.)  Tail jump to signal_sema_helper(s_globalLockSema);
// its return value passes through in $v0 but no caller uses it.
void global_lock_release(void)
{
    signal_sema_helper(s_globalLockSema);
}

// FUNC 0x0026E3B0 break_stub_0
// (bootstrap 0x01012FA8.)  `break` (code 0) trap, then return 0.  Only
// reference: the .sdata pointer at 0x003531C0 (itself unreferenced by code).
int break_stub_0(void)
{
    __asm__ __volatile__("break");
    return 0;
}

// FUNC 0x0026E3C0 break_stub_1
// (bootstrap 0x01012FB8.)  `break` (code 0) trap, then return 0; the
// argument is ignored.  abort() (0x0027D7A0) calls it with 6 (SIGABRT), so
// it stands in for newlib's raise(); also pointed to by .sdata 0x003531C4.
int break_stub_1(int code)
{
    __asm__ __volatile__("break");
    return 0;
}

// FUNC 0x0026E3D0 printf_to_deci2
// (bootstrap 0x01012FC8 tty_puts.)  PrintTarget_flush's backend
// (0x0026C5A8): tail jump to the kernel TTY printf, sce_printf_deci2("%s", s).
void printf_to_deci2(const char *s)
{
    sce_printf_deci2("%s", s);
}
