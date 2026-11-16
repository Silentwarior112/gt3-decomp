/* T006_file_resolve_retry.h - file_resolve_retry (translation unit T006,
 * unit u001), 0x00102960..0x001029E0. */
#ifndef GAME_T006_FILE_RESOLVE_RETRY_H
#define GAME_T006_FILE_RESOLVE_RETRY_H

#include "game/game.h"

/* Resolves path in the gt3.vol volume (file_locate with vol_device_object,
 * T502) until the lookup stops failing (-1): *lsn its first sector,
 * *packedSize the bytes stored, hostName the host: file ("" on the disc).
 * Callers: mpegif (T529) preload / startThread / threadMain. */
void file_resolve_retry(const char *path, int *lsn, u32 *packedSize, char *hostName);    /* 0x00102960 */

#endif /* GAME_T006_FILE_RESOLVE_RETRY_H */
