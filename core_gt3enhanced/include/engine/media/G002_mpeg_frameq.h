/* G002_mpeg_frameq.h - translation unit G002 (plain C, no exceptions; units
 * u121, u122, u123, u124, 0x0025F390..0x0026A7C8).
 *
 * The no-FDE stretch after the C++ code: Polyphony's IPU MPEG-2 video player
 * (input ring, decoder thread, frame queue, PES parser, syntax decoder, MMI
 * motion compensation), the IPU command layer and the still-image decoder,
 * the USB device manager and its PJOY wheel client, blocking libmc wrappers,
 * libscf clock conversions, the SPU2 voice manager and sound-effect sequencer,
 * thread / semaphore helpers, locked "host:" fileio, the MMI strlen and the
 * hand-scheduled single-precision math library.
 *
 * Declared here: the structures more than one unit uses (sound voices and
 * sequences, the IPU DMA / BDEC parameter blocks, the decoder callbacks), the
 * functions other units call and the globals two units share.  The types that
 * other units only pass by pointer stay forward declarations; their
 * definitions are in the owning unit. */
#ifndef GAME_G002_MPEG_FRAMEQ_H
#define GAME_G002_MPEG_FRAMEQ_H

#include "game/game.h"
#include <stddef.h>         /* size_t (strlen) */
#include "libmc.h"          /* sceMcTblGetDir (mc_getdir_sync) */
#include "libcdvd.h"        /* sceCdCLOCK (scf_clock_to_*) */

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* Pointers only: forward declarations                                      */
/* ======================================================================== */

typedef struct MpegPlayer MpegPlayer;           /* u121, 0x24 bytes at scratchpad 0x70002344 */
typedef struct MpegViBuf MpegViBuf;             /* u121, video input ring, scratchpad 0x70002300 */
typedef struct MpegFrameQueue MpegFrameQueue;   /* u121, output frame queue (malloc 0x28) */
typedef struct MpegMv MpegMv;                   /* u121, motion-vector state of a slice */
typedef struct MpegMB MpegMB;                   /* one 0x180-byte macroblock (Y, Cb, Cr), defined below */
/* Input state of a PJOY (USB wheel) device (fno 3): T479's
 * USBSteeringPort reads it. */
typedef struct PjoyState {
    int  x;                                     /* 0x00 the wheel (10 bits) */
    int  y;                                     /* 0x04 */
    int  z;                                     /* 0x08 */
    int  rx;                                    /* 0x0C */
    int  ry;                                    /* 0x10 */
    int  rz;                                    /* 0x14 */
    int  slider[2];                             /* 0x18 the pedals (8 bits) */
    int  pov[1];                                /* 0x20 hat 0..7 */
    u8   buttons[16];                           /* 0x24 */
    int  unk_34;                                /* 0x34 (USBSteeringPort::report) */
} PjoyState;
EGCS_SIZE(PjoyState, 0x38);
/* Device info (fno 2): T479 reads the USB vendor / product ids. */
typedef struct PjoyInfo {
    u16  vendor;                                /* 0x00 (0x46D Logitech) */
    u16  product;                               /* 0x02 */
    u8   unk_04[0x6C];                          /* 0x04 */
} PjoyInfo;
EGCS_SIZE(PjoyInfo, 0x70);
/* One force-feedback command (7 bytes used). */
typedef struct FfCmd {
    u8   b[8];                                  /* 0x00 */
} FfCmd;
EGCS_SIZE(FfCmd, 0x8);
/* What io_getstat fills (the bootstrap's HostFileStat): T502 / T505 keep one
 * on the stack (16 bytes reserved). */
typedef struct IoStat {
    int mode;                                   /* 0x0 SCE_STM_R | SCE_STM_F (0x11), always */
    int size;                                   /* 0x4 the file size (lseek END) */
} IoStat;
EGCS_SIZE(IoStat, 0x8);
/* ? name.  A thread and the semaphore it signals when it exits, 8 bytes
 * (worker_thread_start / worker_thread_join / worker_thread_exit, u124).
 * Held by value: T463's music thread, T055's recording thread, u110's
 * McManager (+0x1400), u111's vol thread state. */
typedef struct ThreadCtx {
    int thread;                                 /* 0x0 the thread id */
    int sema;                                   /* 0x4 signalled when the thread exits */
} ThreadCtx;
EGCS_SIZE(ThreadCtx, 0x8);
typedef struct GtSoundVoice GtSoundVoice;       /* T468_gtsound.h: a crossfaded engine-sound voice */

/* ======================================================================== */
/* IPU / MPEG decoder                                                       */
/* ======================================================================== */

/* toIPU / fromIPU DMA and IPU state saved by ipu_stop_dma (the layout of
 * libipu's sceIpuDmaEnv); held at the start of MpegViBuf. */
