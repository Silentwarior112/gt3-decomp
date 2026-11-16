/* T111 - gt2k_menu::ShortmenuControl, the controller of the short menu.
 * 0x0012C300..0x0012C978.
 *
 * A stack object of run_short_menu (T102): loadPages gives the PageManager
 * the availability checks of page "root" (its SelectableFuncData), begin
 * the qm_* actions (ExecuteFuncData) and both layouts the qm_* hooks, run
 * opens a page and mainLoop runs it until the page manager has exited; the
 * race behind the menu is drawn and kept running by the ShortmenuRender at
 * +0x290.
 *
 * Includes (data pass): retail's .rodata (0x003147C8..0x00314C08) is
 * "bad_alloc", the check strings, g_00314848, the vtable, then the type
 * names ShortmenuControl, SmartIndexT<int>, ResultTermBar, ResultTitleBar,
 * ReplayPauseMenu, PauseMenu, RaceMenuSelectWindow, RaceMenuWindow,
 * RaceMenuButton, the OperatorHolder group, RaceMiniMap .. RaceDisplayBase,
 * RaceCameraBase, UnitArena<64>, the garage lists, Operator (reverse
 * definition order: no RecordsMenuArg, RegurationResult or ShortMenuText).
 * Its .sdata opens with T157's "" (RaceMessageDisplay::clear, through the
 * race displays).  "bad_alloc" and the "" are unreferenced: the anchors. */
// DATA_START 0x003147C8
// DATA_START 0x00351420
#include <new>                              /* "bad_alloc" (retail's first .rodata item) */
#include "game/save/T429_game_status.h"          /* (seen: the garage lists) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/race/display/T154_race_display.h"         /* (seen, not used: the race displays) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen, not used: the OperatorHolder group) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (seen, not used) */
#include "game/race/pause/T169_race_pause.h"           /* (seen, not used) */
#include "game/race/pause/T170_replay_pause.h"         /* (seen, not used) */
#include "game/menu/records/T200_result_board.h"         /* (seen, not used) */
#include "game/menu/race_menu/T111_shortmenu_control.h"
#include "game/menu/race_menu/T113_shortmenu_tables.h"     /* g_0029C6B0, g_0029C758, g_00351454, g_00351480 */
#include "game/app/T002_render_thread.h"        /* render_set_object */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push / menu_heap_pop */
#include "game/menu/framework/T244_button_data.h"          /* ButtonData (name) */
#include "game/menu/framework/T246_render_base.h"          /* TRANSITION_FADEOUT */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C / g_00352750 */
#include "game/race/core/T414_race_random.h"          /* raceRandomSkip */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "libc.h"                           /* strcmp */
#include "game/menu/race_menu/shortmenu_internal.h"                           /* g_003514F0, g_00314848 */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_si, type_info */
#endif

namespace gt2k_menu {

/* The members are built in order: ControlBase(), the vptr, render (the
 * out-of-line ShortmenuRender constructor), PageManager(this); heap and
 * startPage are cleared afterwards.  The landing pads destroy render (its
 * RootObject with flags 2, then ~GameObject with flags 0) and the
 * ControlBase (~ControlBase, flags 0). */
// FUNC 0x0012C300 gt2k_menu::ShortmenuControl::ShortmenuControl
ShortmenuControl::ShortmenuControl()
    : ControlBase(), render(), pm(this)
{
    /* render: ShortmenuRender(); pm: PageManager(this).  Landing pads:
     * RootObject::~RootObject, GameObject::~GameObject, ~ControlBase. */
    /* implicit: ShortmenuRender, PageManager, ~RootObject, ~GameObject (member constructors, landing pads) */
    heap = NULL;
    startPage = NULL;
}

/* Pushes the menu heap `mem' (size bytes); a heap pushed before is popped
 * first. */
// FUNC 0x0012C3B0 gt2k_menu::ShortmenuControl::setHeap
void ShortmenuControl::setHeap(void *mem, int size)
{
    if (heap != NULL)
        menu_heap_pop();
    heap = mem;
    menu_heap_push(mem, size);
}

// FUNC 0x0012C408 gt2k_menu::ShortmenuControl::releaseHeap
void ShortmenuControl::releaseHeap()
{
    menu_heap_pop();
    heap = NULL;
}

} /* namespace gt2k_menu */

