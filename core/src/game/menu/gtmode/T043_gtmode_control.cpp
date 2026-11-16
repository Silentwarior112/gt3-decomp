/* T043 - gt2k_menu::GtmodeControl, the controller of the GT mode menu.
 * 0x00119778..0x0011A2B0.
 *
 * run() (gtmode_main, T038, on a stack object) loads "menu/gt_mode.mbl",
 * hands the GT mode tables to the PageManager (begin), then mainLoop
 * alternates between the root page, played over "stream/title_full.pss" and
 * left after the product database's idle time, and the other pages, until
 * one of them names the page the game goes to next; that name goes to
 * GameStatus::menu for main() (T000).  runTrade() runs the single page
 * "hm/hm_gr_trade" (the garage trade) the same way.
 *
 * Includes: retail's type names (.rodata 0x0030F8B8..0x0030FB08, reverse
 * definition order) are GtmodeControl, SmartIndexT<int>, List<Button>,
 * Cell<Button>, OperatorList, List<OperatorHolder>, OperatorHolder,
 * Cell<OperatorHolder>, the garage lists (GarageWheelList .. Cell<GarageCar>)
 * and Operator, without RegurationResult: so the GameStatus first, then the
 * menu item classes (T206: T216's operator classes, Button's base
 * Cell<Button>), List<Button>, and SmartIndexT<int> after it: ButtonList's
 * header (its List<Button> base, then its `index' member, which instantiates
 * SmartIndexT<int>: T251_smart_index_template.h), GtmodeControl last.  The
 * tables come from headers that add no class. */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (menu), MenuStatus, GarageCar */
#include "game/menu/framework/T206_button.h"               /* (Button, Cell<Button>, T216's operator classes) */
#include "game/menu/framework/menu_list.h"                 /* List<Button> */
#include "game/menu/framework/T244_button_list.h"          /* (seen: ButtonList: List<Button>, then SmartIndexT<int>) */
#include "game/menu/gtmode/T043_gtmode_control.h"       /* GtmodeControl; T247, T046 / T246 (TRANSITION_FADEOUT),
                                               T211 PageManager, T251 SmartIndexT */
#include "game/menu/gtmode/T036_garage.h"               /* MenuHook g_00296B90 / g_00351278 */
#include "game/menu/gtmode/T044_gtmode_menu_factories.h" /* MenuFuncData g_00295FB8 / g_00351088 */
#include "game/menu/gtmode/T037_gtmode_actions.h"       /* ExecuteFuncData g_0029A180 / g_00351294 */
#include "game/menu/gtmode/T045_gtmode_valid_table.h"   /* ValidFuncData g_00296750 / g_00351100 */
#include "game/menu/gtmode/T049_page_button_checks.h"   /* SelectableFuncData g_002959E8 / g_00351040 */
#include "game/menu/gtmode/T048_page_cursor_hooks.h"    /* FocusFuncData g_00295380 / g_00350FF4 */
#include "game/app/T002_render_thread.h"        /* render_set_object */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_0035274C, g_00352750, MENU_SCREEN_W / H */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/app/T000_main.h"                 /* MovieParam */
#include "game/common/T494_text_utils.h"           /* str_equal, str_has_suffix, str_cut_last_underscore */
#include "game/data/T435_product_config.h"       /* product_config, idleSeconds */
#include "engine/io/T507_async_loader.h"         /* async_loader_create / destroy */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync, gs_get_field_mode, PGL_FIELD_HALF_ODD */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "game/common/T017_smart_index.h"          /* __tfQ29gt2k_menut11SmartIndexT1Zi, its storage */
#include "game/menu/gtmode/gtmode_control_internal.h"

