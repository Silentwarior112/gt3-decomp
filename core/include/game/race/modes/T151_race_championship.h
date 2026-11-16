/* T151_race_championship.h - RaceChampionshipInformation (a GT mode
 * championship), RaceChampionship (the race mode of a championship race)
 * and ChampionshipEntry (translation unit T151, unit u050),
 * 0x0018DD80..0x0018F3E0.
 *
 * RaceChampionshipInformation keeps the championship: the global and the
 * current race codes (the codes end in the session number, two digits; the
 * number of sessions is in the two digits before them), the session
 * counter, the 8 cars (entries[0] the player) and their points; it can be
 * suspended into the save data (GameStatus+0x2EF28, T422) and resumed.
 * run_championship (T053) drives it.
 *
 * RaceChampionship is a RaceSinglePlayer registered in the race class
 * registry as "RaceChampionship" (RaceClassInfo 0x002BAD10
 * {"RaceChampionship", 0x34600, RaceChampionship_create,
 * RaceChampionship_construct}, node 0x00351F88 gp-0x5A68;
 * race_championship_static_ctor, .ctors 0x0028F128): with the car icons,
 * the championship result board, a RacePause and a ReplayPause menu and the
 * DynamicsConductorSinglePlayer (RaceBase::dc).
 *
 * The virtual methods keep the slot names of the bases (RaceInformation
 * T124, RaceBase T129, RacePS2Base T130, RaceSinglePlayer T133).
 *
 * The plan put four globals of other translation units here: the start
 * cameras g_00351F94 and __tit9UnitArena1Ui64 (T153),
 * __ti20RaceDisplayDiffEvent (T194) and __ti14_UnitArenaBase (T471) are
 * declared by their owners now. */
#ifndef GAME_T151_RACE_CHAMPIONSHIP_H
#define GAME_T151_RACE_CHAMPIONSHIP_H

#include "game/game.h"
#include <stddef.h>                             /* size_t */
#include "game/race/core/T152_race_gtmode_information.h"  /* base RaceGTmodeInformation */
#include "game/data/T482_car_equipments.h"           /* CarEquipments (held by value) */
#include "game/race/modes/T133_race_single_player.h"       /* base RaceSinglePlayer */
#include "game/car/model/T184_car_icon_maker.h"           /* CarIcon (held by value) */
/* The pause headers before the result header: every race original that
 * sees both defines PauseMenu / ReplayPauseMenu before ResultTitleBar /
 * ResultTermBar (type names Term, Title, ReplayPauseMenu, PauseMenu ..),
 * u042 data pass. */
#include "game/race/pause/T169_race_pause.h"               /* RacePause (held by value) */
#include "game/race/pause/T170_replay_pause.h"             /* ReplayPause (held by value) */
#include "game/menu/records/T118_result_championship.h"      /* ResultChampionship (held by value) */
#include "game/race/dynamics/T316_dc_single_player.h"         /* DynamicsConductorSinglePlayer (held by value) */

class RaceBase;                                 /* T129 */
class RaceOrganization;                         /* T125 (pointers only) */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
/* (__si_type_info 0x004B2130, base RaceSinglePlayer 0x004B1EF0) */
const type_info &__tf16RaceChampionship(void);             /* 0x0018F300 */
/* (__si_type_info 0x004B2120, base RaceGTmodeInformation 0x004B2140) */
const type_info &__tf27RaceChampionshipInformation(void);  /* 0x0018F390 */
}
#endif

/* (? name, u050; u015: RaceEntryInformation) One car of a championship.
 * Not polymorphic (its implicit copy constructor copies the CarEquipments
 * and the three words; getEntryByOrder returns one by value).  0x1F8
 * bytes (CarEquipments is 8-byte aligned). */
struct ChampionshipEntry {
    CarEquipments car;                          /* 0x000 */
    int points;                                 /* 0x1E8 */
    int order;                                  /* 0x1EC the grid / finishing order (-1: none) */
    /* 0x1F0 (u015) the player's line of the standings.  A bool (egcs's is 4
     * bytes, with its own alias set): retail's copy in getEntryByOrder loads
     * it ahead of the order's store and CarEquipments::presetCar's (a bool
     * too) first.  GCC 14 keeps an int (as presetCar; the values are 0 / 1). */
#ifdef BOOT_OLD_GCC
    bool highlight;
#else
    int highlight;
#endif
};
EGCS_SIZE(ChampionshipEntry, 0x1F8);

