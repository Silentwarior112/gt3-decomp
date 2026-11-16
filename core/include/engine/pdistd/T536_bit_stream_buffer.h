/* T536_bit_stream_buffer.h - BitStreamBuffer (translation unit T536, unit u125). */
#ifndef GAME_T536_BIT_STREAM_BUFFER_H
#define GAME_T536_BIT_STREAM_BUFFER_H

#include "game/game.h"

#if defined(__cplusplus) && defined(BOOT_OLD_GCC)
/* The original paired this header with #pragma implementation in T536's
 * source (as pdistd/allocator.h with allocator.cxx): the vtable and the
 * out-of-line copy of the inline setBuffer (0x0026CA48, unreferenced, at the
 * end of T536 before __tf) are emitted only there. */
#pragma interface
#endif

/* MSB-first bit reader / writer over a caller-supplied byte buffer (? method
 * names, except setBuffer: the binary keeps only the class name; Lupo 2 has
 * setBuffer__15BitStreamBufferPv as a linkonce = inline member).  User:
 * AutomobileControlRecord::Recorder (replay input recording, T3xx), which
 * embeds one at +0x00 (its next member is at +0x14).
 * vtable _vt$15BitStreamBuffer 0x00347780 (1 slot), vptr +0x10 (egcs).
 * typeinfo __tf15BitStreamBuffer 0x0026CA50, RTTI name 0x00347790,
 * __user_type_info 0x00353500. */
class BitStreamBuffer {
public:
    BitStreamBuffer();                          /* 0x0026C8B8 */
    BitStreamBuffer(u8 *buffer);                /* 0x0026C8D0 start = buffer (unreferenced) */
    virtual ~BitStreamBuffer();                 /* [0] +0x0C 0x0026C8E8 */

    void rewind();                              /* 0x0026C918 cur = start, byte = bits = 0 */
    u64  read(int nbits);                       /* 0x0026C930 */
    void write(u64 value, int nbits);           /* 0x0026C9B0 */
    int  size();                                /* 0x0026CA38 cur - start */
    /* 0x0026CA48 - inline (definition below); Lupo 2: setBuffer(void *). */
    void setBuffer(void *buffer);

    u8 *start;                                  /* 0x00 buffer start */
    u8 *cur;                                    /* 0x04 next byte to read / write */
    u8  byte;                                   /* 0x08 the partial byte */
    int bits;                                   /* 0x0C read: bits left in byte; write: bits filled */
    /*  vptr                                       0x10 (egcs) */
};
EGCS_SIZE(BitStreamBuffer, 0x14);

/* start = buffer, nothing else (out-of-line copy 0x0026CA48, unreferenced). */
inline void BitStreamBuffer::setBuffer(void *buffer)
{
    start = (u8 *)buffer;
}

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tf15BitStreamBuffer(void);           /* 0x0026CA50 */
#endif

#endif /* GAME_T536_BIT_STREAM_BUFFER_H */
