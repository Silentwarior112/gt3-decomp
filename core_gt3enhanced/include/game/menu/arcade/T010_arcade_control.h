/* T010_arcade_control.h - gt2k_menu::ArcadeControl, the controller of the
 * arcade-mode menu, and gt2k_menu::PageHookTable (translation unit T010, unit
 * u007), 0x00113A38..0x00115618.
 *
 * One ArcadeControl lives on the stack of run_arcade_menu (T015).  "ar/top"
 * chooses the mode; each mode is a small state machine over page functions
 * (states = pages: 2 "ar/level", 6 "ar/select_course", 7 "ar/select_class",
 * 5 "ar/select_car" / "ar/select_car_2p", 8 "ar/select_compulsory", 4
 * "ar/connect" + "ar/wait_other", 10 the i.LINK selection exchange); a page
 * returns 0 to go back one state.  finish() writes the choices to the
 * caller's MenuArgs and the GameStatus.  The methods are spread over T010
 * and the page units T012..T014, T018, T020..T023, T025, T026, T028, T031,
 * all declared here with the class.
 *
 * Players: a `player' argument of 2 means player 2, anything else player
 * 1.  Classes (class1 / class2 values, getClassName / getClassLabel): 0 C,
 * 1 B, 2 A, 3 S, 4 dirt, 5 home garage, 6 guest garage, 7 compulsory, 8
 * preview. */
#ifndef GAME_T010_ARCADE_CONTROL_H
#define GAME_T010_ARCADE_CONTROL_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/arcade/T011_arcade_render.h"        /* gt2k_menu::ArcadeRender (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pages) */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec (car1Spec, car2Spec) */
#include "game/common/T251_smart_index.h"          /* gt2k_menu::SmartIndexT<int> */
#include "game/common/T017_smart_index.h"          /* gt2k_menu::SmartCourseIndex, SmartCarIndex */
#include "game/menu/framework/T250_ipic.h"                 /* gt2k_menu::Ipic (ipic) */

struct MenuArgs;                            /* T000: the results of a menu (pointers only) */
class GarageCar;                            /* T036 */

/* ArcadeControl::mode: the entry chosen on "ar/top" (names g_002944A8). */
enum {
    ARCADE_SINGLE_RACE       = 0,
    ARCADE_COMPULSORY        = 1,
    ARCADE_TIME_ATTACK       = 2,
    ARCADE_TWO_PLAYER        = 3,
    ARCADE_GOODIES           = 4,
    ARCADE_LOAD_GUEST_GARAGE = 5,
    ARCADE_I_LINK            = 6,
    ARCADE_NONE              = 7,           /* after construction */
    /* The enhanced engine: Lupo 2's Pod race (its mode 6 "status" with
     * setPodRace 1 / 2; menus_a.md 9 / 14), appended: the buttons
     * "pod_race" / "pod_race_2p" (podPlayers 1 / 2), the course list
     * "POD_RACE" (g_002944A8[8]), the cars of CLASS_PODRACE. */
    ARCADE_POD_RACE          = 8
};

/* The Pod race's data (Lupo 2's GCF names and 2P race; none of it is in
 * GT3's data, so the mode is never offered there: podRaceAvailable). */
#define ARCADE_POD_COURSES  "POD_RACE"      /* the GCF course list of the mode (Lupo 2's mode name) */
#define ARCADE_POD_CLASS    "CLASS_PODRACE" /* the GCF car list (Lupo 2 L2 0x0035E290) */
#define ARCADE_POD_2P_RACE  "APO20001"      /* the 2-player race (Lupo 2 L2 0x00336D30) */

/* The arcade classes (ArcadeControl::class1 / class2 values; their names
 * getClassName "CLASS_C" .. g_002944C8, getClassLabel "class_c" ..
 * g_002944F0).  Every class but the two garages takes its cars from the
 * product database (ProductConfig, category getPlayerClassName(player)).
 * u003, u004, u005, u007 found them. */
