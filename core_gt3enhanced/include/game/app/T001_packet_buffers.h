/* T001_packet_buffers.h - PacketBuffers, the double GS packet buffer the
 * render thread alternates between (translation unit T001, unit u001),
 * 0x00101290..0x001012E0.
 *
 * The one object g_0028F550 (.data) points at two 0xA0000-byte .bss buffers
 * of this unit, 0x00353700 and 0x003F3700 (file-local).  Each frame the
 * render thread (T002) flips cur and hands buf[cur] to
 * gfx_set_packet_buffer (G001). */
#ifndef GAME_T001_PACKET_BUFFERS_H
#define GAME_T001_PACKET_BUFFERS_H

#include "game/game.h"

/* Not polymorphic.  Size 0x10. */
class PacketBuffers {
public:
    PacketBuffers();                        /* 0x00101290 the two buffers, size 0xA0000, cur 0 */

    void *buf[2];                           /* 0x00 */
    int   size;                             /* 0x08 bytes of each (0xA0000) */
    int   cur;                              /* 0x0C the buffer of the frame being built */
};
EGCS_SIZE(PacketBuffers, 0x10);

extern PacketBuffers g_0028F550;            /* 0x0028F550 */

/* 0x001012C0 (.ctors 0x0028F024): egcs's static initialiser of g_0028F550
 * (_GLOBAL_$I$__13PacketBuffers, T001), PacketBuffers() on it. */

#endif /* GAME_T001_PACKET_BUFFERS_H */
