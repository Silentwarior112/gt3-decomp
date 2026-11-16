/* T497 - the ELF (PJW) hash, of a string and of a byte range; FontDict's
 * bucket function (T496).  T497 has no canonical header: its functions are
 * declared in car_database_internal.h.  0x0023D3D0..0x0023D46C.
 *
 * Data: the frame info only (retail has no .rodata here). */
#include "game/data/car_database_internal.h"

/* hash = (hash << 4) + c; the top nibble is folded into bits 4..7 and
 * cleared. */
// FUNC 0x0023D3D0 hash_string
u32 hash_string(const char *s)
{
    const u8 *p = (const u8 *)s;
    u32 hash = 0;
    u32 c;

    while ((c = *p++) != 0) {
        u32 top;

        hash = (hash << 4) + c;
        top = hash & 0xF0000000;
        if (top != 0)
            hash ^= top >> 24;
        hash &= ~top;
    }
    return hash;
}

/* The same over len bytes.  No callers. */
// FUNC 0x0023D420 hash_bytes
u32 hash_bytes(const u8 *p, int len)
{
    const u8 *end = p + len;
    u32 hash = 0;

    while (p < end) {
        u32 top;

        hash = (hash << 4) + *p++;
        top = hash & 0xF0000000;
        if (top != 0)
            hash ^= top >> 24;
        hash &= ~top;
    }
    return hash;
}
