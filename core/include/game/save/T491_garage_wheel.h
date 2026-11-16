/* T491_garage_wheel.h - GarageWheel, a garage slot holding a wheel name
 * (translation unit T491, unit u109), 0x0023B350..0x0023B564; also T491's
 * copies of Cell<GarageWheel>'s destructor and typeinfo. */
#ifndef GAME_T491_GARAGE_WHEEL_H
#define GAME_T491_GARAGE_WHEEL_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                      /* base gt2k_menu::Cell<T> (over pCell, T495) */

class GarageWheel;

/* Cell<GarageWheel>: vtable 0x00342988, typeinfo
 * __tfQ29gt2k_menut4Cell1Z11GarageWheel 0x0023B4F0 (si, base pCell; storage
 * 0x004B1370).  Its destructor is the template's inline one (menu_list.h:
 * the vptr, then ~pCell); 0x0023B540 is the instance egcs emits in T491, the
 * one its vtable uses (no declaration: an explicit specialization would be
 * a function of its own). */

/* vtable 0x003428F0 (3 slots: [1] / [2] are pCell's isBefore 0x0023C120 /
 * isNot 0x0023C128).  typeinfo __tf11GarageWheel 0x0023B478 (si, base
 * Cell<GarageWheel>; storage 0x004B1280).  Bases: Cell<GarageWheel> at +0x00
 * (pCell: next, prev, vptr +0x8).  Size 0x50 (0xC + the 0x44 bytes it saves). */
class GarageWheel : public gt2k_menu::Cell<GarageWheel> {
public:
    GarageWheel();                              /* 0x0023B350 valid = 0, name = "" */
    /* [0] +0x0C 0x0023B4C8 ~GarageWheel: implicit (virtual through
     * Cell<GarageWheel>; no key function): retail emits it in T491 among the
     * vtable's members, right after __tf11GarageWheel (T491's frame info:
     * __tf11GarageWheel 0x0023B478, ~GarageWheel 0x0023B4C8), the place egcs
     * gives an implicit destructor (an in-class `virtual ~GarageWheel() {}`
     * comes before the __tf: u109's data pass).  T137 / T138 / T084 / ...
     * have the type name "11GarageWheel" in their .rodata, which egcs writes
     * only for a class without a key function (u109 matching: T138's six
     * __tf exact with data). */
    void        clear();                        /* 0x0023B378 empties the slot */
    int         setName(const char *s);         /* 0x0023B388 name = s ("" for NULL), valid = 1; returns 1 */
    const char *getName();                      /* 0x0023B3C8 NULL if empty */
    s64         getCode();                      /* 0x0023B3E8 the 64-bit code of the name */
    static int  saveSize();                     /* 0x0023B408 0x44 */
    const u8   *load(const u8 *src);            /* 0x0023B410 valid + name; the end of what it read */
    u8         *save(u8 *dst);                  /* 0x0023B440 valid + name; the end of what it wrote */

    int  valid;                                 /* 0x0C the slot is used */
    char name[0x40];                            /* 0x10 the wheel; "" = the car's own wheel */
};
EGCS_SIZE(GarageWheel, 0x50);

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tf11GarageWheel(void);                  /* 0x0023B478 */
void *__tfQ29gt2k_menut4Cell1Z11GarageWheel(void); /* 0x0023B4F0 */
}
#endif

#endif /* GAME_T491_GARAGE_WHEEL_H */
