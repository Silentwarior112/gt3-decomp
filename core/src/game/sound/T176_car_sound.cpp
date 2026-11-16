/* T176_car_sound.cpp - CarSound and EngineSound (T176), u059.
 *
 * A car has three streamed engine voices (CarSoundEngine: the voice and the
 * EngineSound bank it plays), 32 slots of looped sound effects (the engine
 * whines, the cabin and body noises, seven per wheel: skids, road noise,
 * rumble, rain) and four one-shot effects ("b_off", "clash".."clash3").
 * Every frame RaceEntryCar::updateSound fills a CarSoundFrame (T121) and
 * calls update: each voice is panned from its position relative to the
 * listener (constant-power pan, 32 / distance attenuation, clamped to 1) and
 * scaled by the two global volume factors g_003521E0 x g_003521E4.
 *
 * Data (retail): .sdata 0x003521E0..0x00352230 (the volume factors, the
 * background flag, the bank kind names), .data 0x002C2088 (the kind table,
 * then the frame info), .rodata 0x003308E8..0x00330A2F (load's two formats,
 * g_00330910, the vtable, then the type names: "11EngineSound" and the
 * GameStatus's garage names, so EngineSound is defined after T429's
 * classes; no "bad_alloc", no RaceCameraBase / UnitArena<64>).
 */
#include "types.h"
#include "libc.h"                           /* memcpy */
#include "cxxrt.h"                          /* type_info, __rtti_si (GCC 14's __tf11EngineSound) */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameOptions (raceSe / replaySe): the garage names */
#include "game/sound/T176_car_sound.h"            /* this translation unit: EngineSound after the garage classes */
#include "game/race/core/T121_race_entry.h"           /* CarSoundFrame, CarSoundVoice(Ex) */
#include "game/sound/T468_gtsound.h"              /* GTSOUNDINSTRUMENT, GtSoundVoice, GtSoundHandle, SoundProgram,
                                               sound_set_spu_addr / header_buffer, sound_load_es_cb, g_00352B8C */
#include "game/sound/T469_sound_effects.h"        /* se_program, se_play, g_roadNoiseBank, SE_* */
#include "engine/media/G002_mpeg_frameq.h"          /* SndVoiceParam, engine_*, snd_play_voice, snd_note_to_pitch,
                                               sinf, cosf */
#include "engine/io/T504_file_queue.h"           /* file_req_alloc, FileDoneFn */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/sound/car_sound_internal.h"                           /* sound_voice_priority, spu_volume_clamp, SPU_VOLUME_* */

/* The SPU RAM CarSound::reserveSpu takes for the two engine banks. */
#define CAR_SOUND_SPU_SIZE      0x25800
/* The SPU allocation pointer after car_sound_reset_spu's reset. */
#define CAR_SOUND_SPU_START     0xAC000
/* file_req_alloc's kind of a sound bank (T504). */
#define FILE_KIND_SOUND_BANK    1
/* A pan closer than this to the listener is moved back along z. */
#define PAN_MIN_DIST            0.25f
#define PAN_REF_DIST            32.0f       /* attenuation = PAN_REF_DIST / distance */
#define QUARTER_PI              0.78539819f
#define HALF_PI                 1.5707964f

/* Updates a pan from a position relative to the listener (inlined in
 * update, updateEngine and updateSlot); returns PAN_REF_DIST / distance.
 * The sqrt.s is a volatile asm (boot_sqrtf_v, types.h), as the original's:
 * retail has no errno call and keeps the code around it in place. */
static inline f32 pan_update(CarSoundPan *pan, const f32 *pos)
{
    f32 x = pos[0];
    f32 y = pos[1];
    f32 z = pos[2];
    f32 dist = boot_sqrtf_v(x * x + z * z);
    if (dist < PAN_MIN_DIST) {
        z = z - PAN_MIN_DIST + dist;
        dist = boot_sqrtf_v(x * x + z * z);
    }
    f32 zn = z / dist;
    f32 xn = x / dist;
    f32 atten = PAN_REF_DIST / dist;
    pan->y = y;
    pan->dist = dist;
    pan->z = zn;
    pan->x = xn;
    int flipL = pan->flipL != 0;
    int flipR = pan->flipR != 0;
    /* which side the sound is on, with hysteresis across the axes */
    if (zn < 0.0f) {
        if (0.0f <= xn)
            flipL = flipR;
        else
            flipR = flipL;
    } else {
        if (0.0f <= xn)
            flipL = flipR ^ 1;
        else
            flipR = flipL ^ 1;
    }
    pan->flipL = flipL;
    pan->flipR = flipR;
    return atten;
}

