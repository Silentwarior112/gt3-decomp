/* T176_car_sound.h - CarSound, the sound of one car, and its parts:
 * EngineSound, CarSoundPan, CarSoundEngine, CarSoundSlot (translation unit
 * T176, unit u059), 0x001A6488..0x001A77E0.
 *
 * A car (RaceEntryCar +0x18C4, T121) has three streamed engine voices
 * (CarSoundEngine: a GtSoundVoice and the EngineSound bank it plays; banks
 * 0 / 1 the car's engine files, bank 2 the road noise bank's header), 32
 * slots of looped sound effects (T469: "turbine2", "turbine", "in_car",
 * "body_noiz", then seven per wheel) and four one-shot effects ("b_off",
 * "clash", "clash2", "clash3": 13, 15, 16, 17).  The effect names are
 * T469's table (0x002E9380, effect id -> se.inf name); se_program(se, 0)
 * is the effect's program A.  Every frame RaceEntryCar::updateSound fills
 * a CarSoundFrame (T121; u042 / u059 CarSoundParams) and calls update:
 * each voice is panned from
 * its position relative to the listener (constant-power sin / cos pan,
 * 32 / distance attenuation clamped to 1) and scaled by the volume factors
 * g_003521E0 x g_003521E4 (both here).
 *
 * CarSoundEngine / CarSoundSlot are here, not in T121 (the plan's): they
 * hold EngineSound by value and CarSound holds them, while T121's
 * RaceEntryCar holds CarSound (h23).  This header must not include T121.
 */
#ifndef GAME_T176_CAR_SOUND_H
#define GAME_T176_CAR_SOUND_H

#include "game/game.h"
#include "game/sound/T468_gtsound.h"              /* base GTSOUNDINSTRUMENT; GtSoundVoice, GtSoundHandle (held by value) */
#include "engine/media/G002_mpeg_frameq.h"          /* SndVoiceParam (SpuVoiceParams) */

struct CarSoundFrame;                       /* T121 (pointers only: forward declarations) */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* the __si_type_info at 0x004B2390 (base __ti17GTSOUNDINSTRUMENT) */
extern "C" const type_info &__tf11EngineSound(void);   /* 0x001A7790 */
#endif

/* u059 / u104's SpuVoiceParams, what snd_play_voice (G002, 0x002685A8)
 * keys a voice on with, is G002's SndVoiceParam (h05): u059 / u104's volL /
 * volR are its `vol' (volL | volR << 16), their adsr2 (+0x0C) its `adsr'
 * and their adsr1 (+0x10) its unk_10 (SoundProgram::unk_0A), byte13 its
 * flags. */
typedef SndVoiceParam SpuVoiceParams;

/* ------------------------------------------------------------------------ */
/* EngineSound                                                              */
/* ------------------------------------------------------------------------ */

/* vtable 0x00330920 (2 slots), typeinfo __tf11EngineSound 0x001A7790 (si,
 * base GTSOUNDINSTRUMENT).  vptr +0x1C (GTSOUNDINSTRUMENT's).  Size 0x124
 * (CarSoundEngine +0x44).  An engine sample bank with its own 0x100-byte
 * header buffer (fixed); bank 0 of a car links bank 1, which is placed
 * right after it in the SPU area CarSound::reserveSpu took. */
class EngineSound : public GTSOUNDINSTRUMENT {
public:
    /* The header buffer of its own (u059 GTSOUNDINSTRUMENT names: own = 1,
     * data = &buf, capacity 0x100): fixed = 1, header = &buf,
     * headerCapacity = 0x100; link = NULL. */
    struct Buffer { u8 b[0x100]; };

