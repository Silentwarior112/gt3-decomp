/* initsys_glue.c - Sony libkernl 2.2.4 initsys/glue part,
 * 0x002860D0..0x00286407:
 *   - the TTY re-open hook used after an IOP reboot
 *   - two INTC vblank-wait helpers
 *   - glue.o: newlib POSIX syscall glue (write/read to the DECI2 TTY, sbrk,
 *     and trivial stubs)
 * The same machine code as the bootstrap's sdk/libkernl/initsys.c
 * (0x01022C40..0x01022F77), reused; the bootstrap's DIntr/EIntr/_InitSys
 * sit elsewhere in this binary (0x0028D6A0..).  Its .data (0x00307A10..
 * 0x00307A18, after libkernl's version string, before alarm.o's patch) is
 * the TTY flag (s_tty_opened) and the program break (s_heap_break), file
 * statics as in the bootstrap's initsys.c, whose names they take; only this
 * file's code uses them.
 *
 * Names from the evidence of this binary's newlib callers (the bootstrap
 * has no callers of the glue, so its names there are positional guesses):
 *   0x002862A0  close   (_close_r calls it)
 *   0x002862A8  unknown (no callers; inside the 0x002862A0 funcs.json entry;
 *                        the bootstrap calls this slot lseek)
 *   0x002862B0  lseek   (_lseek_r calls it; index/bootstrap: stub_ret_minus1_c)
 *   0x00286368  isatty  (__smakebuf calls it after _fstat_r; index/bootstrap: getpid)
 *   0x00286388  getpid  (no callers; index/bootstrap: isatty)
 * The two `return 1` functions are the same code, so the swap changes no
 * behaviour.
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part3_internal.h"

#define INTC_STAT         REG32(0x1000F000)     /* write 1 = acknowledge */
#define INTC_VBLANK_S_BIT (1 << INTC_VBLANK_S)  /* 4 */

/* write()/read() lazily open the DECI2 TTY and set this flag. */
static int s_tty_opened = 0;                    /* 0x00307A10 */

// FUNC 0x002860D0 tty_reinit_after_iop_reset
//   Clear the TTY-opened flag (s_tty_opened) and tail-jump to _deci2_tty_init
//   (re-open the DECI2 TTYP protocol after the IOP - and with it the DECI2
//   manager - was rebooted).  Called from sceSifSyncIop (0x0028D548).
//   Quirk: the flag is left 0 even when the re-open succeeds, so the next
//   write()/read() opens the TTY (another sceDeci2Open) once more.
void tty_reinit_after_iop_reset(void)
{
    s_tty_opened = 0;
    _deci2_tty_init();
}

/* ------------------------------------------------------------------------ */
/* vblank helpers                                                           */
/* ------------------------------------------------------------------------ */

// FUNC 0x002860E0 wait_vblank_start
//   Busy-wait for the next VBLANK start using the raw INTC status: ack
//   INTC_STAT bit 2 (VBON), spin until it is set again, ack it.  Unlike the
//   bootstrap, the game calls it here: pjoy_bind (0x002655B0), pdi1394_bind
//   (0x0026E530), both between sceSifBindRpc retries, and 0x0026EC20
//   (pdi1394), while an RPC is still busy.
void wait_vblank_start(void)
{
    volatile u32 *stat = &INTC_STAT;

    *stat = INTC_VBLANK_S_BIT;
    while ((*stat & INTC_VBLANK_S_BIT) == 0)
        ;
    *stat = INTC_VBLANK_S_BIT;
}

// FUNC 0x00286120 wait_vblank_get_csr
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

extern char _end[];                             /* the link's end of .bss (0x004B2C0C) */

/* The program break for sbrk, initially the end of .bss (crt0 also does
 * InitHeap(_end, -1)); the original's word is 0x004B2C0C = _end. */
static char *s_heap_break = _end;               /* 0x00307A14 */

