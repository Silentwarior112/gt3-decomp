/* gs_internal.h - helpers shared by the translation units of the GS driver
 * (src/gs_texture.cpp, src/gs_driver.cpp, src/gs_state.cpp; unit "gs").
 * Not part of the driver's interface (include/gs_driver.h).
 */
#ifndef BOOT_GS_INTERNAL_H
#define BOOT_GS_INTERNAL_H

#include "types.h"
#include "gs_driver.h"

/* The scratchpad context pointer.  The retail code never folds 0x70003000
 * into the field offsets: every function loads it whole into a register
 * (an unsplit `li`, lui+ori always adjacent) at its start, and a wrapper
 * whose only statement passes it on does not tail-call.  A statement
 * expression around a one-instruction asm reproduces that under egcs; GCC 14
 * gets the plain constant.  Same pointer value either way.                */
#ifdef BOOT_OLD_GCC
#define GS_SPR_CTX ({ GsContext *ctx_; __asm__("li %0, 0x70003000" : "=r"(ctx_)); ctx_; })
#else
#define GS_SPR_CTX GS_CTX
#endif

/* EE COP1 max.s / min.s (the '<bad>' 0x28/0x29 functs in the listing).
 * egcs folds a > b ? a : b on floats to MAX_EXPR only under -ffast-math,
 * but the GNU C++ minimum/maximum operators <? and >? build MIN_EXPR /
 * MAX_EXPR directly (the sminsf3/smaxsf3 patterns = min.s/max.s; tex1.cpp
 * uses >? too).  With one-instruction asms instead, the scheduler leaves
 * the 128.0 load of gs_set_material_param behind the max (92.7% instead of
 * 96.8%).  GCC 14 no longer has the operators: it gets the equivalent
 * comparisons (the EE FPU has no NaNs). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline f32 ee_maxf(f32 a, f32 b) { return a >? b; }
static inline f32 ee_minf(f32 a, f32 b) { return a <? b; }
#else
static inline f32 ee_maxf(f32 a, f32 b) { return a > b ? a : b; }
static inline f32 ee_minf(f32 a, f32 b) { return a < b ? a : b; }
#endif

#endif /* BOOT_GS_INTERNAL_H */