enum ArcadeClass {
    ARCADE_CLASS_C            = 0,
    ARCADE_CLASS_B            = 1,
    ARCADE_CLASS_A            = 2,
    ARCADE_CLASS_S            = 3,
    ARCADE_CLASS_DIRT         = 4,          /* the class of the dirt courses */
    ARCADE_CLASS_HOME_GARAGE  = 5,          /* the cars of the GameStatus (getClassName NULL) */
    ARCADE_CLASS_GUEST_GARAGE = 6,          /* the cars of the guest GameStatus (T004) */
    ARCADE_CLASS_COMPULSORY   = 7,          /* "ar/select_compulsory" */
    ARCADE_CLASS_PREVIEW      = 8,          /* runPreview */
    ARCADE_CLASS_POD          = 9           /* the enhanced engine: the Pod race's cars (CLASS_PODRACE) */
};

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Drawable;                             /* T259 */
class Rectangle;                            /* T256 */
class Item;                                 /* T213 */
class String;                               /* T258 */
class Cross;                                /* T255 */
class Car;                                  /* T212 */
class MpegImage;                            /* T249 */
class Blur2D;                               /* T254 */
class ButtonList;                           /* T244 */
class MenuControl;                          /* T214 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
class MiniCursor;                           /* T024 */
class SelectBox;                            /* T035 */
class TransitiveEX;                         /* T023 */
class CircleGauge;                          /* T032 */

/* A (table, count, ctx) triple of the page manager: PageManager keeps five
 * at +0xB00 (selectable / menu / execute / valid / focus FuncData, T211
 * declares them field by field); the first pass's screens set them as
 * hooks[0..4] (ArcadeControl::setup: g_002902E0 T016, g_002925A0 T027,
 * g_002927B8 T028, g_00350B68 T016, g_00290940 T019). */
struct PageHookTable {
    const void *entries;                    /* 0x0 */
    int         count;                      /* 0x4 */
    void       *context;                    /* 0x8 */
};
EGCS_SIZE(gt2k_menu::PageHookTable, 0xC);

/* vtable 0x0030D980 (2 slots).  typeinfo __tfQ29gt2k_menu13ArcadeControl
 * 0x00115588 (si, base ControlBase; storage 0x004B1500).  ControlBase's
 * data +0x000 .. +0x288, vptr +0x288.  Size 0x3BE0 (the stack object of
 * run_arcade_menu; 8-aligned through MenuCarSpec). */
class ArcadeControl : public ControlBase {
public:
    /* The name function of createMiniCursor's entries, and the change /
     * check callbacks of selectInput (pointers to members). */
    typedef void (ArcadeControl::*NameFn)(char *buf, int index, int arg);
    typedef void (ArcadeControl::*ChangeFn)(int player);
    typedef int  (ArcadeControl::*CheckFn)(int value, int player);

    /* T018 "ar/select_compulsory" (the cars tt01.., their logo, text, spec
     * and records) */
    void compulsoryMiniName(char *buf, int i, int arg); /* 0x00103B70 "tt%02d_mini" (NameFn) */
    void compulsoryCreateDoc();             /* 0x00103B98 "tt%02d_doc", "arrow" */
    void compulsoryCreateSpec();            /* 0x00103CC0 power, weight, tyre items */
    void compulsoryCreateRecords();         /* 0x00103FC8 the three best times (GameStatus+0x246F8) */
    void compulsoryChangeCar();             /* 0x00104390 the items of the new car */
    void compulsoryOpen();                  /* 0x001045D0 builds the page */
    void compulsoryClear();                 /* 0x00104B40 forgets the page's items */
    int  compulsorySelectCar(int result);   /* 0x00104BC8 the input loop: 1 decided, 0 cancelled */
    int  selectCompulsory(int result);      /* 0x00104D08 the page (runCompulsory) */

    /* T020 helpers of the arcade pages */
    void transitionIn();                    /* 0x00104F30 fade's page-in script g_00309978 */
    void transitionOut();                   /* 0x00104F60 the page-out script g_00309A38; frames until
                                               transitionOutDone (T020_arcade_helpers.h) */
    void createArrows(SmartIndexT<int> *idx, PageLayout *layout, Object *parent); /* 0x00104FC8
                                               "arrow_l" / "arrow_r" under parent */
    void updateArrows(SmartIndexT<int> *idx);   /* 0x001050C8 hidden at the ends of a clamped index */
    void clearArrows();                     /* 0x00105140 */
    /* Makes d relative to the bar (anchor type 3): its x / y lose the
     * bar's base x / y.  `this' is not used. */
    void attachToBar(Drawable *d, Cross *bar);  /* 0x00105170 */
    /* The history banners of "ar/top"'s choices, slid in / out on the
     * render root: the level (historyLevel[level]), the area of the course
     * (historyArea[area], none past 5), the class (historyClass[class1]). */
    void showHistoryLevel();                /* 0x001051D0 script g_00309BB8, alphaRatio 1 */
    void hideHistoryLevel();                /* 0x001052E8 g_00309C18 */
    void showHistoryArea();                 /* 0x00105320 g_00309C78, alphaRatio 1 */
    void hideHistoryArea();                 /* 0x00105488 g_00309CD8 */
    void showHistoryClass();                /* 0x001054C0 g_00309D38 */
    void hideHistoryClass();                /* 0x001055F8 g_00309D98 */
    /* A MiniCursor on the hidden item `name' over idx, entry i the item
     * (this->*nameOf)(buf, i, arg) names (missing ones skipped); NULL if
     * `name' is missing. */
    MiniCursor *createMiniCursor(const char *name, SmartIndexT<int> *idx, NameFn nameOf,
                                 int arg);  /* 0x00105630 */
    /* A String of the hidden item `name': its text and font ("sw_16"),
     * centred on it, size `size', its colour, its z.  `this' is not used. */
    String *createLabel(const char *name, PageLayout *layout, f32 size);   /* 0x001057B8 */

