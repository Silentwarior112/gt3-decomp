/* T429_game_status.h - gt2k_game::GameStatus, the game's save data
 * (translation unit T429, unit u098); the garage lists GarageCarList /
 * GarageWheelList have their own headers (T429_garage_car_list.h,
 * T429_garage_wheel_list.h).
 *
 * One GameStatus (gp-0x4F5C), created by createGameStatus and reached
 * through getGameStatus() (about 680 callers).  It holds the day, the money
 * and the statistics, the garage (200 GarageCar slots, the used ones linked
 * in carList, new cars at the head; 200 GarageWheel slots in wheelList), the
 * arcade progress, and the record blocks of T415..T434.  As a
 * Serialize::Base (class "GameStatus", version 7.1) it saves the 0x130 bytes
 * from +0x08, the listed cars and wheels, then the record blocks.
 *
 * T429 also holds out-of-line copies of inline / template code owned
 * elsewhere (no declarations here): List<GarageWheel>::~List 0x0021A4E0,
 * List<GarageCar>::~List 0x0021A508, Cell<GarageCar>::~Cell 0x0021A9C8,
 * __tf9GarageCar 0x0021A9F0, GarageCar::~GarageCar 0x0021AA50,
 * __tfQ29gt2k_menut4Cell1Z9GarageCar 0x0021AAB8 (T036 has the others). */
#ifndef GAME_T429_GAME_STATUS_H
#define GAME_T429_GAME_STATUS_H

#include "game/game.h"
#include "engine/io/T509_serialize.h"            /* base Serialize::Base (declares Serialize::Header,
                                               gt2k_game::GameThumbnail) */
#include "game/menu/framework/menu_list.h"                  /* template gt2k_menu::List (base of the garage lists) */
/* The garage: the car (by value), its list, the wheel (by value), its list,
 * in this order (retail's type names, T429_garage_wheel_list.h); the car
 * from T036_garage_car.h, not T036_garage.h (its hooks and menu classes are
 * not part of the GameStatus: T036_garage_car.h says why). */
#include "game/menu/gtmode/T036_garage_car.h"           /* GarageCar, held by value */
#include "game/save/T429_garage_car_list.h"      /* GarageCarList (T074 sees it without the wheel list) */
#include "game/save/T491_garage_wheel.h"         /* GarageWheel, held by value */
#include "game/save/T429_garage_wheel_list.h"    /* GarageWheelList */
#include "game/save/T420_license_records.h"      /* LicenseRecords, held by value */
#include "game/save/T425_event_results.h"        /* EventResults */
#include "game/save/T434_best_times.h"           /* CourseBestTimes */
#include "game/save/T424_arcade_records.h"       /* ArcadeRecords */
#include "game/save/T417_time_trial_records.h"   /* TimeTrialRecords */
#include "game/save/T416_free_run_records.h"     /* FreeRunRecords */
#include "game/save/T415_machine_test_records.h" /* MachineTestRecords */
#include "game/save/T421_prize_set_flags.h"      /* PrizeSetFlags */
#include "game/save/T422_championship_save.h"    /* ChampionshipSave */
#include "game/save/T428_game_options.h"         /* GameOptions */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus */

/* GameStatus::setLastInputedName's (G007; the same declaration as libc.h's
 * and G007_stdio.h's). */
extern "C" char *strcpy(char *dst, const char *src);   /* 0x0027EB84 */

namespace gt2k_game {

/* vtable _vt$Q29gt2k_game10GameStatus 0x003405E8 (9 slots), typeinfo
 * __tfQ29gt2k_game10GameStatus 0x0021A7E8 (si, base Serialize::Base;
 * __si_type_info 0x004B29A0).  vptr +0x00 (Base has no data).  Size 0x35210
 * (operator new in createGameStatus). */
class GameStatus : public Serialize::Base {
public:
    GameStatus();                                           /* 0x00218668 then init() */
    virtual const Serialize::ClassInfo *getClassInfo();     /* [0] +0x0C 0x002185D8 "GameStatus" (0x002E34B8) */
    virtual ~GameStatus();                                  /* [1] +0x14 0x00218AD0 */
    virtual int getDataSize();                              /* [2] +0x1C 0x00219E60 dataSize(carNum, wheelNum) */
    virtual int getVersion();                               /* [3] +0x24 0x00219F38 7 */
    virtual int getSubVersion();                            /* [4] +0x2C 0x00219F40 1 */
    /*                                                         [5] +0x34 0x00242DA0 Base::checkHeader */
    virtual const void *load(const void *data, void *dest); /* [6] +0x3C 0x00219F48 */
    virtual void *save(void *buf);                          /* [7] +0x44 0x0021A258 */
    virtual void describe(gt2k_game::GameThumbnail *thumb);   /* [8] +0x4C 0x0021A410 kind 0, money,
                                                               cars, licence, day, completion % */