typedef struct IpuDmaEnv {
    u32 d4madr;             /* 0x00 */
    u32 d4tadr;             /* 0x04 */
    u32 d4qwc;              /* 0x08 */
    u32 d4chcr;             /* 0x0C */
    u32 d3madr;             /* 0x10 */
    u32 d3qwc;              /* 0x14 */
    u32 d3chcr;             /* 0x18 */
    u32 ipubp;              /* 0x1C IPU_BP: BP 0..6, IFC 8..11, FP 16..17 */
    u32 ipuctrl;            /* 0x20 */
} IpuDmaEnv;
EGCS_SIZE(IpuDmaEnv, 0x24);

/* An IPU still image as ipu_decode_image reads it (unaligned, so packed;
 * T250's gt2k_menu::IpicHeader is the same record): Sony's IPU stream
 * format, 16 bytes of header then one byte of picture flags and the
 * bitstream.  The toIPU DMA starts at the header; the decoder skips the
 * first 17 bytes with FDEC. */
typedef struct __attribute__((packed)) IpuImageHeader {
    u8  magic[4];           /* 0x00 ? "ipum" in Sony's format; not read */
    u32 size;               /* 0x04 bitstream bytes */
    s16 width;              /* 0x08 */
    s16 height;             /* 0x0A */
    u32 nframes;            /* 0x0C ? not read */
    u8  flags;              /* 0x10 bits 0-1 IDP, 2 DTD, 3-7 IPU_CTRL bits 19-23 (AS, IVF, QST, MP1) */
} IpuImageHeader;
EGCS_SIZE(IpuImageHeader, 0x11);

/* ipu_bdec's argument: the decode state of one macroblock (0x002FF098). */
typedef struct IpuBdecParam {
    s16 *out;               /* 0x00 IDCT output s16[384] (scratchpad 0x70000780); D3_MADR = (out & 0x0FFFFFFF) | 0x80000000 */
    u32  mb_type;           /* 0x04 bit 0 macroblock_intra, bit 1 macroblock_pattern */
    s32  pict_type;         /* 0x08 picture_coding_type -> IPU_CTRL.PCT */
    s32  dct_type;          /* 0x0C -> BDEC.DT */
    s32  qscale;            /* 0x10 quantiser_scale_code -> BDEC.QSC */
    s32  alt_scan;          /* 0x14 alternate_scan -> IPU_CTRL.AS */
    s32  dc_prec;           /* 0x18 intra_dc_precision -> IPU_CTRL.IDP */
    s32  intra_vlc;         /* 0x1C intra_vlc_format -> IPU_CTRL.IVF */
    s32  q_scale_type;      /* 0x20 -> IPU_CTRL.QST */
    s32  dc_reset;          /* 0x24 reset the DC predictors first -> BDEC.DCR */
} IpuBdecParam;
EGCS_SIZE(IpuBdecParam, 0x28);

/* One macroblock of a frame store, as the IPU's CSC reads it ("raw8"): the
 * frame stores are column-major arrays of them (mpeg_player.c's
 * mpeg_get_mb / mpeg_put_mb / mpeg_fetch_ref_mbs, ipu.c's
 * ipu_csc_rgb32).  Copied as quadwords (lq / sq), so 16-byte aligned. */
struct MpegMB {
    u8 y[16][16];           /* 0x000 luma, 16 rows of 16 */
    u8 cb[8][8];            /* 0x100 */
    u8 cr[8][8];            /* 0x140 */
} __attribute__((aligned(16)));
EGCS_SIZE(struct MpegMB, 0x180);

/* A decoder callback, called as func(arg, data): arg is the pointer given to
 * mpeg_set_callback, data the event's argument.  Slots, as mpeg_player_create
 * installs them: 0 IPU input exhausted (mpeg_vibuf_add_dma), 1 stop the IPU
 * DMA (mpeg_vibuf_stop_dma), 2 restart it (mpeg_vibuf_restart_dma), 3 picture
 * ready, data = frame store (mpeg_player_on_picture), 4 sequence size, data =
 * int[2] (mpeg_player_on_size). */
typedef int (*MpegCallback)(void *arg, void *data);

/* One slot of the callback table (g_002FF140[5]). */
typedef struct MpegCallbackSlot {
    MpegCallback func;      /* 0x00 */
    void        *arg;       /* 0x04 */
} MpegCallbackSlot;
EGCS_SIZE(MpegCallbackSlot, 0x8);

/* ======================================================================== */
/* Sound: SPU2 voices                                                       */
/* ======================================================================== */

/* An SPU2 voice slot: 46 of them in SndVoiceBank (0x002FF170).  A voice
 * belongs to a handle byte of its owner (s8 *handle below): snd_play_voice
 * stores the voice index there, and the voice manager writes -1 into it when
 * the voice is stolen or ends. */
