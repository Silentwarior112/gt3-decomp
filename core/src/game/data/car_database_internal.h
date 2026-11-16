/* u109 - GT3 main executable 0x00239ED0..0x0023D690, translation units
 * T489..T498:
 *   T489  car / race database queries (the CarDataBase g_002EBD88), the
 *         regulation check (RegurationResult), race-name parsing.
 *         game/data/T489_car_database.h
 *   T490  GarageCar's methods (+ copies of its inline destructor / typeinfo).
 *         The class is T036's: game/menu/gtmode/T036_garage_car.h (T490 has no header)
 *   T491  GarageWheel, Cell<GarageWheel>.  game/save/T491_garage_wheel.h
 *   T492  GT2KMSGDATABASEPARTS::Message ("DATABASEPARTS" text table).
 *         game/data/messages/T492_msg_database_parts.h
 *   T493  GT2KMSGDATABASERACECATEGORY::Message ("DATABASERACECATEGORY").
 *         game/data/messages/T493_msg_database_race_category.h
 *   T494  string / number formatting helpers, two quad drawers.
 *         game/common/T494_text_utils.h
 *   T495  gt2k_menu::pCell / pList, the intrusive doubly linked list.
 *         game/menu/framework/T495_menu_list.h
 *   T496  the menu font (Font) and its face dictionary.  engine/text/T496_font.h
 *   T497  the ELF (PJW) string hash (no header: only T496 uses it)
 *   T498  the font's text output (Font's methods, in engine/text/T496_font.h)
 *
 * Since the data pass (2026-10-17) each source includes the canonical
 * headers it needs itself, in the order that gives the original's type names
 * and strings (each file's comment says which): this header included ~30
 * headers (<new>'s "bad_alloc", T036_garage.h's operator list classes, the
 * GameStatus's garage) and put 518..846 bytes of type names into every
 * object.  For egcs it keeps only what u109 alone uses: the database record
 * of table 0x21, the unit's globals and the named values of the rules.
 * Offsets in comments are the original (egcs) ones. */
#ifndef U109_H
#define U109_H

#include "types.h"
#include <stddef.h>

/* ======================================================================== */
/* The car database (T481..T485): what only u109 names                      */
/* ======================================================================== */

/* The tables of paramdb that T489 reads (CarDataBase::getRecord /
 * getIndex / getDataBlock; T482_car_equipments.h lists them all). */
enum {
    DB_TABLE_CAR        = 0x1E,                 /* CarRecord */
    DB_TABLE_ENTRY_CAR  = 0x1F,                 /* PresetCarRecord: the entry cars of the races */
    DB_TABLE_RACE       = 0x20,                 /* RaceEvent (RaceRecord) */
    DB_TABLE_CAR_LIST   = 0x21,                 /* CarListRecord */
    DB_TABLE_COURSE     = 0x22,                 /* CourseRecord */
    DB_TABLE_PRESET_CAR = 0x23                  /* PresetCarRecord: the preset cars */
};

/* A record of table 0x21: the cars a race allows (RaceEvent::carList; u108
 * calls it PrizeCarsRecord).  Used only inside u108 / u109 (h11). */
struct CarListRecord {
    u64 code;                                   /* 0x00 */
    u64 cars[0x20];                             /* 0x08 car codes, "-----" ends the list */
};

/* CarEquipments::getDriveTrain's layouts: T482's LAYOUT_*; the rules of a
 * race record (RaceEvent::need*) as check_regulation reads them: T484's
 * NEED_*. */

/* Front tyre part ids (CarInformation::hasPart): PART_FRONT_TIRE + kind, of
 * T482's PART_* (which also has PART_NONE, 0x58). */
enum {
    PART_TIRE_SPORTS = 0x39,                    /* T036 text_price_tir_sports */
    PART_TIRE_SUPER_SOFT = 0x40,                /* T036 text_price_tir_super_soft */
    PART_TIRE_CONTROL = 0x41,                   /* T036 text_price_tir_control */
    PART_TIRE_DIRT = 0x42                       /* T036 text_price_tir_dirt (PART_FRONT_TIRE_DIRT) */
};

