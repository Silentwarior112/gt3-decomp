/* T422_championship_save.h - ChampionshipSave, the suspended championship of
 * the save data (translation unit T422, unit u096), 0x00215EF8..0x00216290.
 *
 * GameStatus+0x2EF28: whether a championship is suspended, the menu page to
 * go back to, the race codes, the session, and the 8 entrants' cars and
 * points (ChampionshipSaveEntry, declared in T417).  Saved: the first 0x8C
 * bytes, then per entry the car's 0x1D0 saved bytes and the points. */
#ifndef GAME_T422_CHAMPIONSHIP_SAVE_H
#define GAME_T422_CHAMPIONSHIP_SAVE_H

#include "game/game.h"
#include "game/save/T417_time_trial_records.h"       /* ChampionshipSaveEntry (by value) */

class CarEquipments;

/* No vtable.  Size 0x1010 (0x90 + 8 * 0x1F0; GameStatus's next member,
 * GameOptions, at 0x2FF38). */
class ChampionshipSave {
public:
    ChampionshipSave();                         /* 0x00215EF8 the entries' cars first, then clear() */
    void clear();                               /* 0x00216010 */
    void setPage(const char *s);                /* 0x00216058 */
    void setGlobalRaceCode(const char *s);      /* 0x00216078 */
    void setCurrentRaceCode(const char *s);     /* 0x00216098 */
    void setSession(int v);                     /* 0x002160B8 */
    void setSessions(int v);                    /* 0x002160C0 */
    void setCar(const CarEquipments *car, int i); /* 0x002160C8 entries[i].car = *car */
    void setPoints(int points, int i);          /* 0x002160F0 */
    char *getPage();                            /* 0x00216108 */
    char *getGlobalRaceCode();                  /* 0x00216110 */
    char *getCurrentRaceCode();                 /* 0x00216118 */
    int  getSession();                          /* 0x00216120 */
    int  getSessions();                         /* 0x00216128 */
    CarEquipments *getCar(int i);               /* 0x00216130 */
    int  getPoints(int i);                      /* 0x00216148 */
    static int getSize();                       /* 0x00216160 0xF2C = 0x8C + 8 * (0x1D0 + 4) */
    const u8 *load(const u8 *src, int arg);     /* 0x00216168 */
    u8 *save(u8 *dst);                          /* 0x002161F8 */

    int  active;                                /* 0x000 a championship is suspended */
    char page[0x40];                            /* 0x004 the menu page to go back to */
    char globalRaceCode[0x20];                  /* 0x044 */
    char currentRaceCode[0x20];                 /* 0x064 */
    int  session;                               /* 0x084 */
    int  sessions;                              /* 0x088 */
    u8   unk_08C[4];                            /* 0x08C ? (not saved) */
    ChampionshipSaveEntry entries[8];           /* 0x090 */
};
EGCS_SIZE(ChampionshipSave, 0x1010);

#endif /* GAME_T422_CHAMPIONSHIP_SAVE_H */
