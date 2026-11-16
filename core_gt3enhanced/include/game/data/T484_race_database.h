/* T484_race_database.h - RaceDataBase: a CarDataBase with the selected race
 * (event) record of table 0x20 (translation unit T484, unit u108),
 * 0x00238390..0x00239008: its entry cars (table 0x1F records), the regulated
 * car list (a table 0x21 record), prize money, target times, the enemy and
 * tyre wear parameters; the course records of table 0x22.
 *
 * Names: the getters 0x00238B80..0x00238F68 (and getRestrictor) are Lupo 2's
 * (symbols.csv); the fields of RaceEvent follow them.  The others are the
 * first pass's, except where the notes of group h11 say otherwise. */
#ifndef GAME_T484_RACE_DATABASE_H
#define GAME_T484_RACE_DATABASE_H

#include "game/game.h"
#include "game/data/T481_car_data_base.h"            /* base CarDataBase */

class CarEquipments;                            /* T482_car_equipments.h (pointers only) */
class RaceDataBase;

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" {
void *__tf12RaceDataBase(void);             /* 0x00238FB8 (__si_type_info 0x004B2A60, base CarDataBase) */
}
#endif

/* The course kinds (CourseRecord::kind, RaceDataBase::getCourseKind;
 * DynamicsConductor::getCourseKind passes it on).  u005, u009, u017, u043,
 * u046, u047, u076, u085 found them. */
enum {
    COURSE_KIND_1    = 1,                   /* ? (isCourseKind1): the second-set tyres, no race tyres
                                               fitted, no dirt on the car, 4 cars in a single race */
    COURSE_KIND_DIRT = 2,                   /* the rally courses: dirt tyres, no driving aids, 2 cars,
                                               no free session */
    COURSE_KIND_NONE = 0xFF                 /* courseKind: none */
};

/* A record of table 0x22: a course. */
struct CourseRecord {
    u64 code;                                   /* 0x0 */
    u8  kind;                                   /* 0x8 (getCourseKind: 2 = dirt) */
    u8  unk_09;                                 /* 0x9 */
    u16 name0A;                                 /* 0xA string of CarDataBase::strings (u109
                                                   db_course_name: the course's name) */
    u16 name0C;                                 /* 0xC string of CarDataBase::uniStrings (getCourseName) */
    u16 name0E;                                 /* 0xE string of CarDataBase::strings */
};

/* A race time: the gold / silver / bronze times of a licence test
 * (RaceEvent::time[3]; u096's PresetTime).  The plan put it in T036 (u010). */
struct RaceTime {
    u8  min;                                    /* 0x0 */
    u8  sec;                                    /* 0x1 */
    u16 ms;                                     /* 0x2 */
};
EGCS_SIZE(RaceTime, 0x4);

/* A record of table 0x20: a race (event).  The same record is RaceRecord
 * (T489_car_database.h, a typedef of this), RaceDBRecord (T125) and
 * RaceDataEntry (T417) in other units.  The enemy arrays are indexed RWD,
 * FWD, 4WD, GTR. */
