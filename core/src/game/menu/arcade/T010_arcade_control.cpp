/* T010 - gt2k_menu::ArcadeControl, the controller of the arcade-mode menu.
 * 0x00113A38..0x00115618.
 *
 * mode (ArcadeControl+0x3A2C) is the entry chosen on "ar/top": SINGLE_RACE,
 * COMPULSORY, TIME_ATTACK, TWO_PLAYER, GOODIES, LOAD_GUEST_GARAGE, I_LINK.
 * Each mode is a small state machine over page functions (the states are
 * the pages, ArcadeState below); a page returns 0 to go back one state.
 * finish() writes the choices to the caller's MenuArgs and the GameStatus.
 * Page functions take the previous page's result and mostly ignore it.
 *
 * Retail's .rodata (0x0030D7F0..0x0030DC10): "bad_alloc" (<new>'s inline
 * what(), read first), the literals, the jump tables and the tables'
 * strings, the vtables of ArcadeControl and SmartIndexT<int>, then the type
 * names ArcadeControl, SmartIndexT<int>, UnitArena<64>, RegurationResult,
 * RecordsMenuArg, the operator classes, the garage lists, Operator: egcs
 * writes them in the reverse order of the classes' definitions, so the
 * GameStatus comes first, then the menu rectangle (the operator classes),
 * the records menu args, the regulation result, the race course (its arena
 * template) and T010_arcade_control.h last.  Its .sdata (0x00350F70..
 * 0x00350FC8): the tables' strings of 8 bytes or less, then g_00350FC0 /
 * g_00350FC4; the strings are referenced only by the tables, hence the
 * anchors. */
// DATA_START 0x0030D7F0
// DATA_START 0x00350F70
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus; T036 GarageCar */
#include "game/menu/framework/T256_menu_rectangle.h"       /* Rectangle (the fade rectangle) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: retail's T010 has RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "libc.h"                           /* free */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_si, __rtti_user (GCC 14's typeinfo functions;
                                               egcs's <new> declares some of its names otherwise) */
#endif
#include "game/app/T000_main.h"                 /* MenuArgs */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder (the guest garage) */
#include "game/menu/arcade/T029_arcade_movies.h"        /* MovieParam g_00292930[8] */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current (pdistd::Allocator) */
#include "game/menu/framework/T220_object.h"               /* Object::attachTo, RootObject */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play_if_changed */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus */
#include "game/data/T435_product_config.h"       /* product_config, ProductConfig */
#include "game/sound/T463_music_seq.h"            /* music_load / music_unload */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "engine/system/T473_link_session.h"         /* g_00352CB0, g_00352CDC */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getCourseKind */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/framework/T016_ending_menu.h"          /* g_002902E0 / g_00350B60, g_00350B68 / g_00350B70 */
#include "game/menu/arcade/T019_page_hooks.h"           /* g_00290940 / g_00350C08 */
#define T027_TABLE_ONLY
#include "game/menu/arcade/T027_level_button_list.h"    /* g_002925A0 / g_00350E88 (only the table: retail's
                                               T010 has no List<Button>) */
#undef T027_TABLE_ONLY
#include "game/menu/arcade/T028_arcade_menu_hooks.h"    /* g_002927B8 / g_00350EA0 */
#include "game/menu/arcade/T030_arcade_item_factories.h"    /* MenuHook g_00292A18 / g_00350F48 */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout::setHooks */
#include "game/menu/arcade/T010_arcade_control.h"       /* last: ArcadeControl, SmartIndexT<int> */
#include "game/menu/arcade/arcade_control_internal.h"

