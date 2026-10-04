/* fileio.c - Sony libkernl 2240 (PsIIlibkernl2240) fileio.o, first part:
 * the EE client of the IOP FILEIO RPC server (0x80000001), 0x0101E9B0..
 * 0x01020977 of BOOTSTRAP_SCUS-97102.  The rest of fileio.o (sceDelDrv,
 * sceDopen .. sceReadlink, 0x01020978..) is the kernl_rest unit's
 * sdk/libkernl/filedev.c, which shares the state defined here (sifdev.h).
 *
 * Protocol, as seen in the binary:
 *  - every entry point takes the global fileio lock (_fio_lock / _fio_unlock,
 *    sema @0x0103DAC4), lazily runs _fio_init, builds its request in the
 *    single send buffer _fio_cmd_buf (@0x0104C300) and sceSifCallRpc()s it
 *    (blocking, mode 0) with a 4-byte receive buffer _fio_recv_buf (@0x0104CF40)
 *    whose first word (read uncached) says whether the IOP accepted it;
 *  - the request header carries a fresh per-call semaphore and the address
 *    of an int on the caller's stack; when the IOP finishes it DMAs a
 *    completion block to _fio_intr_buf (@0x0104CF80, address handed over by
 *    the FIO_F_SETUP call) and raises SIF command 0x80000011, whose handler
 *    _fio_intr copies the result to that int and iSignalSema()s the caller;
 *  - the lock is dropped as soon as the request is accepted, before waiting;
 *  - files opened with SCE_NOWAIT: lseek/read/write record the (negated)
 *    sema id in the pending table _fio_pending and return 0 immediately; the
 *    handler just clears the table entry.  sceIoctl(fd, 1..3) polls that.
 *  - EE file descriptors are indices into the 32-slot table _fio_slots.
 *
 * Boot-chain relevance: sceOpen/sceLseek/sceRead/sceClose are the "host:"
 * (and ImageLoader Host/DiskSource) file path; there is no integrity check
 * in this layer.  sceOpen refuses to run (-0x10004) when the IOP FILEIO
 * module's 4-byte version is neither "2240" nor "....".
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "sif.h"
#include "sifdev.h"

#ifndef E2BIG
#define E2BIG 7
#endif

/* "2240": the last four characters of libkernl's 16-byte, NUL-less version
 * tag "PsIIlibkernl2240" (@0x0103D278); both fileio and loadfile compare
 * the IOP module version against 0x0103D284 directly.
 * Defined by kernl_sys in sdk/libkernl/kernel.c (no header declares it yet;
 * loadfile.c carries the same local extern).                               */
extern char _libkernl_version[16];                    /* @0x0103D278 */
#define FIO_LIB_VERSION   (&_libkernl_version[12])    /* @0x0103D284 "2240" */

/* ======================================================================== */
/* Request layouts in _fio_cmd_buf (all start with _fio_rpc_hdr, sifdev.h)      */
/* ======================================================================== */

struct _fio_open_arg {              /* FIO_F_OPEN, 0x418 bytes */
    _fio_rpc_hdr  h;                /* +0x000 */
    int           flag;             /* +0x00C flag & 0x0FFFFFFF (EE-only bits stripped) */
    int           mode;             /* +0x010 */
    char          path[0x400];      /* +0x014 */
    int           efd;              /* +0x414 EE fd (slot index) */
};

struct _fio_close_arg {             /* FIO_F_CLOSE, 0x14 bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x0C IOP fd */
    int           efd;              /* +0x10 */
};

struct _fio_read_arg {              /* FIO_F_READ, 0x20 bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x0C */
    void         *buf;              /* +0x10 */
    int           nbyte;            /* +0x14 */
    int           _18;              /* +0x18 not written by sceRead */
    int           efd;              /* +0x1C */
};

struct _fio_write_arg {             /* FIO_F_WRITE, 0x30 bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x0C */
    const void   *buf;              /* +0x10 */
    int           nbyte;            /* +0x14 */
    int           head_len;         /* +0x18 bytes up to the next 16-byte boundary */
    u8            head[16];         /* +0x1C those bytes, sent inline */
    int           efd;              /* +0x2C */
};

struct _fio_lseek_arg {             /* FIO_F_LSEEK, 0x1C bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x0C */
    int           offset;           /* +0x10 */
    int           whence;           /* +0x14 */
    int           efd;              /* +0x18 */
};

struct _fio_ioctl_arg {             /* FIO_F_IOCTL / FIO_F_IOCTL2, 0x420 bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x00C */
    int           req;              /* +0x010 */
    u8            arg[0x400];       /* +0x014 */
    void         *bufp;             /* +0x414 ioctl2 output buffer (0 for ioctl) */
    unsigned int  buflen;           /* +0x418 */
    unsigned int  arglen;           /* +0x41C */
};

struct _fio_path_arg {              /* single-path ops, 0x0D + strlen bytes */
    _fio_rpc_hdr  h;
    char          path[0x400];      /* +0x00C */
};

struct _fio_mkdir_arg {             /* FIO_F_MKDIR, 0x11 + strlen bytes */
    _fio_rpc_hdr  h;
    int           mode;             /* +0x00C */
    char          path[0x400];      /* +0x010 */
};

