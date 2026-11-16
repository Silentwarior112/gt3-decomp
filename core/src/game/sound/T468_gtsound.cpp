/* T468 - sound banks (GTSOUNDINSTRUMENT), the SPU / header allocators, and
 * the program voices (GtSoundHandle, GtSoundVoice).  0x0022BBF8..0x0022C3A0.
 *
 * A bank file (.ins, or .es with es = 1) is read into a temporary buffer:
 * its samples go to SPU RAM at the instrument's SPU address, its header is
 * copied into the header buffer (sound_set_header_buffer) and set up again
 * there; the file buffer is freed.  Without setFixed, the header buffer and
 * the SPU address are taken from the two global allocators, which then
 * advance. */
#include "game/sound/gtsound_internal.h"
#include "game/sound/T468_gtsound.h"

/* .sdata, in retail's order (all initialised: written where defined). */
f32 g_seVolume = 1.0f;                          /* 0x00352B88 the effect volume */
int g_00352B8C = 0x5040;                        /* 0x00352B8C the SPU allocation pointer */
SoundBankHeader *g_soundHeaderNext = NULL;      /* 0x00352B90 */
int g_soundHeaderLeft = 0;                      /* 0x00352B94 */

/* The SPU volumes are s16 in -0x4000..0x3FFF. */
static inline int u104_clamp_volume(int vol)
{
    if (vol >= SPU_VOLUME_UNITY)
        return SPU_VOLUME_UNITY - 1;
    if (vol < -SPU_VOLUME_UNITY)
        return -SPU_VOLUME_UNITY;
    return vol;
}

/* Keys program p on on any free voice (fire and forget); volume 0 plays
 * nothing. */
// FUNC 0x0022BBF8 se_play_program
void se_play_program(SoundProgram *p, int pan, f32 volume, int prio)
{
    int vol = (int)(volume * g_seVolume * 16384.0f);    /* scaled, then the voice volume */

    if (volume != 0.0f) {
        SndVoiceParam vp;                   /* sp+0x00 */

        vol = (vol * p->volume) >> 14;
        if (vol >= SPU_VOLUME_UNITY)
            vol = SPU_VOLUME_UNITY - 1;
        else if (vol < -SPU_VOLUME_UNITY)
            vol = -SPU_VOLUME_UNITY;
        snd_pan_volume(vp.vol.lr, vol, pan);    /* 0x00269750 */
        vp.pitch    = snd_note_to_pitch(p->note << 8, p->basePitch);    /* 0x002687B0 */
        vp.unk_10   = p->unk_0A;
        vp.adsr     = SND_HALF_PAIR(p->adsr);   /* as GtSoundHandle::play */
        vp.addr     = p->addr;
        vp.priority = (prio < 0) ? p->prio : (u8)prio;
        vp.flags    = p->flags;
        snd_play_voice(NULL, &vp);          /* 0x002685A8 */
    }
}

// FUNC 0x0022BCE0 GtSoundHandle::GtSoundHandle
GtSoundHandle::GtSoundHandle()
{
    voice = -1;
}

// FUNC 0x0022BCF0 GtSoundHandle::set
void GtSoundHandle::set(SoundProgram *p)
{
    program = p;
    voice = -1;
}

/* Plays the program with the volumes l / r (times the effect volume) and
 * the pitch factor (1.0 = the program's); updates the voice if it already
 * plays.  Both volumes 0: stop(0). */
// FUNC 0x0022BD00 GtSoundHandle::play
void GtSoundHandle::play(int prio, f32 l, f32 r, f32 pitch)
{
    SndVoiceParam vp;                       /* sp+0x00 */
    s16 *lr = vp.vol.lr;
    int volL = (int)(l * g_seVolume * 16384.0f);    /* scaled, then the voice volume */
    int volR = (int)(r * g_seVolume * 16384.0f);
    int factor, notePitch;

    volR = (program->volume * volR) >> 14;
    volL = (program->volume * volL) >> 14;
    if ((volL | volR) == 0) {
        stop(0);
        return;
    }
    lr[0] = u104_clamp_volume(volL);
    lr[1] = u104_clamp_volume(volR);
    factor = (int)(pitch * 4096.0f);        /* 4.12 fixed point */
    notePitch = (snd_note_to_pitch(program->note << 8, program->basePitch) * factor) >> 12;
    if (voice >= 0) {
        if (prio >= 0)
            snd_voice_set_priority(&voice, prio);   /* 0x00267C18 */
        snd_voice_set_pitch(&voice, notePitch);     /* 0x00267BA8 */
        snd_voice_set_volume(&voice, &vp.vol.word);      /* 0x00267C70 */
    } else {
        vp.pitch    = notePitch;
        vp.unk_10   = program->unk_0A;
        /* adsr1 / adsr2 read as one word (a union access: retail keeps the
         * load after the unk_10 store) */
        vp.adsr     = SND_HALF_PAIR(program->adsr);
        vp.addr     = program->addr;
        vp.priority = (prio < 0) ? program->prio : (u8)prio;
        vp.flags    = program->flags;
        snd_play_voice(&voice, &vp);        /* 0x002685A8 */
    }
}