typedef struct SndVoice {
    u8   state;             /* 0x00 0 free, 1 allocated (bound to a handle) */
    u8   playing;           /* 0x01 keyed on and not yet ended */
    u8   keyedOn;           /* 0x02 key-on sent, key-off still due */
    u8   priority;          /* 0x03 */
    u8   unk_04;            /* 0x04 */
    u8   dirty;             /* 0x05 bit 0 pitch, bit 1 volume, bit 2 ADSR */
    u8   keyOnReq;          /* 0x06 */
    u8   keyOffReq;         /* 0x07 */
    u16  volL;              /* 0x08 written as one word with volR (SndVoiceParam.vol) */
    u16  volR;              /* 0x0A */
    u16  pitch;             /* 0x0C 44.1 kHz based (sent as pitch * 0xEB3 / 4096) */
    u8   unk_0E[2];         /* 0x0E */
    u32  addr;              /* 0x10 sample address / 8 */
    u16  adsr1;             /* 0x14 written as one word with adsr2 (SndVoiceParam.adsr) */
    u16  adsr2;             /* 0x16 */
    s8  *owner;             /* 0x18 the owner's handle byte */
    u32  age;               /* 0x1C flushes since key-on (voice stealing: oldest first) */
    s32  unk_20;            /* 0x20 SndVoiceBank.unk_06 * SndVoiceParam.unk_10; non-zero -> command flag 0x100 */
    s32  unk_24;            /* 0x24 cleared by snd_voice_start */
    u8   unk_28;            /* 0x28 SndVoiceParam.flags bit 0; non-zero -> command flags 0xA0 */
    u8   unk_29[3];         /* 0x29 */
} SndVoice;
EGCS_SIZE(SndVoice, 0x2C);

/* The voice bank (0x002FF168, cleared by snd_init). */
typedef struct SndVoiceBank {
    u8   nvoices;           /* 0x00 45 after snd_init (? meaning: not read in G002) */
    u8   unk_01;            /* 0x01 snd_shutdown switches the reverb off (mode 0) if set */
    u8   updated;           /* 0x02 set by snd_voice_start (never read in the image) */
    u8   unk_03;            /* 0x03 */
    u16  rate;              /* 0x04 frames per second (snd_set_rate) */
    u16  unk_06;            /* 0x06 rate * 256 / 60 (snd_set_rate); scales SndVoiceParam.unk_10 */
    SndVoice voices[46];    /* 0x08 */
} SndVoiceBank;
EGCS_SIZE(SndVoiceBank, 0x7F0);

/* A volume pair: snd_pan_volume writes the s16 halves (left, right),
 * snd_voice_set_volume and snd_voice_start take the word volL | volR << 16. */
typedef union SndVolPair {
    s16  lr[2];             /* 0x0 [0] left, [1] right */
    u32  word;              /* 0x0 */
} SndVolPair;
EGCS_SIZE(SndVolPair, 0x4);

/* Two adjacent halfwords read or written as one word (lw / sw): SndVoice
 * volL / volR and adsr1 / adsr2, T468 EngineLayer volL / volR.  The word is
 * a union member (SndVolPair.word): egcs gives a union access alias set 0,
 * so it stays in order with every other access, as retail snd_voice_start
 * (u124) shows; u123's users compile the same either way.  For GCC >= 4
 * the union type may alias the halfwords. */
#if defined(__GNUC__) && __GNUC__ >= 4
typedef union SndVolPair __attribute__((__may_alias__)) SndWordView;
#else
typedef union SndVolPair SndWordView;
#endif
#define SND_HALF_PAIR(lo)   (((SndWordView *)&(lo))->word)

/* What a voice is started with (snd_play_voice, snd_voice_start). */
typedef struct SndVoiceParam {
    SndVolPair vol;         /* 0x00 */
    u16  pitch;             /* 0x04 */
    u8   unk_06[2];         /* 0x06 */
    u32  addr;              /* 0x08 SPU address / 8 */
    u32  adsr;              /* 0x0C adsr1 | adsr2 << 16 */
    u16  unk_10;            /* 0x10 InstZone.unk_0A for notes; times SndVoiceBank.unk_06 -> SndVoice.unk_20 */
    u8   priority;          /* 0x12 */
    u8   flags;             /* 0x13 bit 0 -> SndVoice.unk_28 */
} SndVoiceParam;
EGCS_SIZE(SndVoiceParam, 0x14);

/* ======================================================================== */
/* Sound: sequencer                                                         */
/* ======================================================================== */

/* Linear frame ramp, Bresenham style (snd_ramp_init / snd_ramp_step, u124). */
typedef struct SndRamp {
    u16  count;             /* 0x00 frames left (0 = idle) */
    s16  step;              /* 0x02 per-frame quotient (signed) */
    s16  extra;             /* 0x04 correction step: 0, +1 or -1 */
    u16  rem;               /* 0x06 |delta| % frames */
    u16  denom;             /* 0x08 frames at the start */
    u16  err;               /* 0x0A error accumulator (starts at frames / 2) */
} SndRamp;
EGCS_SIZE(SndRamp, 0x0C);