namespace gt2k_menu {

/* The states of the mode flows: the page each runs. */
enum ArcadeState {
    ARCADE_STATE_LEVEL      = 2,            /* runLevelPage "ar/level" */
    ARCADE_STATE_CONNECT    = 4,            /* runConnectPage + runWaitOtherPage (T013) */
    ARCADE_STATE_CAR        = 5,            /* runCarSelect "ar/select_car" (T023), carSelect2P
                                               "ar/select_car_2p" (T021) */
    ARCADE_STATE_COURSE     = 6,            /* selectCourse "ar/select_course" (T025) */
    ARCADE_STATE_CLASS      = 7,            /* classSelect "ar/select_class" (T022) */
    ARCADE_STATE_COMPULSORY = 8,            /* selectCompulsory "ar/select_compulsory" (T018) */
    ARCADE_STATE_EXCHANGE   = 10            /* runWaitSyncPage, the i.LINK selection exchange (T014) */
};

/* The members are built in order: ControlBase, the vptr, render
 * (RenderBase::RenderBase, then the ArcadeRender vptr), pages
 * (PageManager::PageManager(this)), result, the two MenuCarSpecs
 * (MenuCarSpec::MenuCarSpec), the 13 indices (value 0, [0, 1), wrapping; the
 * T017 constructors are expanded: owner this, player2 0 / 1), courseIpic,
 * ipic.  The body then clears the words 0x3A30..0x3BD8, mode ARCADE_NONE.
 * carBuffer1 / carBuffer2 are left as they are. */
// FUNC 0x00113A38 gt2k_menu::ArcadeControl::ArcadeControl
ArcadeControl::ArcadeControl(MenuArgs *res)
    : ControlBase(),
      render(),                             /* RenderBase::RenderBase, then the ArcadeRender vptr */
      pages(this),                          /* PageManager::PageManager */
      result(res),
      car1Spec(),                           /* MenuCarSpec::MenuCarSpec */
      car2Spec(),
      course(this),
      class1(),
      car1(this, 0),
      color1(),
      racing1(),
      transmission1(),
      opt1d(),
      class2(),
      car2(this, 1),
      color2(),
      racing2(),
      transmission2(),
      opt2d(),
      courseIpic(NULL),
      ipic()
{
    /* implicit: RenderBase, PageManager, MenuCarSpec, Ipic (member constructors), ~CarEquipments, ~RootObject, ~GameObject (member destructors, exception cleanup) */
    level = 0;
    mode = ARCADE_NONE;
    fade = NULL;
    disabledMark = NULL;
    arrowL = NULL;
    arrowR = NULL;
    arrow = NULL;
    logo = NULL;
    doc = NULL;
    unk3A50 = 0;
    historyLevelItem = NULL;
    historyAreaItem = NULL;
    historyClassItem = NULL;
    historyLevel[0] = NULL;
    historyLevel[1] = NULL;
    historyLevel[2] = NULL;
    historyLevel[3] = NULL;
    historyArea[0] = NULL;
    historyArea[1] = NULL;
    historyArea[2] = NULL;
    historyArea[3] = NULL;
    historyArea[4] = NULL;
    historyArea[5] = NULL;
    historyClass[0] = NULL;
    historyClass[1] = NULL;
    historyClass[2] = NULL;
    historyClass[3] = NULL;
    historyClass[4] = NULL;
    historyClass[5] = NULL;
    historyClass[6] = NULL;
    mpegCourse = NULL;
    courseBar = NULL;
    miniCourses = NULL;
    totalLength = NULL;
    courseName = NULL;
    length = NULL;
    area = NULL;
    surface = NULL;
    mpegClass = NULL;
    classBar = NULL;
    miniClasses = NULL;
    crossBar = NULL;
    crossBarY = 0.0f;
    carItem = NULL;
    carView = NULL;
    miniCars = NULL;
    unk3AE4 = 0;
    colorSelection = NULL;
    colorChips = NULL;
    driveTrain = NULL;
    colorName = NULL;
    powerText = NULL;
    torqueText = NULL;
    handlingText = NULL;
    speedText = NULL;
    accelText = NULL;
    handlingGauge = NULL;
    speedGauge = NULL;
    accelGauge = NULL;
    specPanel = NULL;
    autoNo = NULL;
    autoYes = NULL;
    manuNo = NULL;
    manuYes = NULL;
    driNo = NULL;
    driYes = NULL;
    racingNo = NULL;
    racingYes = NULL;
    blur = NULL;
    miniClasses2 = NULL;
    carItem2 = NULL;
    carView2 = NULL;
    miniCars2 = NULL;
    logo2 = NULL;
    unk3B54 = 0;
    colorChips2 = NULL;
    driveTrain2 = NULL;
    powerText2 = NULL;
    torqueText2 = NULL;
    handlingText2 = NULL;
    speedText2 = NULL;
    accelText2 = NULL;
    handlingGauge2 = NULL;
    speedGauge2 = NULL;
    accelGauge2 = NULL;
    autoNo2 = NULL;
    autoYes2 = NULL;
    manuNo2 = NULL;
    manuYes2 = NULL;
    driNo2 = NULL;
    driYes2 = NULL;
    racingNo2 = NULL;
    racingYes2 = NULL;
    bg = NULL;
    power = NULL;
    powerValue = NULL;
    weight = NULL;
    weightValue = NULL;
    tire = NULL;
    tire01 = NULL;
    record = NULL;
    first = NULL;
    firstTime = NULL;
    second = NULL;
    secondTime = NULL;
    third = NULL;
    thirdTime = NULL;
    preview = 0;
}

/* The vptr, then the members in reverse: ~Ipic, the two MenuCarSpecs (their
 * CarEquipments::~CarEquipments, car2Spec's +0x36D0 first), the implicit
 * ~ArcadeRender expanded (RootObject::~RootObject of render.root,
 * GameObject::~GameObject), then ControlBase::~ControlBase(this, flags),
 * which does the delete. */
// FUNC 0x00113EA8 gt2k_menu::ArcadeControl::~ArcadeControl
ArcadeControl::~ArcadeControl()
{
    /* implicit member destruction: Ipic::~Ipic, CarEquipments::~CarEquipments
     * (car2Spec, car1Spec), RootObject::~RootObject and GameObject::~GameObject
     * (render), then ControlBase's */
    /* implicit: ~Ipic, ~CarEquipments, ~RootObject, ~GameObject (member destructors), ~ControlBase (base destructor) */
}

/* One frame: a dropped link (g_00352CDC, the bus reset flag of T473) times
 * an I_LINK run out; then the debug pause of pad 2 over the renderer and
 * ControlBase's frame (vsync, idle count). */
// FUNC 0x00113F38 gt2k_menu::ArcadeControl::waitFrame
void ArcadeControl::waitFrame()
{
    if (mode == ARCADE_I_LINK && g_00352CDC != 0)
        timedOut = 1;
    debugPause(&render);
    ControlBase::waitFrame();
}

/* The two Car model buffers (0xA8000 bytes each, 64-aligned, from the menu
 * heap), the menu file, the music and the course stills. */
// FUNC 0x00113F90 gt2k_menu::ArcadeControl::load
void ArcadeControl::load()
{
    carBuffer1 = menu_heap_current()->allocate(0xA8000, 0x40);
    carBuffer2 = menu_heap_current()->allocate(0xA8000, 0x40);
    pages.loadFile("menu/arcade.mbl");
    music_load();
    courseIpic = (IpicArchiveData *)file_read_blocking("stream/course.ipic", NULL, 0, 1,
                                                       &vol_device_object);
}

// FUNC 0x00114028 gt2k_menu::ArcadeControl::unload
void ArcadeControl::unload()
{
    free(courseIpic);
    courseIpic = NULL;
    music_unload();
    pages.unloadFile();
    menu_heap_current()->free(carBuffer2);
    carBuffer2 = NULL;
    menu_heap_current()->free(carBuffer1);
    carBuffer1 = NULL;
}

/* Before a run: the pads, the background movie (640 x 448, menu_bg2.pss
 * preloaded), the mode from the GameStatus menu state (negative:
 * SINGLE_RACE), the page manager on the renderer's root (no cursor memory,
 * no sound) with the arcade item factories on both layouts and its five
 * FuncData tables (all with this as context), the renderer on the render
 * thread with the fade-out transition, the ranges of the 13 indices and the
 * fade rectangle (at -10, -10, z 99.5, size 0; transitionIn / Out run their
 * scripts on it) under the renderer's root.
 * The inline accessors are expanded here (no calls in the listing):
 * RenderBase::getRootObject, PageManager::setHooks (both layouts) and the
 * set*FuncData, RenderBase::setTransition, MotionValue::set. */
/* An index over [0, end) at 0 that stops at the ends.  Retail keeps each
 * index's address in a register (or a spill slot) for the two calls and the
 * clamp store: the original passed it to a helper (with the members named
 * directly the store folds into this + offset). */
static inline void initIndex(SmartIndexT<int> *idx, int end)
{
    idx->setRange(0, end);
    idx->set(0);
    idx->clamp = 1;
}

// FUNC 0x00114090 gt2k_menu::ArcadeControl::setup
void ArcadeControl::setup()
{
    attach();
    g_002CDC10.open(640, 448);
    g_002CDC10.preload(g_00292930[ARCADE_MOVIE_BG2].file);

    int lastMode = getGameStatus()->getMenuStatus()->arcadeMode;
    if (lastMode >= 0)
        mode = lastMode;
    else
        mode = ARCADE_SINGLE_RACE;

    PageManager *pm = &pages;
    RootObject *root = render.getRootObject();
    pm->initialize(root, NULL);
    pm->disableSound(true);
    pages.setHooks(g_00292A18, g_00350F48, this);
    pm->setExecuteFuncData(g_002927B8, g_00350EA0, this);       /* the buttons' exec names (T028) */
    pm->setMenuFuncData(g_002925A0, g_00350E88, this);          /* "menu_goodies", "menu_level" (T027) */
    pm->setFocusFuncData(g_00290940, g_00350C08, this);         /* T019 */
    pm->setSelectableFuncData(g_002902E0, g_00350B60, this);    /* "ar/select_ending" (T016) */
    pm->setValidFuncData(g_00350B68, g_00350B70, this);         /* T016 */

    render.attach();
    render.setTransition(true, TRANSITION_FADEOUT);

    initIndex(&course, 1);
    initIndex(&class1, 1);
    car1.setRange(0, 1);
    car1.set(0);
    initIndex(&color1, 1);
    initIndex(&racing1, 2);
    initIndex(&transmission1, 2);
    initIndex(&opt1d, 2);
    initIndex(&class2, 1);
    car2.setRange(0, 1);
    car2.set(0);
    initIndex(&color2, 1);
    initIndex(&racing2, 2);
    initIndex(&transmission2, 2);
    initIndex(&opt2d, 2);

    /* Object::operator new (the menu heap, 0xCC bytes), Transitive(), the
     * inline Rectangle(); Object::operator delete if the constructor
     * throws. */
    /* implicit: operator new, Transitive, operator delete (new-expression, inline Rectangle constructor) */
    fade = new Rectangle;
    Rectangle *rect = fade;
    rect->x.set(-10.0f);
    rect->drawFlags |= 1;                   /* own draw settings */
    rect->y.set(-10.0f);
    rect->z = 99.5f;
    rect->w = 0.0f;
    rect->h = 0.0f;
    rect->attachTo(root);
}

/* MenuArgs::selection for the caller (main_arcade, T000): 1 single race,
 * 2 compulsory, 3 time attack, 4 two players, 5 i.LINK, else 0.  (Jump
 * table 0x0030D830, 8 entries.) */
// FUNC 0x00114500 gt2k_menu::ArcadeControl::getSelection
int ArcadeControl::getSelection()
{
    int selection = 0;
    switch (mode) {
    case ARCADE_SINGLE_RACE:
        selection = 1;
        break;
    case ARCADE_COMPULSORY:
        selection = 2;
        break;
    case ARCADE_TIME_ATTACK:
        selection = 3;
        break;
    case ARCADE_TWO_PLAYER:
        selection = 4;
        break;
    case ARCADE_I_LINK:
        selection = 5;
        break;
    case ARCADE_GOODIES:
    case ARCADE_LOAD_GUEST_GARAGE:
    case ARCADE_NONE:
        break;
    }
    return selection;
}

/* Whether the selected course (course of the mode's list, ProductConfig) is
 * a dirt course: kind 2 in a stack RaceDataBase (RaceDataBase::RaceDataBase
 * / ~RaceDataBase around it).  The i.LINK flow then preselects
 * CLASS_DIRT. */
// FUNC 0x00114558 gt2k_menu::ArcadeControl::isDirtCourse
bool ArcadeControl::isDirtCourse()
{
    ProductConfig *config = product_config();
    int index = course.value;
    const char *list = getModeName();
    const char *name = config->courseName(index, list);
    RaceDataBase db;
    bool dirt = db.getCourseKind(nameToCode(name)) == 2;
    return dirt;
}

/* After a run.  If it was decided (ok) in a racing mode: player 1's car
 * (by code, or a home / guest garage car) goes to the GameStatus, and the
 * car, course, raceCode() and the options to *result; in TWO_PLAYER also
 * player 2's car (to the GameStatus menu state) and options.  Always: the
 * mode is remembered in the GameStatus menu state, and the pages, the
 * renderer, the BGM, the background movie and the pads are shut down. */
// FUNC 0x00114610 gt2k_menu::ArcadeControl::finish
void ArcadeControl::finish(int ok)
{
    if (ok != 0 && mode != ARCADE_LOAD_GUEST_GARAGE && mode != ARCADE_GOODIES) {
        const char *className = getPlayerClassName(0);
        const char *carCode1 = NULL;
        if (className != NULL)
            carCode1 = product_config()->carCode(car1.value, className);

        const char *courseCode = NULL;
        if (mode != ARCADE_COMPULSORY) {
            ProductConfig *config = product_config();
            int index = course.value;
            courseCode = config->courseName(index, getModeName());
        }

        if ((unsigned)mode < (unsigned)ARCADE_GOODIES) {  /* the four racing modes (sltiu) */
            int cls = class1.value;
            if (cls == ARCADE_CLASS_HOME_GARAGE) {
                gt2k_game::GameStatus *status = getGameStatus();
                GarageCar *car = status->getCar(car1.value);
                getGameStatus()->setRunningCar(car);
            } else if (cls == ARCADE_CLASS_GUEST_GARAGE) {
                GarageCar *car = getGameStatusHolder()->get()->getCar(car1.value);
                getGameStatus()->setRunningCarCopy(car);
            } else {
                getGameStatus()->setRunningCarByCode(carCode1);
            }
        }

        const char *race = raceCode();
        int tyre = (racing1.value == 1);
        int manual = (transmission1.value == 1);
        int optD = (opt1d.value == 0);
        int color = color1.value;
        int selection = getSelection();
        result->selection     = selection;
        result->car1          = carCode1;
        result->color1        = color;
        result->tyre1         = tyre;
        result->transmission1 = manual;
        result->opt1d         = optD;
        result->course        = courseCode;
        result->race          = race;
        getGameStatus()->getMenuStatus()->setEvent(race);

        if (mode == ARCADE_TWO_PLAYER) {
            const char *className2 = getPlayerClassName(2);
            const char *carCode2 = NULL;
            if (className2 != NULL)
                carCode2 = product_config()->carCode(car2.value, className2);
            int cls2 = class2.value;
            if (cls2 == ARCADE_CLASS_HOME_GARAGE) {
                GarageCar *car = getGameStatus()->getCar(car2.value);
                getGameStatus()->getMenuStatus()->setRaceCar(car, 0);
            } else if (cls2 == ARCADE_CLASS_GUEST_GARAGE) {
                GarageCar *car = getGameStatusHolder()->get()->getCar(car2.value);
                getGameStatus()->getMenuStatus()->setRaceCarCopy(car, 0);
            } else {
                getGameStatus()->getMenuStatus()->setRaceCarByCode(carCode2, 0);
            }
            int tyre2 = (racing2.value == 1);
            int manual2 = (transmission2.value == 1);
            int optD2 = (opt2d.value == 0);
            int color2v = color2.value;
            result->car2          = carCode2;
            result->color2        = color2v;
            result->tyre2         = tyre2;
            result->transmission2 = manual2;
            result->opt2d         = optD2;
        }
    }

    gt2k_game::GameStatus *status = getGameStatus();
    status->getMenuStatus()->arcadeMode = mode;
    pages.finalize();                       /* ($a1 still holds mode: not an argument) */
    render.detach();
    bgm_stop();
    g_002CDC10.stopAndWait();
    g_002CDC10.unload();
    g_002CDC10.close();
    detach();
    gfx_vsync(2);
}

} /* namespace gt2k_menu */

