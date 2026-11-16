/* T103 - gt2k_menu::ShortmenuLicenseControl / ShortmenuLicenseRender, the
 * licence-test variant of the short menu (u015's ShortmenuControl /
 * ShortmenuRender).  0x00134630..0x00134EE8.
 *
 * A stack object of run_short_menu_license (T102, u015), whose only caller
 * is run_license_test (T053): the race behind the menu is a RaceLicense (a
 * RaceSolitaire).  The differences with the plain short menu: the race is
 * started in replay mode (RaceBase::virtual_8(replayMode)) and restarted the
 * same way every time it signals its end (updateRace, once per frame); the
 * renderer draws it in a 280 x 210 sub-window of the frame
 * (RacePS2Base::drawSubView) and draws nothing at all while `hidden' (until
 * the first page is open).
 *
 * Retail's data (u018's data pass): .rodata 0x003168C8..0x00316CC8,
 * "bad_alloc" (<new>, read first), the two vtables, then the type names
 * ShortmenuLicenseControl, ShortmenuLicenseRender, ShortmenuControl,
 * SmartIndexT<int>, ResultTermBar .. RaceMenuButton (the race menu
 * classes), OperatorList .. Cell<OperatorHolder>, RaceMiniMap ..
 * RaceDisplayBase, RaceCameraBase, UnitArena<64>, the garage's (wheel list,
 * then car list), Operator: the reverse of the class definitions, which the
 * includes below give (the race headers T054 includes, without
 * RegurationResult / RecordsMenuArg, which retail's T103 does not have).
 * .sdata 0x00351538: T157's "" (RaceMessageDisplay::clear, through T154);
 * .lit4 0x0034FB10 (1.3).  No .bss. */
#include <new>                              /* first: "bad_alloc" */
#include "types.h"
#include "game/save/T429_game_status.h"          /* (no use: the garage's type names) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (retail has it) */
#include "game/race/core/T153_race_course.h"
#include "game/camera/T383_race_camera_base.h"     /* (no use: RaceCameraBase) */
#include "game/race/display/T154_race_display.h"         /* (no use: the race displays; T157's "" opens the .sdata) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (no use: the menu classes) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (no use: RaceMenuButton) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (no use: RaceMenuWindow, RaceMenuSelectWindow) */
#include "game/race/pause/T169_race_pause.h"           /* (no use: PauseMenu) */
#include "game/race/pause/T170_replay_pause.h"         /* (no use: ReplayPauseMenu) */
#include "game/menu/records/T200_result_board.h"         /* (no use: ResultTitleBar, ResultTermBar) */
#include "game/menu/race_menu/T111_shortmenu_control.h"    /* g_0029C2C8, g_00351440 (the short menu's page "root"
                                               checks); SmartIndexT<int>, ShortmenuControl */
#include "game/menu/race_menu/T103_shortmenu_license.h"
#include "engine/pgl/G001_flare.h"                /* gfx_vsync, pgl_matrix_mode, pgl_push / pop_matrix,
                                               pgl_push / pop_attrib, PGL_MTX_*, PGL_ATTR_* */
#include "game/app/T002_render_thread.h"        /* render_set_object */
#include "game/menu/race_menu/T113_shortmenu_tables.h"     /* g_0029C6B0, g_0029C758, g_00351454, g_00351480 */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base */
#include "game/race/modes/T139_race_solitaire.h"       /* RaceSolitaire */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase, TRANSITION_FADEOUT */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play_if_changed */
#include "game/race/core/T414_race_random.h"          /* raceRandomSkip */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/debug/logger_internal.h"

// DATA_START 0x003168C8
// DATA_START 0x00351538

