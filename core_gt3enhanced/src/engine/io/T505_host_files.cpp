/* T505 - host: file helpers (u111).  0x00241E08..0x00242058.
 * The same machine code as the bootstrap's (src/imageloader_source.cpp
 * 0x01005970..0x01005BC0); the sources below are the bootstrap's, with the
 * core addresses.  vol_mount (type 2) reads the host: volume file with
 * host_read_file and sizes it with host_file_size.
 */
#include "engine/io/file_loader_internal.h"
#include "engine/io/T505_host_files.h"

// FUNC 0x00241E08 host_read_file
//   fd = host_open("host:" + path, SCE_RDONLY); fd < 0 -> NULL.
//   avail = host_lseek(fd, 0, END) - offset (unsigned, no error check);
//   host_lseek(fd, offset, SET); len = (len == 0 || avail < len) ? avail : len
//   (sltu).  buf == NULL: buf = malloc(len), and if that fails the read is
//   skipped.  got = host_read(fd, buf, len); host_close(fd).  Returns buf iff
//   got == len, else NULL (a buffer malloc'ed here then leaks).  Callers:
//   vol_mount (0x0023DE10), 0x001A8CE8.
void *host_read_file(const char *path, void *buf, u32 offset, u32 len)
{
    char hostPath[128];                         /* sp+0x00 */

    strcpy(hostPath, "host:");
    strcat(hostPath, path);
    int fd = host_open(hostPath, SCE_RDONLY);
    if (fd < 0)
        return NULL;

    u32 avail = (u32)host_lseek(fd, 0, SCE_SEEK_END) - offset;
    host_lseek(fd, (int)offset, SCE_SEEK_SET);
    if (len == 0 || avail < len)
        len = avail;

    /* `got' ($s5) is left uninitialised on the malloc-failure path, as in
     * the original: buf is NULL there, so the result is NULL whatever `got'
     * holds. */
    u32 got;
    if (buf == NULL)
        buf = malloc(len);
    if (buf != NULL)
        got = (u32)host_read(fd, buf, (int)len);
    host_close(fd);
    return (got == len) ? buf : NULL;
}

// FUNC 0x00241F10 host_write_file
//   fd = host_open("host:" + path, SCE_WRONLY | SCE_CREAT | SCE_TRUNC
//   (0x602)); fd < 0 -> false.  host_write(fd, buf, len) (result ignored),
//   host_close(fd), true.
bool host_write_file(const char *path, const void *buf, u32 len)
{
    char hostPath[128];                         /* sp+0x00 */

    strcpy(hostPath, "host:");
    strcat(hostPath, path);
    int fd = host_open(hostPath, SCE_WRONLY | SCE_CREAT | SCE_TRUNC);
    if (fd < 0)
        return false;
    host_write(fd, buf, (int)len);
    host_close(fd);
    return true;
}

// FUNC 0x00241FA0 host_file_size
//   st at sp+0 (16 bytes reserved), "host:" + path at sp+0x10.  Returns
//   st.size if io_getstat(hostPath, &st) >= 0 (open + lseek END + close),
//   else 0.
u32 host_file_size(const char *path)
{
    IoStat st;                                  /* sp+0x00 */
    char hostPath[128];                         /* sp+0x10 */

    strcpy(hostPath, "host:");
    strcat(hostPath, path);
    if (io_getstat(hostPath, &st) < 0)
        return 0;
    return (u32)st.size;
}

// FUNC 0x00242000 host_file_exists
//   Whether "host:" + path can be opened (io_getstat(hostPath, &st) >= 0);
//   st is discarded.  No callers in the core.
bool host_file_exists(const char *path)
{
    IoStat st;                                  /* sp+0x00 */
    char hostPath[128];                         /* sp+0x10 */

    strcpy(hostPath, "host:");
    strcat(hostPath, path);
    if (io_getstat(hostPath, &st) < 0)
        return false;
    return true;
}
