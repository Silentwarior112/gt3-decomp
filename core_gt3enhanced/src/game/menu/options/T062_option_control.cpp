/* T062 - gt2k_menu::OptionControl, the options menu ("menu/option.mbl", page
 * "option").  0x0013F940..0x0013FF98.
 *
 * F_0013FE58 (from main and the arcade / GT-mode menus) builds the
 * controller on the stack over a 1 MB menu heap, also on the stack.  The
 * controller edits a copy of the game's GameOptions (GameStatus +0x2FF38),
 * copied back when the page is left.  begin() hands the PageManager the
 * tables of T063..T067; the option items (the T064 hooks, u014's
 * KeyConfigItem / HandicapItem / CalibrationItem) reach the controller
 * through the accessors at the end of this file. */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T062's .rodata (0x00319D18..0x00319FC6): "bad_alloc" (<new>, read first),
 * "menu/option.mbl", OptionControl's vtable, then the type names
 * Cell<Button>, the OperatorHolder group, OptionControl, UnitArena<64>,
 * RegurationResult, RecordsMenuArg, SmartIndexT<int>, the garage lists,
 * Operator: the menu item / button classes after this file's own header.
 * ref/data/u021/notes.md. */
// DATA_START 0x00319D18
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus (the garage) */
#include "game/menu/framework/T211_page_manager.h"         /* (SmartIndexT<int>) gt2k_menu::PageManager */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/menu/options/T062_option_control.h"       /* own */
#include "game/menu/framework/T206_button.h"               /* (no use: the OperatorHolder group, Cell<Button>) */
/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "game/menu/options/T063_option_calib_checks.h"  /* g_002A0A70 (ValidFuncData), g_003516F0 */
#include "game/menu/options/T064_option_hooks.h"         /* g_002A0AD0 (MenuHook), g_00351708 */
#include "game/menu/options/T065_option_sliders.h"       /* g_002A1C38 (MenuSliderFunc), g_00351718 */
#include "game/menu/options/T066_option_multis.h"        /* g_002A1CD8 (MenuMultiFunc), g_0035171C */
#include "game/menu/options/T067_option_actions.h"       /* g_002A2038 (ExecuteFuncData), g_00351720 */
#include "game/menu/options/T101_key_config_item.h"      /* gt2k_menu::KeyConfigItem (handleInput, checkExit) */
#include "game/save/T428_game_options.h"         /* GameOptions */
#include "game/menu/gtmode/car_get_internal.h"