    /* T021 "ar/select_car_2p" (two players' class, car, colour,
     * transmission and racing choices side by side) */
    Car *newCarModel(const char *code, int player);     /* 0x001060B8 above the "car" placeholder
                                               (code: carCode's result) */
    /* One input step of a selector: left / right move sel (onSelChange),
     * up / down move sel2 (onSel2Change); decide refused while selDisabled. */
    int  selectInput(int player, int *result, MenuControl *pad, SmartIndexT<int> *sel,
                     ChangeFn onSelChange, CheckFn selDisabled, SmartIndexT<int> *sel2,
                     ChangeFn onSel2Change);    /* 0x00106210 */
    void openClassList(int player);         /* 0x001064D8 */
    void closeClassList(int player);        /* 0x00106770 */
    void onClassChange(int player);         /* 0x001067E0 (ChangeFn) */
    int  classListInput(int *result, int player);   /* 0x00106928 */
    void openCarView(int player);           /* 0x00106A28 */
    void openCarList(int player);           /* 0x00106C58 the car list, data, labels, gauges */
    void closeCarList(int player);          /* 0x00107158 */
    void closeCarView(int player);          /* 0x001072F0 */
    void onCarChange(int player);           /* 0x00107370 (ChangeFn) */
    void onColorChange(int player);         /* 0x00107978 (ChangeFn) */
    int  carListInput(int *result, int player);     /* 0x001079B8 */
    void openTransmission(int player);      /* 0x00107A70 */
    void onTransmissionChange(int player);  /* 0x00107CD8 (ChangeFn) */
    int  transmissionInput(int *result, int player);    /* 0x00107D70 */
    void closeTransmission(int player);     /* 0x00107E70 */
    void openRacing(int player);            /* 0x00107F10 */
    void onRacingChange(int player);        /* 0x00108178 (ChangeFn) */
    int  racingInput(int *result, int player);  /* 0x00108210 */
    void closeRacing(int player);           /* 0x00108310 */
    /* One frame of one player's state machine; 1 when the page is over. */
    int  carSelect2PStep(int *state, int otherState, int *result, int player); /* 0x001083B0 */
    void carSelect2PAbort(int state, int player);   /* 0x00108650 */
    int  carSelect2P(int result);           /* 0x00108710 the page (runTwoPlayer) */
    /* The enhanced engine (new; Lupo 2 L2 0x0010BC18 / 0x0010BE60 /
     * 0x0010BEE0 / 0x0010BFF0; menus_a.md 17): the assist step of the car
     * selects, one of the levels 0..2 of GameOptions::getAssist (TCS + ASM,
     * TCS, none) for each player.  Only when the page's layout has the
     * items ("assist_title", "assist_N_yes", "assist_N_no"; "_2p" for player
     * 2): GT3's menu/arcade.mbl has none, and then the step is skipped. */
    int  hasAssistStep(int player);         /* the layout has "assist_title" (_2p) */
    void openAssist(int player);            /* the title, the yes / no items of the levels */
    void onAssistChange(int player);        /* (ChangeFn) the level's "yes" shown */
    int  assistInput(int *result, int player);  /* left / right over assist1 / 2 */
    void closeAssist(int player);
    int  selectAssist(int result);          /* player 1's step of "ar/select_car" (runCarSelect) */

