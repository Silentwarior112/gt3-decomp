/* T102_race_menu_helpers.h - running a short menu over a race, and the
 * race-result helpers of the race modes (translation unit T102, unit u015),
 * 0x0012E2C8..0x0012F840.
 *
 * run_short_menu / run_short_menu_license run one short-menu session (the
 * quick menu of the race modes: pause, result, retry) over a race that
 * keeps running and is drawn behind the menu, and return the action code
 * the page chose (QM_*, 1 if none).  A session: args->result = 1 and
 * g_003514F0 = args; a ShortmenuControl (T111; the licence variant
 * ShortmenuLicenseControl, T103) on the stack loads params->file, the menu
 * heap is a 0x40040-byte alloca aligned to 64 bytes, the page params->page
 * runs (startPage, if any, opens over it) until the page manager is done.
 * The rest works on the race's RaceOrganization (T125): start and result
 * orders, the result lines of ShortMenuArgs, the licence and event results
 * (GameStatus, T429), the car condition (odometer, oil) moved between the
 * garage car and the race entry, the memory card screens and the ranking
 * code of a record. */
#ifndef GAME_T102_RACE_MENU_HELPERS_H
#define GAME_T102_RACE_MENU_HELPERS_H

#include "game/game.h"
#include "game/race/core/T128_race_lap_record.h"      /* RACE_TIME_INVALID */
#include "game/save/T420_license_records.h"      /* LicenseRecords (LicenseStatus) */
#include "engine/pdistd/T471_unit_arena.h"           /* _UnitArenaBase (BlockPool) */

class RaceInformation;                      /* T124 (pointers only) */
class RaceOrganization;                     /* T125 */
class RacePS2Base;                          /* T130 */
class RaceSolitaire;                        /* T139 */
class RaceChampionshipInformation;          /* T151 */
class GarageCar;                            /* T036 */

/* Other names of canonical classes (the first pass's, kept for the units
 * that use them; do not forward-declare these names). */
typedef LicenseRecords LicenseStatus;       /* GameStatus's licence results (u009, u015, u017, u109) */
typedef _UnitArenaBase BlockPool;           /* the course data's unit arena (u015, u017, u032):
                                               RaceOrganization unloadCourseData returns it */

/* The action codes the qm_* actions (T113) store in ShortMenuArgs::result;
 * the race modes (T052 ..) act on them. */
enum {
    QM_EXIT         = 1,                    /* also the result when no action ran */
    QM_REPLAY       = 2,
    QM_TRY_AGAIN    = 3,
    QM_SETTING      = 4,
    QM_FREE_RUN     = 5,
    QM_GO_RACE      = 6,                    /* "qm_go_race" and "qm_start" */
    QM_NEXT_SESSION = 7,
    QM_OPTION       = 8,
    QM_SAVE_REPLAY  = 9,
    QM_LOAD_GHOST   = 0xA,
    QM_SAVE_GHOST   = 0xB,
    QM_LOAD_SETTING = 0xC,
    QM_SAVE_SETTING = 0xD,
    QM_LOGGER       = 0xE,
    QM_DEMO         = 0xF,
    QM_SAVE_GAME    = 0x10,
    QM_RECORDS      = 0x11
};

/* (? name, u015) A line of the result list of a short menu (the
 * qm_list_car_name*, qm_list_time*, qm_pt* texts).  Not polymorphic; 0x18
 * bytes (u64 first: 8-aligned).  The inline constructor calls the inline
 * clear(), defined after it: egcs compiles the in-class bodies in order, so
 * the constructor's saved body (and every place that expands it: the array
 * loops of ShortMenuArgs in the race modes, T102's locals) keeps a real call
 * to clear, and each unit that uses it emits a copy of clear (retail: the
 * copies are 7 instructions and do not return `this', not a constructor).
 * 0x0012F820 is T102's copy, the first one linked, which every call reaches;
 * unreferenced copies 0x001313F0 T052, 0x00133DE8 T053, 0x00134610 T054.
 * (u015 matching pass: the out-of-line calls and the copy exact.) */
struct ShortMenuResultEntry {
    ShortMenuResultEntry() { clear(); }
    void clear()                            /* (? name) 0x0012F820 the empty line */
    {
        car = 0;
        points = 0;
        time = RACE_TIME_INVALID;
        highlight = 0;
    }

