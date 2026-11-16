/* T261_menu_draw_state.h - the menu drawing state (translation unit T261,
 * unit u073), 0x001D1AE0..0x001D27F0.
 *
 * The menu is laid out on a 640 x 448 virtual screen.  T261 keeps the screen
 * buffer being drawn, the clip rectangle (centre + size, in virtual-screen
 * units) turned into a GS scissor scaled to that buffer (buffer width / 640,
 * height / 448, clamped to it), the drawing offset / scale handed to the 3D
 * library and the system font, a debug message, and two 640 x 224 GSBuffers.
 * C++ linkage (a C++ translation unit; every user declares them so).
 *
 * Its .sdata is 0x00352720..0x0035275B (after T259's 0x00352718, before
 * T272's 0x00352760); the words other units use are declared below with
 * the rest (the plan had put them in T246, T010, T024 and T056). */
#ifndef GAME_T261_MENU_DRAW_STATE_H
#define GAME_T261_MENU_DRAW_STATE_H

#include "game/game.h"

class GSBuffer;                             /* T514 (pointers / extern objects only) */

/* The menus' virtual screen: 640 x 448, y down (a layout item's y is
 * measured up from the bottom: its centre is (x + w / 2, MENU_SCREEN_H -
 * (y + h / 2))); T261 scales it to the buffer drawn (g_00352758).  Integers,
 * so that they serve int and float expressions alike (the sweep merged the
 * local copies of u003, u005, u009, u010, u063, u065, u067, u068, u071, u072,
 * u073).  Here and not in T259_menu_drawable.h (which includes this header):
 * T261 includes no menu class, its object has no .rodata, as retail's. */
enum {
    MENU_SCREEN_W  = 640,
    MENU_SCREEN_H  = 448,
    MENU_SCREEN_CX = 320,                   /* its centre */
    MENU_SCREEN_CY = 224
};

/* Called from outside T261. */
void menu_draw_reset(void);                         /* 0x001D1AE0 the whole state to its defaults (no
                                                       buffer, full clip, scale 1, no message) */
void menu_set_draw_buffer(GSBuffer *buf, GSBuffer *buf2); /* 0x001D1BD8 the buffer drawn; g_00352754 = buf2 (its
                                                       only caller, RenderBase::beginRender, passes buf twice) */
void menu_clip_reset(void);                         /* 0x001D1BE8 the clip rectangle = the whole virtual
                                                       screen (the scissor is not set) */
void menu_clip_x(f32 cx, f32 w);                    /* 0x001D1D78 the horizontal part, then the scissor */
void menu_clip_y(f32 cy, f32 h);                    /* 0x001D1EB0 the vertical part, then the scissor */
void menu_clip_rect(f32 cx, f32 cy, f32 w, f32 h);  /* 0x001D1FF0 a scissor centred on (cx, cy), the clip
                                                       rectangle unchanged */
void menu_clip_column(f32 cx, f32 w);               /* 0x001D2138 menu_clip_rect(cx, 224, w, 448) */
void menu_clip_row(f32 cy, f32 h);                  /* 0x001D2260 menu_clip_rect(320, cy, 640, h) */
void menu_clip_full(void);                          /* 0x001D2398 the scissor = the whole virtual screen */
void menu_clip_box(f32 x, f32 y, f32 w, f32 h);     /* 0x001D2498 a scissor with top-left (x, y) */
int  menu_is_clipped(f32 cx, f32 cy, f32 w, f32 h); /* 0x001D25B0 1 if the rectangle centred on (cx, cy)
                                                       lies wholly outside the clip rectangle */
void menu_apply_transform(void);                    /* 0x001D2648 the drawing scale and offset to the 3D
                                                       library and the system font */
void menu_message(const char *fmt, ...);            /* 0x001D26B0 formats a message for
                                                       menu_draw_message, waits 10 frames (quirk: leaves
                                                       g_00352728 pointing at its stack buffer) */