    /* T022 "ar/select_class" */
    Item *newDisabledMark(int disabled, int player);    /* 0x00108870 "disabled" / "undisabled" */
    /* Road classes on a dirt course, the dirt class on a road course, an
     * empty garage (CheckFn). */
    int  isClassDisabled(int cls, int player);  /* 0x00108910 */
    void showClassPicture();                /* 0x00108988 the class's still in mpegClass */
    Item *newClassLogo(int player);         /* 0x00108A38 */
    Item *newClassDoc(int player);          /* 0x00108AD0 */
    void formatClassMini(char *buf, int cls, int player);   /* 0x00108C10 "%s_mini" (NameFn) */
    void openClassSelect();                 /* 0x00108C50 */
    void updateClassSelect();               /* 0x00108F38 */
    void clearClassSelect();                /* 0x001090C8 */
    int  classSelect(int forward);          /* 0x00109108 the page (the one-player flows) */

    /* T023 "ar/select_car" (player 1; the car comes from the data base or
     * from a garage after its class) */
    int  F_00109F20(int player);            /* 0x00109F20 (no callers) 0 for the garages and class 8 */
    int  carCount(int player);              /* 0x00109F58 -1 for an unknown class */
    const char *carCode(int player, int car);   /* 0x0010A028 car < 0: the current one */
    void loadCarInfo(int player);           /* 0x0010A128 the current car into its MenuCarSpec */
    const char *carName(const char *code);  /* 0x0010A250 the name of its pictures */
    int  isCarLocked(int car, int player);  /* 0x0010A2F0 (CheckFn) */
    GarageCar *garageCar(int player);       /* 0x0010A3E0 NULL for a data base car */
    /* The enhanced engine (new, Lupo 2 L2 0x0010CDE8; menus_a.md 39): the
     * player's current car is a CVT (CarEquipments::isCVT): its
     * transmission choice is automatic only. */
    int  isCarCVT(int player);
    void setupColorIndex(MenuCarSpec *spec, int cls, SmartIndexT<int> *color); /* 0x0010A480 */
    void resetColorIndex(int player);       /* 0x0010A528 */
    void scrollToRow(int row);              /* 0x0010A570 the cross bar to option row `row' */
    void applyColor();                      /* 0x0010A5C8 */
    Car *createCarView(const char *code);   /* 0x0010A608 player 1's Car (carBuffer1) */
    SelectBox *createColorChips(int player);    /* 0x0010A700 "color_chip" (_2p) */
    Item *createDriveTrainIcon(int player); /* 0x0010A7D0 "ff", "fr", "four_wd", "mr", "rr" (_2p) */
    String *createPowerText(int player);    /* 0x0010A938 "ps" (_2p) */
    String *createTorqueText(int player);   /* 0x0010AA40 "kgm" (_2p) */
    String *createSpecLabel(const char *name);  /* 0x0010AB58 "handling", "speed", "acceleration" */
    void createColorName();                 /* 0x0010ABA8 "color_name" */
    void createSpecPanel();                 /* 0x0010AE68 the TransitiveEX of the dimensions */
    void miniCarImageName(char *buf, int car, int player);  /* 0x0010AF20 "<car name>_mini" (NameFn) */
    void onCarChanged();                    /* 0x0010AF78 */
    void startCarViewMotion(int player);    /* 0x0010B428 script g_0030ACA0 */
    void updateTransmissionButtons();       /* 0x0010B460 */
    void updateRacingButtons();             /* 0x0010B4B8 */
    void buildCarSelect();                  /* 0x0010B510 */
    void clearCarSelect();                  /* 0x0010BF48 */
    void hideMiniCars();                    /* 0x0010BFD8 */
    void showMiniCars();                    /* 0x0010C000 */
    void createBlur();                      /* 0x0010C028 */
    void blurIn();                          /* 0x0010C108 */
    void blurOut();                         /* 0x0010C140 */
    int  selectCarAndColor(int result);     /* 0x0010C178 state 1 */
    int  selectRacing(int result);          /* 0x0010C360 state 3 (row 2) */
    int  selectTransmission(int result);    /* 0x0010C480 state 2 (row 1) */
    int  F_0010C5A0(int result);            /* 0x0010C5A0 (no callers) row 3, opt1d */
    int  runCarSelect(int result);          /* 0x0010C6B8 the page */

