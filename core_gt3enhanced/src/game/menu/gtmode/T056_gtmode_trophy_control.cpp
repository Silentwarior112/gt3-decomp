/* T056 - gt2k_menu::GTmodeTrophyControl and the entry point of the mini prize
 * screens.  0x00137840..0x00138C48.
 *
 * trophy_prize_screen (called by the race modes of T052 and T053) shows,
 * after a race, one of three screens on a 0x40000-byte menu heap, each with
 * the trophy model (a 0x32000-byte buffer for the file):
 *   - type 1..4 (arcade, RaceDataBase::getTrophyType): ArcadeTrophyControl
 *     (its page methods are MiniPrizeControlBase's, T104) for the first
 *     three places;
 *   - type 5..8 (GT mode) with a prize: GTmodeTrophyControl for the first
 *     three places, ConsolationControl (T057) for the others or when the
 *     caller asks for it.  The prize money is then added to the money and to
 *     the total prize money.
 * The trophy is recorded in the GameStatus (count and the small-trophy
 * bits).  Each screen is a ControlBaseWithPageManager running one page of
 * its .mbl file until the page is left; the item factories (MenuHooks) of
 * the page are registered from a table (T056_static_init for this screen).
 *
 * This translation unit also has the vtables, the out-of-line copies of the
 * implicit destructors and the typeinfo functions of the classes it builds
 * (ConsolationControl, GTmodeTrophyControl, ArcadeTrophyControl,
 * MiniPrizeControlBase, ControlBaseWithPageManager) and unreferenced copies
 * of PrizeCounter's. */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T056's .rodata (0x00317590..0x00317ADF): "bad_alloc" (<new>, read first),
 * "%s Cr. %s" (T099's inline PrizeCounter items), the functions' literals,
 * the five vtables, then the type names ConsolationControl,
 * GTmodeTrophyControl, ArcadeTrophyControl, MiniPrizeControlBase,
 * TrophyGetControl, the three PrizeCounter items, PrizeCounter,
 * PrizeControlBase, ControlBaseWithPageManager, SmartIndexT<int>,
 * CallBackWhenPrizeRendering, PrizeModelSetContext, the OperatorHolder
 * group, UnitArena<64>, RegurationResult, RecordsMenuArg, the garage lists,
 * Operator (no LicenseGetControl, TrophyItem: T060's header includes only
 * T059's PrizeControlBase).  Its .sdata (0x003515B8): T099's inline "" and
 * "Cr. %s", the place texts, "count", g_003515F0.  ref/data/u019/notes.md. */
// DATA_START 0x00317590
// DATA_START 0x003515B8
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus (the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the OperatorHolder group) */
#include "game/menu/gtmode/T460_prize_base.h"           /* PrizeBase (PrizeModelSetContext, CallBackWhenPrizeRendering) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (SmartIndexT<int>) */
#include "game/menu/gtmode/T060_trophy_get.h"           /* TrophyGetControl (create_get_cr_prize; ControlBaseWithPageManager,
                                               PrizeControlBase, PrizeCounter and its items) */
#include "game/menu/gtmode/T056_gtmode_trophy_control.h" /* MiniPrizeControlBase .. ConsolationControl */
/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/gtmode/T058_trophy_item.h"          /* create_trophy_gtmode */
#include "game/menu/gtmode/T104_mini_prize_control.h"   /* MiniPrizeInfo */
#include "game/race/modes/T145_mini_prize.h"           /* MiniPrize (setup, loadPrize) */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, MenuTexture */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuHook */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C / g_00352750 */
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play_if_changed */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "game/common/T494_text_utils.h"           /* format_number */
#include "engine/pdistd/T532_heap.h"                 /* memalign, free */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/gtmode/trophy_internal.h"

using namespace gt2k_menu;

/* GTmodeTrophyControl's item factories, filled by T056_static_init (.ctors).
 * A file static: retail has it in .bss (a C++ global would be zeros in
 * .data), the only item of T056's .bss (0x0049BCE8..0x0049BD60, then T057's
 * g_0049BD60). */
static MenuHook g_0049BCE8[5];              /* 0x0049BCE8 */

/* ------------------------------------------------------------------------ */
/* Item factories (MenuHooks; no direct callers)                            */
/* ------------------------------------------------------------------------ */
/* ctx is the screen; parent and p8 .. p14 are not used.  The item's texture
 * is looked up before Object::operator new (the menu heap); the landing pads
 * run Object::operator delete if Item's constructor throws. */

/* "rank_disp" (the GT mode, consolation and arcade tables): the place as
 * text.  A place first: retail branches (bltz) to the "Next ..." text,
 * which follows the switch. */
// FUNC 0x00137840 create_rank_disp
Transitive *create_rank_disp(void *ctx, PageLayout *layout, Object *parent,
                             MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MiniPrizeControlBase *screen = (MiniPrizeControlBase *)ctx;
    char text[0x40];                        /* sp+0x00 */
    MenuTexture *tex = layout->getTexture(item);
    int rank = screen->rank;
    if (rank >= 0) {
        switch (rank) {
        case RANK_FIRST:
            sprintf(text, "1st");
            break;
        case RANK_SECOND:
            sprintf(text, "2nd");
            break;
        case RANK_THIRD:
            sprintf(text, "3rd");
            break;
        default:
            sprintf(text, "%dth", rank + 1);
            break;
        }
    } else {
        sprintf(text, "Next, let's do your best.");
    }
    /* Object::operator new(0x100), Item::Item(item, tex) */
    Item *label = new Item(item, tex); /* implicit: Object::operator delete (if the constructor throws) */
    label->setText(text);
    return label;
}

/* No callers (no table has it): "Get Cr. <prize>" with the prize money of a
 * mini prize screen. */
// FUNC 0x00137988 create_get_cr_mini
Transitive *create_get_cr_mini(void *ctx, PageLayout *layout, Object *parent,
                               MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MiniPrizeControlBase *screen = (MiniPrizeControlBase *)ctx;
    char num[0x40];                         /* sp+0x00 */
    char text[0x80];                        /* sp+0x40 */
    MenuTexture *tex = layout->getTexture(item);
    format_number(num, screen->info.prize);
    sprintf(text, "Get Cr. %s", num);
    /* Object::operator new(0x100), Item::Item(item, tex); Object::operator
     * delete on a throw */
    Item *label = new Item(item, tex); /* implicit: Object::operator delete (if the constructor throws) */
    label->setText(text);
    return label;
}

/* No callers: the same with TrophyGetControl::money (the prize money, T060;
 * a member of the derived screen, see T059's PrizeControlBase). */
// FUNC 0x00137A50 create_get_cr_prize
Transitive *create_get_cr_prize(void *ctx, PageLayout *layout, Object *parent,
                                MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    TrophyGetControl *screen = (TrophyGetControl *)ctx;
    char num[0x40];                         /* sp+0x00 */
    char text[0x80];                        /* sp+0x40 */
    MenuTexture *tex = layout->getTexture(item);
    format_number(num, screen->money);
    sprintf(text, "Get Cr. %s", num);
    /* Object::operator new(0x100), Item::Item(item, tex); Object::operator
     * delete on a throw */
    Item *label = new Item(item, tex); /* implicit: Object::operator delete (if the constructor throws) */
    label->setText(text);
    return label;
}

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* GTmodeTrophyControl                                                      */
/* ------------------------------------------------------------------------ */

/* The menu file, and the trophy model of info into modelBuf (hidden until
 * the "render_model" item shows it): MiniPrize's inline setup (T145; its
 * arguments are evaluated first, so modelBuf is loaded before the two flag
 * stores, as in retail). */
// FUNC 0x00137B18 gt2k_menu::GTmodeTrophyControl::load
void GTmodeTrophyControl::load()
{
    pages.loadFile("menu/gtmode_mini.mbl");
    prize.setup(&info, 0, 0, modelBuf, PRIZE_MODEL_BUF_SIZE, 1);
    /* implicit: loadPrize (the inline setup) */
}

// FUNC 0x00137B70 gt2k_menu::GTmodeTrophyControl::unload
void GTmodeTrophyControl::unload()
{
    pages.unloadFile();
}

/* The pads, one frame, the render on screen (leaving through a fade-out),
 * the item factories in both layouts, no ExecuteFuncData.  The stores to
 * render and pages are the inline accessors of T246 / T211 (RenderBase::
 * setMask / setTransition, PageManager::setHooks / setExecuteFuncData;
 * their out-of-line copies have no callers): retail keeps &render in $s0
 * and &pages in $s2, the inlines' `this'. */
// FUNC 0x00137B90 gt2k_menu::GTmodeTrophyControl::begin
void GTmodeTrophyControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    /* transition before mask: setTransition stores `on' first (T246),
     * and retail's mask store comes last */
    render.setTransition(true, TRANSITION_FADEOUT);
    render.setMask(false);
    pages.initialize(&render.root, NULL);
    pages.setHooks(g_0049BCE8, 5, this);
    pages.setExecuteFuncData(NULL, 0, this);
    g_00352750 = 0;                         /* no screen clear */
    g_0035274C = 0;
}

