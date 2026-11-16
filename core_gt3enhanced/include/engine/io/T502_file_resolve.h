/* T502_file_resolve.h - where a file of the volume is (translation unit T502,
 * unit u111), 0x00240AF0..0x00240FA0.
 *
 * The device the volume is mounted from (VolDevice::type):
 *   0  gt3.vol on the disc: the "RoFS" TOC (vol_toc_lookup), sectors relative
 *      to the volume's first sector;
 *   1  plain ISO 9660 files of the disc (iso9660_find);
 *   2  a host: copy of the volume file: the same TOC, byte offsets
 *      (sector << 11) into "host:" + the volume's name;
 *   3  loose files under "host:data/", with an optional packed "<name>.gz". */
#ifndef GAME_T502_FILE_RESOLVE_H
#define GAME_T502_FILE_RESOLVE_H

#include "game/game.h"

class VolDevice;                                /* T510 (pointers only) */

/* Where a file is (file_resolve's result, on the callers' stacks). */
struct FileLoc {
    u32 flags;                                  /* 0x0 1 a file, 2 a directory, | 4 gzip-packed */
    u32 packedSize;                             /* 0x4 bytes stored on the device (== size unless packed) */
    u32 size;                                   /* 0x8 bytes once unpacked */
    u32 lsn;                                    /* 0xC first sector (relative to the volume; host: 0) */
};
EGCS_SIZE(FileLoc, 0x10);                       /* file_locate: loc sp+0x00, the next local sp+0x10 */

/* Clears *loc, then fills it for path on dev.  0: found (a directory gives
 * flags 2), -1: not found / no valid TOC / unknown device type; type 3:
 * io_getstat's result. */
int file_resolve(const char *path, FileLoc *loc, VolDevice *dev);      /* 0x00240AF0 */
/* Resolves path (once; -1 if not found) and says how to read it: *lsn = its
 * absolute first sector (type 0: + the volume's), its byte offset in the
 * host: volume file (type 2) or 0 (type 3); *packedSize = the bytes stored;
 * hostName = the host: file to open ("" on the disc).  0. */
int file_locate(const char *path, char *hostName, int *lsn, u32 *packedSize,
                VolDevice *dev);                /* 0x00240DB8 */
/* The size of a file, retrying file_resolve while it fails (forever for a
 * missing file): the packed size if packed and the file is gzip-packed,
 * else the unpacked size. */
u32 file_size_retry(const char *path, int packed, VolDevice *dev);      /* 0x00240F28 */

#endif /* GAME_T502_FILE_RESOLVE_H */
