/* T006 - file_resolve_retry.  0x00102960..0x001029E0. */
#include "game/app/T006_file_resolve_retry.h"
#include "engine/io/T502_file_resolve.h"         /* file_locate */
#include "engine/io/T503_vol_thread.h"           /* vol_device_object */

/* Looks path up in the gt3.vol volume (vol_device_object) until the lookup
 * stops failing (-1): *lsn its first sector, *packedSize the bytes stored,
 * hostName the host: file.  Callers: mpegif (T529) preload / startThread /
 * threadMain (0x0025E710, 0x0025EAC0, 0x0025EE38). */
// FUNC 0x00102960 file_resolve_retry
void file_resolve_retry(const char *path, int *lsn, u32 *packedSize, char *hostName)
{
    while (file_locate(path, hostName, lsn, packedSize, &vol_device_object) == -1)
        ;
}
