/* T170_replay_pause.cpp - ReplayPause, the pause menu of a replay
 * (0x001A2EC8..0x001A3580): its ReplayPauseMenu window (page "pause_replay"
 * of pause2.pmb: continue / exit / replay), the replay option window it
 * opens, and T170's copies of the inline RaceMenuWindow destructor and of
 * the RaceMenuSelectWindow / RaceMenuWindow typeinfo functions. */
/* Retail's .rodata (0x003302A8..0x003304B6): "bad_alloc" (<new>'s inline
 * what(), written when the header is read: unreferenced, hence the anchor),
 * the literals, the vtables, then the type names in the reverse order of
 * the classes' definitions: ReplayPause, ReplayPauseMenu,
 * RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton, the operator list
 * classes, RaceCameraBase, UnitArena<64>, Operator (a non-polymorphic base:
 * last). */
// DATA_START 0x003302A8
#include <new>                              /* retail's .rodata starts with its "bad_alloc" */
#include "game/race/pause/replay_pause_internal.h"
#include "runtime/cxx/T548_tinfo.h"                /* type_info, __rtti_si (the GCC 14 __tf functions) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (race) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (retail has it, defined before
                                               RaceCameraBase: T129's header reads T153's first) */
#include "game/race/core/T129_race_base.h"            /* RaceBase::virtual_6 (the camera manager) */
#include "game/race/core/T413_race_input.h"           /* RaceInput (a Control: RaceMenuSelectWindow::input) */
/* RaceMenuButton before the window classes (T199, through T170's header):
 * retail's type names after the unit's own are RaceMenuSelectWindow,
 * RaceMenuWindow, then RaceMenuButton */
