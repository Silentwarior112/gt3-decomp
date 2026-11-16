/* T169_race_pause.cpp - RacePause, the pause menu of a race
 * (0x001A2678..0x001A2EC8): its window PauseMenu (the page "pause_race" of
 * the menu file "pause1": continue / exit, then the exit message with ok /
 * cancel) and the typeinfo functions of PauseMenu and of T199's
 * RaceMenuSelectWindow / RaceMenuWindow, whose inline destructor is emitted
 * here too (T169's copies won the link). */
/* Its own include set, not race_pause_internal.h: retail's .rodata 0x003300A8..0x003302A6
 * is "bad_alloc" (<new>'s inline what()), the strings, the vtables, then
 * the type names 9RacePause, 9PauseMenu, 20RaceMenuSelectWindow,
 * 14RaceMenuWindow (the __tf functions load them), RaceMenuButton, the
 * operator list classes, UnitArena<64> and Operator (egcs writes them in
 * reverse class-definition order, a non-polymorphic base last): T153 with
 * GAME_T153_EMITS_TYPEINFO first (UnitArena<64>, no use), then the menu
 * headers.  Through race_pause_internal.h the template names List / Cell<OperatorHolder>
 * came first. */
#include "types.h"
#include <new>                              /* retail's .rodata starts with its "bad_alloc" */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* UnitArena<64>'s type name (no use) */
#include "runtime/cxx/T548_tinfo.h"                /* type_info, __rtti_si (the GCC 14 __tf functions) */
#include "game/race/pause/T168_pause_base.h"           /* base PauseBase */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuButton (setVisible) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (a Transitive: showItem) */
#include "game/race/core/T413_race_input.h"           /* RaceInput (a Control: RaceMenuSelectWindow::input) */
#include "game/race/pause/T169_race_pause.h"           /* PauseMenu, RacePause; T199 / T197's menu classes */

// DATA_START 0x003300A8

/* RacePause's button ids: a decided button returns its id, but for
 * PAUSE_BUTTON_SWITCH (exit, cancel), which switches the page. */
#define PAUSE_BUTTON_SWITCH 1

// FUNC 0x001A2678 PauseMenu::PauseMenu
//   The bases and members are constructed inline: RaceMenuLayer (root 0, the
//   RootObject, visible 1), RaceMenuWindow (frame 0), RaceMenuSelectWindow
//   (sound 1, the rest 0); the RaceMenuPage (0x001B01B8, out of line); the 4
//   buttons (RaceMenuButton's fields, the RaceMenuImageBlinkButton vtable).
//   Then the page is loaded, its layer "defaultLayer" built (no frame), the
//   buttons m_continue (continue), m_exit (switch), m_ok (exit) and m_cancel
//   (switch) and the item exit_message made under the layer; the main page.
//   (The landing pads run ~RaceMenuPage and ~RaceMenuLayer.)
PauseMenu::PauseMenu()
{
    /* implicit: RootObject (in RaceMenuLayer's inline constructor), RaceMenuPage (member constructor), ~RaceMenuPage, ~RaceMenuLayer (exception cleanup) */
    gt2k_menu::Transitive *layer;

    page.load("pause1", "pause_race");
    load(&page.layout, "defaultLayer", NULL);
    layer = root;
    buttons[0].init(&page.layout, layer, "m_continue", PAUSE_RESULT_CONTINUE);
    buttons[1].init(&page.layout, layer, "m_exit", PAUSE_BUTTON_SWITCH);
    buttons[2].init(&page.layout, layer, "m_ok", PAUSE_RESULT_EXIT);
    buttons[3].init(&page.layout, layer, "m_cancel", PAUSE_BUTTON_SWITCH);
    exitMessage = newItem(&page.layout, "exit_message", 1);
    showMain();
    setVisible(1);
}

