/* G007_stdio.h - the C runtime of the no-FDE stretch G007 (translation unit
 * G007, units u137 .. u142, 0x0027D7A0..0x0028E760, the end of .text):
 *   u137  newlib libc: stdlib (abort .. atof), the Sony EE MMI string
 *         routines, stdio (sscanf, wsetup, fflush, findfp), vfprintf, vfscanf
 *   u138  newlib: findfp/fread/fvwrite/fwalk/makebuf/refill/stdio.c,
 *         mallocr (dlmalloc 2.6, REENT build), locale, memchr, strtod, strtoul
 *   u139  newlib: strtoul, ungetc, reent syscall wrappers, mprec, _read_r,
 *         _realloc_r/_calloc_r; libkernl syscall stub table, initsys glue,
 *         cache/intr/alarm
 *   u140  libkernl 2240: thread.o, tty.o, SIO putc / kernel printf, sifcmd.o,
 *         sifrpc.o, fileio.o (first half)
 *   u141  libkernl 2240: fileio.o (second half), iopheap.o, loadfile.o,
 *         iopreboot.o, initsys.o, deci2.o (sceDeci2Open)
 *   u142  libkernl deci2 stubs, libgcc2 (__fixdfdi .. __umoddi3), libgraph
 *
 * A C header.  The EE's `long' is 64 bits (ee-gcc), so newlib's long /
 * unsigned long are written s64 / u64; int, size_t and pointers are 32 bits.
 *
 * The SDK headers (eekernel.h, sif.h, sifdev.h, libc.h, cxxrt.h, libgraph.h)
 * declare much of the same API with the BOOTSTRAP's addresses; the
 * declarations below are the same prototypes with the core addresses. */
#ifndef GAME_G007_STDIO_H
#define GAME_G007_STDIO_H

#include "game/game.h"
#include <stddef.h>             /* size_t (unsigned int on the EE) */
#include "sif.h"                /* sceSif* types of the libkernl SIF API (typedefs) */
#include "sifdev.h"             /* _fio_rpc_hdr (held by value in _fio_ioctl_arg), _fio_slot */