// FUNC 0x00286180 write
//   Only stdout/stderr (fd 1, 2): lazily open the DECI2 TTY, then send the
//   bytes to the host console (_deci2_tty_write).  -1 for other fds or if
//   the TTY can't be opened.  Caller: _write_r.
int write(int fd, const void *buf, size_t nbytes)
{
    if (fd == 1 || fd == 2) {
        if (s_tty_opened == 0) {
            if (_deci2_tty_init() == 0)
                return -1;
            s_tty_opened = 1;
        }
        return _deci2_tty_write((const char *)buf, nbytes);
    }
    return -1;
}

// FUNC 0x00286200 read
//   Only stdin (fd 0): lazily open the DECI2 TTY, then read a line from the
//   host console's receive ring.  Caller: _read_r.
int read(int fd, void *buf, size_t nbytes)
{
    if (fd == 0) {
        if (s_tty_opened == 0) {
            if (_deci2_tty_init() == 0)
                return -1;
            s_tty_opened = 1;
        }
        return _deci2_tty_read((char *)buf, nbytes);
    }
    return -1;
}

// FUNC 0x00286278 open
//   Unsupported: errno (*__errno()) = EIO, return -1.  No callers.
int open(const char *name, int flags)
{
    errno = EIO;                        /* implicit: *__errno() */
    return -1;
}

// FUNC 0x002862A0 close
//   Unsupported: return -1 (errno untouched).  Caller: _close_r.
int close(int fd)
{
    return -1;
}

// FUNC 0x002862A8 F_002862A8
/* Another `return -1` glue stub, no callers; which newlib syscall it is
 * cannot be told (the bootstrap calls this slot lseek, but _lseek_r here
 * calls 0x002862B0).  (Its own entry since the 2026-10-16 regen.) */
int F_002862A8(void)
{
    return -1;
}

// FUNC 0x002862B0 lseek
//   Unsupported: return -1 (errno untouched).  Caller: _lseek_r, hence the
//   name (index.txt / bootstrap: stub_ret_minus1_c).
s64 lseek(int fd, s64 offset, int whence)
{
    return -1;
}

// FUNC 0x002862B8 sbrk
//   Move the program break (s_heap_break) by incr with interrupts disabled
//   (inline DI loop if Status.EIE was set, `ei` again on exit only in that
//   case).  Fails with errno = ENOMEM and (void *)-1 when the new break
//   passes EndOfHeap() (unsigned compare).  Returns the old break.  Caller:
//   _sbrk_r (0x00282A20, mallocr's MORECORE).
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
        errno = ENOMEM;                 /* implicit: *__errno() */
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

// FUNC 0x00286368 isatty
//   Every fd is a TTY: return 1.  Caller: __smakebuf (0x00281C38, after
//   _fstat_r), hence the name (index.txt / bootstrap: getpid).
int isatty(int fd)
{
    return 1;
}

// FUNC 0x00286370 fstat
//   Everything is a character device: st_mode = S_IFCHR, st_blksize (+0x48,
//   64-bit) = 0, return 0 - newlib then treats all fds as unbuffered TTYs.
//   Caller: _fstat_r.
int fstat(int fd, struct stat *st)
{
    st->st_blksize = 0;
    st->st_mode = S_IFCHR;
    return 0;
}

// FUNC 0x00286388 getpid
//   The only process: pid 1 (pairs with kill()).  No callers.
//   (index.txt / bootstrap: isatty; same code.)
int getpid(void)
{
    return 1;
}

// FUNC 0x00286390 kill
//   kill(1, sig) terminates the program via the Exit syscall stub
//   (0x00285880) with sig as status; any other pid is ignored.  Returns 0.
int kill(int pid, int sig)
{
    if (pid == 1)
        Exit(sig);
    return 0;
}

// FUNC 0x002863B8 link
//   Unsupported: errno = EIO, return -1.
int link(const char *oldpath, const char *newpath)
{
    errno = EIO;                        /* implicit: *__errno() */
    return -1;
}

// FUNC 0x002863E0 unlink
//   Unsupported: errno = EIO, return -1.
int unlink(const char *path)
{
    errno = EIO;                        /* implicit: *__errno() */
    return -1;
}
