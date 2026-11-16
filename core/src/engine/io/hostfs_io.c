/* hostfs_io.c - fileio wrappers serialised by the pdistd global
 * lock (0x00269D80..0x00269F88).  The same machine code as the bootstrap's
 * src/hostfs_io.cpp (host_open .. host_get_file_size); the source is reused.
 * Callers: host_read_file / host_write_file (0x00241E08 / 0x00241F10),
 * file_resolve, host_file_size, host_file_exists.
 */
#include "engine/media/G002_mpeg_frameq.h"
#include "engine/system/system_util_internal.h"

// FUNC 0x00269D80 host_open
// Locked sceOpen(path, flags) (sceOpen is variadic; called with 2 args).
// Returns the EE fileio fd (>= 0) or a negative error.  Callers
// host_read_file, host_write_file, io_getstat.
int host_open(const char *path, int flags)
{
    int fd;

    global_lock_acquire();
    fd = sceOpen(path, flags);
    global_lock_release();
    return fd;
}

// FUNC 0x00269DC8 host_lseek
// Locked sceLseek(fd, offset, whence); returns the new position or a
// negative error.  Used with (0, SCE_SEEK_END) to get file sizes.
int host_lseek(int fd, int offset, int whence)
{
    int pos;

    global_lock_acquire();
    pos = sceLseek(fd, offset, whence);
    global_lock_release();
    return pos;
}

// FUNC 0x00269E20 host_read
// Locked sceRead(fd, buf, len); returns bytes read or a negative error.
int host_read(int fd, void *buf, int len)
{
    int n;

    global_lock_acquire();
    n = sceRead(fd, buf, len);
    global_lock_release();
    return n;
}

// FUNC 0x00269E78 host_write
// Locked sceWrite(fd, buf, len); returns bytes written or a negative error.
int host_write(int fd, const void *buf, int len)
{
    int n;

    global_lock_acquire();
    n = sceWrite(fd, buf, len);
    global_lock_release();
    return n;
}

// FUNC 0x00269ED0 host_close
// Locked sceClose(fd); returns its result.
int host_close(int fd)
{
    int r;

    global_lock_acquire();
    r = sceClose(fd);
    global_lock_release();
    return r;
}

// FUNC 0x00269F08 io_getstat
// (bootstrap host_get_file_size)  stat-like probe of a file: open it
// read-only; on failure return -1 without touching *out.  Otherwise size =
// lseek(fd, 0, SEEK_END), close, then store out->size (+4) first and
// out->mode = 0x11 (+0) second, return 0.  The lseek result is stored
// unchecked (a negative error would be reported as the size); the close
// result is ignored.  The binary sign-extends the lseek result
// (dsll32/dsra32) before the 32-bit store: the original kept it in a `long`
// (64 bits under ee-gcc); the stored word is the same either way.
// mode 0x11 = SCE_STM_R | SCE_STM_F (sifdev.h's stat mode bits: a readable
// file), constant: io_getstat only proves that the file opens.  Callers:
// file_resolve, host_file_size, host_file_exists.
int io_getstat(const char *path, IoStat *out)
{
    int fd;
    long size;      /* 64-bit under ee-gcc: the dsll32/dsra32 */

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
