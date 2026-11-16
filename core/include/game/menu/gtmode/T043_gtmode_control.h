/* T043_gtmode_control.h - gt2k_menu::GtmodeControl, the controller of the
 * GT-mode menu, and gt2k_menu::PageHandlerTable (translation unit T043, unit
 * u009), 0x00119778..0x0011A2B0.
 *
 * gtmode_main (T038) runs one on its stack.  run() loads "menu/gt_mode.mbl",
 * gives the GT-mode tables to the PageManager (begin) and then alternates
 * (mainLoop) between the root page, played over the movie
 * "stream/title_full.pss" and left after the event data base's idle time,
 * and the other pages, until a page names where the game goes next; the name
 * goes to MenuStatus::setPage for main() (T000).  runTrade() runs the single
 * page "hm/hm_gr_trade" (the garage trade) the same way.
 *
 * The tables begin() sets (ctx: the GtmodeControl): selectable g_002959E8
 * (T049), menu g_00295FB8 (T044), execute g_0029A180 (T037), valid
 * g_00296750 (T045), focus g_00295380 (T048); the hooks g_00296B90 (T036) on
 * both layouts. */
#ifndef GAME_T043_GTMODE_CONTROL_H
#define GAME_T043_GTMODE_CONTROL_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/gtmode/T046_gtmode_render.h"        /* gt2k_menu::GtmodeRender (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pm) */
#include "game/common/T251_smart_index.h"          /* gt2k_menu::SmartIndexT<int> (infoMode) */

namespace gt2k_menu {
class RootObject;                           /* T220 (pointers only) */
class ButtonList;                           /* T244 */
class OilChangeCylinder;                    /* T040 */

/* (The first pass's PageHandlerTable, the (table, count, ctx) triples at
 * PageManager +0xB00 that u009, u013, u015, u018 and u040 called
 * handlers[0..4], is not an original type and not declared: T211 declares
 * the five triples field by field (selectable / menu / execute / valid /
 * focus FuncData); T010_arcade_control.h's PageHookTable is the same
 * record.) */

/* GtmodeControl::sortType: the garage's sort order (T037's "sort_*"
 * actions; the index of GarageButtonList's comparators, T219 g_002C8DF8).
 * An enum, not an int: the constructor's store of it is not ordered with
 * the int stores of infoMode (its own alias set), as retail shows
 * (ref/matching/u009/notes.md).  The type name is not known. */
enum GarageSortOrder {
    GARAGE_SORT_DATE       = 0,             /* cmp_index: the garage index (purchase order) */
    GARAGE_SORT_NAME       = 1,
    GARAGE_SORT_MAKER      = 2,
    GARAGE_SORT_COLOR      = 3,
    GARAGE_SORT_PRICE      = 4,
    GARAGE_SORT_DISTANCE   = 5,
    GARAGE_SORT_POWER      = 6,
    GARAGE_SORT_DRIVE      = 7,
    GARAGE_SORT_ASPIRATION = 8
};

/* vtable 0x0030F878 (2 slots).  typeinfo __tfQ29gt2k_menu13GtmodeControl
 * 0x0011A1D0 (si, base ControlBase; storage 0x004B1590).  ControlBase's data
 * +0x000 .. +0x288, vptr +0x288.  Size 0xE74 (the stack object of
 * gtmode_main).
 *
 * The destructor is implicit in the original: 0x0011A220 ([0] +0x0C) does
 * not store the vptr; it destroys render inline (render.root, then the
 * GameObject, flags 2) and calls ~ControlBase with the flags (gtmode_main
 * expands the same).  T043 also has a copy of
 * __tfQ29gt2k_menut11SmartIndexT1Zi (0x0011A270, no callers; T251's
 * header lists the others).
 *
 * The accessors (0x0011A118 ..) are inline (no callers; the other units
 * read the members). */
class GtmodeControl : public ControlBase {
public:
    /* The pages get this as their control (pageManager = &pm); infoMode in
     * [0, 6) at 2; prevPower is not set. */
    GtmodeControl();                        /* 0x00119778 */
    /*  [0] +0x0C 0x0011A220 ~GtmodeControl: implicit (see above) */
    virtual void waitFrame();               /* [1] +0x14 0x00119890 debugPause(&render), then
                                               ControlBase's */