/* A key zone of an INST bank (InstBank.zones[]). */
typedef struct InstZone {
    u32  addr;              /* 0x00 SPU address / 8 */
    u8   unk_04[2];         /* 0x04 */
    s16  vol;               /* 0x06 */
    u16  base_note;         /* 0x08 8.8 semitones, the note that plays at the sample's pitch */
    u16  unk_0A;            /* 0x0A -> SndVoiceParam.unk_10 */
    u8   prio;              /* 0x0C */
    u8   pan;               /* 0x0D */
    u8   flags;             /* 0x0E -> SndVoiceParam.flags */
    u8   unk_0F;            /* 0x0F */
    u8   lo_note;           /* 0x10 key range */
    u8   hi_note;           /* 0x11 */
    u8   bend_down;         /* 0x12 bend range used when the bend is <= 0 */
    u8   bend_up;           /* 0x13 bend range used when the bend is > 0 */
    u32  adsr;              /* 0x14 */
} InstZone;
EGCS_SIZE(InstZone, 0x18);

/* A program of an INST bank (InstBank.programs[]): variable length. */
typedef struct InstProgram {
    u8   nzones;            /* 0x00 */
    u8   pan;               /* 0x01 */
    s16  vol;               /* 0x02 */
    u8   zone[1];           /* 0x04 nzones indices into InstBank.zones */
} InstProgram;

/* One track of a sequence: a byte-code stream (seq_run_tracks, u124). */
typedef struct SeqTrack {
    u8  *pos;               /* 0x00 next event, NULL = track ended */
    s32  wait;              /* 0x04 ticks until the next event (MIDI-style varint delta) */
    u16  volume;            /* 0x08 g_00345F10[cmd 4 operand] */
    u16  volScaled;         /* 0x0A (seq->volEff * volume) >> 14 */
    s16  bend;              /* 0x0C pitch bend, 14-bit value - 0x2000 */
    u8   unk_0E[2];         /* 0x0E */
    u8   pan;               /* 0x10 cmd 5 */
    u8   program;           /* 0x11 cmd 3: index into InstBank.programs */
    s8   loopDepth;         /* 0x12 loop stack top, -1 = empty (seq_start) */
    u8   unk_13;            /* 0x13 */
    u8  *loopPos[4];        /* 0x14 loop start positions */
    u8   loopCount[4];      /* 0x24 loop counters, 0xFF = forever */
} SeqTrack;
EGCS_SIZE(SeqTrack, 0x28);

/* "ENGN" engine sound bank (.es): rpm segments in two sets; the head is
 * T468's SoundBankHeader. */
typedef struct EngnSeg {
    s16  base_rpm;          /* 0x00 rpm at which the sample plays at 'pitch' */
    s16  lo;                /* 0x02 rpm range of the segment */
    s16  hi;                /* 0x04 */
    s16  vol;               /* 0x06 */
    u32  pitch;             /* 0x08 */
    u32  addr;              /* 0x0C SPU address (bytes), relocated by the upload base */
} EngnSeg;
EGCS_SIZE(EngnSeg, 0x10);

typedef struct EngnSet {
    u32      count;         /* 0x00 */
    EngnSeg *segs;          /* 0x04 */
} EngnSet;
EGCS_SIZE(EngnSet, 0x8);

typedef struct EngnBank {
    u32   magic;            /* 0x00 ENGN_MAGIC */
    void *self;             /* 0x04 load address it was built for */
    int   size;             /* 0x08 bytes of the header */
    u32   spuBase;          /* 0x0C */
    int   spuSize;          /* 0x10 bytes of sample data */
    void *samples;          /* 0x14 the sample data (file offset, relocated; T468 samples) */
    EngnSet set[2];         /* 0x18 */
} EngnBank;
EGCS_SIZE(EngnBank, 0x28);

/* The INST bank (.ins); the head is T468's SoundBankHeader. */
typedef struct InstBank {
    u32   magic;            /* 0x00 INST_MAGIC */
    void *self;             /* 0x04 */
    int   size;             /* 0x08 bytes of the header */
    u32   spuBase;          /* 0x0C */
    int   spuSize;          /* 0x10 bytes of sample data */
    void *samples;          /* 0x14 (relocated; T468 samples) */
    u32   nzones;           /* 0x18 */
    InstZone *zones;        /* 0x1C */
    u32   nprograms;        /* 0x20 */
    InstProgram **programs; /* 0x24 */
} InstBank;
EGCS_SIZE(InstBank, 0x28);

/* "SEQG" sequence bank (music.seq): a 0xC-byte head, then the sequences. */
#define SEQ_NUM_TRACKS  16
typedef struct SeqgEntry {
    u16  volume;            /* 0x00 */
    u16  unk_02;            /* 0x02 */
    s32  tempo;             /* 0x04 */
    u8  *tracks[SEQ_NUM_TRACKS];    /* 0x08 */
} SeqgEntry;
EGCS_SIZE(SeqgEntry, 0x48);

typedef struct SeqgBank {
    u32   magic;            /* 0x00 SEQG_MAGIC */
    void *self;             /* 0x04 */
    u32   count;            /* 0x08 */
    SeqgEntry entries[1];   /* 0x0C count entries */
} SeqgBank;

#define ENGN_MAGIC  0x4E474E45u     /* "ENGN" */
#define INST_MAGIC  0x54534E49u     /* "INST" */
#define SEQG_MAGIC  0x47514553u     /* "SEQG" */

