/* T104 - the non-virtual methods of gt2k_menu::MiniPrizeControlBase (its
 * class, constructor, destructor and RTTI are T056's), the methods of
 * MiniPrizeInfo (the trophy record at its +0xE84) and the static
 * initialiser of its item factory table.  0x00137468..0x00137840.
 *
 * The arcade trophy screen (trophy_prize_screen, T056: an
 * ArcadeTrophyControl on its stack, the record built from its arguments, a
 * 0x32000-byte model buffer from memalign) runs MiniPrizeControlBase::run:
 * "menu/arcade_mini.mbl", the trophy model loaded into the buffer, page
 * "arcade_mini" with the factories "rank_disp" and "render_model".
 *
 * Retail's data (u018's data pass): .rodata 0x00317120..0x00317590,
 * "bad_alloc" (<new>, read first), "%s Cr. %s" (T099's inline
 * PrizeCounterRemainItem::virtual_2, written where its header is read),
 * MiniPrizeInfo::set's jump table, the literals, then the type names
 * GTmodeTrophyControl, ArcadeTrophyControl, MiniPrizeControlBase,
 * TrophyGetControl, the prize counter's four, PrizeControlBase,
 * ControlBaseWithPageManager, SmartIndexT<int>, CallBackWhenPrizeRendering,
 * PrizeModelSetContext, OperatorList .. Cell<OperatorHolder>, UnitArena<64>,
 * RegurationResult, RecordsMenuArg, the garage's, Operator: the same set and
 * order as retail T060's (without ConsolationControl; T060 is the trophy
 * get screen: the original T104 saw its header), which the includes below
 * give.  .sdata 0x003515A0: "" and "Cr. %s" (T099's inline items, read
 * with the header), then g_003515B0.  .bss: g_0049BCB8 (a file static). */
#include <new>                              /* first: "bad_alloc" */
#include "types.h"
#include "game/save/T429_game_status.h"          /* (no use: the garage's type names) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (retail has it) */
#include "game/race/core/T153_race_course.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* (no use: the menu classes) */
#include "game/menu/gtmode/T460_prize_base.h"           /* (no use: PrizeModelSetContext, CallBackWhenPrizeRendering) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (SmartIndexT<int>) */
#include "game/menu/gtmode/T060_trophy_get.h"           /* (no use: ControlBaseWithPageManager .. TrophyGetControl) */
#define T056_NO_CONSOLATION                 /* retail T104 has no ConsolationControl (as T060) */
#include "game/menu/gtmode/T056_gtmode_trophy_control.h" /* gt2k_menu::MiniPrizeControlBase, create_rank_disp */
#include "game/menu/gtmode/T058_trophy_item.h"          /* create_trophy_arcade */
#include "game/menu/gtmode/T104_mini_prize_control.h"   /* MiniPrizeInfo, g_003515B0 */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook, MenuHookFn */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_0035274C, g_00352750 */
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play_if_changed */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "game/debug/logger_internal.h"

// DATA_START 0x00317120
// DATA_START 0x003515A0

using namespace gt2k_menu;

/* MiniPrizeControlBase's item factories (filled by T104_static_init; a file
 * static: retail has it in .bss, local to T104). */
static MenuHook g_0049BCB8[2];              /* 0x0049BCB8 */

/* No callers (an out-of-line copy): 1 for the arcade trophy types 1..4. */
// FUNC 0x00137468 MiniPrizeInfo::isArcade
int MiniPrizeInfo::isArcade()
{
    /* Nested tests: `type < 5 && type > 0' folds into one range test
     * ((unsigned)(type - 1) < 4), retail keeps both compares. */
    if (type < 5) {
        if (type > 0)
            return 1;
    }
    return 0;
}

/* No callers. */
// FUNC 0x00137488 MiniPrizeInfo::clearRank
void MiniPrizeInfo::clearRank()
{
    rank = -1;
}

/* type is taken when it is 1..8 (a jump table of 8 cases, each storing its
 * constant), else left as it is; rank: 0, 1, 2 are taken, anything else
 * gives -1.  Caller: trophy_prize_screen (MiniPrizeInfo's inline
 * constructor). */
// FUNC 0x00137498 MiniPrizeInfo::set
void MiniPrizeInfo::set(int r, int t)
{
    switch (t) {
    case 1: type = 1; break;
    case 2: type = 2; break;
    case 3: type = 3; break;
    case 4: type = 4; break;
    case 5: type = 5; break;
    case 6: type = 6; break;
    case 7: type = 7; break;
    case 8: type = 8; break;
    }
    /* One case per rank (CSE turns `rank = 1' / `rank = 2' into stores of
     * r and jump2 merges them: retail's single `sw a1'). */
    switch (r) {
    case 0: rank = 0; break;
    case 1: rank = 1; break;
    case 2: rank = 2; break;
    default: rank = -1; break;
    }
}