struct _fio_format_arg {            /* FIO_F_FORMAT, 0xC10 bytes */
    _fio_rpc_hdr  h;
    char          path[0x400];      /* +0x00C */
    char          blkdev[0x400];    /* +0x40C */
    u8            arg[0x400];       /* +0x80C */
    int           arglen;           /* +0xC0C */
};

struct _fio_adddrv_arg {            /* FIO_F_ADDDRV, 0x10 bytes */
    _fio_rpc_hdr  h;
    int           arg;              /* +0x0C */
};

_Static_assert(sizeof(struct _fio_open_arg)   == 0x418, "open arg");
_Static_assert(sizeof(struct _fio_close_arg)  == 0x14,  "close arg");
_Static_assert(sizeof(struct _fio_read_arg)   == 0x20,  "read arg");
_Static_assert(sizeof(struct _fio_write_arg)  == 0x30,  "write arg");
_Static_assert(sizeof(struct _fio_lseek_arg)  == 0x1C,  "lseek arg");
_Static_assert(sizeof(struct _fio_ioctl_arg)  == 0x420, "ioctl arg");
_Static_assert(sizeof(struct _fio_format_arg) == 0xC10, "format arg");
_Static_assert(sizeof(struct _fio_adddrv_arg) == 0x10,  "adddrv arg");

/* ======================================================================== */
/* Completion block written by the IOP (read only through |0x20000000)       */
/* ======================================================================== */

struct _fio_intr_hdr {
    int           sema;             /* +0x00 request's hdr.sema (negative = SCE_NOWAIT op) */
    int           fno;              /* +0x04 */
    void         *dest;             /* +0x08 request's hdr.result */
    int           size;             /* +0x0C request's hdr.result_size */
};

struct _fio_read_fixup {            /* FIO_F_READ: unaligned head/tail bytes */
    int           head_len;         /* +0x00 */
    int           tail_len;         /* +0x04 */
    u8           *head_dst;         /* +0x08 */
    u8           *tail_dst;         /* +0x0C */
    u8            head[0x40];       /* +0x10 */
    u8            tail[0x40];       /* +0x50 */
};

struct _fio_intr_data {
    struct _fio_intr_hdr hdr;       /* +0x000 @0x0104CF80 */
    int           result;           /* +0x010 @0x0104CF90 result (64-bit for sceIoctl 3) */
    union {                         /* +0x014 @0x0104CF94 per-op extra data */
        struct _fio_read_fixup read;                                   /* fno 2 */
        struct { struct sce_dirent *dst; struct sce_dirent ent; } dread;  /* fno 11 */
        struct { struct sce_stat *dst; struct sce_stat st; } getstat;   /* fno 12 */
        struct { void *dst; unsigned int len; u8 data[0x400]; } buf;     /* fno 23/25/26 */
    } u;
};

_Static_assert(offsetof(struct _fio_intr_data, u.read.tail) == 0x14 + 0x50, "read fixup");
_Static_assert(offsetof(struct _fio_intr_data, u.dread.ent) == 0x18, "dread ent");
_Static_assert(offsetof(struct _fio_intr_data, u.buf.data) == 0x1C, "buf data");

typedef struct { u8   b[0x400]; } _fio_blk400;    /* sceIoctl's inline 1 KB block copy */
typedef struct { char v[4]; }     _fio_version_t;  /* copied with lwl/lwr (4-byte inline copy) */

/* ======================================================================== */
/* Data                                                                     */
/* ======================================================================== */

/* .data 0x0103DA40..0x0103DAD3 */
volatile int _fio_pending[32] = { [0 ... 31] = -1 };          /* @0x0103DA40 SCE_NOWAIT ops in flight */
int _fio_initialized = 0;                            /* @0x0103DAC0 */
int _fio_lock_sema = -1;                             /* @0x0103DAC4 serialises all requests */
int _fio_fd_sema = -1;                               /* @0x0103DAC8 guards _fio_slots */
int _fio_pending_sema = -1;                          /* @0x0103DACC guards _fio_pending */
static const char *_fio_alt_version = "....";        /* @0x0103DAD0 -> "...." @0x01040708 */

/* .bss */
static void *_fio_ioctl_argp;                        /* @0x0104C2C0 sceIoctl's argp */
u8           _fio_cmd_buf[0xC40] __attribute__((aligned(64)));  /* @0x0104C300 RPC send buffer */
int          _fio_recv_buf[0x10] __attribute__((aligned(64)));  /* @0x0104CF40 RPC receive buffer */
static union {
    struct _fio_intr_data d;
    u8                    raw[0x440];
} _fio_intr_buf __attribute__((aligned(64)));        /* @0x0104CF80 completion block */
_fio_slot    _fio_slots[32];                         /* @0x0104D3C0 EE fd table */
sceSifClientData _fio_client;                        /* @0x0104D5C0 */
_fio_version_t _fio_iop_version;                     /* @0x0104D5E8 IOP FILEIO version */
static u8    _fio_intr_harg[0x10];                   /* @0x0104D5F0 ? only its address is used
                                                        (cmd-handler arg, ignored by _fio_intr);
                                                        size unknown */

