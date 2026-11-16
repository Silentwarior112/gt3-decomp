/* T177_race_ambient_sound.cpp - RaceAmbientSound (T177), u059.
 *
 * The ambience of a race, a member of RacePS2Base (+0xC6A8).  Every frame,
 * for every view's camera, the course sound data (CourseData +0x8C, T458)
 * computes a mix that is summed into five channels of {left, right, pitch};
 * channels 0, 1, 3 and 4 drive the looped effects "air", "city", "crowds"
 * and "sea".  The runway attribute under the camera chooses one of two
 * tunnel sounds (stereo pairs of "tunnel" / "tunnel2") and the reverb
 * level, which moves towards its target by at most 1/36 per frame.
 *
 * Data (retail): .data = the frame info alone (0x002C24F0), .lit4
 * 0x0034FF34; .rodata 0x00330A30..0x00330A69, nothing the code references:
 * <new>'s "bad_alloc", then the type names "14RaceCameraBase",
 * "t9UnitArena1Ui64" (UnitArena<64> instantiated before RaceCameraBase is
 * defined: T153 with GAME_T153_EMITS_TYPEINFO first; no GameStatus).
 */
// DATA_START 0x00330A30
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (T153 suppresses it by default) */
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's first .rodata string) */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (data), RunwayQuery: UnitArena<64> */
#include "game/camera/T383_camera_manager.h"       /* CameraManager, CameraView: RaceCameraBase */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, course, race) */
#include "game/race/core/T129_race_base.h"            /* RaceBase::virtual_6 (the camera manager) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase::sortByRank */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase (CameraView::camera) */
#include "game/course/T179_course_data.h"          /* CourseData (sound, runway) */
#include "game/course/T457_runway.h"               /* runway_query */
#include "game/sound/T458_course_sound.h"         /* CourseSound, SoundMix, sound_mix_*, course_sound_mix */
#include "game/sound/T468_gtsound.h"              /* GtSoundHandle */
#include "game/sound/T469_sound_effects.h"        /* se_program, SE_*, SE_PROGRAM_A / B */
#include "engine/media/G002_mpeg_frameq.h"          /* snd_set_effect_volume */
#include "game/sound/T176_car_sound.h"            /* g_003521E0, g_003521E4 */
#include "game/sound/T177_race_ambient_sound.h"   /* this translation unit */
#include "game/sound/car_sound_internal.h"                           /* sound_voice_priority, RUNWAY_ATTR2_*, AMBIENT_REVERB_* */

/* max.s / min.s: volatile asm statements, as every max.s / min.s of the
 * image behaves (u018, u032, u044).  GCC 14: the C ternaries. */
static inline f32 amb_maxf(f32 a, f32 b)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 r;
    __asm__ volatile("max.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
#else
    return a > b ? a : b;
#endif
}

static inline f32 amb_minf(f32 a, f32 b)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 r;
    __asm__ volatile("min.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
#else
    return a < b ? a : b;
#endif
}

/* A channel's volumes and pitch, and its pitch alone (inline helpers: their
 * pointer argument stays in a register for the stores at +4 / +8, as
 * retail's; ch[4]'s address is computed at the start and spilled). */
static inline void channel_set(SoundChannel *c, f32 l, f32 r, f32 p)
{
    c->left = l;
    c->right = r;
    c->pitch = p;
}

static inline void channel_pitch(SoundChannel *c, f32 p)
{
    c->pitch = p;
}

/* Clamps a mix volume to -1..1 (an inline helper with a result of its own:
 * retail loads the volume into $f0 and copies it to the argument register). */
static inline f32 amb_clamp1(f32 v)
{
    f32 r;
    if (v < -1.0f)
        r = -1.0f;
    else if (1.0f < v)
        r = 1.0f;
    else
        r = v;
    return r;
}

