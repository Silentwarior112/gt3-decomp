/* T038 - gtmode_main, the GT mode as main() runs it.  0x001274E0..0x00127790.
 *
 * Data: no variables.  .rodata 0x00312718..0x003129A6: <new>'s "bad_alloc"
 * (unreferenced, so DATA_START), then the type names of the classes the
 * file saw (reverse definition order): List<Button>, Cell<Button>,
 * OperatorList, List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * SmartIndexT<int>, UnitArena<64>, RegurationResult, RecordsMenuArg, the
 * garage's eight, Operator; so <new>, the GameStatus, RecordsMenuArg
 * (T089), RegurationResult, T153's UnitArena<64>, SmartIndexT<int> (T251),
 * the operator classes (T216), Button (T206), ButtonList (T244).  .sdata:
 * "hm/hm" (0x003512B0; -G8, ref/compile_flags.json).  The memory card
 * screens through their entry point only (T068_memorycard_entry.h: the
 * original saw no MemorycardControl). */
// DATA_START 0x00312718
#include <new>                              /* (seen: "bad_alloc", T038's first .rodata) */
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus */
#include "game/menu/records/T089_records_menu_args.h"    /* (seen: RecordsMenuArg) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#define GAME_T153_EMITS_TYPEINFO            /* (seen: UnitArena<64>'s type name) */
#include "game/race/core/T153_race_course.h"
#include "game/common/T251_smart_index.h"          /* (seen: SmartIndexT<int>) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen: the operator classes) */
#include "game/menu/framework/T206_button.h"               /* (seen: Cell<Button>) */
#include "game/menu/framework/T244_button_list.h"          /* (seen: List<Button>) */
#include "game/menu/gtmode/T038_gtmode_main.h"
#include "game/menu/gtmode/T043_gtmode_control.h"       /* GtmodeControl (run, runTrade; its implicit destructor) */
#include "game/save/T427_menu_status.h"          /* MenuStatus::nextMode, setPage */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder (the guest garage) */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* F_0014AE00, gt2k_menu::MEMCARD_* */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push / pop */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset */
#include "game/sound/T463_music_seq.h"            /* music_load / music_unload */
#include "engine/io/T508_loader_thread.h"        /* loader_thread_start / stop */
#include "engine/pdistd/T532_heap.h"                 /* memalign, free */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/gtmode/gtmode_main_internal.h"

/* Runs a GtmodeControl (on the stack, sp+0x10, over a 4 MB menu heap)
 * until its pages leave the GT mode; returns the result of the last run().
 * Two of the modes the pages choose are handled here, and the GT mode
 * starts again afterwards:
 *   "go_trade": the memory card screen loads a guest garage into the
 *        GameStatusHolder's status (T004, then marked loaded); if that
 *        worked, GtmodeControl::runTrade on a new 4 MB heap;
 *   "go_save_in_home": the game is saved (a 13 MB work buffer) and the GT
 *        mode restarts at "hm/hm" (page argument 4).
 * The GtmodeControl's implicit destructor is expanded here (render.root,
 * render's GameObject, then ControlBase, all with flags 2; the landing pads
 * do the same and rethrow).  The guest load's result goes into `result'
 * (retail keeps both in $s2): harmless, run() sets it again before the loop
 * can end.  A `while' loop: `continue' goes straight to the top (a `for'
 * loop's continue label moved the shared free() to the loop head). */
// FUNC 0x001274E0 gtmode_main
int gtmode_main(void)
{
    int result;
    void *heap;

    menu_draw_reset();
    music_load();
    while (1) {
        heap = memalign(MENU_HEAP_ALIGN, GTMODE_HEAP_SIZE);
        menu_heap_push(heap, GTMODE_HEAP_SIZE);
        loader_thread_start();
        {
            gt2k_menu::GtmodeControl ctl;   /* sp+0x10 */
            /* implicit: ~RootObject, ~GameObject, ~ControlBase (the implicit ~GtmodeControl of ctl, inline: render.root, render, the base) */
            result = ctl.run();
            gfx_vsync(2);
        }
        loader_thread_stop();
        menu_heap_pop();
        free(heap);

        int mode = getGameStatus()->getMenuStatus()->nextMode;
        if (mode == MAIN_MODE_GTMODE_TRADE) {
            GameStatusHolder *guest = getGameStatusHolder();
            result = F_0014AE00(gt2k_menu::MEMCARD_GAME_LOAD_GTMODE,
                                0, 0, guest->get(), NULL, 0, 0, 0, NULL);
            getGameStatusHolder()->loaded = 1;
            if (result == 0)
                continue;
            heap = memalign(MENU_HEAP_ALIGN, GTMODE_HEAP_SIZE);
            menu_heap_push(heap, GTMODE_HEAP_SIZE);
            loader_thread_start();
            {
                gt2k_menu::GtmodeControl ctl;   /* sp+0x10 */
                /* implicit: ~RootObject, ~GameObject, ~ControlBase (the implicit ~GtmodeControl of ctl, inline: render.root, render, the base) */
                ctl.runTrade();
                gfx_vsync(2);
            }
            loader_thread_stop();
            menu_heap_pop();
        } else if (mode == MAIN_MODE_GTMODE_SAVE) {
            heap = memalign(MENU_HEAP_ALIGN, GTMODE_SAVE_WORK_SIZE);
            F_0014AE00(gt2k_menu::MEMCARD_GAME_SAVE, 0, 0, NULL, heap,
                       GTMODE_SAVE_WORK_SIZE, 0, 0, NULL);
            getGameStatus()->getMenuStatus()->setPage("hm/hm", 4);
        } else {
            break;
        }
        free(heap);
    }
    music_unload();
    return result;
}