/* T010 .data (0x002944A8..0x00294514, then the frame block) and .sdata
 * (0x00350F70..0x00350FC8; g_00350FC8 / g_00350FCC after it are T013's).
 * Defined here, between finish and getModeName:
 * egcs writes an initialised variable's strings to .rodata where it is
 * defined, and retail's (0x0030D850..0x0030D938) lie between getSelection's
 * jump table and runILink's; the strings of 8 bytes or less are .sdata
 * (-G8), as retail's 0x00350F70..0x00350FB8. */
/* The ProductConfig course lists of the modes (getModeName). */
const char *g_002944A8[8] = {
    "SINGLE_RACE", "COMPULSORY", "TIME_ATTACK", "TWO_PLAYER", "GOODIES", "LOAD_GUEST_GARAGE",
    "I_LINK", NULL
};
/* The ProductConfig car lists of the classes (getClassName; NULL for the
 * garages).  0x002944EC, the word after it, is 0 (alignment). */
const char *g_002944C8[9] = {
    "CLASS_C", "CLASS_B", "CLASS_A", "CLASS_S", "CLASS_DIRT", NULL, NULL, "CLASS_COMPULSORY",
    "CLASS_PREVIEW"
};
/* The layout names of the classes (getClassLabel). */
const char *g_002944F0[9] = {
    "class_c", "class_b", "class_a", "class_s", "class_dirt", "home_garage", "guest_garage",
    "class_compulsory", "class_preview"
};
bool g_00350FC0 = true;                     /* 0x00350FC0 until the first i.LINK connect */
int g_00350FC4 = 0;                         /* 0x00350FC4 back into the i.LINK flow */