// FUNC 0x00137C50 gt2k_menu::GTmodeTrophyControl::end
void GTmodeTrophyControl::end()
{
    bgm_stop();
    pages.finalize();
    render.detach();
    gfx_vsync(2);
    detach();
}

/* The fanfare; page "gtmode_mini", frozen (no input) until the prize
 * counter is done, then until the page is left. */
// FUNC 0x00137C98 gt2k_menu::GTmodeTrophyControl::mainLoop
void GTmodeTrophyControl::mainLoop()
{
    PageManager *pm = &pages;
    menu_bgm_play_if_changed(BGM_PRIZE_FANFARE, 0, 1);
    waitFrame();
    pm->setFreeze(1);
    pm->setPage("gtmode_mini", NULL);
    pm->startPage(NULL);
    while (pages.exited == 0) {             /* !isExit(), expanded */
        if (counter.isDone())
            pm->setFreeze(0);
        pm->update();
        waitFrame();
    }
    pm->closePage();
}

// FUNC 0x00137D78 gt2k_menu::GTmodeTrophyControl::run
void GTmodeTrophyControl::run()
{
    load();
    begin();
    mainLoop();
    end();
    unload();
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* The entry point                                                          */
/* ------------------------------------------------------------------------ */

/* Returns 1 if a screen was shown.  type: the race's trophy type 1..8;
 * rank: 0 = first (the range checks rank 0..2 and type 1..8 are single
 * unsigned compares in the binary).  The controller is on the stack (the
 * arcade and GT mode ones at sp+0x000, the consolation one at sp+0xF00). */
// FUNC 0x00137DC0 trophy_prize_screen
int trophy_prize_screen(int prize, int rank, int type, int consolation)
{
    /* The screens' constructors are inline: ControlBase::ControlBase, the
     * ControlBaseWithPageManager vptr, PageManager::PageManager(this), the
     * MiniPrizeControlBase vptr, MiniPrize::MiniPrize, the MiniPrizeInfo
     * (type / rank -1, the prize, then MiniPrizeInfo::set),
     * RenderBase::RenderBase, modelBuf / rank, the class's vptr, and for
     * the GT mode and consolation screens getGameStatus()->getMoney()
     * twice and PrizeCounter::PrizeCounter(money, money + prize).  Their
     * implicit destructors run at the end of each block: the counter's vptr
     * (its inline destructor), RootObject::~RootObject and
     * GameObject::~GameObject (render), MiniPrize::~MiniPrize,
     * ControlBase::~ControlBase (all with flags 2); the landing pads
     * destroy what was built when a constructor throws. */
    /* implicit: ControlBase, PageManager, MiniPrize, set, RenderBase, PrizeCounter (the inline screen constructors), ~RootObject, ~GameObject, ~MiniPrize, ~ControlBase (their implicit destructors, inline, and landing pads) */
    if (prize < 0)
        return 0;
    if (type < TROPHY_ARCADE_EASY || type > TROPHY_GT_SPECIAL)
        return 0;
    int arcade = type < TROPHY_GT_EASY;
    if (arcade) {
        if (rank < RANK_FIRST || rank > RANK_THIRD)
            return 0;
    } else if (prize == 0) {
        return 0;
    }

    menu_draw_reset();
    void *modelBuf = memalign(0x40, PRIZE_MODEL_BUF_SIZE);  /* $fp */
    void *heap = memalign(0x40, PRIZE_MENU_HEAP_SIZE);      /* sp+0x1E30 */
    if (heap == NULL || modelBuf == NULL) {
        if (heap != NULL)
            free(heap);
        if (modelBuf != NULL)
            free(modelBuf);
        return 0;
    }
    menu_heap_push(heap, PRIZE_MENU_HEAP_SIZE);

    if (arcade) {
        ArcadeTrophyControl screen(modelBuf, rank, type);       /* sp+0x000 */
        screen.run();                       /* MiniPrizeControlBase::run (T104) */
        gfx_vsync(1);
        getGameStatus()->incTrophyNum(1);
        if (rank == RANK_FIRST) {
            switch (type) {
            case TROPHY_ARCADE_EASY:
                getGameStatus()->setSmallTrophy(SMALL_TROPHY_ARCADE_EASY);
                break;
            case TROPHY_ARCADE_NORMAL:
                getGameStatus()->setSmallTrophy(SMALL_TROPHY_ARCADE_NORMAL);
                break;
            case TROPHY_ARCADE_HARD:
                getGameStatus()->setSmallTrophy(SMALL_TROPHY_ARCADE_HARD);
                break;
            case TROPHY_ARCADE_SPECIAL:
                getGameStatus()->setSmallTrophy(SMALL_TROPHY_ARCADE_SPECIAL);
                break;
            }
        }
    } else {
        if (rank < RANK_FIRST || rank > RANK_THIRD || consolation != 0) {
            ConsolationControl screen(modelBuf, rank, type, prize);    /* sp+0xF00 */
            screen.run();
            getGameStatus()->setSmallTrophy(SMALL_TROPHY_CONSOLATION);
        } else {
            GTmodeTrophyControl screen(modelBuf, rank, type, prize);   /* sp+0x000 */
            screen.run();
            getGameStatus()->incTrophyNum(1);
            /* bit (type - 5) * 3 + rank: retail tests the type with a chain
             * of bne (an if / else if chain; a switch on it is a compare
             * tree) and the rank with a switch's tree */
            if (type == TROPHY_GT_EASY) {
                switch (rank) {
                case RANK_FIRST:
                    getGameStatus()->setSmallTrophy(0x1);
                    break;
                case RANK_SECOND:
                    getGameStatus()->setSmallTrophy(0x2);
                    break;
                case RANK_THIRD:
                    getGameStatus()->setSmallTrophy(0x4);
                    break;
                }
            } else if (type == TROPHY_GT_NORMAL) {
                switch (rank) {
                case RANK_FIRST:
                    getGameStatus()->setSmallTrophy(0x8);
                    break;
                case RANK_SECOND:
                    getGameStatus()->setSmallTrophy(0x10);
                    break;
                case RANK_THIRD:
                    getGameStatus()->setSmallTrophy(0x20);
                    break;
                }
            } else if (type == TROPHY_GT_HARD) {
                switch (rank) {
                case RANK_FIRST:
                    getGameStatus()->setSmallTrophy(0x40);
                    break;
                case RANK_SECOND:
                    getGameStatus()->setSmallTrophy(0x80);
                    break;
                case RANK_THIRD:
                    getGameStatus()->setSmallTrophy(0x100);
                    break;
                }
            } else if (type == TROPHY_GT_SPECIAL) {
                switch (rank) {
                case RANK_FIRST:
                    getGameStatus()->setSmallTrophy(0x200);
                    break;
                case RANK_SECOND:
                    getGameStatus()->setSmallTrophy(0x400);
                    break;
                case RANK_THIRD:
                    getGameStatus()->setSmallTrophy(0x800);
                    break;
                }
            }
        }
        getGameStatus()->incPrize(prize);
        getGameStatus()->incMoney(prize);
        gfx_vsync(1);
    }
    menu_heap_pop();
    free(heap);
    free(modelBuf);
    return 1;
}

/* Static initialiser (.ctors entry 0x0028F068): the item factories of
 * GTmodeTrophyControl.  Compiler-generated in the original (the table
 * defined here with MenuHook objects); written out the way build_vec_init
 * expands it (HOOK, trophy_internal.h): an exact form, not the original; it stores the
 * same 5 entries. */
// FUNC 0x00138790 T056_static_init
void T056_static_init(void)
{
    MenuHook *base = g_0049BCE8;
    int left = 4;                           /* build_vec_init's iterator: elements - 1 */

    HOOK("count",          create_count_T056,          0, 0, 0, 0);
    HOOK("count_shadow",   create_count_shadow_T056,   0, 0, 0, 0);
    HOOK("get_money_disp", create_get_money_disp_T056, 0, 0, 0, 0);
    HOOK("rank_disp",      create_rank_disp,           (int)&g_003515F0, (int)&g_003515F0, 0, 0);  /* gp-0x6400: 1.25 */
    HOOK("render_model",   create_trophy_gtmode,       0, 0, 0, 0);
}

/* The "rank_disp" entry's p8 / pC (create_rank_disp does not read them), as
 * T104's g_003515B0: retail has it in T056's .sdata after the static
 * initialiser's "count", i.e. written after the initialiser was compiled. */
f32 g_003515F0 = 1.25f;                     /* 0x003515F0 */

/* ------------------------------------------------------------------------ */
/* RTTI and the vtable destructors                                          */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * MiniPrizeControlBase). */
// FUNC 0x00138878 __tfQ29gt2k_menu18ConsolationControl
extern "C" void *__tfQ29gt2k_menu18ConsolationControl(void)
{
    if (__tiQ29gt2k_menu18ConsolationControl[0] == 0) {
        __tfQ29gt2k_menu20MiniPrizeControlBase();
        __rtti_si(__tiQ29gt2k_menu18ConsolationControl, "Q29gt2k_menu18ConsolationControl",
                  (const type_info *)__tiQ29gt2k_menu20MiniPrizeControlBase);
    }
    return __tiQ29gt2k_menu18ConsolationControl;
}
#endif

