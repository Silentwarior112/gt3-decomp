/* T425_event_results.h - EventResults, the GT mode event results of the save
 * data (translation unit T425, unit u097), 0x00216E58..0x00217538.
 *
 * GameStatus+0x23510: the best place (BestPlace, T418: 0 = won, 8 = never
 * raced) of each of the 382 events of the T435 database's list at +0x18
 * (sorted names, found with ProductConfig::eventIndex), and the "series complete" checks
 * used by the ending (T016), the arcade item factories (T030) and the prize
 * cars (T419).  The league of an event is the byte +0x1E5 of its race data
 * (RaceEvent::difficulty, T484). */
#ifndef GAME_T425_EVENT_RESULTS_H
#define GAME_T425_EVENT_RESULTS_H

#include "game/game.h"
#include "game/save/T418_best_place.h"               /* BestPlace (by value) */

/* One event.  Size 4 (getSize; 382 of them fill 0x5F8). */
class EventResult {
public:
    EventResult();                              /* 0x00216E58 only the BestPlace constructor (8) */
    void clear();                               /* 0x00216E68 */
    static int getSize();                       /* 0x00216E78 4 */
    const u8 *load(const u8 *src, int arg);     /* 0x00216E98 */
    u8 *save(u8 *dst);                          /* 0x00216EB8 */

    BestPlace place;                            /* 0x0 */
};
EGCS_SIZE(EventResult, 4);

/* GameStatus+0x23510.  No vtable.  Size 0x5F8 (getSize; GameStatus's next
 * member, CourseBestTimes (T434), at 0x23B08). */
class EventResults {
public:
    EventResults();                             /* 0x00216ED8 */
    void clear();                               /* 0x00216F40 */
    int  wins();                                /* 0x00216F80 events won */
    int  count();                               /* 0x00216FE0 events (of the database) */
    EventResult *find(const char *event);       /* 0x00217008 */
    int  updatePlace(const char *event, int p); /* 0x00217050 1 if p is a new best place */
    int  getPlace(const char *event);           /* 0x00217080 0 = won */
    /* Only the codes ending in "00xx" or "00" are looked at; a "..00" code
     * takes the league of its first race ("..01"). */
    int  leagueWon(int league);                 /* 0x002170A0 every event of the league won */
    int  beginnerLeagueWon();                   /* 0x002172C8 league 1 */
    int  amateurLeagueWon();                    /* 0x002172E8 league 2 */
    int  professionalLeagueWon();               /* 0x00217308 league 3 */
    int  kindWon(int kind);                     /* 0x00217328 every event race_name_split classes as `kind' won */
    int  dirtEventsWon();                       /* 0x002173F0 kind 8: the "D.." (dirt: rally) events (T419 prize reason 16) */
    int  enduranceEventsWon();                  /* 0x00217410 kind 7: the "E.." (endurance) events (T419 prize reason 17) */
    int  allWon();                              /* 0x00217430 */
    static int getSize();                       /* 0x00217470 0x5F8 */
    const u8 *load(const u8 *src, int arg);     /* 0x00217498 arg unused (GameStatus passes 0) */
    u8 *save(u8 *dst);                          /* 0x002174E8 */

    EventResult e[382];                         /* 0x000 [ProductConfig::eventIndex] */
};
EGCS_SIZE(EventResults, 0x5F8);

#endif /* GAME_T425_EVENT_RESULTS_H */
