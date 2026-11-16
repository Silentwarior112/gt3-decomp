/* T525 - sector cache.  0x0025DE20..0x0025E100.
 *
 * A cache of 0x800-byte disc sectors over pdicdvd_read, set up by vol_init
 * in a caller-given buffer: `capacity' entries of 12 bytes (sorted by lsn,
 * looked up by binary search) followed by the 64-byte aligned sector data.
 * When full, the entry unused for the longest time (clock - stamp) is
 * evicted. */
#include "engine/io/T525_sector_cache.h"
#include "engine/pdi/pdi_internal.h"

/* The file's variables: public C++ globals without an initialiser, which
 * egcs puts in .data (zeros) at the end of the file's output, in definition
 * order, before the frame info (0x002FEA80).  The sector buffers are 64-byte
 * aligned (SIF DMA), so the object's .data starts at 0x002FDA40, after 0x20
 * bytes of padding behind T524's frame info. */
SectorCache g_002FDA40;                                     /* 0x002FDA40 */
/* Unreferenced: a sector buffer no code of the image uses. */
u8 g_002FDA80[SECTOR_SIZE] __attribute__((aligned(64)));   /* 0x002FDA80 */
/* The directory sector iso9660_find_in_dir (T526) searches. */
u8 g_002FE280[SECTOR_SIZE] __attribute__((aligned(64)));   /* 0x002FE280 */

/* Caller: vol_init (0x00241A68).  Each sector costs 0x80C bytes (entry +
 * data), 0x40 bytes are kept for the alignment of the data. */
// FUNC 0x0025DE20 sector_cache_init
void sector_cache_init(void *buf, u32 size)
{
    SectorCache *cache = &g_002FDA40;
    u32 capacity;

    cache->used     = 0;
    capacity = (size - 0x40) / (sizeof(SectorCacheEntry) + SECTOR_SIZE);
    cache->entries  = (SectorCacheEntry *)buf;
    cache->capacity = capacity;
    /* the data: 64-byte aligned after the entries (int mask: addiu -0x40) */
    cache->data     = (u8 *)(((int)((SectorCacheEntry *)buf + capacity) + 0x3F) & ~0x3F);
}

/* Caller: iso9660_find, when the volume descriptor cannot be read. */
// FUNC 0x0025DE70 sector_cache_clear
void sector_cache_clear(void)
{
    g_002FDA40.used = 0;
}

/* Returns the cached sector lsn, reading it (pdicdvd_read, `mode' is passed
 * on) on a miss; NULL when the read fails.  Callers: iso9660_read_pvd (mode
 * 0), iso9660_find_in_dir (mode 1). */
// FUNC 0x0025DE80 sector_cache_read
/* Shape from the listing: the hit is handled after the miss (its block is
 * the function's last), the clock is read where it is used, the used count
 * read again after the search (a copy of the first load), one index for the
 * scans and one for the moves.  93%: the search's lo / hi and the count
 * after it take other registers than retail's (lo $a2, hi the load's $t0,
 * the copy $t5), the same with the reference compiler 991111-01. */
u8 *sector_cache_read(u32 lsn, int mode)
{
    SectorCache      *cache   = &g_002FDA40;
    SectorCacheEntry *entries = cache->entries;
    SectorCacheEntry  fresh;
    SectorCacheEntry *slot;
    int used;
    int lo = 0, hi = cache->used;
    int i, j;

    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        slot = &entries[mid];
        if (lsn == slot->lsn)
            goto hit;
        if (lsn < slot->lsn)
            hi = mid;
        else
            lo = mid + 1;
    }

    /* miss */
    used = cache->used;
    fresh.lsn   = lsn;
    fresh.stamp = cache->clock++;
    if (cache->capacity != used) {
        fresh.data = cache->data + used * SECTOR_SIZE;
    } else {
        /* full: reuse the sector of the oldest entry (unsigned ages; with
         * capacity 0, fresh.data stays unset) */
        u32 oldestAge = 0;
        j = 0;
        for (i = 0; i < used; i++) {
            u32 age = cache->clock - entries[i].stamp;
            if (oldestAge < age) {
                fresh.data = entries[i].data;
                oldestAge = age;
                j = i;
            }
        }
        used--;
        /* Quirk (? off by one): stops at used - 1, so entries[used] is not
         * moved down. */
        for (; j < used - 1; j++)
            entries[j] = entries[j + 1];
    }
    for (i = 0; i < used && entries[i].lsn <= fresh.lsn; i++)
        ;
    slot = &entries[i];
    /* Quirk (? off by one): starts at used - 1, so the old entries[used - 1]
     * is lost and entries[used] is left as it was (unless i == used:
     * appending, the order the iso9660 lookups mostly read in). */
    for (j = used - 1; j > i; j--)
        entries[j] = entries[j - 1];
    *slot = fresh;
    cache->used = used + 1;
    if (pdicdvd_read(slot->data, lsn, SECTOR_SIZE, mode))
        return slot->data;
    slot->lsn   = 0xFFFFFFFF;
    slot->stamp = cache->clock + 1;
    return NULL;

hit:
    slot->stamp = cache->clock++;
    return slot->data;
}