    /* T025 "ar/select_course" */
    int  isCourseLocked(int course);        /* 0x0010DFF0 */
    const char *courseMovieName(const char *course);    /* 0x0010E130 "stream/<course>_full.pss" (static
                                               buffer 0x0049BB50) */
    const char *F_0010E200(const char *name);   /* 0x0010E200 (no callers) "%s1" (0x0049BB90) */
    void loadCourseImage();                 /* 0x0010E240 the course's still from courseIpic */
    void F_0010E310();                      /* 0x0010E310 empty */
    void createCourseLogo();                /* 0x0010E318 */
    void createCourseDoc();                 /* 0x0010E3F8 */
    void createCourseInfo();                /* 0x0010E548 length, area, surface */
    void miniCourseName(char *buf, int course, int a);  /* 0x0010E898 (NameFn) */
    void createCourseScreen();              /* 0x0010E910 */
    void refreshCourseScreen();             /* 0x0010EB48 */
    void clearCourseScreen();               /* 0x0010ECC8 */
    int  selectCourse(int first);           /* 0x0010ED18 the page */

    /* T026 */
    const char *raceCode();                 /* 0x0010F398 the race of the selection (NULL for modes 4,
                                               5, 7); static buffers 0x0049BBD0 / 0x0049BBE0 */

    /* T028 */
    void setModeByName(const char *name);   /* 0x0010FB60 "single_race" .. "i_link_battle" */

    /* T031 */
    int  runTopPage(int prev);              /* 0x00113540 "ar/top" (caches historyLevel / Area / Class) */
    int  runLevelPage(int prev);            /* 0x00113870 "ar/level" */
    int  runGoodiesPage(int prev);          /* 0x00113968 "ar/goodies" */

    /* T010 */
    /* Members in order (pages(this), car(this, player), the indices at
     * [0, 1)), then 0x3A30 .. 0x3BD8 cleared, mode ARCADE_NONE. */
    ArcadeControl(MenuArgs *result);        /* 0x00113A38 */
    virtual ~ArcadeControl();               /* [0] +0x0C 0x00113EA8 */
    /* A dropped link (g_00352CDC) times an I_LINK run out; then
     * ControlBase::debugPause(&render) and ControlBase's frame. */
    virtual void waitFrame();               /* [1] +0x14 0x00113F38 */
    void load();                            /* 0x00113F90 the car buffers, "menu/arcade.mbl",
                                               "stream/course.ipic" */
    void unload();                          /* 0x00114028 */
    void setup();                           /* 0x00114090 the movie, the mode, the page tables, the
                                               indices, the fade rectangle */
    int  getSelection();                    /* 0x00114500 MenuArgs::selection: 1..5 */
    /* The course's kind (RaceDataBase::getCourseKind) is 2: a bool (T022's
     * isClassDisabled tests !isDirtCourse() with xori 1). */
    bool isDirtCourse();                    /* 0x00114558 */
    void finish(int ok);                    /* 0x00114610 the results; shuts the menu down */
    const char *getModeName();              /* 0x00114988 "SINGLE_RACE" .. (g_002944A8) */
    static const char *getClassName(int cls);   /* 0x001149A8 "CLASS_C" .. (g_002944C8; NULL for
                                               the garages) */
    const char *getPlayerClassLabel(int player);    /* 0x001149C0 (no callers) */
    const char *getClassLabel(int cls);     /* 0x001149F0 "class_c" .. (g_002944F0) */
    const char *getPlayerClassName(int player);     /* 0x00114A08 */
    int  runDemo();                         /* 0x00114A38 the attract run of "ar/select_car" */
    int  runILinkOnly();                    /* 0x00114AC0 */
    int  runSingleRace();                   /* 0x00114BF8 level, course, class, car */
    int  runCompulsory();                   /* 0x00114D28 */
    int  runTimeAttack();                   /* 0x00114DD8 course, class, car */
    int  runTwoPlayer();                    /* 0x00114EE8 course, "ar/select_car_2p" */
    int  runILink(int again);               /* 0x00114FC0 connect, course, class, car, exchange */
    /* The enhanced engine (menus_a.md 9 / 14): the Pod race.  Its data is
     * there (cars, courses, the race records: podRaceAvailable) and its
     * flow (course, then the car select of 1 or 2 players). */
    int  podRaceAvailable(int players);
    int  runPodRace();
    int  mainLoop();                        /* 0x001151C0 "ar/top" and the mode flows */
    int  runMenu();                         /* 0x00115348 load, setup, mainLoop, finish, unload */
    int  runAttract();                      /* 0x001153A0 (no callers) "ar/push_start" and runDemo */
    int  runILinkMenu();                    /* 0x00115448 (no callers) */
    int  run();                             /* 0x00115498 runMenu() */
    int  runPreview();                      /* 0x001154B8 arcade_preview_test: class 8 only */
    /* Inline accessors: egcs's out-of-line copies (no callers) come in
     * reverse declaration order; T030's mpeg_class / mpeg_course hooks use
     * the two setters. */
    ButtonList *getButtonList() { return pages.buttonLists[pages.popupLevel]; }  /* 0x00115570 */
    void setMpegCourse(MpegImage *m) { mpegCourse = m; }   /* 0x00115568 */
    void setMpegClass(MpegImage *m) { mpegClass = m; }     /* 0x00115560 */
    int  getMode() { return mode; }                        /* 0x00115558 */
    int  getLevel() { return level; }                      /* 0x00115550 */
    void setLevel(int v) { level = v; }                    /* 0x00115548 */

