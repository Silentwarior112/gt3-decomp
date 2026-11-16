/* T434_best_times.h - CourseBestTimes, 44 best times of the save data
 * (translation unit T434, unit u097), 0x00216BB8..0x00216E58.  (? names.)
 *
 * GameStatus+0x23B08: one best time per entry of the T435 database's list at
 * +0x1C (? which list; found by name with ProductConfig::index1C).  The by-name
 * update / get have no callers: GameStatus only constructs, clears, loads
 * and saves the table. */
#ifndef GAME_T434_BEST_TIMES_H
#define GAME_T434_BEST_TIMES_H

#include "game/game.h"

/* One best time.  Size 4 (getSize; 44 of them fill 0xB0). */
class BestTime {
public:
    BestTime();                                 /* 0x00216BB8 */
    void clear();                               /* 0x00216BE0 */
    void update(u32 t);                         /* 0x00216BF0 time = t if better (unsigned compare) */
    u32  get();                                 /* 0x00216C18 */
    static int getSize();                       /* 0x00216C20 4 */
    const u8 *load(const u8 *src, int arg);     /* 0x00216C28 */
    u8 *save(u8 *dst);                          /* 0x00216C38 */

    u32 time;                                   /* 0x0 0x157529FF = none */
};
EGCS_SIZE(BestTime, 4);

/* GameStatus+0x23B08.  No vtable.  Size 0xB0 (getSize; GameStatus's next
 * member, ArcadeRecords, at 0x23BB8). */
class CourseBestTimes {
public:
    CourseBestTimes();                          /* 0x00216C48 */
    void clear();                               /* 0x00216CB0 */
    BestTime *find(const char *name);           /* 0x00216CF0 */
    void update(const char *name, u32 time);    /* 0x00216D38 (no callers) */
    u32  get(const char *name);                 /* 0x00216D68 (no callers) */
    static int getSize();                       /* 0x00216D90 0xB0 */
    const u8 *load(const u8 *src, int arg);     /* 0x00216DB8 arg unused (GameStatus passes 0) */
    u8 *save(u8 *dst);                          /* 0x00216E08 */

    BestTime t[44];                             /* 0x00 [ProductConfig::index1C] */
};
EGCS_SIZE(CourseBestTimes, 0xB0);

#endif /* GAME_T434_BEST_TIMES_H */