/* A playing sequence (singly linked from SeqSystem.head). */
typedef struct Sequence {
    struct Sequence *next;  /* 0x00 */
    u16  volume;            /* 0x04 sequence volume (fadeable) */
    u16  volEff;            /* 0x06 master volume * volume >> 14 (seq_update_volume) */
    s32  tempo;             /* 0x08 cmd 6 (24-bit): subtracted per tick */
    s32  tickAcc;           /* 0x0C tick accumulator */
    s32  frameStep;         /* 0x10 added to tickAcc every frame */
    s32  ticks;             /* 0x14 ticks to run this frame */
    InstBank *bank;         /* 0x18 instruments (seq_start) */
    SndRamp fade;           /* 0x1C volume fade */
    SeqTrack tracks[16];    /* 0x28 */
} Sequence;
EGCS_SIZE(Sequence, 0x2A8);

/* A sounding note of a sequence (SeqSystem.ch[47]). */
typedef struct SeqChannel {
    Sequence    *seq;       /* 0x00 owner, NULL = owner stopped (release) */
    SeqTrack    *track;     /* 0x04 */
    InstProgram *prog;      /* 0x08 */
    InstZone    *zone;      /* 0x0C */
    s32  remain;            /* 0x10 ticks until key-off (<= 0: held / done) */
    u16  lastVol;           /* 0x14 track->volScaled last sent */
    s16  lastBend;          /* 0x16 track->bend last sent */
    u8   note;              /* 0x18 */
    u8   velocity;          /* 0x19 index into g_00345F10 */
    u8   lastPan;           /* 0x1A track->pan last sent */
    s8   voice;             /* 0x1B voice handle, -1 = none */
} SeqChannel;
EGCS_SIZE(SeqChannel, 0x1C);

/* The sequencer (0x002FF958; seq_init clears all 0x53C bytes). */
typedef struct SeqSystem {
    Sequence *head;         /* 0x00 playing sequences */
    u16  frameRate;         /* 0x04 frames per second (ramp frames = ms * frameRate / 1000) */
    u16  volume;            /* 0x06 master volume (0x3FFF after seq_init) */
    u8   nextChannel;       /* 0x08 where seq_note_on's channel search starts */
    u8   unk_09[3];         /* 0x09 */
    SndRamp fade;           /* 0x0C master fade */
    SeqChannel ch[47];      /* 0x18 (0x002FF970) */
} SeqSystem;
EGCS_SIZE(SeqSystem, 0x53C);

/* ======================================================================== */
/* Globals                                                                  */
/* ======================================================================== */

/* .data placed before T530's .eh_frame block (G002 has no frames of its own). */
struct SpuState;                    /* T520_pdispu2.h, 0x350 bytes */
extern MpegCallbackSlot g_mpeg_callbacks[5]; /* 0x002FF140 */
extern SndVoiceBank g_002FF168;     /* 0x002FF168 voice bank (46 voices) */
extern SeqSystem g_002FF958;        /* 0x002FF958 sequencer globals + 47 channels (0x002FF970) */
extern struct SpuState g_00302698;  /* 0x00302698 SPU2 state block sent to the IOP (pdispu2_send_state) */

extern const u16 g_00345F10[128];   /* 0x00345F10 7-bit volume / velocity -> 0..0x4000 (0x4000 * i / 127 rounded; defined in u124's sound_seq.c) */
extern int g_mb_height;             /* 0x0035313C gp-0x48B4 frame-store height in macroblocks */
extern int g_snd_ready;             /* 0x003531A8 gp-0x4848 sound system up (snd_init / snd_shutdown) */
extern int g_seq_ready;             /* 0x003531AC gp-0x4844 sequencer initialised (seq_init) */

/* ======================================================================== */
/* MPEG player (u121)                                                       */
/* ======================================================================== */

/* mpeg_player.c (one object: the player, the input ring, the frame
 * queue, the PES parser, the video decoder, the MMI code and the macroblock
 * moves, 0x0025F390..0x00263368) */
/* mpeg_player_put_pes results. */
#define PUT_PES_OK          0
#define PUT_PES_STOPPED     1   /* the decoder thread has finished */
#define PUT_PES_EMPTY       2   /* a packet without payload */
#define PUT_PES_NOT_READY   3   /* not running yet, or no room in the ring */

MpegPlayer *mpeg_player_create(int width, int height, int qpriority, int priority); /* 0x0025F390 */
void mpeg_player_delete(MpegPlayer *pl);                        /* 0x0025F510 */
int  mpeg_player_get_width(MpegPlayer *pl);                     /* 0x0025F598 */
int  mpeg_player_get_height(MpegPlayer *pl);                    /* 0x0025F5A0 */
int  mpeg_player_on_picture(MpegPlayer *pl, void *pic);         /* 0x0025F5A8 */
void mpeg_player_thread(void *arg);                             /* 0x0025F5F8 */
int  mpeg_player_put_pes(MpegPlayer *pl, const u8 *pkt);        /* 0x0025F6E0 */
int  mpeg_player_is_frameq_full(MpegPlayer *pl);                /* 0x0025F820 */
u8  *mpeg_player_peek_frame(MpegPlayer *pl);                    /* 0x0025F840 */
void mpeg_player_release_frame(MpegPlayer *pl);                 /* 0x0025F870 */
void mpeg_player_abort_frameq(MpegPlayer *pl);                  /* 0x0025F890 */