/* The constant-power gains of a pan (sinf, cosf): gain[0] / gain[1] get the
 * left / right gains (cos / sin of the angle, negated when the sound is
 * behind).  The two outputs go through pointer locals: retail keeps &gain[0]
 * and &gain[1] in saved registers across the two calls (an argument would
 * be folded into sp+4). */
static inline void pan_gains(CarSoundPan *pan, f32 *gain)
{
    f32 *l = &gain[0];
    f32 *r = &gain[1];
    f32 angle = (pan->x + 1.0f) * QUARTER_PI;
    if (angle < 0.0f)
        angle = 0.0f;
    else if (HALF_PI <= angle)
        angle = HALF_PI;
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    if (pan->flipL)
        c = -c;
    *l = c;
    if (pan->flipR)
        s = -s;
    *r = s;
}

/* max.s: a volatile asm statement, as every max.s / min.s of the image
 * behaves (u018, u032, u044).  GCC 14: the C ternary (the same but for -0.0
 * against 0.0). */
static inline f32 car_sound_maxf(f32 a, f32 b)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 r;
    __asm__ volatile("max.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
#else
    return a > b ? a : b;
#endif
}

/* The header of a bank as the engine bank engine_setup plays (an inline
 * accessor: retail keeps &bank in its own register and loads header from
 * it, which a direct field access folds into one address). */
static inline EngnBank *engine_bank_header(GTSOUNDINSTRUMENT *b)
{
    return (EngnBank *)b->header;
}

/* ------------------------------------------------------------------------ */
/* Data                                                                     */
/* ------------------------------------------------------------------------ */

/* .sdata (-G8): written in definition order, the kind names (8 bytes each,
 * small) just before the table that points at them. */
f32 g_003521E0 = 1.0f;                      /* 0x003521E0 */
f32 g_003521E4 = 1.0f;                      /* 0x003521E4 */
int g_003521E8 = 0;                         /* 0x003521E8 (explicit: .sdata, not the end of the file) */
const char *g_002C2088[8] = {               /* 0x002C2088 */
    "normal0", "normal1", "normal2", "normal3",
    "turbo0", "turbo1", "turbo2", "turbo3",
};

/* ------------------------------------------------------------------------ */
/* EngineSound                                                              */
/* ------------------------------------------------------------------------ */

// FUNC 0x001A6488 EngineSound::EngineSound
//   GTSOUNDINSTRUMENT(); the vptr; the header buffer of its own (fixed = 1,
//   header = &buf, headerCapacity 0x100); link = NULL.
EngineSound::EngineSound()
{
    /* implicit: GTSOUNDINSTRUMENT (base constructor) */
    fixed = 1;
    header = (SoundBankHeader *)&buf;
    headerCapacity = sizeof(buf);
    link = 0;
}

// FUNC 0x001A64D8 EngineSound::~EngineSound
//   The vptr, then ~GTSOUNDINSTRUMENT with the flags (it deletes).
EngineSound::~EngineSound()
{
    /* implicit: ~GTSOUNDINSTRUMENT (base destructor) */
}

// FUNC 0x001A6500 EngineSound::onTransfer
//   At the end of GTSOUNDINSTRUMENT::transfer: the linked bank goes right
//   after this one in SPU RAM and has that much less room.
void EngineSound::onTransfer()
{
    GTSOUNDINSTRUMENT *next = link;
    if (next != 0) {
        next->spuAddr = spuAddr + spuSize;
        next->spuRoom = next->spuRoom - spuSize;
    }
}

// FUNC 0x001A6538 EngineSound::copyHeader
//   memcpy(buf, src->header, src->headerSize); returns this.
EngineSound *EngineSound::copyHeader(const GTSOUNDINSTRUMENT *src)
{
    memcpy(&buf, src->header, src->headerSize);
    return this;
}

// FUNC 0x001A6570 EngineSound::operator=
//   The copy assignment: only the 0x100-byte buffer (a struct copy: a
//   doubleword loop, or an unaligned doubleword one); the base's words and
//   link stay the target's.  Caller: CarSound's compiler-generated
//   operator= (T121, 0x00179230), for each engine.
EngineSound &EngineSound::operator=(const EngineSound &o)
{
    buf = o.buf;
    return *this;
}

/* ------------------------------------------------------------------------ */
/* CarSound                                                                 */
/* ------------------------------------------------------------------------ */

