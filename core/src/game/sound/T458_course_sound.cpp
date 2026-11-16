/* T458 - CourseSound (? name): the ambient sound sources of the course.
 *
 * The file (CourseData+0x8C) has five groups (pointer tables) of sources;
 * a source has emitters (position, volume).  For a listener the groups mix
 * into five channels {left, right, pitch} (SoundMix, u059's
 * RaceAmbientSound plays them; channel 2 is reset to 1, 1, 1 here).  The
 * sources of group 3 also have a section of the lap (two ends located on
 * the runway): their level follows the cars on the section (the leader
 * counts 1, the next 0.8, 0.64, ...), at most 1/60 per frame.
 */
/* No .rodata in retail (no type names, no literals): only headers that
 * emit none. */
#include "game/menu/gtmode/prize_model_internal.h"
#include "engine/media/G002_mpeg_frameq.h"          /* sinf */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization */
#include "game/car/physics/T302_car_state.h"            /* car_lap_distance */
#include "game/course/T457_runway.h"               /* runway_lap_distance */
#include "game/sound/T458_course_sound.h"

// FUNC 0x00228BB0 sound_channel_add
//   The volumes add; the pitch factors add as (a - 1).
void sound_channel_add(SoundChannel *c, const SoundChannel *a)
{
    c->left = c->left + a->left;
    c->right = c->right + a->right;
    c->pitch = c->pitch + (a->pitch - 1.0f);
}

// FUNC 0x00228BF0 sound_mix_clear
//   Every channel {0, 0, 1}.  (Called by RaceAmbientSound::update, T177.)
void sound_mix_clear(SoundMix *m)
{
    SoundChannel *ch = m->ch;

    do {
        ch->left = 0.0f;
        ch->right = 0.0f;
        ch->pitch = 1.0f;
        ch++;
    } while (ch < &m->ch[5]);
}

// FUNC 0x00228C28 sound_mix_add
void sound_mix_add(SoundMix *m, const SoundMix *a)
{
    const SoundChannel *src = a->ch;
    SoundChannel *dst = m->ch;

    do {
        sound_channel_add(dst, src);
        src++;
        dst++;
    } while (src < &a->ch[5]);
}

// FUNC 0x00228C80 sound_source_locate
//   onLap (group 3): level 0 and the lap distances of the two section ends
//   (when both are on the road), ordered so that the section is the
//   shorter way round from ends[0] to ends[1].  Else level 1 and no
//   section (both distances 0).
void sound_source_locate(SoundSource *s, Runway *r, int onLap)
{
    RunwayQuery q;                              /* sp+0x00 */
    struct {
        int found;
        f32 dist;
    } end[2];                                   /* sp+0x40 */
    f32 startDist, endDist;
    int k;

    if (onLap != 0)
        s->level = 0.0f;
    else
        s->level = 1.0f;
    s->ends[0].lapDist = 0.0f;
    s->ends[1].lapDist = 0.0f;
    if (onLap == 0)
        return;
    for (k = 0; k < 2; k++) {
        q.x = s->ends[k].x;
        q.y = s->ends[k].y;
        q.z = s->ends[k].z;
        q.sector = -1;
        end[k].found = runway_lap_distance(r, &q);
        end[k].dist = q.lapDist;
    }
    if (end[0].found == 0)
        return;
    if (end[1].found == 0)
        return;
    startDist = end[0].dist;
    endDist = end[1].dist;
    if ((endDist < startDist && (startDist - endDist) < r->lapLength * 0.5f) ||
        (startDist < endDist && r->lapLength * 0.5f < (endDist - startDist))) {
        f32 swap = startDist;
        startDist = endDist;
        endDist = swap;
    }
    s->ends[0].lapDist = startDist;
    s->ends[1].lapDist = endDist;
}

