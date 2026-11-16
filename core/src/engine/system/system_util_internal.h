/* system_util_internal.h - unit u124 of the GT3 main executable, 0x00268ED8..0x0026A7C8.
 *
 * The end of the no-FDE stretch G002 (plain C, no exceptions).  Files:
 *   sound_seq.c    the tail of Polyphony's sound-effect sequencer /
 *                            SPU2 voice manager (its start is in u123): track
 *                            event interpreter, per-frame tick, channel ->
 *                            voice parameter update, voice flush to the IOP
 *                            (pdispu2.cxx), voice allocation, volume ramps
 *   strlen.S       the game-provided MMI strlen (hand-written asm,
 *                            same code as bootstrap 0x0100F538)
 *   thread_util.c  intr_suspend, semaphores / thread helpers (most
 *                            shared with the bootstrap's thread_util.cpp)
 *   hostfs_io.c    locked "host:" fileio wrappers (bootstrap
 *                            hostfs_io.cpp) and io_getstat
 *   fastmath.c     single-precision math: atan2f, powf (C) and the
 *                            hand-written atanf, cosf, expf, logf, sincosf,
 *                            sinf, tanf (polynomial / Pade approximations,
 *                            using the R5900 FPU accumulator madd/adda; the
 *                            retail instructions in the matching build)
 *
 * The canonical header engine/media/G002_mpeg_frameq.h declares what other units
 * call and the sound types G002 shares (SndVoice, SndVoiceBank,
 * SndVoiceParam, SndRamp, InstZone, InstProgram, SeqTrack, Sequence,
 * SeqChannel, SeqSystem, their globals) and the worker-thread record
 * ThreadCtx.  engine/pdi/T520_pdispu2.h has the SPU2 state block the voices are
 * flushed into (SpuState, SpuVoiceState), engine/pdi/G003_pdi1394_rpc.h thread_create_retry and the global lock.  This
 * header keeps what only the unit uses: IoStat's layout, the unit's own data,
 * its internal functions and named constants.
 */
#ifndef U124_H
#define U124_H

#include "types.h"
#include "eekernel.h"                   /* DI / EI, threads, semaphores, SemaParam / ThreadParam */
#include "sifdev.h"                     /* sceOpen & co., SCE_RDONLY, SCE_SEEK_END, SCE_STM_* */
#include "engine/media/G002_mpeg_frameq.h"      /* own translation unit */
#include "engine/pdi/G003_pdi1394_rpc.h"      /* thread_create_retry, global_lock_acquire / release */
#include "engine/pdi/T520_pdispu2.h"          /* SpuState, SpuVoiceState, pdispu2_send_state, pdispu2_status */

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* The bits of a float (mfc1) and back (mtc1). */
static inline u32 u124_f2u(f32 f) { union { f32 f; u32 u; } c; c.f = f; return c.u; }
static inline f32 u124_u2f(u32 u) { union { f32 f; u32 u; } c; c.u = u; return c.f; }

/* EE COP1 min.s (the EE FPU has no NaNs). */
static inline f32 u124_minf(f32 a, f32 b) { return a < b ? a : b; }

/* SndVoice +0x08 / +0x14: the halfword pairs volL / volR and adsr1 / adsr2
 * are written as one word (sw) from SndVoiceParam.vol / .adsr.  G002 keeps
 * the halfwords (snd_flush_voices reads them with lhu), so the word store
 * goes through a cast; for GCC >= 4 the word type may alias the halfwords. */
/* (SND_HALF_PAIR, G002_mpeg_frameq.h) */

/* ======================================================================== */
/* Sound sequencer (sound_seq.c)                                  */
/* ======================================================================== */

#define SND_NUM_VOICES      46          /* SndVoiceBank.voices (the IOP block has 48) */
#define SEQ_NUM_CHANNELS    47          /* SeqSystem.ch */
#define SEQ_NUM_TRACKS      16          /* Sequence.tracks */
#define SND_VOL_SHIFT       14          /* volumes and gains: 0x4000 = 1.0 */
#define SND_PAN_MAX         0x7F        /* pan 0 = left .. 0x7F = right, 0x40 centre */
#define SND_PAN_BIAS        0x80        /* subtracted from the sum of the three pans (two 0x40 centres) */

/* SndVoice.state */
enum {
    SND_VOICE_FREE      = 0,
    SND_VOICE_ALLOCATED = 1             /* bound to an owner's handle byte */
};

