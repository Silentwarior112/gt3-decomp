/* T468_gtsound.h - sound banks and voices (translation unit T468, unit u104).
 *
 * GTSOUNDINSTRUMENT is a sound bank: a bank file (.ins; .es with es = 1) is
 * read into a temporary buffer, its samples go to SPU RAM at the
 * instrument's SPU address, its header is copied into the header buffer and
 * set up again there (G002 inst_relocate / engn_relocate), the file buffer
 * is freed.  Without setFixed the header buffer and the SPU address come
 * from two global allocators (sound_set_header_buffer, sound_set_spu_addr),
 * which then advance.  GtSoundHandle plays one program on one voice;
 * GtSoundVoice is the crossfaded engine-sound voice of G002's engine_*
 * functions (u123's EngineSound).
 *
 * The part before `#ifdef __cplusplus' is C-compatible for the C units of
 * G002 (u123 dereferences GtSoundVoice; G002_mpeg_frameq.h has its typedef,
 * so it is defined here without one).
 */
#ifndef GAME_T468_GTSOUND_H
#define GAME_T468_GTSOUND_H

#include "game/game.h"

struct EngnSeg;                                 /* u123 (G002): a segment of an "ENGN" bank, 0x10 bytes */

/* A program of a sound bank, 0x18 bytes: the key zone of an INST bank
 * (G002 InstZone, the same layout; T469's se.inf codes index them). */
typedef struct SoundProgram {
    u32 addr;                                   /* 0x00 SPU address */
    u8  unk_04[2];                              /* 0x04 */
    s16 volume;                                 /* 0x06 (1 << 14 = unity) */
    u16 basePitch;                              /* 0x08 InstZone.base_note: 8.8 semitones */
    u16 unk_0A;                                 /* 0x0A -> SndVoiceParam.unk_10 (first pass: adsr1) */
    u8  prio;                                   /* 0x0C the default priority */
    u8  pan;                                    /* 0x0D (InstZone.pan) */
    u8  flags;                                  /* 0x0E -> SndVoiceParam.flags */
    u8  unk_0F;                                 /* 0x0F */
    u8  note;                                   /* 0x10 the note played (InstZone.lo_note) */
    u8  unk_11[3];                              /* 0x11 (InstZone: hi_note, bend_down, bend_up) */
    u32 adsr;                                   /* 0x14 -> SndVoiceParam.adsr (first pass: adsr2) */
} SoundProgram;
EGCS_SIZE(SoundProgram, 0x18);

/* The head of a bank file (.ins: G002's InstBank; .es: EngnBank); the
 * header part is kept in EE RAM. */
typedef struct SoundBankHeader {
    u32           magic;                        /* 0x00 'INST' / 'ENGN' */
    void         *self;                         /* 0x04 the address it was built for */
    int           size;                         /* 0x08 bytes of the header */
    u32           spuBase;                      /* 0x0C the SPU address of the samples (the bank setup) */
    int           spuSize;                      /* 0x10 bytes of sample data */
    void         *samples;                      /* 0x14 the sample data: in the file its offset (= the bytes of the
                                                   header part, file_read_sound_bank), relocated by the setup
                                                   (G002 inst_relocate / engn_relocate: + the load delta) */
    u32           nPrograms;                    /* 0x18 (InstBank.nzones) */
    SoundProgram *programs;                     /* 0x1C (InstBank.zones) */
} SoundBankHeader;

/* One of the two crossfaded SPU voices of a GtSoundVoice, 0x10 bytes (u123). */
typedef struct EngineLayer {
    s8  voice;                                  /* 0x00 the voice handle, -1: none */
    s8  seg;                                    /* 0x01 the segment to play, -1: none */
    s8  prev;                                   /* 0x02 the segment playing */
    u8  unk_03;                                 /* 0x03 */
    s16 volL;                                   /* 0x04 } snd_voice_set_volume takes them */
    s16 volR;                                   /* 0x06 }   as one word, volL | volR << 16 */
    u16 pitch;                                  /* 0x08 */
    u16 unk_0A;                                 /* 0x0A */
    u32 addr;                                   /* 0x0C SPU address / 8 */
} EngineLayer;
EGCS_SIZE(EngineLayer, 0x10);