// FUNC 0x001A6628 car_sound_reset_spu
//   RaceBase::virtual_8: resets the sound allocators (no header buffer, the
//   SPU allocation pointer at 0xAC000).
void car_sound_reset_spu(void)
{
    sound_set_header_buffer(0, 0);
    sound_set_spu_addr(CAR_SOUND_SPU_START);
}

// FUNC 0x001A6658 CarSoundPan::reset
void CarSoundPan::reset()
{
    flipL = 0;
    flipR = 0;
    x = y = z = dist = 0.0f;
}

// FUNC 0x001A6678 car_sound_update_se_switch
//   The sound effects switch: g_003521E0 = 1 when the option for races
//   (replay == 0) / replays is on, else 0 (RaceBase::virtual_29,
//   RacePS2Base::virtual_8).  One conditional expression: retail jumps from
//   each option's test straight to the 1.0 / 0.0 arms.
void car_sound_update_se_switch(int replay)
{
    g_003521E0 = (replay ? getGameStatus()->getGameOptions()->replaySe
                         : getGameStatus()->getGameOptions()->raceSe) ? 1.0f : 0.0f;
}

// FUNC 0x001A66F0 CarSound::CarSound
//   Members: engines[3] (GtSoundVoice(), EngineSound()), slots[32]
//   (GtSoundHandle()).  Then the effects of the slots (program A): 0..3 the
//   car's, then the four wheels' groups of seven (slots 4..31).  The EH
//   landing pads destroy the engines built so far (~EngineSound, flags 2).
CarSound::CarSound()
{
    /* implicit: GtSoundVoice, EngineSound, GtSoundHandle (member constructors), ~EngineSound (exception cleanup) */
    slots[1].handle.set(se_program(SE_TURBINE, 0));
    slots[0].handle.set(se_program(SE_TURBINE2, 0));
    slots[2].handle.set(se_program(SE_IN_CAR, 0));
    slots[3].handle.set(se_program(SE_BODY_NOIZ, 0));
    int first = 4;
    for (int wheel = 3; wheel >= 0; wheel--) {
        slots[first + 0].handle.set(se_program(SE_OFF_SKILL, 0));
        slots[first + 1].handle.set(se_program(SE_ON_SKILL, 0));
        slots[first + 2].handle.set(se_program(SE_OFF_ROADNOIZ, 0));
        slots[first + 3].handle.set(se_program(SE_RUMBLE, 0));
        slots[first + 4].handle.set(se_program(SE_OFF_SKILL, 0));
        slots[first + 5].handle.set(se_program(SE_ON_ROAD, 0));
        slots[first + 6].handle.set(se_program(SE_RAIN_NOIZ, 0));
        first += 7;
    }
    playing = 0;
}

// FUNC 0x001A69A8 CarSound::~CarSound
//   Destroys engines[2..0].bank (~EngineSound, flags 2); egcs's
//   operator delete when asked.
CarSound::~CarSound()
{
    /* implicit: ~EngineSound (member destructors), operator delete (deleting destructor) */
}

// FUNC 0x001A6A30 CarSound::reserveSpu
//   spuBase = the SPU allocation pointer; reserves 0x25800 bytes after it.
void CarSound::reserveSpu()
{
    int addr = g_00352B8C;
    spuBase = addr;
    sound_set_spu_addr(addr + CAR_SOUND_SPU_SIZE);
}

// FUNC 0x001A6A60 CarSound::load
//   The two engine banks share the reserved SPU area (bank 0 links bank 1
//   after itself): "sound/engine/%05d" and "sound/<kind>/%05d" are loaded
//   asynchronously; bank 2 gets the header of the road noise bank.
void CarSound::load(int code, int kind)
{
    EngineSound *bank0 = &engines[0].bank;
    EngineSound *bank1 = &engines[1].bank;
    bank0->link = bank1;
    int base = spuBase;
    bank0->spuRoom = CAR_SOUND_SPU_SIZE;
    bank0->spuAddr = base;
    bank1->spuAddr = base;
    bank1->spuRoom = CAR_SOUND_SPU_SIZE;
    sprintf(path0, "sound/engine/%05d", code);
    file_req_alloc(path0, 0, 0, (FileDoneFn)sound_load_es_cb, bank0, FILE_KIND_SOUND_BANK);
    sprintf(path1, "sound/%s/%05d", g_002C2088[kind], code);
    file_req_alloc(path1, 0, 0, (FileDoneFn)sound_load_es_cb, bank1, FILE_KIND_SOUND_BANK);
    engines[2].bank.copyHeader(&g_roadNoiseBank[0]);
}