#ifdef __cplusplus
extern "C" {
#endif

struct _reent;
/* newlib struct stat as this EE libc fills it (sys/stat.h with the EE's
 * 64-bit `long'): fstat (u139) writes st_mode and st_blksize, __smakebuf
 * (u138) reads them; its frame holds one at sp+0 (0x68 bytes). */
struct stat {
    s16 st_dev;             /* 0x00 dev_t = short */
    u16 st_ino;             /* 0x02 */
    u32 st_mode;            /* 0x04 S_IFMT bits */
    u16 st_nlink;           /* 0x08 */
    u16 st_uid;             /* 0x0A */
    u16 st_gid;             /* 0x0C */
    s16 st_rdev;            /* 0x0E */
    s64 st_size;            /* 0x10 */
    s64 st_atime_;          /* 0x18 */
    s64 st_spare1;          /* 0x20 */
    s64 st_mtime_;          /* 0x28 */
    s64 st_spare2;          /* 0x30 */
    s64 st_ctime_;          /* 0x38 */
    s64 st_spare3;          /* 0x40 */
    s64 st_blksize;         /* 0x48 (the fstat stub 0x00286370 zeroes it) */
    s64 st_blocks;          /* 0x50 */
    s64 st_spare4[2];       /* 0x58 */
};
EGCS_SIZE(struct stat, 0x68);
struct ThreadParam;             /* eekernel.h */
struct SemaParam;               /* eekernel.h */

/* ======================================================================== */
/* newlib stdio (newlib 1.8 layout)                                         */
/* ======================================================================== */

/* stdio buffer (sys/reent.h). */
struct __sbuf {
    unsigned char *_base;       /* 0x00 */
    int            _size;       /* 0x04 */
};
EGCS_SIZE(struct __sbuf, 0x8);

/* newlib struct __sFILE.  The four callbacks get _cookie (= the FILE). */
typedef struct __sFILE {
    unsigned char *_p;          /* 0x00 current position in the buffer */
    int            _r;          /* 0x04 read space left for getc() */
    int            _w;          /* 0x08 write space left for putc() */
    short          _flags;      /* 0x0C __S* flags, 0 = free slot */
    short          _file;       /* 0x0E descriptor, or -1 */
    struct __sbuf  _bf;         /* 0x10 the buffer (at least 1 byte, if !NULL) */
    int            _lbfsize;    /* 0x18 0 or -_bf._size (line buffering) */
    void          *_cookie;     /* 0x1C argument of the four callbacks */
    int          (*_read)(void *cookie, char *buf, int n);          /* 0x20 __sread */
    int          (*_write)(void *cookie, const char *buf, int n);   /* 0x24 __swrite */
    s64          (*_seek)(void *cookie, s64 offset, int whence);    /* 0x28 __sseek (fpos_t = 64-bit long) */
    int          (*_close)(void *cookie);                           /* 0x2C __sclose */
    struct __sbuf  _ub;         /* 0x30 ungetc buffer */
    unsigned char *_up;         /* 0x38 saved _p while reading _ub */
    int            _ur;         /* 0x3C saved _r while reading _ub */
    unsigned char  _ubuf[3];    /* 0x40 reserve ungetc buffer */
    unsigned char  _nbuf[1];    /* 0x43 reserve getc buffer */
    struct __sbuf  _lb;         /* 0x44 fgetline buffer */
    int            _blksize;    /* 0x4C stat.st_blksize (may be != _bf._size) */
    int            _offset;     /* 0x50 current lseek offset */
    struct _reent *_data;       /* 0x54 owning reentrancy structure */
} FILE;
EGCS_SIZE(FILE, 0x58);

/* FILE._flags (newlib stdio.h) */
#define __SLBF  0x0001          /* line buffered */
#define __SNBF  0x0002          /* unbuffered */
#define __SRD   0x0004          /* OK to read */
#define __SWR   0x0008          /* OK to write */
#define __SRW   0x0010          /* open for reading & writing */
#define __SEOF  0x0020          /* found EOF */
#define __SERR  0x0040          /* found error */
#define __SMBF  0x0080          /* _bf._base is from malloc */
#define __SAPP  0x0100          /* fdopen()ed in append mode */
#define __SSTR  0x0200          /* this is an sprintf/snprintf string */
#define __SOPT  0x0400          /* do fseek() optimisation */
#define __SNPT  0x0800          /* do not do fseek() optimisation */
#define __SOFF  0x1000          /* set iff _offset is in fact correct */
#define __SMOD  0x2000          /* true => fgetline modified _p text */

/* I/O vectors of __sfvwrite (fvwrite.h). */
struct __siov {
    const void *iov_base;       /* 0x00 */
    size_t      iov_len;        /* 0x04 */
};
EGCS_SIZE(struct __siov, 0x8);

struct __suio {
    struct __siov *uio_iov;     /* 0x00 */
    int            uio_iovcnt;  /* 0x04 */
    int            uio_resid;   /* 0x08 bytes left to write */
};
EGCS_SIZE(struct __suio, 0xC);

/* findfp.c: a block of FILEs (__sfmoreglue puts the FILEs after it). */
struct _glue {
    struct _glue *_next;        /* 0x00 */
    int           _niobs;       /* 0x04 */
    FILE         *_iobs;        /* 0x08 */
};
EGCS_SIZE(struct _glue, 0xC);

/* atexit table: _reent._atexit0 first, then blocks from malloc. */
#define _ATEXIT_SIZE 32
struct _atexit {
    struct _atexit *_next;      /* 0x00 */
    int             _ind;       /* 0x04 next free index in _fns */
    void          (*_fns[_ATEXIT_SIZE])(void);  /* 0x08 */
};
EGCS_SIZE(struct _atexit, 0x88);

/* newlib struct _reent (impure_data @0x003072A0, through _impure_ptr).
 * 0x058..0x147 is newlib's union _new; only its first word, _rand_next of
 * the _reent arm, is used in this binary. */
struct _reent {
    int             _errno;             /* 0x000 */
    FILE           *_stdin;             /* 0x004 = &__sf[0] */
    FILE           *_stdout;            /* 0x008 = &__sf[1] */
    FILE           *_stderr;            /* 0x00C = &__sf[2] */
    int             _inc;               /* 0x010 tmpnam counter */
    char            _emergency[25];     /* 0x014 */
    int             _current_category;  /* 0x030 setlocale */
    const char     *_current_locale;    /* 0x034 */
    int             __sdidinit;         /* 0x038 __sinit has run */
    void          (*__cleanup)(struct _reent *);  /* 0x03C _cleanup_r once __sinit ran */
    struct _Bigint *_result;            /* 0x040 mprec */
    int             _result_k;          /* 0x044 */
    struct _Bigint *_p5s;               /* 0x048 cached 5^(4*2^n) powers (__pow5mult) */
    struct _Bigint **_freelist;         /* 0x04C Bigint free lists, 16 entries (_Balloc) */
    int             _cvtlen;            /* 0x050 fp conversion buffer (unused here) */
    char           *_cvtbuf;            /* 0x054 */
    unsigned int    _rand_next;         /* 0x058 _new._reent._rand_next (srand, rand) */
    u8              unk_05C[0xEC];      /* 0x05C rest of the _new union (unused here) */
    struct _atexit *_atexit;            /* 0x148 head of the atexit LIFO */
    struct _atexit  _atexit0;           /* 0x14C */
    void         (**_sig_func)(int);    /* 0x1D4 */
    struct _glue    __sglue;            /* 0x1D8 root of the FILE glue chain */
    FILE            __sf[3];            /* 0x1E4 stdin, stdout, stderr */
};
EGCS_SIZE(struct _reent, 0x2EC);

/* ======================================================================== */
/* newlib mprec (Bigint) and mallocr                                        */
/* ======================================================================== */

/* mprec.h Bigint (Pack_32: 32-bit words). */
struct _Bigint {
    struct _Bigint *_next;      /* 0x00 free-list link */
    int             _k;         /* 0x04 log2 of _maxwds */
    int             _maxwds;    /* 0x08 capacity in words (1 << _k) */
    int             _sign;      /* 0x0C */
    int             _wds;       /* 0x10 used words */
    u32             _x[1];      /* 0x14 little-endian words (really _maxwds) */
};
typedef struct _Bigint _Bigint;
EGCS_SIZE(struct _Bigint, 0x18);

/* dlmalloc 2.6 chunk header. */
struct malloc_chunk {
    u32                  prev_size; /* 0x00 size of the previous chunk (if free) */
    u32                  size;      /* 0x04 size in bytes | PREV_INUSE | IS_MMAPPED */
    struct malloc_chunk *fd;        /* 0x08 free-list links (free chunks only) */
    struct malloc_chunk *bk;        /* 0x0C */
};
EGCS_SIZE(struct malloc_chunk, 0x10);

/* ======================================================================== */
/* libkernl fileio.o                                                        */
/* ======================================================================== */

/* FIO_F_IOCTL / FIO_F_IOCTL2 request in _fio_cmd_buf (sceIoctl, sceIoctl2). */
struct _fio_ioctl_arg {
    _fio_rpc_hdr  h;            /* 0x000 */
    int           fd;           /* 0x00C IOP fd */
    int           req;          /* 0x010 */
    u8            arg[0x400];   /* 0x014 */
    void         *bufp;         /* 0x414 ioctl2 output buffer (0 for ioctl) */
    unsigned int  buflen;       /* 0x418 */
    unsigned int  arglen;       /* 0x41C */
};
EGCS_SIZE(struct _fio_ioctl_arg, 0x420);

/* Sizes of the request fields (both fileio halves, u140 and u141; ps2sdk's
 * FIO_PATH_MAX is 256, this IOP FILEIO takes 0x400). */
#define FIO_PATH_MAX      0x400     /* path fields (NUL included) */
#define FIO_DATA_MAX      0x400     /* ioctl / ioctl2 / devctl / format / mount data blocks */

/* libkernl's extended error codes, returned negated (ps2sdk loadfile.h has
 * SCE_EBINDMISS 0x10000, SCE_ECALLMISS 0x10001, SCE_ELOADMISS 0x10003):
 * fileio.o (_fio_init, sceOpen) and loadfile.o (u141) use these two. */
#define SCE_ECALLMISS     0x10001   /* the RPC call failed */
#define SCE_EVERSIONMISS  0x10004   /* ? name (not in ps2sdk): IOP module version mismatch */

/* ======================================================================== */
/* Globals                                                                  */
/* ======================================================================== */

extern struct _reent       *_impure_ptr;            /* 0x0030758C -> impure_data 0x003072A0 (both: u137's G007_stdlib) */
extern struct malloc_chunk *__malloc_av_[258];      /* 0x003075A8 mallocr av_[NAV * 2 + 2], the bins; av_[2] = top */
extern char                 _libkernl_version[16];  /* 0x00307A00 "PsIIlibkernl2240", no NUL; "2240" @0x00307A0C */
extern const double         __mprec_tens[25];       /* 0x00348FD0 mprec tens[]: 1e0 .. 1e24 */
extern const char           _ctype_[];              /* 0x00348730 newlib ctype table (1 + 256 bytes, u137's G007_stdlib:
                                                       indexed as _ctype_ + 1 + c; _U = 1, _L = 2, _N = 4, ...) */
/* The plain global errno of newlib's reent wrappers (_write_r ..): cleared
 * before the system call, copied into ptr->_errno after it (.sbss).  Not
 * spelled `errno': libc.h defines errno as (*__errno()). */
extern int                  sys_errno;              /* 0x00353520 */

/* ======================================================================== */
/* u137: stdlib, string, stdio                                              */
/* ======================================================================== */

/* stdlib.c */
void   abort(void) __attribute__((noreturn));                           /* 0x0027D7A0 */
void   __assert(const char *file, int line, const char *failedexpr);    /* 0x0027D7C0 */
int    atexit(void (*fn)(void));                                        /* 0x0027D7F8 */
int    atoi(const char *s);                                             /* 0x0027D890 */
int   *__errno(void);                                                   /* 0x0027D8B8 &_impure_ptr->_errno */
void   qsort(void *a, size_t n, size_t es, int (*cmp)(const void *, const void *)); /* 0x0027DBE8 */
int    rand(void);                                                      /* 0x0027E620 */

/* string.c (Sony EE MMI versions; strcasecmp, strstr and strupr
 * are plain newlib C) */
int    memcmp(const void *m1, const void *m2, size_t n);                /* 0x0027D8FC */
void  *memcpy(void *dst0, const void *src0, size_t len0);               /* 0x0027D990 */
void  *memmove(void *dst_void, const void *src_void, size_t length);    /* 0x0027DA3C */
void  *memset(void *m, int c, size_t n);                                /* 0x0027DB38 */
int    strcasecmp(const char *s1, const char *s2);                      /* 0x0027E6E0 */
char  *strcat(char *s1, const char *s2);                                /* 0x0027E794 */
char  *strchr(const char *s1, int i);                                   /* 0x0027E8B0 */
int    strcmp(const char *s1, const char *s2);                          /* 0x0027EA40 */
char  *strcpy(char *dst0, const char *src0);                            /* 0x0027EB84 */
int    strncmp(const char *s1, const char *s2, size_t n);               /* 0x0027EC98 */
char  *strncpy(char *dst0, const char *src0, size_t count);             /* 0x0027EE50 */
char  *strstr(const char *searchee, const char *lookfor);               /* 0x0027F008 */
char  *strupr(char *a);                                                 /* 0x0027F2E8 */

/* stdio_part1.c */
int    sscanf(const char *str, const char *fmt, ...);                   /* 0x0027E658 */
int    __swsetup(FILE *fp);                                             /* 0x00280C98 */
int    fflush(FILE *fp);                                                /* 0x00280DC8 */
#ifndef __cplusplus
/* findfp.c's static std(): a C++ translation unit cannot name it (std is
 * the namespace). */
void   std(FILE *fp, int flags, int file, struct _reent *ptr);          /* 0x00280ED0 */
#endif
void   _cleanup_r(struct _reent *ptr);                                  /* 0x00281078 */

/* ======================================================================== */
/* u138: stdio, mallocr, locale, memchr, strtod, strtoul                    */
/* ======================================================================== */

/* stdio_part2.c */
void   __sinit(struct _reent *s);                                       /* 0x00281098 */
size_t fread(void *buf, size_t size, size_t count, FILE *fp);           /* 0x00281128 */
int    __sfvwrite(FILE *fp, struct __suio *uio);                        /* 0x002816D8 */
int    _fwalk(struct _reent *ptr, int (*function)(FILE *));             /* 0x00281AB0 */
void   __smakebuf(FILE *fp);                                            /* 0x00281C38 */
int    __srefill(FILE *fp);                                             /* 0x002828A8 */
int    __sread(void *cookie, char *buf, int n);                         /* 0x00282A80 */
int    __swrite(void *cookie, const char *buf, int n);                  /* 0x00282AE8 */
s64    __sseek(void *cookie, s64 offset, int whence);                   /* 0x00282B68 */
int    __sclose(void *cookie);                                          /* 0x00282BD0 */

/* mallocr_part1.c */
void   _free_r(struct _reent *reent_ptr, void *mem);                    /* 0x00281248 */
void  *_malloc_r(struct _reent *reent_ptr, size_t bytes);               /* 0x00281FE0 */
void   __malloc_lock(struct _reent *ptr);                               /* 0x00282878 (empty) */
void   __malloc_unlock(struct _reent *ptr);                             /* 0x00282880 (empty) */

/* locale.c, memchr.c */
int    _mbtowc_r(struct _reent *r, int *pwc, const char *s, size_t n, int *state); /* 0x00282758 */
void  *memchr(const void *src, int c, size_t length);                   /* 0x00282794 (MMI) */

/* strtod.c, strtoul_r.c */
double strtod(const char *s00, char **se);                              /* 0x00283AD8 */
u64    _strtoul_r(struct _reent *rptr, const char *nptr, char **endptr, int base); /* 0x00283B30 */

/* ======================================================================== */
/* u139: newlib strtoul .. mprec, libkernl syscalls, initsys glue, kernel   */
/* ======================================================================== */

/* strtoul.c, ungetc.c.  strtoul returns the EE's 64-bit
 * unsigned long; GCC 14 (n32, 32-bit long) would call that a builtin
 * mismatch. */
#ifndef BOOT_OLD_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif
u64    strtoul(const char *nptr, char **endptr, int base);              /* 0x00283D50 */
#ifndef BOOT_OLD_GCC
#pragma GCC diagnostic pop
#endif
int    ungetc(int c, FILE *fp);                                         /* 0x00283E78 */

/* syscalls_r.c, readr.c: errno-forwarding wrappers.
 * _write_r / _read_r return newlib's `long' and _lseek_r takes and returns
 * `off_t' (= long): 64-bit on the EE.  The callers prove it: __sread and
 * __swrite truncate the returned value to int (dsll32 / dsra32 right after
 * the jal), and __sseek passes its 64-bit fpos_t offset on with a plain move
 * (no truncation to int). */
s64    _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt); /* 0x00284000 */
int    _close_r(struct _reent *ptr, int fd);                            /* 0x00284060 */
int    _fstat_r(struct _reent *ptr, int fd, struct stat *st);           /* 0x002840B8 */
s64    _lseek_r(struct _reent *ptr, int fd, s64 pos, int whence);       /* 0x00284118 */
s64    _read_r(struct _reent *ptr, int fd, void *buf, size_t cnt);      /* 0x00285198 */

