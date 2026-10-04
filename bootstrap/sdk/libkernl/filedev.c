/* filedev.c - Sony libkernl 2.2.4 (PsIIlibkernl2240) fileio.o, second half:
 * the "extended" FILEIO client calls sceDelDrv .. sceReadlink
 * (0x01020978 .. 0x01021F20).
 *
 * The first half of fileio.o (fd table, _fio_init, sceOpen .. sceAddDrv,
 * _fio_pathcmd) belongs to the kernl_rpc_fio unit (sdk/libkernl/fileio.c).
 *
 * None of the functions in this file has a caller in the bootstrap - they
 * are dead code pulled in with fileio.o.
 *
 * Every call follows the same protocol with the IOP FILEIO server (0x80000001):
 *   _fio_lock(op); lazy _fio_init() (path calls) or init/fd checks (fd calls);
 *   build the command in the shared send buffer @0x0104C300
 *       {+0 completion sema, +4 &result (caller stack), +8 result size, args @+0xC};
 *   sema = CreateSema({max 1, init 0});
 *   sceSifCallRpc(&_fio_client, fno, 0, cmd, ssize, recv @0x0104CF40, 4, NULL, NULL);
 *   read the "accepted" word of the recv buffer through the uncached mirror;
 *   _fio_unlock(); WaitSema(sema) (the SIF command 0x80000011 handler signals
 *   it after the IOP has written the result); DeleteSema(sema); return result.
 * Errors: -1 fileio not initialised (fd calls), -EBADF, -11 (EAGAIN: RPC or
 * accept failure), -19 (dopen: fd table full), -7 (mount: E2BIG),
 * -22 (devctl: EINVAL).
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "sif.h"
#include "sifdev.h"

/* fileio.o state shared with kernl_rpc_fio - defined in sdk/libkernl/fileio.c
 * and declared in include/sifdev.h, which is authoritative for the names
 * (this file just follows it):
 *   _fio_pending      0x0103DA40   _fio_initialized  0x0103DAC0
 *   _fio_fd_sema      0x0103DAC8   _fio_pending_sema 0x0103DACC
 *   _fio_cmd_buf      0x0104C300 (send buffer)
 *   _fio_recv_buf     0x0104CF40 (receive buffer)
 *   _fio_slots        0x0104D3C0 (fd table)
 *   _fio_client       0x0104D5C0 (FILEIO RPC client)                        */

/* ------------------------------------------------------------------------ */
/* Command layouts in the send buffer (offsets verified in the listing).    */
/* ------------------------------------------------------------------------ */
/* Every request starts with _fio_rpc_hdr (sifdev.h):
 *   +0x00 sema (completion semaphore; negated = SCE_NOWAIT request),
 *   +0x04 result (EE address the result is written to - the caller's stack),
 *   +0x08 result_size (4; 8 for lseek64).                                    */
typedef _fio_rpc_hdr _fio_cmd_hdr;

union _fio_cmd {
    _fio_cmd_hdr hdr;
    struct { _fio_cmd_hdr h; int fd; } dclose;                         /* fno 0x0A, sent 0x14 */
    struct { _fio_cmd_hdr h; int fd; struct sce_dirent *buf; } dread;  /* fno 0x0B, sent 0x20 */
    struct { _fio_cmd_hdr h; struct sce_stat *buf; u8 name[0x400]; } getstat;    /* fno 0x0C */
    struct { _fio_cmd_hdr h; unsigned int cbit; struct sce_stat stat;
             u8 name[0x400]; } chstat;                               /* fno 0x0D */
    struct { _fio_cmd_hdr h; char name1[0x400]; char name2[0x400]; } names2;  /* rename 0x11, symlink 0x18 */
    struct { _fio_cmd_hdr h; int _unused0C; int flag; char dev[0x400]; } sync; /* fno 0x13, +0x0C never written */
    struct { _fio_cmd_hdr h; char fsname[0x400]; char devname[0x400];
             u8 data[0x400]; int flag; int datalen; } mount;           /* fno 0x14, sent 0xC14 */
    struct { _fio_cmd_hdr h; int fd; s64 offset; int whence; int eefd; } lseek64; /* fno 0x16, sent 0x20 */
    struct { _fio_cmd_hdr h; char name[0x400]; u8 arg[0x400]; int cmd;
             unsigned int arglen; void *bufp; unsigned int buflen; } devctl;  /* fno 0x17, sent 0x81C */
    struct { _fio_cmd_hdr h; unsigned int bufsize; char *buf; char path[0x400]; } readlink; /* fno 0x19 */
};

