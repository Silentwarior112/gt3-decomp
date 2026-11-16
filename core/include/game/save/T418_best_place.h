/* T418_best_place.h - BestPlace, the best place taken in a ranking
 * (translation unit T418, unit u096), 0x00213E90..0x00213ED8.
 *
 * One int: the best place (0 = won, lower is better, 8 = never raced).
 * Embedded in the time trial courses (T417), the arcade records (T424) and
 * the GT mode event results (T425).  Part of the save data: like every
 * save-data class, a static getSize(), load(src, arg) (returns the end of
 * what it read; arg is unused) and save(dst) (returns the end). */
#ifndef GAME_T418_BEST_PLACE_H
#define GAME_T418_BEST_PLACE_H

#include "game/game.h"

/* No vtable.  Size 4 (getSize, the 0x14-byte ArcadeRecord). */
class BestPlace {
public:
    /* inline: the member constructors of TimeTrialCourse (T417),
     * ArcadeRecord (T424) and EventResult (T425) store 8 */
    BestPlace() : place(8) {}
    void clear() { place = 8; }                 /* inline (T424, T425; ? name) */
    int  get() const { return place; }          /* inline (T424, T425) */

    int update(int p);                          /* 0x00213E90 place = p if better; 1 if so */
    static int getSize();                       /* 0x00213EB0 4 */
    const u8 *load(const u8 *src, int arg);     /* 0x00213EB8 */
    u8 *save(u8 *dst);                          /* 0x00213EC8 */

    int place;                                  /* 0x0 */
};
EGCS_SIZE(BestPlace, 4);

#endif /* GAME_T418_BEST_PLACE_H */