// FUNC 0x001A2880 PauseMenu::showMain
//   continue / exit shown, ok / cancel and the exit message hidden; the
//   items are continue, exit; the cursor on continue.  The buttons are
//   refreshed through their vtable (RaceMenuButton::setVisible, inline).
void PauseMenu::showMain()
{
    confirm = 0;
    buttons[0].setVisible(1);
    buttons[1].setVisible(1);
    buttons[2].setVisible(0);
    buttons[3].setVisible(0);
    showItem(exitMessage, 0);
    list[0] = &buttons[0];
    list[1] = &buttons[1];
    items = list;
    count = 2;
    cursor = 0;
    setVisible(1);
}

// FUNC 0x001A2960 PauseMenu::showConfirm
//   The exit message with ok / cancel; the cursor on cancel.  (The layer's
//   setVisible(1) last: retail stores visible before cursor, both from the
//   register holding 1, which sched1 orders so only when the visible store
//   is the last use.)
void PauseMenu::showConfirm()
{
    confirm = 1;
    buttons[0].setVisible(0);
    buttons[1].setVisible(0);
    buttons[2].setVisible(1);
    buttons[3].setVisible(1);
    showItem(exitMessage, 1);
    list[0] = &buttons[2];
    list[1] = &buttons[3];
    items = list;
    count = 2;
    cursor = 1;
    setVisible(1);
}

// FUNC 0x001A2A40 PauseMenu::toggle
void PauseMenu::toggle()
{
    if (confirm != 0)
        showMain();
    else
        showConfirm();
}

// FUNC 0x001A2A78 RacePause::RacePause
//   PauseBase(), the menu (PauseMenu::PauseMenu), then virtual_1 (a direct
//   call).  `shown' is left to virtual_1.  Callers: the race modes'
//   constructors (RaceArcade, RaceArcadeDemo, RaceFreePractice,
//   RaceSplitBattle ...).  (The landing pads: ~RaceMenuPage of menu.page,
//   ~RaceMenuLayer of the menu with the RaceMenuWindow vtable, ~PauseBase.)
RacePause::RacePause()
{
    /* implicit: PauseBase (base constructor), PauseMenu (member constructor), ~RaceMenuPage, ~RaceMenuLayer (exception cleanup, inline ~PauseMenu), ~PauseBase (exception cleanup) */
    RacePause::virtual_1();
}

// FUNC 0x001A2B28 RacePause::~RacePause
//   (index: RacePause::virtual_0)  The menu's (synthesised) destructor inline: ~RaceMenuPage
//   of its page (egcs flags 2), then the RaceMenuWindow vtable and
//   ~RaceMenuLayer (flags 2); then ~PauseBase with the caller's flags.
RacePause::~RacePause()
{
    /* implicit: ~RaceMenuPage, ~RaceMenuLayer (inline ~PauseMenu of the menu), ~PauseBase (base destructor) */
}

// FUNC 0x001A2BA0 RacePause::virtual_1
//   Opened: the main page, the timer reset, not shown yet.
void RacePause::virtual_1()
{
    menu.showMain();
    resetTimer();
    shown = 0;
}

// FUNC 0x001A2BD8 RacePause::virtual_2
//   The update.  The first frame shows the main page again; then the
//   window's input (RaceMenuSelectWindow::input; RaceInput is a Control).
//   A decided button returns its id (continue 3, exit confirmed 4), but
//   exit / cancel (PAUSE_BUTTON_SWITCH) switch the page; cancelling the main
//   page continues, on the confirmation page it goes back; any other input
//   keeps the pause (1); else the timeout (2) or 0.  Leaving clears `shown';
//   every input restarts the timer.
int RacePause::virtual_2(RaceInput *in, RaceOrganization *org)
{
    if (shown == 0) {
        shown = 1;
        menu.showMain();
    }
    menu.input(in);
    if (menu.isDecided()) {
        int id;

        resetTimer();
        id = menu.current()->id;
        if (id != PAUSE_BUTTON_SWITCH) {
            shown = 0;
            return id;
        }
        menu.toggle();
    }
    if (menu.isCancelled()) {
        resetTimer();
        if (!menu.isConfirm()) {
            shown = 0;
            return PAUSE_RESULT_CONTINUE;
        }
        menu.showMain();
    }
    if (!menu.hasInput()) {
        if (tick() != 0) {
            shown = 0;
            return PAUSE_RESULT_TIMEOUT;
        }
        return PAUSE_RESULT_PAUSED;
    }
    resetTimer();
    return PAUSE_RESULT_INPUT;
}