namespace gt2k_menu {

/* ControlBase::ControlBase, the vptr, RenderBase::RenderBase,
 * PageManager::PageManager(this), the fields, GameOptions::GameOptions;
 * the controller's page manager is its own.  The landing pads undo the
 * members built so far (RootObject::~RootObject, GameObject::~GameObject,
 * ControlBase::~ControlBase). */
// FUNC 0x0013F940 gt2k_menu::OptionControl::OptionControl
OptionControl::OptionControl()
    : render(), pages(this), keyConfig1(NULL), keyConfig1Count(0), keyConfig2(NULL),
      keyConfig2Count(0), calibPort(0), calibWheel(2), options()
{
    /* ControlBase::ControlBase, the vptr, RenderBase::RenderBase,
     * PageManager::PageManager(this), the fields, GameOptions::GameOptions */
    /* implicit: ControlBase (base constructor), RenderBase, PageManager, GameOptions (member constructors), ~RootObject, ~GameObject, ~ControlBase (exception cleanup) */
    pageManager = &pages;
}

/* The vptr (0x00319D38); render's members: RootObject::~RootObject (+0x290,
 * flags 2), GameObject::~GameObject (+0x28C, flags 2); pages has none;
 * options has none (GameOptions); then ControlBase::~ControlBase(this,
 * flags), which does the operator delete. */
// FUNC 0x0013FA20 gt2k_menu::OptionControl::~OptionControl
OptionControl::~OptionControl()
{
    /* RootObject::~RootObject, GameObject::~GameObject (render),
     * ControlBase::~ControlBase: generated */
    /* implicit: ~RootObject, ~GameObject (member destructors of render), ~ControlBase (base destructor) */
}

// FUNC 0x0013FA88 gt2k_menu::OptionControl::load
void OptionControl::load()
{
    pages.loadFile("menu/option.mbl");
}

// FUNC 0x0013FAB0 gt2k_menu::OptionControl::unload
void OptionControl::unload()
{
    pages.unloadFile();
}

/* The pads, one frame, the render (the fade-out transition when it is
 * detached), a copy of the game's options (GameOptions::operator=), the
 * page manager over the render's tree, then the tables of the option
 * pages (counts from .sdata): the hooks (T064), sliders (T065) and multi
 * buttons (T066) to both layouts, the actions (T067, ExecuteFuncData) and
 * the page checks (T063, ValidFuncData); the per-frame screen clear off. */
// FUNC 0x0013FAD0 gt2k_menu::OptionControl::begin
void OptionControl::begin()
{
    attach();
    waitFrame();                            /* (virtual: slot 1) */
    render.attach();
    render.setTransition(true, TRANSITION_FADEOUT);
    /* implicit: operator= (GameOptions assignment) */
    options = getGameStatus()->options;     /* GameOptions::operator= */
    pages.initialize(render.getRootObject(), NULL);
    pages.setHooks(g_002A0AD0, g_00351708, this);
    pages.setSliderFuncs(g_002A1C38, g_00351718, this);
    pages.setMultiFuncs(g_002A1CD8, g_0035171C, this);
    pages.setExecuteFuncData(g_002A2038, g_00351720, this);
    pages.setValidFuncData(g_002A0A70, g_003516F0, this);
    g_00352750 = 0;
    g_0035274C = 0;
}

/* The pages are released, two frames, the render is detached (its
 * transition), the pads. */
// FUNC 0x0013FC58 gt2k_menu::OptionControl::end
void OptionControl::end()
{
    pages.finalize();
    gfx_vsync(2);
    render.detach();
    detach();
}

/* KeyConfigItem's constructor (T101) for player 1's (or the USB) page. */
// FUNC 0x0013FC98 gt2k_menu::OptionControl::attachKeyConfig1
void OptionControl::attachKeyConfig1(KeyConfigItem *item)
{
    keyConfig1 = item;
    keyConfig1Count++;
}

/* KeyConfigItem's destructor (T101). */
// FUNC 0x0013FCB0 gt2k_menu::OptionControl::detachKeyConfig1
void OptionControl::detachKeyConfig1()
{
    if (--keyConfig1Count <= 0)
        keyConfig1 = NULL;
}

// FUNC 0x0013FCD0 gt2k_menu::OptionControl::attachKeyConfig2
void OptionControl::attachKeyConfig2(KeyConfigItem *item)
{
    keyConfig2 = item;
    keyConfig2Count++;
}

// FUNC 0x0013FCE8 gt2k_menu::OptionControl::detachKeyConfig2
void OptionControl::detachKeyConfig2()
{
    if (--keyConfig2Count <= 0)
        keyConfig2 = NULL;
}

/* Page "option" until it is left; every frame the page manager, then the
 * key configuration pages' input (player 2's first; player 1's page also
 * decides when both may be left), the screen saver.  The options are
 * copied in again at the start (begin did it too) and back to the game at
 * the end (GameOptions::operator=). */
// FUNC 0x0013FD08 gt2k_menu::OptionControl::mainLoop
void OptionControl::mainLoop()
{
    /* implicit: operator= (GameOptions assignment, here and at the end) */
    options = getGameStatus()->options;     /* GameOptions::operator= */
    pages.setPage("option", NULL);
    pages.startPage(NULL);
    while (pages.exited == 0) {
        pages.update();
        if (keyConfig2 != NULL)
            keyConfig2->handleInput();
        if (keyConfig1 != NULL) {
            keyConfig1->handleInput();
            keyConfig1->checkExit(keyConfig2);
        }
        updateScreenSaver(&render);
        waitFrame();
    }
    getGameStatus()->options = options;     /* GameOptions::operator= */
}

// FUNC 0x0013FE10 gt2k_menu::OptionControl::run
void OptionControl::run()
{
    load();
    begin();
    mainLoop();
    end();
    unload();
}

} /* namespace gt2k_menu */

using namespace gt2k_menu;

/* The options menu.  Frame 0x101700: the menu heap at sp+0, the
 * controller at sp+0x100000; destroyed with OptionControl::~OptionControl
 * called directly (flags 2; also by the landing pad).  Returns 1. */
// FUNC 0x0013FE58 F_0013FE58
int F_0013FE58(void)
{
    u8 heap[OPTION_HEAP_SIZE];              /* sp+0x000000 */

    menu_draw_reset();
    menu_heap_push(heap, OPTION_HEAP_SIZE);
    {
        OptionControl ctl;                  /* sp+0x100000 OptionControl::OptionControl */
        ctl.run();
        gfx_vsync(1);
    }
    menu_heap_pop();
    return 1;
}

namespace gt2k_menu {

// FUNC 0x0013FF10 gt2k_menu::OptionControl::getCalibWheel
int OptionControl::getCalibWheel()
{
    return calibWheel;
}

// FUNC 0x0013FF18 gt2k_menu::OptionControl::setCalibWheel
void OptionControl::setCalibWheel(int w)
{
    calibWheel = w;
}

// FUNC 0x0013FF20 gt2k_menu::OptionControl::getCalibPort
int OptionControl::getCalibPort()
{
    return calibPort;
}

// FUNC 0x0013FF28 gt2k_menu::OptionControl::setCalibPort
void OptionControl::setCalibPort(int p)
{
    calibPort = p;
}

// FUNC 0x0013FF30 gt2k_menu::OptionControl::getOptions
::GameOptions *OptionControl::getOptions()
{
    return &options;
}

// FUNC 0x0013FF38 gt2k_menu::OptionControl::getPages
PageManager *OptionControl::getPages()
{
    return &pages;
}

/* Player 2's MenuControl (ControlBase +0x134). */
// FUNC 0x0013FF40 gt2k_menu::OptionControl::getControl2
MenuControl *OptionControl::getControl2()
{
    return &control2;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base ControlBase). */
// FUNC 0x0013FF48 __tfQ29gt2k_menu13OptionControl
extern "C" void *__tfQ29gt2k_menu13OptionControl(void)
{
    if (__tiQ29gt2k_menu13OptionControl[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu13OptionControl, "Q29gt2k_menu13OptionControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu13OptionControl;
}
#endif