/* CarRecord::regionLock (region_lock_check): where the car is available.
 * The regions are CarDataBase::isJapan / isUS / isUK / isEurope, the
 * countries COUNTRY_JP / US / BR and the others (T516). */
enum {
    REGION_LOCK_NONE = 0,                       /* everywhere */
    REGION_LOCK_ONLY_BR = 1,
    REGION_LOCK_NOT_BR = 2,
    REGION_LOCK_ONLY_US = 3,
    REGION_LOCK_NOT_US = 4,
    REGION_LOCK_NOT_JP_US = 5,
    REGION_LOCK_ONLY_JP_US = 6,
    REGION_LOCK_ONLY_JP = 7,
    REGION_LOCK_NOT_JP = 8
};

/* enum RaceKind (race_name_split's kinds): game/data/T489_car_database.h. */

/* T489 globals (T489 defines them) */
/* The tyre part ids {PART_TIRE_CONTROL, 0x38, PART_TIRE_SPORTS ..
 * PART_TIRE_SUPER_SOFT, PART_NONE}: PART_NONE ends it (tire_part_index).
 * Not const: retail's is in .data. */
extern int g_002F0670[11];                      /* 0x002F0670 */
extern const char **g_00352F00;                 /* 0x00352F00 gp-0x4AF0 the short names of the cars
                                                   (db_car_short_names, built once) */

#define POWER_UNIT_PS 0x7073                    /* 'ps': ShowRoomCar::power's unit tag */

/* ======================================================================== */
/* T490 / T491 / T492 / T493: RTTI storage used only here                   */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                    /* __rtti_class / __rtti_si / __rtti_user,
                                                   __class_type_info::base_info */
extern "C" {
/* T490's copy of GarageCar's base list (the same words as T036's
 * g_00311EA0): {Cell<GarageCar> 0x004B1350, offset 0, public},
 * {CarInformation 0x004B2A50, offset 0x10, public}. */
extern const __class_type_info::base_info g_003428A8[2];   /* 0x003428A8 */
extern u32 __ti11GarageWheel[3];                            /* 0x004B1280 __si_type_info */
extern u32 __tiQ29gt2k_menut4Cell1Z11GarageWheel[3];        /* 0x004B1370 __si_type_info */
extern u32 __tiQ220GT2KMSGDATABASEPARTS7Message[3];         /* 0x004B2A80 __si_type_info */
extern u32 __tiQ227GT2KMSGDATABASERACECATEGORY7Message[3];  /* 0x004B2A90 __si_type_info */
}
#endif

/* T492 / T493 .data: the keys of the tables (the texts are declared in the
 * files' headers; the instances g_002F1188 / g_002F12D8 are T492's / T493's
 * own).  T492 / T493 define them. */
extern const char *g_002F0F08[0x50];            /* 0x002F0F08 DATABASEPARTS keys */
extern const char *g_002F1268[14];              /* 0x002F1268 DATABASERACECATEGORY keys
                                                   (TEXT_CATEGORY_ARCADE ..) */

/* ======================================================================== */
/* T496 / T497 / T498: the font                                             */
/* ======================================================================== */

/* G001's font renderer parameters (font_set_param / font_get_param; u114's
 * names, local there too). */
#define FONT_PARAM_SCALE_X    0x1100
#define FONT_PARAM_SCALE_Y    0x1101
#define FONT_PARAM_SPACING_X  0x1102
#define FONT_PARAM_SPACING_Y  0x1103

/* T497: the ELF (PJW) hash.  FontDict's bucket function (% 15). */
u32 hash_string(const char *s);                 /* 0x0023D3D0 */
u32 hash_bytes(const u8 *p, int len);           /* 0x0023D420 (no callers) */

#endif /* U109_H */