/* mprec.c */
_Bigint *_Balloc(struct _reent *ptr, int k);                            /* 0x00284178 */
void     _Bfree(struct _reent *ptr, _Bigint *v);                        /* 0x00284220 */
_Bigint *__s2b(struct _reent *ptr, const char *s, int nd0, int nd, u32 y9); /* 0x00284368 */
_Bigint *__i2b(struct _reent *ptr, int i);                              /* 0x002845F8 */
_Bigint *__multiply(struct _reent *ptr, _Bigint *a, _Bigint *b);        /* 0x00284630 */
_Bigint *__pow5mult(struct _reent *ptr, _Bigint *b, int k);             /* 0x00284860 */
_Bigint *__lshift(struct _reent *ptr, _Bigint *b, int k);               /* 0x00284960 */
int      __mcmp(_Bigint *a, _Bigint *b);                                /* 0x00284AD8 */
_Bigint *__mdiff(struct _reent *ptr, _Bigint *a, _Bigint *b);           /* 0x00284B40 */
double   __ulp(double _x);                                              /* 0x00284CD0 */
_Bigint *__d2b(struct _reent *ptr, double _d, int *e, int *bits);       /* 0x00284EE8 */
double   __ratio(_Bigint *a, _Bigint *b);                               /* 0x00285068 */

