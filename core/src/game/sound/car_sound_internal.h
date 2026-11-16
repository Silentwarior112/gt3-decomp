/* car_sound_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u059,
 * 0x001A6488..0x001A9B90: translation units T176..T179.
 *
 *   T176  CarSound: the sound of one car (RaceEntryCar +0x18C4).  Three
 *         streamed engine voices, each with an EngineSound sample bank
 *         (a GTSOUNDINSTRUMENT with its own 0x100-byte header buffer), 32
 *         slots of looped sound effects and four one-shot effects.  Every
 *         voice is panned from its position relative to the listener
 *         (constant-power sin / cos pan, 32 / distance attenuation).  Plus
 *         the sound effects switch (g_003521E0).
 *   T177  RaceAmbientSound: the course ambience of a race - four looped
 *         effects mixed from the course sound data, two stereo pairs of
 *         tunnel sounds chosen by the runway under the camera, and the
 *         reverb level that follows them.
 *   T178  CourseEnv: the environment block of a course file (lights, fog,
 *         sun / flare / reflection parameters), 0xF50 bytes.
 *   T179  CourseData: the loaded course file (relocation, draw helpers) and
 *         the table of the courses of the volume (courses/<region>/...).
 *
 * Every shared class, function and global comes from the canonical headers,
 * which each file includes itself, in the order that gives the original's
 * type names (the data pass: each file's comment lists them).  This header
 * keeps what is local to the unit: the inline sound helpers, the runway
 * tunnel flags, partial views of the two record types that have no
 * canonical header (T164, T195), T178's two light model vectors and T179's
 * course table with its TU-local functions.  It defines no class: where a
 * file includes it does not matter to the type names.  Offsets in comments
 * are the ORIGINAL (egcs) ones.
 */
#ifndef U059_H
#define U059_H

#include "types.h"

class FlareClass;                           /* T159_flare.h (pointers only) */
class CourseData;                           /* T179_course_data.h */

/* ======================================================================== */
/* Sound effects (T469)                                                     */
/* ======================================================================== */

/* The effect ids u059 plays (SE_*, T469's SoundEffect) and se_program's
 * SE_PROGRAM_A / B: T469. */

/* SPU volumes: s16, 0x4000 (1 << 14) = unity, clamped to -0x4000..0x3FFF. */
#define SPU_VOLUME_UNITY    0x4000
#define SPU_VOLUME_MAX      0x3FFF
#define SPU_VOLUME_MIN      (-0x4000)
/* The lowest volume that still plays a one-shot sound (2^-14). */
#define SOUND_MIN_VOLUME    6.1035156e-05f

/* The voice priority of a volume (the same code is inlined in every user):
 * 0 at full volume, up to 255 when silent.  Matching form: the full-volume
 * case first (updateEngine's layout), the clamp as `t >= 0x100 ? 0xFF : t'
 * in an if / else (slti / movn; `prio = 0xFF; if (t < 0x100)' folds into a
 * MIN that tests 0xFF < t). */
static inline int sound_voice_priority(f32 volume)
{
    f32 mag = volume < 0.0f ? -volume : volume;
    int prio;
    if (1.0f <= mag) {
        prio = 0;
    } else {
        int t = (int)((1.0f - mag) * 255.0f);
        if (t >= 0x100)
            prio = 0xFF;
        else
            prio = t;
    }
    return prio;
}

/* Clamps to the SPU volume range [-0x4000, 0x3FFF]. */
static inline int spu_volume_clamp(int v)
{
    int r = SPU_VOLUME_MAX;
    if (v <= SPU_VOLUME_MAX) {
        r = v;
        if (r < SPU_VOLUME_MIN)
            r = SPU_VOLUME_MIN;
    }
    return r;
}

/* ======================================================================== */
/* T177: the runway under the camera                                        */
/* ======================================================================== */

/* RunwayQuery::attr2 bits that RaceAmbientSound reads. */
#define RUNWAY_ATTR2_TUNNEL     0x08        /* "tunnel" and reverb 0.5 */
#define RUNWAY_ATTR2_TUNNEL2    0x10        /* "tunnel2" and reverb 0.25 (both bits: reverb 0.25 only) */
#define RUNWAY_ATTR2_TUNNELS    (RUNWAY_ATTR2_TUNNEL | RUNWAY_ATTR2_TUNNEL2)

/* The reverb level: none, and its largest step per frame. */
#define AMBIENT_REVERB_NONE     0.125f
#define AMBIENT_REVERB_STEP     0.027777778f    /* 1 / 36 */
/* What snd_set_effect_volume gets when the ambience stops. */
#define AMBIENT_REVERB_OFF      0xFFF

/* ======================================================================== */
/* Records without a canonical header                                       */
/* ======================================================================== */

/* partial: owned by T195 (u055; no canonical header).  The flare sources of
 * the course file (CourseData +0x74). */
struct FlareSourceData {
    void relocate();                        /* 0x0019AB38 */
    void submit(FlareClass *fc, int group); /* 0x0019ABD8 group < 0: all */
};

/* partial: owned by T164 (u056; no canonical header).  The mini map record
 * of the course file (CourseData +0x84). */
struct RaceMiniMapData {
    void relocate();                        /* 0x0019E230 (? name, u056's) */
};

/* ======================================================================== */
/* T178 / T179 data and functions used only inside their TU                 */
/* ======================================================================== */

/* course_env_set_course_lights's light model vectors (T178 .data: not
 * const in the original, defined in T178). */
extern f32 g_002C2670[4];                   /* 0x002C2670 {0, 0, 0, 1} PGL_LIGHTMODEL_AMBIENT */
extern f32 g_002C2680[4];                   /* 0x002C2680 {0, 2, 0, 1} PGL_LIGHTMODEL_EYE */

/* An entry of the course table, 0x10 bytes. */
struct CourseListEntry {
    u64         code;                       /* 0x0 nameToCode(name) */
    const char *name;                       /* 0x8 the file name (in the volume's TOC image) */
    u32         unk_C;                      /* 0xC */
};

extern int              g_00352230;         /* 0x00352230 gp-0x57C0 the number of courses */
extern CourseListEntry *g_00352234;         /* 0x00352234 gp-0x57BC the table, sorted by code */
extern int              g_00352238;         /* 0x00352238 gp-0x57B8 the region (index into g_002C2AF0) */
extern const char      *g_002C2AF0[];       /* 0x002C2AF0 "courses/US", "courses/JP", "courses/US", "courses/UK", ... */

/* The course volume directory the table is built from. */
#define COURSE_DIR              "courses/US"
/* The course arena unit course_read reads a course into. */
#define COURSE_FILE_MAX         0xD00000

/* T179's functions that no other TU calls (no canonical declaration). */
void        course_draw_model54(CourseData *cd);                            /* 0x001A91B8 */
void        course_draw_parts(CourseData *cd, const u16 *parts, int flags); /* 0x001A93B0 */
void        course_reset_anims(CourseData *cd);                             /* 0x001A9720 */
void        course_release_clut_anims(CourseData *cd, int id);              /* 0x001A9830 */
int         course_code_compare(const void *a, const void *b);              /* 0x001A9898 */
int         course_find(u64 code);                                          /* 0x001A99D8 */
const char *course_file_name(u64 code);                                     /* 0x001A9A30 */
char       *course_path(u64 code, char *buf);                               /* 0x001A9A60 */
int         course_list_names(const char ***names);                         /* 0x001A9B20 */

#endif /* U059_H */