namespace gt2k_menu {

/* The page file, then the trophy of info into the caller's buffer (the
 * prize hidden and not animated: dispFlag / updateFlag cleared first; keep
 * 1: the buffer is the caller's). */
// FUNC 0x00137550 gt2k_menu::MiniPrizeControlBase::loadPages
void MiniPrizeControlBase::loadPages()
{
    pages.loadFile("menu/arcade_mini.mbl");
    prize.setup(&info, 0, 0, modelBuf, 0x32000, 1);
    /* implicit: loadPrize (the inline setup) */
}

// FUNC 0x001375A8 gt2k_menu::MiniPrizeControlBase::unloadPages
void MiniPrizeControlBase::unloadPages()
{
    pages.unloadFile();
}

/* The pads attached and one frame of the ControlBase, the renderer on
 * screen (no mask), the object tree, the two item factories (g_0049BCB8) in
 * both layouts, no page actions (the execute table {NULL, 0, this}); no
 * screen clear.  The inline members RenderBase::setMask / getRootObject and
 * PageManager::setHooks / setExecuteFuncData: retail keeps &render and
 * &pages in s-registers and stores through them. */
// FUNC 0x001375C8 gt2k_menu::MiniPrizeControlBase::begin
void MiniPrizeControlBase::begin()
{
    attach();
    waitFrame();
    render.attach();
    render.setMask(false);
    pages.initialize(render.getRootObject(), NULL);
    pages.setHooks(g_0049BCB8, 2, this);
    pages.setExecuteFuncData(NULL, 0, this);
    g_00352750 = 0;
    g_0035274C = 0;
}

// FUNC 0x00137678 gt2k_menu::MiniPrizeControlBase::end
void MiniPrizeControlBase::end()
{
    bgm_stop();
    pages.finalize();
    render.detach();
    gfx_vsync(2);
    detach();
}

/* The page's music, then page "arcade_mini" opened frozen (setFreeze(1))
 * for 180 vsyncs (3 s), then run until the page manager exits; the page is
 * closed. */
// FUNC 0x001376C0 gt2k_menu::MiniPrizeControlBase::mainLoop
void MiniPrizeControlBase::mainLoop()
{
    menu_bgm_play_if_changed(BGM_GROUP_ARCADE_MINI, 0, 1);
    waitFrame();
    pages.setPage("arcade_mini", NULL);
    pages.setFreeze(1);
    pages.startPage(NULL);
    gfx_vsync(180);
    pages.setFreeze(0);
    while (pages.exited == 0) {
        pages.update();
        waitFrame();
    }
    pages.closePage();
}

/* Caller: trophy_prize_screen (T056). */
// FUNC 0x00137790 gt2k_menu::MiniPrizeControlBase::run
void MiniPrizeControlBase::run()
{
    loadPages();
    begin();
    mainLoop();
    end();
    unloadPages();
}

} /* namespace gt2k_menu */

/* One table entry: the inline constructor of the original's MenuHook (u006's
 * form, as u019's T056_static_init). */
static inline void set_entry(MenuHook *e, const char *name, MenuHookFn *create,
                             int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* One element as egcs's build_vec_init expands it: the constructor on a
 * copy of the cursor (`base'), then the cursor and the count of the elements
 * left (`left', dead) stepped. */
#define HOOK(name, create, p8, pC, p10, p14)                                      { MenuHook *e = base; set_entry(e, name, create, p8, pC, p10, p14); }       base++;                                                                       left--

/* Static initialiser (.ctors entry 0x0028F0D4): the two item factories;
 * "rank_disp" gets &g_003515B0 (1.25) as both p8 and pC.  Compiler-generated
 * in the original (the table defined here with MenuHook objects built by a
 * constructor, filled at run time by build_vec_init); the decomp keeps the
 * table at its original address, so the function is written out the way
 * build_vec_init expands it (HOOK): an exact form, not the original; it
 * stores the same two entries. */
// FUNC 0x001377D8 T104_static_init
void T104_static_init(void)
{
    MenuHook *base = g_0049BCB8;
    int left = 1;                           /* build_vec_init's iterator: elements - 1 */

    HOOK("rank_disp",    create_rank_disp,     (int)&g_003515B0, (int)&g_003515B0, 0, 0);
    HOOK("render_model", create_trophy_arcade, 0, 0, 0, 0);
}

/* The "rank_disp" entry's p8 / pC (create_rank_disp does not read them):
 * written where it is defined, after the header's "" / "Cr. %s" (T104's
 * last .sdata item; T104_static_init's literals are .rodata).  Defined
 * after the static initialiser as u019's g_003515F0 (T056), the same
 * value. */
f32 g_003515B0 = 1.25f;                     /* 0x003515B0 */