/* vtable 0x0032D6E0 (15 slots; 0, 9 and 11 its own), typeinfo
 * __tf27RaceChampionshipInformation 0x0018F390.  vptr +0x54
 * (RaceInformation's); 0x1258 bytes.  The accessors from getSessionNumber
 * on are named by the symbols (? isLastSession); they are inline. */
class RaceChampionshipInformation : public RaceGTmodeInformation {
public:
    /* The 8 entries (no points, no order); the counters cleared.  Caller:
     * run_championship (T053). */
    RaceChampionshipInformation();              /* 0x0018DD80 */
    virtual ~RaceChampionshipInformation();     /* [ 0] +0x0C 0x0018DEB0 */
    /* The points of entry i (RaceInformation's slot 9 takes the int too:
     * this one reads $a1).  Called directly by run_championship (T053). */
    virtual int  virtual_9(int i);              /* [ 9] +0x54 0x0018EAA0 */
    /* The first session draws or puts back the opponents, the next ones
     * take the last order. */
    virtual void setupOrganization(RaceOrganization *org);  /* [11] +0x64 0x0018DF38 */

    /* (descriptive names, u050; u017's descriptions in quotes) */
    void applyEntryOrder(RaceOrganization *org);    /* 0x0018DF70 the order of entries 0..5 into the race cars */
    /* entries 0..5 ordered by the cars' best lap (car_best_lap, unsigned,
     * ascending; equal times share a place): "after a free run with a time" */
    void orderByTime(RaceOrganization *org);        /* 0x0018DFA8 */
    /* "sets the race up for the session": the player 6th, entries 1..5 by
     * weight / power (ShowRoomCar, ascending) */
    void orderByPowerWeight(RaceOrganization *org); /* 0x0018E190 */
    /* A copy of the entry of 0..5 whose order is `order' (its highlight
     * flag set for the player); no check when there is none. */
    ChampionshipEntry getEntryByOrder(int order);   /* 0x0018E3F0 */
    void pickEnemies(RaceOrganization *org);        /* 0x0018E580 random opponents into entries[1..n-1] */
    void setupFirstSession(RaceOrganization *org);  /* 0x0018E638 */
    void setupNextSession(RaceOrganization *org);   /* 0x0018E738 */
    void addPoints(RaceOrganization *org);          /* 0x0018E838 "records a finished session" */
    void nextSession();                             /* 0x0018E8D0 the next session's race code */
    int  getPlayerRank(RaceOrganization *org);      /* 0x0018E970 (no callers) */
    /* The race code; unless continued (cont != 0) both codes = code,
     * session 0, `sessions' from the code and no points. */
    void setChampionship(const char *code, int cont);   /* 0x0018E9A8 "selects the championship" */
    void F_0018EA98();                              /* 0x0018EA98 empty, no callers */
    void addEnemyPoints(RaceOrganization *org);     /* 0x0018EAB8 "a session without a time" */
    void suspend();                                 /* 0x0018EB58 "before saving the game": into GameStatus+0x2EF28 */
    void clearSuspend();                            /* 0x0018EC48 "after saving the game" */
    void resume();                                  /* 0x0018EC78 "restores a saved championship" */
    int  isPlayerLeading();                         /* 0x0018EDF0 "1 if the championship was won" */
    /* RaceGTmodeInformation's car and entry 0.  Caller: run_championship (T053). */
    void setPlayerCar(const CarEquipments &c);      /* 0x0018EE30 */
    void setPlayerCar(u64 code);                    /* 0x0018EE70 (no callers) */
    /* Inline (u050's data pass): retail's T151 has their copies after
     * __tf16RaceChampionship and before this class's __tf, in the reverse of
     * this order (egcs writes a class's inline members in the reverse of
     * their declaration order when its key function is in the file: the
     * frame info); T053 expands isLastSession.  No caller calls them. */
    int  IsSelectEnemies() { return selectEnemies; }            /* 0x0018F388 (sic) */
    /* The bool is stored as a word. */
    void setSelectEnemies(bool v) { selectEnemies = v; }        /* 0x0018F380 */
    char *getGlobalRaceCode() { return globalRaceCode; }        /* 0x0018F378 */
    char *getCurrentRaceCode() { return currentRaceCode; }      /* 0x0018F370 */
    /* session >= sessions (signed): `sessions' is the last session's index
     * (setChampionship).  No callers. */
    int  isLastSession() { return !(session < sessions); }      /* 0x0018F358 */
    int  getSessionNumber() { return session; }                 /* 0x0018F350 */

