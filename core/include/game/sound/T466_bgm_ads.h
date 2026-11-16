/* T466_bgm_ads.h - the streamed BGM (ADS) (translation unit T466, unit u104).
 *
 * bgm/ads.inf: 17 groups of tracks with their loop markers; a track plays
 * bgm/<name> (bit 0x1000 of the track: the intro file bgm/i_<name> first)
 * through the T521 stream (pdibgm_start / pdibgm_stop).  ADS groups 0..0x10 are the BGM front end's
 * (T464) group ids 0x12..0x22.  The files store offsets relative to their
 * base word; T467 (bgm_track_relocate 0x0022AE38, bgm_tracklist_relocate
 * 0x0022AE88) relocates the track lists of ads.inf and music.inf.
 *
 * BgmTrackList ({tracks, count}, what the *_get_tracks functions return by
 * value) and BgmMarkers are declared here; the plan put them in
 * T130_race_ps2_base.h (u044 defined them most fully).
 */
#ifndef GAME_T466_BGM_ADS_H
#define GAME_T466_BGM_ADS_H

#include "game/game.h"

struct AdsInf;                                  /* u104: bgm/ads.inf */

/* The beat markers of a track (ads.inf): 32 channels of times in units of
 * 1 / rate s, and the loop length.  Relocated in place by
 * bgm_markers_relocate (base = this, times[] made absolute).  Size 0x10C.
 * Users: RaceBGMPS2 (T130) follows them while the track plays. */
typedef struct BgmMarkers {
    struct BgmMarkers *base;                    /* 0x00 the address it was relocated for (u104) */
    int                rate;                    /* 0x04 */
    u32                length;                  /* 0x08 */
    int                count[32];               /* 0x0C the markers of each channel */
    u32               *times[32];               /* 0x8C */
} BgmMarkers;
EGCS_SIZE(BgmMarkers, 0x10C);

/* One track of a group of ads.inf / music.inf, 0x14 bytes.  Words 0x0..0xC
 * are offsets in the file; word 0x10 only in ads.inf. */
typedef struct BgmTrack {
    const char *group;                          /* 0x00 (music.inf: the group name) */
    const char *name;                           /* 0x04 the file name (bgm/<name>) */
    const char *title;                          /* 0x08 (?) */
    const char *artist;                         /* 0x0C (?) */
    union {
        struct BgmMarkers *markers;             /* 0x10 ads.inf: the loop markers */
        int                seq;                 /* 0x10 music.inf: the sequence index in music.seq */
    };
} BgmTrack;
EGCS_SIZE(BgmTrack, 0x14);

/* What the *_get_tracks functions (ads_get_tracks, T463 music_get_tracks,
 * T464 bgm_get_tracks) return by value: the tracks of a group.  Size 8.
 * It has a constructor: retail returns it in memory (the caller's buffer in
 * $a0, the group in $a1: findMusicInfo 0x0021AB24, RaceBGMPS2::playTrack
 * 0x0017E390, bgm_get_tracks' own calls 0x0022B790 / 0x0022B7A0), which
 * egcs does only for a class with a constructor; a plain struct comes back
 * in $v0 / $v1 (u022's MultiButtonInfo).  The (tracks, count) constructor
 * and the memberwise copy constructor are the *_get_tracks functions' (u104):
 * retail builds the result in the caller's buffer (no frame, no copy) and
 * copies a group's list with two lw from the group's address (an implicit
 * copy is an 8-byte ldl / ldr block move). */
typedef struct BgmTrackList {
    BgmTrack *tracks;                           /* 0x0 */
    u32       count;                            /* 0x4 (u104 compares it unsigned; u044: int) */

#ifdef __cplusplus
    BgmTrackList() {}
    BgmTrackList(BgmTrack *t, u32 n) : tracks(t), count(n) {}
    BgmTrackList(const BgmTrackList &l) : tracks(l.tracks), count(l.count) {}
#endif
} BgmTrackList;
EGCS_SIZE(BgmTrackList, 0x8);

/* u037 / u098's name for a track of BGM group 0x14 (what findMusicInfo,
 * T430, returns): the same record (u037 / u098 pad_00 / pad_10 = group /
 * markers). */
typedef BgmTrack MusicInfo;

/* base = this, times[] relocated. */
void bgm_markers_relocate(struct BgmMarkers *m);                /* 0x0022AA78 */
/* The 17 groups, their tracks (with the marker pointers) and markers. */
void ads_inf_relocate(struct AdsInf *inf);                      /* 0x0022AAB8 */
void ads_init(void);                                            /* 0x0022ABB0 loads bgm/ads.inf (once) */
void ads_term(void);                                            /* 0x0022AC08 */
/* The tracks of ADS group `group' (-1: all the tracks of ads.inf). */
struct BgmTrackList ads_get_tracks(int group);                  /* 0x0022AC30 */
void ads_set_volume(f32 volume);                                /* 0x0022AC78 the stream volume */
/* Starts track `track' (-1: a random one; bit 0x1000: the intro) of ADS
 * group `group'; a missing file plays sound effect 4 instead. */
void ads_play(int group, int track, f32 volume, int arg);       /* 0x0022AC98 (T464's order) */
void ads_stop(void);                                            /* 0x0022AE18 */

#endif /* GAME_T466_BGM_ADS_H */
