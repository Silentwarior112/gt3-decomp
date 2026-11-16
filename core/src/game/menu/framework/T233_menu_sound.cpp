/* T233 - the menu music of a page; the menu sound effects.
 * 0x001C4D78..0x001C4FD0.
 *
 * Data (retail .rodata 0x003361C0..0x003362F8): selectMusic's names longer
 * than 8 bytes, then the garage classes' type names (the GameStatus, read
 * for getGameStatus); .sdata 0x00352668..0x003526AC its short names. */
#include "game/menu/framework/page_layout_internal.h"
#include "game/save/T429_game_status.h"          /* getGameStatus, GameOptions::menuSe (T428) */
#include "game/menu/framework/T233_menu_sound.h"
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play_if_changed */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "game/sound/T469_sound_effects.h"        /* se_play, SE_PAN_CENTRE, SE_PRIO_PROGRAM */
#include "game/common/T494_text_utils.h"           /* str_equal, str_has_prefix */

/* PageManager's constructor (0x001B6250): no music yet. */
// FUNC 0x001C4D78 MenuSound::MenuSound
MenuSound::MenuSound()
{
    bgm = MENU_BGM_NONE;
}

/* The music of a page, chosen by its name (mostly the folder prefix):
 * "cd/" and "00cmn/cd_ride_sure" 2, "at/" and "00cmn/ws_fit_sure" 0x10,
 * "hm/" 6, "lc/" 7, "r/" 4, "replay_theater" 0x1A, "00cmn/race_info" 4,
 * "ts/" 0xF, "map" 5, "mt/" 9, "ar/" 0, others MENU_BGM_NONE (the music
 * stops).  Changed only when it differs from the one playing (track 0, arg
 * 1).  The "hm/" test has no break: the later tests still run (and could
 * override it).  PageManager::startPage (0x001B7268).
 *
 * A do { } while (0) of one `if (...) { music = n; break; }' per name, as
 * retail shows: egcs moves the loop's leading exits (up to the "at/" test:
 * its 30-insn limit) behind the body and jumps there first (`b' at the
 * start), loop.c hoists seven string addresses into s-registers (`this' is
 * spilled), and the equal `music = n; break;' blocks are cross-jumped. */
// FUNC 0x001C4D88 MenuSound::selectMusic
int MenuSound::selectMusic(const char *page)
{
    int music = MENU_BGM_NONE;
    do {
        if (str_has_prefix("cd/", page)) {
            music = 2;
            break;
        }
        if (str_equal("00cmn/cd_ride_sure", page)) {
            music = 2;
            break;
        }
        if (str_has_prefix("at/", page)) {
            music = 0x10;
            break;
        }
        if (str_equal("00cmn/ws_fit_sure", page)) {
            music = 0x10;
            break;
        }
        if (str_has_prefix("hm/", page))
            music = 6;
        if (str_has_prefix("lc/", page)) {
            music = 7;
            break;
        }
        if (str_has_prefix("r/", page)) {
            music = 4;
            break;
        }
        if (str_has_prefix("replay_theater", page)) {
            music = 0x1A;
            break;
        }
        if (str_equal("00cmn/race_info", page)) {
            music = 4;
            break;
        }
        if (str_has_prefix("ts/", page)) {
            music = 0xF;
            break;
        }
        if (str_has_prefix("map", page)) {
            music = 5;
            break;
        }
        if (str_has_prefix("mt/", page)) {
            music = 9;
            break;
        }
        if (str_has_prefix("ar/", page))
            music = 0;
    } while (0);
    if (bgm != music) {
        if (music == MENU_BGM_NONE)
            bgm_stop();
        else
            menu_bgm_play_if_changed(music, 0, 1);  /* T262 F_001D27F0 */
        bgm = music;
    }
    return 1;
}

/* Plays menu sound effect se, centred, if the menu sound effects option is
 * on (1 cancel, 3 cursor, 4 refused, 5 decide, 10 left / right as used by
 * the pages). */
// FUNC 0x001C4F80 menu_play_se
void menu_play_se(int se)
{
    if (getGameStatus()->getGameOptions()->menuSe != 0)
        se_play(se, SE_PAN_CENTRE, SE_PRIO_PROGRAM, 1.0f);
}