namespace gt2k_menu {

/* RenderBase(), hidden = 1, the vptr, race = NULL. */
// FUNC 0x00134630 gt2k_menu::ShortmenuLicenseRender::ShortmenuLicenseRender
ShortmenuLicenseRender::ShortmenuLicenseRender()
    : RenderBase()
{
    hidden = 1;
    race = NULL;
}

/* Called by ShortmenuLicenseControl::begin: the render thread draws this. */
// FUNC 0x00134670 gt2k_menu::ShortmenuLicenseRender::begin
void ShortmenuLicenseRender::begin()
{
    attach();
}

/* Called by ShortmenuLicenseControl::end. */
// FUNC 0x00134690 gt2k_menu::ShortmenuLicenseRender::end
void ShortmenuLicenseRender::end()
{
    detach();
}

/* GameObject slot 1.  Only the object tree (RootObject::virtual_1, called
 * directly on the member); the race is run by
 * ShortmenuLicenseControl::updateRace. */
// FUNC 0x001346B0 gt2k_menu::ShortmenuLicenseRender::update
void ShortmenuLicenseRender::update()
{
    root.virtual_1();
}

/* GameObject slot 2.  Nothing at all while hidden.  Else the screen clear
 * is turned off and, with a race: its showGhost is cleared every frame
 * (the ghost hidden: SELECT flips it during the race), and when the race
 * renders, the GS state is saved (pgl_push_attrib of the draw buffer,
 * depth, target B, fog, viewport and scissor, both matrices pushed), the
 * race is drawn into a copy of buf restricted to (32, 58, 280 x 210) with
 * its mirror view set to (80, 10, 120 x 32) x 1.3, and the state is
 * restored; the menu (RenderBase::render) is drawn over it.  The GSBuffer
 * is destroyed with flags 2 (also by its landing pad). */
// FUNC 0x001346D0 gt2k_menu::ShortmenuLicenseRender::render
void ShortmenuLicenseRender::render(GSBuffer &buf)
{
    if (hidden != 0)
        return;
    g_00352750 = 0;
    g_0035274C = 0;
    if (race != NULL) {
        /* The licence test's race is a RaceLicense, a RaceSolitaire (the
         * store is past RacePS2Base: RaceSolitaire::showGhost). */
        /* renderEnabled is read before the showGhost store (both ints:
         * retail's load is ahead of the store, which sits in the branch's
         * delay slot). */
        int enabled = race->renderEnabled;
        ((RaceSolitaire *)race)->showGhost = 0;
        if (enabled != 0) {
            pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B |
                            PGL_ATTR_FOG | PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR);   /* 0x8C7 */
            pgl_matrix_mode(PGL_MTX_PROJECTION);
            pgl_push_matrix();
            pgl_matrix_mode(PGL_MTX_MODELVIEW);
            pgl_push_matrix();
            {
                GSBuffer view;              /* sp+0x00: GSBuffer(), then ~GSBuffer (flags 2) */
                view.copy(&buf);
                view.setRect(32, 58, 280, 210);
                race->mirror.set(1.3f, 80.0f, 10.0f, 120.0f, 32.0f);
                race->drawSubView(&view, 0, 1.0f);
                pgl_pop_attrib();
                pgl_matrix_mode(PGL_MTX_PROJECTION);
                pgl_pop_matrix();
                pgl_matrix_mode(PGL_MTX_MODELVIEW);
                pgl_pop_matrix();
            }
        }
    }
    RenderBase::render(buf);
}

// FUNC 0x00134860 gt2k_menu::ShortmenuLicenseRender::setRace
void ShortmenuLicenseRender::setRace(RacePS2Base *r)
{
    race = r;
}

// FUNC 0x00134868 gt2k_menu::ShortmenuLicenseRender::clearRace
void ShortmenuLicenseRender::clearRace()
{
    race = NULL;
}

/* The members are built in order: ControlBase, the vptr, render (the
 * out-of-line ShortmenuLicenseRender constructor), pm (PageManager(this));
 * then heap = 0, replayMode = 2, startPage = 0 (bgmGroup is left as it is).  The
 * landing pads destroy render (its RootObject with flags 2 - ~RootObject -,
 * then GameObject::~GameObject with flags 0) and the ControlBase
 * (~ControlBase, flags 0). */
