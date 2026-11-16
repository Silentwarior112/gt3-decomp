/* T056_gtmode_trophy_control.h - the mini prize screens shown after a race:
 * gt2k_menu::ControlBaseWithPageManager, MiniPrizeControlBase,
 * ArcadeTrophyControl, GTmodeTrophyControl, ConsolationControl
 * (translation unit T056, unit u019), 0x00137840..0x00138C48.
 *
 * trophy_prize_screen (called by the race modes of T052 / T053) shows, on a
 * 0x40000-byte menu heap with a 0x32000-byte buffer for the trophy model,
 * one of three screens: type 1..4 (arcade) the ArcadeTrophyControl for the
 * first three places; type 5..8 (GT mode) with a prize the
 * GTmodeTrophyControl for the first three places ("menu/gtmode_mini.mbl"),
 * else (or when the caller asks) the ConsolationControl
 * ("menu/consolation_prize.mbl", its methods are T057's), the prize money
 * counted up from the player's money.  The trophy goes into the GameStatus
 * (count, small-trophy bits; the prize into money and total prize).
 *
 * Hierarchy (egcs offsets): ControlBase (T247, 0x28C, vptr +0x288) ->
 * ControlBaseWithPageManager (PageManager at 0x28C; 0xDE8) ->
 * MiniPrizeControlBase (MiniPrize 0xDE8, MiniPrizeInfo 0xE84, RenderBase
 * 0xE90; 0xEF4; its page methods are T104's) -> ArcadeTrophyControl /
 * GTmodeTrophyControl / ConsolationControl (PrizeCounter 0xEF8; 0xF30).
 * PrizeControlBase (T059) and NotCarGetControl (T105) also derive from
 * ControlBaseWithPageManager.
 *
 * The constructors of these classes are inline in the original and the
 * destructors implicit (none stores a vptr): the screens' entry functions
 * expand them (the controllers are stack objects), ControlBaseWithPageManager's
 * vtable and destructor have a copy in every translation unit that builds a
 * derived screen (T056, T059, T060, T105, T061), and the destructors listed
 * below are the copies the vtables use.  PrizeItemEntry (u019 / u020) is T229's MenuHook. */
/* ControlBaseWithPageManager has a guard of its own: T059 includes only it
 * (T056_BASE_ONLY), so that T060 sees PrizeCounter (T099) after
 * PrizeControlBase, as the original did; egcs emits the weak vtables and
 * inline copies in the reverse order of the class definitions, and T060's
 * are TrophyGetControl, PrizeCounter, PrizeControlBase,
 * ControlBaseWithPageManager. */
#ifndef GAME_T056_CONTROL_BASE_WITH_PAGE_MANAGER
#define GAME_T056_CONTROL_BASE_WITH_PAGE_MANAGER

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pages) */

namespace gt2k_menu {

/* vtable 0x003176B8 (2 slots: [1] ControlBase::waitFrame; the copies
 * 0x003186C8 T059, 0x00318B18 T060, 0x00318FA0 T105, 0x003199A8 T061).
 * typeinfo __tfQ29gt2k_menu26ControlBaseWithPageManager 0x00138BD8 (si,
 * base ControlBase; storage 0x004B16A0; unreferenced copies 0x0013A860
 * T059, 0x0013B310 T060, 0x0013B7D8 T105).  vptr +0x288 (ControlBase's);
 * size 0xDE8. */
class ControlBaseWithPageManager : public ControlBase {
public:
    ControlBaseWithPageManager() : pages(this) {}
    /* pages has no destructor: ~ControlBase.  0x00138C28 is the copy the
     * vtables use; unreferenced copies 0x0013A8B0 T059, 0x0013B360 T060,
     * 0x0013B828 T105, 0x0013F888 T061. */
    /* [0] +0x0C 0x00138C28 ~ControlBaseWithPageManager: implicit (no vptr store) */

    /*          ControlBase                    0x000 .. 0x28C (vptr +0x288) */
    PageManager pages;                      /* 0x28C the screen's .mbl */
};
EGCS_SIZE(gt2k_menu::ControlBaseWithPageManager, 0xDE8);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu26ControlBaseWithPageManager(void);   /* 0x00138BD8 */
/* The __si_type_info node that u019, u020 and u021 use (PrizeControlBase's,
 * which the plan also put here, is in T059_license_get_control.h). */
extern u32 __tiQ29gt2k_menu26ControlBaseWithPageManager[3]; /* 0x004B16A0 */
}
#endif

#endif /* GAME_T056_CONTROL_BASE_WITH_PAGE_MANAGER */

#if !defined(GAME_T056_GTMODE_TROPHY_CONTROL_H) && !defined(T056_BASE_ONLY)
#define GAME_T056_GTMODE_TROPHY_CONTROL_H