// FUNC 0x00228E08 sound_source_mix
//   Adds the source to out for the listener at pos (side: the unit axis to
//   the right, ? name; vel: its velocity in m per 1/60 s).  The level: 1,
//   or for a source with a section the cars on it (see the file comment).
//   Per emitter: volume * level / max(1, d / 32); pan = (dir . side + 1) / 2
//   clamped to 0..1, left sin((1 - pan) * pi/2), right sin(pan * pi/2);
//   pitch (Doppler) c / (c - dir . vel) - 1 with c = 5.524167 (331.45 m/s
//   per 1/60 s), weighted by the volume and averaged over the emitters.
void sound_source_mix(SoundSource *s, SoundChannel *out, RaceOrganization *org,
                      const f32 *pos, const f32 *side, const f32 *vel)
{
    f32 level = 1.0f;
    f32 startDist = s->ends[0].lapDist;
    f32 endDist = s->ends[1].lapDist;
    f32 left, right, pitch;
    u32 i;
    RaceEntryBase *entries = org->entries;

    if (startDist != endDist) {
        f32 weight = 1.0f;
        int wraps = 1;                          /* the section crosses the line */
        RaceEntryCar *car;
        f32 cur;

        if (!(endDist < startDist))
            wraps = 0;
        car = entries->rankTop;
        level = 0.0f;
        while (car != NULL) {
            f32 carDist = car_lap_distance(car->state, 0.0f);
            if (wraps == 0) {
                if (startDist <= carDist && carDist <= endDist)
                    level += weight;
            } else {
                if (startDist <= carDist || carDist <= endDist)
                    level += weight;
            }
            car = car->rankNext;
            weight *= 0.8f;
        }
        if (1.0f <= level)
            level = 1.0f;
        cur = s->level;
        if (level < cur)
            cur = cur - u103_min(cur - level, 0.016666668f);
        else
            cur = cur + u103_min(level - cur, 0.016666668f);
        level = cur;
        s->level = cur;
    }

    right = 0.0f;
    left = 0.0f;
    pitch = 0.0f;
    for (i = 0; i < s->emitterCount; i++) {
        f32 dx = s->emitters[i].x - pos[0];
        f32 dy = s->emitters[i].y - pos[1];
        f32 dz = s->emitters[i].z - pos[2];
        f32 vol = s->emitters[i].volume * level;
        f32 dist = boot_sqrtf_v((dx * dx + dy * dy) + dz * dz);
        f32 atten, approach, pan;
        f32 nearDist;                           /* a variable: retail divides by 32.0 (div.s), it
                                                   does not multiply by 1/32 as a constant gives */

        if (dist == 0.0f)
            dist = 1.0f;
        nearDist = 32.0f;
        dx = dx / dist;
        dy = dy / dist;
        atten = 1.0f;
        dz = dz / dist;
        if (!(dist < nearDist))
            atten = dist / nearDist;
        vol = vol / atten;
        approach = (dx * vel[0] + dy * vel[1]) + dz * vel[2];
        pan = (dx * side[0] + dy * side[1]) + dz * side[2];
        pitch += (5.524167f / (5.524167f - approach) - 1.0f) * vol;
        pan = u103_max((pan + 1.0f) * 0.5f, 0.0f);
        pan = u103_min(pan, 1.0f);
        left += sinf((1.0f - pan) * 1.5707964f) * vol;
        right += sinf((pan + 0.0f) * 1.5707964f) * vol;    /* (sic: + 0.0) */
    }
    if (s->emitterCount != 0)
        pitch = pitch / (f32)s->emitterCount;
    out->left = out->left + left;
    out->right = out->right + right;
    out->pitch = out->pitch + pitch;
}

// FUNC 0x002291C8 sound_list_relocate
void sound_list_relocate(SoundSourceList *l, int delta)
{
    int left = l->count;
    SoundSource **item = l->items;

    if (left > 0) {
        do {
            left--;
            u103_reloc(*item, delta);
            item++;
        } while (left != 0);
    }
}

// FUNC 0x00229200 sound_list_locate
void sound_list_locate(SoundSourceList *l, Runway *r, int onLap)
{
    int i;

    for (i = 0; i < l->count; i++)
        sound_source_locate(l->items[i], r, onLap);
}

// FUNC 0x00229288 sound_list_mix
//   The sources of a group into one channel; the pitch is averaged over
//   the sources.
void sound_list_mix(SoundSourceList *l, SoundChannel *out, RaceOrganization *org,
                    const f32 *pos, const f32 *side, const f32 *vel)
{
    int i;

    for (i = 0; i < l->count; i++)
        sound_source_mix(l->items[i], out, org, pos, side, vel);
    if (l->count != 0)
        out->pitch = (out->pitch - 1.0f) / (f32)l->count + 1.0f;
}

// FUNC 0x00229370 course_sound_relocate
//   Relocates a loaded file to its address (always, even when it is
//   there).  Called by course_data_relocate (T179).
void course_sound_relocate(CourseSound *s)
{
    int delta = (u8 *)s - (u8 *)s->self;
    int group;

    s->self = s;
    for (group = 0; group < 5; group++) {
        u103_reloc(s->groups[group], delta);
        sound_list_relocate(s->groups[group], delta);
    }
}

// FUNC 0x002293F0 course_sound_locate
//   Called by F_001A9180 (T179) once the runway is there.
void course_sound_locate(CourseSound *s, Runway *r)
{
    int group;

    for (group = 0; group < 5; group++)
        sound_list_locate(s->groups[group], r, group == 3);
}

// FUNC 0x00229450 course_sound_mix
//   The mix of the five groups for a listener; channel 2 is then {1, 1, 1}
//   (u059: driven by the tunnel sound instead).
void course_sound_mix(CourseSound *s, SoundMix *out, RaceOrganization *org,
                      const f32 *pos, const f32 *side, const f32 *vel)
{
    SoundChannel *ch2 = &out->ch[2];
    int group;

    sound_mix_clear(out);
    for (group = 0; group < 5; group++)
        sound_list_mix(s->groups[group], &out->ch[group], org, pos, side, vel);
    out->ch[2].left = 1.0f;
    ch2->right = 1.0f;
    ch2->pitch = 1.0f;
}