    u64 car;                                /* 0x00 car code; 0: the line shows "---" */
    int points;                             /* 0x08 "%d pt" / "%d pts" */
    int time;                               /* 0x0C ms; RACE_TIME_INVALID: none */
    int highlight;                          /* 0x10 non-zero: the texts get colour 0x800080FF */
    int pad14;                              /* 0x14 never written (copied along) */
};
EGCS_SIZE(ShortMenuResultEntry, 0x18);

/* ShortMenuRankEntry (ShortMenuArgs::rank), defined after ShortMenuResultEntry
 * (u016): egcs writes the out-of-line copies of their clear() in reverse
 * definition order, and retail has RankEntry's first (T052 0x001313D0 /
 * 0x001313F0; T053 and T054 the same) when a unit includes this header
 * before T052's. */
#include "game/app/T052_arcade_race_modes.h"    /* ShortMenuRankEntry (ShortMenuArgs::rank) */

/* (? name, u015) The in / out arguments of a short-menu session: a stack
 * object of the race modes (sp+0x000 in T052's); g_003514F0 (gp-0x6500,
 * used only by u015) points at it while the menu runs.  Not polymorphic.  Size 0x188 (0x190
 * with the enhanced engine's two flags at the end): the
 * race modes construct 6 result lines and 12 rank lines (u015 had 5, the
 * number of qm_rank_* texts).  The constructor is inline (the race modes
 * expand it): the two arrays, then the members (result and bestSpeed are
 * left alone).  laps is set last (u016): as the last use of the -1 its
 * store is scheduled first, and session's fills the delay slot of the next
 * call, as in T052's three expansions (and T053's run_license_test). */
struct ShortMenuArgs {
    ShortMenuArgs()
    {
        raceInfo = NULL;
        bestTime = RACE_TIME_INVALID;
        noSetting = 0;
        noExit = 0;
        yes = 0;
        noFreeSession = 0;
        ghostAsk = 0;
        canLogger = 0;
        canSaveGhost = 0;
        canReplay = 0;
        session = -1;
        laps = -1;
        noLoadGhost = 0;                    /* the enhanced engine: GT3's buttons stay */
        noDemo = 0;
    }

    /* Inline accessors (? names, u017): T053's race modes set and read the
     * members through them.  The object lies beyond sp+0x8000 there, and an
     * inline member's `this' gives each access the object's own address in
     * a register plus the member's offset, as retail has it (direct member
     * accesses split the frame offset into sp+0x38000 + 0x38xx instead).
     * T052 (u016) reads noSetting through getNoSetting(): with a direct
     * member read, jump.c's thread_jumps finds the same load before both
     * `noSetting' tests of a race start and threads the first into the
     * second's body; retail branches to the second test. */
    void setRaceInfo(RaceInformation *info) { raceInfo = info; }
    int  getNoExit() { return noExit; }
    void setNoExit(int v) { noExit = v; }
    int  getNoSetting() { return noSetting; }
    void setNoSetting(int v) { noSetting = v; }
    void setNoFreeSession(int v) { noFreeSession = v; }
    int  getYes() { return yes; }
    void setYes(int v) { yes = v; }
    int  getGhostAsk() { return ghostAsk; }
    void setGhostAsk(int v) { ghostAsk = v; }
    void setCanLogger(int v) { canLogger = v; }
    void setCanSaveGhost(int v) { canSaveGhost = v; }
    void setCanReplay(int v) { canReplay = v; }
    void setBestTime(int v) { bestTime = v; }
    void setBestSpeed(f32 v) { bestSpeed = v; }
    void setLaps(int v) { laps = v; }
    void setSession(int v) { session = v; }
    /* The enhanced engine (menus_a.md 26). */
    void setNoLoadGhost(int v) { noLoadGhost = v; }
    void setNoDemo(int v) { noDemo = v; }
    /* Every result line empty (pad14 is left).  T053's run_gtmode_race
     * expands it: the lines' address is loop.c's, not a GCSE copy of the
     * constructor's array pointer as with the loop written out. */
    void clearList() { for (int i = 0; i < 6; i++) list[i].clear(); }

