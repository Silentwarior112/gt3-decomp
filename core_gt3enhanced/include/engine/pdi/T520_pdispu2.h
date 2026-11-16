/* T520_pdispu2.h - pdispu2.cxx: SpuEffectAttr, SpuState and the SPU2 calls
 * over the IOP (translation unit T520, unit u119).
 *
 * A C header as well: the G002 sound code (u123, u124: C) calls these
 * functions, so they have C linkage. */
#ifndef GAME_T520_PDISPU2_H
#define GAME_T520_PDISPU2_H

#include "game/game.h"

/* The file's two RPC objects are of the class RPC of its anonymous namespace
 * (_GLOBAL_$N$.._.._pdispu2.cxx9KEP7e), derived from PDIClientRpc (T527):
 * vtable 0x00345890, typeinfo 0x0025C7E0, 0x390 bytes (send area +0x40, 0x350
 * bytes: a SpuState).  RPC() 0x0025BF38, ~RPC() [0] +0x0C 0x0025BF80,
 * [1] PDIClientRpc::onCallEnd, transfer 0x0025BFA8; inline: F_0025C598,
 * sendState 0x0025C608, requestStatus 0x0025C740, F_0025C788.
 *   0x004A6F50  server 'SPUP' (0x53505550): the state (fno 4), a status
 *               request (fno 3), fno 5; answers in the 0x40-byte 0x002FCA00;
 *   0x004A6BC0  server 'SPUV' (0x53505556): transfers of an EE buffer (fno 1 / 2).
 * The class stays defined in src/engine/pdi/T520_pdispu2.cpp (see T519_pdicdvd.h). */

/* sceSdEffectAttr (libsd): the effect (reverb) of one SPU2 core. */
typedef struct SpuEffectAttr {
    int   core;                 /* 0x00 */
    int   mode;                 /* 0x04 */
    short depth_L;              /* 0x08 */
    short depth_R;              /* 0x0A */
    int   delay;                /* 0x0C */
    int   feedback;             /* 0x10 */
} SpuEffectAttr;
EGCS_SIZE(SpuEffectAttr, 0x14);

/* One voice's command, rebuilt by the G002 sound code (u124 snd_flush_voices)
 * before every send.  flags: 1 key-off, 2 key-on, 0xA0, 0x100, 0x200 pitch,
 * 0x400 volume, 0x800 ADSR; bits 0xF05C are kept from the last block. */
typedef struct SpuVoiceState {
    s16 flags;                  /* 0x00 init 0x50 */
    s16 pitch;                  /* 0x02 SPU2 pitch (48 kHz based) */
    u16 volL;                   /* 0x04 */
    u16 volR;                   /* 0x06 */
    s32 addr;                   /* 0x08 sample address in bytes */
    u16 adsr1;                  /* 0x0C */
    u16 adsr2;                  /* 0x0E */
} SpuVoiceState;
EGCS_SIZE(SpuVoiceState, 0x10);

typedef struct SpuCoreState {
    u16 unk_00;                 /* 0x00 init 0x3FFF (? master volume L) */
    u16 unk_02;                 /* 0x02 init 0x3FFF (? master volume R) */
    u16 unk_04;                 /* 0x04 init 0x7FFF (? effect volume L), spu_state_set_unk04 */
    u16 unk_06;                 /* 0x06 init 0x7FFF (? effect volume R), spu_state_set_unk04 */
    u8  unk_08;                 /* 0x08 init 0, spu_state_set_unk08 */
    u8  unk_09[3];              /* 0x09 */
    u32 unk_0C;                 /* 0x0C core 0: 0xFC0, core 1: 0xFCC */
    SpuEffectAttr effect;       /* 0x10 effect.core = the core's number */
} SpuCoreState;
EGCS_SIZE(SpuCoreState, 0x24);

/* The whole SPU2 state, sent in one call (fno 4) to 'SPUP'.  The G002 sound
 * code keeps one at 0x00302698 (u124 uses voices 0..45). */
typedef struct SpuState {
    SpuVoiceState voice[48];    /* 0x000 2 cores x 24 voices */
    SpuCoreState  core[2];      /* 0x300 */
    u16 unk_348;                /* 0x348 init 0x3FFF */
    u16 unk_34A;                /* 0x34A init 0x3FFF */
    int dirty;                  /* 0x34C set by spu_state_set_effect, cleared when sent */
} SpuState;
EGCS_SIZE(SpuState, 0x350);

#ifdef __cplusplus
extern "C" {
#endif

void pdispu2_init(void);                                    /* 0x0025C048 binds 'SPUP' and 'SPUV' */
void F_0025C088(void *buf, u32 spuAddr, int size);          /* 0x0025C088 'SPUV' fno 1 {spuAddr, buf, size} */
void F_0025C0C0(void *buf, u32 spuAddr, int size);          /* 0x0025C0C0 'SPUV' fno 2 {spuAddr, buf, size} */
void pdispu2_request_status(void);                          /* 0x0025C0F8 'SPUP' fno 3 */
/* Copies *st with interrupts off, clears st->dirty and sends it (NOWAIT). */
void pdispu2_send_state(SpuState *st);                      /* 0x0025C140 */
void F_0025C280(int a, int b, int c);                       /* 0x0025C280 'SPUP' fno 5 {a, b, c} (NOWAIT) */
void spu_state_init(SpuState *st);                          /* 0x0025C2F8 */
void spu_state_set_unk08(SpuState *st, int core0, int core1); /* 0x0025C3B0 (dirty not set) */
/* The copies' core fields are forced to 0 / 1; sets st->dirty. */
void spu_state_set_effect(SpuState *st, const SpuEffectAttr *core0, const SpuEffectAttr *core1); /* 0x0025C400 */
void spu_state_set_unk04(SpuState *st, int l, int r);       /* 0x0025C4A8 the same pair in both cores (dirty not set) */
/* First doubleword of the last 'SPUP' answer: the voice-active bit mask
 * (bit i = voice i still sounding). */
u64  pdispu2_status(void);                                  /* 0x0025C500 */
/* T520_static_fini 0x0025C530 (.dtors) and T520_static_init 0x0025C568 (.ctors) are
 * egcs's own static destructor / initialiser of the two objects (no source
 * functions: the objects are defined in the .cpp file). */

#if defined(__cplusplus) && !defined(BOOT_OLD_GCC)  /* egcs generates it itself */
void *tf_pdispu2_RPC(void);     /* 0x0025C7E0 __tfQ234_GLOBAL_$N$.._.._pdispu2.cxx9KEP7e3RPC */
#endif

#ifdef __cplusplus
}
#endif

#endif /* GAME_T520_PDISPU2_H */
