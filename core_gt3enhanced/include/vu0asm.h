/* vu0asm.h - VU0 macro-mode special registers in inline assembly.
 *
 * The two assemblers that build this tree spell them differently, and
 * neither takes the other's spelling:
 *   GNU as of the EE toolchain (GCC 14):        $ACC  $Q  $I  $R
 *   ee-as 2.9-ee-991111b (the original compiler): ACC   Q   I   R
 * so inline asm writes them as string macros:
 *   "vmulax.xyzw " VU0_ACC ", $vf20, $vf16x\n\t"
 * The VF registers ($vf0..$vf31, with an x/y/z/w broadcast suffix) are the
 * same for both.
 */
#ifndef VU0ASM_H
#define VU0ASM_H

#if defined(__GNUC__) && __GNUC__ < 3
#define VU0_ACC "ACC"
#define VU0_Q   "Q"
#define VU0_I   "I"
#define VU0_R   "R"
#else
#define VU0_ACC "$ACC"
#define VU0_Q   "$Q"
#define VU0_I   "$I"
#define VU0_R   "$R"
#endif

#endif /* VU0ASM_H */
