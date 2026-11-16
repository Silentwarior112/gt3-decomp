/* fileio_part1.c - Sony libkernl 2240 fileio.o, first part,
 * core 0x00289288..0x0028A94F: the EE client of the IOP FILEIO RPC server
 * (0x80000001), up to sceIoctl.  Same machine code as
 * decomp/bootstrap/sdk/libkernl/fileio.c.
 *
 * Protocol: every entry point takes the global fileio lock (_fio_lock /
 * _fio_unlock), lazily runs _fio_init, builds its request in the single send
 * buffer _fio_cmd_buf (0x004AF9C0) and sceSifCallRpc()s it (blocking, mode
 * 0) with a 4-byte receive buffer _fio_recv_buf (0x004B0600) whose first
 * word (read uncached) says whether the IOP accepted it.  The request header
 * carries a fresh per-call semaphore and the address of an int on the
 * caller's stack; when the IOP finishes it DMAs a completion block to
 * _fio_intr_buf (0x004B0640) and raises SIF command 0x80000011, whose
 * handler _fio_intr copies the result there and iSignalSema()s the caller.
 * Files opened with SCE_NOWAIT: lseek/read/write record the (negated) sema
 * id in _fio_pending and return 0 at once; sceIoctl(fd, 1..3) polls that
 * (ps2sdk's counterpart is fioSync).
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part1_internal.h"

/* sceIoctl requests answered on the EE (descriptive names). */
#define FIO_IOCTL_PENDING   1   /* *(int *)argp = any SCE_NOWAIT op still pending */
#define FIO_IOCTL_RESULT    2   /* *(int *)argp = last completion result          */
#define FIO_IOCTL_RESULT64  3   /* *(u64 *)argp = the same, 64 bits               */

/* Uncached read of the RPC receive buffer's first word (IOP accept flag).  */
#define FIO_RPC_ACK()   (*(int *)UNCACHED(_fio_recv_buf))
/* Copy one field out of the completion block through its uncached mirror
 * (egcs inlines a fixed-size memcpy as lwl/lwr; GCC 14 just loads it).     */
#ifdef BOOT_OLD_GCC
#define FIO_UREAD(dst, src) memcpy(&(dst), UNCACHED(&(src)), sizeof(dst))
#else
#define FIO_UREAD(dst, src) ((dst) = *(__typeof__(src) *)UNCACHED(&(src)))
#endif

/* Sony's nopdelay(), inlined in _fio_init's bind retry loop: 0x100001 passes
 * (the nops are the old compiler's short-loop padding).                    */
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

/* Inlined identically into sceLseek, sceRead and sceWrite for SCE_NOWAIT
 * files: record the request's completion sema in the first free
 * _fio_pending slot and negate it in the request so _fio_intr treats the
 * completion as asynchronous.  If all 32 slots are busy nothing is recorded
 * and the id stays positive.  Only used as `if (...) _fio_pending_add();`
 * (it calls WaitSema / SignalSema on _fio_pending_sema).                    */
#define _fio_pending_add()                                  \
    {                                                       \
        int _pi;                                            \
                                                            \
        WaitSema(_fio_pending_sema);                        \
        for (_pi = 0; _pi < FIO_NR_PENDING; _pi++) {        \
            if (_fio_pending[_pi] == FIO_PENDING_FREE) {    \
                _fio_pending[_pi] = arg->h.sema;            \
                arg->h.sema = -arg->h.sema;                 \
                break;                                      \
            }                                               \
        }                                                   \
        SignalSema(_fio_pending_sema);                      \
    }

/* fileio.o's data.  This file (the object's first half) defines all of it;
 * u141's fileio_part2.c (the second half) uses the shared part through
 * runtime/newlib/G007_stdio.h's externs.
 *
 * .data, initialised, in this order (then _fio_alt_version, defined after
 * _fio_intr below: its literal follows _fio_intr's jump table in .rodata).  */