_Static_assert(offsetof(union _fio_cmd, dclose.fd)       == 0x00C, "dclose");
_Static_assert(offsetof(union _fio_cmd, dread.buf)       == 0x010, "dread");
_Static_assert(offsetof(union _fio_cmd, getstat.name)    == 0x010, "getstat");
_Static_assert(offsetof(union _fio_cmd, chstat.stat)     == 0x010, "chstat.stat");
_Static_assert(offsetof(union _fio_cmd, chstat.name)     == 0x050, "chstat.name");
_Static_assert(offsetof(union _fio_cmd, names2.name2)    == 0x40C, "names2");
_Static_assert(offsetof(union _fio_cmd, sync.flag)       == 0x010, "sync.flag");
_Static_assert(offsetof(union _fio_cmd, sync.dev)        == 0x014, "sync.dev");
_Static_assert(offsetof(union _fio_cmd, mount.data)      == 0x80C, "mount.data");
_Static_assert(offsetof(union _fio_cmd, mount.flag)      == 0xC0C, "mount.flag");
_Static_assert(offsetof(union _fio_cmd, mount.datalen)   == 0xC10, "mount.datalen");
_Static_assert(offsetof(union _fio_cmd, lseek64.offset)  == 0x010, "lseek64.offset");
_Static_assert(offsetof(union _fio_cmd, lseek64.whence)  == 0x018, "lseek64.whence");
_Static_assert(offsetof(union _fio_cmd, lseek64.eefd)    == 0x01C, "lseek64.eefd");
_Static_assert(offsetof(union _fio_cmd, devctl.arg)      == 0x40C, "devctl.arg");
_Static_assert(offsetof(union _fio_cmd, devctl.cmd)      == 0x80C, "devctl.cmd");
_Static_assert(offsetof(union _fio_cmd, devctl.buflen)   == 0x818, "devctl.buflen");
_Static_assert(offsetof(union _fio_cmd, readlink.buf)    == 0x010, "readlink.buf");
_Static_assert(offsetof(union _fio_cmd, readlink.path)   == 0x014, "readlink.path");

#define FIO_CMD       ((union _fio_cmd *)_fio_cmd_buf)
/* The IOP's immediate reply word: non-zero = request accepted.  Always read
 * through the uncached mirror (address | 0x20000000).  A plain load, as in
 * the binary (in the delay slot of the following `jal _fio_unlock`, where
 * egcs never puts a volatile access); it is read between the sceSifCallRpc()
 * and _fio_unlock() calls, which no compiler can move it across or elide it
 * past (GCC 14's code is the same as with volatile).                         */
#define FIO_ACCEPTED() (*(int *)UNCACHED(_fio_recv_buf))

/* Written out in every path-taking call (a Sony macro in the original):
 * copy a path of at most 0x400 bytes including the NUL; i ends as
 * strlen(src), or 0x400 when no NUL was found in the first 0x400 bytes (the
 * callers then force dst[0x3FF] = 0).  The NUL test is on the stored value
 * of the destination's type: getstat/chstat names are u8 in the binary (test
 * on the zero-extended byte), every other field is char (`sll 24` test).   */
#define FIO_COPY_PATH(dst, src, i)                          \
    for ((i) = 0; (i) < 0x400; (i)++) {                     \
        if (((dst)[i] = (src)[i]) == 0)                     \
            break;                                          \
    }

