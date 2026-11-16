/* gcf_internal.h - unit u099 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0021B790..0x0021E7D8.
 *
 * Translation units (link order):
 *   T432  BigNum: arithmetic on little-endian byte-array numbers (shift,
 *         add, subtract, compare, divide), bit-field access into a byte
 *         buffer, and the conversion of a number to decimal digits.  Used by
 *         T431 (RankingCodeBlock::RankingCodeBlock, RankingRecord::encode)
 *         to turn ranking records into codes.
 *   T433  the SAFER SK block cipher (Massey; the 64-bit block, up to 13
 *         rounds, strengthened key schedule) with its exp/log tables, and a
 *         byte-sliding encryption of a buffer.  Used by T431
 *         (RankingHash::compress, RankingCodeBlock::RankingCodeBlock).
 *   T435  ProductConfig: "gameconf/product.gcf", the product database (car
 *         categories, course lists, attract demos, events, prize car sets,
 *         service prices, memory card demos), one instance at gp-0x4F40.
 *   T436..T445  the relocation of the .gcf file: offsets into pointers, one
 *         translation unit per section type (no canonical headers: declared
 *         here).
 *
 * Canonical headers (include/game/):
 *   T432_bignum.h          own: BigNum and its functions
 *   T433_safer.h           own: SaferTables, SaferKey, SaferCtx, the cipher
 *   T435_product_config.h  own: ProductConfig, gcf_regional_car_code,
 *                          product_config(_init), the gcf_find_* searches
 *   T503_vol_thread.h      file_read_blocking, vol_device_object
 *   T494_text_utils.h      str_equal
 *   T516_country.h         Country g_00352FD8, COUNTRY_*
 *   G007_stdio.h           memset, memcpy, memcmp, strcmp
 *   T532_heap.h            free
 * Everything below is local to the unit: the .gcf section layouts (the
 * canonical T435 header only forward-declares them), the relocation
 * functions and T435's data.
 */
#ifndef U099_H
#define U099_H

#include "types.h"
#include <stddef.h>

#include "runtime/newlib/G007_stdio.h"            /* memset, memcpy, memcmp, strcmp */
#include "engine/pdistd/T532_heap.h"             /* free */
#include "engine/io/T503_vol_thread.h"       /* file_read_blocking, vol_device_object */
#include "game/common/T494_text_utils.h"       /* str_equal */
#include "game/data/T516_country.h"          /* g_00352FD8, COUNTRY_* */
#include "engine/pdistd/T432_bignum.h"           /* own */
#include "engine/pdistd/T433_safer.h"            /* own */
#include "game/data/T435_product_config.h"   /* own */

/* ======================================================================== */
/* T435..T445: gameconf/product.gcf                                         */
/* ======================================================================== */

/* In the file every pointer below is an offset from the start of the
 * structure that holds it (section offsets: from the file start); the
 * relocation functions (T436..T445) add the base in place.
 * GCF_REL(type, base, off, field): the pointer of an offset field, through
 * `off`, one `long` local per relocation function.  The retail code shows
 * it: each offset goes through a 64-bit register (lw, then dsll32 / dsra32
 * for the 32-bit add: no effect on an address), and where a function
 * relocates more than one field after the first, the loads share one
 * register and the truncation gets another (one pseudo set twice is not
 * local to a block; an inline helper's fresh parameter per call is, and
 * gets tied).  The function's first relocation has no extension: CSE takes
 * the value straight from the load before any store. */
#define GCF_REL(type, base, off, field) ((off) = (long)(int)(field), (type)((char *)(base) + (off)))

/* The section types (GcfSectionRef::type); the others are skipped. */
enum {
    GCF_GENERAL    = 0x0,               /* GcfGeneral (T438) */
    GCF_EVENTS     = 0x4,               /* GcfNamedTable (T445) */
    GCF_TABLE_06   = 0x6,               /* GcfNamedTable (T440), ProductConfig::table1C */
    GCF_COURSES    = 0x8,               /* GcfCourseSection (T439) */
    GCF_SERVICES   = 0xA,               /* GcfNamedTable (T443) */
    GCF_DEMOS      = 0xC,               /* GcfDemoSection (T441) */
    GCF_CARD_DEMOS = 0xD,               /* GcfNamedTable (T444) */
    GCF_CARS       = 0xE,               /* GcfCarSection (T437) */
    GCF_PRIZES     = 0xF                /* GcfPrizeSection (T442) */
};