    int  result;                            /* 0x00 the action chosen (QM_*); 1 when the session starts */
    int  noExit;                            /* 0x04 non-zero: "exit" and "save_game" hidden (e.g. the
                                               championship is over) */
    /* 0x08 non-zero: "setting", "save_setting", "load_setting" hidden (an
     * arcade car).  A bool (egcs's is 4 bytes; u016): its own alias set shows
     * in T052, where `noSetting = 0' stays after `presetCar = 0' (also a
     * bool, T482) while the int load of args->color1 moves above the
     * `presetCar = 1' store.  GCC 14 keeps an int (its bool is 1 byte; the
     * values are 0 / 1). */
#ifdef BOOT_OLD_GCC
    bool noSetting;
#else
    int  noSetting;
#endif
    int  noFreeSession;                     /* 0x0C non-zero: "free_session" hidden */
    int  yes;                               /* 0x10 the answer of a yes / no page: qm_yes 1, qm_no 0 */
    int  ghostAsk;                          /* 0x14 the ghost question is asked (u015 / u016 unk14):
                                               "yes" takes the new ghost */
    int  canLogger;                         /* 0x18 0: "data_logger" hidden */
    int  canSaveGhost;                      /* 0x1C 0: "save_ghost" hidden */
    int  canReplay;                         /* 0x20 0: "replay" and "save_replay" hidden */
    int  bestTime;                          /* 0x24 qm_best_time (ms) */
    f32  bestSpeed;                         /* 0x28 the speed test, ? m/s as rank[].speed (u017 swc1) */
    int  laps;                              /* 0x2C qm_laps */
    int  session;                           /* 0x30 qm_session_num shows session + 1 */
    RaceInformation *raceInfo;              /* 0x34 qm_license_*: the information of the test (its
                                               race DB, slot 14) */
    ShortMenuResultEntry list[6];           /* 0x38 */
    ShortMenuRankEntry   rank[12];          /* 0xC8 */
    /* The enhanced engine: Lupo 2's two new gates of the root page
     * (shortmenu_root_check L2 0x0016EA00; menus_a.md 26).  Lupo 2 shows
     * "load_ghost" / "demonstration" only when its flags (its +0x20 /
     * +0x28) are set, and its arcade sets neither.  Here a set flag hides
     * the button, so GT3's race modes, which set neither, keep both buttons
     * where their layouts have them; a mode without a ghost load or a
     * demonstration lap (the Pod race) can hide them. */
    int  noLoadGhost;                       /* 0x188 non-zero: "load_ghost" hidden */
    int  noDemo;                            /* 0x18C non-zero: "demonstration" hidden */
};
EGCS_SIZE(ShortMenuArgs, 0x190);            /* (GT3 0x188) */

/* (? name, u015) Which menu a session runs: static data of the race modes
 * (e.g. 0x0029D4FC), copied to the caller's stack.  Not polymorphic. */
struct ShortMenuParams {
    const char *file;                       /* 0x00 the page file */
    const char *page;                       /* 0x04 the first page */
    int         bgmGroup;                   /* 0x08 the menu music: menu_bgm_play_if_changed(bgmGroup,
                                               0, 1) (run_short_menu) / ShortmenuLicenseControl::
                                               bgmGroup (first pass: mode) */
};
EGCS_SIZE(ShortMenuParams, 0xC);

/* (? name, u015) What race_memcard gives the memory card screen
 * (F_0014AE00, T068): the licence and test of the race. */
struct MemCardRaceInfo {
    const char *licenceName;                /* 0x0 F_0023AB18(licence) */
    int         test;                       /* 0x4 -1: none */
    int         licence;                    /* 0x8 */
};
EGCS_SIZE(MemCardRaceInfo, 0xC);

/* (The entry counts of T113's tables, g_00351454 / g_00351480, which the
 * plan put here, are declared with the tables in T113_shortmenu_items.h.) */

/* One short-menu session over race (may be NULL: then nothing runs behind
 * the menu); pageArg: the page imported into the first one (the second
 * argument of PageManager::setPage, LinkManager::setImportPage; NULL:
 * none); startPage (NULL: none) opens over it.  Returns args->result.
 * Callers: the race modes (T052, T053). */
int  run_short_menu(ShortMenuParams *params, ShortMenuArgs *args, RacePS2Base *race,
                    const char *pageArg, const char *startPage);    /* 0x0012E2C8 */
