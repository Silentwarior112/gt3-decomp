/* pdistd_internal.h - unit u125 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0026A7C8..0x0026CA90.
 *
 * Translation units (link order):
 *   T530  pdistd::Allocator (allocator.cxx: the block allocator)    BOOT
 *   T530a PrintFormat's double table (no code: a data-only object)   BOOT
 *   T539  pdistd::PrintFormatTargetBase + pdistd::PrintFormat        BOOT
 *   T531  Random: a 32-bit pseudo-random generator (? name)
 *   T532  the global heap: heap_init_once, malloc family             BOOT
 *   T533  printf.cxx: line-buffered PrintTarget, vprintf, printf     BOOT
 *   T534  sprintf
 *   T535  vsprintf.cxx: string PrintTarget, vsprintf
 *   T536  BitStreamBuffer (MSB-first bit reader / writer)
 *
 * The classes and functions are declared in the canonical headers
 * include/game/T530..T536, T539 (pdistd::Allocator and
 * pdistd::PrintFormatTargetBase through the shared include/pdistd/allocator.h
 * and include/pdistd/printf.h, whose address comments are the bootstrap's).
 * This header keeps only what is local to the unit: the type_info storage
 * (the unit's own .bss / .sdata objects are file statics of T531 / T532).
 *
 * egcs ABI reminder: the vptr is a data member placed after the data members of
 * the first polymorphic class (Allocator: +0x14, BitStreamBuffer: +0x10).  A
 * modern g++ build puts it at +0x00; the offsets in the comments are the
 * original ones.
 */
#ifndef U125_H
#define U125_H

#include "types.h"
#include "libc.h"                       /* memset, memcpy, calloc, vprintf */
#include "eekernel.h"                   /* EI() */
#include "cxxrt.h"                      /* __rtti_user, __rtti_si */
#include <stddef.h>
#include <stdarg.h>
/* No <new>: its inline bad_alloc::what() puts "bad_alloc" into every
 * object's .rodata, and none of this unit's objects has it in retail (T530's
 * .rodata starts with the debug strings at 0x00346010). */

#include "engine/pdistd/T530_allocator.h"        /* pdistd::Allocator, intr_suspend */
#include "engine/pdistd/T539_print_format.h"     /* pdistd::PrintFormatTargetBase, PrintFormat, tables */
#include "engine/pdistd/T531_random.h"           /* Random, random_int, random_float */
#include "engine/pdistd/T532_heap.h"             /* malloc family */
#include "engine/pdistd/T533_printf.h"           /* printf, tf_printf_PrintTarget */
#include "engine/pdistd/T534_sprintf.h"          /* sprintf */
#include "engine/pdistd/T535_vsprintf.h"         /* vsprintf, tf_vsprintf_PrintTarget */
#include "engine/pdistd/T536_bit_stream_buffer.h" /* BitStreamBuffer */
#include "engine/pdi/G003_pdi1394_rpc.h"      /* printf_to_deci2 (PrintTarget::flush's backend) */

/* T532's data (file statics there: heap_object, .bss 0x004A9EC0, built by
 * its compiler-generated static constructor with best fit; the .sdata heap
 * words heap_base .. heap_limit 0x003531C8..0x003531D4, gp-0x4828..-0x481C)
 * and T531's g_random (.sbss 0x003532E0, gp-0x4710) are defined in their
 * files.  The game also builds other Allocators of its own (callers of
 * Allocator(int) and assign() in T2xx). */

/* ======================================================================== */
/* type_info storage (__user_type_info = 2 words, __si_type_info = 3 words) */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ26pdistd9Allocator[2];                  /* 0x003534F0 gp-0x4500 */
extern u32 __tiQ26pdistd21PrintFormatTargetBase[2];     /* 0x003534F8 gp-0x44F8 */
extern u32 __ti15BitStreamBuffer[2];                    /* 0x00353500 gp-0x44F0 */
/* The (anonymous namespace) PrintTargets' __si_type_info nodes (.bss); their
 * symbols have '$' and '.', so they are spelled like their __tf functions. */
extern u32 ti_printf_PrintTarget[3];    /* 0x004B2B10 __tiQ233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget */
extern u32 ti_vsprintf_PrintTarget[3];  /* 0x004B2B20 __tiQ235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTarget */
}
#endif

#endif /* U125_H */
