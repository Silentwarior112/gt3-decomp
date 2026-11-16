/* T265_raw_window_register.h - gt2k_menu::RawWindowRegister, the RawWindow
 * class registry (translation unit T265, unit u075).
 *
 * Every window unit's static constructor builds one RawWindowRegister per
 * class (T272: the 16 Raw* classes, .sdata 0x00352770..0x003528E0; the Play*
 * and Logger* units theirs); the constructor pushes it on the list whose head
 * is the .sdata word 0x003528F8 (gp-0x50F8).  A layout names the class of a
 * window; createRegistered finds the record by name and calls its create
 * function.  The record is RawWindowClass {name, create} (planned in T085,
 * u037 having the fullest copy; it is this translation unit's). */
#ifndef GAME_T265_RAW_WINDOW_REGISTER_H
#define GAME_T265_RAW_WINDOW_REGISTER_H

#include "game/game.h"

namespace gt2k_menu {
struct RawWindowArgs;                       /* T264 (pointers only) */
class RawWindow;                            /* T264 */

/* One record of the class registry (.sdata, beside its registrar): the
 * class name the layouts use and the class's create function. */
struct RawWindowClass {
    const char *name;                       /* 0x0 */
    RawWindow *(*create)(const RawWindowArgs *args);   /* 0x4 */
};
EGCS_SIZE(gt2k_menu::RawWindowClass, 0x8);

/* Not polymorphic.  Size 8 (T272's .sdata: the RawCheckBox registrar
 * 0x00352770, then the RawLabel name pointer 0x00352778). */
class RawWindowRegister {
public:
    RawWindowRegister(const RawWindowClass *cls);  /* 0x001D8220 next = the list head; the head = this */
    ~RawWindowRegister();                   /* 0x001D8238 does not unlink; the deleting form calls the
                                               global operator delete (the static objects: flags 2) */

    RawWindowRegister    *next;             /* 0x0 */
    const RawWindowClass *cls;              /* 0x4 */
};
EGCS_SIZE(gt2k_menu::RawWindowRegister, 0x8);

/* The registrar of the class named name (strcmp with cls->name), NULL if
 * none. */
RawWindowRegister *findRegistered(const char *name);                    /* 0x001D8178 */
/* cls->create(args) of the class named cls, NULL if it is not registered. */
RawWindow *createRegistered(const char *cls, const RawWindowArgs *args); /* 0x001D81D8 */

} /* namespace gt2k_menu */

#endif /* GAME_T265_RAW_WINDOW_REGISTER_H */