/* fade: the volumes to 0 first; then key off and release. */
// FUNC 0x0022BEA0 GtSoundHandle::stop
void GtSoundHandle::stop(int fade)
{
    if (fade) {
        SndVoiceParam vp;                   /* sp+0x00 (only vol is set) */
        s16 *lr = vp.vol.lr;

        lr[0] = 0;
        lr[1] = 0;
        snd_voice_set_volume(&voice, &vp.vol.word);  /* 0x00267C70 */
    }
    snd_voice_keyoff(&voice);               /* 0x00267B48 */
    snd_voice_release(&voice);              /* 0x00267AF0 */
}

/* The copy assignment: the old voice stopped, the program taken, no voice
 * yet (CarSound's compiler-generated operator=, T121, for each slot). */
// FUNC 0x0022BEF8 GtSoundHandle::operator=
GtSoundHandle &GtSoundHandle::operator=(const GtSoundHandle &o)
{
    stop(0);
    program = o.program;
    voice = -1;
    return *this;
}

// FUNC 0x0022BF40 GTSOUNDINSTRUMENT::GTSOUNDINSTRUMENT
GTSOUNDINSTRUMENT::GTSOUNDINSTRUMENT()
    : fixed(0), header(NULL), headerSize(0), headerCapacity(0),
      spuAddr(0), spuSize(0), spuRoom(0)
{
}

// FUNC 0x0022BF70 GTSOUNDINSTRUMENT::~GTSOUNDINSTRUMENT
GTSOUNDINSTRUMENT::~GTSOUNDINSTRUMENT()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the deleting form: operator delete (0x0027C0C0) for flags & 1 */
}

// FUNC 0x0022BFA0 GTSOUNDINSTRUMENT::setFixed
void GTSOUNDINSTRUMENT::setFixed(SoundBankHeader *hdr, int capacity, int spu, int room)
{
    fixed = 1;
    header = hdr;
    headerCapacity = capacity;
    spuAddr = spu;
    spuRoom = room;
}

/* Where the samples of the bank go. */
// FUNC 0x0022BFC0 GTSOUNDINSTRUMENT::nextSpuAddr
int GTSOUNDINSTRUMENT::nextSpuAddr()
{
    if (fixed)
        return spuAddr;
    return g_00352B8C;
}

/* Sets the loaded file up at spuAddr, copies its header into `header' and
 * sets the copy up too, frees the file, onTransfer(). */
// FUNC 0x0022BFE0 GTSOUNDINSTRUMENT::transfer
void GTSOUNDINSTRUMENT::transfer(SoundBankHeader *file, int es)
{
    int size, samples;

    if (es == 0) {
        inst_relocate((InstBank *)file, spuAddr);   /* 0x00268630 */
        size    = file->size;
        samples = file->spuSize;
        memcpy(header, file, size);
        inst_relocate((InstBank *)header, spuAddr);
    } else {
        engn_relocate((EngnBank *)file, spuAddr);   /* 0x00268340 */
        size    = file->size;
        samples = file->spuSize;
        memcpy(header, file, size);
        engn_relocate((EngnBank *)header, spuAddr);
    }
    headerSize = size;
    spuSize = samples;
    free(file);
    onTransfer();
}

// FUNC 0x0022C0B0 GTSOUNDINSTRUMENT::onTransfer
void GTSOUNDINSTRUMENT::onTransfer()
{
}

// FUNC 0x0022C0B8 GTSOUNDINSTRUMENT::load
void GTSOUNDINSTRUMENT::load(SoundBankHeader *file, int es)
{
    if (fixed) {
        transfer(file, es);
        return;
    }
    spuRoom = 0;
    header = g_soundHeaderNext;
    headerCapacity = g_soundHeaderLeft;
    spuAddr = g_00352B8C;
    transfer(file, es);
    g_soundHeaderNext = (SoundBankHeader *)((u8 *)header + headerSize);
    g_soundHeaderLeft -= headerSize;
    g_00352B8C += spuSize;
}