#include "game/race/modes/T145_mini_prize.h"           /* MiniPrize (prize) */
#include "game/menu/gtmode/T104_mini_prize_control.h"   /* MiniPrizeInfo (info) */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/gtmode/T099_prize_counter.h"        /* gt2k_menu::PrizeCounter (counter) */
#include "game/save/T429_game_status.h"          /* getGameStatus() (the inline constructors) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (PrizeItemEntry) */

class GSBuffer;                             /* T514 (pointers only) */

/* An entry of the prize screens' item factory tables (g_0049BCE8 ..
 * g_0049BDF0): name, create, p8, pC, p10, p14 (u019 / u020: arg0, arg1,
 * unk10, unk14).  0x18 bytes. */
typedef gt2k_menu::MenuHook PrizeItemEntry;

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

/* vtable 0x00317690 (2 slots).  typeinfo
 * __tfQ29gt2k_menu20MiniPrizeControlBase 0x00138AB8 (si, base
 * ControlBaseWithPageManager; storage 0x004B16C0).  Size 0xEF4.  Its own
 * methods (T104) run the arcade trophy page "arcade_mini". */
class MiniPrizeControlBase : public ControlBaseWithPageManager {
public:
    MiniPrizeControlBase(void *buf, int r, int type, int money)
        : prize(), info(money, r, type), render()
    {
        modelBuf = buf;
        rank = r;
    }
    /* [0] +0x0C 0x00138B08 ~MiniPrizeControlBase: implicit (no vptr store; render, ~MiniPrize, ~ControlBase) */

    /* T104 */
    /* "menu/arcade_mini.mbl", then the trophy of info into modelBuf (the
     * prize hidden: dispFlag / updateFlag 0). */
    void loadPages();                       /* 0x00137550 */
    void unloadPages();                     /* 0x001375A8 */
    void begin();                           /* 0x001375C8 the item factories g_0049BCB8 in both layouts */
    void end();                             /* 0x00137678 */
    /* Page "arcade_mini", frozen for 180 vsyncs, then until it is left. */
    void mainLoop();                        /* 0x001376C0 */
    void run();                             /* 0x00137790 loadPages, begin, mainLoop, end, unloadPages */

    /*            ControlBaseWithPageManager   0x000 .. 0xDE8 */
    MiniPrize     prize;                    /* 0xDE8 the trophy model */
    MiniPrizeInfo info;                     /* 0xE84 the trophy and the prize money */
    RenderBase    render;                   /* 0xE90 */
    void         *modelBuf;                 /* 0xEE8 the caller's 0x32000 bytes for the model file */
    u8            unk_EEC[4];               /* 0xEEC (not accessed) */
    int           rank;                     /* 0xEF0 the place: 0 first, < 0 none */
};
EGCS_SIZE(gt2k_menu::MiniPrizeControlBase, 0xEF4);

/* vtable 0x00317678 (2 slots).  typeinfo
 * __tfQ29gt2k_menu19ArcadeTrophyControl 0x00138A08 (si, base
 * MiniPrizeControlBase; storage 0x004B16F0).  Size 0xEF4 (sp+0x000 in
 * trophy_prize_screen).  Runs MiniPrizeControlBase::run. */
class ArcadeTrophyControl : public MiniPrizeControlBase {
public:
    ArcadeTrophyControl(void *buf, int r, int type) : MiniPrizeControlBase(buf, r, type, 0) {}
    /* [0] +0x0C 0x00138A58 ~ArcadeTrophyControl: implicit (no vptr store) */
};
EGCS_SIZE(gt2k_menu::ArcadeTrophyControl, 0xEF4);

/* vtable 0x00317660 (2 slots).  typeinfo
 * __tfQ29gt2k_menu19GTmodeTrophyControl 0x00138940 (si, base
 * MiniPrizeControlBase; storage 0x004B1660).  Size 0xF30 (sp+0x000 in
 * trophy_prize_screen).  The counter runs from the player's money to money
 * + prize. */
class GTmodeTrophyControl : public MiniPrizeControlBase {
public:
    GTmodeTrophyControl(void *buf, int r, int type, int money)
        : MiniPrizeControlBase(buf, r, type, money),
          counter(getGameStatus()->getMoney(), getGameStatus()->getMoney() + money) {}
    /* counter's vptr, render, ~MiniPrize, ~ControlBase. */
    /* [0] +0x0C 0x00138990 ~GTmodeTrophyControl: implicit (no vptr store) */
    /* "menu/gtmode_mini.mbl", the trophy into modelBuf (hidden). */
    void load();                            /* 0x00137B18 */
    void unload();                          /* 0x00137B70 */
    /* The render with a transition; the item factories g_0049BCE8. */
    void begin();                           /* 0x00137B90 */
    void end();                             /* 0x00137C50 */
    /* Page "gtmode_mini", frozen until the counter is done. */
    void mainLoop();                        /* 0x00137C98 */
    void run();                             /* 0x00137D78 load, begin, mainLoop, end, unload */