// FUNC 0x001A77E0 RaceAmbientSound::RaceAmbientSound
//   Members: channel[5], tunnelA = &channel[2], tunnelB, tunnel2A, tunnel2B
//   (all GtSoundHandle()).  Effects: channels 0, 1, 3, 4 = "air", "city",
//   "crowds", "sea" (program A); the tunnel pairs = programs A / B of
//   "tunnel" and "tunnel2".
//   98.1%: only the stack slots of three spilled member addresses differ
//   (&channel[4], &tunnel2A, &tunnel2B: reload numbers pseudos in another
//   order); the reference compiler 991111-01 gives exactly retail's code.
RaceAmbientSound::RaceAmbientSound() : tunnelA(&channel[2])
{
    /* implicit: GtSoundHandle (member constructors) */
    channel[0].set(se_program(SE_AIR, SE_PROGRAM_A));
    channel[1].set(se_program(SE_CITY, SE_PROGRAM_A));
    channel[3].set(se_program(SE_CROWDS, SE_PROGRAM_A));
    channel[4].set(se_program(SE_SEA, SE_PROGRAM_A));
    tunnelA->set(se_program(SE_TUNNEL, SE_PROGRAM_A));
    tunnelB.set(se_program(SE_TUNNEL, SE_PROGRAM_B));
    tunnel2A.set(se_program(SE_TUNNEL2, SE_PROGRAM_A));
    tunnel2B.set(se_program(SE_TUNNEL2, SE_PROGRAM_B));
    playing = 0;
    reverb = AMBIENT_REVERB_NONE;
}

