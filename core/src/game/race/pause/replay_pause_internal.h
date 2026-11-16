/* replay_pause_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u058,
 * 0x001A2EC8..0x001A6488, second pass: translation units T170..T175.
 *
 *   T170  ReplayPause, the pause menu of a replay, and its ReplayPauseMenu
 *         window (include/game/race/pause/T170_replay_pause.h)
 *   T171  RacePauseWithRetry, the pause menu with a retry button, and its
 *         PauseMenuWithRetry window (include/game/race/pause/T171_race_pause_with_retry.h)
 *   T172  CourseCamera, the camera file of a course: its records, zones and
 *         piecewise linear tables (include/game/camera/T172_course_camera.h)
 *   T173  RaceSmoke, the tyre smoke / dust particles
 *         (include/game/effects/T173_race_smoke.h)
 *   T174  RaceMemPool, the replay memory of a race
 *         (include/game/race/core/T174_race_mem_pool.h)
 *   T175  RaceBGMBase, the race BGM interface, and its dummy instance
 *         (include/game/sound/T175_race_bgm_base.h)
 *
 * Every class is defined in its canonical header (include/game/).  Each
 * source includes the headers it needs itself, its own last: egcs writes a
 * unit's type names in reverse class-definition order right after its
 * vtables, and an included header's inline functions add their strings and
 * typeinfo names to every object that includes it, so the include set
 * decides .rodata (T170 / T171 / T174 / T175 start with <new>'s "bad_alloc",
 * T172 / T173 do not; T170 / T171 name their own classes, the window classes
 * they use, the operator list classes and UnitArena<64> (T170 RaceCameraBase
 * too), T174 none; T173's .rodata is its five tables; ref/data/u058/notes.md
 * has the data pass).  This header holds what the
 * sources share: the named constants of T170 (T173's constants and EE
 * helpers are in T173_race_smoke.cpp).
 *
 * Offsets in comments are the ORIGINAL (egcs) ones: egcs puts the vptr of
 * the first polymorphic class after that class's data (PauseBase +0x8,
 * RaceMenuLayer +0x2C, RaceMenuButton +0x10, RaceBGMBase +0x4), GCC 14 at
 * +0.
 */
#ifndef U058_H
#define U058_H

#include "types.h"

/* ======================================================================== */
/* T170 / T171: the pause menus                                             */
/* ======================================================================== */

/* PauseBase::virtual_2's results: PAUSE_RESULT_* (T168).  A decided button
 * returns its id: m_continue PAUSE_RESULT_CONTINUE; ReplayPause's m_exit
 * PAUSE_RESULT_EXIT, m_replay PAUSE_RESULT_INPUT (it opens the replay
 * options); RacePauseWithRetry's m_retry PAUSE_RESULT_RETRY, m_ok
 * PAUSE_RESULT_EXIT and m_exit / m_cancel PAUSE_RESULT_INPUT (they switch the
 * page). */

/* ReplayPauseMenu's item that opens the replay options (m_replay). */
enum { REPLAY_PAUSE_ITEM_REPLAY = 2 };

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

#endif /* U058_H */