#include "game/menu/race_menu/T198_race_menu_button.h"
#include "game/race/pause/T170_replay_pause.h"
#ifndef BOOT_OLD_GCC
#include "game/race/pause/T169_race_pause.h"           /* __tf20RaceMenuSelectWindow / __tf14RaceMenuWindow, their __ti
                                               (egcs: T169's PauseMenu would add its type name) */
#endif

/* ReplayOption's (T196) isOpen / setRace, inline in the original: retail
 * keeps &option (this + 0xC) in a register and accesses active / org /
 * camera through it, as an integrated inline member does (T196's
 * out-of-line copies 0x001AFA08 / 0x001AF9F8 have no callers).  Stand-ins
 * until T196's header makes them inline (ref/matching/u061/pending.md). */
static inline int replay_option_is_open(ReplayOption *o)
{
    return o->active;
}

static inline void replay_option_set_race(ReplayOption *o, RaceOrganization *org, CameraManager *cm)
{
    o->org = org;
    o->camera = cm;
}

// FUNC 0x001A2EC8 ReplayPauseMenu::ReplayPauseMenu
//   The bases and members are constructed inline (RaceMenuLayer: root 0,
//   the RootObject, visible 1; RaceMenuWindow, RaceMenuSelectWindow: their
//   fields; the page; the 3 buttons, a loop).  Only RaceMenuLayer's vtable
//   is stored before ReplayPauseMenu's (0x00330320).  Loads the page,
//   builds "defaultLayer" and the buttons m_continue, m_exit, m_replay; the
//   items are the three of them.  (EH: ~RaceMenuPage, ~RaceMenuLayer.)
ReplayPauseMenu::ReplayPauseMenu()
{
    /* implicit: RootObject (in RaceMenuLayer's inline constructor), RaceMenuPage (member constructor), ~RaceMenuPage, ~RaceMenuLayer (exception cleanup) */
    gt2k_menu::Transitive *layer;

    page.load("pause2", "pause_replay");
    RaceMenuWindow::load(&page.layout, "defaultLayer", NULL);
    layer = getRoot();
    buttons[0].init(&page.layout, layer, "m_continue", PAUSE_RESULT_CONTINUE);
    buttons[1].init(&page.layout, layer, "m_exit", PAUSE_RESULT_EXIT);
    buttons[2].init(&page.layout, layer, "m_replay", PAUSE_RESULT_INPUT);
    list[0] = &buttons[0];
    list[1] = &buttons[1];
    list[2] = &buttons[2];
    items = list;
    count = 3;
    setVisible(1);
}

// FUNC 0x001A30C0 ReplayPause::ReplayPause
//   PauseBase(), the option window (ReplayOption(mode)), the menu
//   (ReplayPauseMenu::ReplayPauseMenu), then virtual_1 (called directly).
//   (EH: ~RaceMenuPage, ~RaceMenuLayer, ~ReplayOption, ~PauseBase.)
//   Callers: the race modes' constructors (RaceArcade, RaceFreePractice,
//   RaceSplitBattle, RaceTimeTrial, ...).
ReplayPause::ReplayPause(int mode)
    : option(mode)
{
    /* implicit: PauseBase (base constructor), ReplayOption, ReplayPauseMenu (member constructors), ~RaceMenuPage, ~RaceMenuLayer (exception cleanup, inline ~ReplayPauseMenu), ~ReplayOption, ~PauseBase (exception cleanup) */
    ReplayPause::virtual_1();
}

// FUNC 0x001A31A8 ReplayPause::~ReplayPause
//   (index: ReplayPause::virtual_0)  The menu's destructor inline (the page,
//   ~RaceMenuPage with egcs flags 2, the RaceMenuWindow vtable,
//   RaceMenuLayer::~RaceMenuLayer with flags 2), ~ReplayOption (flags 2),
//   then ~PauseBase with the caller's flags.
ReplayPause::~ReplayPause()
{
    /* implicit: ~RaceMenuPage, ~RaceMenuLayer (inline ~ReplayPauseMenu of the menu), ~ReplayOption (member destructor), ~PauseBase (base destructor) */
}

// FUNC 0x001A3228 ReplayPause::virtual_1
//   Opened: the cursor on continue, the timer reset (resetTimer).
void ReplayPause::virtual_1()
{
    menu.cursor = 0;
    resetTimer();
}

// FUNC 0x001A3248 ReplayPause::F_001A3248
//   Gives the option window the organization and its race's camera
//   manager (RaceBase::virtual_6).  Callers: the race modes' virtual_8 (the
//   start of a race / replay).
void ReplayPause::F_001A3248(RaceOrganization *org)
{
    CameraManager *cameras = org->race->virtual_6();

    replay_option_set_race(&option, org, cameras);
}

// FUNC 0x001A3298 ReplayPause::virtual_2
//   The update.  While the option window is open it gets the input and the
//   pause stays.  Else the menu (input): a decided button returns its id (on
//   "replay" the option window is opened first: its id keeps the pause),
//   cancel continues, the window's anyInput keeps the pause, else the
//   timeout (tick) or PAUSE_RESULT_PAUSED.  The timer restarts (resetTimer) on any
//   input.
int ReplayPause::virtual_2(RaceInput *in, RaceOrganization *org)
{
    ReplayPauseMenu *m;

    if (replay_option_is_open(&option)) {
        option.update(in);
        return PAUSE_RESULT_INPUT;
    }
    /* retail: &menu in $s0, computed in the delay slot of the open test */
    m = &menu;
    m->input(in);
    if (m->decided != 0) {
        resetTimer();
        if (m->cursor == REPLAY_PAUSE_ITEM_REPLAY)
            option.open();
        return m->items[m->cursor]->id;
    }
    if (m->cancelled != 0) {
        resetTimer();
        return PAUSE_RESULT_CONTINUE;
    }
    /* retail: the timeout test falls through, the input case is the tail */
    if (m->anyInput == 0)
        return (tick() != 0) ? PAUSE_RESULT_TIMEOUT : PAUSE_RESULT_PAUSED;
    resetTimer();
    return PAUSE_RESULT_INPUT;
}

// FUNC 0x001A3380 ReplayPause::virtual_3
//   The draw: the menu (RaceMenuLayer::draw(buf, 1)) or, while it is open,
//   the option window (ReplayOption::draw).
void ReplayPause::virtual_3(GSBuffer *buf, RaceOrganization *org, VariableCameraBase *camera)
{
    if (!replay_option_is_open(&option))
        menu.draw(buf, 1);
    else
        option.draw(buf);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001A33C8 __tf11ReplayPause
//   The __si_type_info __ti11ReplayPause, base __ti9PauseBase (gp-0x4630).
extern "C" const type_info &__tf11ReplayPause(void)
{
    if (__ti11ReplayPause[0] == 0) {
        __tf9PauseBase();
        __rtti_si(__ti11ReplayPause, "11ReplayPause", (const type_info *)__ti9PauseBase);
    }
    return *(const type_info *)__ti11ReplayPause;
}

// FUNC 0x001A3418 __tf15ReplayPauseMenu
//   The __si_type_info __ti15ReplayPauseMenu, base __ti20RaceMenuSelectWindow
//   (T169's __tf20RaceMenuSelectWindow, 0x001A2E00).
extern "C" const type_info &__tf15ReplayPauseMenu(void)
{
    if (__ti15ReplayPauseMenu[0] == 0) {
        __tf20RaceMenuSelectWindow();
        __rtti_si(__ti15ReplayPauseMenu, "15ReplayPauseMenu",
                  (const type_info *)__ti20RaceMenuSelectWindow);
    }
    return *(const type_info *)__ti15ReplayPauseMenu;
}
#endif

// FUNC 0x001A3468 ReplayPauseMenu::~ReplayPauseMenu (_$_15ReplayPauseMenu)
/* implicit (synthesized by egcs, see T170_replay_pause.h): the page
 * (~RaceMenuPage, egcs flags 2), then ~RaceMenuWindow inline (its vtable
 * 0x00330140) and RaceMenuLayer::~RaceMenuLayer with the caller's flags;
 * emitted with the vtable at the end of the unit */
/* implicit: ~RaceMenuPage (member destructor), ~RaceMenuLayer (base destructor, inline ~RaceMenuWindow) */

/* T170's copies of inline functions of T199's window classes (egcs emits
 * them in every TU that uses the classes), never referenced: the vtables
 * and callers use T169's, which link first. */

#ifndef BOOT_OLD_GCC
// FUNC 0x001A34B8 __tf20RaceMenuSelectWindow
//   T170's copy of __tf20RaceMenuSelectWindow (0x001A2E00, T169's): the
//   __si_type_info __ti20RaceMenuSelectWindow, base __ti14RaceMenuWindow (it
//   calls T169's __tf14RaceMenuWindow 0x001A2E78); its name string is T170's
//   copy (0x00330368).  A static stand-in here: the extern "C" name is
//   T169's.
static const type_info &T170_tf20RaceMenuSelectWindow(void)
{
    if (__ti20RaceMenuSelectWindow[0] == 0) {
        __tf14RaceMenuWindow();
        __rtti_si(__ti20RaceMenuSelectWindow, "20RaceMenuSelectWindow",
                  (const type_info *)__ti14RaceMenuWindow);
    }
    return *(const type_info *)__ti20RaceMenuSelectWindow;
}
#endif

// FUNC 0x001A3508 RaceMenuWindow::~RaceMenuWindow
/* inline in game/menu/race_menu/T199_race_menu_layer.h; egcs emits this copy (T170's:
 * this+0x2C = _vt$14RaceMenuWindow 0x00330140, then RaceMenuLayer::
 * ~RaceMenuLayer with the caller's egcs flags; T169's 0x001A2E50 is the
 * one used) */
/* implicit: ~RaceMenuLayer (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001A3530 __tf14RaceMenuWindow
//   T170's copy of __tf14RaceMenuWindow (0x001A2E78, T169's): the
//   __si_type_info __ti14RaceMenuWindow, base __ti13RaceMenuLayer
//   (gp-0x4610; __tf13RaceMenuLayer); name string 0x00330380 (T170's
//   copy).  A static stand-in, as above.
static const type_info &T170_tf14RaceMenuWindow(void)
{
    if (__ti14RaceMenuWindow[0] == 0) {
        __tf13RaceMenuLayer();
        __rtti_si(__ti14RaceMenuWindow, "14RaceMenuWindow", (const type_info *)__ti13RaceMenuLayer);
    }
    return *(const type_info *)__ti14RaceMenuWindow;
}
#endif