// FUNC 0x0022C140 GTSOUNDINSTRUMENT::loadFile
void GTSOUNDINSTRUMENT::loadFile(const char *path, int es)
{
    load((SoundBankHeader *)file_read_sound_bank(path, nextSpuAddr(), NULL, 0), es);    /* 0x002418D0 */
}

// FUNC 0x0022C1A0 GTSOUNDINSTRUMENT::program
SoundProgram *GTSOUNDINSTRUMENT::program(int i)
{
    return &header->programs[i];
}

// FUNC 0x0022C1B8 GTSOUNDINSTRUMENT::play
void GTSOUNDINSTRUMENT::play(int i, int pan, f32 volume, int prio)
{
    se_play_program(program(i), pan, volume, prio);
}

/* Sets the SPU allocation pointer; returns the old one. */
// FUNC 0x0022C208 sound_set_spu_addr
int sound_set_spu_addr(int addr)
{
    int old = g_00352B8C;

    g_00352B8C = addr;
    return old;
}

// FUNC 0x0022C218 sound_set_header_buffer
void sound_set_header_buffer(void *buf, int size)
{
    g_soundHeaderNext = (SoundBankHeader *)buf;
    g_soundHeaderLeft = size;
}

// FUNC 0x0022C228 sound_load_ins_cb
void sound_load_ins_cb(GTSOUNDINSTRUMENT *inst, SoundBankHeader *data)
{
    inst->load(data, 0);
}

// FUNC 0x0022C248 sound_load_es_cb
void sound_load_es_cb(GTSOUNDINSTRUMENT *inst, SoundBankHeader *data)
{
    inst->load(data, 1);
}

// FUNC 0x0022C268 GtSoundVoice::GtSoundVoice
GtSoundVoice::GtSoundVoice()
{
    layer[0].voice = -1;
    layer[1].voice = -1;
}

/* The copy assignment: engine_stop on the old voice, then a byte copy of
 * all 0x30 bytes, then no voices (CarSound's compiler-generated operator=,
 * T121, for each engine). */
// FUNC 0x0022C280 GtSoundVoice::operator=
GtSoundVoice &GtSoundVoice::operator=(const GtSoundVoice &o)
{
    struct Raw { u8 b[0x30]; };

    engine_stop(this);                      /* 0x00268278 */
    *(Raw *)this = *(const Raw *)&o;
    layer[0].voice = -1;
    layer[1].voice = -1;
    return *this;
}

// FUNC 0x0022C320 sound_get_spu_addr
int sound_get_spu_addr(void)
{
    return g_00352B8C;
}

// FUNC 0x0022C328 GTSOUNDINSTRUMENT::getHeaderSize
int GTSOUNDINSTRUMENT::getHeaderSize()
{
    return headerSize;
}

// FUNC 0x0022C330 GTSOUNDINSTRUMENT::F_0022C330
SoundBankHeader *GTSOUNDINSTRUMENT::F_0022C330()
{
    return header;
}

// FUNC 0x0022C338 GTSOUNDINSTRUMENT::F_0022C338
SoundBankHeader *GTSOUNDINSTRUMENT::F_0022C338()
{
    return header;
}

// FUNC 0x0022C340 GTSOUNDINSTRUMENT::F_0022C340
SoundBankHeader *GTSOUNDINSTRUMENT::F_0022C340()
{
    return header;
}

// FUNC 0x0022C348 GTSOUNDINSTRUMENT::setSpuRoom
void GTSOUNDINSTRUMENT::setSpuRoom(int v)
{
    spuRoom = v;
}

// FUNC 0x0022C350 GTSOUNDINSTRUMENT::getSpuAddr
int GTSOUNDINSTRUMENT::getSpuAddr()
{
    return spuAddr;
}

// FUNC 0x0022C358 GTSOUNDINSTRUMENT::setSpuAddr
void GTSOUNDINSTRUMENT::setSpuAddr(int v)
{
    spuAddr = v;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (egcs emits it with the vtable). */
// FUNC 0x0022C360 __tf17GTSOUNDINSTRUMENT
extern "C" void *__tf17GTSOUNDINSTRUMENT(void)
{
    if (__ti17GTSOUNDINSTRUMENT[0] == 0)
        __rtti_user(__ti17GTSOUNDINSTRUMENT, "17GTSOUNDINSTRUMENT");
    return __ti17GTSOUNDINSTRUMENT;
}
#endif