/* Request header at the start of _fio_cmd_buf. */
#define FIO_HDR         ((_fio_rpc_hdr *)_fio_cmd_buf)
/* Uncached read of the RPC receive buffer's first word (IOP accept flag).
 * A plain load, as in the binary (it sits in the delay slot of the
 * `jal _fio_unlock` that follows, where egcs never puts a volatile access).
 * It is always read between the sceSifCallRpc() and _fio_unlock() calls,
 * which no compiler can move it across or elide it past, so it is still
 * exactly one load per request (GCC 14's code is the same as with volatile). */
#define FIO_RPC_ACK()   (*(int *)UNCACHED(_fio_recv_buf))
/* Copy one object out of the completion block through its uncached mirror.
 * The source is a plain (void *) address, so egcs inlines the fixed-size
 * memcpy with unaligned lwl/lwr (ldl/ldr) moves, as the binary does for
 * every field _fio_intr reads: no memcpy call.  The GCC 14 build compiles
 * with -fno-builtin, where the same memcpy would become a real call (one per
 * field, from interrupt context); it loads the field directly instead, the
 * same 4-byte load the binary's lwl/lwr pair makes (every field read this
 * way is naturally aligned in the 64-byte aligned block).                   */
#ifdef BOOT_OLD_GCC
#define FIO_UREAD(dst, src) memcpy(&(dst), UNCACHED(&(src)), sizeof(dst))
#else
#define FIO_UREAD(dst, src) ((dst) = *(__typeof__(src) *)UNCACHED(&(src)))
#endif

/* Sony's nopdelay(), inlined in _fio_init's bind retry loop
 * (0x0101EFA8..0x0101EFC8): i = 0x100000; loop { i--; } while (i != -1),
 * i.e. 0x100001 passes.  The loop is empty in the source: the four nops in
 * the binary are the old compiler's padding of a short loop (R5900
 * short-loop erratum; same helper as libcdvd's _sceCdNopDelay).  The GCC 14
 * build writes the nops out, so each pass costs what it did before, and the
 * asm keeps GCC 14 from deleting the loop.                                  */
#ifdef BOOT_OLD_GCC
#define FIO_NOPDELAY_PASS()  __asm__ __volatile__("")
#else
#define FIO_NOPDELAY_PASS()  __asm__ __volatile__("nop\n\tnop\n\tnop\n\tnop")
#endif
static __inline__ void _fio_nopdelay(void)
{
    int i = 0x100000;

    do
        FIO_NOPDELAY_PASS();
    while (i-- != 0);
}

/* Inlined identically into sceLseek, sceRead and sceWrite (and kernl_rest's
 * sceLseek64) for SCE_NOWAIT files: record the request's completion sema in
 * the first free _fio_pending slot and negate it in the request so _fio_intr
 * treats the completion as asynchronous.  If all 32 slots are busy nothing
 * is recorded and the id stays positive (the caller still returns without
 * waiting and deletes the sema; _fio_intr will later iSignalSema that stale
 * id).  A macro, not a static inline function: egcs never inlines a void
 * function that ends in a call (a tail-call candidate; -Winline: "function
 * with sibling calls cannot be inlined").  A plain braced block, not
 * do { } while (0): egcs treats the latter as a loop and 8-byte aligns its
 * head (padding nop).  Only used as `if (...) _fio_pending_add();`.        */
#define _fio_pending_add()                                  \
    {                                                       \
        int _pi;                                            \
                                                            \
        WaitSema(_fio_pending_sema);                        \
        for (_pi = 0; _pi < 32; _pi++) {                    \
            if (_fio_pending[_pi] == -1) {                  \
                _fio_pending[_pi] = arg->h.sema;            \
                arg->h.sema = -arg->h.sema;                 \
                break;                                      \
            }                                               \
        }                                                   \
        SignalSema(_fio_pending_sema);                      \
    }

/* ======================================================================== */
/* Internal helpers                                                         */
/* ======================================================================== */

// FUNC 0x0101E9B0 _fio_fd_sema_init
// One-shot: if the fd-table sema does not exist yet, create it and the
// pending-table sema (both binary: max 1, init 1, option 0).
static void _fio_fd_sema_init(void)
{
    struct SemaParam sp;

    if (_fio_fd_sema == -1) {
        sp.initCount = 1;
        sp.maxCount  = 1;
        sp.option    = 0;
        _fio_fd_sema      = CreateSema(&sp);
        _fio_pending_sema = CreateSema(&sp);
    }
}

// FUNC 0x0101EA10 _fio_fd_alloc
// Reserve the first free fd slot (mode == 0) by setting its mode to
// FIO_SLOT_RESERVED; NULL if all 32 are in use.  Callers: sceOpen,
// sceDopen (kernl_rest).
_fio_slot *_fio_fd_alloc(void)
{
    _fio_slot *s;

    _fio_fd_sema_init();
    WaitSema(_fio_fd_sema);
    for (s = _fio_slots; s < &_fio_slots[32]; s++) {
        if (s->mode == 0) {
            s->mode = FIO_SLOT_RESERVED;
            SignalSema(_fio_fd_sema);
            return s;
        }
    }
    SignalSema(_fio_fd_sema);
    return NULL;
}

