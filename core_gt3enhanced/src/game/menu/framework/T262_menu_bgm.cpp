/* T262 - the menu BGM starters.  0x001D27F0..0x001D28C0.
 * Both start a BGM track (volume 0.8) only when the "menu BGM" game option
 * (GameStatus+0x2FF38 options, +0x4 menuBgm) is on.  The options are read
 * through GameStatus's inline getGameOptions() (retail computes &options
 * first: lui / ori 0xFF38 / addu, then lw 0x4).
 * Only this translation unit includes the game status and the BGM headers:
 * retail's type names of GameStatus's garage lists (15GarageWheelList ..,
 * .rodata 0x003378F8) are T262's alone.
 * Data: retail's T262 .rodata is only those eight names (GarageWheelList,
 * List<GarageWheel>, GarageWheel, Cell<GarageWheel>, GarageCarList,
 * List<GarageCar>, GarageCar with its base list, Cell<GarageCar>): no menu
 * operator class, so it includes its own headers, not menu_draw_state_internal.h (which brings
 * the operator classes' names).  The code references none of it: the first
 * name anchors the range (else T259's ran over it). */
// DATA_START 0x003378F8
#include "types.h"
#include "game/menu/framework/T262_menu_bgm.h"             /* own declarations */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::getGameOptions (T428: menuBgm) */
#include "game/sound/T464_bgm.h"                  /* bgm_play_if_changed, bgm_play */

/* 12 callers (the menu pages).  0.8 is the .sdata literal gp-0x7944. */
// FUNC 0x001D27F0 menu_bgm_play_if_changed
void menu_bgm_play_if_changed(int group, int track, int arg)
{
    if (getGameStatus()->getGameOptions()->menuBgm != 0)
        bgm_play_if_changed(group, track, 0.8f, arg);
}

/* The same through bgm_play (0.8: the literal gp-0x7940). */
// FUNC 0x001D2858 menu_bgm_play
void menu_bgm_play(int group, int track, int arg)
{
    if (getGameStatus()->getGameOptions()->menuBgm != 0)
        bgm_play(group, track, 0.8f, arg);
}
