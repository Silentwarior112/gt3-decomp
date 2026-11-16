/* T057 - gt2k_menu::ConsolationControl, the consolation prize screen of the
 * GT mode ("menu/consolation_prize.mbl", page "consolation_prize").
 * 0x00138C48..0x00138FC8.
 *
 * The same flow as GTmodeTrophyControl (T056): the trophy model, the prize
 * counter, the "rank_disp" text; built by trophy_prize_screen (T056), which
 * also has this class's vtable, destructor and RTTI.  Differences: begin
 * does not set the render's transition, mainLoop starts the fanfare with
 * menu_bgm_play instead of menu_bgm_play_if_changed. */
/* The includes, in the original's class order (as T056's: retail T057 has
 * the same type names).  Retail T057's .rodata (0x00317AE0..0x00317F86):
 * "bad_alloc" (<new>), "%s Cr. %s" (T099's inline PrizeCounter items), the
 * functions' literals, then the type names (no vtables: T056 has them).
 * Its .sdata (0x003515F8): T099's inline "" and "Cr. %s", then "count".
 * ref/data/u019/notes.md. */
// DATA_START 0x00317AE0
// DATA_START 0x003515F8
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* (the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the OperatorHolder group) */
#include "game/menu/gtmode/T460_prize_base.h"           /* (PrizeModelSetContext, CallBackWhenPrizeRendering) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (SmartIndexT<int>) */
#include "game/menu/gtmode/T060_trophy_get.h"           /* (no use: ControlBaseWithPageManager, PrizeControlBase,
                                               PrizeCounter and its items, TrophyGetControl) */
#include "game/menu/gtmode/T056_gtmode_trophy_control.h" /* ConsolationControl (MiniPrizeControlBase ..) */
/* The rest adds no type name. */
#include "game/menu/gtmode/T057_consolation_control.h"  /* g_0049BD60, T057_static_init */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/gtmode/T058_trophy_item.h"          /* create_trophy_consolation */
#include "game/menu/gtmode/T099_prize_counter.h"        /* create_count_T057 .. */
#include "game/race/modes/T145_mini_prize.h"           /* MiniPrize (setup, loadPrize) */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_0035274C / g_00352750 */
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "game/menu/gtmode/trophy_internal.h"

using namespace gt2k_menu;

/* ConsolationControl's item factories, filled by T057_static_init (.ctors).
 * A file static: retail has it in .bss, the only item of T057's .bss
 * (0x0049BD60..0x0049BDD8, then T059's g_0049BDD8). */
static MenuHook g_0049BD60[5];              /* 0x0049BD60 */

namespace gt2k_menu {

/* The menu file, and the model of info into modelBuf, hidden (past the
 * third place info.rank is -1: MiniPrize loads "race/consolation.prm");
 * MiniPrize's inline setup, as GTmodeTrophyControl::load. */
// FUNC 0x00138C48 gt2k_menu::ConsolationControl::load
void ConsolationControl::load()
{
    pages.loadFile("menu/consolation_prize.mbl");
    prize.setup(&info, 0, 0, modelBuf, PRIZE_MODEL_BUF_SIZE, 1);
    /* implicit: loadPrize (the inline setup) */
}

// FUNC 0x00138CA0 gt2k_menu::ConsolationControl::unload
void ConsolationControl::unload()
{
    pages.unloadFile();
}

/* As GTmodeTrophyControl::begin without the render's transition (the same
 * inline accessors expanded). */
// FUNC 0x00138CC0 gt2k_menu::ConsolationControl::begin
void ConsolationControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    render.setMask(false);
    pages.initialize(&render.root, NULL);
    pages.setHooks(g_0049BD60, 5, this);
    pages.setExecuteFuncData(NULL, 0, this);
    g_00352750 = 0;
    g_0035274C = 0;
}

// FUNC 0x00138D70 gt2k_menu::ConsolationControl::end
void ConsolationControl::end()
{
    bgm_stop();
    pages.finalize();
    render.detach();
    gfx_vsync(2);
    detach();
}

/* The fanfare (always restarted); page "consolation_prize", frozen until
 * the prize counter is done. */
// FUNC 0x00138DB8 gt2k_menu::ConsolationControl::mainLoop
void ConsolationControl::mainLoop()
{
    PageManager *pm = &pages;
    menu_bgm_play(BGM_PRIZE_FANFARE, 0, 1);
    waitFrame();
    pm->setFreeze(1);
    pm->setPage("consolation_prize", NULL);
    pm->startPage(NULL);
    while (pages.exited == 0) {             /* !isExit(), expanded */
        if (counter.isDone())
            pm->setFreeze(0);
        pm->update();
        waitFrame();
    }
    pm->closePage();
}

// FUNC 0x00138E98 gt2k_menu::ConsolationControl::run
void ConsolationControl::run()
{
    load();
    begin();
    mainLoop();
    end();
    unload();
}

} /* namespace gt2k_menu */

/* Static initialiser (.ctors entry 0x0028F06C): the item factories of
 * ConsolationControl (T099's _T057 set of the counter items; "rank_disp"
 * without T056's 1.25 arguments).  Written out the way build_vec_init
 * expands it (HOOK, trophy_internal.h): an exact form, not the original. */
// FUNC 0x00138EE0 T057_static_init
void T057_static_init(void)
{
    MenuHook *base = g_0049BD60;
    int left = 4;                           /* build_vec_init's iterator: elements - 1 */

    HOOK("count",          create_count_T057,          0, 0, 0, 0);
    HOOK("count_shadow",   create_count_shadow_T057,   0, 0, 0, 0);
    HOOK("get_money_disp", create_get_money_disp_T057, 0, 0, 0, 0);
    HOOK("rank_disp",      create_rank_disp,           0, 0, 0, 0);
    HOOK("render_model",   create_trophy_consolation,  0, 0, 0, 0);
}
