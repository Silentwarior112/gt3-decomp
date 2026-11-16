#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see T536_bit_stream_buffer.h */
#line 3 "src/engine/pdistd/T536_bit_stream_buffer.cpp"
#pragma implementation "src/engine/pdistd/T536_bit_stream_buffer.cpp"
#pragma implementation "T536_bit_stream_buffer.h"
#endif
/* T536_bit_stream_buffer.cpp - BitStreamBuffer: an MSB-first bit reader /
 * writer over a caller-supplied byte buffer.  Used by
 * AutomobileControlRecord::Recorder (replay input recording).
 *
 * Class (egcs layout, vptr after the data members, sizeof 0x14):
 * include/engine/pdistd/T536_bit_stream_buffer.h.  Method names are descriptive
 * (? names): the binary keeps only the class name (RTTI "15BitStreamBuffer"
 * @0x00347790).  setBuffer is the exception: Lupo 2 keeps
 * setBuffer__15BitStreamBufferPv as a linkonce (inline) function, and here
 * its out-of-line copy sits after the non-inline methods, before __tf, where
 * #pragma implementation puts the inline copies (cf. T530's repValid/reset).
 *
 * #pragma implementation (egcs only, first lines): pairs with the #pragma
 * interface of the header, so that this object gets the vtable and the
 * setBuffer copy (the #line names this file: the SCE driver compiles a
 * temporary .ii).
 */
#include "engine/pdistd/pdistd_internal.h"
#include "engine/pdistd/T536_bit_stream_buffer.h"

// FUNC 0x0026C8B8 BitStreamBuffer::BitStreamBuffer
// Only installs the vptr (0x00347780); returns this.  Caller: 0x001F43B0.
BitStreamBuffer::BitStreamBuffer()
{
}

// FUNC 0x0026C8D0 BitStreamBuffer::BitStreamBuffer(u8 *)
// start = buffer, vptr; returns this.  Unreferenced.
BitStreamBuffer::BitStreamBuffer(u8 *buffer)
    : start(buffer)
{
}

// FUNC 0x0026C8E8 BitStreamBuffer::~BitStreamBuffer
// Vtable slot 0 (+0xC): re-installs the vptr; operator delete (0x0027C0C0)
// if flags & 1.  Callers: 0x001F43B0, AutomobileControlRecord::Recorder's
// destructor.
BitStreamBuffer::~BitStreamBuffer()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x0026C918 BitStreamBuffer::rewind
// cur = start, byte = 0, bits = 0 (retail stores bits, cur, byte: sched2's
// order for these statements in member order).
void BitStreamBuffer::rewind()
{
    cur  = start;
    byte = 0;
    bits = 0;
}

// FUNC 0x0026C930 BitStreamBuffer::read
// Reads nbits (MSB first) into a 64-bit result.  `bits` counts the bits left
// in `byte`, whose unread bits are kept left-aligned (each chunk is taken
// from the top and the byte is shifted left).  A new byte is fetched when it
// is used up.  nbits == 0 returns 0 and leaves the state as it was.
u64 BitStreamBuffer::read(int nbits)
{
    u8 *p    = cur;
    u64 v    = 0;
    u64 b    = byte;                 /* lbu; 64-bit (dsllv / dsrlv), masked on store */
    int left = bits;

    while (nbits != 0) {
        int k;

        if (left == 0) {
            b = *p++;
            left = 8;
        }
        k = (left < nbits) ? left : nbits;          /* slt + movz */
        v = (v << k) | (b >> (8 - k));              /* dsllv / dsrlv */
        b = (b << k) & 0xFF;                        /* dsllv + andi 0xFF */
        nbits -= k;
        left  -= k;
    }
    cur  = p;
    byte = (u8)b;
    bits = left;
    return v;
}

// FUNC 0x0026C9B0 BitStreamBuffer::write
// Writes the low nbits of value, MSB first.  The value is first left-aligned
// in 64 bits (value << (64 - nbits)); each step ORs the top bits into the
// free low part of `byte` (value >> (56 + bits)), and a completed byte is
// stored at cur++.  At the end a partial byte is also stored at *cur (cur not
// advanced) so the buffer always holds the bits written so far.
void BitStreamBuffer::write(u64 value, int nbits)
{
    u8 *p      = cur;
    u64 b      = byte;               /* lbu; 64-bit: the or takes `top` untruncated */
    int filled = bits;

    value <<= 64 - nbits;                           /* dsllv (shift mod 64) */
    while (nbits != 0) {
        int room = 8 - filled;
        u64 top  = value >> (filled + 56);          /* dsrlv */
        int k    = (room < nbits) ? room : nbits;   /* slt + movz */

        filled += k;
        b |= top;
        if (filled == 8) {
            *p++ = (u8)b;
            filled = 0;
            b = 0;
        }
        nbits -= k;
        value <<= k;
    }
    if (filled != 0)
        *p = (u8)b;
    cur  = p;
    byte = (u8)b;
    bits = filled;
}

// FUNC 0x0026CA38 BitStreamBuffer::size
// Bytes completed so far: cur - start.  Callers: 0x001F4998, 0x001F4C00.
int BitStreamBuffer::size()
{
    return cur - start;
}

// FUNC 0x0026CA48 BitStreamBuffer::setBuffer
/* inline in engine/pdistd/T536_bit_stream_buffer.h; egcs emits this copy (#pragma
 * implementation above).  start = buffer, nothing else.  Unreferenced. */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0026CA50 __tf15BitStreamBuffer
extern "C" void *__tf15BitStreamBuffer(void)
{
    if (__ti15BitStreamBuffer[0] == 0)
        __rtti_user(__ti15BitStreamBuffer, "15BitStreamBuffer");
    return __ti15BitStreamBuffer;
}
#endif