/* SndVoice.dirty: what the next flush must send. */
#define SND_DIRTY_PITCH     0x1
#define SND_DIRTY_VOLUME    0x2
#define SND_DIRTY_ADSR      0x4
#define SND_DIRTY_ALL       0x7

/* SpuVoiceState.flags, rebuilt by snd_flush_voices for every voice. */
#define SPU_VOICE_KEY_OFF   0x001
#define SPU_VOICE_KEY_ON    0x002
#define SPU_VOICE_UNK_A0    0x0A0       /* SndVoice.unk_28 set (? meaning: decided on the IOP side) */
#define SPU_VOICE_UNK_100   0x100       /* SndVoice.unk_20 non-zero (? idem) */
#define SPU_VOICE_PITCH     0x200
#define SPU_VOICE_VOLUME    0x400
#define SPU_VOICE_ADSR      0x800
#define SPU_VOICE_REBUILT   0xFA3       /* the bits above; the others (0xF05C) are kept */

/* SndVoice.pitch is 44.1 kHz based, the SPU2's 48 kHz: 4096 * 44100 / 48000. */
#define SND_PITCH_44K_TO_48K    0xEB3
#define SND_PITCH_ONE           4096

/* Track byte code (seq_run_tracks).  Bit 7 / 6 / 5 select the three
 * compact events; below 0x20 the byte is an opcode. */
#define SEQ_EV_NOTE         0x80        /* 1nnnnnnn velocity length(VLQ) */
#define SEQ_EV_BEND         0x40        /* 01hhhhhh llllllll */
#define SEQ_EV_LOOP_SHORT   0x20        /* 001ccccc: loop start, count c + 1 */
enum {
    SEQ_OP_END       = 0,               /* end of track (no delta follows) */
    SEQ_OP_LOOP      = 1,               /* count byte: loop start */
    SEQ_OP_LOOP_END  = 2,
    SEQ_OP_PROGRAM   = 3,               /* program byte */
    SEQ_OP_VOLUME    = 4,               /* 7-bit volume byte */
    SEQ_OP_PAN       = 5,               /* pan byte */
    SEQ_OP_TEMPO     = 6,               /* 24-bit big-endian tempo */
    SEQ_OP_COUNT     = 7                /* 7..0x1F: no operand, ignored */
};
#define SEQ_VLQ_MORE        0x80        /* variable-length quantity: 7 bits per byte */
#define SEQ_BEND_CENTRE     0x2000      /* 14-bit bend value of "no bend" */
#define SEQ_LOOP_FOREVER    0xFF        /* SeqTrack.loopCount */


/* Data only this unit uses (addresses from the listing), defined in
 * sound_seq.c with g_00345F10 (engine/media/G002_mpeg_frameq.h). */
extern const u16 g_snd_pan_law[128];    /* 0x00345E10 0x4000 * sin(pi/2 * i / 127): equal-power pan */
extern int g_snd_flush_count;           /* 0x003531B0 gp-0x4840 snd_flush_voices calls (never read) */

/* Functions of this unit that no other unit calls. */
void seq_run_tracks(Sequence *seq);                         /* 0x00268ED8 */
void seq_update_channels(void);                             /* 0x00269328 */
void snd_ramp_init(SndRamp *r, int from, int to, int ms);   /* 0x00269948 */
int  snd_ramp_step(SndRamp *r);                             /* 0x002699D0 */
void seq_fade(Sequence *seq, u16 to, int ms);               /* 0x00269A28 */
void seq_master_fade(u16 to, int ms);                       /* 0x00269A48 */

/* ======================================================================== */
/* Threads (thread_util.c)                                        */
/* ======================================================================== */

#define YIELD_PRIORITY      0x7E        /* yield_low_priority's temporary priority */
#define YIELD_PASSES        100
#define YIELD_SPIN          10          /* busy_wait_delay argument per pass */
#define WORKER_EXIT_PRIORITY 1          /* worker_thread_exit: not preempted before it exits */

void busy_wait_delay(s64 n);                                /* 0x00269F88 */

/* ======================================================================== */
/* "host:" fileio (hostfs_io.c)                                   */
/* ======================================================================== */

/* IoStat (what io_getstat fills): engine/media/G002_mpeg_frameq.h. */

/* ======================================================================== */
/* Math (fastmath.c)                                              */
/* ======================================================================== */

f32  atanf(f32 x);                          /* 0x0026A088 (only caller atan2f) */

#ifndef __cplusplus
#endif

#ifdef __cplusplus
}
#endif

#endif /* U124_H */
