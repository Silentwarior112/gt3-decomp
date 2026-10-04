/* initsys.c - Sony libkernl 2.2.4 (PsIIlibkernl2240), 0x01022B68 .. 0x01022F78:
 *   - diei.o      DIntr / EIntr
 *   - initsys.o   private SetSyscall stub, user-syscall clearing, _InitSys,
 *                 the TTY re-open hook used after an IOP reboot
 *   - two dead INTC vblank-wait helpers
 *   - glue.o      newlib POSIX syscall glue (write/read to the DECI2 TTY,
 *                 sbrk, and trivial stubs)
 *
 * None of the glue functions has a caller in the bootstrap: Polyphony's
 * printf/allocator do not go through newlib's stdio or sbrk.
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "kernl_rest_internal.h"

#define INTC_STAT        REG32(0x1000F000)   /* write 1 = acknowledge */
#define INTC_VBLANK_S_BIT (1 << INTC_VBLANK_S)  /* 4 */

/* mfc0 $12 as a volatile asm (must not be moved across di/ei). */
static __inline__ u32 _cop0_status(void)
{
    u32 status;
    __asm__ __volatile__("mfc0 %0, $12" : "=r"(status));
    return status;
}

/* ------------------------------------------------------------------------ */
/* diei.o                                                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x01022B68 DIntr
//   Disable interrupts: if Status.EIE is set, loop `di; sync.p` until EIE
//   reads back clear (R5900 di erratum) and return 1; else return 0.
//   23 callers (libkernl, libcdvd, game code).
//   The retry loop is 8-byte aligned (the nop before `di`) by an explicit
//   .p2align, and written with a label: as a do/while, egcs's loop optimiser
//   would hoist the 0x10000 out of it, which the binary does not.
//   Remaining difference: the binary has the final `move v0, a0` in the
//   delay slot of the last `jr ra` - libkernl's assembler filled the delay
//   slot of the epilogue's `j $31`, which the bundled one leaves as a nop.
int DIntr(void)
{
    int eie, res, st;

    __asm__ __volatile__("mfc0\t%0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie) {
        res = eie != 0;
        __asm__(".p2align 3");
    retry:
        __asm__ __volatile__("di" : : : "memory");
        __asm__ __volatile__("sync.p" : : : "memory");
        __asm__ __volatile__("mfc0\t%0, $12" : "=r"(st));
        st &= EE_COP0_STATUS_EIE;
        if (st)
            goto retry;
    } else
        res = 0;
    return res;
}

// FUNC 0x01022BB0 EIntr
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

/* initsys.o's private copy of the SetSyscall stub (the public one is
 * 0x0101BF60 in syscalls.S).  Local symbol, only called by
 * clear_user_syscalls.                                                     */
void _initsys_SetSyscall(int syscall_no, void *handler);

// FUNC 0x01022BC8 _initsys_SetSyscall
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
    "\taddiu $3, $0, 0x74\n"
    "\tsyscall\n"
    "\tjr $31\n"
    "\tnop\n"
    "\t.set pop\n"
    "\t.size _initsys_SetSyscall, .-_initsys_SetSyscall\n"
    "\t.previous\n");

// FUNC 0x01022BD8 clear_user_syscalls
//   Reset every user-definable kernel syscall slot 0x80..0xFF to NULL.
//   Only called from _InitSys.
static void clear_user_syscalls(void)
{
    int i;

    for (i = 0x80; i < 0x100; i++)
        _initsys_SetSyscall(i, NULL);
}

// FUNC 0x01022C18 _InitSys
//   libkernl start-up, called by crt0 (F_01000008) before main: clear user
//   syscalls, install the alarm kernel patch (InitAlarm 0x0101C558) and
//   tail-call the thread-server init (InitThread 0x0101C708).
void _InitSys(void)
{
    clear_user_syscalls();
    InitAlarm();
    InitThread();                       /* j 0x0101C708 */
}

/* write()/read() lazily open the DECI2 TTY and set this flag.              */
static int s_tty_opened = 0;            /* 0x0103DADC .data */

