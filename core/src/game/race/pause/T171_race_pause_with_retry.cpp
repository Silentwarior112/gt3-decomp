/* T171_race_pause_with_retry.cpp - RacePauseWithRetry, the pause menu with
 * a retry button (0x001A3580..0x001A3EA8): its PauseMenuWithRetry window
 * (page "pause_retry" of pause3.pmb: continue / exit / retry, then the exit
 * message with ok / cancel), and T171's copies of the inline
 * RaceMenuWindow destructor and of the RaceMenuSelectWindow /
 * RaceMenuWindow typeinfo functions.  Used by the time trial, the license
 * tests (LicensePause) and the machine test. */
/* Retail's .rodata (0x003304B8..0x003306C6): "bad_alloc" (<new>'s inline
 * what(): unreferenced, hence the anchor), the literals, the vtables, then
 * the type names in the reverse order of the classes' definitions:
 * RacePauseWithRetry, PauseMenuWithRetry, RaceMenuSelectWindow,
 * RaceMenuWindow, RaceMenuButton, the operator list classes, UnitArena<64>,
 * Operator (a non-polymorphic base: last).  No RaceCameraBase (no T129 /
 * T383 header). */
// DATA_START 0x003304B8
#include <new>                              /* retail's .rodata starts with its "bad_alloc" */
#include "game/race/pause/replay_pause_internal.h"
#include "runtime/cxx/T548_tinfo.h"                /* type_info, __rtti_si (the GCC 14 __tf functions) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (retail has it, defined before */
#include "game/race/core/T153_race_course.h"          /*   the operator list classes; no use) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::find, RACE_EVENT_LAP_RECORD */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "game/race/core/T413_race_input.h"           /* RaceInput (a Control: RaceMenuSelectWindow::input) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (a Transitive: showItem) */
/* RaceMenuButton before the window classes (T199, through T171's header):
 * retail's type names after the unit's own are RaceMenuSelectWindow,
 * RaceMenuWindow, then RaceMenuButton */