// FUNC 0x001388C8 gt2k_menu::ConsolationControl::~ConsolationControl
/* the implicit destructor (game/menu/gtmode/T056_gtmode_trophy_control.h), no vptr store
 * of its own: the counter's vptr (+0xF28) = 0x00313B08 (PrizeCounter's
 * inline destructor), render (RootObject::~RootObject +0xE94,
 * GameObject::~GameObject +0xE90), MiniPrize::~MiniPrize (+0xDE8), all with
 * flags 2, then ControlBase::~ControlBase(this, flags), which does the
 * operator delete */
/* implicit: ~RootObject, ~GameObject (member destructors of render), ~MiniPrize (member destructor), ~ControlBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * MiniPrizeControlBase). */
// FUNC 0x00138940 __tfQ29gt2k_menu19GTmodeTrophyControl
extern "C" void *__tfQ29gt2k_menu19GTmodeTrophyControl(void)
{
    if (__tiQ29gt2k_menu19GTmodeTrophyControl[0] == 0) {
        __tfQ29gt2k_menu20MiniPrizeControlBase();
        __rtti_si(__tiQ29gt2k_menu19GTmodeTrophyControl, "Q29gt2k_menu19GTmodeTrophyControl",
                  (const type_info *)__tiQ29gt2k_menu20MiniPrizeControlBase);
    }
    return __tiQ29gt2k_menu19GTmodeTrophyControl;
}
#endif