// FUNC 0x01022C40 tty_reinit_after_iop_reset
//   Clear the TTY-opened flag and tail-jump to _deci2_tty_init (re-open the
//   DECI2 TTYP protocol after the IOP - and with it the DECI2 manager - was
//   rebooted).  Called from sceSifSyncIop.  Quirk: the flag is left 0 even
//   when the re-open succeeds, so the next write()/read() opens the TTY
//   (another sceDeci2Open) once more.  void: the only caller ignores the
//   result (the tail call leaves _deci2_tty_init's in $v0), and this
//   compiler only emits a `j` tail call in a function returning void.
void tty_reinit_after_iop_reset(void)
{
    s_tty_opened = 0;
    _deci2_tty_init();
}

/* ------------------------------------------------------------------------ */
/* vblank helpers (dead code: no callers)                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x01022C50 wait_vblank_start
//   Busy-wait for the next VBLANK start using the raw INTC status: ack
//   INTC_STAT bit 2 (VBON), spin until it is set again, ack it.  No callers.
void wait_vblank_start(void)
{
    volatile u32 *stat = &INTC_STAT;

    *stat = INTC_VBLANK_S_BIT;
    while ((*stat & INTC_VBLANK_S_BIT) == 0)
        ;
    *stat = INTC_VBLANK_S_BIT;
}

// FUNC 0x01022C90 wait_vblank_get_csr
//   Register a kernel vsync flag (SetVSyncFlag: at the next vsync the kernel
//   sets *flag and stores the GS CSR into *csr), ack INTC VBON, spin until
//   VBON is raised or the kernel flag is set, ack VBON, return the CSR
//   snapshot.  No callers.
u64 wait_vblank_get_csr(void)
{
    volatile u32 *stat;
    volatile u32 flag;                  /* sp+0 */
    volatile u64 csr;                   /* sp+8 */

    flag = 0;
    SetVSyncFlag((u32 *)&flag, (u64 *)&csr);
    stat = &INTC_STAT;
    *stat = INTC_VBLANK_S_BIT;
    while ((*stat & INTC_VBLANK_S_BIT) == 0 && flag == 0)
        ;
    *stat = INTC_VBLANK_S_BIT;
    return csr;
}

/* ------------------------------------------------------------------------ */
/* glue.o - newlib POSIX syscall glue                                       */
/* ------------------------------------------------------------------------ */

/* newlib struct stat as seen by this EE libc (64-bit `long`).  Only st_mode
 * (+0x04) and the 64-bit field at +0x48 are evidenced by fstat; the rest is
 * the newlib layout of that era.  ? layout                                  */
struct stat {
    s16 st_dev;         /* +0x00 */
    u16 st_ino;         /* +0x02 */
    u32 st_mode;        /* +0x04 */
    u16 st_nlink;       /* +0x08 */
    u16 st_uid;         /* +0x0A */
    u16 st_gid;         /* +0x0C */
    s16 st_rdev;        /* +0x0E */
    s64 st_size;        /* +0x10 */
    s64 st_atime_;      /* +0x18 */
    s64 st_spare1;      /* +0x20 */
    s64 st_mtime_;      /* +0x28 */
    s64 st_spare2;      /* +0x30 */
    s64 st_ctime_;      /* +0x38 */
    s64 st_spare3;      /* +0x40 */
    s64 st_blksize;     /* +0x48 */
    s64 st_blocks;      /* +0x50 */
    s64 st_spare4[2];   /* +0x58 */
};
_Static_assert(offsetof(struct stat, st_blksize) == 0x48, "stat.st_blksize");

#define S_IFCHR 0x2000

extern char _end[];                     /* linker: end of .bss = 0x0104DE94 */

/* Program break for sbrk (crt0 also does InitHeap(_end, -1)).              */
static char *s_heap_break = _end;       /* 0x0103DAE0 .data */