/* syscalls.S: EE kernel syscall stubs (prototypes as eekernel.h /
 * sif.h; the number is the syscall). */
int    AddIntcHandler(int cause, int (*handler)(int cause), int next);  /* 0x00285940 #0x10 */
int    AddIntcHandler2(int cause, int (*handler)(int cause, void *arg, void *addr), int next, void *arg); /* 0x00285950 #0x10 */
int    RemoveIntcHandler(int cause, int handler_id);                    /* 0x00285960 #0x11 */
int    AddDmacHandler(int channel, int (*handler)(int channel), int next); /* 0x00285970 #0x12 */
int    AddDmacHandler2(int channel, int (*handler)(int channel, void *arg, void *addr), int next, void *arg); /* 0x00285980 #0x12 */
int    RemoveDmacHandler(int channel, int handler_id);                  /* 0x00285990 #0x13 */
int    SetAlarm(u16 time, void (*callback)(int alarm_id, u16 time, void *arg), void *arg); /* 0x002859E0 #0xFC */
int    CreateThread(struct ThreadParam *param);                         /* 0x00285A60 #0x20 */
int    DeleteThread(int tid);                                           /* 0x00285A70 #0x21 */
int    StartThread(int tid, void *arg);                                 /* 0x00285A80 #0x22 */
void   ExitDeleteThread(void);                                          /* 0x00285AA0 #0x24 */
int    TerminateThread(int tid);                                        /* 0x00285AB0 #0x25 */
int    ChangeThreadPriority(int tid, int priority);                     /* 0x00285AF0 #0x29 */
int    RotateThreadReadyQueue(int priority);                            /* 0x00285B10 #0x2B */
int    GetThreadId(void);                                               /* 0x00285B50 #0x2F */
int    ReferThreadStatus(int tid, struct ThreadParam *info);            /* 0x00285B60 #0x30 */
int    SleepThread(void);                                               /* 0x00285B80 #0x32 */
int    WakeupThread(int tid);                                           /* 0x00285B90 #0x33 */
int    _iWakeupThread(int tid);                                         /* 0x00285BA0 #-0x34 */
int    CancelWakeupThread(int tid);                                     /* 0x00285BB0 #0x35 */
int    SuspendThread(int tid);                                          /* 0x00285BD0 #0x37 */
int    _iSuspendThread(int tid);                                        /* 0x00285BE0 #-0x38 */
int    CreateSema(struct SemaParam *param);                             /* 0x00285C60 #0x40 */
int    DeleteSema(int sid);                                             /* 0x00285C70 #0x41 */
int    SignalSema(int sid);                                             /* 0x00285C80 #0x42 */
int    iSignalSema(int sid);                                            /* 0x00285C90 #-0x43 */
int    WaitSema(int sid);                                               /* 0x00285CA0 #0x44 */
int    PollSema(int sid);                                               /* 0x00285CB0 #0x45 */
int    iPollSema(int sid);                                              /* 0x00285CC0 #-0x46 */
void   GetOsdConfigParam(void *config);                                 /* 0x00285D10 #0x4B */
void   FlushCache(int mode);                                            /* 0x00285EE0 #0x64 */
void   sceSifStopDma(void);                                             /* 0x00285F30 #0x6B */
int    GetOsdConfigParam2(void *config, int size, int offset);          /* 0x00285F70 #0x6F (int: u130's libscf getters) */
u64    GsPutIMR(u64 imr);                                               /* 0x00285FA0 #0x71 */
int    sceSifDmaStat(unsigned int id);                                  /* 0x00286000 #0x76 (<0 = done) */
unsigned int sceSifSetDma(sceSifDmaData *sdd, int len);                 /* 0x00286020 #0x77 */
unsigned int isceSifSetDma(sceSifDmaData *sdd, int len);                /* 0x00286030 #-0x77 */
int    sceSifSetDChain(void);                                           /* 0x00286040 #0x78 */
int    isceSifSetDChain(void);                                          /* 0x00286050 #-0x78 (int: u140's _sif_cmd_intr_handler) */
int    sceSifSetReg(unsigned int reg, unsigned int value);              /* 0x00286060 #0x79 */
int    sceSifGetReg(unsigned int reg);                                  /* 0x00286070 #0x7A */
int    Deci2Call(int func, u32 *param);                                 /* 0x00286090 #0x7C */