// FUNC 0x001A2CF0 RacePause::virtual_3
//   The draw: the menu (RaceMenuLayer::draw, sorted by priority).  The
//   camera is not used.
void RacePause::virtual_3(GSBuffer *buf, RaceOrganization *org, VariableCameraBase *camera)
{
    menu.draw(buf, 1);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001A2D10 __tf9RacePause
//   The __si_type_info __ti9RacePause, base __ti9PauseBase (gp-0x4630).
extern "C" const type_info &__tf9RacePause(void)
{
    if (__ti9RacePause[0] == 0) {
        __tf9PauseBase();
        __rtti_si(__ti9RacePause, "9RacePause", (const type_info *)__ti9PauseBase);
    }
    return *(const type_info *)__ti9RacePause;
}

// FUNC 0x001A2D60 __tf9PauseMenu
//   The __si_type_info __ti9PauseMenu, base __ti20RaceMenuSelectWindow.
extern "C" const type_info &__tf9PauseMenu(void)
{
    if (__ti9PauseMenu[0] == 0) {
        __tf20RaceMenuSelectWindow();
        __rtti_si(__ti9PauseMenu, "9PauseMenu", (const type_info *)__ti20RaceMenuSelectWindow);
    }
    return *(const type_info *)__ti9PauseMenu;
}
#endif

// FUNC 0x001A2DB0 PauseMenu::~PauseMenu
/* implicit: synthesised by egcs (no destructor is declared, see
 * T169_race_pause.h): ~RaceMenuPage of the page (flags 2), then
 * ~RaceMenuWindow inline (its vtable) and ~RaceMenuLayer with the caller's
 * flags. */

#ifndef BOOT_OLD_GCC
// FUNC 0x001A2E00 __tf20RaceMenuSelectWindow
//   (index: no name)  RaceMenuSelectWindow's (T199; no vtable of its own
//   survives): the __si_type_info __ti20RaceMenuSelectWindow, base
//   __ti14RaceMenuWindow.  Callers: __tf9PauseMenu, __tf15ReplayPauseMenu,
//   __tf18PauseMenuWithRetry.
extern "C" const type_info &__tf20RaceMenuSelectWindow(void)
{
    if (__ti20RaceMenuSelectWindow[0] == 0) {
        __tf14RaceMenuWindow();
        __rtti_si(__ti20RaceMenuSelectWindow, "20RaceMenuSelectWindow",
                  (const type_info *)__ti14RaceMenuWindow);
    }
    return *(const type_info *)__ti20RaceMenuSelectWindow;
}
#endif

// FUNC 0x001A2E50 RaceMenuWindow::~RaceMenuWindow
/* inline in game/menu/race_menu/T199_race_menu_layer.h; egcs emits this copy (its vtable,
 * then ~RaceMenuLayer with the caller's flags; the vtables of T169 / T170 /
 * T171 / T200 all use it) */
/* implicit: ~RaceMenuLayer (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001A2E78 __tf14RaceMenuWindow
//   RaceMenuWindow's (T199): the __si_type_info __ti14RaceMenuWindow, base
//   __ti13RaceMenuLayer (gp-0x4610).  Callers: __tf20RaceMenuSelectWindow
//   and the copies of T170 / T171, __tf9ResultBar (T200).
extern "C" const type_info &__tf14RaceMenuWindow(void)
{
    if (__ti14RaceMenuWindow[0] == 0) {
        __tf13RaceMenuLayer();
        __rtti_si(__ti14RaceMenuWindow, "14RaceMenuWindow", (const type_info *)__ti13RaceMenuLayer);
    }
    return *(const type_info *)__ti14RaceMenuWindow;
}
#endif