/* An engine-sound voice, 0x30 bytes (G002 engine_setup / engine_update /
 * engine_stop / engine_set_priority; u059's CarSound engines). */
struct GtSoundVoice {
#ifdef __cplusplus
    GtSoundVoice();                             /* 0x0022C268 both layers' voice = -1 */
    /* engine_stop on the old voice, a byte copy of all 0x30 bytes, then
     * both layers' voice = -1 (the copy plays nothing).  The copy
     * assignment, not a copy constructor: CarSound's compiler-generated
     * operator= (T121, 0x00179230) calls it for each engine (u042). */
    GtSoundVoice &operator=(const GtSoundVoice &o);     /* 0x0022C280 */
#endif

    struct EngnSeg *segs;                       /* 0x00 the segments of the bank's set */
    u8              nsegs;                      /* 0x04 */
    u8              prio;                       /* 0x05 the voices' priority */
    s16             unk_06;                     /* 0x06 (0x4000) */
    u16             rpm;                        /* 0x08 picks / crossfades the segments */
    s16             pitch;                      /* 0x0A 0x1000 = 1.0 */
    s16             volL;                       /* 0x0C */
    s16             volR;                       /* 0x0E */
    EngineLayer     layer[2];                   /* 0x10 (first pass: sub0 = layer[0].voice, sub1 = layer[1].voice) */
};
EGCS_SIZE(struct GtSoundVoice, 0x30);

#ifdef __cplusplus

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and its storage itself (and rejects
 * hand declarations). */
extern "C" {
void *__tf17GTSOUNDINSTRUMENT(void);        /* 0x0022C360 */
/* the base node of EngineSound's __si_type_info (T176) */
extern u32 __ti17GTSOUNDINSTRUMENT[2];      /* 0x00353468 gp-0x4588 __user_type_info */
}
#endif

/* ? name (u042 / u059).  A program and the voice it plays on, 8 bytes. */
struct GtSoundHandle {
    GtSoundHandle();                            /* 0x0022BCE0 voice = -1 */
    /* stop(0) of the old voice, then program = o.program, voice = -1.  The
     * copy assignment, not a copy constructor: CarSound's compiler-generated
     * operator= (T121, 0x00179230) calls it for each slot (u042). */
    GtSoundHandle &operator=(const GtSoundHandle &o);   /* 0x0022BEF8 */
    void set(SoundProgram *p);                  /* 0x0022BCF0 program = p, voice = -1 */
    /* Plays the program at the volumes l / r (times the effect volume) and
     * the pitch factor; updates the voice if it already plays (prio >= 0:
     * its priority too).  Both volumes 0: stop(0). */
    void play(int prio, f32 l, f32 r, f32 pitch);   /* 0x0022BD00 */
    /* fade: the volumes to 0 first; then key off and release. */
    void stop(int fade);                        /* 0x0022BEA0 */

    SoundProgram *program;                      /* 0x0 */
    s8            voice;                        /* 0x4 the voice handle, -1: none */
    u8            unk_05[3];                    /* 0x5 */
};
EGCS_SIZE(GtSoundHandle, 0x8);

/* vtable 0x003415A0 (2 slots), typeinfo __tf17GTSOUNDINSTRUMENT 0x0022C360
 * (__user_type_info 0x00353468).  0x1C bytes of data, egcs vptr +0x1C; 0x20
 * bytes (the bank arrays of T469).  Derived: EngineSound (T176). */