    char globalRaceCode[0x20];                  /* 0x248 the championship's code (u017: champName) */
    char currentRaceCode[0x20];                 /* 0x268 the race of the current session */
    int  session;                               /* 0x288 the current session (0-based) */
    /* the two digits before the session number minus one: the last
     * session's index (isLastSession: session >= sessions) */
    int  sessions;                              /* 0x28C */
    int  selectEnemies;                         /* 0x290 the opponents are chosen */
    ChampionshipEntry entries[8];               /* 0x298 [0]: the player (8-byte aligned) */
};
EGCS_SIZE(RaceChampionshipInformation, 0x1258);

/* vtable 0x0032D518 (56 slots; 0, 8, 24 and 43 its own), typeinfo
 * __tf16RaceChampionship 0x0018F300.  vptr +0x00 (GameObject's); 0x34600
 * bytes (the race classes are 64-byte aligned: T132). */
class RaceChampionship : public RaceSinglePlayer {
public:
    /* The race pool (g_002B3BF0), 16-byte aligned; the size is ignored. */
    static void *operator new(size_t size);     /* 0x0018EEA0 */
    static void operator delete(void *p, size_t size);  /* 0x0018EEC8 */
    /* The placement form of run_championship's alloca'd race (T053: new (p)
     * RaceChampionship, not null-tested, so not <new>'s throw() one).  A
     * member template: an exact form, not the original.  An inline member
     * would be emitted with the vtable in T151 (the key function's file:
     * every inline member), which retail's T151 has not (its frame info;
     * u046 / u050's data passes, as T134's RaceArcade); a template is
     * emitted only where it is instantiated (T053, inline there).
     * RaceChampionship_construct uses the global one of <new> instead
     * (egcs's is throw(): the null test retail has there). */
    template <class T>
    static void *operator new(size_t size, T *p) { return p; }

    /* The members, then dc = &dcSingle (the race mode is not named).
     * Callers: run_championship (T053), create / construct. */
    RaceChampionship();                         /* 0x0018EF88 */
    /* (deleting form: operator delete(this, 0x34600)) */
    virtual ~RaceChampionship();                /* [ 0] +0x00C 0x0018F130 */
    /* Race start: RaceSinglePlayer's, then the race pause menu and the
     * result (a race) or the replay pause menu (a replay). */
    virtual void virtual_8(int replay);         /* [ 8] +0x04C 0x0018F1E0 */
    virtual const char *virtual_24();           /* [24] +0x0CC 0x0018EEF0 "RaceChampionship" */
    /* Before the race runs: the car icons for this organization, given to
     * the result board and to the race display. */
    virtual void virtual_43();                  /* [43] +0x164 0x0018F278 */

    CarIcon            icons[7];                /* 0x16EC0 */
    ResultChampionship result;                  /* 0x2509C the points board */
    RacePause          pause;                   /* 0x25A44 the pause menu of a race */
    ReplayPause        replayPause;             /* 0x25CF0 (mode 0) */
    DynamicsConductorSinglePlayer dcSingle;     /* 0x26230 RaceBase::dc (never destroyed) */
    /*  0x345DC .. 0x345FF: alignment */
};
EGCS_SIZE(RaceChampionship, 0x34600);

/* RaceClassInfo::create of "RaceChampionship": new RaceChampionship. */
RaceBase *RaceChampionship_create(void);                    /* 0x0018EF00 */
/* RaceClassInfo::construct: a RaceChampionship in buf (NULL if buf is
 * NULL); arg is not used. */
RaceBase *RaceChampionship_construct(void *buf, int arg);   /* 0x0018EF60 */
/* .ctors entry 0x0028F128: registers "RaceChampionship". */
void      race_championship_static_ctor(void);              /* 0x0018F2D8 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_info {vptr, name, base} the two __tf functions fill. */
extern "C" {
extern u32 __ti27RaceChampionshipInformation[3];   /* 0x004B2120 (u050 g_004B2120) */
extern u32 __ti16RaceChampionship[3];              /* 0x004B2130 (u050 g_004B2130) */
}
#endif

#endif /* GAME_T151_RACE_CHAMPIONSHIP_H */