/* the input ring */
void mpeg_vibuf_set_geometry(int log2n, int log2elm);           /* 0x0025F698 */
MpegViBuf *mpeg_vibuf_create(int priority);                     /* 0x0025F898 */
void mpeg_vibuf_delete(MpegViBuf *vb);                          /* 0x0025F938 */
int  mpeg_vibuf_get_size(void);                                 /* 0x0025F978 */
int  mpeg_player_get_buffered(MpegPlayer *pl);                  /* 0x0025F980 */
int  mpeg_vibuf_add_dma(MpegViBuf *vb, void *arg);              /* 0x0025FC48 callback slot 0 */
int  mpeg_vibuf_stop_dma(MpegViBuf *vb, void *arg);             /* 0x0025FF08 callback slot 1 */
int  mpeg_vibuf_restart_dma(MpegViBuf *vb, void *arg);          /* 0x0025FF48 callback slot 2 */

/* the frame queue */
MpegFrameQueue *mpeg_frameq_create(int elemSize, int count, int priority, int threadPriority); /* 0x002602E8 threadPriority: only stored */
void mpeg_frameq_delete(MpegFrameQueue *q);                     /* 0x002603B8 */

/* the PES parser */
int  mpeg_pes_parse(const u8 *pkt, const u8 **data, int *len, s64 *pts, s64 *dts); /* 0x00260748 */

/* the video decoder */
int  mpeg_video_decode_slice(int mb_total);                     /* 0x00260EC0 */
int  mpeg_video_macroblock_modes(int *mb_type_out, int *dct_type_out, MpegMv *mv); /* 0x00261260 */
int  mpeg_video_slice_header(int *mb_addr, int *mb_inc);        /* 0x002613F0 */
void mpeg_video_next_start_code(void);                          /* 0x00261560 */
void mpeg_video_sequence_header(void);                          /* 0x002615A8 */
void mpeg_video_motion_vector(MpegMv *mv, int *pmv, int r_size_h, int r_size_v); /* 0x00261B78 */
void mpeg_video_motion_compensate(MpegMv *mv, u32 mb_type, int mb_x, int mb_y);  /* 0x00261D08 */
void mpeg_mc_form_prediction(u8 *ref, int x, int y, int dx, int dy, int avg);    /* 0x00261DE0 */

/* the MMI pixel code and the macroblock moves */
void mpeg_mc_chroma(u8 *dst, const u8 *src, int x, int y, int dx, int dy, int avg); /* 0x00262610 */
void mpeg_get_mb(MpegMB *dst, u8 *frame, int x, int y);         /* 0x00262B60 */
void mpeg_fetch_ref_mbs(u8 *dst, u8 *frame, int x, int y, int dx, int dy); /* 0x00262C48 */
void mpeg_put_mb(const MpegMB *src, u8 *frame, int x, int y);   /* 0x00263280 */

/* ======================================================================== */
/* IPU layer (u122)                                                         */
/* ======================================================================== */

/* dma_util.c */
void *mpeg_dma_alloc(u32 size);                                 /* 0x00263368 64-byte aligned */
void  mpeg_dma_free(void *p);                                   /* 0x002633A8 */

/* ipu.c */
int  ipu_bdec(IpuBdecParam *p);                                 /* 0x00263688 0 ok, 1 start code, 2 nothing to do, 3 VLC error */
void ipu_csc_rgb32(u32 dst, u32 src, int nmb);                  /* 0x00263938 */
void ipu_align_byte(void);                                      /* 0x00263C00 */
void ipu_bclr(u32 bp);                                          /* 0x00263C38 */
void ipu_stop_dma(IpuDmaEnv *env);                              /* 0x00263D90 */
void ipu_dma_reset(void);                                       /* 0x00263F28 */
u32  ipu_getbits(int n);                                        /* 0x00264070 */
u32  ipu_showbits(int n);                                       /* 0x002640B0 */
u32  ipu_showbits24(void);                                      /* 0x00264120 */
void ipu_flushbits(int n);                                      /* 0x00264180 */
void ipu_fdec8(void);                                           /* 0x002641C8 */
void ipu_init(void);                                            /* 0x002641F0 */
void ipu_load_intra_matrix(void);                               /* 0x002643D0 */
void ipu_load_nonintra_matrix(void);                            /* 0x00264480 */
void ipu_sync_init(u32 priority);                               /* 0x00264758 the decoder thread's (stored only) */
void ipu_sync_term(void);                                       /* 0x00264868 */
MpegCallback mpeg_set_callback(int slot, MpegCallback func, void *arg); /* 0x00264968 returns func */
void ipu_sync_set_thread(s32 tid);                              /* 0x00264C60 */
int  ipu_vdec_mb_type(int pict_type);                           /* 0x00264C70 */
int  ipu_vdec_mb_addr_inc(void);                                /* 0x00264D00 */
s16  ipu_vdec_motion_code(void);                                /* 0x00264DA8 */
s16  ipu_vdec_dmvector(void);                                   /* 0x00264E00 */

