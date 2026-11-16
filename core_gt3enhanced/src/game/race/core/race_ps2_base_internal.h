/* race_ps2_base_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u044,
 * 0x0017CF30..0x0017E6C0, second pass: translation unit T130 (one source
 * file, T130_race_ps2_base.cpp).
 *
 *   RaceEntryPS2  the entry list of the PS2 race modes: a RaceEntryBase with
 *                 7 inline car slots (0x3C bytes)
 *   RacePS2Base   the PS2 layer of every race mode (RaceBase -> RacePS2Base
 *                 -> RaceBasic -> ...), 0xC880 bytes: the effect managers
 *                 (replay blur, flares, tyre smoke, particles, rigid bodies),
 *                 the error view, the race BGM, the ambient sound, the
 *                 rear-view mirror rectangle, the pad input and the two
 *                 weather buffers
 *   RaceBGMPS2    the race music: plays the play list / the demo music and
 *                 follows 32 channels of beat markers of the current song
 *
 * The three classes are declared in the canonical header
 * include/game/race/core/T130_race_ps2_base.h, everything they use in the canonical
 * headers T130_race_ps2_base.cpp includes itself (since u044's data pass, in
 * the order that gives retail's type names).  This header keeps only what
 * has no canonical home: the unit's named constants.
 *
 * Offsets in comments are the ORIGINAL (egcs) ones: egcs puts the vptr of
 * the first polymorphic class after that class's data (RaceBGMBase +4,
 * RaceEntryBase +0x1C), GCC 14 at +0.
 */
#ifndef U044_H
#define U044_H

#include "types.h"
#ifndef BOOT_OLD_GCC
#include <float.h>                          /* FLT_MAX */
#endif
#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38F             /* (egcs has no float.h) 0x7F7FFFFF */
#endif

/* ------------------------------------------------------------------------ */
/* Constants                                                                */
/* ------------------------------------------------------------------------ */

/* The race pool's alignment (every race object, g_002B3BF0). */
#define RACE_POOL_ALIGN         0x10

/* RacePS2Base::input's PadManager slot (slots 20-21 are steering[], T475). */
#define RACE_PS2_INPUT_SLOT     0x15

/* The two weather buffers (G001: Weather / WeatherDrop are opaque). */
#define WEATHER_SIZE            0x40
#define WEATHER_DROPS_SIZE      0x6400

/* RaceBase::replayMode (virtual_8's argument) and the endCode that makes
 * RacePS2Base::virtual_41 run the race again (RaceBasic::virtual_30's
 * virtual_34(4) for the pause menu's retry button, id 5, RacePauseWithRetry,
 * T171 / T132): RACE_LIVE .. RACE_REPLAY_ALT, RACE_END_RETRY (T129). */

/* takeSignal() / virtual_38() value of the race thread's end. */
#define RACE_SIGNAL_END         1

/* Bit 29 of the input's buttons (Control::getButtons): no race frame while
 * it is held (T413's raceinput_padButtons sets it for raw 0x8013). */
#define RACE_INPUT_HOLD_FRAME   0x20000000

/* The random seed of the race start (lcg_rand_seed). */
#define RACE_RANDOM_SEED        0x7F7F7F7F

/* The replay blur's f-number for the race (ReplayBlur() sets 2). */
#define RACE_BLUR_F_NUMBER      9.0f

/* BGM groups (T464): 0x14 the play-list songs (ADS group 2), 0x22 the demo
 * music (ADS group 0x10).  RaceBGMPS2::play's code is a group; 0x14
 * plays the play list's next song, any other group a random track. */
#define BGM_GROUP_SONGS         0x14
#define BGM_GROUP_DEMO          0x22
#define BGM_TRACK_RANDOM        ((u32)-1)

/* The marker channels of a song (BgmMarkers::count / times). */
#define BGM_MARKER_CHANNELS     32

/* RaceBGMPS2::update looks one frame ahead of the stream position. */
#define BGM_LOOKAHEAD           0.033333335f    /* 1/30 s (gp-0x7D10) */

/* The play-list songs' volume: 0.7 in replays, 0.5 in races. */
#define BGM_SONG_VOLUME_REPLAY  0.7f
#define BGM_SONG_VOLUME_RACE    0.5f

#endif /* U044_H */