    /*           MiniPrizeControlBase          0x000 .. 0xEF4 */
    PrizeCounter counter;                   /* 0xEF8 */
};
EGCS_SIZE(gt2k_menu::GTmodeTrophyControl, 0xF30);

/* T056_NO_CONSOLATION (u020's T060): retail T060's .rodata has the type
 * names of MiniPrizeControlBase, ArcadeTrophyControl and GTmodeTrophyControl
 * but not ConsolationControl's (T056, T058 and T099 have it, defined last),
 * so the original T060 saw this header without it (? the class was in
 * another header, T057's). */
#ifndef T056_NO_CONSOLATION
/* vtable 0x00317648 (2 slots).  typeinfo
 * __tfQ29gt2k_menu18ConsolationControl 0x00138878 (si, base
 * MiniPrizeControlBase; storage 0x004B1680).  Size 0xF30 (sp+0xF00 in
 * trophy_prize_screen).  Its methods are T057's (the vtable, destructor
 * and RTTI T056's): GTmodeTrophyControl's flow without the render's
 * transition, "menu/consolation_prize.mbl", page "consolation_prize". */
class ConsolationControl : public MiniPrizeControlBase {
public:
    ConsolationControl(void *buf, int r, int type, int money)
        : MiniPrizeControlBase(buf, r, type, money),
          counter(getGameStatus()->getMoney(), getGameStatus()->getMoney() + money) {}
    /* [0] +0x0C 0x001388C8 ~ConsolationControl: implicit (no vptr store) */

    /* T057 */
    void load();                            /* 0x00138C48 */
    void unload();                          /* 0x00138CA0 */
    void begin();                           /* 0x00138CC0 the item factories g_0049BD60 */
    void end();                             /* 0x00138D70 */
    void mainLoop();                        /* 0x00138DB8 menu_bgm_play instead of _if_changed */
    void run();                             /* 0x00138E98 */

    /*           MiniPrizeControlBase          0x000 .. 0xEF4 */
    PrizeCounter counter;                   /* 0xEF8 */
};
EGCS_SIZE(gt2k_menu::ConsolationControl, 0xF30);
#endif /* !T056_NO_CONSOLATION */

} /* namespace gt2k_menu */

/* The item factories (MenuHookFn, T229; ctx: the screen).  "rank_disp" (the
 * GT mode and consolation tables, T104's): an Item of the element with the
 * place as text ("1st", "2nd", "3rd", "%dth"; "Next, let's do your best."
 * without a place), ctx a MiniPrizeControlBase.  "Get Cr. <prize>" with a
 * MiniPrizeControlBase's info.prize / a PrizeControlBase's param: no
 * callers.  The definitions use ctx, layout (the item's texture) and item
 * only. */
gt2k_menu::Transitive *create_rank_disp(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00137840 */
gt2k_menu::Transitive *create_get_cr_mini(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00137988 */
gt2k_menu::Transitive *create_get_cr_prize(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00137A50 */
/* trophy_prize_screen: in its own header (its callers did not see the
 * classes above: u016's data pass). */
#include "game/menu/gtmode/T056_trophy_prize_entry.h"
/* .ctors entry 0x0028F068: g_0049BCE8 = "count", "count_shadow",
 * "get_money_disp" (T099 _T056), "rank_disp" (p8 = pC = &g_003515F0),
 * "render_model" (T058 create_trophy_gtmode). */
void T056_static_init(void);                /* 0x00138790 */

/* g_0049BCE8 (0x0049BCE8, .bss) GTmodeTrophyControl's item factories: a
 * file static of T056 (retail has it in .bss), defined there (u019's data
 * pass). */
extern f32 g_003515F0;                    /* 0x003515F0 gp-0x6400 1.25 (T056 .sdata): the "rank_disp"
                                               entry's p8 / pC, as T104's g_003515B0 (create_rank_disp
                                               does not read them) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu18ConsolationControl(void);           /* 0x00138878 */
void *__tfQ29gt2k_menu19GTmodeTrophyControl(void);          /* 0x00138940 */
void *__tfQ29gt2k_menu19ArcadeTrophyControl(void);          /* 0x00138A08 */
void *__tfQ29gt2k_menu20MiniPrizeControlBase(void);         /* 0x00138AB8 */
}
#endif

#endif /* GAME_T056_GTMODE_TROPHY_CONTROL_H */