/* initsys_glue.c: TTY / vblank helpers and the newlib POSIX glue. */
void   tty_reinit_after_iop_reset(void);                                /* 0x002860D0 */
void   wait_vblank_start(void);                                         /* 0x002860E0 */
void  *sbrk(int incr);                                                  /* 0x002862B8 */
int    isatty(int fd);                                                  /* 0x00286368 returns 1 */

/* kernel.c: cache.o, intr.o, alarm.o */
void   SyncDCache(void *start, void *end);                              /* 0x002864B0 */
int    DisableIntc(int cause);                                          /* 0x00286688 */
int    EnableIntc(int cause);                                           /* 0x002866F0 */
int    DisableDmac(int channel);                                        /* 0x00286758 */
int    EnableDmac(int channel);                                         /* 0x002867C0 */
int    iDisableIntc(int cause);                                         /* 0x00286848 */
int    _kCopy_handler(u32 *dst, const u32 *src, int nbytes);            /* 0x002868C8 */
void   InitAlarm(void);                                                 /* 0x00286910 */

/* ======================================================================== */
/* u140: libkernl thread.o, tty.o, kprintf, sifcmd.o, sifrpc.o, fileio.o    */
/* ======================================================================== */

/* thread.c */
int    InitThread(void);                                                /* 0x00286AC0 (_InitSys tail-jumps here) */
int    iWakeupThread(int tid);                                          /* 0x00286B98 */