// FUNC 0x001A7980 RaceAmbientSound::~RaceAmbientSound
//   Nothing to destroy; egcs's operator delete when asked.
RaceAmbientSound::~RaceAmbientSound()
{
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x001A79A8 RaceAmbientSound::start
//   RacePS2Base::startSounds.
void RaceAmbientSound::start()
{
    if (playing == 0)
        playing = 1;
}

// FUNC 0x001A79C0 RaceAmbientSound::update
//   RacePS2Base::virtual_29, unless in the background mode.  Note: `playing'
//   is not tested.  The ranks are sorted first (sortByRank): the sources
//   with a lap section (group 3, "crowds") follow the cars on it, walked in
//   rank order from rankTop by course_sound_mix (T458).
//   92.9% (991111-01: 94.5%): one counter for the views and the channels
//   and one variable for the reverb mode and the SPU depth (retail's s1 /
//   s2), the channels walked by pointer, the volatile max.s / min.s, the
//   switch's decision tree with `case 0', the channel helpers.  Left: the
//   tunnel levels and the -1.0 bound in other saved FP registers (global
//   priorities), the spill slots of the addresses hoisted to the entry (GCSE
//   order; 991111-01 has two of the seven swapped), the pos[3] / side[3] /
//   vel[3] store order, and the play call's `this' loaded before the
//   priority (computing the priority in a statement first puts it right but
//   hoists the views == 1 constant).  Tried: 30 orders of the side / pos
//   stores, if-chain and case orders, clamps by value / pointer / ternary,
//   -ffast-math.
void RaceAmbientSound::update(RaceOrganization *org)
{
    CourseData *course = org->course->data;
    CourseSound *sound = course->sound;
    if (sound == 0)
        return;

    int i;                                  /* the view, then the channel */
    int reverbMode = 0;                     /* 0 none, 1 reverb 0.25, 2 reverb 0.5 */
    Runway *runway = course->runway;
    org->entries->sortByRank();
    f32 tunnelLevel = 0.0f;
    f32 tunnel2Level = 0.0f;
    CameraManager *cameras = org->race->virtual_6();
    int views = cameras->viewCount;
    SoundMix mix;
    sound_mix_clear(&mix);

    for (i = 0; i < views; i++) {
        f32 pos[4], side[4], vel[4];
        SoundMix sample;                    /* (a 0x40-byte stack slot) */
        RunwayQuery query;

        cameras->setCurrentView(i);
        pos[0] = cameras->getPositionX();
        pos[1] = cameras->getPositionY();
        pos[2] = cameras->getPositionZ();
        /* the camera's x axis: column 0 of its view matrix */
        side[3] = 1.0f;
        VariableCameraBase *cam = cameras->views[cameras->current].camera;
        side[0] = cam->matrix[0][0];
        side[1] = cam->matrix[1][0];
        side[2] = cam->matrix[2][0];
        pos[3] = 1.0f;
        if (cam->doppler != 0) {
            /* the eye's motion since the last frame */
            vel[0] = pos[0] - cam->position[1].x;
            vel[1] = pos[1] - cam->position[1].y;
            vel[3] = 1.0f;
            vel[2] = pos[2] - cam->position[1].z;
        } else {
            vel[3] = 1.0f;
            vel[0] = 0.0f;
            vel[1] = 0.0f;
            vel[2] = 0.0f;
        }
        course_sound_mix(sound, &sample, org, pos, side, vel);

        query.x = pos[0];
        query.y = pos[1];
        query.z = pos[2];
        query.sector = -1;
        int attr2 = runway_query(runway, &query) != 0 ? query.attr2 : 0;
        switch (attr2 & RUNWAY_ATTR2_TUNNELS) {
        case 0:
            break;
        case RUNWAY_ATTR2_TUNNELS:
            reverbMode = 1;
            break;
        case RUNWAY_ATTR2_TUNNEL:
            tunnelLevel = 1.0f;
            reverbMode = 2;
            break;
        case RUNWAY_ATTR2_TUNNEL2:
            tunnel2Level = 1.0f;
            reverbMode = 1;
            break;
        }
        sound_mix_add(&mix, &sample);
    }

    /* the reverb: a split screen has none of the tunnel reverb */
    if (!(views < 2))
        reverbMode = 0;
    f32 target = reverbMode != 0 ? (f32)reverbMode * 0.25f : AMBIENT_REVERB_NONE;
    f32 level = reverb;
    f32 step = target - level;
    f32 maxStep = AMBIENT_REVERB_STEP;
    if (step < 0.0f)
        step = amb_maxf(step, -maxStep);
    else
        step = amb_minf(step, maxStep);
    level += step;
    reverb = level;
    reverbMode = (int)(level * 32767.0f);   /* now the SPU depth */
    snd_set_effect_volume(reverbMode, reverbMode);

    /* groups 0 ("air") and 1 ("city") with any source play at full level */
    if (sound->groups[0]->count != 0)
        channel_set(&mix.ch[0], 1.0f, 1.0f, 1.0f);
    if (sound->groups[1]->count != 0)
        channel_set(&mix.ch[1], 1.0f, 1.0f, 1.0f);

    f32 gain = g_003521E0 * g_003521E4;
    channel_pitch(&mix.ch[4], 1.0f);
    tunnelLevel = tunnelLevel * gain;
    tunnel2Level = tunnel2Level * gain;
    SoundChannel *c = mix.ch;
    for (i = 0; i < 5; i++, c++) {
        if (i == 2)
            continue;                       /* channel 2 is tunnelA, played below */
        f32 l = amb_clamp1(c->left) * gain;
        f32 r = amb_clamp1(c->right) * gain;
        f32 pitch = c->pitch;
        f32 louder = amb_maxf(l, r);
        if (views != 1) {
            /* several screens: centred, unpitched */
            r = louder;
            pitch = 1.0f;
            l = r;
        }
        channel[i].play(sound_voice_priority(louder), l, r, pitch);
    }

    int prio = sound_voice_priority(tunnelLevel);
    tunnelA->play(prio, tunnelLevel, 0.0f, 1.0f);
    tunnelB.play(prio, 0.0f, tunnelLevel, 1.0f);
    int prio2 = sound_voice_priority(tunnel2Level);
    tunnel2A.play(prio2, 0.0f, tunnel2Level, 1.0f);
    tunnel2B.play(prio2, tunnel2Level, 0.0f, 1.0f);
}

// FUNC 0x001A8000 RaceAmbientSound::stop
//   RacePS2Base::stopSounds: stops everything (with the fade), reverb back
//   to none and the SPU's reverb level snd_set_effect_volume(0xFFF, 0xFFF).
void RaceAmbientSound::stop()
{
    if (playing != 0) {
        playing = 0;
        for (int i = 0; i < 5; i++) {
            if (i != 2)
                channel[i].stop(1);
        }
        tunnelA->stop(1);
        tunnelB.stop(1);
        tunnel2A.stop(1);
        tunnel2B.stop(1);
        reverb = AMBIENT_REVERB_NONE;
        snd_set_effect_volume(AMBIENT_REVERB_OFF, AMBIENT_REVERB_OFF);
    }
}
