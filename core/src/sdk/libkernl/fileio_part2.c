/* fileio_part2.c - Sony libkernl 2240 fileio.o, second half
 * (0x0028A950..0x0028C7F8): sceIoctl2, the single-path helper _fio_pathcmd
 * and the FILEIO client calls sceRemove .. sceReadlink.  The first half of
 * fileio.o (fd table, _fio_init, _fio_lock, sceOpen .. sceIoctl) is unit
 * u140.  Same machine code as the bootstrap's sdk/libkernl/fileio.c and
 * filedev.c (0x01020078..0x01021F20).
 *
 * Every call: _fio_lock(op); lazy _fio_init() (path calls) or init/fd checks
 * (fd calls); build the request in _fio_cmd_buf {+0 completion sema, +4
 * &result (caller stack), +8 result size, args from +0xC}; sema =
 * CreateSema({max 1, init 0}); sceSifCallRpc(&_fio_client, fno, 0, request,
 * size, _fio_recv_buf, 4, NULL, NULL); read the accept word uncached;
 * _fio_unlock(); WaitSema(sema) (signalled by the SIF completion handler
 * _fio_intr, u140); DeleteSema(sema); return result.
 * Callers in the core: none of these has a caller, except _fio_pathcmd
 * (6 wrappers here).
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part2_internal.h"

// FUNC 0x0028A950 sceIoctl2
// FIO_F_IOCTL2 (26) {IOP fd, request, arg (arglen bytes, memcpy), bufp,
// buflen, arglen}; the IOP's output (<= 0x400 bytes) is copied to bufp by
// _fio_intr.  Runs _fio_init if needed.  Returns IOP result; -EBADF,
// -EAGAIN, -EINVAL if arglen or buflen > 0x400.
// BUG (reproduced): the -EINVAL return does not call _fio_unlock, so the
// fileio lock stays taken and every later fileio call blocks forever.
// No callers in the core either.
int sceIoctl2(int fd, int request, void *argp, unsigned int arglen,
              void *bufp, unsigned int buflen)
{
    struct _fio_ioctl_arg *arg = (struct _fio_ioctl_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
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
    if (arglen > FIO_DATA_MAX || buflen > FIO_DATA_MAX)
        return -EINVAL;                 /* lock not released (see above) */

    if (argp == NULL)
        arg->arglen = 0;                /* dead store: overwritten below */
    else
        memcpy(arg->arg, argp, arglen);

    arg->fd     = slot->iop_fd;
    arg->req    = request;
    arg->arglen = arglen;
    sema_param.maxCount  = 1;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->buflen        = buflen;
    arg->h.result      = &result;
    arg->h.result_size = sizeof(result);
    arg->bufp          = bufp;
    arg->h.sema        = sema;
    sceSifWriteBackDCache(_fio_cmd_buf, sizeof(*arg));

    if (sceSifCallRpc(&_fio_client, FIO_F_IOCTL2, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x0028AB28 _fio_pathcmd
// Shared body of the single-path requests: lock (with fno as the op id),
// init if needed, copy `path` (truncated to 0x3FF chars) to +0x0C and send
// RPC `fno` with size 0x0D + strlen.  Returns IOP result or -EAGAIN.
// Callers: sceRemove, sceRmdir, sceDelDrv, sceDopen, sceChdir, sceUmount.
// (Not static: sifdev.h declares it, as the bootstrap's filedev.c needs.)
int _fio_pathcmd(const char *path, int fno)
{
    struct _fio_path_arg *arg = (struct _fio_path_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int sema, ack, len;
    int result;

    _fio_lock(fno);
    if (!_fio_initialized)
        _fio_init();

    FIO_COPY_PATH(arg->path, path, len)
    if (len == FIO_PATH_MAX) {
        arg->path[FIO_PATH_MAX - 1] = '\0';
        len = FIO_PATH_MAX - 1;
    }

    sema_param.initCount = 0;
    sema_param.maxCount  = 1;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = sizeof(result);

    if (sceSifCallRpc(&_fio_client, fno, 0, _fio_cmd_buf, offsetof(struct _fio_path_arg, path) + len + 1,
                      _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x0028ACD0 sceRemove
// _fio_pathcmd(filename, FIO_F_REMOVE).  No callers.
int sceRemove(const char *filename)
{
    return _fio_pathcmd(filename, FIO_F_REMOVE);
}

// FUNC 0x0028ACF0 sceMkdir
// FIO_F_MKDIR {mode, path (<= 0x3FF chars)}, size 0x11 + strlen.  Returns
// IOP result or -EAGAIN.  No callers.
int sceMkdir(const char *dirname, int mode)
{
    struct _fio_mkdir_arg *arg = (struct _fio_mkdir_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int sema, ack, len;
    int result;

    _fio_lock(FIO_F_MKDIR);
    if (!_fio_initialized)
        _fio_init();

    FIO_COPY_PATH(arg->path, dirname, len)
    if (len == FIO_PATH_MAX) {
        arg->path[FIO_PATH_MAX - 1] = '\0';
        len = FIO_PATH_MAX - 1;
    }
    arg->mode = mode;

    sema_param.maxCount  = 1;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = sizeof(result);

    if (sceSifCallRpc(&_fio_client, FIO_F_MKDIR, 0, _fio_cmd_buf, offsetof(struct _fio_mkdir_arg, path) + len + 1,
                      _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x0028AEA0 sceRmdir
// _fio_pathcmd(dirname, FIO_F_RMDIR).  No callers.
int sceRmdir(const char *dirname)
{
    return _fio_pathcmd(dirname, FIO_F_RMDIR);
}

// FUNC 0x0028AEC0 sceFormat
// FIO_F_FORMAT (14) {path, blockdev ("" if NULL), arg bytes, arglen}, fixed
// size 0xC10.  Both strings are copied into the shared buffer (truncated to
// 0x3FF chars) before arglen is checked; arglen > 0x400 (signed) returns
// -E2BIG (-7) after unlocking.  Returns IOP result or -EAGAIN.  No callers.
int sceFormat(const char *path, const char *blockdev, void *arg, int arglen)
{
    struct _fio_format_arg *req = (struct _fio_format_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int sema, ack, i;
    int result;

    _fio_lock(FIO_F_FORMAT);
    if (!_fio_initialized)
        _fio_init();

    FIO_COPY_PATH(req->dev, path, i)
    if (i == FIO_PATH_MAX)
        req->dev[FIO_PATH_MAX - 1] = '\0';

    if (blockdev == NULL) {
        req->blockdev[0] = '\0';
    } else {
        FIO_COPY_PATH(req->blockdev, blockdev, i)
        if (i == FIO_PATH_MAX)
            req->blockdev[FIO_PATH_MAX - 1] = '\0';
    }

    if (arglen > FIO_DATA_MAX) {
        _fio_unlock();
        return -E2BIG;
    }
    for (i = 0; i < arglen; i++)
        req->arg[i] = ((const u8 *)arg)[i];
    req->arglen = arglen;

    sema_param.maxCount  = 1;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    req->h.result      = &result;
    req->h.sema        = sema;
    req->h.result_size = sizeof(result);
    sceSifWriteBackDCache(_fio_cmd_buf, sizeof(*req));

    if (sceSifCallRpc(&_fio_client, FIO_F_FORMAT, 0, _fio_cmd_buf, sizeof(*req), _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x0028B130 sceAddDrv
// FIO_F_ADDDRV (15) with a single word at +0x0C: the IOP address of a device
// descriptor, which the IOP FILEIO server hands to its AddDrv (ps2sdk:
// FIO_F_ADDDRV between FORMAT and DELDRV in fileio-common.h, ioman.h
// `int AddDrv(iop_device_t *device)').  `int' as sifdev.h declares it: the
// EE never dereferences it.  Unlike the other calls its failure codes are -1
// (not -EAGAIN).  No callers.
int sceAddDrv(int device)
{
    struct _fio_adddrv_arg *arg = (struct _fio_adddrv_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int sema, ack;
    int result;

    _fio_lock(FIO_F_ADDDRV);
    if (!_fio_initialized)
        _fio_init();
    arg->device = device;

    sema_param.maxCount  = 1;
    sema_param.initCount = 0;
    sema_param.option    = 0;
    sema = CreateSema(&sema_param);
    arg->h.sema        = sema;
    arg->h.result      = &result;
    arg->h.result_size = sizeof(result);

    if (sceSifCallRpc(&_fio_client, FIO_F_ADDDRV, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
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

// FUNC 0x0028B250 sceDelDrv
//   FILEIO fno 0x10 (DELDRV): single-path request through the shared helper.
int sceDelDrv(const char *devname)
{
    return _fio_pathcmd(devname, FIO_F_DELDRV);
}

// FUNC 0x0028B270 sceDopen
//   Reserve an EE fd slot, issue DOPEN (fno 9) for the path; on success store
//   the IOP descriptor in the slot and return the EE fd (slot index).  On an
//   RPC/IOP failure the reserved slot is released and the negative result
//   returned.  -ENODEV when the 32-entry table is full.
int sceDopen(const char *dirname)
{
    _fio_slot *slot;
    int result;

    slot = _fio_fd_alloc();
    if (slot == NULL)
        return -ENODEV;

    result = _fio_pathcmd(dirname, FIO_F_DOPEN);
    if (result < 0) {
        WaitSema(_fio_fd_sema);
        slot->mode = 0;
        SignalSema(_fio_fd_sema);
        return result;
    }
    WaitSema(_fio_fd_sema);
    slot->iop_fd = result;
    result = slot - _fio_slots;         /* the EE fd: (slot - 0x004B0A80) >> 4 */
    SignalSema(_fio_fd_sema);
    return result;
}

// FUNC 0x0028B310 sceDclose
//   FILEIO fno 0x0A (DCLOSE).  The EE slot is freed as soon as the RPC was
//   sent successfully (before the accept check).  Non-negative IOP results
//   are folded to 0.
int sceDclose(int fd)
{
    struct _fio_dclose_arg *arg = (struct _fio_dclose_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    _fio_slot *slot;
    int sema;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_DCLOSE);
    if (_fio_initialized == 0) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    arg->fd = slot->iop_fd;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.sema        = sema;
    arg->h.result      = &result;
    arg->h.result_size = sizeof(result);
    if (sceSifCallRpc(&_fio_client, FIO_F_DCLOSE, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    slot->mode = 0;
    ack = FIO_RPC_ACK();                /* read before the unlock */
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

// FUNC 0x0028B478 sceDread
//   FILEIO fno 0x0B (DREAD): the completion handler (_fio_intr) copies the
//   full 0x144-byte sce_dirent the IOP sent back into *buf.  Returns the IOP
//   result (>0 entry read, 0 end of directory, <0 error).
//   SONY BUG reproduced: when sceSifCallRpc fails it calls WaitSema(sema)
//   (not DeleteSema) on the fresh count-0 semaphore - the caller would block
//   forever - and the semaphore is never deleted.
int sceDread(int fd, struct sce_dirent *buf)
{
    struct _fio_dread_arg *arg = (struct _fio_dread_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    _fio_slot *slot;
    int sema;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_DREAD);
    if (_fio_initialized == 0) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    arg->fd  = slot->iop_fd;
    arg->buf = buf;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.sema        = sema;
    arg->h.result      = &result;
    arg->h.result_size = sizeof(result);
    if (sceSifCallRpc(&_fio_client, FIO_F_DREAD, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
        WaitSema(sema);                 /* sic - see above */
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0028B5D0 sceGetstat
//   FILEIO fno 0x0C (GETSTAT): {+0x0C stat buffer, +0x10 path}; only
//   strlen(path)+1 bytes of the path are sent (size = len + 0x11).  The
//   0x40-byte sce_stat is copied back by the completion handler.
int sceGetstat(const char *name, struct sce_stat *buf)
{
    struct _fio_getstat_arg *arg = (struct _fio_getstat_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    int len;
    int sema;

    _fio_lock(FIO_F_GETSTAT);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(arg->name, name, len)
    if (len == FIO_PATH_MAX) {
        arg->name[FIO_PATH_MAX - 1] = 0;
        len = FIO_PATH_MAX - 1;
    }
    arg->buf = buf;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = sizeof(result);
    if (sceSifCallRpc(&_fio_client, FIO_F_GETSTAT, 0, _fio_cmd_buf, offsetof(struct _fio_getstat_arg, name) + len + 1,
                      _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0028B770 sceChstat
//   FILEIO fno 0x0D (CHSTAT): {+0x0C cbit, +0x10 sce_stat (0x40, copied with
//   ldl/ldr - a struct assignment), +0x50 path}.  Writes back the D-cache of
//   the whole 0x450-byte request, sends len + 0x51 bytes.
int sceChstat(const char *name, struct sce_stat *buf, unsigned int cbit)
{
    struct _fio_chstat_arg *arg = (struct _fio_chstat_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    int len;
    int sema;

    _fio_lock(FIO_F_CHSTAT);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(arg->name, name, len)
    if (len == FIO_PATH_MAX) {
        arg->name[FIO_PATH_MAX - 1] = 0;
        len = FIO_PATH_MAX - 1;
    }
    arg->stat = *buf;
    arg->cbit = cbit;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = sizeof(result);
    sceSifWriteBackDCache(_fio_cmd_buf, sizeof(*arg));
    if (sceSifCallRpc(&_fio_client, FIO_F_CHSTAT, 0, _fio_cmd_buf, offsetof(struct _fio_chstat_arg, name) + len + 1,
                      _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0028B9B0 sceRename
//   FILEIO fno 0x11 (RENAME): old name @+0x0C, new name @+0x40C (0x400 each),
//   D-cache write-back of the whole 0x80C-byte request, sends 0x80C.
int sceRename(const char *oldname, const char *newname)
{
    struct _fio_rename_arg *arg = (struct _fio_rename_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    int sema;
    int len;

    _fio_lock(FIO_F_RENAME);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(arg->oldname, oldname, len)
    if (len == FIO_PATH_MAX)
        arg->oldname[FIO_PATH_MAX - 1] = 0;
    FIO_COPY_PATH(arg->newname, newname, len)
    if (len == FIO_PATH_MAX)
        arg->newname[FIO_PATH_MAX - 1] = 0;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = sizeof(result);
    sceSifWriteBackDCache(_fio_cmd_buf, sizeof(*arg));
    if (sceSifCallRpc(&_fio_client, FIO_F_RENAME, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0028BBA0 sceChdir
//   FILEIO fno 0x12 (CHDIR): single-path request through the shared helper.
int sceChdir(const char *name)
{
    return _fio_pathcmd(name, FIO_F_CHDIR);
}

// FUNC 0x0028BBC0 sceSync
//   FILEIO fno 0x13 (SYNC): {+0x10 flag, +0x14 device path}, sends 0x414
//   (no D-cache write-back; +0x0C is left untouched).
int sceSync(const char *devname, int flag)
{
    struct _fio_sync_arg *arg = (struct _fio_sync_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    int sema;
    int len;

    _fio_lock(FIO_F_SYNC);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(arg->dev, devname, len)
    if (len == FIO_PATH_MAX)
        arg->dev[FIO_PATH_MAX - 1] = 0;
    arg->flag = flag;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = sizeof(result);
    if (sceSifCallRpc(&_fio_client, FIO_F_SYNC, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0028BD58 sceMount
//   FILEIO fno 0x14 (MOUNT): fs device @+0x0C, block device @+0x40C, option
//   data @+0x80C (arglen bytes, signed compare), flag @+0xC0C, arglen @+0xC10.
//   The arglen <= 0x400 check (signed) is evaluated after both names have
//   been copied into the buffer; failure unlocks and returns -E2BIG (-7).
int sceMount(const char *fsdevname, const char *blockdevname, int flag,
             void *arg, int arglen)
{
    struct _fio_mount_arg *req = (struct _fio_mount_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    int arglen_ok;
    int sema;
    int i;

    _fio_lock(FIO_F_MOUNT);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(req->fsdev, fsdevname, i)
    arglen_ok = (arglen <= FIO_DATA_MAX);
    if (i == FIO_PATH_MAX)
        req->fsdev[FIO_PATH_MAX - 1] = 0;
    FIO_COPY_PATH(req->blockdev, blockdevname, i)
    if (i == FIO_PATH_MAX)
        req->blockdev[FIO_PATH_MAX - 1] = 0;
    if (!arglen_ok) {
        _fio_unlock();
        return -E2BIG;
    }
    for (i = 0; i < arglen; i++)
        req->arg[i] = ((const u8 *)arg)[i];
    req->arglen = arglen;
    req->flag   = flag;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    req->h.result      = &result;
    req->h.sema        = sema;
    req->h.result_size = sizeof(result);
    sceSifWriteBackDCache(_fio_cmd_buf, sizeof(*req));
    if (sceSifCallRpc(&_fio_client, FIO_F_MOUNT, 0, _fio_cmd_buf, sizeof(*req), _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0028BFC8 sceUmount
//   FILEIO fno 0x15 (UMOUNT): single-path request through the shared helper.
int sceUmount(const char *fsdevname)
{
    return _fio_pathcmd(fsdevname, FIO_F_UMOUNT);
}

// FUNC 0x0028BFE8 sceLseek64
//   FILEIO fno 0x16 (LSEEK64): {+0x0C IOP fd, +0x10 s64 offset, +0x18 whence,
//   +0x1C EE fd}, 8-byte result.  For an fd opened SCE_NOWAIT the completion
//   sema is parked in the first free slot of the 32-entry pending table and
//   the request's sema word is negated (async marker); after the RPC is
//   accepted such a call deletes its sema and returns 0 at once (the pending
//   table keeps the stale id - Sony behaviour).  If no pending slot is free
//   the sema word is simply not negated (the packet looks synchronous to the
//   completion handler), but the call still deletes the sema and returns 0
//   right after the accept - it never waits for the result.
s64 sceLseek64(int fd, s64 offset, int whence)
{
    struct _fio_lseek64_arg *arg = (struct _fio_lseek64_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    s64 result;
    _fio_slot *slot;
    unsigned int mode;
    int sema;
    int i;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(FIO_F_LSEEK64);
    if (_fio_initialized == 0) {
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
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.result_size = sizeof(result);
    arg->h.sema        = sema;
    if (mode & SCE_NOWAIT) {
        WaitSema(_fio_pending_sema);
        for (i = 0; i < 32; i++) {
            if (_fio_pending[i] == -1) {    /* free entry */
                _fio_pending[i] = arg->h.sema;
                arg->h.sema = -arg->h.sema;
                break;
            }
        }
        SignalSema(_fio_pending_sema);
    }
    if (sceSifCallRpc(&_fio_client, FIO_F_LSEEK64, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
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

// FUNC 0x0028C220 sceDevctl
//   FILEIO fno 0x17 (DEVCTL): device @+0x0C, input arg @+0x40C, cmd @+0x80C,
//   arglen @+0x810, bufp @+0x814, buflen @+0x818; write-back + send 0x81C.
//   SONY BUG reproduced: the arglen/buflen > 0x400 (unsigned) path returns
//   -EINVAL WITHOUT _fio_unlock(), leaking the fileio lock (every later
//   fileio call would then block in _fio_lock).
int sceDevctl(const char *devname, int cmd, void *arg, unsigned int arglen,
              void *bufp, unsigned int buflen)
{
    struct _fio_devctl_arg *req = (struct _fio_devctl_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    int arglen_ok;
    int i;
    int sema;

    _fio_lock(FIO_F_DEVCTL);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(req->dev, devname, i)
    arglen_ok = (arglen <= FIO_DATA_MAX);
    if (i == FIO_PATH_MAX)
        req->dev[FIO_PATH_MAX - 1] = 0;
    if (!arglen_ok || !(buflen <= FIO_DATA_MAX))
        return -EINVAL;                 /* sic: lock not released */
    for (i = 0; i < arglen; i++)        /* i < arglen <= 0x400: unsigned compare */
        req->arg[i] = ((const u8 *)arg)[i];
    req->arglen = arglen;
    req->cmd    = cmd;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    req->buflen        = buflen;
    req->h.result      = &result;
    req->h.result_size = sizeof(result);
    req->bufp          = bufp;
    req->h.sema        = sema;
    sceSifWriteBackDCache(_fio_cmd_buf, sizeof(*req));
    if (sceSifCallRpc(&_fio_client, FIO_F_DEVCTL, 0, _fio_cmd_buf, sizeof(*req), _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0028C450 sceSymlink
//   FILEIO fno 0x18 (SYMLINK): existing @+0x0C, new name @+0x40C, sends 0x80C
//   (no D-cache write-back, unlike sceRename).  Passes FIO_F_RENAME (0x11)
//   to _fio_lock - copy/paste in Sony's source, harmless (op is unused).
int sceSymlink(const char *existing, const char *newname)
{
    struct _fio_rename_arg *arg = (struct _fio_rename_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    int sema;
    int len;

    _fio_lock(FIO_F_RENAME);            /* sic */
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(arg->oldname, existing, len)
    if (len == FIO_PATH_MAX)
        arg->oldname[FIO_PATH_MAX - 1] = 0;
    FIO_COPY_PATH(arg->newname, newname, len)
    if (len == FIO_PATH_MAX)
        arg->newname[FIO_PATH_MAX - 1] = 0;
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = sizeof(result);
    if (sceSifCallRpc(&_fio_client, FIO_F_SYMLINK, 0, _fio_cmd_buf, sizeof(*arg), _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x0028C630 sceReadlink
//   FILEIO fno 0x19 (READLINK): {+0x0C bufsize, +0x10 buf, +0x14 path};
//   bufsize >= 0x400 (unsigned) is clamped to 0x3FF.  Writes back the D-cache
//   of the OUTPUT buffer (buf, bufsize) before the call.  Sends 0x80C bytes,
//   sceSymlink's size, although the request is 0x414 (the rest is whatever
//   _fio_cmd_buf holds).  Also passes lock id FIO_F_RENAME (copy/paste,
//   harmless).
int sceReadlink(const char *path, char *buf, unsigned int bufsize)
{
    struct _fio_readlink_arg *arg = (struct _fio_readlink_arg *)_fio_cmd_buf;
    struct SemaParam sema_param;
    int ack;
    int result;
    int sema;
    int len;

    _fio_lock(FIO_F_RENAME);            /* sic */
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(arg->path, path, len)
    if (len == FIO_PATH_MAX)
        arg->path[FIO_PATH_MAX - 1] = 0;
    if (bufsize >= FIO_PATH_MAX)
        bufsize = FIO_PATH_MAX - 1;
    arg->buf     = buf;
    arg->bufsize = bufsize;
    sceSifWriteBackDCache(buf, bufsize);
    FIO_SEMA_PARAM(sema_param);
    sema = CreateSema(&sema_param);
    arg->h.result      = &result;
    arg->h.sema        = sema;
    arg->h.result_size = sizeof(result);
    if (sceSifCallRpc(&_fio_client, FIO_F_READLINK, 0, _fio_cmd_buf, sizeof(struct _fio_rename_arg) /* sic: 0x80C */,
                      _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    ack = FIO_RPC_ACK();                /* read before the unlock */
    _fio_unlock();
    if (ack == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}