    EngineSound();                          /* 0x001A6488 */
    /* The vptr, then ~GTSOUNDINSTRUMENT with the flags (it deletes). */
    virtual ~EngineSound();                 /* [0] +0x0C 0x001A64D8 (index: virtual_0) */
    /* At the end of transfer(): the linked bank goes right after this one
     * in SPU RAM (spuAddr + spuSize) and has spuSize less room (u059
     * virtual_1; h09). */
    virtual void onTransfer();              /* [1] +0x14 0x001A6500 */
    /* memcpy(buf, src->header, src->headerSize); returns this (CarSound::
     * load: bank 2 from the road noise bank g_roadNoiseBank, T469). */
    EngineSound *copyHeader(const GTSOUNDINSTRUMENT *src);      /* 0x001A6538 */
    /* The copy assignment: buf = o.buf only (a doubleword or unaligned-
     * doubleword struct copy), returns *this; the base's words and link
     * stay the target's own (bank 0's link to its own bank 1).  CarSound's
     * compiler-generated operator= (T121, 0x00179230) calls it for each
     * engine (u042; the first pass took it for a copy constructor's stand-in
     * and named it copyBuffer). */
    EngineSound &operator=(const EngineSound &o);               /* 0x001A6570 */
    void setLink(GTSOUNDINSTRUMENT *next);  /* 0x001A7788 link = next */

    GTSOUNDINSTRUMENT *link;                /* 0x020 the bank placed after this one */
    Buffer             buf;                 /* 0x024 the header */
};
EGCS_SIZE(EngineSound, 0x124);

/* ------------------------------------------------------------------------ */
/* CarSound                                                                 */
/* ------------------------------------------------------------------------ */

/* Where a sound is, seen from the listener, 0x14 bytes. */
struct CarSoundPan {
    void reset();                           /* 0x001A6658 all 0 */

    s8  flipL;                              /* 0x00 negate the left gain (the sound is behind) */
    s8  flipR;                              /* 0x01 negate the right gain */
    u8  unk_02[2];                          /* 0x02 */
    f32 x;                                  /* 0x04 x / distance (-1 left .. 1 right) */
    f32 y;                                  /* 0x08 height */
    f32 z;                                  /* 0x0C z / distance */
    f32 dist;                               /* 0x10 horizontal distance */
};
EGCS_SIZE(CarSoundPan, 0x14);

/* An engine voice and its bank, 0x168 bytes (CarSound +0x14 + i x 0x168). */
struct CarSoundEngine {
    CarSoundPan  pan;                       /* 0x000 */
    GtSoundVoice voice;                     /* 0x014 */
    EngineSound  bank;                      /* 0x044 */
};
EGCS_SIZE(CarSoundEngine, 0x168);

/* A program slot, 0x1C bytes (CarSound +0x44C + i x 0x1C). */
struct CarSoundSlot {
    CarSoundPan   pan;                      /* 0x00 */
    GtSoundHandle handle;                   /* 0x14 */
};
EGCS_SIZE(CarSoundSlot, 0x1C);

/* The sound of a car.  Not polymorphic, 0x818 bytes (RaceEntryCar
 * +0x18C4). */
class CarSound {
public:
    /* The members (engines: GtSoundVoice, EngineSound; slots:
     * GtSoundHandle), then the effects (program A) of the slots: 0
     * "turbine2" (0x21), 1 "turbine" (0x20), 2 "in_car" (0x17), 3
     * "body_noiz" (0x0E), and per wheel w (slots 4 + 7 w ..): "off_skill",
     * "on_skill", "off_roadnoiz", "rumble", "off_skill", "on_road",
     * "rain_noiz" (0x1A, 0x1D, 0x19, 0x1F, 0x1A, 0x1C, 0x1E); playing = 0. */
    CarSound();                             /* 0x001A66F0 */
    /* The copy assignment is compiler-generated (memberwise: the engines'
     * GtSoundVoice / EngineSound and the slots' GtSoundHandle operator=),
     * emitted in T121 for RaceEntryCar::copyFrom (0x00179230). */
    /* engines[2..0].bank's destructors (flags 2). */
    ~CarSound();                            /* 0x001A69A8 */
    /* spuBase = the SPU allocation pointer (g_00352B8C), which advances by
     * 0x25800. */
    void reserveSpu();                      /* 0x001A6A30 */
    /* Banks 0 and 1 share the reserved area (0 links 1): "sound/engine/
     * %05d" and "sound/<g_002C2088[kind]>/%05d" of code, loaded
     * asynchronously (file_req_alloc kind 1, sound_load_es_cb); bank 2 gets
     * the header of g_roadNoiseBank (sound/rn_01.es, T469; it plays
     * CarSoundFrame::engine[2], u042's wind). */
    void load(int code, int kind);          /* 0x001A6A60 */
    /* Unless playing: the pans reset, the engine voices started on their
     * banks (engine_setup), field_7CC = 0, playing = 1. */
    void start();                           /* 0x001A6B80 */
    /* The engines, the slots, then the car's own pan and the four one-shot
     * sounds (only the first panned) when loud enough. */
    void update(const CarSoundFrame *f);    /* 0x001A6C38 */
    /* Keys program A of effect id on a free voice (snd_play_voice with no
     * handle; `this' is not used). */
    void playOneShot(int id, f32 vol, f32 atten, f32 l, f32 r); /* 0x001A6ED8 */
    /* When playing: the engine voices and the slots stopped. */
    void stop();                            /* 0x001A70C0 */
    /* Volume, pan, pitch and rpm of engine i, its priority, the voice's
     * update (G002 engine_*). */
    void updateEngine(int i, const CarSoundFrame *f);   /* 0x001A7140 */
    /* Volume and pan of slot i; GtSoundHandle::play. */
    void updateSlot(int i, const CarSoundFrame *f);     /* 0x001A74A0 */

