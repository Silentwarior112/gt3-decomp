/* T261 - the menu drawing state.  0x001D1AE0..0x001D27F0.
 *
 * The menu is laid out on a 640 x 448 virtual screen.  The clip rectangle
 * (g_00352744 / g_00352740 centre, g_0035273C / g_00352738 size) is scaled
 * to the buffer being drawn (g_00352758: width / 640, displayHeight / 448)
 * and becomes the GS scissor (menu_set_scissor, clamped to the buffer).  The
 * "- 0.0f" in the conversions is the original's: a coordinate minus the
 * virtual screen's origin (0, 0), kept by egcs where the coordinate is a
 * variable (x - 0.0 is not folded) and folded where it is a constant.
 *
 * It includes only what it uses, not menu_draw_state_internal.h: no menu class, so its object
 * has no .rodata (no type names), as retail's (T262's names follow T259's
 * at 0x003378F8).  It defines its .sdata and .data. */
#include "types.h"
#include <stddef.h>
#include <stdarg.h>
#include "game/menu/framework/T261_menu_draw_state.h"      /* own declarations, MENU_SCREEN_* */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync, gfx_set_scissor, gs_set_clear_color,
                                               gs_clear_buffers (PGL_CLEAR_COLOR), pgl_translate,
                                               vu0_scale */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer (width, psm, displayHeight, allocVram, freeVram) */
#include "engine/pdistd/T535_vsprintf.h"             /* vsprintf */

/* The two static buffers: 640 x 224 (one field), PSMCT32. */
enum {
    MENU_BUFFER_W   = 640,
    MENU_BUFFER_H   = 224,
    MENU_BUFFER_PSM = 0                     /* SCE_GS_PSMCT32 */
};

/* The two 640 x 224 buffers (.data, zero-filled; built / destroyed by the
 * static initialiser / destructor at the end). */
GSBuffer g_002CF290;                        /* 0x002CF290 */
GSBuffer g_002CF2C8;                        /* 0x002CF2C8 */

/* The drawing state (.sdata, zero: egcs writes these uninitialised globals
 * after the functions, in the order of the header's declarations). */
char     *g_00352728;                       /* 0x00352728 the message */
f32       g_0035272C;                       /* 0x0035272C the drawing offset y */
f32       g_00352730;                       /* 0x00352730 the drawing offset x */
f32       g_00352734;                       /* 0x00352734 the drawing scale */
f32       g_00352738;                       /* 0x00352738 clip height */
f32       g_0035273C;                       /* 0x0035273C clip width */
f32       g_00352740;                       /* 0x00352740 clip centre y */
f32       g_00352744;                       /* 0x00352744 clip centre x */
int       g_00352748;                       /* 0x00352748 nonzero: RenderBase draws nothing */
u32       g_0035274C;                       /* 0x0035274C the clear colour */
#ifdef BOOT_OLD_GCC
bool      g_00352750;                       /* 0x00352750 nonzero: clear the screen first */
#else
int       g_00352750;                       /* 0x00352750 nonzero: clear the screen first */
#endif
GSBuffer *g_00352754;                       /* 0x00352754 menu_set_draw_buffer's buf2 */
GSBuffer *g_00352758;                       /* 0x00352758 the screen buffer being drawn */
/* The scale from the virtual screen to the buffer drawn: buffer width / 640
 * and buffer height / 448 (GSBuffer::displayHeight, a call each time). */
static inline f32 scaleX(void)
{
    return (f32)g_00352758->width / MENU_SCREEN_W;
}

static inline f32 scaleY(void)
{
    return (f32)g_00352758->displayHeight() / MENU_SCREEN_H;
}

/* A virtual-screen coordinate in buffer pixels (before truncation). */
static inline f32 toBufX(f32 x)
{
    return (x - 0.0f) * scaleX();
}

static inline f32 toBufY(f32 y)
{
    return (y - 0.0f) * scaleY();
}