// FUNC 0x00134870 gt2k_menu::ShortmenuLicenseControl::ShortmenuLicenseControl
ShortmenuLicenseControl::ShortmenuLicenseControl()
    : ControlBase(), render(), pm(this)
{
    /* render: ShortmenuLicenseRender(); pm: PageManager(this).  Landing
     * pads: ~RootObject (flags 2) and GameObject::~GameObject for render,
     * ~ControlBase. */
    /* implicit: ShortmenuLicenseRender, PageManager (member constructors), ~RootObject, ~GameObject (exception cleanup) */
    heap = NULL;
    replayMode = 2;
    startPage = NULL;
}

/* Pushes the menu heap `mem' (size bytes); a heap pushed before is popped
 * first. */
// FUNC 0x00134928 gt2k_menu::ShortmenuLicenseControl::setHeap
void ShortmenuLicenseControl::setHeap(void *mem, int size)
{
    if (heap != NULL)
        menu_heap_pop();
    heap = mem;
    menu_heap_push(mem, size);
}

// FUNC 0x00134980 gt2k_menu::ShortmenuLicenseControl::releaseHeap
void ShortmenuLicenseControl::releaseHeap()
{
    menu_heap_pop();
    heap = NULL;
}

/* Loads the page file (the object tree is the renderer's root; no cursor
 * memory) and makes the item checks of the short menu (T111's page "root"
 * table, g_00351440 = 1 entry) the selectable table, ctx this (the fields
 * set directly, not through setSelectableFuncData). */
// FUNC 0x001349A8 gt2k_menu::ShortmenuLicenseControl::loadPages
void ShortmenuLicenseControl::loadPages(const char *file)
{
    pm.initialize(&render.root, NULL);
    pm.loadFile(file);
    pm.setSelectableFuncData(g_0029C2C8, g_00351440, this);
}

// FUNC 0x00134A18 gt2k_menu::ShortmenuLicenseControl::unloadPages
void ShortmenuLicenseControl::unloadPages()
{
    pm.finalize();
    pm.unloadFile();
}

/* Starts the session (as ShortmenuControl::begin): the menu draw state
 * reset, the pads attached and one frame of the ControlBase, the renderer
 * on screen with a fade-out transition for its end, the qm_* actions (T113,
 * g_00351454 = 21 of them) as the execute table and the qm_* hooks
 * (g_00351480 = 45) in both page layouts; no screen clear..  The inlines
 * RenderBase::setTransition, PageManager::setExecuteFuncData / setHooks.
 * Retail loads the action count before the hook count and stores
 * transitionOn late, as u015's ShortmenuControl::begin: RenderBase::
 * transitionOn and g_00352750 are bools (4 bytes in egcs, their own alias
 * set; applied in u015's revisit, see ref/matching/u015/notes.md). */
// FUNC 0x00134A48 gt2k_menu::ShortmenuLicenseControl::begin
void ShortmenuLicenseControl::begin()
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

// FUNC 0x00134B10 gt2k_menu::ShortmenuLicenseControl::end
void ShortmenuLicenseControl::end()
{
    pm.finalize();
    gfx_vsync(2);
    render.end();
    detach();
}

/* Opens the page set by run (then the menu music of bgmGroup, and startPage
 * as a popup over it, if any), shows the renderer and runs the page until
 * the page manager exits: one frame of the race per menu frame
 * (updateRace) and one step of the race random generator; then the render
 * thread is given no object and the BGM stops. */
// FUNC 0x00134B50 gt2k_menu::ShortmenuLicenseControl::mainLoop
void ShortmenuLicenseControl::mainLoop()
{
    pm.startPage(NULL);
    menu_bgm_play_if_changed(bgmGroup, 0, 1);
    if (startPage != NULL) {
        pm.pushPopup(NULL);
        pm.setPage(startPage, NULL);
        pm.startPage(NULL);
    }
    render.hidden = 0;
    while (pm.exited == 0) {
        pm.update();
        updateRace();
        waitFrame();
        raceRandomSkip(1);
    }
    gfx_vsync(2);
    render_set_object(NULL);
    gfx_vsync(2);
    bgm_stop();
}