    void init();                                /* 0x00218CF0 a new game ("initial" money, day 1) */
    void setRunningCarIndex(int i);             /* 0x00218F08 */
    GarageCar *getRunningCar();                 /* 0x00218F40 */
    void clearRunningCar();                     /* 0x00218F68 */
    GarageCar *findFreeCar();                   /* 0x00218F88 a slot with !valid */
    void addCar(GarageCar *car);                /* 0x00218FE0 at the head of carList */
    GarageCar *newCar(const char *code, int colorIndex);    /* 0x00219048 NULL: no free slot / no such car */
    GarageCar *newCarCopy(const GarageCar *src);        /* 0x00219128 */
    void removeCar(GarageCar *car);             /* 0x00219190 */
    void buyCar(const char *code, int colorIndex);  /* 0x00219250 newCar, its price paid, a new engine */
    void buyUsedCar(const GarageCar *src);      /* 0x002192A8 */
    void presentCar(const char *code, int colorCode);   /* 0x002192E8 a prize car (setColorCode), free */
    void sellCar(int index);                    /* 0x00219368 a quarter of the price back */
    int  getAverageOrder();                     /* 0x002193D8 */
    int  getWinRate();                          /* 0x00219410 */
    int  getCarNum();                           /* 0x00219448 */
    GarageCarList *getCarList();                /* 0x00219450 */
    GarageCar *getCar(int index);               /* 0x00219460 */
    s64  getTotalCarPrice();                    /* 0x00219488 the listed cars' prices, summed in 64 bits */
    int  getWheelNum();                         /* 0x00219500 */
    GarageWheelList *getWheelList();            /* 0x00219528 */
    GarageWheel *getWheel(int index);           /* 0x00219538 */
    GarageWheel *addWheel(const char *name);    /* 0x00219560 */
    void removeWheel(GarageWheel *wheel);       /* 0x00219620 */
    void buyWheel(const char *name);            /* 0x002196A8 the "wheel" service price */
    void sellWheel(int index);                  /* 0x002196F8 */
    int  hasWheel(const char *name);            /* 0x00219778 */
    void setRunningCarByCode(const char *code); /* 0x00219800 */
    void setRunningCar(GarageCar *car);         /* 0x00219840 */
    void setRunningCarCopy(const GarageCar *car);       /* 0x002198D8 */
    f32  getCompletionRatio();                  /* 0x00219918 0..1 */
    int  allArcadeRacesWon(const char *race, int level);    /* 0x00219A28 */
    void updateArcadeProgress();                /* 0x00219B20 after an arcade place changed */
    int  isArcadeLevelOpen(int level);          /* 0x00219D60 */
    int  isLicenseSPassed();                    /* 0x00219D80 every test of the S licence (LICENSE_S) has a medal */
    static int dataSize(int cars, int wheels);  /* 0x00219DC0 the saved size */
    static int maxDataSize();                   /* 0x00219E80 */
    static int minDataSize();                   /* 0x00219EA0 */
    void saveRandomSeed();                      /* 0x00219EC0 */
    void restoreRandomSeed(const Serialize::Header *hdr);   /* 0x00219EE8 */