// FUNC 0x00138990 gt2k_menu::GTmodeTrophyControl::~GTmodeTrophyControl
/* the implicit destructor (game/menu/gtmode/T056_gtmode_trophy_control.h), the same code
 * as ~ConsolationControl: the counter's vptr, RootObject::~RootObject,
 * GameObject::~GameObject, MiniPrize::~MiniPrize, then
 * ControlBase::~ControlBase(this, flags) */
/* implicit: ~RootObject, ~GameObject (member destructors of render), ~MiniPrize (member destructor), ~ControlBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * MiniPrizeControlBase). */
// FUNC 0x00138A08 __tfQ29gt2k_menu19ArcadeTrophyControl
extern "C" void *__tfQ29gt2k_menu19ArcadeTrophyControl(void)
{
    if (__tiQ29gt2k_menu19ArcadeTrophyControl[0] == 0) {
        __tfQ29gt2k_menu20MiniPrizeControlBase();
        __rtti_si(__tiQ29gt2k_menu19ArcadeTrophyControl, "Q29gt2k_menu19ArcadeTrophyControl",
                  (const type_info *)__tiQ29gt2k_menu20MiniPrizeControlBase);
    }
    return __tiQ29gt2k_menu19ArcadeTrophyControl;
}
#endif

// FUNC 0x00138A58 gt2k_menu::ArcadeTrophyControl::~ArcadeTrophyControl
/* the implicit destructor (game/menu/gtmode/T056_gtmode_trophy_control.h), no vptr
 * store: render (RootObject::~RootObject +0xE94, GameObject::~GameObject
 * +0xE90), MiniPrize::~MiniPrize (+0xDE8), all with flags 2, then
 * ControlBase::~ControlBase(this, flags) */
