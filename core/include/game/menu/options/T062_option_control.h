/* T062_option_control.h - gt2k_menu::OptionControl, the options menu
 * ("menu/option.mbl", page "option") (translation unit T062, unit u021),
 * 0x0013F940..0x0013FF98.
 *
 * F_0013FE58 (main, the arcade and GT-mode menus) builds the controller on
 * the stack over a 1 MB menu heap, also on the stack.  The controller edits
 * a copy of the game's options (GameStatus +0x2FF38), copied in when the
 * page starts and back when it is left.  begin() hands the PageManager the
 * tables of T063..T067: the layout hooks g_002A0AD0 (T064), the sliders
 * g_002A1C38 (T065) and the multi buttons g_002A1CD8 (T066) to both layouts,
 * the actions g_002A2038 (T067, ExecuteFuncData) and the page checks
 * g_002A0A70 (T063, ValidFuncData), ctx this.  The items of the pages (T064
 * hooks, u014's KeyConfigItem / HandicapItem / CalibrationItem) reach it
 * through the accessors. */
#ifndef GAME_T062_OPTION_CONTROL_H
#define GAME_T062_OPTION_CONTROL_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pages) */
#include "game/save/T428_game_options.h"         /* GameOptions (options) */

namespace gt2k_menu {
class KeyConfigItem;                        /* T101 (pointers only) */
class MenuControl;                          /* T214 */

/* vtable 0x00319D38 (2 slots: [1] ControlBase::waitFrame).  typeinfo
 * __tfQ29gt2k_menu13OptionControl 0x0013FF48 (si, base ControlBase;
 * storage 0x004B1870).  Size 0x16E0 (F_0013FE58's frame: sp+0x100000 ..
 * sp+0x1016E0). */
class OptionControl : public ControlBase {
public:
    OptionControl();                        /* 0x0013F940 calibWheel 2; pageManager = &pages */
    virtual ~OptionControl();               /* [0] +0x0C 0x0013FA20 render's (~RootObject,
                                               GameObject::~GameObject), ~ControlBase */
    void load();                            /* 0x0013FA88 "menu/option.mbl" */
    void unload();                          /* 0x0013FAB0 */
    void begin();                           /* 0x0013FAD0 the render, a copy of the game's options,
                                               the T063..T067 tables */
    void end();                             /* 0x0013FC58 */
    /* KeyConfigItem's constructor / destructor (T101): the page of player
     * 1 (or the USB page) / of player 2 that mainLoop runs every frame,
     * counted. */
    void attachKeyConfig1(KeyConfigItem *item); /* 0x0013FC98 keyConfig1 = item, keyConfig1Count++ */
    void detachKeyConfig1();                /* 0x0013FCB0 --keyConfig1Count; at 0 keyConfig1 = NULL */
    void attachKeyConfig2(KeyConfigItem *item); /* 0x0013FCD0 */
    void detachKeyConfig2();                /* 0x0013FCE8 */
    /* Page "option" until it is left; every frame the PageManager, then
     * keyConfig2's handleInput, keyConfig1's handleInput and
     * checkExit(keyConfig2); then the options go back to the game. */
    void mainLoop();                        /* 0x0013FD08 */
    void run();                             /* 0x0013FE10 load, begin, mainLoop, end, unload */
    int  getCalibWheel();                   /* 0x0013FF10 */
    void setCalibWheel(int w);              /* 0x0013FF18 */
    int  getCalibPort();                    /* 0x0013FF20 */
    void setCalibPort(int p);               /* 0x0013FF28 */
    ::GameOptions *getOptions();            /* 0x0013FF30 &options */
    PageManager *getPages();                /* 0x0013FF38 &pages */
    MenuControl *getControl2();             /* 0x0013FF40 &control2 */

    /*             ControlBase                 0x000 .. 0x28C (vptr +0x288) */
    RenderBase     render;                  /* 0x28C */
    PageManager    pages;                   /* 0x2E4 */
    KeyConfigItem *keyConfig1;              /* 0xE40 player 1's (or the USB) KeyConfigItem */
    int            keyConfig1Count;         /* 0xE44 */
    KeyConfigItem *keyConfig2;              /* 0xE48 player 2's */
    int            keyConfig2Count;         /* 0xE4C */
    int            calibPort;               /* 0xE50 the port of the USB wheel pages: 0 / 1 */
                                            /* (T067 set_port_*) */
    int            calibWheel;              /* 0xE54 the wheel type of those pages: 2, 3 "jaguar" */
    ::GameOptions  options;                 /* 0xE58 the options being edited */
};
EGCS_SIZE(gt2k_menu::OptionControl, 0x16E0);

} /* namespace gt2k_menu */

/* The options menu; 1. */
int F_0013FE58(void);                       /* 0x0013FE58 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu13OptionControl(void);    /* 0x0013FF48 */
#endif

#endif /* GAME_T062_OPTION_CONTROL_H */