// FUNC 0x0101EA98 _fio_fd_to_slot
// &_fio_slots[fd], or NULL if fd is out of range (unsigned compare, so
// negative fds fail too).  Does not check that the slot is open; callers
// test slot->mode.  The sema round-trip protects nothing but is in the code.
_fio_slot *_fio_fd_to_slot(int fd)
{
    _fio_slot *s;

    _fio_fd_sema_init();
    WaitSema(_fio_fd_sema);
    if ((unsigned int)fd >= 32) {
        SignalSema(_fio_fd_sema);
        return NULL;
    }
    s = &_fio_slots[fd];
    SignalSema(_fio_fd_sema);
    return s;
}

// FUNC 0x0101EB08 _fio_intr
// SIF command 0x80000011 (FILEIO completion) handler, installed by _fio_init;
// interrupt context.  Both arguments are ignored: everything is read from the
// completion block through its uncached mirror.  Copies hdr.size bytes of
// result (from +0x10) to hdr.dest (memcpy call), then per function number
// (jump table @0x010406A0 indexed by fno-2, 25 entries):
//   2  (read)          patch the unaligned head/tail bytes into the user buffer
//   11 (dread)         copy the 0x144-byte sce_dirent to the caller's buffer
//   12 (getstat)       copy the 0x40-byte sce_stat
//   23/25/26 (devctl/readlink/ioctl2)  copy up to 0x400 output bytes
//   others             nothing
// Finally a non-negative sema is iSignalSema'd (blocking caller); a negative
// one is an SCE_NOWAIT op whose entry is cleared from _fio_pending.
// (The function detector merged this into _fio_fd_to_slot's tail.)
static void _fio_intr(void *pkt, void *harg)
{
    /* the completion header, field by field (stack, in this order) */
    int          sema;
    int          fno;
    void        *dest;
    int          size;
    /* per-op output pointer / length (shared by the cases below) */
    void        *dst;
    unsigned int len;
    struct _fio_read_fixup *rd;
    u8          *p;
    int          i;

    FIO_UREAD(sema, _fio_intr_buf.d.hdr.sema);
    FIO_UREAD(fno,  _fio_intr_buf.d.hdr.fno);
    FIO_UREAD(dest, _fio_intr_buf.d.hdr.dest);
    FIO_UREAD(size, _fio_intr_buf.d.hdr.size);
    memcpy(dest, UNCACHED(&_fio_intr_buf.d.result), size);

    switch (fno) {
    case FIO_F_READ:
        rd = (struct _fio_read_fixup *)UNCACHED(&_fio_intr_buf.d.u.read);
        if (rd->head_len > 0) {
            p = rd->head_dst;
            for (i = 0; i < rd->head_len; i++)
                p[i] = rd->head[i];
        }
        if (rd->tail_len > 0) {
            p = rd->tail_dst;
            for (i = 0; i < rd->tail_len; i++)
                p[i] = rd->tail[i];
        }
        break;

    case FIO_F_DREAD:
        FIO_UREAD(dst, _fio_intr_buf.d.u.dread.dst);
        memcpy(dst, UNCACHED(&_fio_intr_buf.d.u.dread.ent), sizeof(struct sce_dirent));
        break;

    case FIO_F_GETSTAT:
        FIO_UREAD(dst, _fio_intr_buf.d.u.getstat.dst);
        memcpy(dst, UNCACHED(&_fio_intr_buf.d.u.getstat.st), sizeof(struct sce_stat));
        break;

    case FIO_F_DEVCTL:
    case FIO_F_READLINK:
    case FIO_F_IOCTL2:
        FIO_UREAD(dst, _fio_intr_buf.d.u.buf.dst);
        FIO_UREAD(len, _fio_intr_buf.d.u.buf.len);
        if (len > 0x400)
            len = 0x400;
        memcpy(dst, UNCACHED(&_fio_intr_buf.d.u.buf.data), len);
        break;

    default:
        break;
    }

    if (sema < 0) {
        sema = -sema;
        for (i = 0; i < 32; i++) {
            if (_fio_pending[i] == sema) {
                _fio_pending[i] = -1;
                break;
            }
        }
    } else {
        iSignalSema(sema);
    }
}

// FUNC 0x0101EEC0 _fio_lock_sema_init
// One-shot creation of the request-serialising sema (max 1, init 1).
static void _fio_lock_sema_init(void)
{
    struct SemaParam sp;

    if (_fio_lock_sema == -1) {
        sp.initCount = 1;
        sp.maxCount  = 1;
        sp.option    = 0;
        _fio_lock_sema = CreateSema(&sp);
    }
}

// FUNC 0x0101EF10 _fio_lock
// Take the fileio lock.  `op` (the caller's FIO_F_* number) is ignored in
// this build.  Always returns 0.
int _fio_lock(int op)
{
    _fio_lock_sema_init();
    WaitSema(_fio_lock_sema);
    return 0;
}

// FUNC 0x0101EF40 _fio_unlock
// Release the fileio lock (tail call of SignalSema, so $v0 happens to hold
// SignalSema's result; no caller uses it).  void: this compiler only turns a
// final call into a `j` tail call in a function returning void.
void _fio_unlock(void)
{
    SignalSema(_fio_lock_sema);
}

