/* libc.h - C library interface of the GT3 US bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * Shared by every decompilation unit.  Declares exactly the libc symbols that
 * exist in (or are referenced by) this binary, with their addresses:
 *
 *   - Sony EE libc (MMI-optimised string/memory routines, abort, errno) - owned
 *     by the "libc" unit (runtime/libc.c);
 *   - the game-provided 128-bit strlen (@0x0100F538, also in the libc unit);
 *   - malloc/free/realloc/calloc and printf/vprintf, which in this binary are
 *     Polyphony's pdistd implementations (allocator / printf units) that
 *     override the libc archive copies - Sony libraries (libkernl loadfile,
 *     libpad, ...) call them as plain libc symbols;
 *   - the newlib POSIX syscall glue implemented inside libkernl (kernl_rest
 *     unit), declared here so the one owner and any reader agree on types.
 *
 * Do NOT include the toolchain's newlib headers (<string.h>, <stdlib.h>,
 * <stdio.h>, <errno.h>) in decompiled sources; use this header instead.
 * <stddef.h>/<stdarg.h> are the compiler's freestanding headers and are safe.
 */
#ifndef BOOT_LIBC_H
#define BOOT_LIBC_H

#include "types.h"
#include <stddef.h>   /* size_t (unsigned int on the EE), NULL, offsetof */
#include <stdarg.h>   /* va_list for vprintf */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* Sony EE libc: memory / string                                            */
/* ------------------------------------------------------------------------ */

void  *memcpy(void *dst, const void *src, size_t n);          /* 0x0101B010 */
void  *memmove(void *dst, const void *src, size_t n);         /* 0x0101B0BC */
void  *memset(void *dst, int c, size_t n);                    /* 0x0101B1B8 */
int    memcmp(const void *s1, const void *s2, size_t n);      /* 0x0102694C (2nd libc pass, after libgcc) */
char  *strcat(char *dst, const char *src);                    /* 0x0101B264 */
int    strcmp(const char *a, const char *b);                  /* 0x0101B380 */
char  *strcpy(char *dst, const char *src);                    /* 0x0101B4C4 */
char  *strncpy(char *dst, const char *src, size_t n);         /* 0x0101B5D8 (used by libmc / loadfile) */

/* Game-provided MMI strlen (lq + pceqb + plzcw); it satisfies libc's and libmc's
 * references, so there is no separate libc strlen in the binary.            */
size_t strlen(const char *s);                                 /* 0x0100F538 */

/* ------------------------------------------------------------------------ */
/* Sony EE libc: process / errno                                            */
/* ------------------------------------------------------------------------ */

/* abort: loop { break_stub_1(6); _exit(1); } - never returns.              */
void   abort(void) __attribute__((noreturn));                 /* 0x0101AFF0 */

/* crt0's _exit: tail-jumps to Exit(0) - NOTE the status argument is ignored
 * (a0 is forced to 0).  Lives in src/crt0.S (main unit).                    */
void   _exit(int status);                                     /* 0x010000B8 */

/* The "raise"-like hook abort calls with 6 (SIGABRT): `break; return 0`.
 * Pointer to it is also stored at .sdata 0x01041E4C.  Owned by the allocator
 * unit (src/pdistd/pdistd_misc.cpp); name from the mapping pass.            */
int    break_stub_1(int code);                                /* 0x01012FB8 */

/* newlib-style errno: returns &_impure_ptr->_errno; _impure_ptr @0x0103DDD4
 * points at struct _reent @0x0103DAE8 whose first word is _errno.           */
int   *__errno(void);                                         /* 0x01026940 */
#ifndef errno
#define errno (*__errno())
#endif

/* errno values observed in this binary (newlib / IOP numbering).            */
#ifndef EIO
#define EIO     5
#endif
#ifndef EBADF
#define EBADF   9
#endif
#ifndef EAGAIN
#define EAGAIN  11
#endif
#ifndef ENOMEM
#define ENOMEM  12
#endif
#ifndef ENODEV
#define ENODEV  19
#endif
#ifndef EINVAL
#define EINVAL  22
#endif

/* ------------------------------------------------------------------------ */
/* Heap - Polyphony pdistd::Allocator front-ends (allocator unit)           */
/* ------------------------------------------------------------------------ */

void  *malloc(size_t size);                                   /* 0x01011390 */
void   free(void *ptr);                                       /* 0x01011368 */
void  *realloc(void *ptr, size_t size);                       /* 0x010113C8 */
void  *calloc(size_t nmemb, size_t size);                     /* 0x01011320 (unreferenced) */
/* memalign(align, size): allocate with alignment max(align, 16).  Callers:
 * startup screen thread 0x01000A50 and 0x01005458, both with align 0x40.    */
void  *memalign(size_t align, size_t size);                   /* 0x01011410 */

/* ------------------------------------------------------------------------ */
/* stdio - Polyphony printf.cxx (printf unit)                               */
/* ------------------------------------------------------------------------ */

/* The C-library printf of this binary.  Game code and Sony libraries that
 * call "printf" (libpad debug, libkernl sceSifRebootIop, ...) land here.
 * The libkernl kernel/TTY printf at 0x0101D2A8 is a different function:
 * see scePrintf() in eekernel.h.                                            */
int    printf(const char *fmt, ...);                          /* 0x01011678 */
int    vprintf(const char *fmt, va_list ap);                  /* 0x010115D8 */

/* ------------------------------------------------------------------------ */
/* newlib POSIX syscall glue implemented in libkernl (kernl_rest unit).     */
/* Nothing in the game calls these; they exist for newlib's benefit.        */
/* ------------------------------------------------------------------------ */

struct stat;   /* newlib layout; fstat only writes +0x04 (st_mode=0x2000 S_IFCHR) and u64 @+0x48 */

int    write(int fd, const void *buf, size_t nbytes);         /* 0x01022CF0 (-> DECI2 TTY) */
int    read(int fd, void *buf, size_t nbytes);                /* 0x01022D70 (<- DECI2 TTY) */
int    open(const char *name, int flags);                     /* 0x01022DE8 errno=EIO, -1 */
int    close(int fd);                                         /* 0x01022E10 return -1 */
int    lseek(int fd, int offset, int whence);                 /* 0x01022E18 return -1 */
/*     0x01022E20: third `return -1` stub; identity unknown (ioctl?)                 */
void  *sbrk(int incr);                                        /* 0x01022E28 break @0x0103DAE0 */
int    getpid(void);                                          /* 0x01022ED8 return 1 */
int    fstat(int fd, struct stat *st);                        /* 0x01022EE0 */
int    isatty(int fd);                                        /* 0x01022EF8 return 1 */
int    kill(int pid, int sig);                                /* 0x01022F00 pid==1 -> Exit(sig) */
int    link(const char *oldpath, const char *newpath);        /* 0x01022F28 errno=EIO, -1 */
int    unlink(const char *path);                              /* 0x01022F50 errno=EIO, -1 */

#ifdef __cplusplus
}
#endif

#endif /* BOOT_LIBC_H */
