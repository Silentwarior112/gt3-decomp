/* T502 - file_resolve: where a file of the volume is (u111).
 * 0x00240AF0..0x00240FA0.
 *
 * The device the volume is mounted from (VolDevice::type):
 *   0  gt3.vol on the disc: the "RoFS" TOC (vol_toc_lookup), sectors
 *      relative to the volume's first sector;
 *   1  plain ISO 9660 files of the disc (iso9660_find);
 *   2  a host: copy of the volume file: the same TOC, byte offsets
 *      (sector << 11) into "host:" + the volume's name;
 *   3  loose files under "host:data/", with an optional packed "<name>.gz".
 */
#include "engine/io/file_loader_internal.h"
#include "engine/io/T502_file_resolve.h"

/* vol_toc_is_plain (T510 0x0023E0A0) as an inline copy: T510 calls its
 * out-of-line one, T502 has the body inline (its result register copies
 * show the inline's return). */
static inline int toc_is_plain(const VolToc *toc)
{
    const u8 *data = toc->data;
    if (data == NULL)
        return 0;
    int plain = 0;
    if (data[0] == 'R' && data[1] == 'o' && data[2] == 'F')
        plain = data[3] == 'S';
    return plain;
}

/* The entry record's accessors (? names).  The type tests read the record
 * through vol_dir_entry, as store-flags (retail: xori / beqz, the form of a
 * helper's returned comparison); the words through vol_toc_word's body
 * (T510 0x0023E080, inline here): data + (off + k), another address than
 * the type's (retail recomputes it, off first). */
static inline u32 toc_word(const VolToc *toc, int off) { return *(u32 *)(toc->data + off); }
static inline int dir_is_dir(const VolDir *d) { return vol_dir_entry(d)->type == VOL_ENTRY_DIR; }
static inline int dir_is_packed(const VolDir *d) { return vol_dir_entry(d)->type == VOL_ENTRY_PACKED; }
static inline u32 dir_lsn(const VolDir *d) { return toc_word(d->toc, d->off + 4); }
static inline u32 dir_size(const VolDir *d) { return toc_word(d->toc, d->off + 8); }
static inline u32 dir_packed_size(const VolDir *d) { return toc_word(d->toc, d->off + 0xC); }
/* the bytes stored: the packed size of a packed file, else its size */
static inline u32 dir_stored(const VolDir *d) { return dir_is_packed(d) ? dir_packed_size(d) : dir_size(d); }

// FUNC 0x00240AF0 file_resolve
//   Clears *loc, then fills it for `path' on `dev'.  Returns 0 (found; a
//   directory gives flags 2), -1 (not found / no valid TOC / unknown device
//   type) or, for host:data/, io_getstat's result.  Note io_getstat always
//   reports mode 0x11, so the host directory test (SCE_STM_D) never fires.
int file_resolve(const char *path, FileLoc *loc, VolDevice *dev)
{
    int type = dev->type;

    loc->flags = 0;
    loc->packedSize = 0;
    loc->size = 0;
    loc->lsn = 0;

    switch (type) {
    case VOL_DEV_DISC_VOLUME:
    case VOL_DEV_HOST_VOLUME: {
        VolDir dir;                             /* sp+0x00 */

        vol_toc_lookup(&dir, &dev->toc, path);
        if (!toc_is_plain(dir.toc))
            return -1;
        /* the record is re-read through dir.toc->data after the stores into
         * *loc (GCSE does not carry the load over them) */
        if (dir_is_dir(&dir)) {
            loc->flags = FILE_LOC_DIR;
        } else {
            loc->flags = FILE_LOC_FILE;
            loc->packedSize = dir_stored(&dir);
            loc->size = dir_stored(&dir);
            loc->lsn = dir_lsn(&dir);
            if (dir_is_packed(&dir)) {
                loc->flags |= FILE_LOC_PACKED;
                loc->size = dir_size(&dir);
            }
        }
        return 0;
    }
    case VOL_DEV_HOST_FILES: {
        IoStat st;                              /* sp+0x000 (16 bytes) */
        char hostPath[256];                     /* sp+0x010 */
        char packedPath[256];                   /* sp+0x110 */
        int result;

        strcpy(hostPath, "host:data/");
        strcat(hostPath, path);
        result = io_getstat(hostPath, &st);
        if (result >= 0) {
            if (st.mode & SCE_STM_D) {
                loc->flags = FILE_LOC_DIR;
            } else {
                loc->flags = FILE_LOC_FILE;
                loc->packedSize = st.size;
                loc->size = st.size;
                strcpy(packedPath, hostPath);
                strcat(packedPath, ".gz");
                if (io_getstat(packedPath, &st) >= 0) {
                    loc->flags |= FILE_LOC_PACKED;
                    loc->packedSize = st.size;
                }
            }
        }
        return result;
    }
    case VOL_DEV_DISC_FILES: {
        IsoDirRecord *rec = iso9660_find(path);
        if (rec == NULL)
            return -1;
        if (rec->flags & 2) {                   /* ISO 9660: a directory */
            loc->flags = FILE_LOC_DIR;
        } else {
            loc->flags = type;                  /* 1 = FILE_LOC_FILE */
            loc->packedSize = iso_read_u32(&rec->size);
            loc->size = iso_read_u32(&rec->size);
            loc->lsn = iso_read_u32(&rec->lsn);
        }
        return 0;
    }
    default:
        return -1;
    }
}

// FUNC 0x00240DB8 file_locate
//   Resolves `path' (once, -1 if not found) and says how to read it: *lsn =
//   its absolute first sector (type 0: + the volume's), or its byte offset
//   in the host: volume file (type 2: sector << 11), or 0 (type 3);
//   *packedSize = the bytes stored; hostName = the host: file to open (""
//   on the disc).  Callers: file_resolve_retry (0x00102960), 0x0022AC98.
int file_locate(const char *path, char *hostName, int *lsn, u32 *packedSize,
                VolDevice *dev)
{
    FileLoc loc;                                /* sp+0x00 */

    if (file_resolve(path, &loc, dev) == -1)
        return -1;

    hostName[0] = '\0';
    *lsn = loc.lsn;
    *packedSize = loc.packedSize;
    switch (dev->type) {
    case VOL_DEV_DISC_VOLUME: {
        VolExtent extent = dev->extent;         /* sp+0x10, copied by value */
        *lsn += extent.lsn;
        break;
    }
    case VOL_DEV_HOST_VOLUME:
        strcpy(hostName, "host:");
        strcat(hostName, dev->name);
        *lsn <<= VOL_SECTOR_SHIFT;
        break;
    case VOL_DEV_HOST_FILES:
        strcpy(hostName, "host:data/");
        strcat(hostName, path);
        break;
    default:                                    /* 1: the disc, lsn as is */
        break;
    }
    return 0;
}

// FUNC 0x00240F28 file_size_retry
//   The size of a file, retrying file_resolve while it returns -1 (forever
//   for a missing file): the packed size if `packed' and the file is
//   gzip-packed, else the (unpacked) size.
u32 file_size_retry(const char *path, int packed, VolDevice *dev)
{
    FileLoc loc;                                /* sp+0x00 */

    while (file_resolve(path, &loc, dev) == -1)
        ;
    if (packed && (loc.flags & FILE_LOC_PACKED))
        return loc.packedSize;
    return loc.size;
}