// FUNC 0x0101EF50 _fio_init
// Lazy client init (called with the lock held when _fio_initialized == 0):
// sceSifInitRpc(0); install _fio_intr for SIF cmd 0x80000011 under
// DIntr/EIntr; bind FIO_SERVER_ID, re-binding after a busy-wait until the
// server answers (cd.serve != NULL); free all fd slots; FIO_F_SETUP call
// sends the EE address of the completion block and receives the IOP FILEIO
// version.  Returns 0, -1 (bind error) or 0xFFFEFFFF (setup call failed).
// Callers ignore the result.
int _fio_init(void)
{
    _fio_slot *s;
    /* sp+0x10: sent to the IOP (4 bytes) as the RPC send buffer, i.e. a SIF
     * DMA source, which must be quadword aligned (sp+0x10 of a 16-aligned
     * frame in the binary).  Unaligned, the IOP reads the wrong word as the
     * completion-block address and every later fileio request hangs. */
    void      *setup __attribute__((aligned(16)));

    sceSifInitRpc(0);
    DIntr();
    sceSifAddCmdHandler(SIF_CMDC_FILEIO_END, _fio_intr, _fio_intr_harg);
    EIntr();

    for (;;) {
        if (sceSifBindRpc(&_fio_client, FIO_SERVER_ID, 0) < 0)
            return -1;
        if (_fio_client.serve != NULL)
            break;
        _fio_nopdelay();
    }

    _fio_fd_sema_init();
    WaitSema(_fio_fd_sema);
    for (s = _fio_slots; s < &_fio_slots[32]; s++)
        s->mode = 0;
    SignalSema(_fio_fd_sema);

    setup = &_fio_intr_buf;             /* 0x0104CF80 */
    if (sceSifCallRpc(&_fio_client, FIO_F_SETUP, 0, &setup, 4, _fio_recv_buf, 4, NULL, NULL) < 0)
        return (int)0xFFFEFFFF;

    _fio_iop_version = *(_fio_version_t *)UNCACHED(_fio_recv_buf);
    _fio_initialized = 1;
    return 0;
}

// FUNC 0x0101F0F0 _fio_version_check
// Non-zero (1) if the IOP FILEIO version is unusable: it differs from "2240"
// AND from "...." AND those two differ from each other (the third memcmp is
// constant-true here).  Only caller: sceOpen.
static int _fio_version_check(void)
{
    const char *lib = FIO_LIB_VERSION;
    const char *iop = (const char *)&_fio_iop_version;

    return memcmp(iop, lib, 4) != 0 && memcmp(iop, _fio_alt_version, 4) != 0
        && memcmp(lib, _fio_alt_version, 4) != 0;
}

/* ======================================================================== */
/* Public API                                                               */
/* ======================================================================== */

// FUNC 0x0101F180 sceFsReset
// Forget the IOP-side state (call after an IOP reboot): clear the init flag
// and the IOP version so the next call re-binds.  Called once by main's IOP
// reboot sequence (F_01000880).  Semaphores and the fd table are kept.
int sceFsReset(void)
{
    _fio_initialized = 0;
    memset(&_fio_iop_version, 0, 4);
    return 0;
}