namespace gt2k_menu {

/* "SINGLE_RACE", "COMPULSORY", "TIME_ATTACK", "TWO_PLAYER", "GOODIES",
 * "LOAD_GUEST_GARAGE", "I_LINK" (g_002944A8[7] is NULL): the ProductConfig
 * course lists of the modes. */
// FUNC 0x00114988 gt2k_menu::ArcadeControl::getModeName
const char *ArcadeControl::getModeName()
{
    return g_002944A8[mode];
}

/* "CLASS_C", "CLASS_B", "CLASS_A", "CLASS_S", "CLASS_DIRT", NULL, NULL,
 * "CLASS_COMPULSORY", "CLASS_PREVIEW": the ProductConfig car lists (NULL
 * for the garages). */
// FUNC 0x001149A8 gt2k_menu::ArcadeControl::getClassName
const char *ArcadeControl::getClassName(int cls)
{
    return g_002944C8[cls];
}

/* No callers.  player 2: player 2's class, anything else player 1's. */
// FUNC 0x001149C0 gt2k_menu::ArcadeControl::getPlayerClassLabel
const char *ArcadeControl::getPlayerClassLabel(int player)
{
    return getClassLabel(player != 2 ? class1.value : class2.value);
}

/* "class_c", "class_b", "class_a", "class_s", "class_dirt", "home_garage",
 * "guest_garage", "class_compulsory", "class_preview": the layout names. */
// FUNC 0x001149F0 gt2k_menu::ArcadeControl::getClassLabel
const char *ArcadeControl::getClassLabel(int cls)
{
    return g_002944F0[cls];
}

// FUNC 0x00114A08 gt2k_menu::ArcadeControl::getPlayerClassName
const char *ArcadeControl::getPlayerClassName(int player)
{
    return getClassName(player != 2 ? class1.value : class2.value);
}

/* The attract run (runAttract): "ar/select_car" in SINGLE_RACE until it is
 * left or the idle time-out fires (then 0, the count reset).  The state
 * never changes: a one-state machine. */
// FUNC 0x00114A38 gt2k_menu::ArcadeControl::runDemo
int ArcadeControl::runDemo()
{
    int done = 0;
    int r = 1;
    int state = ARCADE_STATE_CAR;

    mode = ARCADE_SINGLE_RACE;
    level = 0;
    do {
        switch (state) {
        case ARCADE_STATE_CAR:
            r = runCarSelect(r);
            done = 1;
            break;
        }
        if (timedOut != 0) {
            frameCount = 0;                 /* resetFrameCount(), expanded */
            r = 0;
            timedOut = 0;
            done = 1;
        }
    } while (done == 0);
    return r;
}

/* The i.LINK flow of runILinkMenu: the first time "ar/connect" (result
 * ignored); then the course, the class becomes CLASS_DIRT for a dirt course,
 * else CLASS_A; the car and the selection exchange; in live mode
 * (g_00352CB0) straight to the exchange.  Pages cannot go back here. */
// FUNC 0x00114AC0 gt2k_menu::ArcadeControl::runILinkOnly
int ArcadeControl::runILinkOnly()
{
    mode = ARCADE_I_LINK;
    level = 0;
    if (g_00350FC0) {
        runConnectPage(1);
        g_00350FC0 = false;
    }

    int done = 0;
    int state = (g_00352CB0 != 0) ? ARCADE_STATE_EXCHANGE : ARCADE_STATE_COURSE;
    int r = 1;
    do {
        switch (state) {
        case ARCADE_STATE_COURSE:
            r = selectCourse(r);
            state = ARCADE_STATE_CAR;
            if (isDirtCourse())
                class1.set(ARCADE_CLASS_DIRT);
            else
                class1.set(ARCADE_CLASS_A);
            break;
        case ARCADE_STATE_CAR:
            r = runCarSelect(r);
            state = ARCADE_STATE_EXCHANGE;
            break;
        case ARCADE_STATE_EXCHANGE:
            r = runWaitSyncPage(r);
            done = 1;
            break;
        }
    } while (done == 0);
    return r;
}

/* SINGLE_RACE: level -> course -> class -> car; classes C .. home garage. */
// FUNC 0x00114BF8 gt2k_menu::ArcadeControl::runSingleRace
int ArcadeControl::runSingleRace()
{
    int done = 0;
    int state = ARCADE_STATE_LEVEL;
    int r = 1;

    class1.setRange(ARCADE_CLASS_C, ARCADE_CLASS_GUEST_GARAGE);
    do {
        switch (state) {
        case ARCADE_STATE_LEVEL:
            r = runLevelPage(r);
            if (r == 0)
                done = 1;
            else
                state = ARCADE_STATE_COURSE;
            break;
        case ARCADE_STATE_COURSE:
            r = selectCourse(r);
            state = (r == 0) ? ARCADE_STATE_LEVEL : ARCADE_STATE_CLASS;
            break;
        case ARCADE_STATE_CLASS:
            r = classSelect(r);
            state = (r == 0) ? ARCADE_STATE_COURSE : ARCADE_STATE_CAR;
            break;
        case ARCADE_STATE_CAR:
            r = runCarSelect(r);
            if (r == 0)
                state = ARCADE_STATE_CLASS;
            else
                done = 1;
            break;
        }
    } while (done == 0);
    return r;
}

/* COMPULSORY: only "ar/select_compulsory", class CLASS_COMPULSORY, racing
 * option 0. */
// FUNC 0x00114D28 gt2k_menu::ArcadeControl::runCompulsory
int ArcadeControl::runCompulsory()
{
    int state = ARCADE_STATE_COMPULSORY;
    int done = 0;
    int r = 1;

    menu_bgm_play_if_changed(0xE, 0, 1);
    class1.setRange(ARCADE_CLASS_COMPULSORY, ARCADE_CLASS_PREVIEW);
    class1.set(ARCADE_CLASS_COMPULSORY);
    racing1.set(0);
    do {
        switch (state) {
        case ARCADE_STATE_COMPULSORY:
            done = 1;
            r = selectCompulsory(r);
            break;
        }
    } while (done == 0);
    return r;
}

/* TIME_ATTACK: course -> class -> car; classes C .. home garage. */
// FUNC 0x00114DD8 gt2k_menu::ArcadeControl::runTimeAttack
int ArcadeControl::runTimeAttack()
{
    int done = 0;
    int state = ARCADE_STATE_COURSE;
    int r = 1;

    menu_bgm_play_if_changed(3, 0, 1);
    class1.setRange(ARCADE_CLASS_C, ARCADE_CLASS_GUEST_GARAGE);
    do {
        switch (state) {
        case ARCADE_STATE_COURSE:
            r = selectCourse(r);
            if (r == 0)
                done = 1;
            else
                state = ARCADE_STATE_CLASS;
            break;
        case ARCADE_STATE_CLASS:
            r = classSelect(r);
            state = (r == 0) ? ARCADE_STATE_COURSE : ARCADE_STATE_CAR;
            break;
        case ARCADE_STATE_CAR:
            r = runCarSelect(r);
            if (r == 0)
                state = ARCADE_STATE_CLASS;
            else
                done = 1;
            break;
        }
    } while (done == 0);
    return r;
}

/* TWO_PLAYER: course -> "ar/select_car_2p"; classes C .. guest garage for
 * both players. */
// FUNC 0x00114EE8 gt2k_menu::ArcadeControl::runTwoPlayer
int ArcadeControl::runTwoPlayer()
{
    int done = 0;
    int state = ARCADE_STATE_COURSE;
    int r = 1;

    menu_bgm_play_if_changed(1, 0, 1);
    class1.setRange(ARCADE_CLASS_C, ARCADE_CLASS_COMPULSORY);
    class2.setRange(ARCADE_CLASS_C, ARCADE_CLASS_COMPULSORY);
    do {
        switch (state) {
        case ARCADE_STATE_COURSE:
            r = selectCourse(r);
            if (r == 0)
                done = 1;
            else
                state = ARCADE_STATE_CAR;
            break;
        case ARCADE_STATE_CAR:
            r = carSelect2P(r);
            if (r == 0)
                state = ARCADE_STATE_COURSE;
            else
                done = 1;
            break;
        }
    } while (done == 0);
    return r;
}

/* I_LINK: connect + wait -> course -> class -> car -> selection exchange;
 * live mode goes from the connection straight to the exchange.  `again'
 * (g_00350FC4: back from an i.LINK race) starts at the course / the
 * exchange without reconnecting.  The class starts at CLASS_A, classes C ..
 * dirt, the courses of the I_LINK list.  An idle time-out (or a dropped
 * link, waitFrame) restarts at the connection with r = 1.  Jump table
 * 0x0030D940 on state - 4 (7 entries; 8 and 9 do nothing). */
// FUNC 0x00114FC0 gt2k_menu::ArcadeControl::runILink
int ArcadeControl::runILink(int again)
{
    int done = 0;
    int r = 1;
    int state;

    if (again == 0) {
        state = ARCADE_STATE_CONNECT;
    } else {
        g_00352CB0 = g_00350FCC;
        state = (g_00350FCC != 0) ? ARCADE_STATE_EXCHANGE : ARCADE_STATE_COURSE;
    }
    level = 0;
    class1.set(ARCADE_CLASS_A);
    ProductConfig *config = product_config();
    int count = config->courseCount(getModeName());
    course.setRange(0, count);
    class1.setRange(ARCADE_CLASS_C, ARCADE_CLASS_HOME_GARAGE);
    do {
        switch (state) {
        case ARCADE_STATE_CONNECT:
            r = runConnectPage(r);
            if (r != 0 && (r = runWaitOtherPage(r)) != 0)
                state = (g_00352CB0 != 0) ? ARCADE_STATE_EXCHANGE : ARCADE_STATE_COURSE;
            else
                done = 1;
            break;
        case ARCADE_STATE_COURSE:
            r = selectCourse(r);
            state = (r == 0) ? ARCADE_STATE_CONNECT : ARCADE_STATE_CLASS;
            break;
        case ARCADE_STATE_CLASS:
            r = classSelect(r);
            state = (r == 0) ? ARCADE_STATE_COURSE : ARCADE_STATE_CAR;
            break;
        case ARCADE_STATE_CAR:
            r = runCarSelect(r);
            state = (r == 0) ? ARCADE_STATE_CLASS : ARCADE_STATE_EXCHANGE;
            break;
        case ARCADE_STATE_EXCHANGE:
            r = runWaitSyncPage(r);
            if (r == 0)
                state = ARCADE_STATE_CAR;
            else
                done = 1;
            break;
        }
        if (timedOut != 0) {
            frameCount = 0;                 /* resetFrameCount(), expanded */
            r = 1;
            timedOut = 0;
            done = 0;
            state = ARCADE_STATE_CONNECT;
        }
    } while (done == 0);
    return r;
}

} /* namespace gt2k_menu */