    void loadPages();                       /* 0x001198C0 pm.loadFile("menu/gt_mode.mbl") */
    void unloadPages();                     /* 0x001198E8 */
    /* attach(), the PageManager on render.root and the cursor memory, the
     * tables; the ridden car to GameStatus::setRunningCar again. */
    void begin();                           /* 0x00119908 */
    void end();                             /* 0x00119A70 pm.finalize(), gfx_vsync(2), render.end(),
                                               detach() */
    /* Runs page until the PageManager exits; requestRoot() ends it early. */
    void runPage(const char *page);         /* 0x00119AB0 */
    /* The root page over the title movie; frames without input on "root"
     * past the event data base's idle time (s * 60) make the PageManager
     * exit.  Returns pm.decided.  pageArg is not used. */
    int  runRootPage(const char *page, int pageArg);    /* 0x00119B60 */
    /* MenuStatus::getPage ("root" / "replay_theater": the root page), the
     * pages until one names where to go (cut at the last '_' for an
     * "_entry" page in nextMode 2 / 3 / 4), MenuStatus::setPage(name, -1).
     * mode before each root page: 1 after requestRoot, else by nextMode
     * (0x0B 2, 0x1A 3, 0x19 4, 0x18 5, 9 6, else 0).  Returns the last
     * runRootPage (1 if none ran). */
    int  mainLoop();                        /* 0x00119D38 */
    int  run();                             /* 0x0011A040 loadPages, begin, mainLoop, end, unloadPages */
    void runTrade();                        /* 0x0011A098 "hm/hm_gr_trade", then setPage("hm/hm", 2) */
    /* Inline accessors (egcs's copies 0x0011A118..0x0011A1B8, in reverse
     * declaration order, have no callers; T036 / T037 expand getButtonList). */
    ButtonList *getButtonList() { return pm.buttonLists[pm.popupLevel]; }  /* 0x0011A1B8 (+0xDD4 /
                                               +0xDC8 from this) */
    PageManager *getPageManager() { return &pm; }               /* 0x0011A1B0 */
    void requestRoot() { rootRequest = true; }                  /* 0x0011A1A0 */
    void requestMap() { mapRequest = true; }                    /* 0x0011A190 */
    void setMode(int v) { mode = v; }                           /* 0x0011A188 */
    int  getMode() { return mode; }                             /* 0x0011A180 */
    void setSortType(GarageSortOrder v) { sortType = v; }       /* 0x0011A178 */
    GarageSortOrder getSortType() { return sortType; }          /* 0x0011A170 */
    void setInfoMode(int v) { infoMode.set(v); }                /* 0x0011A150 */
    SmartIndexT<int> *getInfoMode() { return &infoMode; }       /* 0x0011A148 */
    void setPrevPower(int v) { prevPower = v; }                 /* 0x0011A140 */
    int  getPrevPower() { return prevPower; }                   /* 0x0011A138 */
    GtmodeRender *getRender() { return &render; }               /* 0x0011A130 */
    RootObject *getRoot() { return &render.root; }              /* 0x0011A128 */
    void setOilCylinder(OilChangeCylinder *c) { oilCylinder = c; }  /* 0x0011A120 */
    OilChangeCylinder *getOilCylinder() { return oilCylinder; } /* 0x0011A118 */

    GtmodeRender      render;               /* 0x28C */
    PageManager       pm;                   /* 0x2E4 */
    u8                unk_E40[4];           /* 0xE40 ? (not set by the constructor) */
    /* The two requests are bools (4 bytes in egcs, lw / sw): the
     * constructor's stores of them are not ordered with infoMode's int
     * stores, only with its bool clamp / wrapped (u009 notes). */
    bool              rootRequest;          /* 0xE44 set: leave the page for the root page */
    int               mode;                 /* 0xE48 how the root page was entered (0..6): its
                                               cursor (T048 cursor_from_owner) */
    bool              mapRequest;           /* 0xE4C set: leave the root page for "map" */
    int               prevPower;            /* 0xE50 the power before a part was fitted (T036
                                               "car_previous_torque") */
    GarageSortOrder   sortType;             /* 0xE54 the garage's sort order (T037 "sort_*";
                                               GarageButtonList) */
    SmartIndexT<int>  infoMode;             /* 0xE58 the garage's info column, [0, 6)
                                               (GarageButtonList infoMode) */
    OilChangeCylinder *oilCylinder;         /* 0xE70 the page's "oil_change_cylinder" (T036), NULL:
                                               none */
};
EGCS_SIZE(gt2k_menu::GtmodeControl, 0xE74);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates and places them itself */
extern "C" {
void *__tfQ29gt2k_menu13GtmodeControl(void);    /* 0x0011A1D0 */
/* (__ti16RegurationResult, which the plan put here, is declared with its
 * class in T045_gtmode_page_checks.h.) */
}
#endif

#endif /* GAME_T043_GTMODE_CONTROL_H */