    /* T012 */
    int  runPushStartPage(int prev);        /* 0x001156F8 "ar/push_start" */
    /* T013 */
    int  runConnectPage(int prev);          /* 0x001157D0 "ar/connect" */
    int  runWaitOtherPage(int prev);        /* 0x00115A60 "ar/wait_other" */
    void F_00115C80(int sel);               /* 0x00115C80 the nodes' link modes; g_00352CA8 = sel */
    void setLinkMode(int node, int linkMode);   /* 0x00115CE8 (2 = live) */
    /* T014 */
    int  runWaitSyncPage(int prev);         /* 0x00115E10 the selection exchange */

    /*               ControlBase                0x000 .. 0x28C (vptr +0x288) */
    ArcadeRender     render;                /* 0x28C */
    PageManager      pages;                 /* 0x2E4 "menu/arcade.mbl" */
    void            *carBuffer1;            /* 0xE40 0xA8000 bytes of the menu heap: player 1's Car
                                               model buffer */
    void            *carBuffer2;            /* 0xE44 player 2's */
    MenuArgs        *result;                /* 0xE48 */
    /*               (alignment)                0xE4C */
    MenuCarSpec      car1Spec;              /* 0xE50 player 1's car */
    MenuCarSpec      car2Spec;              /* 0x2388 player 2's car */
    SmartCourseIndex course;                /* 0x38C0 */
    SmartIndexT<int> class1;                /* 0x38DC player 1's class */
    SmartCarIndex    car1;                  /* 0x38F4 the car of the class */
    SmartIndexT<int> color1;                /* 0x3914 (MenuArgs::color1) */
    SmartIndexT<int> racing1;               /* 0x392C 0 "racing", 1 "dri" (MenuArgs::tyre1 = == 1) */
    SmartIndexT<int> transmission1;         /* 0x3944 0 automatic, 1 manual (MenuArgs::transmission1 = == 1) */
    SmartIndexT<int> opt1d;                 /* 0x395C (MenuArgs::opt1d = == 0; only F_0010C5A0) */
    SmartIndexT<int> class2;                /* 0x3974 player 2 */
    SmartCarIndex    car2;                  /* 0x398C */
    SmartIndexT<int> color2;                /* 0x39AC */
    SmartIndexT<int> racing2;               /* 0x39C4 */
    SmartIndexT<int> transmission2;         /* 0x39DC */
    SmartIndexT<int> opt2d;                 /* 0x39F4 */
    /* file_read_blocking's buffer, free()d by unload */
    IpicArchiveData *courseIpic;            /* 0x3A0C "stream/course.ipic": the course and class stills */
    Ipic             ipic;                  /* 0x3A10 the still shown */
    int              mode;                  /* 0x3A2C ARCADE_* */
    int              level;                 /* 0x3A30 0 easy, 1 normal, 2 hard, 3 professional */
    Rectangle       *fade;                  /* 0x3A34 the full-screen rectangle of transitionIn / Out */
    Item            *disabledMark;          /* 0x3A38 */
    Item            *arrowL;                /* 0x3A3C "arrow_l" */
    Item            *arrowR;                /* 0x3A40 "arrow_r" */
    Item            *arrow;                 /* 0x3A44 "arrow" */
    Item            *logo;                  /* 0x3A48 the course / class / car logo */
    Item            *doc;                   /* 0x3A4C the course / class / car text */
    int              unk3A50;               /* 0x3A50 */
    Item            *historyLevelItem;      /* 0x3A54 showHistoryLevel */
    Item            *historyAreaItem;       /* 0x3A58 showHistoryArea */
    Item            *historyClassItem;      /* 0x3A5C showHistoryClass */
    MenuLayoutItem  *historyLevel[4];       /* 0x3A60 "history_level_easy" .. "_pro" of "ar/top" */
    MenuLayoutItem  *historyArea[6];        /* 0x3A70 "history_area_a" .. "_f" */
    MenuLayoutItem  *historyClass[7];       /* 0x3A88 "history_class_c" .. "_guest_garage" */
    MpegImage       *mpegCourse;            /* 0x3AA4 "mpeg_course" (T030) */
    Cross           *courseBar;             /* 0x3AA8 the course select's cross bar */
    MiniCursor      *miniCourses;           /* 0x3AAC "mini_courses" */
    Item            *totalLength;           /* 0x3AB0 "total_length" */
    Item            *courseName;            /* 0x3AB4 */
    Item            *length;                /* 0x3AB8 "length" */
    Item            *area;                  /* 0x3ABC "area_a" .. "area_f" (single race only) */
    Item            *surface;               /* 0x3AC0 */
    MpegImage       *mpegClass;             /* 0x3AC4 "mpeg_class" (T030) */
    Cross           *classBar;              /* 0x3AC8 the class select's cross bar */
    MiniCursor      *miniClasses;           /* 0x3ACC "mini_classes" */
    Cross           *crossBar;              /* 0x3AD0 the car selects' cross bar */
    f32              crossBarY;             /* 0x3AD4 its y when built (scrollToRow) */
    Item            *carItem;               /* 0x3AD8 "car" (hidden: the place of the Car) */
    Car             *carView;               /* 0x3ADC */
    MiniCursor      *miniCars;              /* 0x3AE0 "mini_cars" */
    int              unk3AE4;               /* 0x3AE4 */
    Item            *colorSelection;        /* 0x3AE8 "color_selection" */
    SelectBox       *colorChips;            /* 0x3AEC */
    Item            *driveTrain;            /* 0x3AF0 */
    Item            *colorName;             /* 0x3AF4 "color_name" */
    String          *powerText;             /* 0x3AF8 "ps" */
    String          *torqueText;            /* 0x3AFC "kgm" */
    String          *handlingText;          /* 0x3B00 "handling" */
    String          *speedText;             /* 0x3B04 "speed" */
    String          *accelText;             /* 0x3B08 "acceleration" */
    CircleGauge     *handlingGauge;         /* 0x3B0C */
    CircleGauge     *speedGauge;            /* 0x3B10 */
    CircleGauge     *accelGauge;            /* 0x3B14 */
    TransitiveEX    *specPanel;             /* 0x3B18 */
    Item            *autoNo;                /* 0x3B1C "auto_no" */
    Item            *autoYes;               /* 0x3B20 "auto_yes" */
    Item            *manuNo;                /* 0x3B24 "manu_no" */
    Item            *manuYes;               /* 0x3B28 "manu_yes" */
    Item            *driNo;                 /* 0x3B2C "dri_no" */
    Item            *driYes;                /* 0x3B30 "dri_yes" */
    Item            *racingNo;              /* 0x3B34 "racing_no" */
    Item            *racingYes;             /* 0x3B38 "racing_yes" */
    Blur2D          *blur;                  /* 0x3B3C */
    MiniCursor      *miniClasses2;          /* 0x3B40 player 2 ("_2p" items) */
    Item            *carItem2;              /* 0x3B44 */
    Car             *carView2;              /* 0x3B48 */
    MiniCursor      *miniCars2;             /* 0x3B4C */
    Item            *logo2;                 /* 0x3B50 */
    int              unk3B54;               /* 0x3B54 */
    SelectBox       *colorChips2;           /* 0x3B58 */
    Item            *driveTrain2;           /* 0x3B5C */
    String          *powerText2;            /* 0x3B60 */
    String          *torqueText2;           /* 0x3B64 */
    String          *handlingText2;         /* 0x3B68 */
    String          *speedText2;            /* 0x3B6C */
    String          *accelText2;            /* 0x3B70 */
    CircleGauge     *handlingGauge2;        /* 0x3B74 */
    CircleGauge     *speedGauge2;           /* 0x3B78 */
    CircleGauge     *accelGauge2;           /* 0x3B7C */
    Item            *autoNo2;               /* 0x3B80 */
    Item            *autoYes2;              /* 0x3B84 */
    Item            *manuNo2;               /* 0x3B88 */
    Item            *manuYes2;              /* 0x3B8C */
    Item            *driNo2;                /* 0x3B90 */
    Item            *driYes2;               /* 0x3B94 */
    Item            *racingNo2;             /* 0x3B98 */
    Item            *racingYes2;            /* 0x3B9C */
    Item            *bg;                    /* 0x3BA0 "tt%02d_bg" (the compulsory page, T018) */
    Item            *power;                 /* 0x3BA4 "power" */
    Item            *powerValue;            /* 0x3BA8 "power_value" */
    Item            *weight;                /* 0x3BAC "weight" */
    Item            *weightValue;           /* 0x3BB0 "weight_value" */
    Item            *tire;                  /* 0x3BB4 "tire" */
    Item            *tire01;                /* 0x3BB8 "tire01" */
    Item            *record;                /* 0x3BBC "record" */
    Item            *first;                 /* 0x3BC0 "first" */
    Item            *firstTime;             /* 0x3BC4 "first_time" */
    Item            *second;                /* 0x3BC8 "second" */
    Item            *secondTime;            /* 0x3BCC "second_time" */
    Item            *third;                 /* 0x3BD0 "third" */
    Item            *thirdTime;             /* 0x3BD4 "third_time" */
    int              preview;               /* 0x3BD8 runPreview */
    /* The enhanced engine (Lupo 2's assist step, its ArcadeControl +0x39B4
     * / +0x3A64 and the items +0x3B54.. / +0x3BEC..; menus_a.md 17):
     * appended here.  The levels start at GameOptions::getAssist (setup);
     * assistChosen is set when the player decided the step (finish passes
     * the level on, MenuArgs::assist1 / 2, else -1: the options'). */
    SmartIndexT<int> assist1;               /* 0x3BDC player 1's assist level, [0, 3) */
    SmartIndexT<int> assist2;               /* 0x3BF4 player 2's */
    Item            *assistTitle[2];        /* 0x3C0C "assist_title" / "assist_title_2p" */
    Item            *assistYes[2][3];       /* 0x3C14 "assist_N_yes" (_2p): shown for the level chosen */
    Item            *assistNo[2][3];        /* 0x3C2C "assist_N_no" (_2p) */
    int              assistChosen[2];       /* 0x3C44 */
    /* The enhanced engine: the Pod race's players (Lupo 2's podRace +0x3C48,
     * setPodRace L2 0x002B2640; menus_a.md 14): 1 or 2, set with the mode by
     * the "pod_race" / "pod_race_2p" buttons. */
    int              podPlayers;            /* 0x3C4C */
};
EGCS_SIZE(gt2k_menu::ArcadeControl, 0x3C50);   /* (GT3 0x3BE0: 0x3BDC padded to 8) */

} /* namespace gt2k_menu */