void menu_draw_message(void);                       /* 0x001D2700 prints it on a cleared screen (the message
                                                       as the format; RenderBase::beginRender) */
/* 0x001D2788 (.dtors 0x0028F238) / 0x001D27C0 (.ctors 0x0028F15C): egcs's
 * static destructor / initialiser of the two buffers g_002CF290 / g_002CF2C8
 * (_GLOBAL_$D$g_002CF290 / _GLOBAL_$I$g_002CF290, T261). */

/* T261's own (no callers outside it). */
void menu_alloc_buffers(void);                      /* 0x001D1B40 (F_001D1B40, no callers) VRAM for the two
                                                       640 x 224 buffers (psm 0) */
void menu_free_buffers(void);                       /* 0x001D1BA8 (F_001D1BA8, no callers) frees it */
void menu_set_scissor(int x, int y, int w, int h);  /* 0x001D1C20 the GS scissor in buffer pixels, clamped
                                                       to the buffer */

/* T261's .sdata (zero-filled; defined in T261), in address order: egcs
 * emits uninitialised globals at the end of the unit in the order of their
 * first declaration, so this order is T261's .sdata layout. */
/* The message menu_draw_message prints, NULL none (only u073). */
extern char     *g_00352728;                /* 0x00352728 gp-0x52C8 */
/* The drawing offset / scale, set by RenderBase::beginRender (T246) every
 * frame from its magnify spring; menu_apply_transform hands them on. */
extern f32       g_0035272C;                /* 0x0035272C gp-0x52C4 224 / magnify - 224: the drawing offset y */
extern f32       g_00352730;                /* 0x00352730 gp-0x52C0 320 / magnify - 320: the drawing offset x */
extern f32       g_00352734;                /* 0x00352734 gp-0x52BC magnify (1 .. 2): the drawing scale */
/* The clip rectangle (only u073). */
extern f32       g_00352738;                /* 0x00352738 gp-0x52B8 clip height (448) */
extern f32       g_0035273C;                /* 0x0035273C gp-0x52B4 clip width (640) */
extern f32       g_00352740;                /* 0x00352740 gp-0x52B0 clip centre y (224) */
extern f32       g_00352744;                /* 0x00352744 gp-0x52AC clip centre x (320) */
/* Nonzero: RenderBase::render (T246) draws nothing (set by
 * PageManager::lockRoot while a page changes, T211, and by T031 when
 * "ar/top" is cancelled). */
extern int       g_00352748;                /* 0x00352748 gp-0x52A8 */
/* While g_00352750 is set RenderBase::beginRender first clears the screen
 * to g_0035274C.  The page manager (T211) and the hooks (T028, ...) set
 * them. */
extern u32       g_0035274C;                /* 0x0035274C gp-0x52A4 the clear colour (RGBA8) */
/* g_00352750 is a bool (egcs's is 4 bytes; u018 / u015: with it and
 * RenderBase::transitionOn bool, T111's / T103's begin are exact); GCC 14
 * keeps an int (the values are 0 / 1). */
#ifdef BOOT_OLD_GCC
extern bool      g_00352750;                /* 0x00352750 gp-0x52A0 nonzero: clear the screen first */
#else
extern int       g_00352750;                /* 0x00352750 gp-0x52A0 nonzero: clear the screen first */
#endif
/* menu_set_draw_buffer's buf2 (? a second buffer); never read;
 * menu_draw_reset clears it (only u073). */
extern GSBuffer *g_00352754;                /* 0x00352754 gp-0x529C */
/* menu_set_draw_buffer sets it. */
extern GSBuffer *g_00352758;                /* 0x00352758 gp-0x5298 the screen buffer being drawn */

/* T261's .data (only u073 uses it). */
extern GSBuffer  g_002CF290;                /* 0x002CF290 a 640 x 224 buffer */
extern GSBuffer  g_002CF2C8;                /* 0x002CF2C8 a 640 x 224 buffer */

#endif /* GAME_T261_MENU_DRAW_STATE_H */