/* Every call uses a fresh binary semaphore: only maxCount/initCount/option
 * are written (sp+0x14/0x18/0x24), the other SemaParam words are stack junk.
 * A comma expression, not do { } while (0): egcs treats the latter as a loop
 * and 8-byte aligns its head, which can add a padding nop.                  */
#define FIO_SEMA_PARAM(sp) \
    ((sp).maxCount = 1, (sp).initCount = 0, (sp).option = 0)

// FUNC 0x01020978 sceDelDrv
//   FILEIO fno 0x10 (DELDRV): single-path request through the shared helper.
int sceDelDrv(const char *devname)
{
    return _fio_pathcmd(devname, FIO_F_DELDRV);
}

// FUNC 0x01020998 sceDopen
//   Reserve an EE fd slot, issue DOPEN (fno 9) for the path; on success store
//   the IOP descriptor in the slot and return the EE fd (slot index).  On an
//   RPC/IOP failure the reserved slot is released and the negative result
//   returned.  -19 (-ENODEV) when the 32-entry table is full.
int sceDopen(const char *dirname)
{
    _fio_slot *slot;
    int r;

    slot = _fio_fd_alloc();
    if (slot == NULL)
        return -19;

    r = _fio_pathcmd(dirname, FIO_F_DOPEN);
    if (r < 0) {
        WaitSema(_fio_fd_sema);
        slot->mode = 0;
        SignalSema(_fio_fd_sema);
        return r;
    }
    WaitSema(_fio_fd_sema);
    slot->iop_fd = r;
    r = slot - _fio_slots;              /* (slot - 0x0104D3C0) >> 4 */
    SignalSema(_fio_fd_sema);
    return r;
}