/* A virtual-screen size in buffer pixels (before truncation). */
static inline f32 sizeToBufX(f32 w)
{
    return w * scaleX();
}

static inline f32 sizeToBufY(f32 h)
{
    return h * scaleY();
}

/* The scissor of the rectangle centred on (cx, cy), w x h (virtual screen):
 * the centre and the size are truncated to pixels, then the corner is
 * centre - size / 2 (integer division). */
static inline void scissorCentred(f32 cx, f32 cy, f32 w, f32 h)
{
    int x      = (int)toBufX(cx);
    int y      = (int)toBufY(cy);
    int width  = (int)sizeToBufX(w);
    int height = (int)sizeToBufY(h);
    menu_set_scissor(x - width / 2, y - height / 2, width, height);
}

/* scissorCentred of the stored clip rectangle, one statement at a time.  The
 * globals are the helpers' arguments, read where each statement starts (cy
 * before the first displayHeight call, w after it, h before the second one),
 * as the original does: `g_00352738 * scaleY()' in place reads h after the
 * call (menu_clip_x / _y 96.8 / 96.9%). */
static inline void scissorStored(void)
{
    int x      = (int)toBufX(g_00352744);
    int y      = (int)toBufY(g_00352740);
    int width  = (int)sizeToBufX(g_0035273C);
    int height = (int)sizeToBufY(g_00352738);
    menu_set_scissor(x - width / 2, y - height / 2, width, height);
}

/* Resets the drawing state (21 callers: the menu pages before they run): no
 * buffer, the whole screen as clip rectangle, scale 1 and no offset, the
 * screen cleared to black every frame, no message. */
// FUNC 0x001D1AE0 menu_draw_reset
void menu_draw_reset(void)
{
    g_00352750 = 1;                         /* clear the screen first */
    g_00352744 = MENU_SCREEN_CX;
    g_00352740 = MENU_SCREEN_CY;
    g_0035273C = MENU_SCREEN_W;
    g_00352738 = MENU_SCREEN_H;
    g_00352734 = 1.0f;                      /* the drawing scale */
    g_00352758 = NULL;
    g_00352754 = NULL;
    g_0035274C = 0;                         /* the clear colour: black */
    g_00352730 = 0.0f;                      /* sw $zero: the drawing offset x */
    g_0035272C = 0.0f;                      /* sw $zero: the drawing offset y */
    g_00352728 = NULL;
}

/* No callers in the image: the two 640 x 224 buffers get their VRAM. */
// FUNC 0x001D1B40 menu_alloc_buffers
void menu_alloc_buffers(void)
{
    g_002CF290.width  = MENU_BUFFER_W;
    g_002CF290.height = MENU_BUFFER_H;
    g_002CF290.psm    = MENU_BUFFER_PSM;
    g_002CF290.allocVram();
    g_002CF2C8.width  = MENU_BUFFER_W;
    g_002CF2C8.height = MENU_BUFFER_H;
    g_002CF2C8.psm    = MENU_BUFFER_PSM;
    g_002CF2C8.allocVram();
}

/* No callers in the image: frees the two buffers' VRAM (in reverse order). */
// FUNC 0x001D1BA8 menu_free_buffers
void menu_free_buffers(void)
{
    g_002CF2C8.freeVram();
    g_002CF290.freeVram();
}

/* g_00352754 is never read; the only caller (RenderBase::beginRender, T246)
 * passes the draw buffer for buf2 too. */
// FUNC 0x001D1BD8 menu_set_draw_buffer
void menu_set_draw_buffer(GSBuffer *buf, GSBuffer *buf2)
{
    g_00352758 = buf;
    g_00352754 = buf2;
}

/* The clip rectangle = the whole virtual screen (the scissor is not set). */
// FUNC 0x001D1BE8 menu_clip_reset
void menu_clip_reset(void)
{
    g_00352744 = MENU_SCREEN_CX;
    g_00352740 = MENU_SCREEN_CY;
    g_0035273C = MENU_SCREEN_W;
    g_00352738 = MENU_SCREEN_H;
}