struct RaceEvent {
    u64      code;                              /* 0x000 */
    u64      course;                            /* 0x008 the course (table 0x22) */
    u64      entries[32];                       /* 0x010 entry cars (table 0x1F codes), "-" ends */
    int      entryColor[32];                    /* 0x110 their colours */
    u64      carList;                           /* 0x190 the regulated cars: a table 0x21 record
                                                   (32 car codes, "-----" ends), "-" = any car */
    u8       launchSpeed;                       /* 0x198 (start speed of a licence test) */
    u8       numberOfLaps;                      /* 0x199 */
    u8       licenseType;                       /* 0x19A */
    u8       needLicense;                       /* 0x19B 0 none, 1 B, 2 A, 3 IB, 4 IA, 5 S, 6 R */
    u8       enemyMuVolumeF[4];                 /* 0x19C */
    u8       enemyMuVolumeR[4];                 /* 0x1A0 */
    u8       enemySkillA[4];                    /* 0x1A4 */
    u8       enemySkillB[4];                    /* 0x1A8 */
    u8       enemySkillC[4];                    /* 0x1AC */
    u8       enemySkillC2[4];                   /* 0x1B0 */
    u8       enemyDtMasterVolume;               /* 0x1B4 */
    u8       entryCount;                        /* 0x1B5 getEntries */
    u8       enemyBoostType;                    /* 0x1B6 */
    u8       fastEnemyRestrainMax;              /* 0x1B7 */
    u8       fastEnemyRestrainThreshold1;       /* 0x1B8 */
    u8       fastEnemyRestrainThreshold2;       /* 0x1B9 */
    u8       slowEnemyBoostThreshold1;          /* 0x1BA */
    u8       slowEnemyBoostThreshold2;          /* 0x1BB */
    u8       slowEnemyBoostMax;                 /* 0x1BC */
    u8       twMaxTireWear;                     /* 0x1BD */
    u8       twMaxTireGripDown;                 /* 0x1BE */
    u8       twPitTireGripDown;                 /* 0x1BF */
    u8       twInitialTireWear;                 /* 0x1C0 */
    u8       twInitialTireGripDown;             /* 0x1C1 */
    u8       twTerminalTireWear;                /* 0x1C2 */
    u8       twTerminalTireGripDown;            /* 0x1C3 */
    RaceTime time[3];                           /* 0x1C4 gold, silver, bronze */
    u8       unk_1D0[0x2];                      /* 0x1D0 */
    u8       needTire;                          /* 0x1D2 the tyre rule 0..9 (icons "tire_0".."tire_8",
                                                   "tire_dirt"); 9: dirt tyres only (part 0x42) */
    u8       trophyType;                        /* 0x1D3 */
    u16      prize[6];                          /* 0x1D4 prize money / 100 by position */
    u8       twGoodTireWear;                    /* 0x1E0 */
    u8       twGoodTireGripDown;                /* 0x1E1 */
    u16      bonus;                             /* 0x1E2 (? a championship prize) / 100 (F_00238BC0) */
    u8       limitTime;                         /* 0x1E4 */
    u8       difficulty;                        /* 0x1E5 */
    u16      launchPoint;                       /* 0x1E6 */
    u64      raceModeCode;                      /* 0x1E8 (racemode.db, T487) */
    u16      name;                              /* 0x1F0 string of CarDataBase::strings (db_race_name) */
    u8       countDown;                         /* 0x1F2 */
    u8       startAction;                       /* 0x1F3 */
    u8       needTuning;                        /* 0x1F4 0 any, 1 racing, 2 tuned, 3 normal */
    u8       needAspiration;                    /* 0x1F5 0 any, 1 NA, 2 turbo */
    u8       needDrivetrain;                    /* 0x1F6 0 any, 1 FF, 2 FR, 3 4WD, 4 MR, 5 RR */
};

/* The rules of a race record (RaceEvent::need*) as check_regulation (T489)
 * reads them; they index the regulation icon tables (u012, u109).  0 is
 * "any" in each.  The drive train rule has another order than
 * CarEquipments::getDriveTrain's LAYOUT_* (T482). */
enum {
    NEED_LICENSE_NONE = 0,                      /* needLicense: LicenseIndex (T420) + 1 */
    NEED_LICENSE_B    = 1,
    NEED_LICENSE_A    = 2,
    NEED_LICENSE_IB   = 3,
    NEED_LICENSE_IA   = 4,
    NEED_LICENSE_S    = 5,
    NEED_LICENSE_R    = 6
};
enum {
    NEED_TIRE_ANY  = 0,                         /* needTire */
    NEED_TIRE_DIRT = 9                          /* "tire_dirt": dirt tyres only (part 0x42) */
};
enum {
    NEED_TUNING_ANY    = 0,                     /* needTuning */
    NEED_TUNING_RACING = 1,
    NEED_TUNING_TUNED  = 2,
    NEED_TUNING_NORMAL = 3
};
enum {
    NEED_ASPIRATION_ANY   = 0,                  /* needAspiration */
    NEED_ASPIRATION_NA    = 1,
    NEED_ASPIRATION_TURBO = 2
};
enum {
    NEED_DRIVETRAIN_ANY = 0,                    /* needDrivetrain */
    NEED_DRIVETRAIN_FF  = 1,
    NEED_DRIVETRAIN_FR  = 2,
    NEED_DRIVETRAIN_4WD = 3,
    NEED_DRIVETRAIN_MR  = 4,
    NEED_DRIVETRAIN_RR  = 5
};

