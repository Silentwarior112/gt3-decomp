/* T068_memorycard_entry.h - F_0014AE00, the memory card screens' entry
 * point, and its modes (translation unit T068, unit u026; the controller
 * classes are in T068_memorycard_control.h, which includes this header).
 *
 * In its own header since u001's data pass (2026-10-17): of the originals,
 * only the memory card units' (T068..T076) .rodata have MemorycardControl's
 * type names; the callers of F_0014AE00 (T000, T015, T038, T052..T055,
 * T080, T102) saw the function and its modes without the classes (egcs
 * writes the type name of every class it sees whose virtual functions are
 * all inline).  MemorycardControl::MODE_* are these values. */
#ifndef GAME_T068_MEMORYCARD_ENTRY_H
#define GAME_T068_MEMORYCARD_ENTRY_H

#include "game/game.h"

namespace gt2k_menu {

struct MemorycardLoadInfo;                  /* T068_memorycard_control.h (pointers only) */

/* F_0014AE00's mode: the screen run (? names; the callers pass these:
 * T000, T052..T055, T080, T102's race_memcard). */
enum MemorycardMode {
    MEMCARD_DEMO_REPLAY_LOAD = 0x0C,        /* a demo replay of the disc (page "demo_loader"; main) */
    MEMCARD_REPLAY_SAVE      = 0x10,        /* a MemorycardReplayFile of "ReplayData" (the races) */
    MEMCARD_REPLAY_LOAD      = 0x11,        /* main */
    MEMCARD_GHOST_SAVE       = 0x12,        /* a MemorycardBestlapReplayFile of "GhostData" ("MaxSpeed"
                                               for the VMAX test, kind 0x11; the races) */
    MEMCARD_GHOST_LOAD       = 0x13,        /* the races */
    MEMCARD_GHOST_SAVE2      = 0x14,        /* begin() treats it as 0x12, isSaveMode() not (no caller) */
    MEMCARD_LOGGER_LOAD      = 0x15,        /* the data logger's replay (T055) */
    MEMCARD_SETTING_SAVE     = 0x16,        /* a MemorycardSettingFile of "SettingSerialize" (T080) */
    MEMCARD_SETTING_LOAD     = 0x17,        /* T080 */
    MEMCARD_FILE_UTILITY     = 0x18,        /* page "file_operation" (main; the game save screen) */
    MEMCARD_GAME_SAVE        = 0x19,        /* a MemorycardGameFile of "GameStatus", page "gamesave" */
    MEMCARD_GAME_LOAD        = 0x1A,        /* page "gameload" (main) */
    MEMCARD_AUTOLOAD         = 0x1B,        /* page "autoload" (main, at boot) */
    MEMCARD_GAME_LOAD_GUEST  = 0x1C,        /* page "gameload_g" (the arcade's guest game, T015) */
    MEMCARD_GAME_LOAD_GTMODE = 0x1D         /* page "gameload_g" (GT mode, T038) */
};

} /* namespace gt2k_menu */

/* The memory card screens: runs `mode' (see above) and returns
 * MemorycardControl::result.  arg: a MemCardRaceInfo (T102) for the race
 * saves, else the loads' destination; buf / size: the work buffer for the
 * icon heaps; licence / test: the category the file list starts on;
 * loadInfo: where a load puts the save's thumbnail and date (NULL: not
 * wanted).  The frame is 0x30870 bytes, the controller at sp+0. */
int F_0014AE00(int mode, u64 race, u64 car, void *arg, void *buf, int size, int licence, int test,
               gt2k_menu::MemorycardLoadInfo *loadInfo);    /* 0x0014AE00 */

#endif /* GAME_T068_MEMORYCARD_ENTRY_H */
