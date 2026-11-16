/* T512 - the render context g_002F4230 (? name) (u111).
 * 0x002430E0..0x00243850.
 *
 * The display mode and screen size, the VRAM allocator (T513, +0x20) and
 * the GS buffers: two 640x224 PSMCT24 (psm 1) frame buffers (one per field),
 * two 640x448 PSMZ24 (psm 0x31) depth buffers and a 640x224 PSMZ24 one, all
 * in one 0x69000-word VRAM block (the second frame buffer at +0x46000, the
 * second PSMZ24 buffer and bufB38 at +0x23000, overlapping the first frame
 * buffer region of the PSMZ24 one), plus a 0x800-word block.
 */
#include "engine/io/file_loader_internal.h"
#include "engine/gfx/T512_render_context.h"
#include "libgraph.h"                         /* SCE_GS_PSMCT24, SCE_GS_PSMZ24 */

#define SCREEN_W            0x280               /* 640 */
#define SCREEN_H            0x1C0               /* 448 */
#define FIELD_H             0xE0                /* 224: one field */
/* PSMs: SCE_GS_PSMCT24, SCE_GS_PSMZ24 (libgraph.h); setField's modes: G001's
 * PGL_FIELD_HALF_CUR (one field per buffer), PGL_FIELD_FRAME_SHIFT (the full
 * frame). */
#define VRAM_BUFFERS_SIZE   0x69000             /* words: all the buffers */
#define VRAM_FRAME1_OFFSET  0x46000             /* the second frame buffer */
#define VRAM_Z1_OFFSET      0x23000             /* the second PSMZ24 buffer and bufB38 */
#define VRAM_SMALL_SIZE     0x800               /* words: unkB70's block */

/* GSBufferPair's current / other buffer (u045's helpers): retail takes the
 * pair's address (this + 0xA50, kept in a register) and reads cur at +0x70
 * from it, the form of an inline accessor (the original's were most likely
 * GSBufferPair members).  Local here, not in T512_render_context.h: an
 * inline function in a widely included header shifts egcs's label numbers,
 * which changes other units' .gcc_except_table (u045's notes). */
static inline GSBuffer *pair_current(GSBufferPair *p) { return &p->buf[p->cur]; }
static inline GSBuffer *pair_other(GSBufferPair *p) { return &p->buf[1 - p->cur]; }
/* GSBufferPair::resetIndex as the original's inline member (init stores
 * cur = 0 through &frame); the constructor's call is not integrated, as
 * the out-of-line copy at the end of the unit shows (resetIndex below). */
static inline void pair_reset(GSBufferPair *p) { p->cur = 0; }
/* The size of a buffer in one inline step: retail stores width and height
 * through one address (cur is not re-read between them). */
static inline void gsbuf_set_size(GSBuffer *b, int w, int h) { b->width = w; b->height = h; }

// FUNC 0x002430E0 RenderContext::setDisplayMode
//   Remembers (inter, omode, ffmd), waits a vsync, gfx_change_display_mode,
//   gfx_set_display_enable(0), and marks both frame buffers not visible.
//   Caller: the render thread (0x001012E0) with (1, 2, 1).
void RenderContext::setDisplayMode(int inter, int omode, int ffmd)
{
    modeA = inter;
    modeB = omode;
    modeC = ffmd;
    gfx_vsync(1);
    gfx_change_display_mode(inter, omode, ffmd);
    gfx_set_display_enable(0);
    pair_current(&frame)->visible = 0;
    pair_other(&frame)->visible = 0;
}

// FUNC 0x00243180 RenderContext::setScreenSize
void RenderContext::setScreenSize(int w, int h)
{
    screenW = w;
    screenH = h;
    gfx_set_screen_size(w, h);
}

