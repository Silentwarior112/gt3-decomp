/* hostfs_io.cpp - "host:" file I/O wrappers over the SIF fileio client
 * (unit "io").  Functions 0x0100F6D0..0x0100F8D8.
 *
 * Each wrapper serialises one fileio call with the pdistd global lock
 * (semaphore id at .sdata 0x01041E60, created lazily by global_lock_acquire
 * via create_mutex_sema(1); released through signal_sema_helper, i.e.
 * iSignalSema).  The functions themselves are path-agnostic: the "host:"
 * prefix (.sdata 0x01041E18) is prepended by the callers in the ImageLoader
 * host source code (host_read_file 0x01005970, host_write_file 0x01005A78,
 * host_file_size 0x01005B08, host_file_exists 0x01005B68).
 */
#include "types.h"
#include "sifdev.h"
#include "boot_io.h"
/* global_lock_acquire (0x01012F68: create the 0x01041E60 sema on first use,
 * WaitSema) and global_lock_release (0x01012FA0: tail jump to
 * signal_sema_helper with that sema id); allocator unit, C linkage. */
#include "pdistd/allocator.h"

// FUNC 0x0100F6D0 host_open
// Locked sceOpen(path, flags) (sceOpen is variadic; called with 2 args).
// Returns the EE fileio fd (>= 0) or a negative error.  Callers:
// host_read_file / host_write_file / host_get_file_size.
int host_open(const char *path, int flags)
{
    int fd;

    global_lock_acquire();
    fd = sceOpen(path, flags);
    global_lock_release();
    return fd;
}

// FUNC 0x0100F718 host_lseek
// Locked sceLseek(fd, offset, whence); returns the new position or a
// negative error.  Used with (0, SCE_SEEK_END) to get file sizes and to
// position host_read_file.
int host_lseek(int fd, int offset, int whence)
{
    int pos;

    global_lock_acquire();
    pos = sceLseek(fd, offset, whence);
    global_lock_release();
    return pos;
}

// FUNC 0x0100F770 host_read
// Locked sceRead(fd, buf, len); returns bytes read or a negative error.
// Caller: host_read_file (0x01005970).
int host_read(int fd, void *buf, int len)
{
    int n;

    global_lock_acquire();
    n = sceRead(fd, buf, len);
    global_lock_release();
    return n;
}

// FUNC 0x0100F7C8 host_write
// Locked sceWrite(fd, buf, len); returns bytes written or a negative error.
// Caller: host_write_file (0x01005A78, itself unreferenced - debug dump helper).
int host_write(int fd, const void *buf, int len)
{
    int n;

    global_lock_acquire();
    n = sceWrite(fd, buf, len);
    global_lock_release();
    return n;
}

// FUNC 0x0100F820 host_close
// Locked sceClose(fd); returns its result.
int host_close(int fd)
{
    int r;

    global_lock_acquire();
    r = sceClose(fd);
    global_lock_release();
    return r;
}

// FUNC 0x0100F858 host_get_file_size
// stat-like probe of a host file: open it read-only; on failure return -1
// without touching *out.  Otherwise size = lseek(fd, 0, SEEK_END), close,
// then store out->size (+4) first and out->mode = 0x11 (+0) second, return 0.
// The lseek result is stored unchecked (a negative error would be reported as
// the size); the close result is ignored.  The binary sign-extends the lseek
// result (dsll32/dsra32) before the 32-bit store: the original kept it in a
// `long` (64 bits under ee-gcc, 32 under GCC 14); the stored word is the same
// either way.  0x11 = SCE_STM_R | SCE_STM_F
// ("readable regular file" in memory-card attribute terms) (?) - no reader of
// the field has been found.  Callers: host_file_size (0x01005B08, uses .size)
// and host_file_exists (0x01005B68, uses only the return value).
int host_get_file_size(const char *path, HostFileStat *out)
{
    int fd;
    long size;      /* 64-bit under ee-gcc: the dsll32/dsra32 below */

    fd = host_open(path, SCE_RDONLY);
    if (fd < 0)
        return -1;

    size = host_lseek(fd, 0, SCE_SEEK_END);
    host_close(fd);
    /* egcs schedules the .size store (+4) first, then .mode (+0) */
    out->mode = SCE_STM_R | SCE_STM_F;   /* 0x11 */
    out->size = size;
    return 0;
}