/* ipu_image.c (with its two interrupt handlers, to 0x00265530:
 * ipu_internal.h declares them) */
void ipu_decode_image(u32 dst, const IpuImageHeader *img);      /* 0x00264E58 into RGBA32 */

/* ======================================================================== */
/* Interrupts, USB, PJOY, memory card, clock (u123)                         */
/* ======================================================================== */

/* usbd.c */
int  usbd_init(int priority);                                   /* 0x00265610 */
void usbjoy_start(void);                                        /* 0x00265770 */

/* pjoy.c */
int  pjoy_get_event_count(int port);                            /* 0x00265E50 */
int  pjoy_read_state(int port, PjoyState *out);                 /* 0x00265EA0 */
int  pjoy_get_info(int port, PjoyInfo *out);                    /* 0x002660E0 */
int  ff_send_download_play(int port, FfCmd *cmd);               /* 0x00266830 */
FfCmd *ff_build_constant(FfCmd *c, int f0, int f1, int f2, int f3); /* 0x00266998 */

/* mcard_io.c: blocking libmc calls under one lock */
int  mc_close_sync(int fd);                                     /* 0x002670F0 */
int  mc_delete_sync(int port, int slot, const char *name);      /* 0x00267160 */
int  mc_format_sync(int port, int slot);                        /* 0x002671F0 */
int  mc_getdir_sync(int port, int slot, const char *name, unsigned int mode,
                    int maxent, sceMcTblGetDir *table);         /* 0x00267270 */
int  mc_getentspace_sync(int port, int slot, const char *path); /* 0x00267330 */
int  mc_getinfo_sync(int port, int slot, int *type, int *freeSpace, int *format); /* 0x002673C0 */
int  mc_getslotmax_sync(int port);                              /* 0x00267470 */
void mc_init_once(void);                                        /* 0x002674E0 */
int  mc_mkdir_sync(int port, int slot, const char *name);       /* 0x00267548 */
int  mc_open_sync(int port, int slot, const char *name, int mode); /* 0x002675D8 */
int  mc_read_sync(int fd, void *buf, int size);                 /* 0x00267678 */
int  mc_rename_sync(int port, int slot, const char *org, const char *newname); /* 0x00267708 */
int  mc_seek_sync(int fd, int offset, int origin);              /* 0x002677A8 */
int  mc_setfileinfo_sync(int port, int slot, const char *name,
                         const char *info, unsigned int valid); /* 0x00267838 */
int  mc_unformat_sync(int port, int slot);                      /* 0x00267948 */
int  mc_write_sync(int fd, const void *buf, int size);          /* 0x002679C8 */

/* scf_lock.c */
void scf_lock_init(void);                                       /* 0x00267A58 */
void scf_clock_to_gmt(sceCdCLOCK *rtc);                         /* 0x00267A80 */
void scf_clock_to_local(sceCdCLOCK *rtc);                       /* 0x00267AB8 */

/* ======================================================================== */
/* Sound (u123, u124)                                                       */
/* ======================================================================== */

/* sound_driver.c */
void snd_voice_release(s8 *handle);                             /* 0x00267AF0 */
void snd_voice_keyoff(s8 *handle);                              /* 0x00267B48 */
void snd_voice_set_pitch(s8 *handle, int pitch);                /* 0x00267BA8 */
void snd_voice_set_priority(s8 *handle, int prio);              /* 0x00267C18 */
void snd_voice_set_volume(s8 *handle, const u32 *vol);          /* 0x00267C70 *vol = volL | volR << 16 */
void engine_update(GtSoundVoice *es);                           /* 0x00267F30 */
void engine_setup(GtSoundVoice *es, EngnBank *bank, int set, int prio); /* 0x00268208 */
void engine_stop(GtSoundVoice *es);                             /* 0x00268278 */
void engine_set_priority(GtSoundVoice *es, int prio);           /* 0x002682D0 */
EngnBank *engn_relocate(EngnBank *b, u32 spu_base);             /* 0x00268340 */
void snd_shutdown(void);                                        /* 0x00268430 */
void snd_init(void);                                            /* 0x00268470 */
void snd_set_rate(int rate);                                    /* 0x002684D8 */
void seq_init(void);                                            /* 0x00268508 */
void seq_set_frame_rate(int rate);                              /* 0x00268598 */
void snd_play_voice(s8 *handle, SndVoiceParam *p);              /* 0x002685A8 */
InstBank *inst_relocate(InstBank *b, u32 spu_base);             /* 0x00268630 */
SeqgBank *seqg_relocate(SeqgBank *b);                           /* 0x00268718 */
u16  snd_note_to_pitch(int note, int base);                     /* 0x002687B0 8.8 semitones */
void seq_update_volume(Sequence *seq);                          /* 0x00268860 */
void seq_restart(Sequence *seq);                                /* 0x002688A8 */
void F_00268978(int v);                                         /* 0x00268978 spu_state_set_unk08(&g_00302698, v, v) */
void snd_set_reverb(int mode, ...);                             /* 0x00268990 [delay] [feedback] [depthL depthR] */
void snd_set_effect_volume(int l, int r);                       /* 0x00268A48 spu_state_set_unk04(&g_00302698, l, r) (T177: the reverb level) */
void seq_unlink(Sequence *seq);                                 /* 0x00268A60 */
void seq_start(Sequence *seq, SeqgEntry *e, InstBank *bank);    /* 0x00268AB0 */
void seq_set_master_volume(u16 vol);                            /* 0x00268BF8 (the caller masks: andi 0xFFFF) */
void seq_note_on(Sequence *seq, SeqTrack *t, int note, int velocity, int duration); /* 0x00268C40 */

