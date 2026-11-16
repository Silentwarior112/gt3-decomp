/* T487_race_mode_database.h - RaceModeDatabase (translation unit T487, unit
 * u108), 0x002393D8..0x00239930: database/racemode.db (the race modes: music
 * and sound for play / replay / demo, up to 32 detail codes) and
 * database/racedetail.db (the details: the display objects of a mode's
 * layout, with their position, size and alignment).  Both files are DataBlock
 * tables (T481).  One static object, g_002F00C0.
 *
 * Names: the record accessors (getAlignY .. getNumRaceModes) are Lupo 2's
 * (symbols.csv), the fields follow them; the rest are the first pass's. */
#ifndef GAME_T487_RACE_MODE_DATABASE_H
#define GAME_T487_RACE_MODE_DATABASE_H

#include "game/game.h"

class DataBlock;                                /* T481_car_data_base.h (pointers only) */
class RaceModeDatabase;

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" {
void *__tf16RaceModeDatabase(void);         /* 0x002398F0 (__user_type_info 0x003534B0, gp-0x4540) */
}
#endif

/* A record of racemode.db: a race mode. */
struct RaceModeRecord {
    u64 code;                                   /* 0x00 nameToCode of its name */
    u8  id;                                     /* 0x08 (selectModeById, getModeId) */
    u8  musicPlay;                              /* 0x09 */
    u8  soundPlay;                              /* 0x0A */
    u8  musicReplay;                            /* 0x0B */
    u8  soundReplay;                            /* 0x0C */
    u8  musicDemo;                              /* 0x0D */
    u8  soundDemo;                              /* 0x0E */
    u8  unk_0F;                                 /* 0x0F */
    u64 details[32];                            /* 0x10 racedetail.db codes, "(Null)" ends the list */
};

/* A record of racedetail.db: one display object of a mode's layout (u052:
 * RaceLayoutRecord, read through g_002F00C0.detail by RaceDisplay). */
struct RaceDetailRecord {
    u64 code;                                   /* 0x00 */
    u16 type;                                   /* 0x08 the object (1..26, see RaceDisplay::setLayout) */
    u16 subType;                                /* 0x0A per type: a sub-index, a variant, a panel */
    u16 alignY;                                 /* 0x0C */
    u8  unk_0E[0x4];                            /* 0x0E */
    u16 alignX;                                 /* 0x12 */
    u8  unk_14[0x4];                            /* 0x14 */
    s16 mode;                                   /* 0x18 the layout variant it belongs to (0x7F: all) */
    s16 x;                                      /* 0x1A */
    s16 y;                                      /* 0x1C */
    s16 width;                                  /* 0x1E */
    s16 height;                                 /* 0x20 */
};

/* vtable 0x00342528 (1 slot).  typeinfo __tf16RaceModeDatabase 0x002398F0
 * (__user_type_info).  Data, then the vptr at +0x10 (egcs).  Size 0x14. */
class RaceModeDatabase {
public:
    RaceModeDatabase();                         /* 0x002393D8 everything NULL */
    virtual ~RaceModeDatabase();                /* [0] +0x0C 0x00239400 (the buffers are not freed) */
    void *loadFile(const char *path, void *buf, int size);  /* 0x00239430 buf, NULL if not read */
    int  load();                                /* 0x00239478 frees and reloads both files; 1 if both read */
    void release();                             /* 0x00239520 frees both buffers (the pointers are kept) */
    int  selectMode(const char *name);          /* 0x00239568 selectMode(nameToCode(name)) */
    int  selectMode(u64 code);                  /* 0x002395A0 0 if absent (unchanged then) */
    int  selectModeById(int id);                /* 0x002395F8 the first mode with that id; 0 if none */
    int  getModeId(const char *name);           /* 0x00239650 -1 if absent */
    int  getNumDetails();                       /* 0x002396A8 of the selected mode, before "(Null)" (<= 32) */
    int  selectDetail(int i);                   /* 0x00239718 detail i of the selected mode; 0 if none */
    /* The accessors are inline, defined in T487_race_mode_database.cpp (no
     * caller anywhere: retail's copies are T487's, written with the
     * end-of-file group because the key function is there). */
    inline u16 getAlignY();                     /* 0x002397E0 */
    inline u16 getAlignX();                     /* 0x002397F0 */
    inline s16 getY();                          /* 0x00239800 */
    inline s16 getX();                          /* 0x00239810 */
    inline s16 getHeight();                     /* 0x00239820 */
    inline s16 getWidth();                      /* 0x00239830 */
    inline s16 getMode();                       /* 0x00239840 */
    inline u16 getSubType();                    /* 0x00239850 */
    inline u16 getType();                       /* 0x00239860 */
    inline u8  getSoundDemo();                  /* 0x00239870 */
    inline u8  getMusicDemo();                  /* 0x00239880 */
    inline u8  getSoundReplay();                /* 0x00239890 */
    inline u8  getMusicReplay();                /* 0x002398A0 */
    inline u8  getSoundPlay();                  /* 0x002398B0 */
    inline u8  getMusicPlay();                  /* 0x002398C0 */
    inline u16 getVersion();                    /* 0x002398D0 racemode.db's version */
    inline u16 getNumRaceModes();               /* 0x002398E0 racemode.db's record count */

    DataBlock        *modes;                    /* 0x00 database/racemode.db */
    DataBlock        *details;                  /* 0x04 database/racedetail.db */
    RaceModeRecord   *mode;                     /* 0x08 the selected mode */
    RaceDetailRecord *detail;                   /* 0x0C the selected detail */
    /*                vptr                         0x10 (egcs) */
};
EGCS_SIZE(RaceModeDatabase, 0x14);

/* The race modes (u045 / u047: "the table of race mode names", u052: the
 * display layout table). */
extern RaceModeDatabase g_002F00C0;             /* 0x002F00C0 */

/* The static destructor / constructor of g_002F00C0 (.dtors entry
 * 0x0028F260, .ctors entry 0x0028F18C) are egcs's _GLOBAL_$D$ / _GLOBAL_$I$
 * of T487 (0x00239798 / 0x002397C0; the object is defined there). */

#endif /* GAME_T487_RACE_MODE_DATABASE_H */
