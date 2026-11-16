/* T458_course_sound.h - CourseSound (? name, as in u059): the ambient sound
 * sources of the course (translation unit T458, unit u103).
 *
 * The file (CourseData +0x8C) has five groups (pointer tables) of sources; a
 * source has emitters (position, volume).  For a listener (position, side
 * axis, velocity) the groups mix into five channels {left, right, pitch}:
 * distance attenuation (1 / (d / 32) beyond 32 m), a pan from the side axis
 * and a Doppler factor (c = 5.524167: 331.45 m/s per 1/60 s).  u059's
 * RaceAmbientSound plays them; course_sound_mix sets channel 2 to {1, 1, 1}.
 * The sources of group 3 also have a section of the lap (two ends located on
 * the runway): their level follows the cars on the section.
 */
#ifndef GAME_T458_COURSE_SOUND_H
#define GAME_T458_COURSE_SOUND_H

#include "game/game.h"

struct Runway;                                  /* T457_runway.h */
#ifdef __cplusplus
class RaceOrganization;                         /* T125_race_organization.h (a class there) */
#else
typedef struct RaceOrganization RaceOrganization;   /* (C: pointers only) */
#endif

/* ? names.  One channel of the mix: the left / right volumes and the pitch
 * factor (1.0: none). */
typedef struct SoundChannel {
    f32 left;                                   /* 0x00 */
    f32 right;                                  /* 0x04 */
    f32 pitch;                                  /* 0x08 */
} SoundChannel;
EGCS_SIZE(SoundChannel, 0xC);

/* The mix of the five groups, one channel each. */
typedef struct SoundMix {
    SoundChannel ch[5];                         /* 0x00 */
} SoundMix;
EGCS_SIZE(SoundMix, 0x3C);

/* ? name.  A point of a sound source, 0x10 bytes. */
typedef struct SoundEmitter {
    f32 x;                                      /* 0x00 */
    f32 y;                                      /* 0x04 */
    f32 z;                                      /* 0x08 */
    f32 volume;                                 /* 0x0C */
} SoundEmitter;
EGCS_SIZE(SoundEmitter, 0x10);

/* ? name.  An end of a source's lap section (group 3), 0x10 bytes. */
typedef struct SoundSectionEnd {
    f32 x;                                      /* 0x00 */
    f32 y;                                      /* 0x04 */
    f32 z;                                      /* 0x08 */
    f32 lapDist;                                /* 0x0C runtime: its distance along the lap */
} SoundSectionEnd;
EGCS_SIZE(SoundSectionEnd, 0x10);

/* ? name.  A sound source. */
typedef struct SoundSource {
    u32             emitterCount;               /* 0x00 */
    u8              unk_04[8];                  /* 0x04 */
    f32             level;                      /* 0x0C runtime: the level of the cars on the section */
    SoundSectionEnd ends[2];                    /* 0x10 the section of the lap (group 3) */
    SoundEmitter    emitters[1];                /* 0x30 [emitterCount] */
} SoundSource;

/* ? name.  A group: a table of sources. */
typedef struct SoundSourceList {
    int          count;                         /* 0x00 (u059 tests it for groups 0 and 1) */
    SoundSource *items[1];                      /* 0x04 [count] */
} SoundSourceList;

/* ? name.  The file. */
typedef struct CourseSound {
    u8                  unk_00[4];              /* 0x00 */
    struct CourseSound *self;                   /* 0x04 the address the offsets are for */
    u8                  unk_08[0xC];            /* 0x08 */
    SoundSourceList    *groups[5];              /* 0x14 */
} CourseSound;

/* The volumes add; the pitch factors add as (a - 1). */
void sound_channel_add(SoundChannel *c, const SoundChannel *a);     /* 0x00228BB0 */
void sound_mix_clear(SoundMix *m);                                  /* 0x00228BF0 every channel {0, 0, 1} */
void sound_mix_add(SoundMix *m, const SoundMix *a);                 /* 0x00228C28 */
/* onLap (group 3): level 0 and the lap distances of the section ends;
 * else level 1 and no section. */
void sound_source_locate(SoundSource *s, struct Runway *r, int onLap);  /* 0x00228C80 */
/* Adds the source to out for the listener at pos (side: the unit axis to
 * the right, ? name; vel: in m per 1/60 s). */
void sound_source_mix(SoundSource *s, SoundChannel *out, RaceOrganization *org,
                      const f32 *pos, const f32 *side, const f32 *vel);  /* 0x00228E08 */
void sound_list_relocate(SoundSourceList *l, int delta);            /* 0x002291C8 */
void sound_list_locate(SoundSourceList *l, struct Runway *r, int onLap); /* 0x00229200 */
/* The sources of a group into one channel (the pitch averaged). */
void sound_list_mix(SoundSourceList *l, SoundChannel *out, RaceOrganization *org,
                    const f32 *pos, const f32 *side, const f32 *vel);    /* 0x00229288 */
/* Relocates the file in place (always). */
void course_sound_relocate(CourseSound *s);                         /* 0x00229370 */
/* Once the runway is there: locates the sections of group 3. */
void course_sound_locate(CourseSound *s, struct Runway *r);         /* 0x002293F0 */
void course_sound_mix(CourseSound *s, SoundMix *out, RaceOrganization *org,
                      const f32 *pos, const f32 *side, const f32 *vel);  /* 0x00229450 */

#endif /* GAME_T458_COURSE_SOUND_H */
