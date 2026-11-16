/* T105_not_car_get.h - gt2k_menu::NotCarGetControl, the page shown instead
 * of a prize car when the garage is full (translation unit T105, unit
 * u020), 0x0013B380..0x0013B848.
 *
 * not_car_get (called by T061's car_get_prizes when the garage already
 * holds 200 cars) runs page "not_car_get" of "menu/not_get_car.mbl" on a
 * 256 KB menu heap with the controller on the stack; the page has no item
 * factories and no actions.  The end of the translation unit has
 * unreferenced copies of ControlBaseWithPageManager's implicit destructor
 * and typeinfo function (0x0013B7D8 / 0x0013B828, documented with the class
 * in T056). */
#ifndef GAME_T105_NOT_CAR_GET_H
#define GAME_T105_NOT_CAR_GET_H

#include "game/game.h"
/* only ControlBaseWithPageManager: retail T105's .rodata has the type names
 * NotCarGetControl, PrizeControlBase, ControlBaseWithPageManager in this
 * order (egcs: reverse definition order), none of T056's screens */
#define T056_BASE_ONLY
#include "game/menu/gtmode/T056_gtmode_trophy_control.h" /* base gt2k_menu::ControlBaseWithPageManager */
#undef T056_BASE_ONLY
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */

namespace gt2k_menu {

/* vtable 0x00318F88 (2 slots: [1] ControlBase::waitFrame).  typeinfo
 * __tfQ29gt2k_menu16NotCarGetControl 0x0013B738 (si, base
 * ControlBaseWithPageManager; storage 0x004B1810).  Size 0xE40 (sp+0x000
 * in not_car_get; inline constructor, implicit destructor). */
class NotCarGetControl : public ControlBaseWithPageManager {
public:
    NotCarGetControl() : render() {}
    /* render, ~ControlBase. */
    /* [0] +0x0C 0x0013B788 ~NotCarGetControl: implicit (no vptr store) */
    void load();                            /* 0x0013B380 "menu/not_get_car.mbl" */
    void unload();                          /* 0x0013B3A8 */
    void begin();                           /* 0x0013B3C8 the render with a transition; empty item tables */
    void end();                             /* 0x0013B480 */
    void mainLoop();                        /* 0x0013B4C0 page "not_car_get" until it is left */
    void run();                             /* 0x0013B580 load, begin, mainLoop, end, unload */

    /*         ControlBaseWithPageManager      0x000 .. 0xDE8 */
    RenderBase render;                      /* 0xDE8 */
};
EGCS_SIZE(gt2k_menu::NotCarGetControl, 0xE40);

} /* namespace gt2k_menu */

/* 0 if the menu heap cannot be allocated, else 1. */
int not_car_get(void);                      /* 0x0013B5C8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu16NotCarGetControl(void);  /* 0x0013B738 */
#endif

#endif /* GAME_T105_NOT_CAR_GET_H */