/* tty.c: DECI2 TTYP console driver */
int    _deci2_tty_write(const char *buf, int len);                      /* 0x00286F88 */
int    _deci2_tty_read(char *buf, int len);                             /* 0x002870D8 */
int    _deci2_tty_init(void);                                           /* 0x002871A8 1 ok, 0 sceDeci2Open failed */

/* kprintf.c */
void   sio_putchar_crlf(int c);                                         /* 0x00287350 */
int    sce_printf_deci2(const char *fmt, ...);                          /* 0x00287B80 (the bootstrap's scePrintf) */

/* sifcmd.c */
void   sceSifAddCmdHandler(unsigned int fcode, sceSifCmdHandler func, void *data); /* 0x00287F40 */
void   sceSifRemoveCmdHandler(unsigned int fcode);                      /* 0x00287F70 */
void   sceSifWriteBackDCache(void *ptr, int size);                      /* 0x00288278 */

/* sifrpc.c */
void   sceSifInitRpc(unsigned int mode);                                /* 0x00288328 */
void   sceSifExitRpc(void);                                             /* 0x002884C8 */
int    sceSifBindRpc(sceSifClientData *cd, unsigned int command, unsigned int mode); /* 0x00288998 */
int    sceSifCallRpc(sceSifClientData *cd, unsigned int fno, unsigned int mode,
                     void *send, int ssize, void *receive, int rsize,
                     sceSifEndFunc end_func, void *end_para);           /* 0x00288B68 */