    /* The accessors.  Inline in the original: T429's copies 0x0021A530..
     * 0x0021A7E0 have no callers and follow the code egcs emits at the end
     * of the unit (the static constructor / destructor, template copies);
     * the trophy and prize screens (u019 / u020) expand getMoney, incMoney,
     * incPrize, incTrophyNum, setLargeTrophy and setSmallTrophy.  Names:
     * Lupo 2's.  Declared in the reverse order of the copies (getMenuStatus
     * 0x0021A530 last): egcs writes a class's inline members at the end of
     * the unit in the reverse of their order here (u098's data pass, the
     * frame info: ref/data/u098/notes.md).  So no accessor calls one
     * declared after it (that one's body is not compiled yet: the call
     * would stay, as getRaceCar's through getMenuStatus did). */
    void setDay(int d) { day = d; }
    void setMoney(s64 m) { money = m; }         /* (Lupo 2: setMoney(long)) */
    /* (? names) player 2's race car (MenuStatus::setRaceCar*, slot 0). */
    void setRaceCar2ByCode(const char *code) { menu.setRaceCarByCode(code, 0); }
    void setRaceCar2(GarageCar *car) { menu.setRaceCar(car, 0); }
    void setRaceCar2Copy(const GarageCar *car) { menu.setRaceCarCopy(car, 0); }
    void incDay(int days) { day += days; }
    /* incPrize / incMoney: `+=' (expanded in trophy_prize_screen and
     * trophy_get, retail adds the field first: daddu v1, v1, prize; the
     * out-of-line copies and u013's incMoney(-price) are the same either
     * way). */
    void incMoney(int m) { money += m; }
    void incBattleNum() { battleNum++; }
    void incOrder(int order) { orderSum += order; }
    void incPrize(int prize) { totalPrize += prize; }
    void incWinnerNum() { winnerNum++; }
    void incDistance(unsigned int d) { distance += d; }
    void incTrophyNum(int n) { trophyNum += n; }
    int  getDay() const { return day; }
    s64  getMoney() const { return money; }
    int  getBattleNum() const { return battleNum; }
    int  getWinnerNum() const { return winnerNum; }
    s64  getTotalPrize() const { return totalPrize; }
    unsigned int getDistance() const { return distance; }
    int  getTrophyNum() const { return trophyNum; }
    int  getPresentNum() { return presentNum; }
    int  getArcadeProgressLevel() const { return arcadeLevel; }
    /* (? name) Player 1's race car (MenuStatus+0x540).  Through a pointer to
     * menu: retail's copy computes &menu first (`menu.raceCar[0]' folds the
     * two offsets); getMenuStatus() is declared after it (above). */
    GarageCar *getRaceCar() { MenuStatus *m = &menu; return m->raceCar[0]; }
    /* 1 if the race car is a garage car (editableCarIndex >= 0). */
    int  isRunningCarEditable() { return (u32)~editableCarIndex >> 31; }
    /* strcpy (G007, declared above the class); retail's copy 0x0021A650 is
     * among the accessors. */
    void setLastInputedName(const char *name) { strcpy(lastInputedName, name); }
    const char *getLastInputedName() const { return lastInputedName; }
    int  hasSeenEasyEnding() const { return easyEnding; }
    int  hasSeenNormalEnding() const { return normalEnding; }
    void setEasyEndingFlag(bool f) { easyEnding = f; }
    void setNormalEndingFlag(bool f) { normalEnding = f; }
    int  hasLargeTrophy(unsigned int bits) const { return (largeTrophy & bits) != 0; }
    void setLargeTrophy(unsigned int bits) { largeTrophy |= bits; }
    int  hasSmallTrophy(unsigned int bits) const { return (smallTrophy & bits) != 0; }
    void setSmallTrophy(unsigned int bits) { smallTrophy |= bits; }
    int  getLanguage() const { return language; }
    LicenseRecords     *getLicenseRecords() { return &licences; }
    EventResults       *getEventResults() { return &events; }
    CourseBestTimes    *getCourseBestTimes() { return &bestTimes; }
    ArcadeRecords      *getArcadeRecords() { return &arcade; }
    TimeTrialRecords   *getTimeTrialRecords() { return &timeTrial; }
    FreeRunRecords     *getFreeRunRecords() { return &freeRun; }
    MachineTestRecords *getMachineTestRecords() { return &machineTest; }
    PrizeSetFlags      *getPrizeSetFlags() { return &prizeSets; }
    ChampionshipSave   *getChampionshipSave() { return &championship; }
    ::GameOptions      *getGameOptions() { return &options; }
    MenuStatus         *getMenuStatus() { return &menu; }