/* sound_seq.c */
void seq_update(void);                                          /* 0x002691F0 */
void seq_stop(Sequence *seq);                                   /* 0x002694E8 */
void snd_flush(void);                                           /* 0x00269530 */
void snd_flush_voices(void);                                    /* 0x00269550 */
void snd_pan_volume(s16 *out, int vol, int pan);                /* 0x00269750 out[0] left, out[1] right */
int  snd_voice_alloc(int priority);                             /* 0x00269798 voice index, -1 none */
void snd_voice_start(int idx, const SndVoiceParam *p);          /* 0x00269890 */

/* ======================================================================== */
/* strlen, threads, host fileio, math (u124)                                */
/* ======================================================================== */

/* strlen.S */
size_t strlen(const char *s);                                   /* 0x00269A68 */

/* thread_util.c */
int  intr_suspend(void);                                        /* 0x00269B00 di; 1 if interrupts were on */
/* The end of every intr_suspend section (Polyphony's inline, no function of
 * its own): a bare `ei' only when interrupts were on before.  Under egcs
 * without a "memory" clobber, as the original's: retail's GCSE hoists loads
 * across it (u037 PlayMusicBox::virtual_9 0x00169B50 loads checkBox before
 * the colour loop and the ei; with the clobber it stays after the ei).  No
 * other user of it changed (15 files scored, ref/matching/u037/notes.md).
 * GCC 14 keeps the clobber: the moddable build keeps memory accesses inside
 * the section. */
static __inline__ void intr_resume(int wasEnabled)
{
    if (wasEnabled)
#ifdef BOOT_OLD_GCC
        __asm__ __volatile__("ei");
#else
        __asm__ __volatile__("ei" : : : "memory");
#endif
}
int  create_mutex_sema(int initCount);                          /* 0x00269B30 */
void yield_low_priority(void);                                  /* 0x00269BA0 */
int  signal_sema_helper(int semaId);                            /* 0x00269C10 */
int  thread_start_retry(void (*entry)(void *), void *arg, int priority, void *stack, int stackSize); /* 0x00269C30 */
void worker_thread_start(ThreadCtx *w, void (*entry)(void *), void *arg, int priority,
                         void *stack, int stackSize);           /* 0x00269C90 */
/* 0x00269D18: the definition returns DeleteSema's result (retail keeps
 * jal + frame, no tail jump), but its callers were compiled against a void
 * prototype (T055 logger_record_join reuses $v0 right after the call), so
 * the original had two declarations; thread_util.c defines
 * G002_WORKER_JOIN_INT before including this header. */
#ifdef G002_WORKER_JOIN_INT
int  worker_thread_join(ThreadCtx *w);
#else
void worker_thread_join(ThreadCtx *w);
#endif
void worker_thread_exit(ThreadCtx *w);                          /* 0x00269D48 */

/* hostfs_io.c */
int  host_open(const char *path, int flags);                    /* 0x00269D80 */
int  host_lseek(int fd, int offset, int whence);                /* 0x00269DC8 */
int  host_read(int fd, void *buf, int len);                     /* 0x00269E20 */
int  host_write(int fd, const void *buf, int len);              /* 0x00269E78 */
int  host_close(int fd);                                        /* 0x00269ED0 */
int  io_getstat(const char *path, IoStat *out);                 /* 0x00269F08 */

/* fastmath.c */
f32  atan2f(f32 y, f32 x);                                      /* 0x00269FC8 */
f32  cosf(f32 x);                                               /* 0x0026A1A0 */
f32  expf(f32 x);                                               /* 0x0026A270 */
f32  logf(f32 x);                                               /* 0x0026A388 */
f32  powf(f32 x, f32 y);                                        /* 0x0026A4A0 */
void sincosf(f32 x, f32 *s, f32 *c);                            /* 0x0026A4D0 */
f32  sinf(f32 x);                                               /* 0x0026A5F0 */
f32  tanf(f32 x);                                               /* 0x0026A6C0 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_G002_MPEG_FRAMEQ_H */
