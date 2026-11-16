/* game.h - common base of the canonical game headers (include/game/).
 *
 * Every canonical header documents the ORIGINAL layout, the one SCE ee-gcc
 * 2.9-ee-991111b (egcs) produced: member offsets in comments, sizes through
 * EGCS_SIZE().  tools/hdrcheck.py computes each documented offset and size with
 * egcs and compares.  GCC 14 lays polymorphic classes and bool out differently
 * (vptr at +0 instead of after the first polymorphic base's data; bool is 1 byte,
 * egcs makes it 4), so the size checks are compiled only by egcs; a GCC 14 build is
 * consistent with itself.
 *
 * The headers are written in the C++ both compilers accept: egcs's C++98 dialect
 * (no override, = default, = delete, in-class member initialisers, enum class,
 * constexpr; static_assert only at namespace scope, through types.h). */
#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "types.h"

#ifdef BOOT_OLD_GCC
#define EGCS_SIZE(T, n) static_assert(sizeof(T) == (n), #T)
#else
#define EGCS_SIZE(T, n) typedef char BOOT_CAT(egcs_size_unchecked_, __LINE__)
#endif

#endif /* GAME_GAME_H */
