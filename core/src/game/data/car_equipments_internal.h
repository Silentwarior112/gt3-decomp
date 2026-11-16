/* car_equipments_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u106,
 * 0x00231190..0x00234020: translation units T481, T482 (T482 is one file
 * here, T482_car_equipments.cpp, which also holds the functions at u107's
 * and u108's addresses up to 0x00237860).
 *
 *   T481  The car database: CarDataBase holds the three files
 *         database/paramdb_us.db (the parameter tables, read into a
 *         64-byte-aligned buffer), database/paramstr_us.db and
 *         database/paramunistr_us.db (string tables).  The parameter file is
 *         a header with an index of tables; each table is a DataBlock:
 *         fixed-size records sorted by their 64-bit code (searched by
 *         bisection).  A code is the rotate-left-7 hash of a name
 *         (nameToCode).  One global instance, g_002EBD88.
 *   T482  CarEquipments: a car (its code, colour, the code of each fitted
 *         part and the settings of the adjustable parts) over the database:
 *         loading a car (its stock parts, or a table 0x23 preset), the
 *         default settings of a part, and getSpec, which builds the car
 *         parameter block (CarSpec) from the records of all the parts; the
 *         records of the fitted parts, the part ids (0x00..0x56, 0x58 =
 *         none: getPartCategory / getPartKind, the get<Part>Part() kind ->
 *         id tables below) and their EUC-JP names (g_002F1048); the tyre
 *         helpers, the save block copies, the class-inline accessors and
 *         part setters, the typeinfo functions.
 *
 * Every class is defined in its canonical header (include/game/T481, T482);
 * this header adds the outside declarations the units need and what only
 * they use: the typeinfo storage of DataBlock / StringsDataBase (T481's
 * .sbss) and of CarEquipments / CarSaveEquipments, and T482's data tables.
 * CARDB_*, PART_*, TIRE_KIND_*, LAYOUT_*: game/data/T482_car_equipments.h.
 */
#ifndef U106_H
#define U106_H

#include "types.h"
#include "libc.h"                       /* strlen, memset, memcpy, free, memalign */
#include "cxxrt.h"                      /* type_info, __rtti_user */
/* No <new> (u106): its "bad_alloc" would start T481's .rodata, retail's
 * starts with "database/paramstr_us.db". */

/* own translation units */
#include "game/data/T481_car_data_base.h"
#include "game/data/T482_car_equipments.h"

/* outside the units */
#include "game/car/physics/T294_gear_auto.h"        /* gear_auto_ratios, the ratios of an auto level (T482) */
#include "game/data/T485_car_color.h"        /* g_00352D98, the colour lists of the models (T482) */
#include "game/data/T489_car_database.h"     /* region_lock_check (T482) */
#include "game/data/messages/T492_msg_database_parts.h"   /* g_002F1048, the part names (T482) */
#include "engine/io/T502_file_resolve.h"     /* file_size_retry (a file's size) */
#include "engine/io/T503_vol_thread.h"       /* vol_device_object, file_read_blocking */
#include "game/data/T516_country.h"          /* g_00352FD8 (the country), COUNTRY_* */
#include "game/data/T518_unit_settings.h"    /* g_002F59D8, the units of power and torque (T482) */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"            /* __rtti_class, __class_type_info::base_info (T482's
                                           typeinfo functions, written out for GCC 14 only) */
#endif

/* ======================================================================== */
/* RTTI (GCC 14 only: egcs generates the typeinfo functions and storage)    */
/* ======================================================================== */

#ifndef BOOT_OLD_GCC
/* The type_info storage of the classes nobody else names (__ti11CarDataBase
 * is in T481_car_data_base.h): __class_type_info = 4 words,
 * __user_type_info = 2. */
extern "C" {
extern u32 __ti9DataBlock[2];           /* 0x00353488 gp-0x4568 */
extern u32 __ti15StringsDataBase[2];    /* 0x00353498 gp-0x4558 */
extern u32 __ti13CarEquipments[4];      /* 0x004B2A40 */
extern u32 __ti17CarSaveEquipments[2];  /* 0x003534A0 gp-0x4550 */
}

/* The __class_type_info base list of CarEquipments (.rodata), both bases
 * public. */
extern const __class_type_info::base_info g_003422E0[2];   /* 0x003422E0 {CarSaveEquipments, +0},
                                                               {CarDataBase, +0x1D0} */
