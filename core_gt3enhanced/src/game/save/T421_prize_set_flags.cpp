/* T421 (0x00215D00..0x00215EF8): the prize sets won (GameStatus+0x2EE60), one
 * byte per set of the product database (product_config, by prizeSetIndex).
 * Only the single-car sets are flagged (PrizeCarList skips a won one). */
#include "game/save/prize_records_internal.h"
#include "game/save/T421_prize_set_flags.h"

// FUNC 0x00215D00 PrizeSetFlags::PrizeSetFlags
PrizeSetFlags::PrizeSetFlags()
{
}

// FUNC 0x00215D08 PrizeSetFlags::clear
void PrizeSetFlags::clear()
{
    for (int i = 0; i < (int)sizeof(won); i++)   /* loop.c reverses it (bgez) */
        won[i] = 0;
}

// FUNC 0x00215D38 PrizeSetFlags::set
void PrizeSetFlags::set(const char *set)
{
    won[product_config()->prizeSetIndex(set)] = 1;
}

// FUNC 0x00215D80 PrizeSetFlags::isSet
int PrizeSetFlags::isSet(const char *set)
{
    return won[product_config()->prizeSetIndex(set)] != 0;
}

/* No caller. */
// FUNC 0x00215DC8 PrizeSetFlags::carCode
const char *PrizeSetFlags::carCode(const char *set, int i)
{
    return product_config()->prizeCarCode(set, i, NULL);
}

/* No caller.  The flags won (of the first 0xC5 only) over the number of
 * single-car prize sets (ProductConfig::singleCarPrizeSetCount). */
// FUNC 0x00215E10 PrizeSetFlags::ratio
f32 PrizeSetFlags::ratio()
{
    int wonCount = 0;
    for (int i = 0; i < 0xC5; i++) {
        if (won[i] != 0)
            wonCount++;
    }
    return (f32)wonCount / (f32)product_config()->singleCarPrizeSetCount();
}

// FUNC 0x00215E88 PrizeSetFlags::getSize
int PrizeSetFlags::getSize()
{
    return sizeof(PrizeSetFlags);               /* 0xC8 */
}

// FUNC 0x00215E90 PrizeSetFlags::load
const u8 *PrizeSetFlags::load(const u8 *src, int arg)
{
    u32 size = 0xC8;                            /* a variable: retail calls memcpy */

    memcpy(this, src, size);
    return src + size;
}

// FUNC 0x00215EC0 PrizeSetFlags::save
u8 *PrizeSetFlags::save(u8 *dst)
{
    u32 size = 0xC8;

    memcpy(dst, this, size);
    return dst + size;
}
