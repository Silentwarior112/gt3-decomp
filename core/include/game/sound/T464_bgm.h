/* T464_bgm.h - the BGM front end (translation unit T464, unit u104).
 *
 * Group ids 0..0x11 are the sequenced music groups (T463), 0x12..0x22 the
 * ADS stream groups 0..0x10 (T466).  A master volume scales every track's
 * volume; play / play-if-changed / stop run under the BGM semaphore.
 */
#ifndef GAME_T464_BGM_H
#define GAME_T464_BGM_H

#include "game/game.h"

struct BgmTrackList;                            /* T466_bgm_ads.h */

/* 0: sequenced music, 1: ADS stream, 2: none. */
int  bgm_kind(int group);                                       /* 0x0022B588 */
/* The parameter order of the bgm_play* functions is retail's: every caller
 * loads the volume ($f12) before the arg ($a2), and track is compared with
 * li -1 (signed). */
void bgm_play_unlocked(int group, int track, f32 volume, int arg);  /* 0x0022B5B8 */
void bgm_stop_unlocked(void);                                   /* 0x0022B678 */
void bgm_init(void);                                            /* 0x0022B6E0 */
void bgm_term(void);                                            /* 0x0022B710 */
/* The tracks of a group (none for kind 2). */
struct BgmTrackList bgm_get_tracks(int group);                  /* 0x0022B750 (F_0022B750) */
f32  bgm_get_master_volume(void);                               /* 0x0022B7D0 (1.0) */
/* Sets the master volume and applies it to what plays; returns the old one. */
f32  bgm_set_master_volume(f32 volume);                         /* 0x0022B7D8 */
/* Plays track `track' (-1: a random one; bit 0x1000: the intro first) of
 * group `group' unless the same song already plays: another track number,
 * -1, bit 0x1000 or another kind always restart; else the songs are
 * compared (sequence index / file name). */
void bgm_play_if_changed(int group, int track, f32 volume, int arg); /* 0x0022B860 (F_0022B860) */
/* Plays track `track' of group `group'. */
void bgm_play(int group, int track, f32 volume, int arg);       /* 0x0022B9B0 (F_0022B9B0) */
/* Stops the music. */
void bgm_stop(void);                                            /* 0x0022BA18 (F_0022BA18) */

#endif /* GAME_T464_BGM_H */
