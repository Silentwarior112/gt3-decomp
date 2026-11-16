/* arcade_course_select_internal.h - unit u005 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0010C9E0..0x0010FF90, second pass: the arcade course select and the
 * small arcade menu units.
 *
 * Translation units (link order), each with its canonical header:
 *   T024  gt2k_menu::MiniCursor, the strip of small pictures under the
 *         arcade selects (include/game/menu/framework/T024_mini_cursor.h)
 *   T025  "ar/select_course", the arcade course select: ArcadeControl
 *         methods (declared with the class, T010_arcade_control.h)
 *   T026  ArcadeControl::raceCode, the race of the arcade selection (T010)
 *   T027  gt2k_menu::LevelButtonList / GoodiesButtonList and their
 *         MenuFuncData table (T027_level_button_list.h)
 *   T028  the ExecuteFuncData hooks of the arcade menu
 *         (T028_arcade_menu_hooks.h; setModeByName is T010's)
 *   T029  the arcade background movie table (T029_arcade_movies.h)
 *
 * Every class is defined in a canonical header.  Each file includes the
 * canonical headers it needs itself, in the order that gives retail's type
 * names (egcs writes the type names of the classes a file sees in the
 * reverse order of their definition; each file's comment lists retail's),
 * then this header: the data only this unit reads (T026's tables, the
 * typeinfo storage of the unit's classes), T026's file-local helper and a
 * few named constants.  It includes no class.
 */
#ifndef U005_H
#define U005_H

#include "types.h"
#include <stddef.h>
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_class, __class_type_info::base_info */
#endif

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* menu_play_se ids: T233's MENU_SE_* (MENU_SE_LIST_MOVE is MENU_SE_LIST_MOVE). */

/* enum ArcadeClass (ARCADE_CLASS_*): game/menu/arcade/T010_arcade_control.h. */

/* COURSE_KIND_* (RaceDataBase::getCourseKind): game/data/T484_race_database.h. */

/* ShowRoomCar::power's unit tag (a multi-character constant; not read). */
enum { UNIT_TAG_PS = 0x7073 /* 'ps' */ };

/* The menus' virtual screen width: T259's MENU_SCREEN_W. */

/* ======================================================================== */
/* T025 / T026 data (.bss / .data / .rodata only this unit reads)           */
/* ======================================================================== */

/* T025's static buffers (0x0049BB50 courseMovieName's result, 0x0049BB90
 * F_0010E200's) are function-local statics of T025. */

/* T026: the class of a home garage car by its weight / power ratio
 * (.rodata, 0xC bytes a row): lo <= ratio < hi gives the class letter and
 * a step added to the race code's last digit.  The row whose letter is '-'
 * ends the table (and is used when no row matches). */
struct CarClassRange {
    f32  lo;                                /* 0x0 */
    f32  hi;                                /* 0x4 */
    char letter;                            /* 0x8 'S', 'A', 'B', 'C'; '-' ends */
    u8   step;                              /* 0x9 added to the race code's last digit */
    u8   pad_A[2];
};
extern const CarClassRange g_0030B660[];    /* 0x0030B660 */
/* The race code column of each ArcadeControl::class1 value: 0 C, 1 B, 2 A,
 * 3 S, 4 dirt (the garages' 0 is not used: their row is from the ratio). */
extern const int  g_0030B810[];             /* 0x0030B810 */
/* The arcade race codes [level][column]: "A" + level letter (E, N, H, P) +
 * class (C, B, A, S) + "T0001", "A?AD0001" for dirt (.data). */
extern const char *g_002924B0[4][5];        /* 0x002924B0 */
/* raceCode's result buffers g_0049BBD0 / g_0049BBE0 are T026's file statics. */

/* T026's file-local helper (its only caller is ArcadeControl::raceCode). */
void garage_car_race_code(char *dst, const char *base);    /* 0x0010F228 */

/* ======================================================================== */
/* RTTI storage of the unit's classes                                       */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC                        /* egcs generates the __tf functions and their storage */
extern "C" {
extern u32 __tiQ29gt2k_menu10MiniCursor[3];         /* 0x004B14B0 __si_type_info (base Item) */
extern u32 __tiQ29gt2k_menu17GoodiesButtonList[4];  /* 0x004B14D0 __class_type_info */
extern u32 __tiQ29gt2k_menu15LevelButtonList[4];    /* 0x004B14F0 __class_type_info */
}
/* The base list both T027 classes' __class_type_info share: one public
 * base, ButtonList at offset 0 ({__tiQ29gt2k_menu10ButtonList 0x004B2650,
 * 0x40000000}). */
extern const __class_type_info::base_info g_00350E90[1];   /* 0x00350E90 */
#endif /* !BOOT_OLD_GCC */

#endif /* U005_H */