namespace gt2k_menu {

/* infoMode's SmartIndexT<int> constructor is expanded (value 0, begin 0,
 * end 1, clamp 0, wrapped 0, the vptr 0x0030D998); prevPower and unk_E40 are
 * not set.  setClamp(false) in the body: flow deletes the constructor's
 * clamp store (the inline repeats it through the same register).  98.6%
 * since T251's `wrapped' is a bool: bool requests and int mode / value then
 * tie in sched1 (2 dependents each), luid order wins (u009 notes, Revisit 2). */
// FUNC 0x00119778 gt2k_menu::GtmodeControl::GtmodeControl
GtmodeControl::GtmodeControl()
    : ControlBase(), render(), pm(this),
      rootRequest(false), mode(ROOT_ENTRY_DEFAULT), mapRequest(false), sortType(GARAGE_SORT_DATE),
      infoMode(), oilCylinder(0)
{
    /* (render: GtmodeRender(); pm: PageManager(this).  The landing pads
     * destroy render, RootObject::~RootObject of render.root with flags 2
     * then GameObject::~GameObject, and the ControlBase.) */
    /* implicit: GtmodeRender, PageManager (member constructors), ~RootObject, ~GameObject (member destructors, exception cleanup) */
    infoMode.setClamp(false);               /* the info column wraps around */
    infoMode.setRange(0, GARAGE_INFO_MODES);
    infoMode.set(GARAGE_INFO_MODE_DEFAULT);
    pageManager = &pm;
}

/* The debug pause of the second pad on render, then ControlBase's frame. */
// FUNC 0x00119890 gt2k_menu::GtmodeControl::waitFrame
void GtmodeControl::waitFrame()
{
    debugPause(&render);
    ControlBase::waitFrame();
}

// FUNC 0x001198C0 gt2k_menu::GtmodeControl::loadPages
void GtmodeControl::loadPages()
{
    pm.loadFile("menu/gt_mode.mbl");
}

// FUNC 0x001198E8 gt2k_menu::GtmodeControl::unloadPages
void GtmodeControl::unloadPages()
{
    pm.unloadFile();
}

/* Starts the session: the pads, a frame (the virtual waitFrame), no screen
 * clear, render's transition on (type 0, the fade-out), the PageManager on
 * render's tree and the menu status's cursor memory, the hook table on both
 * layouts and the five FuncData tables, each with this as its context.  The
 * ridden car is given to setRunningCar again.  (The inline setters of
 * RenderBase / PageManager: retail stores through &render and &pm, and in
 * this order of the tables.) */
// FUNC 0x00119908 gt2k_menu::GtmodeControl::begin
void GtmodeControl::begin()
{
    attach();
    waitFrame();
    g_00352750 = 0;
    render.setTransition(true, TRANSITION_FADEOUT);
    g_0035274C = 0;
    pm.initialize(&render.root, &getGameStatus()->menu.cursor);
    pm.setHooks(g_00296B90, g_00351278, this);
    pm.setMenuFuncData(g_00295FB8, g_00351088, this);        /* T044, the menu_* factories */
    pm.setExecuteFuncData(g_0029A180, g_00351294, this);     /* T037 */
    pm.setValidFuncData(g_00296750, g_00351100, this);       /* T045, the checks */
    pm.setSelectableFuncData(g_002959E8, g_00351040, this);  /* T049 */
    pm.setFocusFuncData(g_00295380, g_00350FF4, this);       /* T048 */

    GarageCar *car = getGameStatus()->getRunningCar();
    if (car != NULL)
        getGameStatus()->setRunningCar(car);
}

// FUNC 0x00119A70 gt2k_menu::GtmodeControl::end
void GtmodeControl::end()
{
    pm.finalize();
    gfx_vsync(2);
    render.end();                           /* (u013's F_001273C0) */
    detach();
}

/* Runs page until the PageManager exits; a page that calls requestRoot
 * leaves it for the root page (mainLoop reads rootRequest). */
// FUNC 0x00119AB0 gt2k_menu::GtmodeControl::runPage
void GtmodeControl::runPage(const char *page)
{
    pm.setPage(page, NULL);
    pm.startPage(NULL);
    render_set_object(&render);
    rootRequest = false;
    while (pm.exited == 0) {
        pm.update();
        updateScreenSaver(&render);
        waitFrame();
    }
    render_set_object(NULL);
    pm.closeAndExit();
}

/* Runs the root page over the title movie.  While "root" is the current
 * page the frames without input are counted; past the product database's
 * idle time (seconds * 60) the PageManager is asked to exit (doExit).  A
 * page can ask for the map (requestMap).  Returns pm.decided.  pageArg
 * (MenuStatus::pageArg) is not used.  (doExit, setRenderEnable and isDecided
 * are the inline members: retail stores through a copy of &pm and through
 * &render.) */
// FUNC 0x00119B60 gt2k_menu::GtmodeControl::runRootPage
int GtmodeControl::runRootPage(const char *page, int pageArg)
{
    MovieParam movie;
    int idleFrames;
    int decided;

    g_002CDC10.suspend(0);
    idleFrames = 0;
    g_002CDC10.open(MENU_SCREEN_W, MENU_SCREEN_H);
    movie.file      = "stream/title_full.pss";
    movie.width     = MENU_SCREEN_W;
    movie.height    = MENU_SCREEN_H;
    movie.field     = 1;
    movie.withSound = 0;
    movie.oneShot   = 0;
    g_002CDC10.start(&movie);
    pm.setPage(page, NULL);
    pm.startPage(NULL);
    render_set_object(&render);
    mapRequest = false;
    while (pm.exited == 0) {
        if (pm.update() != 0)
            idleFrames = 0;
        if (str_equal(pm.currentPage, "root") != 0) {
            idleFrames++;
            if (product_config()->idleSeconds() * FRAMES_PER_SECOND < idleFrames)
                pm.doExit();
        }
        updateScreenSaver(&render);
        waitFrame();
    }
    if (gs_get_field_mode() == PGL_FIELD_HALF_ODD)
        gfx_vsync(1);
    render.setRenderEnable(false);
    gfx_vsync(1);
    g_002CDC10.stopAndWait();
    g_002CDC10.close();
    render_set_object(NULL);
    render.setRenderEnable(true);
    decided = pm.isDecided();
    pm.closeAndExit();
    return decided;
}

/* The menu session.  The start page is MenuStatus::getPage ("root" if
 * none): "root" and "replay_theater" start on the root page (state 0), the
 * others on that page (state 1).  Before each root page, mode says how it
 * was entered: ROOT_ENTRY_FROM_PAGE after a page asked for it, else from
 * the mode main() came back from (MenuStatus::nextMode, then cleared).  The
 * loop ends when a page names where to go (getReturnPage): after a page
 * other than the root, a name ending in "_entry" is cut at its last '_' when
 * nextMode is a championship / GT mode race; it goes to
 * MenuStatus::setPage(name, -1).  Returns the last runRootPage (1 if none
 * ran).
 * Matching (97.4%, the same under the reference compiler 991111-01): only
 * register numbers differ.  Retail's first reload (the high part of "root",
 * which PRE shares with case 1's strcpy, rematerialised into $v0) comes one
 * reload earlier than ours, so its round-robin reload registers ($v0 / $v1 /
 * $a0: the switch's 1, `mode' 1 .. 4, `done' 1) are ours rotated by one.
 * Tried: six forms of the start state test, `getMenuStatus()` for every
 * MenuStatus access (made the rest exact). */
// FUNC 0x00119D38 gt2k_menu::GtmodeControl::mainLoop
int GtmodeControl::mainLoop()
{
    char page[0x40];        /* sp+0x00 the page to run */
    char next[0x40];        /* sp+0x40 the page to go to */
    char cut[0x40];         /* sp+0x80 */
    int  done;              /* sp+0xC0 */
    int  state;             /* 0 the root page, 1 another page */
    int  rootDecided;

    async_loader_create(0xA8000);
    getGameStatus()->getMenuStatus()->getPage(page);
    if (str_equal(page, "root") || str_equal(page, "replay_theater"))
        state = 0;
    else
        state = 1;
    done = 0;
    rootDecided = 1;
    do {
        switch (state) {
        case 0:
            if (rootDecided == 0) {
                mode = ROOT_ENTRY_FROM_PAGE;
            } else {
                int from = getGameStatus()->getMenuStatus()->nextMode;
                if (from == MAIN_MODE_ARCADE)
                    mode = ROOT_ENTRY_DEFAULT;
                else if (from == MAIN_MODE_CARD_REPLAY)
                    mode = ROOT_ENTRY_CARD_REPLAY;
                else if (from == MAIN_MODE_GAME_LOAD)
                    mode = ROOT_ENTRY_GAME_LOAD;
                else if (from == MAIN_MODE_GAME_SAVE)
                    mode = ROOT_ENTRY_GAME_SAVE;
                else if (from == MAIN_MODE_FILE_UTILITY)
                    mode = ROOT_ENTRY_FILE_UTILITY;
                else if (from == MAIN_MODE_OPTIONS)
                    mode = ROOT_ENTRY_OPTIONS;
                else
                    mode = ROOT_ENTRY_DEFAULT;
            }
            getGameStatus()->getMenuStatus()->nextMode = 0;
            rootDecided = runRootPage(page, getGameStatus()->getMenuStatus()->pageArg);
            if (mapRequest) {
                state = 1;
                strcpy(page, "map");
            } else {
                pm.getReturnPage(next);
                done = 1;
                getGameStatus()->getMenuStatus()->setPage(next, -1);
            }
            break;
        case 1:
            runPage(page);
            if (rootRequest) {
                state = 0;
                rootDecided = 0;
                strcpy(page, "root");
            } else {
                pm.getReturnPage(next);
                done = 1;
                if (getGameStatus()->getMenuStatus()->nextMode == MAIN_MODE_CHAMPIONSHIP ||
                    getGameStatus()->getMenuStatus()->nextMode == MAIN_MODE_GTMODE_RACE ||
                    getGameStatus()->getMenuStatus()->nextMode == MAIN_MODE_GTMODE_RACE2) {
                    if (str_has_suffix("_entry", next) != 0) {
                        strcpy(cut, next);
                        str_cut_last_underscore(next, cut);
                    }
                }
                getGameStatus()->getMenuStatus()->setPage(next, -1);
            }
            break;
        }
    } while (done == 0);
    async_loader_destroy();
    return rootDecided;
}

// FUNC 0x0011A040 gt2k_menu::GtmodeControl::run
int GtmodeControl::run()
{
    loadPages();
    begin();
    int decided = mainLoop();
    end();
    unloadPages();
    return decided;
}

/* The garage trade page; the game then goes to "hm/hm" (arg 2). */
// FUNC 0x0011A098 gt2k_menu::GtmodeControl::runTrade
void GtmodeControl::runTrade()
{
    loadPages();
    begin();
    async_loader_create(0);
    runPage("hm/hm_gr_trade");
    getGameStatus()->menu.setPage("hm/hm", 2);
    async_loader_destroy();
    end();
    unloadPages();
}

/* The accessors: inline (no callers in the binary). */
// FUNC 0x0011A118 gt2k_menu::GtmodeControl::getOilCylinder
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A120 gt2k_menu::GtmodeControl::setOilCylinder
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A128 gt2k_menu::GtmodeControl::getRoot
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A130 gt2k_menu::GtmodeControl::getRender
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A138 gt2k_menu::GtmodeControl::getPrevPower
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A140 gt2k_menu::GtmodeControl::setPrevPower
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A148 gt2k_menu::GtmodeControl::getInfoMode
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A150 gt2k_menu::GtmodeControl::setInfoMode
/* inline in game/menu/gtmode/T043_gtmode_control.h (infoMode.set(v)); egcs emits this copy */
/* implicit: set (infoMode.set in the inline setInfoMode) */

// FUNC 0x0011A170 gt2k_menu::GtmodeControl::getSortType
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A178 gt2k_menu::GtmodeControl::setSortType
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A180 gt2k_menu::GtmodeControl::getMode
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A188 gt2k_menu::GtmodeControl::setMode
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A190 gt2k_menu::GtmodeControl::requestMap
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A1A0 gt2k_menu::GtmodeControl::requestRoot
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A1B0 gt2k_menu::GtmodeControl::getPageManager
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

// FUNC 0x0011A1B8 gt2k_menu::GtmodeControl::getButtonList
/* inline in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Builds the __si_type_info __tiQ29gt2k_menu13GtmodeControl (base
 * ControlBase: __tfQ29gt2k_menu11ControlBase first) on the first call. */
// FUNC 0x0011A1D0 __tfQ29gt2k_menu13GtmodeControl
extern "C" void *__tfQ29gt2k_menu13GtmodeControl(void)
{
    if (__tiQ29gt2k_menu13GtmodeControl[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu13GtmodeControl, "Q29gt2k_menu13GtmodeControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu13GtmodeControl;
}
#endif

// FUNC 0x0011A220 gt2k_menu::GtmodeControl::~GtmodeControl
/* implicit in game/menu/gtmode/T043_gtmode_control.h; egcs emits this copy for the
 * vtable: no vptr store; pm and infoMode have trivial destructors; render is
 * destroyed inline (gt2k_menu::RootObject::~RootObject of render.root at
 * +0x290 with flags 2, then GameObject::~GameObject at +0x28C with flags 2),
 * then gt2k_menu::ControlBase::~ControlBase with this destructor's flags
 * (ControlBase does the operator delete). */
/* implicit: ~RootObject, ~GameObject (member destructors of render), ~ControlBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* T043's copy of the typeinfo function of SmartIndexT<int> (no callers;
 * T017's 0x00103690 is the one called).  Weak so that the copies link. */
// FUNC 0x0011A270 __tfQ29gt2k_menut11SmartIndexT1Zi
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut11SmartIndexT1Zi(void)
{
    if (__tiQ29gt2k_menut11SmartIndexT1Zi[0] == 0)
        __rtti_user(__tiQ29gt2k_menut11SmartIndexT1Zi, "Q29gt2k_menut11SmartIndexT1Zi");
    return __tiQ29gt2k_menut11SmartIndexT1Zi;
}
#endif