using namespace gt2k_menu;

/* The availability check of every button of page "root" (g_0029C2C8: 18
 * times this function in g_00314848): 0 makes the button unavailable.  The
 * button is known by its name (ButtonData::name, the record's first
 * member); ctx (the ShortmenuControl) is not used, the flags are those of
 * the running session.  Buttons not named here are available.  The ten
 * tests have one shape (`free_session' too: a `return flag == 0' there
 * fills the delay slots differently). */
// FUNC 0x0012C430 shortmenu_root_check
int shortmenu_root_check(void *ctx, ButtonData *data)
{
    ShortMenuArgs *args = g_003514F0;

    if (strcmp(data->name, "replay") == 0 && args->canReplay == 0)
        return 0;
    if (strcmp(data->name, "save_replay") == 0 && args->canReplay == 0)
        return 0;
    if (strcmp(data->name, "save_ghost") == 0 && args->canSaveGhost == 0)
        return 0;
    if (strcmp(data->name, "data_logger") == 0 && args->canLogger == 0)
        return 0;
    if (strcmp(data->name, "setting") == 0 && args->noSetting != 0)
        return 0;
    if (strcmp(data->name, "save_setting") == 0 && args->noSetting != 0)
        return 0;
    if (strcmp(data->name, "load_setting") == 0 && args->noSetting != 0)
        return 0;
    if (strcmp(data->name, "exit") == 0 && args->noExit != 0)
        return 0;
    if (strcmp(data->name, "save_game") == 0 && args->noExit != 0)
        return 0;
    if (strcmp(data->name, "free_session") == 0 && args->noFreeSession != 0)
        return 0;
    return 1;
}

/* The checks of page "root": 18 times shortmenu_root_check (T111 .rodata,
 * after the check's strings and before the vtable: defined here). */
ShortMenuButtonCheck *const g_00314848[18] = {                 /* 0x00314848 */
    shortmenu_root_check, shortmenu_root_check, shortmenu_root_check,
    shortmenu_root_check, shortmenu_root_check, shortmenu_root_check,
    shortmenu_root_check, shortmenu_root_check, shortmenu_root_check,
    shortmenu_root_check, shortmenu_root_check, shortmenu_root_check,
    shortmenu_root_check, shortmenu_root_check, shortmenu_root_check,
    shortmenu_root_check, shortmenu_root_check, shortmenu_root_check
};

/* The entries of g_0029C2C8 (T103's ShortmenuLicenseControl::loadPages
 * reads it; loadPages here uses the constant).  T111 .sdata between the
 * check's "exit" and the initialiser's "root": defined here. */
int g_00351440 = sizeof(g_0029C2C8) / sizeof(g_0029C2C8[0]);   /* 0x00351440 */

/* The SelectableFuncData of page "root" (T111 .data, before the frame info;
 * zeros in the image: T111_static_init fills it, the original's entry was
 * built by a constructor at run time).  No initialiser: egcs writes it as
 * zeros in .data at the end of the file's output. */
SelectableFuncData g_0029C2C8[1];           /* 0x0029C2C8 */

