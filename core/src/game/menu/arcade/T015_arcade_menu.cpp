/* T015 - entry points of the arcade menu.  0x00116508..0x001168B8.
 *
 * The menu runs on its own heap (memalign'ed, pushed as the menu heap) with
 * the ArcadeControl (0x3BE0 bytes) on the stack.
 *
 * Retail's .rodata (0x0030DF88..0x0030E130): "bad_alloc" (<new>, read
 * first), the two ending movies, then the type names SmartIndexT<int>,
 * UnitArena<64>, RegurationResult, RecordsMenuArg and the garage lists:
 * T010's view without the menu classes.  The memory card screens and the
 * movie player only through their entry points (no MemorycardControl /
 * MovieControl names in retail's T015). */
// DATA_START 0x0030DF88
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* getGameStatus, MenuStatus (nextMode) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: retail's T015 has RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "libc.h"                           /* memalign, free */
#include "game/app/T000_main.h"                 /* MenuArgs, MovieParam */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder (the guest garage) */
#include "game/menu/framework/T051_movie_entry.h"          /* play_movie */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* F_0014AE00, MEMCARD_GAME_LOAD_GUEST */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push / pop */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset */
#include "engine/io/T508_loader_thread.h"        /* loader_thread_start / stop */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/arcade/T015_arcade_menu.h"
#include "game/menu/arcade/T010_arcade_control.h"       /* last: ArcadeControl (SmartIndexT<int>) */
#include "game/menu/arcade/arcade_control_internal.h"

/* The arcade menu's heap. */
#define ARCADE_MENU_HEAP_SIZE   0x380000    /* 3.5 MB */
#define ARCADE_PREVIEW_HEAP_SIZE 0x400000   /* 4 MB */

/* One run of the arcade menu: the menu draw state reset, the menu heap, the
 * loader thread, then ArcadeControl(args).run() (args filled in by
 * ArcadeControl::finish); its result. */
// FUNC 0x00116508 run_arcade_menu
int run_arcade_menu(MenuArgs *args)
{
    menu_draw_reset();
    void *heap = memalign(0x40, ARCADE_MENU_HEAP_SIZE);
    menu_heap_push(heap, ARCADE_MENU_HEAP_SIZE);
    loader_thread_start();
    gfx_vsync(2);
    int r;
    {
        gt2k_menu::ArcadeControl control(args); /* sp+0x00 */
        r = control.run();
        gfx_vsync(1);
    }                                       /* ~ArcadeControl(&control, 2) */
    loader_thread_stop();
    menu_heap_pop();
    free(heap);
    return r;
}

/* The arcade menu as main_arcade (T000) calls it.  Runs the menu again
 * after loading a guest garage from a memory card (GameStatus
 * menu.nextMode NEXT_MODE_GUEST_GARAGE: F_0014AE00 mode 0x1C into the
 * GameStatusHolder's status, which is then marked loaded) and after the
 * ending movies (nextMode 0xE / 0xF, chosen on "ar/select_ending", T016);
 * any other nextMode returns the menu's result. */
/* The ending movies (640 x 448, frames, with sound, once).  Inline in the
 * original: retail stores the file in each branch and the rest through the
 * MovieParam's address, which it keeps in $s0 (jump2 merged the two copies'
 * equal tails). */
static inline void play_ending(MovieParam *movie)
{
    movie->width     = 640;
    movie->height    = 448;
    movie->field     = 0;
    movie->withSound = 1;
    movie->oneShot   = 1;
    play_movie(movie);
}

// FUNC 0x001165D8 arcade_menu
int arcade_menu(MenuArgs *args)
{
    MovieParam movie;                       /* sp+0x10 */

    for (;;) {
        int r = run_arcade_menu(args);
        gfx_vsync(2);
        if (getGameStatus()->getMenuStatus()->nextMode == NEXT_MODE_GUEST_GARAGE) {
            F_0014AE00(gt2k_menu::MEMCARD_GAME_LOAD_GUEST, 0, 0,
                       getGameStatusHolder()->get(), NULL, 0, 0, 0, NULL);
            getGameStatusHolder()->loaded = 1;
        } else {
            if (getGameStatus()->getMenuStatus()->nextMode == NEXT_MODE_ENDING_SHORT)
                movie.file = "stream/ending_short_full.pss";
            else if (getGameStatus()->getMenuStatus()->nextMode == NEXT_MODE_ENDING_LONG)
                movie.file = "stream/ending_long_full.pss";
            else
                return r;
            play_ending(&movie);         /* implicit: play_movie (the inline play_ending) */
        }
        gfx_vsync(2);
    }
}

/* No callers (a test entry): fills a 4 MB menu heap with 0xA5, runs the
 * CLASS_PREVIEW car selection (ArcadeControl::runPreview) with a scratch
 * MenuArgs, and afterwards walks the last 16 bytes of the heap with an empty
 * loop (a guard-byte check compiled out; kept as in the original). */
// FUNC 0x00116748 arcade_preview_test
int arcade_preview_test(void)
{
    menu_draw_reset();
    u8 *heap = (u8 *)memalign(0x40, ARCADE_PREVIEW_HEAP_SIZE);
    for (int i = 0; i <= ARCADE_PREVIEW_HEAP_SIZE - 1; i++)
        heap[i] = 0xA5;
    menu_heap_push(heap, ARCADE_PREVIEW_HEAP_SIZE);
    loader_thread_start();
    gfx_vsync(2);

    MenuArgs args;                          /* sp+0x00 (the inline MenuArgs()) */
    int r;
    {
        gt2k_menu::ArcadeControl control(&args);    /* sp+0x40 */
        r = control.runPreview();
        gfx_vsync(1);
    }                                       /* ~ArcadeControl(&control, 2) */
    loader_thread_stop();
    for (int i = ARCADE_PREVIEW_HEAP_SIZE - 0x10; i <= ARCADE_PREVIEW_HEAP_SIZE - 1; i++) {
    }
    menu_heap_pop();
    free(heap);
    return r;
}