/* For T000's i.LINK battle: ArcadeControl::getClassName(cls). */
const char *arcade_class_name(int cls);     /* 0x001151A0 */

/* T010 .data / .sdata. */
extern const char *g_002944A8[9];           /* 0x002944A8 mode names "SINGLE_RACE" .. "I_LINK", NULL
                                               (the enhanced engine: + "POD_RACE") */
extern const char *g_002944C8[10];          /* 0x002944C8 class names "CLASS_C" .. "CLASS_PREVIEW"
                                               (+ "CLASS_PODRACE") */
extern const char *g_002944F0[10];          /* 0x002944F0 class labels "class_c" .. "class_preview"
                                               (+ "class_podrace") */
/* A bool (4 bytes under egcs): runILinkOnly's load of it does not conflict
 * with the int stores to mode / level, which is what lets sched2 leave it
 * behind the register saves as retail does (as an int it is hoisted). */
extern bool g_00350FC0;                     /* 0x00350FC0 gp-0x6A30 true until the first i.LINK connect */
extern int g_00350FC4;                      /* 0x00350FC4 gp-0x6A2C nonzero: back into the i.LINK flow */
/* T013 .sdata (after T010's: u007's data pass).  The frames left on
 * "ar/connect" (540 when the page opens, -1 a frame; T013, T030's
 * "countdown"); cleared by the "cancel_select_game" hook (T028).  The plan
 * put it in T024. */
extern int g_00350FC8;                      /* 0x00350FC8 gp-0x6A28 */
extern int g_00350FCC;                      /* 0x00350FCC gp-0x6A24 i.LINK live (1) / play (0) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu13ArcadeControl(void);    /* 0x00115588 */
extern u32 __tiQ29gt2k_menu13ArcadeControl[3];  /* 0x004B1500 its __si_type_info */
}
#endif

#endif /* GAME_T010_ARCADE_CONTROL_H */