#include "game/menu/race_menu/T198_race_menu_button.h"
#include "game/race/pause/T171_race_pause_with_retry.h"
#ifndef BOOT_OLD_GCC
#include "game/race/pause/T169_race_pause.h"           /* __tf20RaceMenuSelectWindow / __tf14RaceMenuWindow, their __ti
                                               (egcs: T169's PauseMenu would add its type name) */
#endif

// FUNC 0x001A3580 PauseMenuWithRetry::PauseMenuWithRetry
//   The bases and members are constructed inline (as ReplayPauseMenu's;
//   the 5 buttons in a loop; `confirm' is not initialized here).  Loads the
//   page, builds "defaultLayer", the buttons m_continue, m_exit, m_retry,
//   m_ok, m_cancel and the item exit_message (newItem), then shows the main
//   page (showMain).  (EH: ~RaceMenuPage, ~RaceMenuLayer; RootObject() of
//   the layer's obj.)
PauseMenuWithRetry::PauseMenuWithRetry()
{
    /* implicit: RootObject (in RaceMenuLayer's inline constructor), RaceMenuPage (member constructor), ~RaceMenuPage, ~RaceMenuLayer (exception cleanup) */
    gt2k_menu::Transitive *layer;

    page.load("pause3", "pause_retry");
    RaceMenuWindow::load(&page.layout, "defaultLayer", NULL);
    layer = getRoot();
    buttons[0].init(&page.layout, layer, "m_continue", PAUSE_RESULT_CONTINUE);
    buttons[1].init(&page.layout, layer, "m_exit", PAUSE_RESULT_INPUT);
    buttons[2].init(&page.layout, layer, "m_retry", PAUSE_RESULT_RETRY);
    buttons[3].init(&page.layout, layer, "m_ok", PAUSE_RESULT_EXIT);
    buttons[4].init(&page.layout, layer, "m_cancel", PAUSE_RESULT_INPUT);
    exitMessage = newItem(&page.layout, "exit_message", 1);
    showMain();
    setVisible(1);
}

// FUNC 0x001A37A0 PauseMenuWithRetry::showMain
//   continue / exit / retry shown, ok / cancel and the exit message hidden
//   (showItem); the items are continue, exit, retry; the cursor on
//   continue.  (RaceMenuButton::setVisible, inline: retail stores through
//   each button's address and refreshes it through its vtable, slot 1.)
void PauseMenuWithRetry::showMain()
{
    confirm = 0;
    buttons[0].setVisible(1);
    buttons[1].setVisible(1);
    buttons[2].setVisible(1);
    buttons[3].setVisible(0);
    buttons[4].setVisible(0);
    showItem(exitMessage, 0);
    list[0] = &buttons[0];
    list[1] = &buttons[1];
    list[2] = &buttons[2];
    items = list;
    count = 3;
    cursor = 0;
}

// FUNC 0x001A38A8 PauseMenuWithRetry::showConfirm
//   The exit message (showItem) with ok / cancel; the cursor on cancel.
//   (count before items: retail's store order.)
void PauseMenuWithRetry::showConfirm()
{
    confirm = 1;
    buttons[0].setVisible(0);
    buttons[1].setVisible(0);
    buttons[2].setVisible(0);
    buttons[3].setVisible(1);
    buttons[4].setVisible(1);
    showItem(exitMessage, 1);
    cursor = 1;
    list[0] = &buttons[3];
    list[1] = &buttons[4];
    count = 2;
    items = list;
}

// FUNC 0x001A39A0 PauseMenuWithRetry::toggle
//   showMain on the confirmation page, else showConfirm.
void PauseMenuWithRetry::toggle()
{
    if (confirm != 0)
        showMain();
    else
        showConfirm();
}

// FUNC 0x001A39D8 PauseMenuWithRetry::setRetryEnabled
//   The retry button's `enabled', refreshed (virtual, slot 1).
void PauseMenuWithRetry::setRetryEnabled(int on)
{
    RaceMenuButton *retryButton = &buttons[2];

    retryButton->enabled = on;
    retryButton->refresh();
}

// FUNC 0x001A3A08 RacePauseWithRetry::RacePauseWithRetry
//   PauseBase(), the menu (PauseMenuWithRetry::PauseMenuWithRetry), then
//   virtual_1 (called directly).  (EH: ~RaceMenuPage, ~RaceMenuLayer,
//   ~PauseBase.)  Callers: the RaceTimeTrial, RaceLicense, RaceMachineTest
//   constructors.
RacePauseWithRetry::RacePauseWithRetry()
{
    /* implicit: PauseBase (base constructor), PauseMenuWithRetry (member constructor), ~RaceMenuPage, ~RaceMenuLayer (exception cleanup, inline ~PauseMenuWithRetry), ~PauseBase (exception cleanup) */
    RacePauseWithRetry::virtual_1();
}

// FUNC 0x001A3AB8 RacePauseWithRetry::~RacePauseWithRetry
//   (index: RacePauseWithRetry::virtual_0)  The menu's destructor inline
//   (the page, ~RaceMenuPage with egcs flags 2, the RaceMenuWindow vtable,
//   RaceMenuLayer::~RaceMenuLayer with flags 2), then ~PauseBase with the
//   caller's flags.  Also called by LicensePause::~LicensePause.
RacePauseWithRetry::~RacePauseWithRetry()
{
    /* implicit: ~RaceMenuPage, ~RaceMenuLayer (inline ~PauseMenuWithRetry of the menu), ~PauseBase (base destructor) */
}

// FUNC 0x001A3B30 RacePauseWithRetry::virtual_1
//   Opened: retry enabled, the main page, the timer reset (resetTimer), not
//   shown yet.
void RacePauseWithRetry::virtual_1()
{
    menu.setRetryEnabled(1);
    menu.showMain();
    resetTimer();
    shown = 0;
}

// FUNC 0x001A3B80 RacePauseWithRetry::virtual_2
//   The update.  The first frame shows the main page; a lap record event of
//   the organization (taken: RaceEventQueue::find) disables retry.  Then the
//   window's input: a decided button returns its id, except exit / cancel
//   (PAUSE_RESULT_INPUT), which switch the page (toggle); cancelling the main page
//   continues, on the confirmation page it goes back (showMain); anyInput
//   keeps the pause; else the timeout (tick) or PAUSE_RESULT_PAUSED.  Leaving (any
//   result but PAUSE_RESULT_PAUSED / PAUSE_RESULT_INPUT) clears `shown'; the timer restarts
//   (resetTimer) on any input.
int RacePauseWithRetry::virtual_2(RaceInput *in, RaceOrganization *org)
{
    PauseMenuWithRetry *m;

    if (shown == 0) {
        shown = 1;
        menu.showMain();
    }
    /* retail: &menu in $s0 from here (computed on both paths) */
    m = &menu;
    if (org != NULL && org->eventQueue.find(RACE_EVENT_LAP_RECORD, NULL, 1) != 0)
        m->setRetryEnabled(0);
    m->input(in);
    if (m->decided != 0) {
        int id;

        resetTimer();
        id = m->items[m->cursor]->id;
        if (id != PAUSE_RESULT_INPUT) {
            shown = 0;
            return id;
        }
        m->toggle();
    }
    if (m->cancelled != 0) {
        resetTimer();
        if (m->confirm == 0) {
            shown = 0;
            return PAUSE_RESULT_CONTINUE;
        }
        m->showMain();
    }
    /* retail: the timeout test falls through, the input case is the tail */
    if (m->anyInput == 0) {
        if (tick() != 0) {
            shown = 0;
            return PAUSE_RESULT_TIMEOUT;
        }
        return PAUSE_RESULT_PAUSED;
    }
    resetTimer();
    return PAUSE_RESULT_INPUT;
}

// FUNC 0x001A3CD0 RacePauseWithRetry::virtual_3
//   The draw: the menu (RaceMenuLayer::draw(buf, 1)).
void RacePauseWithRetry::virtual_3(GSBuffer *buf, RaceOrganization *org, VariableCameraBase *camera)
{
    menu.draw(buf, 1);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001A3CF0 __tf18RacePauseWithRetry
//   The __si_type_info __ti18RacePauseWithRetry, base __ti9PauseBase
//   (gp-0x4630).  Caller: __tf12LicensePause.
extern "C" const type_info &__tf18RacePauseWithRetry(void)
{
    if (__ti18RacePauseWithRetry[0] == 0) {
        __tf9PauseBase();
        __rtti_si(__ti18RacePauseWithRetry, "18RacePauseWithRetry",
                  (const type_info *)__ti9PauseBase);
    }
    return *(const type_info *)__ti18RacePauseWithRetry;
}

// FUNC 0x001A3D40 __tf18PauseMenuWithRetry
//   The __si_type_info __ti18PauseMenuWithRetry, base
//   __ti20RaceMenuSelectWindow (T169's __tf20RaceMenuSelectWindow,
//   0x001A2E00).
extern "C" const type_info &__tf18PauseMenuWithRetry(void)
{
    if (__ti18PauseMenuWithRetry[0] == 0) {
        __tf20RaceMenuSelectWindow();
        __rtti_si(__ti18PauseMenuWithRetry, "18PauseMenuWithRetry",
                  (const type_info *)__ti20RaceMenuSelectWindow);
    }
    return *(const type_info *)__ti18PauseMenuWithRetry;
}
#endif

// FUNC 0x001A3D90 PauseMenuWithRetry::~PauseMenuWithRetry (_$_18PauseMenuWithRetry)
/* implicit (synthesized by egcs, see T171_race_pause_with_retry.h): the
 * page (~RaceMenuPage, egcs flags 2), then ~RaceMenuWindow inline (its
 * vtable 0x00330140) and RaceMenuLayer::~RaceMenuLayer with the caller's
 * flags; emitted with the vtable at the end of the unit */
/* implicit: ~RaceMenuPage (member destructor), ~RaceMenuLayer (base destructor, inline ~RaceMenuWindow) */

/* T171's copies of inline functions of T199's window classes (egcs emits
 * them in every TU that uses the classes), never referenced: the vtables
 * and callers use T169's, which link first. */

#ifndef BOOT_OLD_GCC
// FUNC 0x001A3DE0 __tf20RaceMenuSelectWindow
//   T171's copy of __tf20RaceMenuSelectWindow (0x001A2E00, T169's): the
//   __si_type_info __ti20RaceMenuSelectWindow, base __ti14RaceMenuWindow (it
//   calls T169's __tf14RaceMenuWindow 0x001A2E78); name string 0x00330590
//   (T171's copy).  A static stand-in here: the extern "C" name is T169's.
static const type_info &T171_tf20RaceMenuSelectWindow(void)
{
    if (__ti20RaceMenuSelectWindow[0] == 0) {
        __tf14RaceMenuWindow();
        __rtti_si(__ti20RaceMenuSelectWindow, "20RaceMenuSelectWindow",
                  (const type_info *)__ti14RaceMenuWindow);
    }
    return *(const type_info *)__ti20RaceMenuSelectWindow;
}
#endif

// FUNC 0x001A3E30 RaceMenuWindow::~RaceMenuWindow
/* inline in game/menu/race_menu/T199_race_menu_layer.h; egcs emits this copy (T171's:
 * this+0x2C = _vt$14RaceMenuWindow 0x00330140, then RaceMenuLayer::
 * ~RaceMenuLayer with the caller's egcs flags; T169's 0x001A2E50 is the
 * one used) */
/* implicit: ~RaceMenuLayer (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001A3E58 __tf14RaceMenuWindow
//   T171's copy of __tf14RaceMenuWindow (0x001A2E78, T169's): the
//   __si_type_info __ti14RaceMenuWindow, base __ti13RaceMenuLayer
//   (gp-0x4610; __tf13RaceMenuLayer); name string 0x003305A8 (T171's
//   copy).  A static stand-in, as above.
static const type_info &T171_tf14RaceMenuWindow(void)
{
    if (__ti14RaceMenuWindow[0] == 0) {
        __tf13RaceMenuLayer();
        __rtti_si(__ti14RaceMenuWindow, "14RaceMenuWindow", (const type_info *)__ti13RaceMenuLayer);
    }
    return *(const type_info *)__ti14RaceMenuWindow;
}
#endif
