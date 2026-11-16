/* T435_product_config.h - ProductConfig, the product database
 * "gameconf/product.gcf" (translation unit T435, unit u099).
 *
 * The file is read whole, relocated in place (its offsets turned into
 * pointers by T436..T445, one translation unit per section type) and its
 * sections kept by type: car categories, course lists, attract demos, events,
 * prize car sets, service prices, memory card demos.  One instance, built by
 * product_config_init (gp-0x4F40), read through product_config() by about 60
 * callers (they call the methods as F_0021Dxxxx(db, ...)).  ? name (u099).
 * The section structures (Gcf*) are u099's only: pointers here. */
#ifndef GAME_T435_PRODUCT_CONFIG_H
#define GAME_T435_PRODUCT_CONFIG_H

#include "game/game.h"

struct GcfHeader;                           /* the file start: section table */
struct GcfGeneral;                          /* type 0x0 (T438) */
struct GcfCarSection;                       /* type 0xE (T437) */
struct GcfCourseSection;                    /* type 0x8 (T439) */
struct GcfDemoSection;                      /* type 0xC (T441) */
struct GcfNamed;                            /* {name, value} of types 0x4, 0x6, 0xA, 0xD */
struct GcfNamedTable;                       /* types 0x4, 0x6, 0xA, 0xD (T445, T440, T443, T444) */
struct GcfPrizeSet;                         /* one set of type 0xF */
struct GcfPrizeSection;                     /* type 0xF (T442) */

/* Not polymorphic.  Size 0x2C (operator new in product_config_init). */
class ProductConfig {
public:
    ProductConfig(const char *path);                        /* 0x0021CF48 reads and relocates the file */
    ~ProductConfig();                                       /* 0x0021D0B8 */

    int         idleSeconds();                              /* 0x0021D2A0 the title page's idle time (u009) */
    int         F_0021D2B0();                               /* 0x0021D2B0 general +4: the attract demo's
                                                               RaceInformation::timeLimit (s) */
    int         carCount(const char *cat);                  /* 0x0021D2C0 cars of category cat */
    const char *carCode(int i, const char *cat);            /* 0x0021D2E0 */
    const char *carRace(int i, const char *cat);            /* 0x0021D300 the car's +4, a race code
                                                               (ArcadeControl::raceCode runs it) */
    int         F_0021D320(int i, const char *cat);         /* 0x0021D320 the arcade level the car needs
                                                               (s16 +0xA; GameStatus::isArcadeLevelOpen) */
    int         carRaceStep(int i, const char *cat);        /* 0x0021D340 the car's +0xC, added to the
                                                               last character of the arcade race code */
    int         F_0021D360(const char *cat);                /* 0x0021D360 the largest F_0021D320; no callers */
    int         carSelectable(int i, const char *cat);      /* 0x0021D3E0 */
    const char *carRaceOf(const char *code);                /* 0x0021D408 carRace of the car `code`, NULL
                                                               if none; no callers */
    int         courseCount(const char *list);              /* 0x0021D4E8 courses of list */
    const char *courseName(int i, const char *list);        /* 0x0021D508 */
    int         F_0021D528(int i, const char *list);        /* 0x0021D528 the course's s8 +5; no callers */
    int         courseLevel(int i, const char *list);       /* 0x0021D548 the arcade level the course needs (s16 +6;
                                                               also its "area_a".."area_f" picture,
                                                               ArcadeControl::historyArea) */
    int         F_0021D568(const char *list);               /* 0x0021D568 the largest courseLevel of the list */
    int         courseLength(int i, const char *list);      /* 0x0021D5E8 the course length in metres (+8;
                                                               ArcadeControl::createCourseInfo) */
    int         courseAvailable(int i, const char *list);   /* 0x0021D608 */
    int         courseIndex(const char *name, const char *list); /* 0x0021D630 -1 if absent */
    int         demoCount();                                /* 0x0021D6C8 the attract demos (run_attract_demo) */
    const char *demoCar(int i);                             /* 0x0021D6D8 the demo's race code (not a car:
                                                               RaceInformation::setRaceCode) */
    const char *F_0021D6F8(int i);                          /* 0x0021D6F8 the demo's course code (+4) */
    const char *F_0021D718(int i);                          /* 0x0021D718 the demo's shop car code (+8;
                                                               RaceArcadeDemoInformation::shopCar) */
    int         F_0021D738(int i);                          /* 0x0021D738 the demo's shopCarPos (+0xC) */
    int         F_0021D758(int i);                          /* 0x0021D758 the demo's requirement (+0x10):
                                                               GameStatus::isLicenseSPassed() >= it */
    GcfNamed  **findEvent(const char *name);                /* 0x0021D778 NULL if absent */
    int         hasEvent(const char *name);                 /* 0x0021D7A0 */
    int         eventCount();                               /* 0x0021D7D0 */
    u32         eventIndex(const char *name);               /* 0x0021D7E0 */
    int         eventValue(const char *name);               /* 0x0021D810 the requirement (event_unlocked, T036) */
    const char *eventName(int i);                           /* 0x0021D838 */
    GcfNamed  **find1C(const char *name);                   /* 0x0021D858 (the type 0x6 table) */
    int         count1C();                                  /* 0x0021D880 */
    u32         index1C(const char *name);                  /* 0x0021D890 CourseBestTimes::find's slot */
    int         singleCarPrizeSetCount();                   /* 0x0021DA90 the prize sets of exactly one car
                                                               (PrizeSetFlags::ratio) */
    int         prizeSetCount();                            /* 0x0021DAE0 */
    const char *prizeSetName(int i);                        /* 0x0021DAF0 */
    GcfPrizeSet *findPrizeSet(const char *set);             /* 0x0021DB10 */
    int         prizeCarCount(const char *set);             /* 0x0021DB40 */
    const char *prizeCarCode(const char *set, int i, int *replaced); /* 0x0021DB60 the regional code */
    u32         prizeCarColor(const char *set, int i);      /* 0x0021DBD8 (u32)-1 out of range */
    u32         prizeSetIndex(const char *set);             /* 0x0021DC38 */
    int         prizeCarWeight(const char *set, int i);     /* 0x0021DC78 its chance */
    GcfNamed   *findService(const char *name);              /* 0x0021DCB0 */
    int         servicePrice(const char *name);             /* 0x0021DCE0 "initial" (the starting money), "wheel", ...;
                                                               x100 for COUNTRY_JP */
    int         cardDemoCount();                            /* 0x0021DD18 */
    const char *cardDemoName(int i);                        /* 0x0021DD28 */
    int         cardDemoLevel(int i);                       /* 0x0021DD48 GameStatus::isLicenseSPassed() >= it */