struct GcfSectionRef {
    int   type;                             /* 0x0 GCF_* (0x0 .. 0xF) */
    void *data;                             /* 0x4 */
};

struct GcfHeader {                          /* the file start */
    int            count;                   /* 0x0 */
    GcfSectionRef *sections;                /* 0x4 */
};

/* Type 0x0 (T438): general values (not relocated). */
struct GcfGeneral {
    int idleSeconds;                        /* 0x0 idle time of the title page (GtmodeControl::runRootPage) */
    int demoTimeLimit;                      /* 0x4 the attract demo's RaceInformation::timeLimit (seconds) */
};

/* Type 0xE (T437): cars, by category ("class list"). */
struct GcfCar {
    const char *code;                       /* 0x0 car code */
    const char *race;                       /* 0x4 (? a race code: its only reader, ArcadeControl::raceCode,
                                               runs it; ProductConfig::carRace returns it) */
    s16         selectable;                 /* 0x8 1: can be chosen */
    s16         level;                      /* 0xA the arcade level it needs (GameStatus::isArcadeLevelOpen) */
    int         raceStep;                   /* 0xC added to the last character of the arcade race code
                                               (ArcadeControl::raceCode, classes 0..4) */
};
struct GcfCarCategory {
    const char *name;                       /* 0x0 */
    int         count;                      /* 0x4 */
    GcfCar    **cars;                       /* 0x8 */
};
struct GcfCarSection {
    int              count;                 /* 0x0 */
    GcfCarCategory **list;                  /* 0x4 */

    GcfCarCategory *find(const char *category);         /* 0x0021D100 */
    GcfCar         *car(int i, const char *category);   /* 0x0021D198 */
};

/* Type 0x8 (T439): courses, by list (game mode). */
struct GcfCourse {
    const char *name;                       /* 0x0 */
    s8          available;                  /* 0x4 1: available */
    s8          unk5;                       /* 0x5 (read only by F_0021D528, which has no callers) */
    s16         level;                      /* 0x6 the arcade level it needs */
    int         length;                     /* 0x8 (ArcadeControl::createCourseInfo:
                                               UnitSettings::convertDistance) */
};
struct GcfCourseList {
    const char *name;                       /* 0x0 */
    int         count;                      /* 0x4 */
    GcfCourse **courses;                    /* 0x8 */
};
struct GcfCourseSection {
    int             count;                  /* 0x0 */
    GcfCourseList **list;                   /* 0x4 */

    GcfCourseList *find(const char *list);              /* 0x0021D1D0 */
    GcfCourse     *course(int i, const char *list);     /* 0x0021D268 */
};

/* Type 0xC (T441): the attract demos (run_attract_demo, T000). */
struct GcfDemo {
    const char *race;                       /* 0x00 race code (RaceInformation::setRaceCode;
                                               ProductConfig::demoCar returns it) */
    const char *course;                     /* 0x04 course code (RaceDataBase::getCourseKind,
                                               RaceInformation::setCourseCode) */
    const char *shopCar;                    /* 0x08 car code or NULL (RaceArcadeDemoInformation::shopCar) */
    int         shopCarPos;                 /* 0x0C RaceArcadeDemoInformation::shopCarPos */
    int         requirement;                /* 0x10 shown once GameStatus::isLicenseSPassed() >= it */
};
struct GcfDemoSection {
    int       count;                        /* 0x0 */
    GcfDemo **list;                         /* 0x4 */
};

/* Types 0x4, 0x6, 0xA, 0xD (T445, T440, T443, T444): named values; the
 * first three are sorted by name (binary search).  value: an event's
 * requirement (GameStatus::isLicenseSPassed() >= it), a service price, a
 * memory card demo's level; type 0x6's is never read here. */
struct GcfNamed {
    const char *name;                       /* 0x0 */
    int         value;                      /* 0x4 */
};
struct GcfNamedTable {
    int        count;                       /* 0x0 */
    GcfNamed **list;                        /* 0x4 */
};

