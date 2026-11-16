/* T177_race_ambient_sound.h - RaceAmbientSound, the ambient sound of a race
 * (translation unit T177, unit u059), 0x001A77E0..0x001A80E0.
 *
 * A member of RacePS2Base (T130, +0xC6A8): built by its constructor
 * (0x0017D070), started / stopped by startSounds / stopSounds and updated by
 * virtual_29 every frame outside the background mode; destroyed by
 * virtual_0.  update() mixes the course sound data (CourseSound, T458;
 * CourseData +0x8C) for the camera of every view (course_sound_mix,
 * sound_mix_add) into five channels {left, right, pitch} and plays four of
 * them on looped sound effects (T469): 0 "air", 1 "city", 3 "crowds", 4
 * "sea" (course_sound_mix sets channel 2 to {1, 1, 1}; the handle of
 * channel 2 is the left half of the "tunnel" pair below).  The runway
 * attribute attr2 under each camera (runway_query, T457) chooses the tunnel
 * sound and the reverb level:
 *   attr2 & 0x18 == 0x08  "tunnel" at full volume, reverb 0.5
 *                 == 0x10  "tunnel2" at full volume, reverb 0.25
 *                 == 0x18  reverb 0.25, no tunnel sound
 *                 == 0     reverb 0.125 (none)
 * Each tunnel sound is a stereo pair: programs A and B of its effect
 * (se_program(se, 1 / 2)) played hard left and hard right.  The reverb level
 * moves towards its target by at most 1/36 per frame and goes to the SPU as
 * level x 32767 (snd_set_effect_volume, both sides).  With several views the channels
 * play centred and unpitched and there is no tunnel reverb.
 *
 * Names: the methods are u044's (its partial RaceAmbientSound) and u059's;
 * the fields u059's (the first pass: active, ch, tunnelA0, tunnelA1,
 * tunnelB0, tunnelB1, reverb; u044 had only unk_00[0x4C]).
 */
#ifndef GAME_T177_RACE_AMBIENT_SOUND_H
#define GAME_T177_RACE_AMBIENT_SOUND_H

#include "game/game.h"
#include "game/sound/T468_gtsound.h"              /* GtSoundHandle (held by value) */

class RaceOrganization;                     /* T125 (pointers only: forward declarations) */

/* ? name (the first pass's; u044 T177Object).  Not polymorphic, 0x4C bytes
 * (RacePS2Base +0xC6A8). */
class RaceAmbientSound {
public:
    /* The handles in member order (channel[5]; tunnelA = &channel[2] is
     * stored before tunnelB is built; tunnelB, tunnel2A, tunnel2B), then the
     * programs: channels 0 / 1 / 3 / 4 the effects "air", "city", "crowds",
     * "sea" (program A), tunnelA / tunnelB "tunnel" A / B, tunnel2A /
     * tunnel2B "tunnel2" A / B; playing = 0, reverb = 0.125. */
    RaceAmbientSound();                     /* 0x001A77E0 */
    /* Nothing to release; egcs's flags argument (bit 0: operator delete).
     * Callers: RacePS2Base's constructor (EH), RacePS2Base::virtual_0. */
    ~RaceAmbientSound();                    /* 0x001A7980 */
    /* playing = 1 (RacePS2Base::startSounds). */
    void start();                           /* 0x001A79A8 */
    /* Per frame (RacePS2Base::virtual_29): org's course sound mixed for
     * every view's camera (its side axis, the matrix's column 0, and its
     * motion since the last frame when the camera's doppler is set), the
     * tunnel and reverb from the runway, then the channels and the tunnel
     * pairs played at the volume factors g_003521E0 x g_003521E4 (T176).
     * Nothing without course sound data.  `playing' is not tested. */
    void update(RaceOrganization *org);     /* 0x001A79C0 */
    /* When playing: playing = 0, channels 0, 1, 3, 4 and the tunnel pairs
     * stopped (GtSoundHandle::stop(1)), reverb = 0.125 and the SPU's level
     * 0xFFF (RacePS2Base::stopSounds). */
    void stop();                            /* 0x001A8000 */

    int            playing;                 /* 0x00 start / stop (u059 active) */
    GtSoundHandle  channel[5];              /* 0x04 the course sound mix's: 0 "air", 1 "city", 2 tunnelA,
                                               3 "crowds", 4 "sea" (u059 ch) */
    GtSoundHandle *tunnelA;                 /* 0x2C &channel[2]: "tunnel" program A, hard left (u059 tunnelA0) */
    GtSoundHandle  tunnelB;                 /* 0x30 "tunnel" program B, hard right (u059 tunnelA1) */
    GtSoundHandle  tunnel2A;                /* 0x38 "tunnel2" program A, hard right (u059 tunnelB0) */
    GtSoundHandle  tunnel2B;                /* 0x40 "tunnel2" program B, hard left (u059 tunnelB1) */
    f32            reverb;                  /* 0x48 the reverb level: 0.125 (none), 0.25, 0.5 */
};
EGCS_SIZE(RaceAmbientSound, 0x4C);

#endif /* GAME_T177_RACE_AMBIENT_SOUND_H */