/* implicit: ~RootObject, ~GameObject (member destructors of render), ~MiniPrize (member destructor), ~ControlBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * ControlBaseWithPageManager). */
// FUNC 0x00138AB8 __tfQ29gt2k_menu20MiniPrizeControlBase
extern "C" void *__tfQ29gt2k_menu20MiniPrizeControlBase(void)
{
    if (__tiQ29gt2k_menu20MiniPrizeControlBase[0] == 0) {
        __tfQ29gt2k_menu26ControlBaseWithPageManager();
        __rtti_si(__tiQ29gt2k_menu20MiniPrizeControlBase, "Q29gt2k_menu20MiniPrizeControlBase",
                  (const type_info *)__tiQ29gt2k_menu26ControlBaseWithPageManager);
    }
    return __tiQ29gt2k_menu20MiniPrizeControlBase;
}
#endif

// FUNC 0x00138B08 gt2k_menu::MiniPrizeControlBase::~MiniPrizeControlBase
/* the implicit destructor (game/menu/gtmode/T056_gtmode_trophy_control.h), the same code
 * as ~ArcadeTrophyControl: RootObject::~RootObject, GameObject::~GameObject,
 * MiniPrize::~MiniPrize, then ControlBase::~ControlBase(this, flags) */
/* implicit: ~RootObject, ~GameObject (member destructors of render), ~MiniPrize (member destructor), ~ControlBase (base destructor) */