/* The GS scissor (buffer pixels), clamped to the virtual screen's edges in
 * the buffer: the left / top edge (0 x scale, truncated) and the right /
 * bottom edge (640 / 448 x scale). */
// FUNC 0x001D1C20 menu_set_scissor
void menu_set_scissor(int x, int y, int w, int h)
{
    int x1 = x + w;
    int y1 = y + h;
    f32 sx = (f32)g_00352758->width / MENU_SCREEN_W;
    int right  = (int)(sx * MENU_SCREEN_W);
    int left   = (int)(sx * 0.0f);
    int top    = (int)((f32)g_00352758->displayHeight() / MENU_SCREEN_H * 0.0f);
    int bottom = (int)((f32)g_00352758->displayHeight() / MENU_SCREEN_H * MENU_SCREEN_H);

    /* The corner clamped into its own variables (retail keeps x / y in $s5 /
     * $s6 and builds the corner in $a0 / $a1 from left / top: movz); the far
     * edges are clamped in place. */
    int x0 = (x < left) ? left : x;
    int y0 = (y < top) ? top : y;

    if (x1 < left)
        x1 = left;
    if (y1 < top)
        y1 = top;
    if (right < x1)
        x1 = right;
    if (right < x0)
        x0 = right;
    if (bottom < y1)
        y1 = bottom;
    if (bottom < y0)
        y0 = bottom;
    gfx_set_scissor(x0, y0, x1 - x0, y1 - y0);
}

/* Sets the horizontal part of the clip rectangle and applies it. */
// FUNC 0x001D1D78 menu_clip_x
void menu_clip_x(f32 cx, f32 w)
{
    g_00352744 = cx;
    g_0035273C = w;
    scissorStored();                        /* implicit: displayHeight (twice), menu_set_scissor (inline helper) */
}

/* Sets the vertical part of the clip rectangle and applies it. */
// FUNC 0x001D1EB0 menu_clip_y
void menu_clip_y(f32 cy, f32 h)
{
    g_00352740 = cy;
    g_00352738 = h;
    scissorStored();                        /* implicit: displayHeight (twice), menu_set_scissor (inline helper) */
}

/* A scissor for (cx, cy, w, h) without changing the stored clip rectangle. */
// FUNC 0x001D1FF0 menu_clip_rect
void menu_clip_rect(f32 cx, f32 cy, f32 w, f32 h)
{
    scissorCentred(cx, cy, w, h);           /* implicit: displayHeight (twice), menu_set_scissor (inline helper) */
}

/* menu_clip_rect(cx, 224, w, 448): a full-height column. */
// FUNC 0x001D2138 menu_clip_column
void menu_clip_column(f32 cx, f32 w)
{
    scissorCentred(cx, MENU_SCREEN_CY, w, MENU_SCREEN_H); /* implicit: displayHeight (twice),
                                                             menu_set_scissor (inline helper) */
}

/* menu_clip_rect(320, cy, 640, h): a full-width row. */
// FUNC 0x001D2260 menu_clip_row
void menu_clip_row(f32 cy, f32 h)
{
    scissorCentred(MENU_SCREEN_CX, cy, MENU_SCREEN_W, h); /* implicit: displayHeight (twice),
                                                             menu_set_scissor (inline helper) */
}

/* The scissor = the whole virtual screen (by its corner: no centring). */
// FUNC 0x001D2398 menu_clip_full
void menu_clip_full(void)
{
    /* implicit: displayHeight (twice, inline toBufY) */
    int x = (int)toBufX(0.0f);
    int y = (int)toBufY(0.0f);
    int w = (int)toBufX(MENU_SCREEN_W);
    int h = (int)toBufY(MENU_SCREEN_H);
    menu_set_scissor(x, y, w, h);
}

/* A scissor for the rectangle with top-left (x, y), w x h; the far edge is
 * converted as x0 + w * scale, then both edges are truncated. */