/* The effects of the four one-shot sounds (CarSoundFrame::oneShot[i]):
 * "b_off", "clash", "clash2", "clash3".  .rodata after load's formats and
 * before the vtable: an extern const (header) is written where it is
 * defined. */
const int g_00330910[4] = { SE_B_OFF, SE_CLASH, SE_CLASH2, SE_CLASH3 };   /* 0x00330910 */

// FUNC 0x001A6B80 CarSound::start
//   Unless playing: the pans reset, the engine voices started on their
//   banks (engine_setup, set 0, priority 0), field_7CC = 0.  One counter for
//   both loops (retail's s1; the first loop is reversed into a count-down).
void CarSound::start()
{
    int i;
    if (playing == 0) {
        playing = 1;
        for (i = 0; i < 3; i++) {
            engines[i].pan.reset();
            engine_setup(&engines[i].voice, engine_bank_header(&engines[i].bank), 0, 0);
        }
        for (i = 0; i < 32; i++)
            slots[i].pan.reset();
        field_7CC = 0;
    }
}

// FUNC 0x001A6C38 CarSound::update
//   The engines, the slots, then the car's own pan (pan_update inlined:
//   sinf, cosf) and its four one-shot effects (g_00330910: "b_off",
//   "clash".."clash3"; only the first one is panned, the others play at the
//   louder gain on both sides) when loud enough.  The clashes are a switch
//   (cases 1..3): retail tests i < 4 and i >= 1 (as pointer compares), the
//   range test of a switch; `if (i >= 1)' is one test.
void CarSound::update(const CarSoundFrame *f)
{
    for (int i = 0; i < 3; i++)
        updateEngine(i, f);
    for (int i = 0; i < 32; i++)
        updateSlot(i, f);

    f32 gain[2];
    f32 atten = pan_update(&pan, f->pos);
    pan_gains(&pan, gain);  /* implicit: sinf, cosf (inline pan_gains) */

    for (int i = 0; i < 4; i++) {
        f32 volume = f->oneShot[i];
        int loud = (SOUND_MIN_VOLUME <= volume) ? 1 : 0;
        f32 l = gain[0];
        f32 r = gain[1];
        switch (i) {                        /* the clashes: centred, at the louder gain */
        case 1:
        case 2:
        case 3:
            l = boot_fabsf(gain[0]);
            r = boot_fabsf(gain[1]);
            r = car_sound_maxf(l, r);
            l = r;
            break;
        }
        if (loud)
            playOneShot(g_00330910[i], volume, atten, l, r);
    }
}

// FUNC 0x001A6ED8 CarSound::playOneShot
//   Keys program A of effect `id' on a free voice, with no handle
//   (snd_play_voice(NULL, ...); `this' is unused): the volumes times the
//   program's (1 << 14 = unity), its pitch, ADSR, address and flags.
void CarSound::playOneShot(int id, f32 vol, f32 atten, f32 l, f32 r)
{
    f32 v = vol * atten * (g_003521E0 * g_003521E4);
    if (v == 0.0f)
        return;
    SoundProgram *prog = se_program(id, 0);
    int prio = sound_voice_priority(v);
    int priority;
    if (prio >= 0x100)
        priority = 0xFF;
    else
        priority = prio;
    /* The parameter block with its word view: a union, whose members egcs
     * gives alias set 0, so every program load stays after the previous
     * store, as in retail (a plain SndVoiceParam lets the loads move up). */
    union {
        SndVoiceParam p;
        u32           w[5];
    } u;
    s16 *volume = u.p.vol.lr;
    volume[0] = spu_volume_clamp((spu_volume_clamp((int)(v * l * (f32)SPU_VOLUME_UNITY)) * prog->volume) >> 14);
    volume[1] = spu_volume_clamp((spu_volume_clamp((int)(v * r * (f32)SPU_VOLUME_UNITY)) * prog->volume) >> 14);
    u.p.pitch = snd_note_to_pitch(prog->note << 8, prog->basePitch);
    u.p.unk_10 = prog->unk_0A;
    u.p.adsr = prog->adsr;
    u.p.addr = prog->addr;
    u.p.priority = priority;
    u.p.flags = prog->flags;
    snd_play_voice(0, &u.p);
}

// FUNC 0x001A70C0 CarSound::stop
//   When playing: the engine voices stopped (engine_stop), the slots too
//   (GtSoundHandle::stop with the fade).
void CarSound::stop()
{
    if (playing != 0) {
        playing = 0;
        for (int i = 0; i < 3; i++)
            engine_stop(&engines[i].voice);
        for (int i = 0; i < 32; i++)
            slots[i].handle.stop(1);
    }
}