volatile int _fio_pending[FIO_NR_PENDING] = {          /* 0x003081D0 all FIO_PENDING_FREE */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};
int _fio_initialized = 0;                               /* 0x00308250 */
int _fio_lock_sema = -1;                                /* 0x00308254 */
int _fio_fd_sema = -1;                                  /* 0x00308258 */
int _fio_pending_sema = -1;                             /* 0x0030825C */

/* .bss: C commons, each placed at its address by the full link.  In the
 * original these were fileio.o's statics, one .bss block in this order
 * (between sifrpc.o's and iopheap.o's statics), but the block interleaves
 * the variables both halves use (_fio_cmd_buf, _fio_recv_buf, _fio_slots,
 * _fio_client: globals in our split) with this half's own, so no static .bss
 * section of this file could sit at the original's addresses.  The RPC
 * send / receive buffers and the completion block are SIF DMA buffers,
 * 64-byte aligned.                                                          */
void           *_fio_ioctl_argp;                                        /* 0x004AF980 sceIoctl's argp */
u8              _fio_cmd_buf[0xC40] __attribute__((aligned(64)));      /* 0x004AF9C0 */
int             _fio_recv_buf[0x10] __attribute__((aligned(64)));      /* 0x004B0600 */
_fio_intr_block _fio_intr_buf __attribute__((aligned(64)));            /* 0x004B0640 completion block */
_fio_slot       _fio_slots[FIO_NR_SLOTS];                               /* 0x004B0A80 */
sceSifClientData _fio_client;                                           /* 0x004B0C80 */
_fio_version_t  _fio_iop_version;                                       /* 0x004B0CA8 IOP FILEIO version */
/* ? size: only its address is used (_fio_intr's ignored harg).  0x50 covers
 * fileio.o's .bss up to iopheap.o's (0x004B0D00): the 0x40 bytes after
 * harg + 0x10 are fileio.o's (the bootstrap image has the same 0x40 before
 * another object, loadfile.o), unreferenced; one more static of the original
 * would hold them as well, zeros either way.                                */
u8              _fio_intr_harg[0x50];                                   /* 0x004B0CB0 */

// FUNC 0x00289288 _fio_fd_sema_init
//   One-shot: if the fd-table sema does not exist yet, create it and the
//   pending-table sema (both binary: max 1, init 1, option 0).
static void _fio_fd_sema_init(void)
{
    struct SemaParam sema_param;

    if (_fio_fd_sema == -1) {
        sema_param.initCount = 1;
        sema_param.maxCount  = 1;
        sema_param.option    = 0;
        _fio_fd_sema      = CreateSema(&sema_param);
        _fio_pending_sema = CreateSema(&sema_param);
    }
}

// FUNC 0x002892E8 _fio_fd_alloc
//   Reserve the first free fd slot (mode == 0) by setting its mode to
//   FIO_SLOT_RESERVED; NULL if all 32 are in use.  Callers: sceOpen and one
//   outside the unit (sceDopen).
_fio_slot *_fio_fd_alloc(void)
{
    _fio_slot *slot;

    _fio_fd_sema_init();
    WaitSema(_fio_fd_sema);
    for (slot = _fio_slots; slot < &_fio_slots[FIO_NR_SLOTS]; slot++) {
        if (slot->mode == 0) {
            slot->mode = FIO_SLOT_RESERVED;
            SignalSema(_fio_fd_sema);
            return slot;
        }
    }
    SignalSema(_fio_fd_sema);
    return NULL;
}

// FUNC 0x00289370 _fio_fd_to_slot
//   &_fio_slots[fd], or NULL if fd is out of range (unsigned compare, so
//   negative fds fail too).  Does not check that the slot is open; callers
//   test slot->mode.  The sema round-trip protects nothing but is in the code.
_fio_slot *_fio_fd_to_slot(int fd)
{
    _fio_slot *slot;

    _fio_fd_sema_init();
    WaitSema(_fio_fd_sema);
    if ((unsigned int)fd >= FIO_NR_SLOTS) {
        SignalSema(_fio_fd_sema);
        return NULL;
    }
    slot = &_fio_slots[fd];
    SignalSema(_fio_fd_sema);
    return slot;
}