#endif /* !BOOT_OLD_GCC */

/* ======================================================================== */
/* T482 data (TU-private)                                                   */
/* ======================================================================== */

/* Defined in T482 (u106's data pass), before getBrakePart: the two name
 * tables, then the kind -> part id tables in part-id order except the drive
 * train's (retail's .sdata has it after the brake controller's).  Not
 * `const': retail has them in .data / .sdata (a const table would be
 * .rodata). */

/* part id (0x00..0x56) -> index of g_002F1048: the short name
 * (getPartName: "ノーマル", "ステージ１", "装着済み" (fitted), "施工済み"
 * (done), "-", ...) and the full one (getPartFullName: "クラッチ／スポーツ",
 * ...).  PART_COUNT entries; the 2 zero bytes after each are the 8-byte
 * alignment of the next table (index 0x57 reads them, PART_NONE the next
 * table's first entry). */
extern u16 g_002EC3C0[PART_COUNT];          /* 0x002EC3C0 */
extern u16 g_002EC470[PART_COUNT];          /* 0x002EC470 */

/* The maker names ("acura", "alfaromeo", ... "volkswagen", 46) and the same
 * list with the tuners ("mines", "mugen", "nismo", "spoon", "toms"). */
extern const char *g_002EFF20[];            /* 0x002EFF20 */
extern const char *g_002EFFD8[];            /* 0x002EFFD8 */

/* kind -> part id; PART_NONE for a kind without a part. */
extern int g_002EC520[4];                   /* 0x002EC520 clutch 0x04..0x07 */
extern int g_002EC530[3];                   /* 0x002EC530 displacement 0x0A..0x0C */
extern int g_002EC540[4];                   /* 0x002EC540 flywheel 0x0F..0x12 */
extern int g_002EC550[4];                   /* 0x002EC550 transmission 0x13..0x16 */
extern int g_002EC560[3];                   /* 0x002EC560 intercooler 0x17..0x19 */
extern int g_002EC570[4];                   /* 0x002EC570 lightweight 0x1A..0x1D */
extern int g_002EC580[6];                   /* 0x002EC580 LSD 0x1E..0x23 */
extern int g_002EC598[4];                   /* 0x002EC598 muffler 0x24..0x27 */
extern int g_002EC5A8[4];                   /* 0x002EC5A8 NA tune 0x28..0x2B */
extern int g_002EC5B8[4];                   /* 0x002EC5B8 suspension 0x32..0x35 */
/* front / rear tyres, kinds 0..24 (TIRE_KIND_*): 0..9 -> 0x38..0x41 /
 * 0x43..0x4C, 0xA / 0xB -> PART_NONE, 0xC (dirt) -> 0x42 / 0x4D, and the
 * second set 13..24 as 0..11 (the table's own entries: the word after them
 * is the 8-byte alignment of the next table). */
extern int g_002EC5C8[25];                  /* 0x002EC5C8 */
extern int g_002EC630[25];                  /* 0x002EC630 */
extern int g_002EC698[5];                   /* 0x002EC698 turbine 0x4E..0x52 */
/* (T482 .sdata) */
extern int g_00352CF8[2];                   /* 0x00352CF8 gp-0x4CF8 brake 0x00, 0x01 */
extern int g_00352D00[2];                   /* 0x00352D00 gp-0x4CF0 brake controller 0x02, 0x03 */
extern int g_00352D08[2];                   /* 0x00352D08 gp-0x4CE8 drive train 0x55, 0x56 (VCD) */
extern int g_00352D10[2];                   /* 0x00352D10 gp-0x4CE0 computer 0x08, 0x09 */
extern int g_00352D18[2];                   /* 0x00352D18 gp-0x4CD8 engine balance 0x0D, 0x0E */
extern int g_00352D20[2];                   /* 0x00352D20 gp-0x4CD0 port polish 0x2C, 0x2D */
extern int g_00352D28[2];                   /* 0x00352D28 gp-0x4CC8 propeller shaft 0x2E, 0x2F */
extern int g_00352D30[2];                   /* 0x00352D30 gp-0x4CC0 racing modification 0x30, 0x31 */
extern int g_00352D38[2];                   /* 0x00352D38 gp-0x4CB8 TCSC 0x36, 0x37 */
extern int g_00352D40[2];                   /* 0x00352D40 gp-0x4CB0 ASCC 0x53, 0x54 */

#endif /* U106_H */