// FUNC 0x01022CF0 write
//   Only stdout/stderr (fd 1, 2): lazily open the DECI2 TTY, then send the
//   bytes to the host console.  -1 for other fds or if the TTY can't be opened.
int write(int fd, const void *buf, size_t nbytes)
{
    if (fd == 1 || fd == 2) {
        if (s_tty_opened == 0) {
            if (_deci2_tty_init() == 0)
                return -1;
            s_tty_opened = 1;
        }
        return _deci2_tty_write(buf, nbytes);
    }
    return -1;
}

// FUNC 0x01022D70 read
//   Only stdin (fd 0): lazily open the DECI2 TTY, then read a line from the
//   host console's receive ring.
int read(int fd, void *buf, size_t nbytes)
{
    if (fd == 0) {
        if (s_tty_opened == 0) {
            if (_deci2_tty_init() == 0)
                return -1;
            s_tty_opened = 1;
        }
        return _deci2_tty_read(buf, nbytes);
    }
    return -1;
}

// FUNC 0x01022DE8 open
//   Unsupported: errno = EIO, return -1.  (? identity: position after read
//   suggests open.)
int open(const char *name, int flags)
{
    errno = EIO;
    return -1;
}

// FUNC 0x01022E10 close
//   Unsupported: return -1 (errno untouched).  ? identity (close/lseek/...).
int close(int fd)
{
    return -1;
}

// FUNC 0x01022E18 lseek
//   Unsupported: return -1 (errno untouched).  ? identity.
int lseek(int fd, int offset, int whence)
{
    return -1;
}

// FUNC 0x01022E20 stub_ret_minus1_c
//   Third `return -1` glue stub; which newlib syscall it implements is not
//   evidenced (no references).  ? identity
int stub_ret_minus1_c(void)
{
    return -1;
}

// FUNC 0x01022E28 sbrk
//   Move the program break by incr with interrupts disabled (inline DI loop
//   if Status.EIE was set, `ei` again on exit only in that case).  Fails with
//   errno = ENOMEM and (void *)-1 when the new break passes EndOfHeap()
//   (unsigned compare).  Returns the old break.  No callers here.
void *sbrk(int incr)
{
    int eie;
    char *old_break;
    char *new_break;

    __asm__ __volatile__("mfc0\t%0, $12" : "=r"(eie));
    eie &= EE_COP0_STATUS_EIE;
    if (eie)
        DI();
    new_break = s_heap_break + incr;
    if ((u32)EndOfHeap() < (u32)new_break) {
        errno = ENOMEM;
        if (eie)
            EI();
        return (void *)-1;
    }
    old_break = s_heap_break;           /* re-read after the call */
    s_heap_break = new_break;
    if (eie)
        EI();
    return old_break;
}

// FUNC 0x01022ED8 getpid
//   The only process: pid 1 (pairs with kill()).
int getpid(void)
{
    return 1;
}

// FUNC 0x01022EE0 fstat
//   Everything is a character device: st_mode = S_IFCHR, st_blksize (+0x48,
//   64-bit) = 0, return 0 - newlib then treats all fds as unbuffered TTYs.
int fstat(int fd, struct stat *st)
{
    st->st_blksize = 0;
    st->st_mode = S_IFCHR;
    return 0;
}

// FUNC 0x01022EF8 isatty
//   Every fd is a TTY.
int isatty(int fd)
{
    return 1;
}

// FUNC 0x01022F00 kill
//   kill(1, sig) terminates the program via the Exit syscall stub
//   (0x0101B800, the same one crt0 jumps to) with sig as status; any other
//   pid is ignored.  Returns 0.
int kill(int pid, int sig)
{
    if (pid == 1)
        Exit(sig);
    return 0;
}

// FUNC 0x01022F28 link
//   Unsupported: errno = EIO, return -1.  (identity from the mapping pass)
int link(const char *oldpath, const char *newpath)
{
    errno = EIO;
    return -1;
}

// FUNC 0x01022F50 unlink
//   Unsupported: errno = EIO, return -1.  (identity from the mapping pass)
int unlink(const char *path)
{
    errno = EIO;
    return -1;
}
