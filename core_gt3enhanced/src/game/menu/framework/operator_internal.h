/* operator_internal.h - unit u066 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001BBC58..0x001BF390, second pass.
 *
 * Translation units (link order):
 *   T217  gt2k_menu::Operator: the interpreter of the operator scripts that
 *         animate a Transitive (position, size, colours, alpha, blur, F0..F3),
 *         34 handlers dispatched through the pointer-to-member table
 *         g_002C8850 (defined in T217)
 *   T218  Color4f: RGBA8 <-> float colour, colour order by HSV
 *   T219  gt2k_menu::GarageButtonList / GarageButton: the car list of the
 *         garage ("hm/hm_gr_panel"): one button per garage car with an info
 *         column (power, price, maker, distance, drive, aspiration; L2 / R2
 *         switch it), a colour chip and a mark on the current car; nine sort
 *         orders (g_002C8DF8)
 *
 * Every class is defined in its canonical header (include/game/T217..T219
 * and the menu library's).  Each source includes the canonical headers it
 * needs itself, in the order that gives the original's type names (data
 * pass); this header only adds what T219 uses besides them: the sort orders,
 * the info columns, the comparator table, and the typeinfo storage and base
 * lists of its classes for GCC 14.  T217 and T218 do not include it.
 */
#ifndef U066_H
#define U066_H

#include "types.h"

/* ======================================================================== */
/* T219 - data only used here                                               */
/* ======================================================================== */
/* GarageButtonList::sortType: the index in g_002C8DF8. */
enum GarageSortType {
    GARAGE_SORT_INDEX      = 0,             /* cmp_index: the garage index (GarageCar::index) */
    GARAGE_SORT_NAME       = 1,             /* cmp_name */
    GARAGE_SORT_MAKER      = 2,             /* cmp_maker */
    GARAGE_SORT_COLOR      = 3,             /* cmp_color */
    GARAGE_SORT_PRICE      = 4,             /* cmp_price */
    GARAGE_SORT_DISTANCE   = 5,             /* cmp_distance */
    GARAGE_SORT_POWER      = 6,             /* cmp_power */
    GARAGE_SORT_DRIVE      = 7,             /* cmp_drive */
    GARAGE_SORT_ASPIRATION = 8,             /* cmp_aspiration */
    GARAGE_SORT_COUNT      = 9
};

/* The info column (GarageButton::infoMode->value; the column header's
 * child index). */
enum GarageInfoMode {
    GARAGE_INFO_POWER      = 0,             /* "power" */
    GARAGE_INFO_PRICE      = 1,             /* "price" (the sell price) */
    GARAGE_INFO_MAKER      = 2,             /* "maker" */
    GARAGE_INFO_DISTANCE   = 3,             /* "distance" (the odometer) */
    GARAGE_INFO_DRIVE      = 4,             /* "drive" */
    GARAGE_INFO_ASPIRATION = 5,             /* "aspiration" */
    GARAGE_INFO_COUNT      = 6
};

/* T219 .data: the qsort comparators by sortType (over GarageButton *),
 * defined in T219 after the comparators (refresh reads it before). */
extern int (*g_002C8DF8[GARAGE_SORT_COUNT])(const void *, const void *); /* 0x002C8DF8 */
/* (T219 .bss: the record handed to the page opened, g_0049C840, is a file
 * static of T219.) */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __class_type_info::base_info */
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The __class_type_info storage (.bss, 4 words) and the base lists (.sdata,
 * one public base at +0 each). */
extern "C" {
extern u32 __tiQ29gt2k_menu16GarageButtonList[4];          /* 0x004B2520 */
extern u32 __tiQ29gt2k_menu12GarageButton[4];              /* 0x004B2510 */
}
extern const __class_type_info::base_info g_003525D8[1];   /* 0x003525D8 GarageButtonList: {ButtonList
                                                              0x004B2650, +0} */
extern const __class_type_info::base_info g_003525E0[1];   /* 0x003525E0 GarageButton: {Button
                                                              0x004B2450, +0} */
#endif /* !BOOT_OLD_GCC */

#endif /* U066_H */
