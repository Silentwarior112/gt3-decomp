/* T421_prize_set_flags.h - PrizeSetFlags, the prize sets won (translation
 * unit T421, unit u096), 0x00215D00..0x00215EF8.
 *
 * GameStatus+0x2EE60: one byte per prize set of the T435 database (by its
 * index, F_0021DC38), set once the set has been won.  car_get_prizes (T061)
 * flags the single-car sets only, and PrizeCarList no longer gives a flagged
 * one; ratio() divides by the number of single-car sets. */
#ifndef GAME_T421_PRIZE_SET_FLAGS_H
#define GAME_T421_PRIZE_SET_FLAGS_H

#include "game/game.h"

/* No vtable.  Size 0xC8 (getSize; GameStatus's next member,
 * ChampionshipSave, at 0x2EF28). */
class PrizeSetFlags {
public:
    PrizeSetFlags();                            /* 0x00215D00 */
    void clear();                               /* 0x00215D08 */
    void set(const char *set);                  /* 0x00215D38 */
    int  isSet(const char *set);                /* 0x00215D80 */
    const char *carCode(const char *set, int i); /* 0x00215DC8 */
    f32  ratio();                               /* 0x00215E10 sets won / ProductConfig::singleCarPrizeSetCount */
    static int getSize();                       /* 0x00215E88 0xC8 */
    const u8 *load(const u8 *src, int arg);     /* 0x00215E90 */
    u8 *save(u8 *dst);                          /* 0x00215EC0 */

    s8 won[0xC8];                               /* 0x00 [F_0021DC38 index] */
};
EGCS_SIZE(PrizeSetFlags, 0xC8);

#endif /* GAME_T421_PRIZE_SET_FLAGS_H */
