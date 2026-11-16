/* T007_game_init.h - the game's start-up and the race begin / end bracket
 * (translation unit T007, unit u001), 0x00102B58..0x00102E38. */
#ifndef GAME_T007_GAME_INIT_H
#define GAME_T007_GAME_INIT_H

#include "game/game.h"

class IOPLoadModuleBase;                    /* T524 (pointers only) */

/* Before every race: RaceCourse::initArena, RaceCarModel::initArena, the
 * control record pool (AutomobileControlRecord::allocBuffers(recordSize,
 * records), T318), RaceCourse::loadCameras, "race/brake.bin"
 * (brake_file_load), race_menu_heap_alloc (T197).  The callers pass
 * (0x20000, 2); the 8-car i.LINK battle (0x20000, 8). */
void race_begin(int recordSize, int records);   /* 0x00102B58 */
void race_end(void);                        /* 0x00102BB8 race_begin's steps undone, in reverse order */
/* hot (started by the bootstrap): gfx_init(1, 1, 2, 0) keeps the display
 * and the render thread starts with the boot fade; else
 * gfx_init_default(2).  Then the IOP modules (iop->load()), gt3.vol, the
 * system font, the databases and subsystems, the guest GameStatus
 * recreated (T004), and render_thread_release (T002). */
void game_init(IOPLoadModuleBase *iop, int hot);    /* 0x00102C00 */
/* main's first call: hot = argv[1] is "hot"; game_init with a
 * RaceIOPLoadModules on the stack (T009). */
void game_boot(int argc, char **argv);      /* 0x00102D98 */

#endif /* GAME_T007_GAME_INIT_H */
