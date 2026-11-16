/* T514_gs_buffer.h - GSBuffer, a GS frame / texture buffer descriptor
 * (translation unit T514, unit u112), 0x00244400..0x00244FA8.
 *
 * Address, PSM, size, field mode and display rectangle of a buffer in VRAM.
 * The VRAM comes from the allocator g_002F4250 (owned = 1) or is set from
 * outside (setAddr, captureDrawBuffer, copy: owned = 0).  The name is from the
 * vtable slot names "render(GSBuffer&)" of its users. */
#ifndef GAME_T514_GS_BUFFER_H
#define GAME_T514_GS_BUFFER_H

#include "game/game.h"

struct GsTexRegs;                               /* G001 (pointers only) */
struct Tex1Image;                               /* G001 (pointers only) */

/* Not polymorphic.  Size 0x38 (GSBufferPair::buf[2] stride). */
class GSBuffer {
public:
    GSBuffer();                                 /* 0x00244400 addr -1, no rectangle, field 0 (psm / size left) */
    /* Inline in the original: copy-constructing a GSBuffer (T131, T245,
     * T254) is the call of copy alone, without GSBuffer() first. */
    GSBuffer(const GSBuffer &src) { copy(&src); }
    ~GSBuffer();                                /* 0x00244428 frees the VRAM if owned */
    GSBuffer(int psm, int w, int h);            /* 0x00244488 a w x h buffer of psm with its own VRAM */
    GSBuffer(const GSBuffer *src, int psm);     /* 0x002444D0 like src (size, field, rectangle), of psm, own VRAM */
    GSBuffer *copy(const GSBuffer *src);        /* 0x00244518 every field but owned (cleared): shares src's VRAM; this */
    GSBuffer *blit(GSBuffer *src, int magFilter, int minFilter); /* 0x00244590 draws src stretched over it (TEX1 MMAG /
                                                   MMIN: 0 nearest, 1 linear); this */
    void blitField(GSBuffer *src, int half, f32 t); /* 0x00244680 draws src over it for the current field: two passes
                                                   v-shifted by +-(1 - t), the second blended 50/50 (? deflicker) */
    void setAddr(int a);                        /* 0x00244950 VRAM at a, not owned */
    void setField(int field);                   /* 0x00244960 halfHeight = field is not 0 or 9 (the full-height modes) */
    void setRect(int x, int y, int w, int h);   /* 0x00244988 the display / draw rectangle, hasRect = 1 */
    int  vramSize();                            /* 0x002449A8 the VRAM words it needs */
    int  displayHeight();                       /* 0x002449D8 rectH with a rectangle, else height (x2 for one field) */
    void setTargetB();                          /* 0x00244A00 makes it target B: gfx_set_target_b(addr, psm, width, height) */
    void setDrawBuffer();                       /* 0x00244A30 draws into it (draw buffer, field mode, scan mask, area) */
    void setZBuffer();                          /* 0x00244A98 makes it the depth buffer: gfx_set_depth_buffer(addr, psm) */
    void setTexture();                          /* 0x00244AB8 makes it the current texture */
    void makeTexRegs(GsTexRegs *regs);          /* 0x00244B68 a Tex1 register block that textures from it */
    void setDisplay();                          /* 0x00244C20 shows it if visible, else gfx_set_display_enable(0) */
    void captureDrawBuffer();                   /* 0x00244CA8 describes the current DRAW target (not owned): the inverse
                                                   of setDrawBuffer (gfx_get_draw_buffer / _draw_area) */
    int  allocVram();                           /* 0x00244D30 VRAM for psm / width / height: its address */
    int  allocVram(int size);                   /* 0x00244D60 owned = 1 even if it failed; the old VRAM is kept */
    /* Inline in the original (FlareClass::begin, T159, keeps the buffer's
     * address in a register for the three stores; name and parameter order
     * ?): a w x h buffer of psm with new VRAM (the old is kept). */
    int  allocVram(int newPsm, int w, int h) { width = w; height = h; psm = newPsm; return allocVram(); }
    void freeVram();                            /* 0x00244DB0 */
    void download(void *pixels, int psm);       /* 0x00244E00 reads the whole buffer back as psm (gs_download_image) */
    void upload(const void *pixels, int psm);   /* 0x00244E68 queues pixels into the whole buffer (GIF list) */
    void loadTex1(Tex1Image *img);              /* 0x00244EF0 new VRAM for a Tex1 set, linked, uploads queued */

    int addr;                                   /* 0x00 VRAM word address, -1 none */
    int psm;                                    /* 0x04 pixel storage mode (0 PSMCT32, 1 PSMCT24, 2 PSMCT16, 0x31 PSMZ24) */
    int width;                                  /* 0x08 */
    int height;                                 /* 0x0C */
    int field;                                  /* 0x10 the field mode (gfx_set_field_mode: 0 frame, 1 / 2 by the
                                                   current field, 3 / 4 field 0 / 1, 9 frame shifted by the field;
                                                   captureDrawBuffer: gs_get_field_mode, 0 / 3 / 4 / 9) */
    int scanMask;                               /* 0x14 the scan mask mode (gs_set_scanmask_mode: 0 off, 1 / 2 by
                                                   the field, 3 / 4 even / odd lines masked; gs_get_scanmask_state) */
    int rectX;                                  /* 0x18 the display / draw rectangle (setRect, captureDrawBuffer) */
    int rectY;                                  /* 0x1C */
    int rectW;                                  /* 0x20 */
    int rectH;                                  /* 0x24 */
    /* bool in the original (4 bytes under egcs): copy's loads pass its
     * stores of the int fields (other alias sets).  int under GCC 14, whose
     * bool is one byte, so that the layout stays. */
#ifdef BOOT_OLD_GCC
    bool visible;                               /* 0x28 (? shown by setDisplay; set once drawn) */
    bool owned;                                 /* 0x2C addr was allocated from g_002F4250 */
    bool hasRect;                               /* 0x30 */
    bool halfHeight;                            /* 0x34 field != 0 && field != 9: one field per buffer */
#else
    int visible;                                /* 0x28 (? shown by setDisplay; set once drawn) */
    int owned;                                  /* 0x2C addr was allocated from g_002F4250 */
    int hasRect;                                /* 0x30 */
    int halfHeight;                             /* 0x34 field != 0 && field != 9: one field per buffer */
#endif
};
EGCS_SIZE(GSBuffer, 0x38);

#endif /* GAME_T514_GS_BUFFER_H */