    void             *buf;                  /* 0x00 the file (malloc'd) */
    GcfHeader        *hdr;                  /* 0x04 = buf, relocated */
    GcfGeneral       *general;              /* 0x08 type 0x0 */
    GcfCarSection    *cars;                 /* 0x0C type 0xE */
    GcfCourseSection *courses;              /* 0x10 type 0x8 */
    GcfDemoSection   *demos;                /* 0x14 type 0xC */
    GcfNamedTable    *events;               /* 0x18 type 0x4 (sorted) */
    GcfNamedTable    *table1C;              /* 0x1C type 0x6 (sorted) */
    GcfPrizeSection  *prizes;               /* 0x20 type 0xF (sorted) */
    GcfNamedTable    *services;             /* 0x24 type 0xA (sorted) */
    GcfNamedTable    *cardDemos;            /* 0x28 type 0xD (not cleared by the constructor) */
};
EGCS_SIZE(ProductConfig, 0x2C);

/* The car code of the current country (g_00352FD8) for code (the table
 * 0x0049D0E0, a local static: Vauxhall in BR, Dodge / S2000 "a" in US,
 * S2000 "e" in Europe);
 * *replaced = 1 when it differs. */
const char    *gcf_regional_car_code(const char *code, int *replaced);    /* 0x0021D8C0 */
void           product_config_init(void);                                 /* 0x0021DD68 */
ProductConfig *product_config(void);                                      /* 0x0021DDE0 the instance */
/* Binary searches by name; NULL if absent. */
GcfNamed     **gcf_find_event(GcfNamed **list, int n, const char *name);  /* 0x0021DDE8 */
GcfNamed     **gcf_find_1C(GcfNamed **list, int n, const char *name);     /* 0x0021DE88 */
GcfPrizeSet  **gcf_find_prize_set(GcfPrizeSet **list, int n, const char *name); /* 0x0021DF28 */
GcfNamed     **gcf_find_service(GcfNamed **list, int n, const char *name); /* 0x0021DFC8 */

#endif /* GAME_T435_PRODUCT_CONFIG_H */