int    sceSifCheckStatRpc(sceSifRpcData *cd);                           /* 0x00288D58 */
void   sceSifSetRpcQueue(sceSifQueueData *qd, int thread_id);           /* 0x00288D98 */
void   sceSifRegisterRpc(sceSifServeData *sd, unsigned int command,
                         sceSifRpcFunc func, void *buff,
                         sceSifRpcFunc cfunc, void *cbuff,
                         sceSifQueueData *qd);                          /* 0x00288E30 */
void   sceSifRpcLoop(sceSifQueueData *qd);                              /* 0x00289248 never returns; the two
                                                                           EE RPC server threads tail-jump here */

/* fileio_part1.c */
_fio_slot *_fio_fd_alloc(void);                                         /* 0x002892E8 NULL if full */
_fio_slot *_fio_fd_to_slot(int fd);                                     /* 0x00289370 NULL if fd >= 32 */
int    _fio_lock(int op);                                               /* 0x002897E8 */
void   _fio_unlock(void);                                               /* 0x00289818 */
int    _fio_init(void);                                                 /* 0x00289828 */
int    sceFsReset(void);                                                /* 0x00289A58 */
int    sceOpen(const char *filename, int flag, ...);                    /* 0x00289A90 */
int    sceClose(int fd);                                                /* 0x00289D18 */
int    sceLseek(int fd, int offset, int whence);                        /* 0x00289E98 */
int    sceRead(int fd, void *buf, int nbyte);                           /* 0x0028A0D0 */
int    sceWrite(int fd, const void *buf, int nbyte);                    /* 0x0028A340 */