/* Type 0xF (T442): the prize car sets, sorted by name. */
struct GcfPrizeCar {
    int         unk0;                       /* 0x0 (never read here) */
    const char *code;                       /* 0x4 car code */
    int         color;                      /* 0x8 */
    int         weight;                     /* 0xC its chance */
};
struct GcfPrizeSet {
    const char   *name;                     /* 0x0 */
    int           count;                    /* 0x4 */
    GcfPrizeCar **cars;                     /* 0x8 */
};
struct GcfPrizeSection {
    int           count;                    /* 0x0 */
    GcfPrizeSet **list;                     /* 0x4 */
};

/* The regional car code table of gcf_regional_car_code (a function-local
 * static: 0x0049D0E0 .. 0x0049D140, its guard 0x00352AB4 gp-0x4F3C). */
struct GcfCarAlias {
    const char *from;                       /* 0x0 */
    const char *to;                         /* 0x4 */
    int         countries;                  /* 0x8 bit c: applies in country c (COUNTRY_*) */

#ifdef BOOT_OLD_GCC
    /* egcs builds the table with it at run time (the per-field stores
     * through a cursor in retail). */
    GcfCarAlias(const char *f, const char *t, int c) : from(f), to(t), countries(c) {}
#endif
};
EGCS_SIZE(GcfCarAlias, 0xC);


/* Relocation (T436..T445).  Each takes the section (or entry) and returns it. */
GcfHeader        *gcf_relocate_header(GcfHeader *hdr);            /* 0x0021E068 T436 */
GcfCar           *gcf_relocate_car(GcfCar *car);                  /* 0x0021E0B8 T437 */
GcfCarCategory   *gcf_relocate_car_category(GcfCarCategory *cat); /* 0x0021E0E0 T437 */
GcfCarSection    *gcf_relocate_cars(GcfCarSection *sec);          /* 0x0021E180 T437 */
GcfGeneral       *gcf_relocate_general(GcfGeneral *general);      /* 0x0021E208 T438 */
GcfCourse        *gcf_relocate_course(GcfCourse *course);         /* 0x0021E210 T439 */
GcfCourseList    *gcf_relocate_course_list(GcfCourseList *list);  /* 0x0021E228 T439 */
GcfCourseSection *gcf_relocate_courses(GcfCourseSection *sec);    /* 0x0021E2C8 T439 */
GcfNamed         *gcf_relocate_named_06(GcfNamed *entry);         /* 0x0021E350 T440 */
GcfNamedTable    *gcf_relocate_table_06(GcfNamedTable *table);    /* 0x0021E368 T440 */
GcfDemo          *gcf_relocate_demo(GcfDemo *demo);               /* 0x0021E3F0 T441 */
GcfDemoSection   *gcf_relocate_demos(GcfDemoSection *sec);        /* 0x0021E430 T441 */
GcfPrizeCar      *gcf_relocate_prize_car(GcfPrizeCar *car);       /* 0x0021E4B8 T442 */
GcfPrizeSet      *gcf_relocate_prize_set(GcfPrizeSet *set);       /* 0x0021E4D0 T442 */
GcfPrizeSection  *gcf_relocate_prizes(GcfPrizeSection *sec);      /* 0x0021E570 T442 */
GcfNamed         *gcf_relocate_named_0A(GcfNamed *entry);         /* 0x0021E5F8 T443 */
GcfNamedTable    *gcf_relocate_table_0A(GcfNamedTable *table);    /* 0x0021E610 T443 */
GcfNamed         *gcf_relocate_named_0D(GcfNamed *entry);         /* 0x0021E698 T444 */
GcfNamedTable    *gcf_relocate_table_0D(GcfNamedTable *table);    /* 0x0021E6B0 T444 */
GcfNamed         *gcf_relocate_named_04(GcfNamed *entry);         /* 0x0021E738 T445 */
GcfNamedTable    *gcf_relocate_table_04(GcfNamedTable *table);    /* 0x0021E750 T445 */

/* T435's data (only T435 uses it; defined there).  The regional car code
 * table and its guard are gcf_regional_car_code's local static. */
extern ProductConfig *g_00352AB0;           /* 0x00352AB0 gp-0x4F40 the instance (product_config) */

#endif /* U099_H */
