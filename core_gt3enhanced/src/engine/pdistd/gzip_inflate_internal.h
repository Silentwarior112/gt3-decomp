/* gzip_inflate_internal.h - unit u126 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0026CA90..0x0026E310.
 *
 * Translation units (link order):
 *   T537  GZIP::InflateBase and GZIP::crc32: Polyphony's C++ port of
 *         gzip-1.2.4's inflate.c (bump-allocated Huffman tables in a
 *         0x3000-byte arena inside the object, 64-bit bit buffer, a virtual
 *         input-refill hook, no error checks).  The same machine code as the
 *         bootstrap's src/gzip_inflate.cpp (0x01011700..0x01012F34); the
 *         source and the comments are taken from there.
 *         Canonical header: engine/pdistd/T537_gzip_inflate.h (Huft, InflateState,
 *         InflateBase, crc32, the typeinfo function and node).
 *   T538  a single float helper, fmod_wrap: x mod y (truncating), optionally
 *         wrapped into [0, y) for negative x.  engine/pdistd/T538_fmod_wrap.h.
 *
 * Only T537's data tables are local to the unit (no other unit uses them).
 */
#ifndef U126_H
#define U126_H

#include "types.h"
#include "cxxrt.h"                      /* __rtti_user */
#include "engine/pdistd/T537_gzip_inflate.h"
#include "engine/pdistd/T538_fmod_wrap.h"

namespace GZIP {

/* T537's tables (.data 0x00303608..0x00303B1B, non-const in the original,
 * never written; the .eh_frame block of T537 follows at 0x00303B1C).  Same
 * contents as the bootstrap's (bootstrap src/gzip_inflate.cpp); defined in
 * T537 (the data pass), in this order. */
extern u32 crc_table[256];   /* 0x00303608 reflected CRC-32 table (poly 0xEDB88320) */
extern u16 cplens[31];       /* 0x00303A08 copy lengths for literal codes 257..285 (+2 unused) */
extern u16 cplext[31];       /* 0x00303A48 extra bits for literal codes 257..285 (99 = invalid) */
extern u16 cpdist[32];       /* 0x00303A88 copy offsets for distance codes 0..29 (+2 padding 0) */
extern u16 cpdext[32];       /* 0x00303AC8 extra bits for distance codes (+2 padding 0) */
extern u8  border[19];       /* 0x00303B08 order of the bit length code lengths */

} /* namespace GZIP */

#endif /* U126_H */
