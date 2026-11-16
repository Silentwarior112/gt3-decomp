/* newlib_part3_internal.h - unit u139 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00283D50..0x002869E8.  Translation unit G007 (no-FDE C code, shared with
 * units u137/u138 before and the rest of libkernl after).
 *
 * Contents, in address (link) order:
 *   strtoul.c       0x00283D50  newlib strtoul (wrapper of _strtoul_r)
 *   ungetc.c        0x00283D88  newlib ungetc.o: __submore, ungetc
 *   syscalls_r.c    0x00284000  newlib reentrant syscall wrappers
 *                                         _write_r, _close_r, _fstat_r, _lseek_r
 *   mprec.c         0x00284178  newlib mprec.o (Bigint helpers of
 *                                         strtod / dtoa) .. _mprec_log10
 *   readr.c         0x00285198  newlib _read_r
 *   mallocr_part2.c       0x002851F8  newlib mallocr (Doug Lea 2.6):
 *                                         _realloc_r, _calloc_r
 *   syscalls.S      0x00285840  Sony libkernl syscall stub table
 *                                         (#0x00 .. #0x7F)
 *   initsys_glue.c       0x002860D0  libkernl initsys/glue: TTY re-init,
 *                                         vblank waits, newlib POSIX glue
 *   kernel.c        0x00286408  libkernl cache.o, intr.o, alarm.o
 *
 * Everything from 0x00285840 on is the same machine code as the bootstrap's
 * libkernl (decomp/bootstrap/sdk/libkernl, runtime/libc_syscall_stubs.S),
 * except the PollSema..GetCop0 stubs and the close/(unknown) pair at
 * 0x002862A0, which funcs.json cuts differently.
 *
 * The shared newlib types (FILE, struct _reent, _Bigint, malloc_chunk), the
 * data (_impure_ptr, __malloc_av_, __mprec_tens, sys_errno) and every function
 * another unit calls are in runtime/newlib/G007_stdio.h, the canonical header of G007.
 * This header keeps what only u139 uses.
 *
 * syscalls.S includes this header too: everything C is behind
 * __ASSEMBLER__.
 */
#ifndef U139_H
#define U139_H

#ifndef __ASSEMBLER__

#include "types.h"
#include "libc.h"               /* errno, EIO, ENOMEM; the newlib POSIX glue prototypes
                                   (write, read, open, close, lseek, fstat, getpid, kill, ...) */
#include "eekernel.h"           /* DI/EI, the syscall stubs (_EnableIntc .., EndOfHeap,
                                   SetVSyncFlag, Exit), DIntr/EIntr, the i-cache wrappers */
#include "runtime/newlib/G007_stdio.h"    /* own TU: FILE, _reent, _Bigint, malloc_chunk, _impure_ptr,
                                   __malloc_av_, __mprec_tens, sys_errno, the G007 prototypes */

/* ======================================================================== */
/* newlib constants (stdio.h)                                               */
/* ======================================================================== */

#define EOF     (-1)
#define BUFSIZ  1024

/* ======================================================================== */
/* newlib mprec.o: the routines only mprec itself calls                     */
/* ======================================================================== */

_Bigint *__multadd(struct _reent *ptr, _Bigint *b, int m, int a);        /* 0x00284250 */
int      __hi0bits(u32 x);                                               /* 0x002844B0 */
int      __lo0bits(u32 *y);                                              /* 0x00284538 */
double   __b2d(_Bigint *a, int *e);                                      /* 0x00284D68 */
double   _mprec_log10(int dig);                                          /* 0x00285128 (no callers) */

/* pow5mult's p05[3] = {5, 25, 125} (0x00348FC0, .rodata) is defined in
 * __pow5mult as the original's function-local static; __mprec_tens (extern
 * in runtime/newlib/G007_stdio.h), __mprec_bigtens and __mprec_tinytens (extern in
 * newlib_part2_internal.h for strtod) are defined in mprec.c. */

/* mprec.h's union double_union: the IEEE halves, little-endian (IEEE_8087):
 * word0 = sign/exponent/high mantissa = the high 32 bits of the 64-bit
 * register a double travels in. */
union double_union {
    double d;
    u32    i[2];
};
#define word0(x) ((x).i[1])
#define word1(x) ((x).i[0])

/* ======================================================================== */
/* newlib mallocr (Doug Lea malloc 2.6.x): only __submore / _Balloc call    */
/* ======================================================================== */

typedef struct malloc_chunk *mchunkptr;

void *_realloc_r(struct _reent *reent_ptr, void *oldmem, size_t bytes);  /* 0x002851F8 */
void *_calloc_r(struct _reent *reent_ptr, size_t n, size_t elem_size);   /* 0x00285768 */

/* ======================================================================== */
/* libkernl (initsys / glue, kernel.c)                                      */
/* ======================================================================== */

/* struct stat: runtime/newlib/G007_stdio.h. */
#define S_IFCHR  0x2000     /* character special */

/* initsys glue data (.data 0x00307A10..0x00307A18): file statics of
 * initsys_glue.c, as in the bootstrap's initsys.c, whose names they
 * take: s_tty_opened (0x00307A10, the DECI2 TTY opened flag, 0) and
 * s_heap_break (0x00307A14, sbrk's program break, initially _end =
 * 0x004B2C0C).  (A C `static' after an `extern' of the same name does not
 * compile with GCC 14, so no extern here.) */

/* initsys glue functions no other unit calls. */
u64  wait_vblank_get_csr(void);                 /* 0x00286120 (no callers) */
int  F_002862A8(void);                          /* 0x002862A8 (inside the 0x002862A0 entry) */

/* alarm.o private syscall stubs (same 16-byte shape as the stub table).    */
void  _alarm_SetSyscall(int syscall_no, void *handler);         /* 0x002868A8 #0x74 */
void *_alarm_Copy(void *dst, const void *src, int nbytes);      /* 0x002868B8 #0x5A */
void *_alarm_GetEntryAddress(int syscall_no);                   /* 0x00286900 #0x5B */

/* alarm.o data (.data 0x00307A18..0x003081B8): file statics of
 * kernel.c, as in the bootstrap's kernel.c, whose names they take:
 * _alarm_patch_code (0x00307A18, the 0x740-byte kernel patch copied to
 * 0x80076000), _alarm_patch_stub (0x00308158, the 0x20-byte callback
 * trampoline copied to 0x00082000) and _alarm_syscalls (0x00308178, the
 * install table).  (A C `static' after an `extern' of the same name does not
 * compile with GCC 14, so no externs here.) */
struct _alarm_syscall {
    int   num;          /* 0x00 */
    void *handler;      /* 0x04 (entries 2..7: 0, resolved via GetEntryAddress) */
};
EGCS_SIZE(struct _alarm_syscall, 0x8);

#endif /* !__ASSEMBLER__ */

#endif /* U139_H */