namespace gt2k_menu {

/* Loads the page file (pm.initialize: the object tree is the renderer's
 * root, no cursor memory; pm.loadFile) and sets the "root" checks as the
 * selectable table {g_0029C2C8, 1 entry, this} (the inline
 * PageManager::setSelectableFuncData: retail stores through &pm). */
// FUNC 0x0012C5C8 gt2k_menu::ShortmenuControl::loadPages
void ShortmenuControl::loadPages(const char *file)
{
    pm.initialize(&render.root, NULL);
    pm.loadFile(file);
    pm.setSelectableFuncData(g_0029C2C8, 1, this);
}

// FUNC 0x0012C638 gt2k_menu::ShortmenuControl::unloadPages
void ShortmenuControl::unloadPages()
{
    pm.finalize();
    pm.unloadFile();
}

/* Starts the session: the draw state reset (menu_draw_reset), the pads
 * (attach) and one frame (waitFrame, a virtual call), the renderer on the
 * render thread (render.begin) with a fade-out for its detach, the qm_*
 * actions as the execute table and the qm_* hooks in both layouts (the
 * inlines RenderBase::setTransition, PageManager::setExecuteFuncData /
 * setHooks: retail keeps &render and &pm in registers); the per-frame
 * screen clear off.  Retail loads the action count (g_00351454) before the
 * hook count and stores transitionOn late: RenderBase::transitionOn (T246)
 * and g_00352750 (T261) are bools, 4 bytes in egcs with their own alias set
 * (as int they gave 92%; T103's ShortmenuLicenseControl::begin is the same
 * code). */
// FUNC 0x0012C668 gt2k_menu::ShortmenuControl::begin
void ShortmenuControl::begin()
{
    menu_draw_reset();
    attach();
    waitFrame();
    render.begin();
    render.setTransition(true, TRANSITION_FADEOUT);
    pm.setExecuteFuncData(g_0029C6B0, g_00351454, this);
    pm.setHooks(g_0029C758, g_00351480, this);
    g_00352750 = 0;
    g_0035274C = 0;
}

// FUNC 0x0012C730 gt2k_menu::ShortmenuControl::end
void ShortmenuControl::end()
{
    pm.finalize();
    gfx_vsync(2);
    render.end();
    detach();
}

/* Builds the page set by run (and startPage over it as a popup, if any)
 * and runs it until the page manager has exited: a frame of input, a frame
 * of the control (waitFrame), one step of the race random generator.  Then
 * the race behind the menu stops (raceActive = 0 through the inline
 * getRender(): retail computes this + 0x290 into $s1 before the loop, and
 * the loop test is duplicated in front of it) and the render thread draws
 * nothing. */
// FUNC 0x0012C770 gt2k_menu::ShortmenuControl::mainLoop
void ShortmenuControl::mainLoop()
{
    pm.startPage(NULL);
    if (startPage != NULL) {
        pm.pushPopup(NULL);
        pm.setPage(startPage, NULL);
        pm.startPage(NULL);
    }
    while (pm.exited == 0) {
        pm.update();
        waitFrame();
        raceRandomSkip(1);
    }
    getRender()->raceActive = 0;
    gfx_vsync(2);
    render_set_object(NULL);
    gfx_vsync(2);
}

/* arg: the page imported into `page' (PageManager::setPage's importPage;
 * NULL: none). */
// FUNC 0x0012C848 gt2k_menu::ShortmenuControl::run
void ShortmenuControl::run(const char *page, const char *arg)
{
    begin();
    pm.setPage(page, arg);
    mainLoop();
    end();
}

} /* namespace gt2k_menu */

/* Static initialiser (.ctors entry 0x0028F0D8): the one SelectableFuncData
 * entry, the 18 checks of page "root", stored in member order (page,
 * checks, count; as u001's T016_static_init).  Compiler-generated in the
 * original (an entry built by a constructor); written out. */
// FUNC 0x0012C8A8 T111_static_init
void T111_static_init(void)
{
    g_0029C2C8[0].page   = "root";
    g_0029C2C8[0].checks = (ShortMenuButtonCheck **)g_00314848;
    g_0029C2C8[0].count  = 0x12;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base ControlBase). */
// FUNC 0x0012C8D8 __tfQ29gt2k_menu16ShortmenuControl
extern "C" void *__tfQ29gt2k_menu16ShortmenuControl(void)
{
    if (__tiQ29gt2k_menu16ShortmenuControl[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu16ShortmenuControl, "Q29gt2k_menu16ShortmenuControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu16ShortmenuControl;
}
#endif

// FUNC 0x0012C928 gt2k_menu::ShortmenuControl::~ShortmenuControl
/* implicit in game/menu/race_menu/T111_shortmenu_control.h; egcs emits this copy (slot
 * [0]): render's inline destructor (RootObject::~RootObject at +0x294 with
 * flags 2, then GameObject::~GameObject at +0x290 with flags 2; pm has
 * none), then ~ControlBase with this destructor's flags (it does the
 * delete); no vptr store. */
/* implicit: ~RootObject, ~GameObject, ~ControlBase (member / base destructors) */
