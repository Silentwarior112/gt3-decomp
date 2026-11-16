/* T080_setting_control.h - gt2k_menu::SettingControl and setting_menu, the
 * car setting menu (translation unit T080, unit u032), 0x0015C998..0x0015D448.
 *
 * setting_menu (from the GT-mode / race menus) runs SettingControl
 * ("menu/setting.mbl", page "root") over a 256 KB menu heap, on a copy of the
 * GameStatus's setting car (GameStatus menu state +0x540).  Its pages (the
 * hooks of T081 .. T084 and T096, all with the SettingControl as context)
 * end it with a request: 1 load or 2 save a SettingSerialize (T488) through
 * the memory card screens (F_0014AE00, T068, modes 0x17 / 0x16; g_00352EE8,
 * T077, points at it), after which the menu runs again ("load_warning" over
 * "root" when the loaded setting did not fit the car); 0 leaves.
 *
 * The rest of T080 (0x0015D308..0x0015D448) is T080's unreferenced copies of
 * Cell<GarageCar>'s and GarageCar's inline destructors and of their typeinfo
 * functions (T036 has the ones in use). */
#ifndef GAME_T080_SETTING_CONTROL_H
#define GAME_T080_SETTING_CONTROL_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pages) */
/* GarageCar only (T036_garage.h, which includes the menu classes, Item and
 * its operator bases, after the class: u013's data pass).  Retail T080 and
 * T096 have SettingControl's type name after the operator classes' (it was
 * defined before them): the original header did not see them. */
#include "game/menu/gtmode/T036_garage_car.h"           /* GarageCar (car) */

class RaceOrganization;                     /* T125 (pointers only) */

namespace gt2k_menu {
class Slider;                               /* T238 (pointers only) */
class Button;                               /* T206 */

/* vtable 0x00320AB0 (2 slots: [1] ControlBase's waitFrame 0x001CBD30).
 * typeinfo __tfQ29gt2k_menu14SettingControl 0x0015D210 (si, base
 * ControlBase; storage 0x004B1610).  ControlBase's data +0x000 .. +0x288,
 * vptr +0x288.  Size 0x10C8, 8-aligned (GarageCar's u64): setting_menu's
 * stack object.  The context of the setting pages' hooks (T081 .. T084,
 * T096). */
class SettingControl : public ControlBase {
public:
    /* request: what setting_menu does once the pages are left (T083's
     * "go_load" / "go_save" set 1 / 2; ? names). */
    enum REQUEST {
        REQUEST_NONE = 0,                   /* leave the setting menu */
        REQUEST_LOAD = 1,                   /* load a setting from a card, then the menu again */
        REQUEST_SAVE = 2                    /* save the setting car's setting, then the menu again */
    };

    /* ControlBase(), the vptr, render, pages(this), car; request and changed
     * cleared. */
    SettingControl();                       /* 0x0015C998 */
    /* No vptr store: car's inline ~GarageCar, render's ~RenderBase, then
     * ~ControlBase with the flags (which does the delete). */
    /* [0] +0x0C 0x0015D260 ~SettingControl: implicit (no vptr store) */

    void load();                            /* 0x0015CA50 pages.loadFile("menu/setting.mbl") */
    void unload();                          /* 0x0015CA78 */
    /* attach(), a frame, render.attach() with transition 0, pages.initialize
     * (&render.root, NULL); the tables, with this as their context: T083's
     * page actions g_002ACCC8 (pages.executeFuncs +0xB18), T096's checks
     * g_0029AEA0 (validFuncs +0xB24), T081's hooks g_002A9080 (setHooks)
     * and T082's sliders g_002ABAB0 (setSliderFuncs) to both layouts, T084's
     * g_002AD058 (selectableFuncs +0xB00); the counts are the tables'
     * (g_00351950 .. g_00351994 of T081 .. T084, T096's g_003512DC); g_00352750 /
     * g_0035274C (T246) cleared. */
    void begin();                           /* 0x0015CA98 */
    void end();                             /* 0x0015CBE8 */
    /* car = the GameStatus's setting car (if any); the page "root" (and
     * "load_warning" over it if loadWarning), until the pages are left. */
    void mainLoop(int loadWarning);         /* 0x0015CC28 */
    int  run(int loadWarning);              /* 0x0015CD08 load, begin, mainLoop, end, unload;
                                               changed */

    RenderBase  render;                     /* 0x0028C */
    PageManager pages;                      /* 0x002E4 */
    GarageCar   car;                        /* 0x00E40 the copy of the GameStatus's setting car */
    Slider     *gearSliders[8];             /* 0x01070 "slider_gear_ratio_1".."_7", "_final" (T081
                                               gear_setup) */
    Button     *gearAutoLevel;              /* 0x01090 "slider_gear_auto_level" (gear_setup) */
    int         unk1094;                    /* 0x01094 cleared by gear_setup */
    u8          unk_1098[0x20];             /* 0x01098 */
    int         unk10B8;                    /* 0x010B8 cleared by gear_setup */
    int         request;                    /* 0x010BC set by the pages: REQUEST_* */
    int         changed;                    /* 0x010C0 the setting car was changed: run()'s result */
    /*  (alignment padding)                    0x010C4 */
};
EGCS_SIZE(gt2k_menu::SettingControl, 0x10C8);

} /* namespace gt2k_menu */

/* setting_menu, the car setting menu: in its own header (the race modes'
 * T052 did not see SettingControl: u016's data pass). */
#include "game/menu/car_settings/T080_setting_entry.h"

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu14SettingControl(void); /* 0x0015D210 */
#endif

/* The rest of T036_garage.h (included here before u013's data pass): the
 * includers see the same declarations as before. */
#include "game/menu/gtmode/T036_garage.h"

#endif /* GAME_T080_SETTING_CONTROL_H */
