/* pdistd_misc.cpp - small pdistd helpers of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), units "allocator" and "printf",
 * .text 0x01012F38..0x01012FD8.
 *
 * These sit after GZIP inflate and, unlike the surrounding Polyphony C++
 * code, have no .eh_frame / metadata record: one object compiled without
 * exception support (plain C, or C++ with -fno-exceptions), which is also
 * why egcs turned the calls whose result is unused into tail jumps
 * (global_lock_release, tty_puts).  All are C linkage here; the matching
 * build compiles this file with -fno-exceptions (ref/compile_flags.json).
 *
 *   intr_suspend          interrupt disable used by every pdistd::Allocator
 *                         entry point and heap_init_once
 *   global_lock_acquire / global_lock_release
 *                         lazily created binary semaphore serialising the
 *                         "host:" fileio wrappers (hostfs_io unit)
 *   break_stub_0 / break_stub_1
 *                         `break; return 0` traps, referenced from .sdata
 *                         pointers; break_stub_1 is abort()'s raise(SIGABRT)
 *   tty_puts              printf.cxx's PrintTarget flush backend
 *                         (printf unit, declared in pdistd/printf.h)
 *
 * .sdata of this object: s_globalLockSema @0x01041E60, then the "%s"
 * literal @0x01041E68.
 */
#include "types.h"
#include "eekernel.h"            /* DI(), _ee_get_status(), WaitSema, scePrintf */
#include "libc.h"                /* break_stub_1 prototype */
#include "boot_io.h"             /* create_mutex_sema, signal_sema_helper */
#include "pdistd/allocator.h"
#include "pdistd/printf.h"       /* tty_puts */

extern "C" {

/* Semaphore id of the global lock; -1 until the first global_lock_acquire. */
static int s_globalLockSema = -1;               /* @0x01041E60 (.sdata) */

// FUNC 0x01012F38 intr_suspend
// Polyphony's own DIntr: reads COP0 Status, then the R5900 di-errata loop
// (`di; sync.p; mfc0 Status` until Status.EIE (bit 16) reads back clear),
// and returns the EIE bit of the Status value read BEFORE disabling
// (1 = interrupts were enabled).  Unlike libkernl DIntr it executes `di`
// unconditionally.  Callers pair it with `if (r) ei` (Allocator::allocate,
// reallocate, free, getMaxFreeSize, getTotalFreeSize, debug, countFreeReps,
// heap_init_once).  Inline asm: mfc0 / di / sync.p have no C equivalent.
// The retail function is hand-scheduled assembly: status >> 16 is computed
// inside the loop, in the slot after the second mfc0, the branch delay slot
// stays empty although that srl could fill it, and it uses $a1/$a0 where egcs
// would pick $v0/$v1 (and egcs would add loop alignment and an R5900
// short-loop nop).  The egcs build therefore assembles the retail
// instructions verbatim; the modern build keeps the equivalent C.
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

// FUNC 0x01012F68 global_lock_acquire
// If the lock semaphore does not exist yet (id == -1), creates it with
// create_mutex_sema(1) (binary semaphore, initially available) and stores
// the id; then WaitSema(id) (syscall 0x44), whose result is returned (it is
// the last call: $v0 passes through; egcs does not turn a call whose value
// is returned into a tail jump, hence jal + empty delay slot).  The id stays
// in $a0 from the first load, or the fresh one after the creation.  The
// lazy creation itself is not protected against a concurrent first use.
// Callers: host_open, host_lseek, host_read, host_write, host_close
// (hostfs_io unit, src/hostfs_io.cpp), which ignore the result.
int global_lock_acquire(void)
{
    int sema = s_globalLockSema;

    if (sema == -1) {
        sema = create_mutex_sema(1);
        s_globalLockSema = sema;
    }
    return WaitSema(sema);
}

// FUNC 0x01012FA0 global_lock_release
// Tail jump to signal_sema_helper(s_globalLockSema) (0x0100F6B0, which calls
// iSignalSema); its return value passes through in $v0 but no caller uses
// it.  Same five callers as global_lock_acquire.
void global_lock_release(void)
{
    signal_sema_helper(s_globalLockSema);
}

// FUNC 0x01012FA8 break_stub_0
// `break` (code 0) trap, then return 0.  Only reference: the .sdata pointer
// g_break_hook_0 at 0x01041E48 (itself unreferenced by code).  Role unknown;
// looks like a debugger-trap placeholder for a libc hook (its neighbour
// break_stub_1 stands in for raise()).
int break_stub_0(void)
{
    __asm__ __volatile__("break");
    return 0;
}

// FUNC 0x01012FB8 break_stub_1
// `break` (code 0) trap, then return 0; the argument is ignored.  abort()
// (0x0101AFF0) calls it with 6 (SIGABRT) in its `for (;;) { raise(6);
// _exit(1); }` loop, so it replaces newlib's raise(); a pointer to it is also
// stored at .sdata 0x01041E4C (g_break_hook_1).
int break_stub_1(int code)
{
    __asm__ __volatile__("break");
    return 0;
}

// FUNC 0x01012FC8 tty_puts
// PrintTarget::flush's backend (printf unit): tail jump to the libkernl
// kernel TTY printf, scePrintf("%s", s) (0x0101D2A8).  The "%s" literal is
// the 3-byte .sdata string at 0x01041E68, addressed with lui/addiu like
// every string literal.
void tty_puts(const char *s)
{
    scePrintf("%s", s);
}

/* The two .sdata words pointing at the stubs (no code reads them).  They sit
 * at 0x01041E48 / 0x01041E4C, between PrintFormat's "(null)" (0x01041E40,
 * src/pdistd/printformat.cpp) and the allocator front-ends' s_heapBase
 * (0x01041E50), so they belong to one of those two objects (or to a
 * data-only object between them), not to this one, whose .sdata starts at
 * 0x01041E60 (tools/layout.py, "LINK ORDER").  No code references them, so
 * nothing pins the object; they stay here, next to the stubs. */
int (*g_break_hook_0)(void) = break_stub_0;     /* @0x01041E48 (.sdata) */
int (*g_break_hook_1)(int)  = break_stub_1;     /* @0x01041E4C (.sdata) */

} /* extern "C" */
