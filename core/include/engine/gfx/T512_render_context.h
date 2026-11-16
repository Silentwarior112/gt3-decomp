/* T512_render_context.h - the render context g_002F4230 (? name) (translation
 * unit T512, unit u111), 0x002430E0..0x00243850.
 *
 * The display mode and screen size, the VRAM allocator (T513, +0x20) and the
 * GS buffers: two 640x224 PSMCT24 (psm 1) frame buffers (one per field), two
 * 640x448 PSMZ24 (psm 0x31) depth buffers and a 640x224 PSMZ24 one, all in
 * one 0x69000-word VRAM block (the second frame buffer at +0x46000, the
 * second PSMZ24 buffer and bufB38 at +0x23000), plus a 0x800-word block. */
#ifndef GAME_T512_RENDER_CONTEXT_H
#define GAME_T512_RENDER_CONTEXT_H

#include "game/game.h"
#include "engine/gfx/T513_vram_allocator.h"           /* VramAllocator, held by value */
#include "engine/gfx/T514_gs_buffer.h"                /* GSBuffer, held by value */

/* Two GSBuffers and the index of the current one.  Size 0x74 (RenderContext:
 * frame +0xA50, unkAC4 +0xAC4). */
struct GSBufferPair {
    GSBuffer buf[2];                            /* 0x00 */
    int      cur;                               /* 0x70 the current one */

    /* Inline in the original (RenderContext's constructor: the two
     * GSBuffers, then the call of resetIndex). */
    GSBufferPair() { resetIndex(); }
    /* Inline too, its body after the constructor's: egcs keeps the call and
     * writes a copy at the end of T512 (0x00243848, after the static
     * initialiser). */
    void resetIndex() { cur = 0; }              /* 0x00243848 */
};
EGCS_SIZE(GSBufferPair, 0x74);

/* Not polymorphic; no destructor of its own (T512's static destructor runs
 * the members' ones) and no constructor of its own: egcs synthesizes it
 * (0x00243580, the members) while it compiles T512's static initialiser,
 * which calls it.  Size 0xB78. */
class RenderContext {
public:
    /* Remembers (inter, omode, ffmd), waits a vsync,
     * gfx_change_display_mode(inter, omode, ffmd), gfx_set_display_enable(0),
     * both frame buffers not visible.  The render thread: (1, 2, 1). */
    void setDisplayMode(int inter, int omode, int ffmd);    /* 0x002430E0 */
    void setScreenSize(int w, int h);           /* 0x00243180 gfx_set_screen_size */
    void init();                                /* 0x002431B0 vram.init(), 640x448, the buffers (game init) */
    void term();                                /* 0x00243480 */
    /* Inline in the original (? name): GSBuffer's alloc / dealloc calls
     * (T514) keep &g_002F4230 in a register and add 0x20, as an inline
     * member's this. */
    VramAllocator *getVram() { return &vram; }

    int           modeA;                        /* 0x000 gfx_change_display_mode's arguments: inter */
    int           modeB;                        /* 0x004 omode */
    int           modeC;                        /* 0x008 ffmd */
    int           screenW;                      /* 0x00C */
    int           screenH;                      /* 0x010 */
    f32           unk14;                        /* 0x014 0.5: the t of GSBuffer::blitField (? field blend) */
    int           unk18;                        /* 0x018 1 (? frame mode): the other units give it to
                                                   blitField as half and copy the screen with
                                                   setField(unk18 ? 9 : 0) */
    int           wide;                         /* 0x01C (? the 16:9 screen mode is on: GameOptions) */
    VramAllocator vram;                         /* 0x020 the VRAM allocator g_002F4250 */
    GSBufferPair  frame;                        /* 0xA50 the two frame buffers */
    GSBufferPair  unkAC4;                       /* 0xAC4 two PSMZ24 (depth) buffers; the race view is drawn over
                                                   a copy of the current one */
    GSBuffer      bufB38;                       /* 0xB38 g_002F4D68, PSMZ24 640x224 (setZBuffer) */
    int           unkB70;                       /* 0xB70 a 0x800-word VRAM block */
    int           vramB74;                      /* 0xB74 the 0x69000-word VRAM block of the buffers */
};
EGCS_SIZE(RenderContext, 0xB78);

extern RenderContext g_002F4230;                /* 0x002F4230 (T512 .data, defined there) */

/* Members of g_002F4230 that the other units address directly.  Macros, not
 * objects of their own: a separate symbol would hide from the compiler that
 * it is the same memory, and a behaviour build has no fixed addresses. */
#define g_002F424C (g_002F4230.wide)            /* 0x002F424C int: (? the 16:9 screen mode) */
#define g_002F4250 (g_002F4230.vram)            /* 0x002F4250 VramAllocator: "the texture manager" */
#define g_002F4D68 (g_002F4230.bufB38)          /* 0x002F4D68 GSBuffer (given to setZBuffer, 0x00244A98) */

/* 0x002434B8 (.dtors 0x0028F280) / 0x00243828 (.ctors 0x0028F1B0): egcs's
 * static destructor (the members' destructors) and initialiser
 * (RenderContext()) of g_002F4230, compiler-generated (T512 defines the
 * object). */

#endif /* GAME_T512_RENDER_CONTEXT_H */
