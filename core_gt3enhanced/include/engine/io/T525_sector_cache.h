/* T525_sector_cache.h - the disc sector cache over pdicdvd_read
 * (translation unit T525, unit u119). */
#ifndef GAME_T525_SECTOR_CACHE_H
#define GAME_T525_SECTOR_CACHE_H

#include "game/game.h"

/* Sets the cache up in buf (size bytes): (size - 0x40) / 0x80C sectors, each a
 * 12-byte entry (sorted by lsn) plus 0x800 bytes of 64-byte aligned data.
 * The state block is 0x002FDA40. */
void sector_cache_init(void *buf, u32 size);                /* 0x0025DE20 */
void sector_cache_clear(void);                              /* 0x0025DE70 */
/* The cached sector lsn, read on a miss (pdicdvd_read, mode passed on;
 * full: the entry unused for the longest time is evicted).  NULL: read error. */
u8  *sector_cache_read(u32 lsn, int mode);                  /* 0x0025DE80 */

#endif /* GAME_T525_SECTOR_CACHE_H */
