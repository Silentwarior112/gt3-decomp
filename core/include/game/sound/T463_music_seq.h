/* T463_music_seq.h - the sequenced music (translation unit T463, unit u104).
 *
 * music/music.inf (named groups of tracks, T466's BgmTrack records with a
 * sequence index), the music bank (music/music.ins at SPU 0xAC000, its header
 * in a 0x4000-byte buffer) and the sequences (music/music.seq), and the music
 * driver thread: woken by a VBLANK_E handler, it ticks the sequencer, the
 * ADS stream (T521) and the clock (T465) once a frame.  Group ids 0..0x11 of
 * the BGM front end (T464).
 *
 * C-compatible.  The music thread's ThreadCtx is defined in
 * G002_mpeg_frameq.h, with the worker_thread_* functions that use it.
 */
#ifndef GAME_T463_MUSIC_SEQ_H
#define GAME_T463_MUSIC_SEQ_H

#include "game/game.h"

struct BgmTrackList;                            /* T466_bgm_ads.h */
struct MusicInf;                                /* u104: music/music.inf */

void music_inf_relocate(struct MusicInf *inf);              /* 0x0022AF18 */
/* Stops the sequence player if a sequence plays (no lock). */
void music_seq_stop(void);                                  /* 0x0022AF98 */
/* INTC_VBLANK_E handler: wakes the music thread. */
int  music_vblank_handler(int cause);                       /* 0x0022AFC8 */
/* The music thread: one tick per VBLANK_E until told to quit. */
void music_thread(void *arg);                               /* 0x0022AFF8 */
/* Loads music.inf, links the group table, sets the sequencer up, starts
 * the thread and its VBLANK_E handler. */
void music_init(void);                                      /* 0x0022B058 */
void music_term(void);                                      /* 0x0022B220 */
/* The tracks of music group `group' (-1: all of music.inf). */
struct BgmTrackList music_get_tracks(int group);            /* 0x0022B298 */
void music_set_volume(f32 volume);                          /* 0x0022B2F8 1.0 = 0x3FFF */
/* The music bank and music.seq (once; under the music semaphore). */
void music_load(void);                                      /* 0x0022B338 */
void music_unload(void);                                    /* 0x0022B410 */
/* Plays track `track' (-1: a random one) of group `group' (arg unused). */
void music_play(int group, int track, f32 volume, int arg); /* 0x0022B460 (T464's order) */
void music_stop(void);                                      /* 0x0022B558 */

#endif /* GAME_T463_MUSIC_SEQ_H */
