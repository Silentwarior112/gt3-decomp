/* T464 - the BGM front end.  Group ids 0..0x11 are the sequenced music
 * groups (T463), 0x12..0x22 the ADS groups 0..0x10 (T466).
 * 0x0022B588..0x0022BA48. */
#include "game/sound/gtsound_internal.h"
#include "game/sound/T464_bgm.h"

/* .sdata, in retail's order (all initialised: written where defined). */
int g_bgmPlaying = BGM_PLAYING_NONE;            /* 0x00352B64 */
int g_bgmSema = -1;                             /* 0x00352B68 */
f32 g_bgmMasterVolume = 1.0f;                   /* 0x00352B6C */
u32 g_bgmGroup = 0xFFFFFFFF;                    /* 0x00352B70 */
int g_bgmTrack = -1;                            /* 0x00352B74 */
f32 g_bgmVolume = 1.0f;                         /* 0x00352B78 */

/* BGM_KIND_SEQ, BGM_KIND_ADS or BGM_KIND_NONE (above 0x22; negative groups
 * count as sequenced music). */
// FUNC 0x0022B588 bgm_kind
int bgm_kind(int group)
{
    if (group < BGM_GROUP_ADS)
        return BGM_KIND_SEQ;
    if (group <= BGM_GROUP_LAST)
        return BGM_KIND_ADS;
    return BGM_KIND_NONE;
}

/* The volume played is volume * the master volume; the one remembered is
 * volume. */
// FUNC 0x0022B5B8 bgm_play_unlocked
void bgm_play_unlocked(int group, int track, f32 volume, int arg)
{
    f32 played = volume * g_bgmMasterVolume;

    bgm_stop_unlocked();
    switch (bgm_kind(group)) {
    case BGM_KIND_SEQ:
        g_bgmPlaying = BGM_PLAYING_SEQ;
        music_play(group, track, played, arg);
        break;
    case BGM_KIND_ADS:
        g_bgmPlaying = BGM_PLAYING_ADS;
        ads_play(group - BGM_GROUP_ADS, track, played, arg);
        break;
    }
    g_bgmGroup = group;
    g_bgmTrack = track;
    g_bgmVolume = volume;
}

// FUNC 0x0022B678 bgm_stop_unlocked
void bgm_stop_unlocked(void)
{
    switch (g_bgmPlaying) {
    case BGM_PLAYING_ADS:
        ads_stop();
        break;
    case BGM_PLAYING_SEQ:
        music_stop();
        break;
    }
    g_bgmTrack = -1;
    g_bgmGroup = 0xFFFFFFFF;
    g_bgmPlaying = BGM_PLAYING_NONE;
}

// FUNC 0x0022B6E0 bgm_init
void bgm_init(void)
{
    g_bgmSema = create_mutex_sema(1);
    ads_init();
    music_init();
}

// FUNC 0x0022B710 bgm_term
void bgm_term(void)
{
    bgm_stop();
    music_term();
    ads_term();
    DeleteSema(g_bgmSema);
    g_bgmSema = -1;
}

// FUNC 0x0022B750 bgm_get_tracks
BgmTrackList bgm_get_tracks(int group)
{
    switch (bgm_kind(group)) {
    case BGM_KIND_SEQ:
        return music_get_tracks(group);
    case BGM_KIND_ADS:
        return ads_get_tracks(group - BGM_GROUP_ADS);
    }
    return BgmTrackList(NULL, 0);
}

// FUNC 0x0022B7D0 bgm_get_master_volume
f32 bgm_get_master_volume(void)
{
    return g_bgmMasterVolume;
}

/* Sets the master volume, applies it to what plays; returns the old one. */
// FUNC 0x0022B7D8 bgm_set_master_volume
f32 bgm_set_master_volume(f32 volume)
{
    f32 old;

    WaitSema(g_bgmSema);
    old = g_bgmMasterVolume;
    g_bgmMasterVolume = volume;
    volume *= g_bgmVolume;                  /* now the volume played */
    switch (g_bgmPlaying) {
    case BGM_PLAYING_ADS:
        ads_set_volume(volume);
        break;
    case BGM_PLAYING_SEQ:
        music_set_volume(volume);
        break;
    }
    signal_sema_helper(g_bgmSema);
    return old;
}

/* Plays the track unless the same song already plays: a different track
 * number, BGM_TRACK_RANDOM, BGM_TRACK_INTRO or another kind always
 * restart; else the songs are compared (sequence index / file name).
 * The sequence case indexes both lists with g_bgmTrack (== track there):
 * retail multiplies the reloaded g_bgmTrack once for both. */
// FUNC 0x0022B860 bgm_play_if_changed
void bgm_play_if_changed(int group, int track, f32 volume, int arg)
{
    int changed = 0;

    WaitSema(g_bgmSema);
    if (track != g_bgmTrack || track == BGM_TRACK_RANDOM || (track & BGM_TRACK_INTRO)) {
        changed = 1;
    } else if (bgm_kind(group) != bgm_kind(g_bgmGroup)) {
        changed = 1;
    } else {
        BgmTrackList cur = bgm_get_tracks(g_bgmGroup);
        BgmTrackList nxt = bgm_get_tracks(group);

        switch (bgm_kind(group)) {
        case BGM_KIND_ADS:
            changed = strcmp(cur.tracks[g_bgmTrack].name, nxt.tracks[track].name) != 0;
            break;
        case BGM_KIND_SEQ:
            changed = cur.tracks[g_bgmTrack].seq != nxt.tracks[g_bgmTrack].seq;
            break;
        }
    }
    if (changed)
        bgm_play_unlocked(group, track, volume, arg);
    signal_sema_helper(g_bgmSema);
}

// FUNC 0x0022B9B0 bgm_play
void bgm_play(int group, int track, f32 volume, int arg)
{
    WaitSema(g_bgmSema);
    bgm_play_unlocked(group, track, volume, arg);
    signal_sema_helper(g_bgmSema);
}

// FUNC 0x0022BA18 bgm_stop
void bgm_stop(void)
{
    WaitSema(g_bgmSema);
    bgm_stop_unlocked();
    signal_sema_helper(g_bgmSema);
}