// FUNC 0x002893E0 _fio_intr
//   SIF command 0x80000011 (FILEIO completion) handler, installed by
//   _fio_init; interrupt context.  Both arguments are ignored: everything is
//   read from the completion block through its uncached mirror.  Copies
//   hdr.size bytes of result (from +0x10) to hdr.dest (memcpy call), then per
//   function number (jump table @0x003492F0 indexed by fno-2, 25 entries):
//     2  (read)          patch the unaligned head/tail bytes into the user buffer
//     11 (dread)         copy the 0x144-byte sce_dirent to the caller's buffer
//     12 (getstat)       copy the 0x40-byte sce_stat
//     23/25/26 (devctl/readlink/ioctl2)  copy up to 0x400 output bytes
//     others             nothing
//   Finally a non-negative sema is iSignalSema'd (blocking caller); a negative
//   one is an SCE_NOWAIT op whose entry is cleared from _fio_pending.
static void _fio_intr(void *pkt, void *harg)
{
    /* the completion header, field by field */
    int          sema;
    int          fno;
    void        *dest;
    int          size;
    /* per-op output pointer / length (shared by the cases below) */
    void        *dst;
    unsigned int len;
    struct _fio_read_fixup *fixup;
    u8          *p;
    int          i;

    FIO_UREAD(sema, _fio_intr_buf.d.hdr.sema);
    FIO_UREAD(fno,  _fio_intr_buf.d.hdr.fno);
    FIO_UREAD(dest, _fio_intr_buf.d.hdr.dest);
    FIO_UREAD(size, _fio_intr_buf.d.hdr.size);
    memcpy(dest, UNCACHED(&_fio_intr_buf.d.result), size);

    switch (fno) {
    case FIO_F_READ:
        fixup = (struct _fio_read_fixup *)UNCACHED(&_fio_intr_buf.d.u.read);
        if (fixup->head_len > 0) {
            p = fixup->head_dst;
            for (i = 0; i < fixup->head_len; i++)
                p[i] = fixup->head[i];
        }
        if (fixup->tail_len > 0) {
            p = fixup->tail_dst;
            for (i = 0; i < fixup->tail_len; i++)
                p[i] = fixup->tail[i];
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
        if (len > FIO_DATA_MAX)
            len = FIO_DATA_MAX;
        memcpy(dst, UNCACHED(&_fio_intr_buf.d.u.buf.data), len);
        break;

    default:
        break;
    }

    if (sema < 0) {
        sema = -sema;
        for (i = 0; i < FIO_NR_PENDING; i++) {
            if (_fio_pending[i] == sema) {
                _fio_pending[i] = FIO_PENDING_FREE;
                break;
            }
        }
    } else {
        iSignalSema(sema);
    }
}

// FUNC 0x00289798 _fio_lock_sema_init
//   One-shot creation of the request-serialising sema (max 1, init 1).
static void _fio_lock_sema_init(void)
{
    struct SemaParam sema_param;

    if (_fio_lock_sema == -1) {
        sema_param.initCount = 1;
        sema_param.maxCount  = 1;
        sema_param.option    = 0;
        _fio_lock_sema = CreateSema(&sema_param);
    }
}

// FUNC 0x002897E8 _fio_lock
//   Take the fileio lock.  `op` (the caller's FIO_F_* number) is ignored in
//   this build.  Always returns 0.  22 callers (6 here, 16 outside).
int _fio_lock(int op)
{
    _fio_lock_sema_init();
    WaitSema(_fio_lock_sema);
    return 0;
}

// FUNC 0x00289818 _fio_unlock
//   Release the fileio lock (tail call of SignalSema; $v0 happens to hold
//   SignalSema's result, no caller uses it).
void _fio_unlock(void)
{
    SignalSema(_fio_lock_sema);
}

// FUNC 0x00289828 _fio_init
//   Lazy client init (called with the lock held when _fio_initialized == 0):
//   sceSifInitRpc(0); install _fio_intr for SIF cmd 0x80000011 under
//   DIntr/EIntr; bind FIO_SERVER_ID, re-binding after a busy-wait until the
//   server answers (cd.serve != NULL); free all fd slots; FIO_F_SETUP call
//   sends the EE address of the completion block and receives the IOP
//   FILEIO version.  Returns 0, -1 (bind error) or -SCE_ECALLMISS
//   (0xFFFEFFFF, setup call failed).  Callers ignore the result.
int _fio_init(void)
{
    _fio_slot *slot;
    /* sp+0x10: the RPC send buffer (4 bytes), a SIF DMA source: must be
     * quadword aligned, as it is in the binary's 16-aligned frame.          */
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
    for (slot = _fio_slots; slot < &_fio_slots[FIO_NR_SLOTS]; slot++)
        slot->mode = 0;
    SignalSema(_fio_fd_sema);

    setup = &_fio_intr_buf;             /* 0x004B0640 */
    if (sceSifCallRpc(&_fio_client, FIO_F_SETUP, 0, &setup, 4, _fio_recv_buf, 4, NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    _fio_iop_version = *(_fio_version_t *)UNCACHED(_fio_recv_buf);
    _fio_initialized = 1;
    return 0;
}

/* The second accepted IOP FILEIO version (.data after the semas, -> "...."
 * at 0x00349358, after _fio_intr's jump table in .rodata). */
static const char *_fio_alt_version = "....";           /* 0x00308260 */

// FUNC 0x002899C8 _fio_version_check
//   Non-zero (1) if the IOP FILEIO version is unusable: it differs from
//   "2240" AND from "...." AND those two differ from each other (the third
//   memcmp is constant-true here).  The three calls go to 0x0027D8FC, which
//   symbols.csv names "memcpy_small" but is memcmp (libmatch).  Only
//   caller: sceOpen.
static int _fio_version_check(void)
{
    const char *lib = FIO_LIB_VERSION;
    const char *iop = (const char *)&_fio_iop_version;

    return memcmp(iop, lib, 4) != 0 && memcmp(iop, _fio_alt_version, 4) != 0
        && memcmp(lib, _fio_alt_version, 4) != 0;
}

// FUNC 0x00289A58 sceFsReset
//   Forget the IOP-side state (call after an IOP reboot): clear the init flag
//   and the IOP version so the next call re-binds.  Semaphores and the fd
//   table are kept.  One caller outside the unit.
int sceFsReset(void)
{
    _fio_initialized = 0;
    memset(&_fio_iop_version, 0, 4);
    return 0;
}

// FUNC 0x00289A90 sceOpen
//   Variadic like Sony's (a2..t3 are spilled; `mode` is the first va slot).
//   Allocates an EE fd slot, sends FIO_F_OPEN {flag & ~FIO_FLAG_EE_ONLY
//   (0x0FFFFFFF), mode, path truncated to 0x3FF chars, efd}, waits, and on
//   success stores the IOP fd and ORs the full flag word into the slot.
//   Returns the EE fd, the IOP error, -SCE_EVERSIONMISS (-0x10004) on a bad
//   IOP version, -ENODEV (-19) if no slot is free, -EAGAIN (-11) if the
//   request was not accepted (the reserved slot is then leaked: it stays
//   FIO_SLOT_RESERVED).  2 callers outside the unit.
int sceOpen(const char *filename, int flag, ...)
{
    struct _fio_open_arg *arg = (struct _fio_open_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    _fio_slot *slot;
    va_list ap;
    int mode, efd, sema, ret, i;
    int result;

    _fio_lock(FIO_F_OPEN);
    if (!_fio_initialized)
        _fio_init();
    if (_fio_version_check()) {
        _fio_unlock();
        return -SCE_EVERSIONMISS;
    }

    slot = _fio_fd_alloc();
    if (slot == NULL) {
        _fio_unlock();
        return -ENODEV;
    }

    va_start(ap, flag);
    mode = va_arg(ap, int);
    va_end(ap);
    for (i = 0; i < FIO_PATH_MAX; i++) {
        if ((arg->path[i] = filename[i]) == '\0')
            break;
    }
    if (i == FIO_PATH_MAX)
        arg->path[FIO_PATH_MAX - 1] = '\0';

    efd = slot - _fio_slots;
    arg->flag = flag & ~FIO_FLAG_EE_ONLY;
    arg->mode = mode;
    sema_param.maxCount  = 1;
    arg->efd  = efd;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = 4;

    if (sceSifCallRpc(&_fio_client, FIO_F_OPEN, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x00289D18 sceClose
//   FIO_F_CLOSE {IOP fd, efd}.  The slot is freed (mode = 0) as soon as the
//   RPC is sent, before the IOP's answer, without taking the fd-table sema.
//   Returns 0 on success (any result >= 0), the IOP error, -1 if fileio was
//   never initialised (does not init), -EBADF, or -EAGAIN.
int sceClose(int fd)
{
    struct _fio_close_arg *arg = (struct _fio_close_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
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
    sema_param.maxCount  = 1;
    arg->efd = slot - _fio_slots;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.sema        = sema;
    arg->h.result      = &result;
    arg->h.result_size = 4;

    if (sceSifCallRpc(&_fio_client, FIO_F_CLOSE, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x00289E98 sceLseek
//   FIO_F_LSEEK {IOP fd, offset, whence, efd}.  Returns the new position or
//   the IOP error; -1 (not initialised, no init), -EBADF, -EAGAIN.
//   SCE_NOWAIT files: the op is recorded in _fio_pending and 0 is returned.
int sceLseek(int fd, int offset, int whence)
{
    struct _fio_lseek_arg *arg = (struct _fio_lseek_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
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
    sema_param.maxCount  = 1;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.result_size = 4;
    arg->h.sema        = sema;
    if (mode & SCE_NOWAIT)
        _fio_pending_add();     /* implicit: WaitSema / SignalSema(_fio_pending_sema) */

    if (sceSifCallRpc(&_fio_client, FIO_F_LSEEK, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x0028A0D0 sceRead
//   FIO_F_READ {IOP fd, buf, nbyte, -, efd}.  The IOP DMAs the 16-byte-
//   aligned middle of the data straight into `buf`; the unaligned head/tail
//   bytes come back in the completion block and are patched in by _fio_intr.
//   Writes back the D-cache of `buf` (unless SCE_NOWBDC), of the first 0xA4
//   bytes of the completion block and of the request.  Returns bytes read /
//   IOP error; -1, -EBADF, -EAGAIN; 0 at once for SCE_NOWAIT files.
int sceRead(int fd, void *buf, int nbyte)
{
    struct _fio_read_arg *arg = (struct _fio_read_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
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
    sema_param.maxCount = 1;
    arg->efd   = slot - _fio_slots;
    arg->buf   = buf;
    arg->nbyte = nbyte;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.result_size = 4;
    arg->h.sema        = sema;
    if (mode & SCE_NOWAIT)
        _fio_pending_add();     /* implicit: WaitSema / SignalSema(_fio_pending_sema) */

    if (!(mode & SCE_NOWBDC))
        sceSifWriteBackDCache(buf, nbyte);
    /* the completion header, result and read fixup: 0x14 + 0x90 = 0xA4 */
    sceSifWriteBackDCache(&_fio_intr_buf, offsetof(struct _fio_intr_data, u) + sizeof(struct _fio_read_fixup));
    sceSifWriteBackDCache(arg, sizeof(*arg));

    if (sceSifCallRpc(&_fio_client, FIO_F_READ, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x0028A340 sceWrite
//   FIO_F_WRITE {IOP fd, buf, nbyte, head_len, head[16], efd}: the bytes up
//   to buf's next 16-byte boundary (at most nbyte) are sent inline, read
//   through the uncached mirror of `buf`; the IOP fetches the rest from EE
//   memory.  D-cache of `buf` is written back unless SCE_NOWBDC.  Returns
//   bytes written / IOP error; -1, -EBADF, -EAGAIN; 0 at once for SCE_NOWAIT.
int sceWrite(int fd, const void *buf, int nbyte)
{
    struct _fio_write_arg *arg = (struct _fio_write_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
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
    sema_param.maxCount = 1;
    arg->efd   = slot - _fio_slots;
    arg->nbyte = nbyte;
    arg->buf   = buf;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.result_size = 4;
    arg->h.sema        = sema;
    if (mode & SCE_NOWAIT)
        _fio_pending_add();     /* implicit: WaitSema / SignalSema(_fio_pending_sema) */

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

    if (sceSifCallRpc(&_fio_client, FIO_F_WRITE, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x0028A600 sceIoctl
//   Requests 1..3 are answered on the EE without an RPC:
//     1 FIO_IOCTL_PENDING: *(int *)argp = 1 if any SCE_NOWAIT op is still
//        pending, else 0 (written through the global copy _fio_ioctl_argp
//        @0x004AF980);
//     2 FIO_IOCTL_RESULT: *(int *)argp = last completion result (uncached
//        0x004B0650);
//     3 FIO_IOCTL_RESULT64: *(64-bit *)argp = same, 64 bits (ld/sd: argp
//        must be 8-aligned).
//   Anything else: FIO_F_IOCTL {IOP fd, req, 0x400 bytes copied from argp
//   (or arglen 0 if argp is NULL), bufp 0, buflen 0, arglen 0x400}.  Unlike
//   close/lseek/read/write it runs _fio_init if needed.  Returns 0 / IOP
//   result; -EBADF, -EAGAIN.  No callers.
int sceIoctl(int fd, int req, void *argp)
{
    struct _fio_ioctl_arg *arg = (struct _fio_ioctl_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    _fio_slot *slot;
    int sema, ack, i;
    int result;
    /* `one` is always 1 and only feeds case 1's `return one - 1` (0): the
     * binary keeps the constant 1 in $s5 from the prologue and compares
     * case 1 against it.  Same behaviour.                                   */
    int one = 1;

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
    case FIO_IOCTL_PENDING:
        WaitSema(_fio_pending_sema);
        for (i = 0; i < FIO_NR_PENDING; i++) {
            if (_fio_pending[i] != FIO_PENDING_FREE)
                break;
        }
        if (i == FIO_NR_PENDING)
            *(int *)_fio_ioctl_argp = 0;
        else
            *(int *)_fio_ioctl_argp = 1;
        SignalSema(_fio_pending_sema);
        _fio_unlock();
        return one - 1;

    case FIO_IOCTL_RESULT:
        *(int *)argp = *(volatile int *)UNCACHED(&_fio_intr_buf.d.result);
        _fio_unlock();
        return 0;

    case FIO_IOCTL_RESULT64:
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
        arg->arglen = FIO_DATA_MAX;
        /* inline 1 KB block copy in the binary (no memcpy call) */
        *(_fio_blk400 *)arg->arg = *(const _fio_blk400 *)argp;
    }

    sema_param.initCount = 0;
    sema_param.maxCount  = 1;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = 4;
    sceSifWriteBackDCache(_fio_cmd_buf, sizeof(*arg));

    if (sceSifCallRpc(&_fio_client, FIO_F_IOCTL, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
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