class GTSOUNDINSTRUMENT {
public:
    GTSOUNDINSTRUMENT();                        /* 0x0022BF40 */
    virtual ~GTSOUNDINSTRUMENT();               /* [0] +0x0C 0x0022BF70 */
    /* Called at the end of transfer(), once spuSize is known (empty here;
     * EngineSound places the bank linked to it right after it). */
    virtual void onTransfer();                  /* [1] +0x14 0x0022C0B0 (virtual_1) */

    /* The header buffer and the SPU address are the caller's (fixed = 1). */
    void  setFixed(SoundBankHeader *hdr, int capacity, int spuAddr, int spuRoom); /* 0x0022BFA0 */
    /* Where the samples go: spuAddr when fixed, else the SPU allocator. */
    int   nextSpuAddr();                        /* 0x0022BFC0 */
    /* Sets the loaded file up at spuAddr, copies its header into `header',
     * sets the copy up, frees the file, onTransfer(). */
    void  transfer(SoundBankHeader *file, int es);  /* 0x0022BFE0 */
    /* transfer; unless fixed, from the global allocators, which advance. */
    void  load(SoundBankHeader *file, int es);  /* 0x0022C0B8 */
    void  loadFile(const char *path, int es);   /* 0x0022C140 */
    SoundProgram *program(int i);               /* 0x0022C1A0 &header->programs[i] */
    /* (volume before prio: retail loads $f12 before $a3 at se_play's calls) */
    void  play(int i, int pan, f32 volume, int prio);   /* 0x0022C1B8 se_play_program */
    int   getHeaderSize();                      /* 0x0022C328 */
    SoundBankHeader *F_0022C330();              /* 0x0022C330 header */
    SoundBankHeader *F_0022C338();              /* 0x0022C338 header */
    SoundBankHeader *F_0022C340();              /* 0x0022C340 header */
    void  setSpuRoom(int v);                    /* 0x0022C348 (setUnk18) */
    int   getSpuAddr();                         /* 0x0022C350 */
    void  setSpuAddr(int v);                    /* 0x0022C358 */

    int              fixed;                     /* 0x00 1: header buffer and SPU address set by setFixed */
    SoundBankHeader *header;                    /* 0x04 the header copy */
    int              headerSize;                /* 0x08 */
    int              headerCapacity;            /* 0x0C */
    int              spuAddr;                   /* 0x10 */
    int              spuSize;                   /* 0x14 bytes transferred to SPU RAM */
    int              spuRoom;                   /* 0x18 bytes of SPU RAM it may use (u059; 0 from load) */
    /*  vptr                                       0x1C */
};
EGCS_SIZE(GTSOUNDINSTRUMENT, 0x20);

/* Keys program p on on any free voice (fire and forget); pan 0x40 = centre,
 * prio < 0: the program's.  (volume before prio: GTSOUNDINSTRUMENT::play
 * loads $f12 before $a2.) */
void se_play_program(SoundProgram *p, int pan, f32 volume, int prio);   /* 0x0022BBF8 */
/* Sets the SPU allocation pointer; returns the old one. */
int  sound_set_spu_addr(int addr);                                      /* 0x0022C208 */
void sound_set_header_buffer(void *buf, int size);                      /* 0x0022C218 */
/* Callbacks of the asynchronous file loads: inst->load(data, 0 / 1). */
void sound_load_ins_cb(GTSOUNDINSTRUMENT *inst, SoundBankHeader *data); /* 0x0022C228 */
void sound_load_es_cb(GTSOUNDINSTRUMENT *inst, SoundBankHeader *data);  /* 0x0022C248 */
int  sound_get_spu_addr(void);                                          /* 0x0022C320 */

/* The SPU allocation pointer (sound_set_spu_addr / sound_get_spu_addr; the
 * banks loaded without setFixed take their SPU address from it; 0x5040 at
 * boot).  CarSound (T176) reads it too. */
extern int g_00352B8C;                                                  /* 0x00352B8C gp-0x4E64 */

#endif /* __cplusplus */

#endif /* GAME_T468_GTSOUND_H */