// FUNC 0x001D2498 menu_clip_box
void menu_clip_box(f32 x, f32 y, f32 w, f32 h)
{
    f32 left   = toBufX(x);
    f32 top    = toBufY(y);
    f32 right  = left + w * ((f32)g_00352758->width / MENU_SCREEN_W);
    f32 bottom = top + h * ((f32)g_00352758->displayHeight() / MENU_SCREEN_H);
    menu_set_scissor((int)left, (int)top, (int)right - (int)left, (int)bottom - (int)top);
}

/* 1 if the rectangle centred on (cx, cy), w x h, lies completely outside
 * the clip rectangle (left of, right of, above or below it).  The far-side
 * tests are written `>' with the rectangle's edge first: retail computes
 * cx - w / 2 before the clip edge (c.lt.s with the operands swapped). */
// FUNC 0x001D25B0 menu_is_clipped
int menu_is_clipped(f32 cx, f32 cy, f32 w, f32 h)
{
    if (cx + w * 0.5f < g_00352744 - g_0035273C * 0.5f)
        return 1;
    if (cx - w * 0.5f > g_00352744 + g_0035273C * 0.5f)
        return 1;
    if (cy + h * 0.5f < g_00352740 - g_00352738 * 0.5f)
        return 1;
    if (cy - h * 0.5f > g_00352740 + g_00352738 * 0.5f)
        return 1;
    return 0;
}

/* The drawing scale and offset, for the 3D library and the system font. */
// FUNC 0x001D2648 menu_apply_transform
void menu_apply_transform(void)
{
    vu0_scale(g_00352734, g_00352734, 1.0f);
    pgl_translate(g_00352730, g_0035272C, 0.0f);
    g_002F1968.setOffset(g_00352730, g_0035272C);
    g_002F1968.setScale(g_00352734);
}

/* Formats a message, has menu_draw_message show it and waits 10 frames
 * (4 callers, e.g. PageManager's "no such file [%s]", T211).  The buffer
 * size is the frame's: egcs gives 0x401..0x410 bytes of
 * locals this frame (0x4A0, $ra at +0x410), 0x400 would give 0x490.
 * Quirk: g_00352728 is left pointing at this function's stack buffer after
 * it returns (the message only stays valid during the wait). */
// FUNC 0x001D26B0 menu_message
void menu_message(const char *fmt, ...)
{
    char text[0x410];
    va_list args;

    va_start(args, fmt);
    vsprintf(text, fmt, args);
    va_end(args);
    g_00352728 = text;
    gfx_vsync(10);
}

/* Prints the message on a cleared screen (RenderBase::beginRender, T246).
 * Quirk: the message is passed as the format. */
// FUNC 0x001D2700 menu_draw_message
void menu_draw_message(void)
{
    if (g_00352728 != NULL) {
        gs_set_clear_color(0);              /* black */
        gs_clear_buffers(PGL_CLEAR_COLOR);
        g_002F1968.setPos(0x20, 0x60);
        g_002F1968.setStyle("sq_9#1");
        g_002F1968.setColor(0x80808080);    /* grey, alpha 0x80 */
        g_002F1968.setAlign(0);             /* left */
        g_002F1968.print(g_00352728);
    }
}

/* Compiler-generated: egcs's static destructor and initialiser of the two
 * buffers defined at the top (named after the unit's first global), emitted
 * here, after the last function. */
// FUNC 0x001D2788 T261_static_dtor (_GLOBAL_$D$g_002CF290)
//   (.dtors entry 0x0028F238) the two buffers, in reverse order
//   (GSBuffer::~GSBuffer(buf, 2)).
/* implicit: ~GSBuffer */
// FUNC 0x001D27C0 T261_static_init (_GLOBAL_$I$g_002CF290)
//   (.ctors entry 0x0028F15C) GSBuffer() of the two buffers.
/* implicit: GSBuffer */
