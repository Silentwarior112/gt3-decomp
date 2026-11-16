/* T105 - gt2k_menu::NotCarGetControl, the "no prize car" page.
 * 0x0013B380..0x0013B848.
 *
 * not_car_get() (called by T061's car_get_prizes instead of the prize car
 * screen when the garage already holds 200 cars) runs the page
 * "not_car_get" of "menu/not_get_car.mbl" on a 256 KB menu heap until it is
 * left.  The page has no item factories and no ExecuteFuncData.
 *
 * Also an unreferenced copy of ControlBaseWithPageManager's implicit
 * destructor and typeinfo function (vtable copy 0x00318FA0). */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T105's .rodata (0x00318F50..0x0031928F): "bad_alloc" (<new>, read first),
 * the two literals, the two vtables, then the type names NotCarGetControl,
 * PrizeControlBase, ControlBaseWithPageManager, SmartIndexT<int>,
 * CallBackWhenPrizeRendering, PrizeModelSetContext, the OperatorHolder
 * group, UnitArena<64>, RegurationResult, RecordsMenuArg, the garage lists,
 * Operator (T060's set without the PrizeCounter and the trophy screens).
 * No .sdata.  ref/data/u020/notes.md. */
// DATA_START 0x00318F50
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* (no use: the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (no use: the OperatorHolder group) */
#include "game/menu/gtmode/T460_prize_base.h"           /* (no use: PrizeModelSetContext, CallBackWhenPrizeRendering) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (SmartIndexT<int>) */
/* PrizeControlBase alone (its type name precedes ControlBaseWithPageManager's) */
#define T059_BASE_ONLY
#include "game/menu/gtmode/T059_license_get_control.h"  /* (no use: ControlBaseWithPageManager, PrizeControlBase) */
#undef T059_BASE_ONLY
#include "game/menu/gtmode/T105_not_car_get.h"          /* own */
/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase, TRANSITION_FADEOUT */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (attach, detach, waitFrame) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#define U020_T105                           /* trophy_get_internal.h: this unit's constants */
#include "game/menu/gtmode/trophy_get_internal.h"

namespace gt2k_menu {

// FUNC 0x0013B380 gt2k_menu::NotCarGetControl::load
void NotCarGetControl::load()
{
    pages.loadFile("menu/not_get_car.mbl");
}

// FUNC 0x0013B3A8 gt2k_menu::NotCarGetControl::unload
void NotCarGetControl::unload()
{
    pages.unloadFile();
}

/* As TrophyGetControl::begin without the music: the render on the render
 * thread with the fade-out transition and no mask, the page manager on the
 * render's tree with empty hook tables, no ExecuteFuncData, no screen
 * clear (the inline accessors expanded the same way: &render, &pages in
 * s-registers). */
// FUNC 0x0013B3C8 gt2k_menu::NotCarGetControl::begin
void NotCarGetControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    render.setTransition(true, TRANSITION_FADEOUT); /* before setMask: retail's store order with
                                                       T246's on-first setTransition */
    render.setMask(false);
    pages.initialize(render.getRootObject(), NULL); /* no cursor memory */
    pages.setHooks(NULL, 0, this);
    pages.setExecuteFuncData(NULL, 0, this);
    g_00352750 = 0;                                 /* no screen clear, colour 0 */
    g_0035274C = 0;
}

// FUNC 0x0013B480 gt2k_menu::NotCarGetControl::end
void NotCarGetControl::end()
{
    pages.finalize();
    render.detach();
    gfx_vsync(2);
    detach();
}

/* Page "not_car_get": opened with the pad frozen, released at once, 15
 * frames, then frames until the page is left. */
// FUNC 0x0013B4C0 gt2k_menu::NotCarGetControl::mainLoop
void NotCarGetControl::mainLoop()
{
    PageManager *pm = &pages;
    waitFrame();
    pm->setFreeze(1);
    pm->setPage("not_car_get", NULL);
    pm->startPage(NULL);
    pm->setFreeze(0);
    gfx_vsync(15);
    while (pages.exited == 0) {
        pm->update();
        waitFrame();
    }
    pm->closePage();
}

// FUNC 0x0013B580 gt2k_menu::NotCarGetControl::run
void NotCarGetControl::run()
{
    load();
    begin();
    mainLoop();
    end();
    unload();
}

} /* namespace gt2k_menu */

using namespace gt2k_menu;

/* Returns 0 if the menu heap cannot be allocated, else 1.  The controller
 * is a stack object. */
// FUNC 0x0013B5C8 not_car_get
int not_car_get(void)
{
    menu_draw_reset();
    void *heap = memalign(0x40, NOT_CAR_GET_HEAP_SIZE);
    if (heap == NULL)
        return 0;
    menu_heap_push(heap, NOT_CAR_GET_HEAP_SIZE);
    {
        /* sp+0x000.  The inline constructor is expanded: ControlBase(), the
         * ControlBaseWithPageManager vptr, PageManager(this), the
         * NotCarGetControl vptr, RenderBase() (the landing pads:
         * ~ControlBase with flags 0, then __throw). */
        NotCarGetControl screen;
        /* implicit: ControlBase, PageManager, RenderBase (inline constructor) */
        screen.run();
        gfx_vsync(1);
        /* The implicit destructor is expanded: ~RootObject and
         * GameObject::~GameObject (render), then ~ControlBase, all with
         * flags 2. */
        /* implicit: ~RootObject, ~GameObject, ~ControlBase (member / base destructors) */
    }
    menu_heap_pop();
    free(heap);
    return 1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * ControlBaseWithPageManager). */
// FUNC 0x0013B738 __tfQ29gt2k_menu16NotCarGetControl
extern "C" void *__tfQ29gt2k_menu16NotCarGetControl(void)
{
    if (__tiQ29gt2k_menu16NotCarGetControl[0] == 0) {
        __tfQ29gt2k_menu26ControlBaseWithPageManager();
        __rtti_si(__tiQ29gt2k_menu16NotCarGetControl, "Q29gt2k_menu16NotCarGetControl",
                  (const type_info *)__tiQ29gt2k_menu26ControlBaseWithPageManager);
    }
    return __tiQ29gt2k_menu16NotCarGetControl;
}
#endif

// FUNC 0x0013B788 gt2k_menu::NotCarGetControl::~NotCarGetControl
/* implicit (game/menu/gtmode/T105_not_car_get.h); egcs emits this copy for the vtable:
 * render (~RootObject +0xDEC, GameObject::~GameObject +0xDE8, flags 2),
 * then ~ControlBase(this, flags), which does the operator delete */
/* implicit: ~RootObject, ~GameObject (member destructor), ~ControlBase (base destructor) */

/* ------------------------------------------------------------------------ */
/* T105's copies of ControlBaseWithPageManager's typeinfo function and      */
/* implicit destructor (unreferenced; the vtable copy uses T056's).         */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* T056's is 0x00138BD8 (this copy has its own name string 0x00318FF8).
 * Weak, so that the owner's definition wins, as at the original link. */
// FUNC 0x0013B7D8 __tfQ29gt2k_menu26ControlBaseWithPageManager
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menu26ControlBaseWithPageManager(void)
{
    if (__tiQ29gt2k_menu26ControlBaseWithPageManager[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu26ControlBaseWithPageManager,
                  "Q29gt2k_menu26ControlBaseWithPageManager",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu26ControlBaseWithPageManager;
}
#endif

// FUNC 0x0013B828 gt2k_menu::ControlBaseWithPageManager::~ControlBaseWithPageManager
/* implicit (game/menu/gtmode/T056_gtmode_trophy_control.h); egcs emits this copy (T056's
 * is 0x00138C28): ~ControlBase(this, flags) */
/* implicit: ~ControlBase (base destructor) */