/* vtable 0x003423D8 (1 slot).  typeinfo __tf12RaceDataBase 0x00238FB8.  Base
 * CarDataBase at +0x00 (its vptr +0x0C is the object's).  Size 0x28 (built
 * on the stack by some 40 callers). */
class RaceDataBase : public CarDataBase {
public:
    RaceDataBase();                             /* 0x00238390 clear(), then the car database g_002EBD88 */
    virtual ~RaceDataBase();                    /* [0] +0x0C 0x00238408 */
    void clear();                               /* 0x00238430 no race selected */
    int  select(u64 code);                      /* 0x00238440 race `code` and its course; 0 if none */
    void setCourse(u64 course);                 /* 0x002384A0 */
    u64  getCourse();                           /* 0x002384A8 the course set, else the event's */
    u64  getEntryCar(int i);                    /* 0x002384C8 */
    int  getNumEntryCars();                     /* 0x002384E0 before the first "-" (<= 32) */
    int  setupEntryCar(int i, CarEquipments *car);  /* 0x00238550 car = entry car i (a preset car);
                                                   0 if none */
    int  getEntryColor(int i);                  /* 0x002385F8 */
    u64  F_00238610(int i);                     /* 0x00238610 +0x18 of entry car i's racing
                                                   modification record */
    int  getNumRegulatedCars();                 /* 0x00238660 the cars of carList; 0 = any car */
    const char *getRegulatedCarName(int i);     /* 0x00238700 name (first part); NULL if any car */
    void getRegulatedCarFullName(char *buf, int i); /* 0x002387D0 "%s %s"; "" if any car */
    u64  getRegulatedCar(int i);                /* 0x002388D8 0 if any car */
    int  getPrize(int rank);                    /* 0x00238950 prize[rank] * 100 (x100 again in region 0) */
    u8   getCourseKind(u64 course);             /* 0x002389A0 CourseRecord.kind (2 = dirt) */
    const char *F_002389C8(u64 course);         /* 0x002389C8 CourseRecord.name0A */
    const char *getCourseName(u64 course);      /* 0x00238A00 CourseRecord.name0C, NULL if unknown */
    const char *F_00238A48(u64 code);           /* 0x00238A48 table 0x22 record `code`'s name0E */
    s8   getChampionshipPoint(int rank);        /* 0x00238A80 {10, 6, 4, 3, 2, 1, 0, 0} (this unused) */
    const char *F_00238A90();                   /* 0x00238A90 F_00238A48(code) */
    const char *F_00238AB0();                   /* 0x00238AB0 getCourseName(course) */
    const char *F_00238AD0();                   /* 0x00238AD0 F_002389C8(course) */
    int  getBronzeTime();                       /* 0x00238AF0 time[2] in ms */
    int  getSilverTime();                       /* 0x00238B20 time[1] in ms */
    int  getGoldTime();                         /* 0x00238B50 time[0] in ms */
    u8   getLicenseType();                      /* 0x00238B80 */
    u16  getLaunchPoint();                      /* 0x00238B90 */
    u8   getLimitTime();                        /* 0x00238BA0 */
    u8   getTrophyType();                       /* 0x00238BB0 */
    int  F_00238BC0();                          /* 0x00238BC0 bonus * 100 (x100 again in region 0) */
    u8   getNeedTire();                         /* 0x00238C08 */
    u8   getNeedAspiration();                   /* 0x00238C18 */
    u8   getNeedTuning();                       /* 0x00238C28 */
    int  getRestrictor();                       /* 0x00238C38 always 0 */
    u8   getNeedDrivetrain();                   /* 0x00238C40 */
    int  F_00238C50();                          /* 0x00238C50 the low word of carList */
    u8   getTwGoodTireGripDown();               /* 0x00238C68 */
    u8   getTwGoodTireWear();                   /* 0x00238C78 */
    u8   getTwTerminalTireGripDown();           /* 0x00238C88 */
    u8   getTwTerminalTireWear();               /* 0x00238C98 */
    u8   getTwInitialTireGripDown();            /* 0x00238CA8 */
    u8   getTwInitialTireWear();                /* 0x00238CB8 */
    u8   getTwPitTireGripDown();                /* 0x00238CC8 */
    u8   getTwMaxTireGripDown();                /* 0x00238CD8 */
    u8   getTwMaxTireWear();                    /* 0x00238CE8 */
    u8   getSlowEnemyBoostMax();                /* 0x00238CF8 */
    u8   getSlowEnemyBoostThreshold2();         /* 0x00238D08 */
    u8   getSlowEnemyBoostThreshold1();         /* 0x00238D18 */
    u8   getFastEnemyRestrainThreshold2();      /* 0x00238D28 */
    u8   getFastEnemyRestrainThreshold1();      /* 0x00238D38 */
    u8   getFastEnemyRestrainMax();             /* 0x00238D48 */
    u8   getEnemyBoostType();                   /* 0x00238D58 */
    u8   getEnemyDtMasterVolume();              /* 0x00238D68 */
    u8   getEnemySkillA_GTR();                  /* 0x00238D78 */
    u8   getEnemySkillA_4WD();                  /* 0x00238D88 */
    u8   getEnemySkillA_FWD();                  /* 0x00238D98 */
    u8   getEnemySkillA_RWD();                  /* 0x00238DA8 */
    u8   getEnemySkillB_GTR();                  /* 0x00238DB8 */
    u8   getEnemySkillB_4WD();                  /* 0x00238DC8 */
    u8   getEnemySkillB_FWD();                  /* 0x00238DD8 */
    u8   getEnemySkillB_RWD();                  /* 0x00238DE8 */
    u8   getEnemySkillC2_GTR();                 /* 0x00238DF8 */
    u8   getEnemySkillC2_4WD();                 /* 0x00238E08 */
    u8   getEnemySkillC2_FWD();                 /* 0x00238E18 */
    u8   getEnemySkillC2_RWD();                 /* 0x00238E28 */
    u8   getEnemySkillC_GTR();                  /* 0x00238E38 */
    u8   getEnemySkillC_4WD();                  /* 0x00238E48 */
    u8   getEnemySkillC_FWD();                  /* 0x00238E58 */
    u8   getEnemySkillC_RWD();                  /* 0x00238E68 */
    u8   getEnemyMuVolumeR_GTR();               /* 0x00238E78 */
    u8   getEnemyMuVolumeR_4WD();               /* 0x00238E88 */
    u8   getEnemyMuVolumeR_FWD();               /* 0x00238E98 */
    u8   getEnemyMuVolumeR_RWD();               /* 0x00238EA8 */
    u8   getEnemyMuVolumeF_GTR();               /* 0x00238EB8 */
    u8   getEnemyMuVolumeF_4WD();               /* 0x00238EC8 */
    u8   getEnemyMuVolumeF_FWD();               /* 0x00238ED8 */
    u8   getEnemyMuVolumeF_RWD();               /* 0x00238EE8 */
    u8   getNeedLicense();                      /* 0x00238EF8 */
    u8   getLaunchSpeed();                      /* 0x00238F08 */
    u8   getNumberOfLaps();                     /* 0x00238F18 */
    u8   getStartAction();                      /* 0x00238F28 */
    u8   getCountDown();                        /* 0x00238F38 */
    u64  getRaceModeCode();                     /* 0x00238F48 */
    u8   getDifficulty();                       /* 0x00238F58 */
    u8   getEntries();                          /* 0x00238F68 */
    u8   F_00238F78();                          /* 0x00238F78 getCourseKind(course) */
    int  F_00238F98();                          /* 0x00238F98 F_00238BC0() != 0 */

    /*         CarDataBase                         0x00 (vptr 0x0C) */
    u64        code;                            /* 0x10 the selected race */
    RaceEvent *event;                           /* 0x18 its record */
    u8         unk_1C[0x4];                     /* 0x1C */
    u64        course;                          /* 0x20 the course (0: the event's) */
};
EGCS_SIZE(RaceDataBase, 0x28);

#endif /* GAME_T484_RACE_DATABASE_H */
