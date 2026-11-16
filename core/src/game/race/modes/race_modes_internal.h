/* race_modes_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u046,
 * 0x001817F8..0x001848A8, second pass: translation units T133..T137, the
 * race modes built on RaceBasic (T132):
 *
 *   T133  RaceSinglePlayer (+ RaceSinglePlayerInformation): one player, one
 *         pad input (RaceInput, pad slot 0x14) with its replay record
 *         (include/game/race/modes/T133_race_single_player.h)
 *   T134  RaceArcade, RaceArcadeSingle (+ RaceArcadeInformation): the 7 car
 *         icons, the arcade result, the pause / replay pause menus and a
 *         DynamicsConductorSinglePlayer (include/game/race/modes/T134_race_arcade.h)
 *   T135  RaceArcadeDemo (+ RaceArcadeDemoInformation): the arcade race run
 *         as a demo / shop test drive (include/game/race/modes/T135_race_arcade_demo.h)
 *   T136  RaceFreePractice (include/game/race/modes/T136_race_free_practice.h)
 *   T137  RaceSplitBattle (+ RaceSplitBattleInformation, the templates
 *         RaceBattleT<2>, RaceBattleInformationT<2>): the 2-player split
 *         screen battle (include/game/race/modes/T137_race_split_battle.h)
 *
 * Every class is defined in its canonical header (include/game/); this header
 * has only the named constants the unit's sources share.  (u046's data pass:
 * each source includes the canonical headers it needs itself, in the order
 * that gives retail's type names, and defines its registry data: the
 * RaceClassInfo records in .data, the RaceClassNode nodes in .sdata.)
 */
#ifndef U046_H
#define U046_H

#include "types.h"

/* ======================================================================== */
/* Constants (only used here)                                               */
/* ======================================================================== */

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* RaceBase::replayMode (virtual_8's argument) RACE_LIVE / RACE_REPLAY and
 * RaceBase::virtual_34's RACE_END_DEMO_TIME_UP: T129. */

/* The inputId of player i (RaceEntryInformation::inputId, RaceBase::
 * virtual_26). */
enum { INPUT_ID_PLAYER = 0x100 };

/* The pad manager slots of the players' inputs (PadManager::attach, T475:
 * 20-21 pad + wheel). */
enum { PAD_SLOT_PLAYER = 0x14 };

/* COURSE_KIND_* (RaceDataBase::getCourseKind): game/data/T484_race_database.h. */

/* The bytes of a RaceInfoBlock the race writes (its size word): the input
 * records follow them (RaceBase::virtual_10 / 16: + virtual_15 / 22). */
enum { RACE_INFO_BLOCK_SIZE = 0x191C };

/* The idle time of the demo, frames (30 s at 60 Hz). */
enum { DEMO_IDLE_FRAMES = 1800 };

#endif /* U046_H */
