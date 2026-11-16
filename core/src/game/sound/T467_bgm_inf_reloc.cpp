/* T467 - relocation of the track lists of bgm/ads.inf and music/music.inf.
 * 0x0022AE38..0x0022AF18.  (No header of its own: T466's and T463's
 * relocations are the only callers; declared in gtsound_internal.h.)  No data but its
 * frame info. */
#include "game/sound/gtsound_internal.h"

/* Words 0x0..0xC of a track are offsets; 0x10 too when `markers' (ads.inf;
 * in music.inf it is the sequence index). */
// FUNC 0x0022AE38 bgm_track_relocate
void bgm_track_relocate(BgmTrack *t, int delta, int markers)
{
    u104_reloc(t->group, delta);
    u104_reloc(t->name, delta);
    u104_reloc(t->title, delta);
    u104_reloc(t->artist, delta);
    if (markers)
        u104_reloc(t->markers, delta);
}

// FUNC 0x0022AE88 bgm_tracklist_relocate
void bgm_tracklist_relocate(BgmTrackList *l, int delta, int markers)
{
    u32 i;

    u104_reloc(l->tracks, delta);
    for (i = 0; i < l->count; i++)
        bgm_track_relocate(&l->tracks[i], delta, markers);
}
