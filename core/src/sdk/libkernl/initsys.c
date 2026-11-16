/* initsys.c - Sony libkernl 2240 diei.o (DIntr, EIntr) and
 * initsys.o (_InitSys and its private SetSyscall stub), 0x0028D6A0..
 * 0x0028D778.  Same code as the bootstrap's sdk/libkernl/initsys.c.
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part2_internal.h"

/* mfc0 $12 (COP0 Status) as a volatile asm (must not be moved across di/ei). */
static __inline__ u32 _cop0_status(void)
{
    u32 status;
    __asm__ __volatile__("mfc0 %0, $12" : "=r"(status));
    return status;
}

// FUNC 0x0028D6A0 DIntr
//   Disable interrupts: if Status.EIE is set, loop `di; sync.p` until EIE
//   reads back clear (R5900 di erratum) and return 1; else return 0.
//   35 callers (libkernl, libcdvd, game code).
//   The retry loop is 8-byte aligned (the nop before `di`) by an explicit
//   .p2align, and written with a label: as a do/while, egcs's loop optimiser
//   would hoist the 0x10000 out of it, which the binary does not.
//   The binary has the final `move v0, a0` in the delay slot of the last
//   `jr ra`: libkernl's assembler (standard GAS) filled the delay slot of the
//   epilogue's `j $31`, which the bundled one leaves as a nop; the file's
//   gas_swap pass (ref/compile_flags.json) does the same.
int DIntr(void)
{
    int eie, was_enabled, status;

    __asm__ __volatile__("mfc0\t%0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie) {
        was_enabled = eie != 0;
        __asm__(".p2align 3");
    retry:
        __asm__ __volatile__("di" : : : "memory");
        __asm__ __volatile__("sync.p" : : : "memory");
        __asm__ __volatile__("mfc0\t%0, $12" : "=r"(status));
        status &= EE_COP0_STATUS_EIE;
        if (status)
            goto retry;
    } else
        was_enabled = 0;
    return was_enabled;
}

// FUNC 0x0028D6E8 EIntr
//   Enable interrupts; returns whether they were enabled before the `ei`.
int EIntr(void)
{
    u32 eie = _cop0_status() & EE_COP0_STATUS_EIE;

    EI();
    return eie != 0;
}

/* ------------------------------------------------------------------------ */
/* initsys.o                                                                */
/* ------------------------------------------------------------------------ */

#define SYSCALL_SET_SYSCALL     0x74    /* kernel syscall number of SetSyscall */
#define USER_SYSCALL_FIRST      0x80    /* user-definable syscall slots 0x80..0xFF */
#define USER_SYSCALL_END        0x100

/* initsys.o's private copy of the SetSyscall stub (index.txt names it
 * SetSyscall; the public stub is in the syscall table 0x00285840..).  A
 * local symbol of the asm below (no .globl), only called by
 * clear_user_syscalls.                                                     */
void _initsys_SetSyscall(int syscall_no, void *handler);          /* 0x0028D700 */

// FUNC 0x0028D700 _initsys_SetSyscall
//   Raw kernel call: `li $v1, 0x74 (SetSyscall); syscall; jr $ra; nop`.
//   (.section/.previous, not .pushsection/.popsection: the SCE assembler
//   2.9-ee-991111b lacks the latter.)
__asm__(
    "\t.section .text\n"
    "\t.align 3\n"
    "\t.type _initsys_SetSyscall, @function\n"
    "_initsys_SetSyscall:\n"
    "\t.set push\n"
    "\t.set noreorder\n"
    "\taddiu $3, $0, 0x74\n"            /* SYSCALL_SET_SYSCALL */
    "\tsyscall\n"
    "\tjr $31\n"
    "\tnop\n"
    "\t.set pop\n"
    "\t.size _initsys_SetSyscall, .-_initsys_SetSyscall\n"
    "\t.previous\n");

// FUNC 0x0028D710 clear_user_syscalls
//   Reset every user-definable kernel syscall slot 0x80..0xFF to NULL.
//   Only called from _InitSys.
static void clear_user_syscalls(void)
{
    int syscall_no;

    for (syscall_no = USER_SYSCALL_FIRST; syscall_no < USER_SYSCALL_END; syscall_no++)
        _initsys_SetSyscall(syscall_no, NULL);
}

// FUNC 0x0028D750 _InitSys
//   libkernl start-up, called by crt0 (_start, the part funcs.json lists as
//   F_00100014; runtime/startup/G000_collect2_ctors.h) before main: clear user
//   syscalls, install the alarm kernel patch (InitAlarm 0x00286910) and
//   tail-call the thread-server init (InitThread 0x00286AC0).
void _InitSys(void)
{
    clear_user_syscalls();
    InitAlarm();
    InitThread();                       /* j 0x00286AC0 */
}