/* One frame of the race behind the menu (not rendered meanwhile:
 * renderEnabled 0 around it); when the race signals its end (takeSignal)
 * it is ended (virtual_35) and started again in replayMode (virtual_8):
 * the replay loops. */
// FUNC 0x00134C30 gt2k_menu::ShortmenuLicenseControl::updateRace
void ShortmenuLicenseControl::updateRace()
{
    RacePS2Base *race = render.race;
    if (race == NULL)
        return;
    race->renderEnabled = 0;
    race->update();
    if (race->takeSignal() == RACE_SIGNAL_END) {
        race->virtual_35();
        race->virtual_8(replayMode);
    }
    race->renderEnabled = 1;
}

/* Hides the renderer, starts the race in replayMode and runs a first frame
 * of it, then begin, the page (setPage(page, importPage)), mainLoop, end,
 * and ends the race.  The race is used without a NULL check
 * (run_short_menu_license allows NULL). */
// FUNC 0x00134CD8 gt2k_menu::ShortmenuLicenseControl::run
void ShortmenuLicenseControl::run(const char *page, const char *arg)
{
    render.setHidden(1);
    render.getRace()->virtual_8(replayMode);
    render.getRace()->update();
    begin();
    pm.setPage(page, arg);
    mainLoop();
    end();
    render.getRace()->virtual_35();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00134D98 __tfQ29gt2k_menu23ShortmenuLicenseControl
extern "C" void *__tfQ29gt2k_menu23ShortmenuLicenseControl(void)
{
    if (__tiQ29gt2k_menu23ShortmenuLicenseControl[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu23ShortmenuLicenseControl, "Q29gt2k_menu23ShortmenuLicenseControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu23ShortmenuLicenseControl;
}
#endif

// FUNC 0x00134DE8 gt2k_menu::ShortmenuLicenseControl::~ShortmenuLicenseControl
/* implicit in game/menu/race_menu/T103_shortmenu_license.h (no vptr store); egcs emits this copy.
 * Slot 0.  pm has no destructor; render is destroyed inline (its RootObject
 * at +0x294 with flags 2: ~RootObject, then GameObject::~GameObject at
 * +0x290 with flags 2), then ~ControlBase runs with this destructor's flags
 * (ControlBase does the operator delete).  run_short_menu_license has the
 * same code inline. */
/* implicit: ~RootObject, ~GameObject (member destructors of render), ~ControlBase (base destructor) */

// FUNC 0x00134E38 gt2k_menu::ShortmenuLicenseRender::getRace
/* inline in game/menu/race_menu/T103_shortmenu_license.h; egcs emits this copy (no callers) */

// FUNC 0x00134E40 gt2k_menu::ShortmenuLicenseRender::isHidden
/* inline in game/menu/race_menu/T103_shortmenu_license.h; egcs emits this copy (no callers) */

// FUNC 0x00134E48 gt2k_menu::ShortmenuLicenseRender::setHidden
/* inline in game/menu/race_menu/T103_shortmenu_license.h; egcs emits this copy (no callers) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00134E50 __tfQ29gt2k_menu22ShortmenuLicenseRender
extern "C" void *__tfQ29gt2k_menu22ShortmenuLicenseRender(void)
{
    if (__tiQ29gt2k_menu22ShortmenuLicenseRender[0] == 0) {
        __tfQ29gt2k_menu10RenderBase();
        __rtti_si(__tiQ29gt2k_menu22ShortmenuLicenseRender, "Q29gt2k_menu22ShortmenuLicenseRender",
                  (const type_info *)__tiQ29gt2k_menu10RenderBase);
    }
    return __tiQ29gt2k_menu22ShortmenuLicenseRender;
}
#endif

// FUNC 0x00134EA0 gt2k_menu::ShortmenuLicenseRender::~ShortmenuLicenseRender
/* implicit in game/menu/race_menu/T103_shortmenu_license.h (no vptr store); egcs emits this copy.
 * Slot 0: the implicit RenderBase destructor expanded: ~RootObject(root, 2),
 * then GameObject::~GameObject(this, flags), which does the delete. */
/* implicit: ~RootObject, ~GameObject (the implicit RenderBase destructor) */