/* For main_arcade's i.LINK battle (T000): the class name of a class index
 * (LinkPlayerInfo::carClass). */
// FUNC 0x001151A0 arcade_class_name
const char *arcade_class_name(int cls)
{
    return gt2k_menu::ArcadeControl::getClassName(cls);
}

namespace gt2k_menu {

/* "ar/top" (skipped while g_00350FC4 says to go back into the i.LINK flow),
 * then the chosen mode's flow; a flow returning 0 goes back to "ar/top",
 * otherwise the menu ends with its result.  LOAD_GUEST_GARAGE ends at once
 * with GameStatus menu.nextMode NEXT_MODE_GUEST_GARAGE (arcade_menu loads the
 * garage and comes back).  Cancelling "ar/top" ends with 0.  Jump table
 * 0x0030D960 (8 entries). */
// FUNC 0x001151C0 gt2k_menu::ArcadeControl::mainLoop
int ArcadeControl::mainLoop()
{
    int done = 0;
    int r = 1;

    getGameStatus()->getMenuStatus()->nextMode = 0;
    do {
        menu_bgm_play_if_changed(0, 0, 1);
        if (g_00350FC4 == 0)
            r = runTopPage(r);
        if (r == 0) {
            done = 1;
            g_00350FC4 = 0;
        } else {
            timedOut = 0;
            switch (mode) {
            case ARCADE_SINGLE_RACE:
                r = runSingleRace();
                break;
            case ARCADE_COMPULSORY:
                r = runCompulsory();
                break;
            case ARCADE_TIME_ATTACK:
                r = runTimeAttack();
                break;
            case ARCADE_TWO_PLAYER:
                r = runTwoPlayer();
                break;
            case ARCADE_GOODIES:
                r = runGoodiesPage(r);
                break;
            case ARCADE_LOAD_GUEST_GARAGE:
                getGameStatus()->getMenuStatus()->nextMode = NEXT_MODE_GUEST_GARAGE;
                break;
            case ARCADE_I_LINK:
                r = runILink(g_00350FC4);
                if (r != 0)
                    g_00350FC4 = 1;
                break;
            case ARCADE_NONE:
                break;
            }
            /* this test order lets jump2 share the store with the
             * cancelled "ar/top" path above (retail's layout) */
            if (r != 0)
                done = 1;
            else
                g_00350FC4 = 0;
        }
        waitFrame();
    } while (done == 0);
    waitFrame();
    return r;
}

/* The normal arcade menu (run, from run_arcade_menu). */
// FUNC 0x00115348 gt2k_menu::ArcadeControl::runMenu
int ArcadeControl::runMenu()
{
    load();
    setup();
    int r = mainLoop();
    finish(r);
    unload();
    return r;
}

/* No callers.  The attract screen: "ar/push_start" (decided by start or
 * decide meanwhile, by decide alone after: PageManager::setDecideButtons
 * expanded), each start runs runDemo; ARCADE_ATTRACT_TIMEOUT idle frames
 * time a page out (setTimeout expanded). */
// FUNC 0x001153A0 gt2k_menu::ArcadeControl::runAttract
int ArcadeControl::runAttract()
{
    PageManager *pm = &pages;
    load();
    int r = 0;
    setup();
    timedOut = 0;
    pm->setDecideButtons(CONTROL_DECIDE | CONTROL_START);
    timeout = ARCADE_ATTRACT_TIMEOUT;
    frameCount = 0;
    /* retail tests at the top and jumps back at the bottom: a loop
     * without an exit test egcs could rotate */
    do {
        if (r != 0)
            break;
        if (runPushStartPage(1) == 0)
            break;
        r = runDemo();
    } while (1);
    finish(r);
    unload();
    pm->setDecideButtons(CONTROL_DECIDE);
    return r;
}

/* No callers.  Only the i.LINK flow; always reports success. */
// FUNC 0x00115448 gt2k_menu::ArcadeControl::runILinkMenu
int ArcadeControl::runILinkMenu()
{
    load();
    setup();
    runILinkOnly();
    finish(1);
    unload();
    return 1;
}

// FUNC 0x00115498 gt2k_menu::ArcadeControl::run
int ArcadeControl::run()
{
    return runMenu();
}

/* For arcade_preview_test: "ar/select_car" over CLASS_PREVIEW only. */
// FUNC 0x001154B8 gt2k_menu::ArcadeControl::runPreview
int ArcadeControl::runPreview()
{
    load();
    setup();
    preview = 1;
    mode = ARCADE_SINGLE_RACE;
    level = 0;
    class1.setRange(ARCADE_CLASS_PREVIEW, ARCADE_CLASS_PREVIEW + 1);
    class1.set(ARCADE_CLASS_PREVIEW);
    int r = runCarSelect(0);
    finish(r);
    unload();
    return r;
}

/* Inline accessors: the out-of-line copies egcs emitted (no callers). */
// FUNC 0x00115548 gt2k_menu::ArcadeControl::setLevel
/* inline in game/menu/arcade/T010_arcade_control.h; egcs emits this copy */

// FUNC 0x00115550 gt2k_menu::ArcadeControl::getLevel
/* inline in game/menu/arcade/T010_arcade_control.h; egcs emits this copy */

// FUNC 0x00115558 gt2k_menu::ArcadeControl::getMode
/* inline in game/menu/arcade/T010_arcade_control.h; egcs emits this copy */

// FUNC 0x00115560 gt2k_menu::ArcadeControl::setMpegClass
/* inline in game/menu/arcade/T010_arcade_control.h; egcs emits this copy */

// FUNC 0x00115568 gt2k_menu::ArcadeControl::setMpegCourse
/* inline in game/menu/arcade/T010_arcade_control.h; egcs emits this copy */

// FUNC 0x00115570 gt2k_menu::ArcadeControl::getButtonList
/* inline in game/menu/arcade/T010_arcade_control.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base ControlBase). */
// FUNC 0x00115588 __tfQ29gt2k_menu13ArcadeControl
extern "C" void *__tfQ29gt2k_menu13ArcadeControl(void)
{
    if (__tiQ29gt2k_menu13ArcadeControl[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu13ArcadeControl, "Q29gt2k_menu13ArcadeControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu13ArcadeControl;
}

/* T010's copy of the typeinfo function of the template instance
 * SmartIndexT<int> (__user_type_info, no bases; no callers: T017's
 * 0x00103690 is the one called; both build the node at 0x003532F8).  Weak
 * so that the copies link. */
// FUNC 0x001155D8 __tfQ29gt2k_menut11SmartIndexT1Zi
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut11SmartIndexT1Zi(void)
{
    if (__tiQ29gt2k_menut11SmartIndexT1Zi[0] == 0)
        __rtti_user(__tiQ29gt2k_menut11SmartIndexT1Zi, "Q29gt2k_menut11SmartIndexT1Zi");
    return __tiQ29gt2k_menut11SmartIndexT1Zi;
}
#endif /* !BOOT_OLD_GCC */
