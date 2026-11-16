/* T068_memorycard_load_info.h - gt2k_menu::MemorycardLoadInfo, what
 * F_0014AE00's loads give back (translation unit T068, unit u026; the
 * struct of T068_memorycard_control.h, which includes this header where it
 * defined it).
 *
 * In a header of its own since u018's data pass (2026-10-17): the logger
 * (T055) has the record by value but saw neither MemorycardControl nor the
 * memory card file classes (retail T055's .rodata has none of their type
 * names; T108_memorycard_file.h's templates and T068_memorycard_control.h's
 * dialogs put theirs into every includer).  A plain record with an empty
 * inline destructor: moving it changes no object. */
#ifndef GAME_T068_MEMORYCARD_LOAD_INFO_H
#define GAME_T068_MEMORYCARD_LOAD_INFO_H

#include "game/game.h"
#include "game/menu/memorycard/T108_game_thumbnail.h"       /* gt2k_game::GameThumbnail (thumbnail) */
#include "engine/io/T501_memorycard_manager.h"   /* gt2k_menu::McDateTime (date) */

namespace gt2k_menu {

/* (? name, u030) Where a load puts the loaded save's thumbnail and date:
 * F_0014AE00's a8, written by T074's LoadWorkingMenu (gt3->thumbnail,
 * gt3->dir.entry.create).  The logger (u018) passes its LoggerReplayInfo,
 * the same record (it reads the comment / licence name at +0x28 and the
 * date at +0x60).  Not polymorphic; size 0x68, 8-aligned. */
struct MemorycardLoadInfo {
    /* An empty inline destructor (u018): the logger's stack copies (logger_run,
     * logger_main) open an exception region at their declaration whose
     * landing pad only rethrows (retail's `b; jal __throw' after the
     * setDate call, and its region in the eh table); a trivial destructor
     * gives none.  Only u018 has these objects by value. */
    ~MemorycardLoadInfo() {}

    gt2k_game::GameThumbnail thumbnail;     /* 0x00 */
    McDateTime               date;          /* 0x60 */
};
EGCS_SIZE(gt2k_menu::MemorycardLoadInfo, 0x68);

} /* namespace gt2k_menu */

#endif /* GAME_T068_MEMORYCARD_LOAD_INFO_H */
