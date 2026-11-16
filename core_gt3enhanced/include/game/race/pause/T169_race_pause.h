/* T169_race_pause.h - RacePause, the pause menu of a race, and its window
 * PauseMenu (translation unit T169, unit u057), 0x001A2678..0x001A2EC8.
 *
 * PauseMenu is the page "pause_race" of the menu file "pause1"
 * (RaceMenuPage::load(file, page)): continue (id 3) / exit,
 * and after exit the message exit_message with ok (id 4) / cancel.  The
 * buttons with id 1 (exit, cancel) switch the page.  RacePause::virtual_2
 * returns a decided button's id, 3 when the main page is cancelled, 1 while
 * the window takes input (anyInput), 2 on the timeout, else 0.
 *
 * RaceMenuWindow (the plan's) and RaceMenuSelectWindow are defined in
 * T199_race_menu_layer.h, between RaceMenuLayer and its own classes: the
 * chain RaceMenuLayer (T199) <- RaceMenuWindow <- RaceMenuSelectWindow
 * (T199) <- PauseMenu (here) would make the two headers include each other.
 * Both have only inline constructors and destructors; T169 emits weak
 * copies of RaceMenuWindow's destructor (0x001A2E50) and of both typeinfo
 * functions, which the vtables of T170 / T171 / T200 use (T169 links
 * first).  Their typeinfo functions and nodes are declared here.
 */
#ifndef GAME_T169_RACE_PAUSE_H
#define GAME_T169_RACE_PAUSE_H

#include "game/game.h"
#include "game/race/pause/T168_pause_base.h"           /* base PauseBase */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* base RaceMenuSelectWindow (and RaceMenuWindow) */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* RaceMenuPage (held by value) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuImageBlinkButton (held by value) */

class RaceInput;                            /* T413 (pointers only: forward declarations) */
class RaceOrganization;                     /* T125 */
class GSBuffer;                             /* T514 */
class VariableCameraBase;                   /* T397 */
class RaceMenuButton;                       /* T198 */
namespace gt2k_menu { class Item; }         /* T213 */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
/* the __si_type_info at 0x004B2360 (base __ti9PauseBase) */
const type_info &__tf9RacePause(void);              /* 0x001A2D10 */
/* the __si_type_info at 0x004B12B0 (base __ti20RaceMenuSelectWindow) */
const type_info &__tf9PauseMenu(void);              /* 0x001A2D60 */
/* RaceMenuSelectWindow's (T199): the __si_type_info at 0x004B1210 (base
 * __ti14RaceMenuWindow).  T170 / T171 have unreferenced copies. */
const type_info &__tf20RaceMenuSelectWindow(void);  /* 0x001A2E00 */
/* RaceMenuWindow's (T199): the __si_type_info at 0x004B12D0 (base
 * __ti13RaceMenuLayer).  T170 / T171 / T200 have copies (T200's
 * 0x001B2778). */
const type_info &__tf14RaceMenuWindow(void);        /* 0x001A2E78 */
}
#endif

/* vtable 0x00330130 (1 slot), typeinfo __tf9PauseMenu 0x001A2D60 (si,
 * base RaceMenuSelectWindow).  vptr +0x2C (RaceMenuLayer's).  Size 0x29C
 * (RacePause +0xC .. +0x2A8). */
class PauseMenu : public RaceMenuSelectWindow {
public:
    /* The bases and members inline; the page, its "defaultLayer"
     * (RaceMenuWindow::load), the buttons m_continue (3), m_exit (1), m_ok
     * (4), m_cancel (1), the item exit_message; then showMain. */
    PauseMenu();                            /* 0x001A2678 */
    /* No destructor declared: egcs synthesises the virtual one ([0] +0x0C
     * 0x001A2DB0: the page (flags 2), ~RaceMenuWindow inline,
     * ~RaceMenuLayer).  A synthesised destructor has an empty body, so it
     * does not store PauseMenu's vtable first (retail), a written `{}' one
     * does; it is also inline in ~RacePause (retail). */
    /* continue / exit shown, the others hidden; items = them, cursor 0. */
    void showMain();                        /* 0x001A2880 */
    /* ok / cancel and exit_message shown; cursor on cancel. */
    void showConfirm();                     /* 0x001A2960 */
    void toggle();                          /* 0x001A2A40 showMain or showConfirm */
    int  isConfirm() { return confirm; }    /* RacePause::virtual_2 */

    int                      confirm;       /* 0x050 the confirmation page is shown */
    RaceMenuPage             page;          /* 0x054 page "pause_race" of menu/<country>/pause1.pmb */
    RaceMenuImageBlinkButton buttons[4];    /* 0x210 continue (3), exit (1), ok (4), cancel (1) */
    RaceMenuButton          *list[2];       /* 0x290 the items of the page shown */
    gt2k_menu::Item         *exitMessage;   /* 0x298 RaceMenuLayer::newItem "exit_message" (u057 RaceMenuImage *) */
};
EGCS_SIZE(PauseMenu, 0x29C);

/* vtable 0x00330108 (4 slots), typeinfo __tf9RacePause 0x001A2D10 (si,
 * base PauseBase).  vptr +0x8 (PauseBase's).  Size 0x2AC (held by value by
 * the race modes: RaceArcade +0x259E8, RaceFreePractice, RaceSplitBattle,
 * RaceLinkBattle, RaceFreeRun ...). */
class RacePause : public PauseBase {
public:
    /* PauseBase(), the menu, then RacePause::virtual_1 (direct). */
    RacePause();                            /* 0x001A2A78 */
    virtual ~RacePause();                   /* [0] +0x0C 0x001A2B28 (index: virtual_0) */
    /* Opened: the main page, the timer reset, shown = 0. */
    virtual void virtual_1();               /* [1] +0x14 0x001A2BA0 */
    /* The first call shows the main page; then the window's input
     * (RaceMenuSelectWindow::input) and the result (see above).  Leaving
     * clears shown; any input restarts the timer. */
    virtual int  virtual_2(RaceInput *in, RaceOrganization *org);  /* [2] +0x1C 0x001A2BD8 */
    /* menu.draw(buf, 1) (RaceMenuLayer). */
    virtual void virtual_3(GSBuffer *buf, RaceOrganization *org,
                           VariableCameraBase *camera);            /* [3] +0x24 0x001A2CF0 */

    PauseMenu menu;                         /* 0x00C */
    int       shown;                        /* 0x2A8 the main page was shown since it opened */
};
EGCS_SIZE(RacePause, 0x2AC);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
extern u32 __ti9RacePause[3];               /* 0x004B2360 __si_type_info (u057 g_004B2360) */
extern u32 __ti9PauseMenu[3];               /* 0x004B12B0 __si_type_info (u057 g_004B12B0) */
/* The nodes of T199's window classes, filled by the __tf functions above
 * (u057 / u058 / u062: g_004B1210 / g_004B12D0, u32[4]); the plan put them
 * in T167. */
extern u32 __ti20RaceMenuSelectWindow[3];   /* 0x004B1210 __si_type_info */
extern u32 __ti14RaceMenuWindow[3];         /* 0x004B12D0 __si_type_info */
/* (__ti13RaceMenuLayer, the base node of __tf14RaceMenuWindow, is declared
 * with T199's __tf13RaceMenuLayer in T199_race_menu_layer.h.) */
}
#endif

#endif /* GAME_T169_RACE_PAUSE_H */
