/* T418 (0x00213E90..0x00213ED8): BestPlace, the best place taken in a
 * ranking (T417 time trial courses, T424, T425). */
#include "game/save/prize_records_internal.h"
#include "game/save/T418_best_place.h"

// FUNC 0x00213E90 BestPlace::update
int BestPlace::update(int p)
{
    if (p < place) {
        place = p;
        return 1;
    }
    return 0;
}

// FUNC 0x00213EB0 BestPlace::getSize
int BestPlace::getSize()
{
    return 4;
}

// FUNC 0x00213EB8 BestPlace::load
const u8 *BestPlace::load(const u8 *src, int arg)
{
    place = *(const int *)src;
    return src + 4;
}

// FUNC 0x00213EC8 BestPlace::save
u8 *BestPlace::save(u8 *dst)
{
    *(int *)dst = place;
    return dst + 4;
}