// FUNC 0x01020A38 sceDclose
//   FILEIO fno 0x0A (DCLOSE).  The EE slot is freed as soon as the RPC was
//   sent successfully (before the accept check).  Non-negative IOP results
//   are folded to 0.
int sceDclose(int fd)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    _fio_slot *slot;
    int sema;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(0x0A);
    if (_fio_initialized == 0) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    cmd->dclose.fd = slot->iop_fd;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.sema   = sema;
    cmd->hdr.result = &result;
    cmd->hdr.result_size  = 4;
    if (sceSifCallRpc(&_fio_client, FIO_F_DCLOSE, 0, _fio_cmd_buf, 0x14, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    slot->mode = 0;
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    if (result < 0)
        return result;
    return 0;
}

// FUNC 0x01020BA0 sceDread
//   FILEIO fno 0x0B (DREAD): the completion handler (_fio_intr) copies the
//   full 0x144-byte sce_dirent the IOP sent back into *buf.  Returns the IOP
//   result (>0 entry read, 0 end of directory, <0 error).
//   SONY BUG reproduced: when sceSifCallRpc fails it calls WaitSema(sema)
//   (not DeleteSema) on the fresh count-0 semaphore - the caller would block
//   forever - and the semaphore is never deleted.
int sceDread(int fd, struct sce_dirent *buf)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    _fio_slot *slot;
    int sema;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(0x0B);
    if (_fio_initialized == 0) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    cmd->dread.fd  = slot->iop_fd;
    cmd->dread.buf = buf;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.sema   = sema;
    cmd->hdr.result = &result;
    cmd->hdr.result_size  = 4;
    if (sceSifCallRpc(&_fio_client, FIO_F_DREAD, 0, _fio_cmd_buf, 0x20, _fio_recv_buf, 4, NULL, NULL) < 0) {
        WaitSema(sema);                 /* sic - see above */
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x01020CF8 sceGetstat
//   FILEIO fno 0x0C (GETSTAT): {+0x0C stat buffer, +0x10 path}; only
//   strlen(path)+1 bytes of the path are sent (ssize = len + 0x11).  The
//   0x40-byte sce_stat is copied back by the completion handler.
int sceGetstat(const char *name, struct sce_stat *buf)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    int len;
    int sema;

    _fio_lock(0x0C);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(cmd->getstat.name, name, len)
    if (len == 0x400) {
        cmd->getstat.name[0x3FF] = 0;
        len = 0x3FF;
    }
    cmd->getstat.buf = buf;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.result = &result;
    cmd->hdr.sema   = sema;
    cmd->hdr.result_size  = 4;
    if (sceSifCallRpc(&_fio_client, FIO_F_GETSTAT, 0, _fio_cmd_buf, len + 0x11, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x01020E98 sceChstat
//   FILEIO fno 0x0D (CHSTAT): {+0x0C cbit, +0x10 sce_stat (0x40, copied with
//   ldl/ldr - a struct assignment), +0x50 path}.  Writes back the D-cache of
//   the first 0x450 bytes of the command, sends len + 0x51 bytes.
int sceChstat(const char *name, struct sce_stat *buf, unsigned int cbit)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    int len;
    int sema;

    _fio_lock(0x0D);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(cmd->chstat.name, name, len)
    if (len == 0x400) {
        cmd->chstat.name[0x3FF] = 0;
        len = 0x3FF;
    }
    cmd->chstat.stat = *buf;
    cmd->chstat.cbit = cbit;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.result = &result;
    cmd->hdr.sema   = sema;
    cmd->hdr.result_size  = 4;
    sceSifWriteBackDCache(_fio_cmd_buf, 0x450);
    if (sceSifCallRpc(&_fio_client, FIO_F_CHSTAT, 0, _fio_cmd_buf, len + 0x51, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x010210D8 sceRename
//   FILEIO fno 0x11 (RENAME): old name @+0x0C, new name @+0x40C (0x400 each),
//   D-cache write-back of the whole 0x80C-byte command, sends 0x80C.
int sceRename(const char *oldname, const char *newname)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    int sema;
    int i;

    _fio_lock(0x11);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(cmd->names2.name1, oldname, i)
    if (i == 0x400)
        cmd->names2.name1[0x3FF] = 0;
    FIO_COPY_PATH(cmd->names2.name2, newname, i)
    if (i == 0x400)
        cmd->names2.name2[0x3FF] = 0;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.result = &result;
    cmd->hdr.sema   = sema;
    cmd->hdr.result_size  = 4;
    sceSifWriteBackDCache(_fio_cmd_buf, 0x80C);
    if (sceSifCallRpc(&_fio_client, FIO_F_RENAME, 0, _fio_cmd_buf, 0x80C, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x010212C8 sceChdir
//   FILEIO fno 0x12 (CHDIR): single-path request through the shared helper.
int sceChdir(const char *name)
{
    return _fio_pathcmd(name, FIO_F_CHDIR);
}

// FUNC 0x010212E8 sceSync
//   FILEIO fno 0x13 (SYNC): {+0x10 flag, +0x14 device path}, sends 0x414
//   (no D-cache write-back; +0x0C is left untouched).
int sceSync(const char *devname, int flag)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    int sema;
    int i;

    _fio_lock(0x13);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(cmd->sync.dev, devname, i)
    if (i == 0x400)
        cmd->sync.dev[0x3FF] = 0;
    cmd->sync.flag = flag;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.result = &result;
    cmd->hdr.sema   = sema;
    cmd->hdr.result_size  = 4;
    if (sceSifCallRpc(&_fio_client, FIO_F_SYNC, 0, _fio_cmd_buf, 0x414, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x01021480 sceMount
//   FILEIO fno 0x14 (MOUNT): fs device @+0x0C, block device @+0x40C, option
//   data @+0x80C (arglen bytes, signed compare), flag @+0xC0C, arglen @+0xC10.
//   The arglen <= 0x400 check (signed) is evaluated after both names have
//   been copied into the buffer; failure unlocks and returns -7 (-E2BIG).
int sceMount(const char *fsdevname, const char *blockdevname, int flag,
             void *arg, int arglen)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    int size_ok;
    int sema;
    int i;

    _fio_lock(0x14);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(cmd->mount.fsname, fsdevname, i)
    size_ok = (arglen < 0x401);
    if (i == 0x400)
        cmd->mount.fsname[0x3FF] = 0;
    FIO_COPY_PATH(cmd->mount.devname, blockdevname, i)
    if (i == 0x400)
        cmd->mount.devname[0x3FF] = 0;
    if (!size_ok) {
        _fio_unlock();
        return -7;
    }
    for (i = 0; i < arglen; i++)
        cmd->mount.data[i] = ((const u8 *)arg)[i];
    cmd->mount.datalen = arglen;
    cmd->mount.flag    = flag;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.result = &result;
    cmd->hdr.sema   = sema;
    cmd->hdr.result_size  = 4;
    sceSifWriteBackDCache(_fio_cmd_buf, 0xC14);
    if (sceSifCallRpc(&_fio_client, FIO_F_MOUNT, 0, _fio_cmd_buf, 0xC14, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x010216F0 sceUmount
//   FILEIO fno 0x15 (UMOUNT): single-path request through the shared helper.
int sceUmount(const char *fsdevname)
{
    return _fio_pathcmd(fsdevname, FIO_F_UMOUNT);
}

// FUNC 0x01021710 sceLseek64
//   FILEIO fno 0x16 (LSEEK64): {+0x0C IOP fd, +0x10 s64 offset, +0x18 whence,
//   +0x1C EE fd}, 8-byte result.  For an fd opened SCE_NOWAIT the completion
//   sema is parked in the first free slot of the 32-entry pending table and
//   the command's sema word is negated (async marker); after the RPC is
//   accepted such a call deletes its sema and returns 0 at once (the pending
//   table keeps the stale id - Sony behaviour).  If no pending slot is free
//   the sema word is simply not negated (the packet looks synchronous to the
//   completion handler), but the call still deletes the sema and returns 0
//   right after the accept - it never waits for the result.
s64 sceLseek64(int fd, s64 offset, int whence)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    s64 result;
    _fio_slot *slot;
    unsigned int mode;
    int sema;
    int i;

    slot = _fio_fd_to_slot(fd);
    _fio_lock(0x16);
    if (_fio_initialized == 0) {
        _fio_unlock();
        return -1;
    }
    if (slot == NULL || slot->mode == 0) {
        _fio_unlock();
        return -EBADF;
    }
    mode = slot->mode;
    cmd->lseek64.fd     = slot->iop_fd;
    cmd->lseek64.offset = offset;
    cmd->lseek64.whence = whence;
    cmd->lseek64.eefd   = slot - _fio_slots;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.result = &result;
    cmd->hdr.result_size  = 8;
    cmd->hdr.sema   = sema;
    if (mode & SCE_NOWAIT) {
        WaitSema(_fio_pending_sema);
        for (i = 0; i < 32; i++) {
            if (_fio_pending[i] == -1) {
                _fio_pending[i] = cmd->hdr.sema;
                cmd->hdr.sema = -cmd->hdr.sema;
                break;
            }
        }
        SignalSema(_fio_pending_sema);
    }
    if (sceSifCallRpc(&_fio_client, FIO_F_LSEEK64, 0, _fio_cmd_buf, 0x20, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
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

// FUNC 0x01021948 sceDevctl
//   FILEIO fno 0x17 (DEVCTL): device @+0x0C, input arg @+0x40C, cmd @+0x80C,
//   arglen @+0x810, bufp @+0x814, buflen @+0x818; write-back + send 0x81C.
//   SONY BUG reproduced: the arglen/buflen > 0x400 (unsigned) path returns
//   -22 (-EINVAL) WITHOUT _fio_unlock(), leaking the fileio lock (every later
//   fileio call would then block in _fio_lock).
int sceDevctl(const char *devname, int cmd_no, void *arg, unsigned int arglen,
              void *bufp, unsigned int buflen)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    int arglen_ok;
    int i;
    int sema;

    _fio_lock(0x17);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(cmd->devctl.name, devname, i)
    arglen_ok = (arglen < 0x401);
    if (i == 0x400)
        cmd->devctl.name[0x3FF] = 0;
    if (!arglen_ok || !(buflen < 0x401))
        return -EINVAL;                 /* sic: lock not released */
    for (i = 0; i < arglen; i++)        /* i < arglen <= 0x400: unsigned compare */
        cmd->devctl.arg[i] = ((const u8 *)arg)[i];
    cmd->devctl.arglen = arglen;
    cmd->devctl.cmd    = cmd_no;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->devctl.buflen = buflen;
    cmd->hdr.result    = &result;
    cmd->hdr.result_size     = 4;
    cmd->devctl.bufp   = bufp;
    cmd->hdr.sema      = sema;
    sceSifWriteBackDCache(_fio_cmd_buf, 0x81C);
    if (sceSifCallRpc(&_fio_client, FIO_F_DEVCTL, 0, _fio_cmd_buf, 0x81C, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x01021B78 sceSymlink
//   FILEIO fno 0x18 (SYMLINK): existing @+0x0C, new name @+0x40C, sends 0x80C
//   (no D-cache write-back, unlike sceRename).  Passes 0x11 (RENAME's number)
//   to _fio_lock - copy/paste in Sony's source, harmless (op is unused).
int sceSymlink(const char *existing, const char *newname)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    int sema;
    int i;

    _fio_lock(0x11);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(cmd->names2.name1, existing, i)
    if (i == 0x400)
        cmd->names2.name1[0x3FF] = 0;
    FIO_COPY_PATH(cmd->names2.name2, newname, i)
    if (i == 0x400)
        cmd->names2.name2[0x3FF] = 0;
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.result = &result;
    cmd->hdr.sema   = sema;
    cmd->hdr.result_size  = 4;
    if (sceSifCallRpc(&_fio_client, FIO_F_SYMLINK, 0, _fio_cmd_buf, 0x80C, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}

// FUNC 0x01021D58 sceReadlink
//   FILEIO fno 0x19 (READLINK): {+0x0C bufsize, +0x10 buf, +0x14 path};
//   bufsize >= 0x400 (unsigned) is clamped to 0x3FF.  Writes back the D-cache
//   of the OUTPUT buffer (buf, bufsize) before the call, sends 0x80C.  Also
//   passes lock id 0x11 (copy/paste, harmless).
int sceReadlink(const char *path, char *buf, unsigned int bufsize)
{
    union _fio_cmd *cmd = FIO_CMD;
    struct SemaParam sp;
    int accepted;
    int result;
    int sema;
    int i;

    _fio_lock(0x11);
    if (_fio_initialized == 0)
        _fio_init();
    FIO_COPY_PATH(cmd->readlink.path, path, i)
    if (i == 0x400)
        cmd->readlink.path[0x3FF] = 0;
    if (bufsize >= 0x400)
        bufsize = 0x3FF;
    cmd->readlink.buf     = buf;
    cmd->readlink.bufsize = bufsize;
    sceSifWriteBackDCache(buf, bufsize);
    FIO_SEMA_PARAM(sp);
    sema = CreateSema(&sp);
    cmd->hdr.result = &result;
    cmd->hdr.sema   = sema;
    cmd->hdr.result_size  = 4;
    if (sceSifCallRpc(&_fio_client, FIO_F_READLINK, 0, _fio_cmd_buf, 0x80C, _fio_recv_buf, 4, NULL, NULL) < 0) {
        DeleteSema(sema);
        _fio_unlock();
        return -EAGAIN;
    }
    accepted = FIO_ACCEPTED();          /* read before the unlock */
    _fio_unlock();
    if (accepted == 0) {
        DeleteSema(sema);
        return -EAGAIN;
    }
    WaitSema(sema);
    DeleteSema(sema);
    return result;
}