/* fileio.o's state, shared by both halves (u140, u141).  sifdev.h declares
 * the same objects (same types) with the bootstrap's addresses. */
extern volatile int     _fio_pending[32];   /* 0x003081D0 .data SCE_NOWAIT completion semas, -1 = free */
extern int              _fio_initialized;   /* 0x00308250 .data _fio_init done */
extern int              _fio_lock_sema;     /* 0x00308254 .data -1 until created (_fio_lock) */
extern int              _fio_fd_sema;       /* 0x00308258 .data -1 until created (fd table) */
extern int              _fio_pending_sema;  /* 0x0030825C .data -1 until created (_fio_pending) */
extern u8               _fio_cmd_buf[0xC40]; /* 0x004AF9C0 .bss RPC send buffer (64-aligned): one
                                               request, _fio_rpc_hdr first */
extern int              _fio_recv_buf[0x10]; /* 0x004B0600 .bss RPC receive buffer: word 0 (read
                                                uncached) = IOP accept flag */
extern _fio_slot        _fio_slots[32];     /* 0x004B0A80 .bss EE fd table */
extern sceSifClientData _fio_client;        /* 0x004B0C80 .bss bound to the IOP FILEIO server 0x80000001 */

/* ======================================================================== */
/* u141: libkernl iopheap.o, loadfile.o, iopreboot.o, initsys.o, deci2.o    */
/* ======================================================================== */

int    sceSifInitIopHeap(void);                                         /* 0x0028C7F8 */
int    sceSifLoadFileReset(void);                                       /* 0x0028CBE8 */
int    sceSifLoadModule(const char *filename, int args, const char *argp); /* 0x0028D090 */
int    sceSifSyncIop(void);                                             /* 0x0028D548 */
int    sceSifRebootIop(const char *img);                                /* 0x0028D590 */
int    DIntr(void);                                                     /* 0x0028D6A0 1 if interrupts were enabled */
int    EIntr(void);                                                     /* 0x0028D6E8 */
void   _InitSys(void);                                                  /* 0x0028D750 */
int    sceDeci2Open(unsigned short protocol, void *opt,
                    void (*handler)(int event, int param, void *opt));  /* 0x0028D778 */

/* ======================================================================== */
/* u142: libkernl deci2 stubs, libgcc2, libgraph                            */
/* ======================================================================== */

/* deci2_part2.c */
int    sceDeci2ReqSend(int s, char dest);                               /* 0x0028D7E8 */
int    sceDeci2Poll(int s);                                             /* 0x0028D818 */
int    sceDeci2ExRecv(int s, void *buf, unsigned short len);            /* 0x0028D840 */
int    sceDeci2ExSend(int s, void *buf, unsigned short len);            /* 0x0028D878 */
void   deci2_write(const char *s);                                      /* 0x0028D930 Deci2Call(0x10, {s}) (the bootstrap's kPuts) */

/* libgcc2_part3.c (DWtype = s64, UDWtype = u64, DFtype = double) */
s64    __fixdfdi(double a);                                             /* 0x0028D958 */
u64    __fixunsdfdi(double a);                                          /* 0x0028D9B8 */
u64    __udivdi3(u64 n, u64 d);                                         /* 0x0028DAA8 */
u64    __umoddi3(u64 u, u64 v);                                         /* 0x0028E078 */

/* libgraph.c */
void   sceGsResetGraph(short mode, short inter, short omode, short ffmode); /* 0x0028E5D0 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_G007_STDIO_H */