// FUNC 0x00138B68 gt2k_menu::PrizeCounter::~PrizeCounter
/* inline in game/menu/gtmode/T099_prize_counter.h; egcs emits this copy in T056 too
 * (unreferenced: T056's vtable copy 0x003176A8 points at T099's 0x0012A240,
 * the linker keeps one): stores the vptr (+0x30) = 0x00313B08, then the
 * global operator delete (0x0027C0C0) if flags & 1 */
/* implicit: operator delete (deleting destructor, the inline ~PrizeCounter in the header) */

#ifndef BOOT_OLD_GCC
/* T056's copy of PrizeCounter's typeinfo function (unreferenced; T099's is
 * 0x0012A270).  No bases: __rtti_user.  Weak: T099's definition wins at
 * link time, as in the original. */
// FUNC 0x00138B98 __tfQ29gt2k_menu12PrizeCounter
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menu12PrizeCounter(void)
{
    if (__tiQ29gt2k_menu12PrizeCounter[0] == 0)
        __rtti_user(__tiQ29gt2k_menu12PrizeCounter, "Q29gt2k_menu12PrizeCounter");
    return __tiQ29gt2k_menu12PrizeCounter;
}

/* Compiler-generated typeinfo function (__si_type_info, base ControlBase;
 * the one all five vtables use: also T059, T060, T105 and T061's
 * CarGetControl family). */
// FUNC 0x00138BD8 __tfQ29gt2k_menu26ControlBaseWithPageManager
extern "C" void *__tfQ29gt2k_menu26ControlBaseWithPageManager(void)
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

// FUNC 0x00138C28 gt2k_menu::ControlBaseWithPageManager::~ControlBaseWithPageManager
/* the implicit destructor (game/menu/gtmode/T056_gtmode_trophy_control.h): pages has no
 * destructor, so ControlBase::~ControlBase(this, flags) only (a call, not a
 * tail jump); in all five vtables */
/* implicit: ~ControlBase (base destructor) */