    CarSoundPan    pan;                     /* 0x000 the car itself (u042 CarSoundWords head) */
    CarSoundEngine engines[3];              /* 0x014 */
    CarSoundSlot   slots[32];               /* 0x44C */
    int            field_7CC;               /* 0x7CC 0 by start */
    int            spuBase;                 /* 0x7D0 the SPU area of banks 0 / 1 (u042 field_7D0) */
    char           path0[32];               /* 0x7D4 "sound/engine/%05d" (u042 name_7D4) */
    char           path1[32];               /* 0x7F4 "sound/<kind>/%05d" (u042 name_7F4) */
    int            playing;                 /* 0x814 (u042 field_814) */
};
EGCS_SIZE(CarSound, 0x818);

/* RaceBase::virtual_8: resets the SPU allocators (sound_set_header_buffer
 * (0, 0), sound_set_spu_addr(0xAC000)).  (u059 car_sound_reset_spu.) */
void car_sound_reset_spu(void);             /* 0x001A6628 */
/* The sound effects switch: g_003521E0 = the option for races (replay 0) /
 * replays ? 1 : 0 (GameOptions raceSe / replaySe, +0x10 / +0x58).
 * Callers: RaceBase::virtual_29, RacePS2Base::virtual_8.  (u059
 * car_sound_update_se_switch.) */
void car_sound_update_se_switch(int replay);    /* 0x001A6678 */
/* se_play(se, pan 0x40, prio -1 (the program's), the volume factors)
 * unless g_003521E8 is set.  Users: u051 / u052 (0x14 "count_d" the
 * countdown beep, 0x15 "count_s" the start).  (u059 race_se_play.) */
void race_se_play(int se);                  /* 0x001A7750 */

/* ------------------------------------------------------------------------ */
/* Data                                                                     */
/* ------------------------------------------------------------------------ */

/* The sound effects volume (0 or 1, car_sound_update_se_switch). */
extern f32 g_003521E0;                      /* 0x003521E0 gp-0x5810 */
/* A second sound volume factor (1.0; RaceBase::virtual_8 and RacePS2Base::
 * virtual_8 set 1, u093 the 0..1 setting of its camera +0x9630; read here
 * and by T177).  T176's .sdata, after g_003521E0 (the plan put it and
 * g_003521E8 in T125). */
extern f32 g_003521E4;                      /* 0x003521E4 gp-0x580C */
/* RaceBase::setBackgroundMode mirrors backgroundMode here: race_se_play
 * plays no sound effect while it is set (u059). */
extern int g_003521E8;                      /* 0x003521E8 gp-0x5808 */
/* The engine bank kinds: "normal0".."normal3", "turbo0", ... */
extern const char *g_002C2088[];            /* 0x002C2088 */
/* The effects of the four one-shot sounds: {13, 15, 16, 17} = "b_off",
 * "clash", "clash2", "clash3" (T469; CarSoundFrame::oneShot[i]). */
extern const int g_00330910[4];             /* 0x00330910 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
extern u32 __ti11EngineSound[3];            /* 0x004B2390 __si_type_info (u059 g_004B2390) */
}
#endif

#endif /* GAME_T176_CAR_SOUND_H */