// FUNC 0x002431B0 RenderContext::init
//   The VRAM allocator, 640x448, then the buffers (see the top of the
//   file); the current index of frame is reset, unkAC4's is used as it is.
//   Every step goes through buf[cur] / buf[1 - cur] again (the original
//   re-reads cur each time, through the pair's address: pair_current /
//   pair_other).  Caller: game init (0x00102C00).
void RenderContext::init()
{
    vram.init();
    unk18 = 1;
    wide = 0;
    unk14 = 0.5f;
    setScreenSize(SCREEN_W, SCREEN_H);

    pair_reset(&frame);
    int base = vram.alloc(VRAM_BUFFERS_SIZE, NULL);
    vramB74 = base;

    pair_current(&frame)->setAddr(base);
    pair_current(&frame)->psm = SCE_GS_PSMCT24;
    gsbuf_set_size(pair_current(&frame), SCREEN_W, FIELD_H);
    pair_current(&frame)->setField(PGL_FIELD_HALF_CUR);
    pair_current(&frame)->scanMask = 0;

    pair_other(&frame)->setAddr(base + VRAM_FRAME1_OFFSET);
    pair_other(&frame)->psm = SCE_GS_PSMCT24;
    gsbuf_set_size(pair_other(&frame), SCREEN_W, FIELD_H);
    pair_other(&frame)->setField(PGL_FIELD_HALF_CUR);
    pair_other(&frame)->scanMask = 0;

    pair_current(&unkAC4)->setAddr(base);
    pair_current(&unkAC4)->psm = SCE_GS_PSMZ24;
    gsbuf_set_size(pair_current(&unkAC4), SCREEN_W, SCREEN_H);
    pair_current(&unkAC4)->setField(PGL_FIELD_FRAME_SHIFT);
    pair_current(&unkAC4)->scanMask = 0;

    pair_other(&unkAC4)->setAddr(base + VRAM_Z1_OFFSET);
    pair_other(&unkAC4)->psm = SCE_GS_PSMZ24;
    gsbuf_set_size(pair_other(&unkAC4), SCREEN_W, SCREEN_H);
    pair_other(&unkAC4)->setField(PGL_FIELD_FRAME_SHIFT);
    pair_other(&unkAC4)->scanMask = 0;

    /* bufB38 through a pointer (retail: this + 0xB38 in a register), width
     * before psm here */
    GSBuffer *z = &bufB38;
    z->setAddr(base + VRAM_Z1_OFFSET);
    z->width = SCREEN_W;
    z->psm = SCE_GS_PSMZ24;
    z->height = FIELD_H;
    z->setField(PGL_FIELD_HALF_CUR);
    z->scanMask = 0;

    unkB70 = vram.alloc(VRAM_SMALL_SIZE, NULL);
}

// FUNC 0x00243480 RenderContext::term
//   Waits a vsync, gfx_set_display_enable(0), vram.term().  No callers.
void RenderContext::term()
{
    gfx_vsync(1);
    gfx_set_display_enable(0);
    vram.term();
}

/* The render context (.data 0x002F4230, the whole of T512's data: egcs
 * puts a public object that has a constructor in .data). */
RenderContext g_002F4230;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002F4230 (.dtors entry 0x0028F280, .ctors entry 0x0028F1B0), emitted at
 * the end of the unit, the destructor first, and between them RenderContext's
 * implicit constructor, which egcs synthesizes while it compiles the
 * initialiser (the frame info's FDE order, as u105's PadManager).
 * RenderContext has no destructor of its own, so the static destructor
 * runs the members' ones inline: GSBuffer::~GSBuffer of bufB38 (flags 2),
 * unkAC4.buf[1], [0], frame.buf[1], [0] (flags 0), then
 * VramAllocator::~VramAllocator of vram (flags 2). */
// FUNC 0x002434B8 T512_static_dtor (_GLOBAL_$D$setDisplayMode__13RenderContextiii)
/* implicit: ~GSBuffer, ~VramAllocator (the static destructor of g_002F4230: the members) */

// FUNC 0x00243580 RenderContext::RenderContext (__13RenderContext)
/* implicit: the members: VramAllocator::VramAllocator (vram), frame and
 * unkAC4 (their two GSBuffer::GSBuffer, then the call of resetIndex: the
 * inline GSBufferPair constructor), bufB38; the rest is the EH cleanup
 * (~GSBuffer / ~VramAllocator of what was built if a constructor throws) */

// FUNC 0x00243828 T512_static_init (_GLOBAL_$I$setDisplayMode__13RenderContextiii)
/* implicit: RenderContext (the static initialiser of g_002F4230) */

// FUNC 0x00243848 GSBufferPair::resetIndex (resetIndex__12GSBufferPair)
/* inline in engine/gfx/T512_render_context.h (cur = 0): the GSBufferPair
 * constructor's call stays (its body comes after the constructor's), and
 * egcs writes this copy at the end of the unit */
