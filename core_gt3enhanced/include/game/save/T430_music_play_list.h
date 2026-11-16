/* T430_music_play_list.h - MusicPlayList, MusicEntry (translation unit T430,
 * unit u098).
 *
 * The songs of BGM group 0x14 (the tracks of bgm_get_tracks, T464), each
 * known by the 64-bit code of its name (nameToCode, T481), and their play order:
 * straight or shuffled with a 16-bit seed that advances on every shuffle.
 * next() walks the order and skips the songs already played in this round
 * (a disabled song counts as played).  Users: GameOptions+0x228 (T428), the
 * race BGM player RaceBGMPS2 (u044, two of them), the menu play list (u037).
 * ? names (u037 / u044 / u098 agree). */
#ifndef GAME_T430_MUSIC_PLAY_LIST_H
#define GAME_T430_MUSIC_PLAY_LIST_H

#include "game/game.h"
#include "game/sound/T466_bgm_ads.h"              /* MusicInfo (a typedef of BgmTrack: no forward declaration) */

#ifdef __cplusplus                          /* a C++ translation unit */

/* One song of a play list.  0x10 bytes (the u64 aligns it to 8). */
struct MusicEntry {
    u64 code;                               /* 0x00 the song's code: nameToCode of its name */
    s8  played;                             /* 0x08 played in this round (or disabled) */
    s8  enabled;                            /* 0x09 1: the song is played */
    s16 order;                              /* 0x0A its position in the play order, -1 none */
};
EGCS_SIZE(MusicEntry, 0x10);

/* Not polymorphic.  0x410 bytes (GameOptions+0x228 to +0x638).  No
 * user-declared constructor: its owners fill it with init()
 * (GameOptions::setDefaults, T428; RaceBGMPS2's constructor, T130, on both
 * of its lists).  init() is not a constructor: it does not return this (an
 * egcs constructor does), and GameOptions() does not call it (u097). */
struct MusicPlayList {
    void init();                            /* 0x0021ABB0 (? name) a new seed, every song of group 0x14,
                                               shuffled */
    void resetPlayed();                     /* 0x0021AC98 a new round: played = !enabled */
    void setRandom(s8 r);                   /* 0x0021ACE0 0 straight, 1 shuffled; then reorder() */
    void reorder();                         /* 0x0021AD00 resetPlayed() and a new play order */
    void orderStraight();                   /* 0x0021AD38 order = table order */
    void orderShuffle();                    /* 0x0021AD90 a random order from seed */
    MusicEntry *next();                     /* 0x0021AEE8 the next song to play */

    u32        count;                       /* 0x000 songs */
    u16        position;                    /* 0x004 the next position of the play order */
    s8         random;                      /* 0x006 0 straight, 1 shuffled */
    s8         allPlayed;                   /* 0x007 every song played (or disabled) */
    u16        seed;                        /* 0x008 the shuffle's seed */
    MusicEntry entries[64];                 /* 0x010 */
};
EGCS_SIZE(MusicPlayList, 0x410);

/* The song of BGM group 0x14 whose name has the code *code, its number in
 * *index (unless NULL); the first song (index 0) when none has it. */
MusicInfo *findMusicInfo(const u64 *code, u32 *index);  /* 0x0021AB08 */

#endif /* __cplusplus */

#endif /* GAME_T430_MUSIC_PLAY_LIST_H */