// FUNC 0x001A7140 CarSound::updateEngine
//   Volume, pan (pan_update inlined: sinf, cosf), pitch (pitch x 4096,
//   0..0x3FFF) and rpm (param) of engine voice i, then its priority
//   (engine_set_priority) and the voice's update (engine_update).
void CarSound::updateEngine(int i, const CarSoundFrame *f)
{
    const CarSoundVoiceEx *src = &f->engine[i];
    CarSoundEngine *eng = &engines[i];
    GtSoundVoice *voice = &eng->voice;
    f32 gain[2];
    f32 atten = pan_update(&eng->pan, src->pos);
    pan_gains(&eng->pan, gain);  /* implicit: sinf, cosf (inline pan_gains) */

    f32 a = atten * f->scale;
    f32 k;
    if (a < -1.0f)
        k = -1.0f;
    else if (1.0f < a)
        k = 1.0f;
    else
        k = a;
    f32 master = g_003521E0 * g_003521E4;
    f32 volume = k * src->volume * master;
    int prio = sound_voice_priority(volume);
    int priority;
    if (prio >= 0x100)
        priority = 0xFF;
    else
        priority = prio;
    /* The volumes go through the halfword-pair view (SndWordView, G002): a
     * union access, which egcs gives alias set 0 and keeps in place, as
     * retail stores each volume right after its clamp (plain s16 stores
     * sink below the next computation).  The original's GtSoundVoice most
     * likely held them as a SndVolPair; the canonical header keeps two s16
     * for u123's engine code. */
    ((SndWordView *)&voice->volL)->lr[0] = spu_volume_clamp((int)(gain[0] * volume * (f32)SPU_VOLUME_UNITY));
    ((SndWordView *)&voice->volL)->lr[1] = spu_volume_clamp((int)(gain[1] * volume * (f32)SPU_VOLUME_UNITY));
    int t = (int)(src->pitch * 4096.0f);
    int pitch;
    if (t < 0)
        pitch = 0;
    else if (t >= 0x4000)
        pitch = 0x3FFF;
    else
        pitch = t;
    voice->pitch = pitch;
    voice->rpm = (int)src->param;
    engine_set_priority(voice, priority & 0xFF);
    engine_update(voice);
}

// FUNC 0x001A74A0 CarSound::updateSlot
//   Volume and pan of slot i (pan_update inlined: sinf, cosf);
//   GtSoundHandle::play plays / updates its effect.
void CarSound::updateSlot(int i, const CarSoundFrame *f)
{
    const CarSoundVoice *src = &f->voice[i];
    CarSoundSlot *slot = &slots[i];
    GtSoundHandle *h = &slot->handle;
    f32 gain[2];
    f32 atten = pan_update(&slot->pan, src->pos);
    pan_gains(&slot->pan, gain);  /* implicit: sinf, cosf (inline pan_gains) */

    f32 a = atten * f->scale;
    f32 k;
    if (a < -1.0f)
        k = -1.0f;
    else if (1.0f < a)
        k = 1.0f;
    else
        k = a;
    f32 master = g_003521E0 * g_003521E4;
    f32 volume = k * src->volume * master;
    int prio = sound_voice_priority(volume);
    gain[0] = gain[0] * volume;
    gain[1] = gain[1] * volume;
    h->play(prio < 0x100 ? prio : 0xFF, gain[0], gain[1], src->pitch);
}

// FUNC 0x001A7750 race_se_play
//   se_play(se, centre, the program's priority, the volume factors) unless
//   g_003521E8 (the background mode) is set.
void race_se_play(int se)
{
    if (g_003521E8 == 0)
        se_play(se, 0x40, -1, g_003521E0 * g_003521E4);
}

// FUNC 0x001A7788 EngineSound::setLink
void EngineSound::setLink(GTSOUNDINSTRUMENT *next)
{
    link = next;
}

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001A7790 __tf11EngineSound
//   Builds the __si_type_info __ti11EngineSound (0x004B2390) on the first
//   call, base __ti17GTSOUNDINSTRUMENT (gp-0x4588).  (egcs generates it: its
//   "11EngineSound" comes right after the vtable, as retail's, since the
//   class is defined after the GameStatus's garage classes.)
extern "C" const type_info &__tf11EngineSound(void)
{
    if (__ti11EngineSound[0] == 0) {
        __tf17GTSOUNDINSTRUMENT();
        __rtti_si(__ti11EngineSound, "11EngineSound", (const type_info *)__ti17GTSOUNDINSTRUMENT);
    }
    return *(const type_info *)__ti11EngineSound;
}
#endif
