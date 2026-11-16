/* T036_garage_car.h - GarageCar, a slot of the garage (its methods are
 * T490's, unit u109; the class was in T036_garage.h).
 *
 * In its own header since u040's data pass (2026-10-17): the GameStatus
 * (T429) and the MenuStatus (T427) hold GarageCars by value, and through
 * T036_garage.h every unit that saw them also saw T036's hook declarations
 * and the menu classes that header includes (Item, PageLayout, MenuBgMovie:
 * the type names of OperatorList, List / Cell<OperatorHolder>,
 * OperatorHolder and Operator).  Retail's T089 sees the GameStatus and none
 * of those (its .rodata 0x00324F90..0x003251C8: its own classes' names,
 * RegurationResult's and the garage lists' only).  T036_garage.h includes
 * this header where it defined the class, so its includers see the same
 * classes in the same order as before. */
#ifndef GAME_T036_GARAGE_CAR_H
#define GAME_T036_GARAGE_CAR_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                 /* base gt2k_menu::Cell<GarageCar> */
#include "game/data/T483_car_information.h"      /* base CarInformation */

/* A slot of the garage (GameStatus::cars[200], T429; the menu status's car
 * buffers, T427).  Bases: gt2k_menu::Cell<GarageCar> at +0x00 (vptr +0x08)
 * and CarInformation at +0x10 (vptr +0x1EC).  vtables 0x00311C30 (primary, 3
 * slots: [1] / [2] pCell's 0x0023C120 / 0x0023C128) and 0x00311C20 (the
 * CarInformation part, 1 slot, this-delta -0x10); more {secondary, primary}
 * pairs 0x00312FE8 (T096), 0x00316030, 0x00320AC8, 0x003403E0, 0x00340698,
 * 0x00342818, all with T036's destructor.  typeinfo __tf9GarageCar
 * 0x00125A38 (class, base list g_00311EA0 {Cell<GarageCar> +0, CarInformation
 * +0x10}; storage 0x004B1230; it builds CarInformation's node first).  Size
 * 0x230, 8-aligned (CarInformation's u64).
 *
 * The methods are T490's (u109; T490 has no header). */
class GarageCar : public gt2k_menu::Cell<GarageCar>, public CarInformation {
public:
    /* Cell(), CarInformation(), then the vptrs; index -1, valid 0. */
    GarageCar();                            /* 0x0023AEA0 */
    /* [0] +0x0C 0x00125A98 ~GarageCar: implicit (virtual through the
     * bases): both vptrs, ~CarInformation (flags 0), then Cell<GarageCar>'s
     * destructor with the flags, inline where it was compiled first.
     * Out-of-line copies 0x00125A98 (T036, the one in the vtables),
     * 0x00128280 (T096), 0x00133E90 (T053), 0x0015D390 (T080), 0x00218198
     * (T427), 0x0021AA50 (T429), 0x0023B2A0 (T490).  T490's calls
     * Cell<GarageCar>'s destructor (0x00125A10) instead: egcs synthesizes
     * the implicit destructor there before Cell<GarageCar>'s is compiled; a
     * user-declared inline one inlines it (u109 matching). */
    void clear();                           /* 0x0023AF40 CarInformation::clear(), valid 0, index -1 */
    void setCar(const char *code);          /* 0x0023AF78 loadCar(nameToCode(code)), valid 1 */
    const char *getName();                  /* 0x0023AFC0 the car's code name (getShortName), NULL if
                                               !valid */
    u32  getColor(u32 orMask);              /* 0x0023AFE8 the RGBA8 of the car's colour | orMask */
    const char *getModelName();             /* 0x0023B040 car_list_name(getModelCode()) (T446) */
    const char *getWheelModelName();        /* 0x0023B068 the wheel to show: getModelName() for the
                                               default wheel, else getWheelName() */
    /* Copies the CarInformation, index and valid (? operator=); *this. */
    GarageCar &assign(const GarageCar &o);  /* 0x0023B0C0 */
    void getPower(int *power, int *rpm);    /* 0x0023B110 the power ('ps') of a spec built with the
                                               car's condition, and its rpm */
    static int saveSize();                  /* 0x0023B168 CarInformation::dataSize() + 8 (this not
                                               passed on) */
    const u8 *load(const u8 *src);          /* 0x0023B188 the CarInformation, valid, index; the end */
    u8  *save(u8 *dst);                     /* 0x0023B1D0 */

#ifdef BOOT_OLD_GCC
    /* bool (4 bytes in egcs, an alias set of its own): assign's two loads
     * pass the int store (u109 matching, 0x0023B0C0) */
    bool valid;                             /* 0x228 the slot is used */
#else
    int valid;                              /* 0x228 (int: the saved slot's layout) */
#endif
    int index;                              /* 0x22C the car's serial number, -1 */
};
EGCS_SIZE(GarageCar, 0x230);

#ifdef BOOT_OLD_GCC
/* (data layout only: egcs's function order; an exact form, not the
 * original) Cell<GarageCar>'s destructor, an inline template member, is
 * instantiated when a function body first uses it: the original did that
 * right after GarageCar's class.  Retail T429's end-of-unit copies (0x0021A978
 * __tf of List<GarageCar>, 0x0021A9C8 Cell<GarageCar>::~Cell, 0x0021A9F0
 * __tf9GarageCar) put its instantiation after GarageCar's and before
 * List<GarageCar>'s (GarageCarList's base); in T036, T053 and T080 it comes
 * before the classes defined after the garage (RegurationResult,
 * ShortMenuRankEntry, SettingControl).  Without a use here the first is in
 * the unit's code (T036's text_ps_after) or at the end of the unit, and the
 * copy lands first among the end-of-unit functions (the frame info's order;
 * ref/data/u010/notes.md).  What the original had here is not known (an
 * inline function or constructor that destroys a Cell<GarageCar>; not
 * ~GarageCar: T490's copy, synthesized at the end, calls ~Cell instead of
 * inlining it).  Never called; it emits nothing. */
inline void garage_car_cell_delete(gt2k_menu::Cell<GarageCar> *cell)
{
    delete cell;
}
#endif

#endif /* GAME_T036_GARAGE_CAR_H */
