/* T466 - the streamed BGM (ADS): bgm/ads.inf and the stream calls.
 * 0x0022AA78..0x0022AE38. */
#include "game/sound/gtsound_internal.h"
#include "game/sound/T466_bgm_ads.h"

/* .sdata: the pointer first (written at its definition), then the
 * functions' short literals ("bgm/", "i_"). */
AdsInf *g_adsInf = NULL;                        /* 0x00352B00 */

/* Relocates the beat markers of a track: base = this, times[] += delta. */
// FUNC 0x0022AA78 bgm_markers_relocate
void bgm_markers_relocate(BgmMarkers *m)
{
    BgmMarkers *built = m->base;
    int delta, ch;

    m->base = m;
    delta = (u8 *)m - (u8 *)built;
    for (ch = 0; ch < 32; ch++)
        u104_reloc(m->times[ch], delta);
}

/* Relocates bgm/ads.inf: the 17 groups, their tracks (with the marker
 * pointers) and each track's markers. */
// FUNC 0x0022AAB8 ads_inf_relocate
void ads_inf_relocate(AdsInf *inf)
{
    u32 group, track;
    long delta = (long)inf - inf->base;     /* 64-bit: lwu, dsubu, truncated once */

    inf->base = (u32)inf;
    for (group = 0; group < ADS_GROUP_COUNT; group++) {
        bgm_tracklist_relocate(&inf->group[group], delta, 1);
        for (track = 0; track < inf->group[group].count; track++)
            bgm_markers_relocate(inf->group[group].tracks[track].markers);
    }
}

// FUNC 0x0022ABB0 ads_init
void ads_init(void)
{
    if (g_adsInf == NULL) {
        AdsInf *inf = (AdsInf *)file_read_blocking("bgm/ads.inf", NULL, 0, 1, &vol_device_object);
        ads_inf_relocate(inf);
        g_adsInf = inf;
    }
}

// FUNC 0x0022AC08 ads_term
void ads_term(void)
{
    AdsInf *inf = g_adsInf;

    if (inf != NULL) {
        g_adsInf = NULL;
        free(inf);
    }
}

/* The tracks of a group; -1: all the tracks (from group 0's, ads.inf
 * total). */
// FUNC 0x0022AC30 ads_get_tracks
BgmTrackList ads_get_tracks(int group)
{
    if (group == -1)
        return BgmTrackList(g_adsInf->group[0].tracks, g_adsInf->total);
    return g_adsInf->group[group];
}

// FUNC 0x0022AC78 ads_set_volume
void ads_set_volume(f32 volume)
{
    pdibgm_set_volume(volume);              /* 0x0025C9F8 */
}

/* Starts track `track' of ADS group `group' (BGM_TRACK_RANDOM: a random one;
 * bit BGM_TRACK_INTRO: the intro file i_<name>).  A missing file plays sound
 * effect 4 instead. */
// FUNC 0x0022AC98 ads_play
void ads_play(int group, int track, f32 volume, int arg)
{
    ads_stop();
    BgmTrackList list = ads_get_tracks(group);   /* sp+0x00 (built in place) */
    if (list.count == 0)
        return;

    char hostName[0x70];                    /* sp+0x10 the host: file ("" on the disc) */
    char path[0x70];                        /* sp+0x80 */
    int  lsn;                               /* sp+0xF0 */
    u32  size;                              /* sp+0xF4 the packed size */
    int  intro = 0;
    BgmTrack *t;

    if (track == BGM_TRACK_RANDOM)
        track = (u32)rand() % list.count;
    if (track & BGM_TRACK_INTRO) {
        intro = 1;
        track &= ~BGM_TRACK_INTRO;
    }
    t = &list.tracks[track % list.count];
    strcpy(path, "bgm/");
    if (intro)
        strcat(path, "i_");
    strcat(path, t->name);

    if (file_locate(path, hostName, &lsn, &size, &vol_device_object) == -1) {
        se_play(4, SE_PAN_CENTRE, -1, 1.0f);    /* 0x0022C658 */
        return;
    }
    ads_set_volume(volume);
    /* 64-bit add: (size + 0x7FF) >> 11 sectors; a 0x40000-byte stream buffer */
    pdibgm_start(lsn, (int)(((u64)size + 0x7FF) >> 11), arg, 0x40000, hostName);   /* 0x0025C8C8 */
}

// FUNC 0x0022AE18 ads_stop
void ads_stop(void)
{
    pdibgm_stop();                          /* 0x0025C9D0 */
}
