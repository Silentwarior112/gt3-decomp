/* G006_fp_bit.h - the no-FDE stretch G006 (unit u135, C): fp-bit.o and frame.o of the
 * GCC 2.9x-ee runtime.
 *
 *   fp-bit.o  0x0027A400..  config/fp-bit.c compiled for FLOAT (soft float; the
 *                           game uses the conversions, the EE FPU does the rest)
 *   frame.o   0x0027AFE0..  gcc/frame.c, the DWARF2 unwind-info registry
 * Same machine code as the bootstrap's runtime/libgcc_math.c (fp-bit part) and
 * runtime/frame.c.  Also declared (bootstrap addresses) in include/cxxrt.h.
 */
#ifndef GAME_G006_FP_BIT_H
#define GAME_G006_FP_BIT_H

#include "game/game.h"
#include "runtime/libgcc/G005_dp_bit.h"                   /* fp_class_type; SFtype, DFtype, USItype (G004) */

#ifdef __cplusplus
extern "C" {
#endif

struct object;          /* frame.c registration record, 0x18 bytes (G000_collect2_ctors.h) */
struct frame_state;     /* unwind state, 0x2F0 bytes (T542_eh.h) */

/* ---- fp-bit.o ------------------------------------------------------------ */
USItype fptoui(SFtype arg_a);                   /* 0x0027AEA0 [__fixunssfsi] */
/* Packs an unpacked float; called by dptofp (G005). */
SFtype  __make_fp(fp_class_type fp_class, unsigned int sign, int exp,
                  USItype frac);                /* 0x0027AF70 */
DFtype  fptodp(SFtype arg_a);                   /* 0x0027AFA0 [__extendsfdf2] */

/* ---- frame.o ------------------------------------------------------------- */
/* crt glue (G000): registers / deregisters the .eh_frame of the program
 * (FDE-pointer table 0x0028E780). */
void   __register_frame_info_table(void *begin, struct object *ob);    /* 0x0027BBD0 */
struct object *__deregister_frame_info(void *begin);                    /* 0x0027BC28 */
/* The CFA program interpreter for the EH runtime's __throw (T542): fills
 * *state_in for the frame of pc_target; NULL if no FDE covers it. */
struct frame_state *__frame_state_for(void *pc_target,
                                      struct frame_state *state_in);   /* 0x0027BCC0 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_G006_FP_BIT_H */
