/* T510_vol_device.h - VolDevice, a mounted volume (translation unit T510, unit u110),
 * 0x0023DCA8..0x0023E0F0. */
#ifndef GAME_T510_VOL_DEVICE_H
#define GAME_T510_VOL_DEVICE_H

#include "game/game.h"
#include "engine/io/T500_vol_toc.h"                  /* VolToc, held by value */

/* Where the volume file is: filled by vol_mount, copied by value by T502
 * (file_locate: a 16-byte block copy, so a struct of its own). */
struct VolExtent {
    s32 mounted;                                /* 0x0 1 once mounted (types 0 and 2) */
    u32 size;                                   /* 0x4 the volume file's size */
    u32 size2;                                  /* 0x8 the same size again */
    u32 lsn;                                    /* 0xC first sector of the volume file (0 on host:) */
};
EGCS_SIZE(VolExtent, 0x10);

/* VolDevice::type.  An enum in the original: file_locate (T502, 0x00240DB8)
 * loads the type after its int store through lsn but before its u32 store
 * through packedSize, which only a type of its own (another alias set than
 * int's) allows. */
enum VolDeviceType {
    VOL_DEV_DISC_VOLUME = 0,                    /* gt3.vol on the disc (an ISO 9660 file) */
    VOL_DEV_DISC_FILES  = 1,                    /* plain ISO 9660 files of the disc */
    VOL_DEV_HOST_VOLUME = 2,                    /* a host: copy of the volume file */
    VOL_DEV_HOST_FILES  = 3                     /* loose host:data/ files */
};

/* A volume, linked at the head of the device list (0x00352F58, gp-0x4A98) by
 * its constructor and unlinked by its destructor (interrupts off).
 * vol_device_object (0x002F33B0, T503) is the gt3.vol one.  Not polymorphic.
 * The members end at 0x2C (vol_thread_object follows at +0x30: egcs aligns
 * data objects to 8). */
class VolDevice {
public:
    VolDevice();                                /* 0x0023DD18 links it, toc(), toc_buf = NULL */
    ~VolDevice();                               /* 0x0023DD80 freeToc, unlinks it */
    void freeToc();                             /* 0x0023E048 free(toc_buf), toc_buf = NULL */

    VolDevice  *prev;                           /* 0x00 */
    VolDevice  *next;                           /* 0x04 */
    VolDeviceType type;                         /* 0x08 VOL_DEV_*: 0 gt3.vol on the disc (ISO 9660 file),
                                                   1 plain disc files, 2 a host: volume file, 3 loose
                                                   host:data/ files */
    const char *name;                           /* 0x0C the volume file's name */
    VolToc      toc;                            /* 0x10 its TOC (vol_toc_lookup's root) */
    VolExtent   extent;                         /* 0x18 */
    u8         *toc_buf;                        /* 0x28 the memalign(0x40) TOC buffer */
};

/* Mounts the volume file name as type 0 (on the disc: iso9660_find, retried
 * until found) or 2 (a host: file); the TOC is nsect sectors (0: from the
 * header's sizes), read into toc_buf, decoded if needed, cwd = the root.
 * Other types: only type and name are stored. */
void vol_mount(VolDevice *dev, int type, const char *name, u32 nsect);  /* 0x0023DE10 */

#endif /* GAME_T510_VOL_DEVICE_H */