// FUNC 0x0101F1B8 sceOpen
// Variadic like Sony's (a2..t3 are spilled; `mode` is the first va slot).
// Allocates an EE fd slot, sends FIO_F_OPEN {flag & 0x0FFFFFFF, mode,
// path truncated to 0x3FF chars, efd}, waits, and on success stores the IOP
// fd and ORs the full flag word into the slot.  Returns the EE fd, the IOP
// error, -0x10004 on a bad IOP version, -ENODEV (-19) if no slot is free,
// -EAGAIN (-11) if the request was not accepted (the reserved slot is then
// leaked: it stays FIO_SLOT_RESERVED).  Callers: ImageLoader host/disk
// sources (F_01004F58, F_010051F8) and the hostfs wrapper F_0100F6D0.
int sceOpen(const char *filename, int flag, ...)
{
    struct _fio_open_arg *arg = (struct _fio_open_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    _fio_slot *slot;
    va_list ap;
    int mode, efd, sema, ret, i;
    int result;

    _fio_lock(FIO_F_OPEN);
    if (!_fio_initialized)
        _fio_init();
    if (_fio_version_check()) {
        _fio_unlock();
        return (int)0xFFFEFFFC;
    }

    slot = _fio_fd_alloc();
    if (slot == NULL) {
        _fio_unlock();
        return -ENODEV;
    }

    va_start(ap, flag);
    mode = va_arg(ap, int);
    va_end(ap);
    for (i = 0; i < 0x400; i++) {
        if ((arg->path[i] = filename[i]) == '\0')
            break;
    }
    if (i == 0x400)
        arg->path[0x3FF] = '\0';

    efd = slot - _fio_slots;
    arg->flag = flag & 0x0FFFFFFF;
    arg->mode = mode;
    sp.maxCount  = 1;
    arg->efd  = efd;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = 4;

    if (sceSifCallRpc(&_fio_client, FIO_F_OPEN, 0, _fio_cmd_buf, 0x418, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ret = FIO_RPC_ACK();
    _fio_unlock();
    if (ret == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);

    if (result < 0) {
        WaitSema(_fio_fd_sema);
        slot->mode = 0;
        SignalSema(_fio_fd_sema);
        return result;
    }
    ret = efd;
    WaitSema(_fio_fd_sema);
    slot->iop_fd = result;
    slot->mode  |= flag;                /* keeps FIO_SLOT_RESERVED set */
    SignalSema(_fio_fd_sema);
    return ret;
}

// FUNC 0x0101F440 sceClose
// FIO_F_CLOSE {IOP fd, efd}.  The slot is freed (mode = 0) as soon as the
// RPC is sent, before the IOP's answer, without taking the fd-table sema.
// Returns 0 on success (any result >= 0), the IOP error, -1 if fileio was
// never initialised (does not init), -EBADF, or -EAGAIN.
int sceClose(int fd)
{
    struct _fio_close_arg *arg = (struct _fio_close_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    _fio_slot *slot;
    int sema, ack;
    int result;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_CLOSE);
    if (!_fio_initialized) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }

    arg->fd  = slot->iop_fd;
    sp.maxCount  = 1;
    arg->efd = slot - _fio_slots;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.sema        = sema;
    arg->h.result      = &result;
    arg->h.result_size = 4;

    if (sceSifCallRpc(&_fio_client, FIO_F_CLOSE, 0, _fio_cmd_buf, 0x14, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    slot->mode = 0;
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    if (result < 0)
        return result;
    return 0;
}

// FUNC 0x0101F5C0 sceLseek
// FIO_F_LSEEK {IOP fd, offset, whence, efd}.  Returns the new position or the
// IOP error; -1 (not initialised, no init), -EBADF, -EAGAIN.  SCE_NOWAIT
// files: the op is recorded in _fio_pending and 0 is returned at once.
int sceLseek(int fd, int offset, int whence)
{
    struct _fio_lseek_arg *arg = (struct _fio_lseek_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    _fio_slot *slot;
    unsigned int mode;
    int sema, ack;
    int result;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_LSEEK);
    if (!_fio_initialized) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    mode = slot->mode;

    arg->fd     = slot->iop_fd;
    arg->offset = offset;
    arg->whence = whence;
    arg->efd    = slot - _fio_slots;
    sp.maxCount  = 1;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.result      = &result;
    arg->h.result_size = 4;
    arg->h.sema        = sema;
    if (mode & SCE_NOWAIT)
        _fio_pending_add();

    if (sceSifCallRpc(&_fio_client, FIO_F_LSEEK, 0, _fio_cmd_buf, 0x1C, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    if (mode & SCE_NOWAIT) {
        DeleteSema(sema);
        return 0;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0101F7F8 sceRead
// FIO_F_READ {IOP fd, buf, nbyte, -, efd}.  The IOP DMAs the 16-byte-aligned
// middle of the data straight into `buf`; the unaligned head/tail bytes come
// back in the completion block and are patched in by _fio_intr.  Writes back
// the D-cache of `buf` (unless SCE_NOWBDC), of the first 0xA4 bytes of the
// completion block and of the request.  Returns bytes read / IOP error; -1,
// -EBADF, -EAGAIN; 0 at once for SCE_NOWAIT files.
int sceRead(int fd, void *buf, int nbyte)
{
    struct _fio_read_arg *arg = (struct _fio_read_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    _fio_slot *slot;
    unsigned int mode;
    int sema, ack;
    int result;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_READ);
    if (!_fio_initialized) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    mode = slot->mode;

    arg->fd    = slot->iop_fd;
    sp.maxCount = 1;
    arg->efd   = slot - _fio_slots;
    arg->buf   = buf;
    arg->nbyte = nbyte;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.result      = &result;
    arg->h.result_size = 4;
    arg->h.sema        = sema;
    if (mode & SCE_NOWAIT)
        _fio_pending_add();

    if (!(mode & SCE_NOWBDC))
        sceSifWriteBackDCache(buf, nbyte);
    sceSifWriteBackDCache(&_fio_intr_buf, 0xA4);
    sceSifWriteBackDCache(arg, 0x20);

    if (sceSifCallRpc(&_fio_client, FIO_F_READ, 0, _fio_cmd_buf, 0x20, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    if (mode & SCE_NOWAIT) {
        DeleteSema(sema);
        return 0;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0101FA68 sceWrite
// FIO_F_WRITE {IOP fd, buf, nbyte, head_len, head[16], efd}: the bytes up to
// buf's next 16-byte boundary (at most nbyte) are sent inline, read through
// the uncached mirror of `buf`; the IOP fetches the rest from EE memory.
// D-cache of `buf` is written back unless SCE_NOWBDC.  Returns bytes
// written / IOP error; -1, -EBADF, -EAGAIN; 0 at once for SCE_NOWAIT files.
// Only caller: hostfs wrapper F_0100F7C8.
int sceWrite(int fd, const void *buf, int nbyte)
{
    struct _fio_write_arg *arg = (struct _fio_write_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    _fio_slot *slot;
    unsigned int mode;
    int head, sema, ack, i;
    int result;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_WRITE);
    if (!_fio_initialized) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    mode = slot->mode;

    arg->fd    = slot->iop_fd;
    sp.maxCount = 1;
    arg->efd   = slot - _fio_slots;
    arg->nbyte = nbyte;
    arg->buf   = buf;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.result      = &result;
    arg->h.result_size = 4;
    arg->h.sema        = sema;
    if (mode & SCE_NOWAIT)
        _fio_pending_add();

    /* bytes up to the next 16-byte boundary: 16 - (buf & 15) */
    if (((u32)buf & 0xF) == 0)
        head = 0;
    else
        head = ((u32)buf / 16 * 16) + 16 - (u32)buf;
    if (nbyte < head)
        head = nbyte;

    if (!(mode & SCE_NOWBDC))
        sceSifWriteBackDCache((void *)buf, nbyte);

    buf = UNCACHED(buf);
    arg->head_len = head;
    for (i = 0; i < head; i++)
        arg->head[i] = ((const u8 *)buf)[i];

    if (sceSifCallRpc(&_fio_client, FIO_F_WRITE, 0, _fio_cmd_buf, 0x30, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    if (mode & SCE_NOWAIT) {
        DeleteSema(sema);
        return 0;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0101FD28 sceIoctl
// Requests 1..3 are answered on the EE without an RPC:
//   1: *(int *)argp = 1 if any SCE_NOWAIT op is still pending, else 0
//      (written through the global copy _fio_ioctl_argp);
//   2: *(int *)argp = last completion result (uncached 0x0104CF90);
//   3: *(64-bit *)argp = same, 64 bits (ld/sd: argp must be 8-aligned).
// Anything else: FIO_F_IOCTL {IOP fd, req, 0x400 bytes copied from argp (or
// arglen 0 if argp is NULL), bufp 0, buflen 0, arglen 0x400}.  Unlike close/
// lseek/read/write it runs _fio_init if needed.  Returns 0 / IOP result;
// -EBADF, -EAGAIN.  No callers in the bootstrap.
int sceIoctl(int fd, int req, void *argp)
{
    struct _fio_ioctl_arg *arg = (struct _fio_ioctl_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    _fio_slot *slot;
    int sema, ack, i;
    int result;
    /* `ret` is always 1 and only feeds case 1's `return ret - 1` (0): an
     * exact form, not the original - retail keeps the constant 1 in a
     * callee-saved register from the prologue ($s5) and compares case 1
     * against it, which egcs does when CSE can substitute a register
     * already holding 1 for the case constant.  Same behaviour. */
    int ret = 1;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_IOCTL);
    _fio_ioctl_argp = argp;
    if (!_fio_initialized)
        _fio_init();
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    arg->bufp   = NULL;
    arg->buflen = 0;

    switch (req) {
    case 1:
        WaitSema(_fio_pending_sema);
        for (i = 0; i < 32; i++) {
            if (_fio_pending[i] != -1)
                break;
        }
        if (i == 32)
            *(int *)_fio_ioctl_argp = 0;
        else
            *(int *)_fio_ioctl_argp = 1;
        SignalSema(_fio_pending_sema);
        _fio_unlock();
        return ret - 1;

    case 2:
        *(int *)argp = *(volatile int *)UNCACHED(&_fio_intr_buf.d.result);
        _fio_unlock();
        return 0;

    case 3:
        *(u64 *)argp = *(volatile u64 *)UNCACHED(&_fio_intr_buf.d.result);
        _fio_unlock();
        return 0;

    default:
        break;
    }

    arg->fd  = slot->iop_fd;
    arg->req = req;
    if (argp == NULL) {
        arg->arglen = 0;
    } else {
        arg->arglen = 0x400;
        /* inline 1 KB block copy in the binary (no memcpy call) */
        *(_fio_blk400 *)arg->arg = *(const _fio_blk400 *)argp;
    }

    sp.initCount = 0;
    sp.maxCount  = 1;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = 4;
    sceSifWriteBackDCache(_fio_cmd_buf, 0x420);

    if (sceSifCallRpc(&_fio_client, FIO_F_IOCTL, 0, _fio_cmd_buf, 0x420, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x01020078 sceIoctl2
// FIO_F_IOCTL2 (26) {IOP fd, request, arg (arglen bytes, memcpy), bufp,
// buflen, arglen}; the IOP's output (<= 0x400 bytes) is copied to bufp by
// _fio_intr.  Runs _fio_init if needed.  Returns IOP result; -EBADF,
// -EAGAIN, -EINVAL if arglen or buflen > 0x400.
// BUG (reproduced): the -EINVAL return does not call _fio_unlock, so the
// fileio lock stays taken and every later fileio call blocks forever.
// No callers in the bootstrap.
int sceIoctl2(int fd, int request, void *argp, unsigned int arglen,
              void *bufp, unsigned int buflen)
{
    struct _fio_ioctl_arg *arg = (struct _fio_ioctl_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    _fio_slot *slot;
    int sema, ack;
    int result;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_IOCTL2);
    if (!_fio_initialized)
        _fio_init();
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    if (arglen > 0x400 || buflen > 0x400)
        return -EINVAL;                 /* lock not released (see above) */

    if (argp == NULL)
        arg->arglen = 0;                /* dead store: overwritten below */
    else
        memcpy(arg->arg, argp, arglen);

    arg->fd     = slot->iop_fd;
    arg->req    = request;
    arg->arglen = arglen;
    sp.maxCount  = 1;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->buflen        = buflen;
    arg->h.result      = &result;
    arg->h.result_size = 4;
    arg->bufp          = bufp;
    arg->h.sema        = sema;
    sceSifWriteBackDCache(_fio_cmd_buf, 0x420);

    if (sceSifCallRpc(&_fio_client, FIO_F_IOCTL2, 0, _fio_cmd_buf, 0x420, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x01020250 _fio_pathcmd
// Shared body of the single-path requests: lock (with fno as the op id),
// init if needed, copy `path` (truncated to 0x3FF chars) to +0x0C and send
// RPC `fno` with size 0x0D + strlen.  Returns IOP result or -EAGAIN.
// Callers: sceRemove (6), sceRmdir (8); kernl_rest's sceDelDrv, sceDopen,
// sceChdir, sceUmount.
int _fio_pathcmd(const char *path, int fno)
{
    struct _fio_path_arg *arg = (struct _fio_path_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    int sema, ack, i;
    int result;

    _fio_lock(fno);
    if (!_fio_initialized)
        _fio_init();

    for (i = 0; i < 0x400; i++) {
        if ((arg->path[i] = path[i]) == '\0')
            break;
    }
    if (i == 0x400) {
        arg->path[0x3FF] = '\0';
        i = 0x3FF;
    }

    sp.initCount = 0;
    sp.maxCount  = 1;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = 4;

    if (sceSifCallRpc(&_fio_client, fno, 0, _fio_cmd_buf, i + 0xD, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x010203F8 sceRemove
// _fio_pathcmd(filename, FIO_F_REMOVE).  No callers in the bootstrap.
int sceRemove(const char *filename)
{
    return _fio_pathcmd(filename, FIO_F_REMOVE);
}

// FUNC 0x01020418 sceMkdir
// FIO_F_MKDIR {mode, path (<= 0x3FF chars)}, size 0x11 + strlen.  Returns
// IOP result or -EAGAIN.  No callers in the bootstrap.
int sceMkdir(const char *dirname, int mode)
{
    struct _fio_mkdir_arg *arg = (struct _fio_mkdir_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    int sema, ack, i;
    int result;

    _fio_lock(FIO_F_MKDIR);
    if (!_fio_initialized)
        _fio_init();

    for (i = 0; i < 0x400; i++) {
        if ((arg->path[i] = dirname[i]) == '\0')
            break;
    }
    if (i == 0x400) {
        arg->path[0x3FF] = '\0';
        i = 0x3FF;
    }
    arg->mode = mode;

    sp.maxCount  = 1;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = 4;

    if (sceSifCallRpc(&_fio_client, FIO_F_MKDIR, 0, _fio_cmd_buf, i + 0x11, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x010205C8 sceRmdir
// _fio_pathcmd(dirname, FIO_F_RMDIR).  No callers in the bootstrap.
int sceRmdir(const char *dirname)
{
    return _fio_pathcmd(dirname, FIO_F_RMDIR);
}

// FUNC 0x010205E8 sceFormat
// FIO_F_FORMAT (14) {path, blockdev ("" if NULL), arg bytes, arglen}, fixed
// size 0xC10.  Both strings are copied into the shared buffer (truncated to
// 0x3FF chars) before arglen is checked; arglen > 0x400 (signed) returns
// -E2BIG (-7) after unlocking.  Returns IOP result or -EAGAIN.  No callers.
int sceFormat(const char *path, const char *blockdev, void *arg, int arglen)
{
    struct _fio_format_arg *req = (struct _fio_format_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    int sema, ack, i;
    int result;

    _fio_lock(FIO_F_FORMAT);
    if (!_fio_initialized)
        _fio_init();

    for (i = 0; i < 0x400; i++) {
        if ((req->path[i] = path[i]) == '\0')
            break;
    }
    if (i == 0x400)
        req->path[0x3FF] = '\0';

    if (blockdev == NULL) {
        req->blkdev[0] = '\0';
    } else {
        for (i = 0; i < 0x400; i++) {
            if ((req->blkdev[i] = blockdev[i]) == '\0')
                break;
        }
        if (i == 0x400)
            req->blkdev[0x3FF] = '\0';
    }

    if (arglen > 0x400) {
        _fio_unlock();
        return -E2BIG;
    }
    for (i = 0; i < arglen; i++)
        req->arg[i] = ((const u8 *)arg)[i];
    req->arglen = arglen;

    sp.maxCount  = 1;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    req->h.result      = &result;
    req->h.sema        = sema;
    req->h.result_size = 4;
    sceSifWriteBackDCache(_fio_cmd_buf, 0xC10);

    if (sceSifCallRpc(&_fio_client, FIO_F_FORMAT, 0, _fio_cmd_buf, 0xC10, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x01020858 sceAddDrv
// FIO_F_ADDDRV (15) with a single int argument at +0x0C (presumably an IOP
// address of a device descriptor) /* ? Sony name/arg type unconfirmed */.
// Unlike the other calls its failure codes are -1 (not -EAGAIN).  No callers.
int sceAddDrv(int drv)
{
    struct _fio_adddrv_arg *arg = (struct _fio_adddrv_arg *)_fio_cmd_buf;
    struct SemaParam sp;
    int sema, ack;
    int result;

    _fio_lock(FIO_F_ADDDRV);
    if (!_fio_initialized)
        _fio_init();
    arg->arg = drv;

    sp.maxCount  = 1;
    sp.initCount = 0;
    sp.option    = 0;
    sema = CreateSema(&sp);
    arg->h.sema        = sema;
    arg->h.result      = &result;
    arg->h.result_size = 4;

    if (sceSifCallRpc(&_fio_client, FIO_F_ADDDRV, 0, _fio_cmd_buf, 0x10, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -1;
    }
    ack = FIO_RPC_ACK();
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -1;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}