/* The same with ShortmenuLicenseControl (T103): params->bgmGroup goes to
 * the control, flag != 0 sets its replayMode (+0xE54) to 1 (else 2). */
int  run_short_menu_license(ShortMenuParams *params, ShortMenuArgs *args, RacePS2Base *race,
                            const char *pageArg, const char *startPage, int flag); /* 0x0012E480 */
/* The place of entry `index' in the rank order (0 first; the entry count
 * if it is not there). */
int  race_entry_rank(RaceOrganization *org, int index);             /* 0x0012E638 */
/* The start order by weight / power, the player (entry 0) last. */
void race_order_by_power(RaceOrganization *org);                    /* 0x0012E670 */
/* The order by best lap (car_best_lap), the fastest first; nothing when
 * the player has no time. */
void race_order_by_time(RaceOrganization *org);                     /* 0x0012E920 */
/* Records the result of licence test `race' (medal from the organization's
 * rank: 0 gold .. 2 bronze, -1 none); *licence = its licence (0..5, -1);
 * *flags |= 1 when the licence is passed now and was not before, |= 2 the
 * same for all gold.  Returns the medal. */
int  license_record_result(RaceOrganization *org, const char *race, int *licence,
                           int *flags);                             /* 0x0012EBB0 */
/* Records `rank' as the result of event `race' (GameStatus +0x23510). */
void event_record_result(int rank, const char *race);               /* 0x0012EDA8 */
/* strcpy, then the copy in lower case (A..Z only); returns dst. */
char *strcpy_lower(char *dst, const char *src);                     /* 0x0012EDF0 */
/* Before the race: entry `entry' starts with the garage car's condition. */
void race_set_entry_condition(GarageCar *car, RaceOrganization *org, int entry); /* 0x0012EE78 */
/* After the race: the garage car takes the entry's condition; returns the
 * distance driven (the odometer gain). */
int  race_get_entry_condition(GarageCar *car, RaceOrganization *org, int entry); /* 0x0012EED8 */
/* The memory card screen `mode' (F_0014AE00) for race / car, with the
 * licence and test of raceName (race_name_split's kind and index); the
 * saves (0x10 replay, 0x12 ghost, 0x19 game; T068) borrow a unit of the
 * course arena as their work buffer. */
void race_memcard(int mode, u64 race, u64 car, const char *raceName,
                  RaceOrganization *org);                           /* 0x0012EF40 */
/* args->list in rank order: car, points, the time; `highlight': the
 * player's line marked. */
void result_list_by_rank(RaceOrganization *org, ShortMenuArgs *args, int highlight); /* 0x0012F070 */
/* args->list[order] of every entry (the time: car_best_lap); noTime: no
 * times. */
void result_list_by_order(RaceOrganization *org, ShortMenuArgs *args, int noTime);   /* 0x0012F160 */
/* args->list[i] from champ->getEntryByOrder(i) (car, points, highlight; no
 * time). */
void result_list_from_entries(RaceOrganization *org, ShortMenuArgs *args,
                              RaceChampionshipInformation *champ);  /* 0x0012F290 */
/* The 0x20-byte ranking code text of a record (RankingRecord::encode,
 * T431; 0x1E characters): the car, the course, `kind' (0 the time trial,
 * 1 the free run, 2 a licence test, 3 / 4 / 5 the machine tests 0-400 m /
 * 0-1000 m / max speed, whose `time' is the speed x 1000), `time', the
 * tyres.  course: a course code (db_course_index, getCourseKind; the
 * callers pass RaceInformation::getCourseCode).  name: the name entered for
 * the record (every caller passes it; stored in the record's frame at
 * +0x28, which the encoder does not read). */
void make_ranking_code(char *out, int time, GarageCar *car, u64 course, int kind,
                       const char *name);                           /* 0x0012F3F0 */
/* Reads "demo/<name>" and applies its setting to the race; 0 if the file
 * cannot be read, else 1. */
int  load_demo_setting(RaceSolitaire *race, const char *name);      /* 0x0012F5A8 */
/* A RaceDataBase that selects race `name' and is destroyed again: no
 * lasting effect (select only stores the record in the local). */
void select_race(const char *name);                                 /* 0x0012F7A8 */

#endif /* GAME_T102_RACE_MENU_HELPERS_H */