    /*   vptr                                      0x00000 */
    int  unk_04;                                /* 0x00004 (? not saved) */
    int  day;                                   /* 0x00008 1 at the start; the saved block starts here */
    int  battleNum;                             /* 0x0000C races run */
    int  orderSum;                              /* 0x00010 the places taken, summed (incOrder) */
    int  winnerNum;                             /* 0x00014 races won */
    s64  money;                                 /* 0x00018 */
    s64  totalPrize;                            /* 0x00020 */
    unsigned int distance;                      /* 0x00028 */
    int  carSerial;                             /* 0x0002C the next car's index */
    int  runningCarIndex;                       /* 0x00030 position of the running car in carList, -1 */
    int  editableCarIndex;                      /* 0x00034 position of the race car in carList, -1 when it is
                                                   not a garage car (isRunningCarEditable) */
    int  carNum;                                /* 0x00038 */
    int  wheelNum;                              /* 0x0003C */
    int  unk_40;                                /* 0x00040 */
    int  arcadeLevel;                           /* 0x00044 getArcadeProgressLevel */
    int  arcadeClear[12];                       /* 0x00048 [level]: every arcade race of the level won at N,
                                                   [6 + level]: at H (? use) */
    char lastInputedName[0x40];                 /* 0x00078 */
    int  trophyNum;                             /* 0x000B8 */
    int  easyEnding;                            /* 0x000BC seen (u021: the short ending) */
    int  normalEnding;                          /* 0x000C0 seen (u021: the long ending) */
    int  presentNum;                            /* 0x000C4 cars won */
    unsigned int largeTrophy;                   /* 0x000C8 bits */
    unsigned int smallTrophy;                   /* 0x000CC bits */
    int  language;                              /* 0x000D0 the language when saved (gp-0x4A18) */
    u32  randomSeed;                            /* 0x000D4 the race random seed when saved */
    int  unk_D8[24];                            /* 0x000D8 */
    GarageCar          cars[200];               /* 0x00138 the garage slots */
    GarageCarList      carList;                 /* 0x1B6B8 the used ones */
    GarageWheel        wheels[200];             /* 0x1B6C4 */
    GarageWheelList    wheelList;               /* 0x1F544 */
    LicenseRecords     licences;                /* 0x1F550 (T420) */
    EventResults       events;                  /* 0x23510 (T425) */
    CourseBestTimes    bestTimes;               /* 0x23B08 (T434) */
    ArcadeRecords      arcade;                  /* 0x23BB8 (T424) */
    TimeTrialRecords   timeTrial;               /* 0x246F8 (T417) */
    FreeRunRecords     freeRun;                 /* 0x26160 (T416) */
    MachineTestRecords machineTest;             /* 0x2E560 (T415) */
    PrizeSetFlags      prizeSets;               /* 0x2EE60 (T421) */
    ChampionshipSave   championship;            /* 0x2EF28 (T422) */
    ::GameOptions      options;                 /* 0x2FF38 (T428) */
    u8                 unk_307C0[0x8];          /* 0x307C0 */
    MenuStatus         menu;                    /* 0x307C8 (T427) */
};

} /* namespace gt2k_game */

EGCS_SIZE(gt2k_game::GameStatus, 0x35210);

void createGameStatus(void);                    /* 0x002185E8 once, then GameOptions::apply() */
gt2k_game::GameStatus *getGameStatus(void);     /* 0x00218660 the instance (F_00218660 in the units) */
Serialize::Base *game_status_get(void);         /* 0x00219DB0 the "GameStatus" factory's get: the instance */
void game_status_release(Serialize::Base *obj); /* 0x00219DB8 its release: nothing */
void T429_static_fini(void);                    /* 0x0021A488 .dtors: ~ClassInfo of "GameStatus" */
void T429_static_init(void);                    /* 0x0021A4B0 .ctors: the ClassInfo "GameStatus" */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand declarations). */
extern "C" {
void *__tfQ29gt2k_game10GameStatus(void);               /* 0x0021A7E8 */
void *__tf15GarageWheelList(void);                      /* 0x0021A838 */
void *__tfQ29gt2k_menut4List1Z11GarageWheel(void);      /* 0x0021A8B0 */
void *__tf13GarageCarList(void);                        /* 0x0021A900 */
void *__tfQ29gt2k_menut4List1Z9GarageCar(void);         /* 0x0021A978 */
}
#endif

#endif /* GAME_T429_GAME_STATUS_H */
