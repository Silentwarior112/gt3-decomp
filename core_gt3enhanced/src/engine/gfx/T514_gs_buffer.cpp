/* T514 - GSBuffer: a GS frame / texture buffer descriptor.  VRAM comes from
 * the allocator g_002F4250 (owned = 1) or is set from outside (setAddr,
 * captureDrawBuffer, copy: owned = 0).  The name is from the vtable slot names
 * "render(GSBuffer&)" of its users. */
#include "engine/gfx/vram_internal.h"
#include "engine/gfx/T514_gs_buffer.h"

// FUNC 0x00244400 GSBuffer::GSBuffer
//   No VRAM (addr -1), no rectangle, frame mode, no scan mask.  psm / width
//   / height are left as they are.
GSBuffer::GSBuffer()
{
    field = PGL_FIELD_FRAME;
    addr = -1;
    scanMask = 0;
    visible = 0;
    owned = 0;
    hasRect = 0;
    halfHeight = 0;
}

// FUNC 0x00244428 GSBuffer::~GSBuffer
//   Frees the VRAM it allocated (addr stays as it is); egcs's deleting form
//   calls operator delete when (flags & 1).
GSBuffer::~GSBuffer()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    if (owned != 0)
        g_002F4230.getVram()->dealloc(addr);
}

// FUNC 0x00244488 GSBuffer::GSBuffer(int, int, int)
//   A w x h buffer of psm with its own VRAM.
GSBuffer::GSBuffer(int newPsm, int w, int h)
{
    field = PGL_FIELD_FRAME;
    scanMask = 0;
    hasRect = 0;
    halfHeight = 0;
    psm = newPsm;
    width = w;
    height = h;
    allocVram();
}

// FUNC 0x002444D0 GSBuffer::GSBuffer(const GSBuffer *, int)
//   A buffer like src (size, field mode, rectangle) but of psm, with its
//   own VRAM.
GSBuffer::GSBuffer(const GSBuffer *src, int newPsm)
{
    copy(src);
    psm = newPsm;
    visible = 0;
    allocVram();
}

// FUNC 0x00244518 GSBuffer::copy
//   Copies every field but owned (cleared): the VRAM is shared, not owned.
GSBuffer *GSBuffer::copy(const GSBuffer *src)
{
    addr = src->addr;
    visible = src->visible;
    owned = 0;
    psm = src->psm;
    hasRect = src->hasRect;
    width = src->width;
    halfHeight = src->halfHeight;
    height = src->height;
    rectX = src->rectX;
    rectY = src->rectY;
    rectW = src->rectW;
    rectH = src->rectH;
    field = src->field;
    scanMask = src->scanMask;
    return this;
}

// FUNC 0x00244590 GSBuffer::blit
//   Draws src stretched over the whole buffer, colour 1.0, with the given
//   magnification / minification filters (0 nearest, 1 linear), no alpha
//   test, blending or depth test.  Returns this.
GSBuffer *GSBuffer::blit(GSBuffer *src, int magFilter, int minFilter)
{
    pgl_push_attrib(PGL_ATTRIB_BLIT);
    src->setTexture();
    setDrawBuffer();
    gfx_set_field_mode(PGL_FIELD_FRAME);
    gfx_tex_param(PGL_TEXPARAM_MMAG, magFilter);
    gfx_tex_param(PGL_TEXPARAM_MMIN, minFilter);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gs_draw_textured_sprite_strips_f(0.0f, 0.0f, (f32)width, (f32)height,
                                     0.0f, 0.0f, (f32)src->width, (f32)src->height);
    pgl_pop_attrib();
    return this;
}

// FUNC 0x00244680 GSBuffer::blitField
//   Draws src over the buffer (its rectangle, halved in height for a
//   one-field buffer) in frame mode, no scan mask, linear filtering, no
//   blending / tests / depth writes, white at alpha 0x80.  Then by the
//   current field (gs_get_field_mode):
//     even field (3): two passes, v shifted by +(1 - t) and -(1 - t) texels,
//                     the second blended 50 / 50 over the first;
//     else with half: one pass shifted by half a texel;
//     else:           two passes, v shifted by 1 - (1 - t) and 1 + (1 - t),
//                     the second blended 50 / 50.
//   (? a deflicker / field interpolation filter.)  The x offsets are a zero
//   kept in a register (x + 0.0, -0.0, x - 0.0 in the binary).
void GSBuffer::blitField(GSBuffer *src, int half, f32 t)
{
    const f32 one = 1.0f;
    f32 zero;                               /* a variable: kept in a register */
    int srcW, srcH, dstW, dstH;
    f32 vShift;

    pgl_push_attrib(PGL_ATTRIB_BLIT);
    srcW = src->width;
    srcH = src->height;
    dstW = width;
    dstH = height;
    if (hasRect != 0) {
        dstH = rectH;
        dstW = rectW;
        if (halfHeight != 0)
            dstH = dstH >> 1;
    }
    src->setTexture();
    zero = 0.0f;
    vShift = one - t;
    setDrawBuffer();
    gfx_set_field_mode(PGL_FIELD_FRAME);
    gs_set_scanmask_mode(0);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_FOG);
    gfx_depth_mask(0);
    pgl_color(GS_RGBA_WHITE);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    gfx_set_texture_function(PGL_TFX_MODULATE);
    if (gs_get_field_mode() == PGL_FIELD_HALF_EVEN) {
        f32 x1 = (f32)dstW;
        f32 v1 = (f32)srcH;
        f32 y1, u1;

        gfx_disable(PGL_CAP_BLEND);
        y1 = (f32)dstH;
        u1 = (f32)srcW;
        gs_draw_textured_sprite_strips_f(zero, zero, x1 + zero, y1,
                                         zero, vShift, u1, v1 + vShift);
        gfx_blend_func(PGL_BLEND_FIX, 0.5f);
        gfx_enable(PGL_CAP_BLEND);
        gs_draw_textured_sprite_strips_f(-zero, zero, x1 - zero, y1,
                                         zero, -vShift, u1, v1 - vShift);
    } else if (half != 0) {
        gfx_disable(PGL_CAP_BLEND);
        gs_draw_textured_sprite_strips_f(zero, zero, (f32)dstW, (f32)dstH,
                                         zero, 0.5f, (f32)srcW, (f32)srcH + 0.5f);
    } else {
        f32 x1 = (f32)dstW;
        f32 y1, v1, u1;

        gfx_disable(PGL_CAP_BLEND);
        y1 = (f32)dstH;
        v1 = (f32)srcH + one;
        u1 = (f32)srcW;
        gs_draw_textured_sprite_strips_f(zero, zero, x1 + zero, y1,
                                         zero, one - vShift, u1, v1 - vShift);
        gfx_blend_func(PGL_BLEND_FIX, 0.5f);
        gfx_enable(PGL_CAP_BLEND);
        gs_draw_textured_sprite_strips_f(-zero, zero, x1 - zero, y1,
                                         zero, vShift + one, u1, v1 + vShift);
    }
    pgl_pop_attrib();
}

// FUNC 0x00244950 GSBuffer::setAddr
//   Uses VRAM at a it does not own.
void GSBuffer::setAddr(int a)
{
    addr = a;
    owned = 0;
}

// FUNC 0x00244960 GSBuffer::setField
//   halfHeight = 1 unless the mode is a full-height one (0 or 9).
void GSBuffer::setField(int mode)
{
    field = mode;
    if (mode == PGL_FIELD_FRAME || mode == PGL_FIELD_FRAME_SHIFT)
        halfHeight = 0;
    else
        halfHeight = 1;
}

// FUNC 0x00244988 GSBuffer::setRect
void GSBuffer::setRect(int x, int y, int w, int h)
{
    rectX = x;
    rectY = y;
    rectW = w;
    rectH = h;
    hasRect = 1;
}

// FUNC 0x002449A8 GSBuffer::vramSize
//   The VRAM words the buffer needs (-1 for a psm tex_vram_size does not
//   know).
int GSBuffer::vramSize()
{
    return tex_vram_size(psm, width, height, NULL);
}

// FUNC 0x002449D8 GSBuffer::displayHeight
//   rectH with a rectangle, else height (doubled for a one-field buffer).
int GSBuffer::displayHeight()
{
    if (hasRect != 0)
        return rectH;
    return halfHeight != 0 ? height << 1 : height;
}

// FUNC 0x00244A00 GSBuffer::setTargetB
//   Makes it target B, the secondary render target (effects; cleared by
//   gs_clear_buffers bit 2).
void GSBuffer::setTargetB()
{
    gfx_set_target_b(addr, psm, width, height);
}

// FUNC 0x00244A30 GSBuffer::setDrawBuffer
//   Draws into it: the draw buffer, its field mode and scan mask, and the
//   draw area if it has a rectangle.
void GSBuffer::setDrawBuffer()
{
    gfx_set_draw_buffer(addr, psm, width, height);
    gfx_set_field_mode(field);
    gs_set_scanmask_mode(scanMask);
    if (hasRect != 0)
        gfx_set_draw_area(rectX, rectY, rectW, rectH);
}

// FUNC 0x00244A98 GSBuffer::setZBuffer
//   Makes it the depth buffer.
void GSBuffer::setZBuffer()
{
    gfx_set_depth_buffer(addr, psm);
}

// FUNC 0x00244AB8 GSBuffer::setTexture
//   Makes it the current texture (level 0): the buffer width rounded up to
//   64, the clamp region the whole buffer, clamped in u and v, modulate, no
//   mipmaps.
void GSBuffer::setTexture()
{
    gs_flush_dirty_regs();
    gfx_texflush();
    gfx_set_texture_buffer(0, addr, psm, (width + 0x3F) & ~0x3F);
    gfx_set_texture_size(width, height);
    gfx_set_texture_region(PGL_REGION_U, 0, width - 1);
    gfx_set_texture_region(PGL_REGION_V, 0, height - 1);
    gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_CLAMP);
    gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_CLAMP);
    gfx_tex_param(PGL_TEXPARAM_TFX, PGL_TFX_MODULATE);
    gfx_tex_param(PGL_TEXPARAM_MXL, 0);
}

// FUNC 0x00244B68 GSBuffer::makeTexRegs
//   Fills a Tex1 register block that textures from the buffer, like
//   setTexture (TFX / MXL left at texregs_init's).
void GSBuffer::makeTexRegs(GsTexRegs *regs)
{
    texregs_init(regs);
    texregs_set_buffer(regs, 0, addr, psm, (width + 0x3F) & ~0x3F);
    texregs_set_size(&regs->tex0, width, height);
    texregs_set_clamp_region(regs, PGL_REGION_U, 0, width - 1);
    texregs_set_clamp_region(regs, PGL_REGION_V, 0, height - 1);
    texregs_set_mode(regs, PGL_TEXPARAM_WMS, PGL_WRAP_CLAMP);
    texregs_set_mode(regs, PGL_TEXPARAM_WMT, PGL_WRAP_CLAMP);
}

// FUNC 0x00244C20 GSBuffer::setDisplay
//   Shows the buffer (its rectangle or 0, 0, width, height) if visible,
//   else turns the display off.
void GSBuffer::setDisplay()
{
    if (visible != 0) {
        if (hasRect != 0)
            gfx_set_display_area(rectX, rectY, rectW, rectH);
        else
            gfx_set_display_area(0, 0, width, height);
        gfx_set_display_fb(addr, psm, width, height);
        return;
    }
    gfx_set_display_enable(0);
}

// FUNC 0x00244CA8 GSBuffer::captureDrawBuffer
//   Describes the current draw target (not owned), the inverse of
//   setDrawBuffer: the draw buffer, the field mode for the current field,
//   the scan mask and the draw area.  No callers.
void GSBuffer::captureDrawBuffer()
{
    if (owned != 0)
        g_002F4230.getVram()->dealloc(addr);
    owned = 0;
    gfx_get_draw_buffer((u32 *)&addr, (u32 *)&psm, &width, &height);
    field = gs_get_field_mode();
    scanMask = gs_get_scanmask_state();
    gfx_get_draw_area(&rectX, &rectY, &rectW, &rectH);
    hasRect = 1;
}

// FUNC 0x00244D30 GSBuffer::allocVram()
//   VRAM for psm / width / height; returns its address (the value of
//   allocVram(int) passes through).
int GSBuffer::allocVram()
{
    return allocVram(vramSize());
}

// FUNC 0x00244D60 GSBuffer::allocVram(int)
//   Does not free the old VRAM.  owned = 1 even if the allocation failed
//   (addr -1).
int GSBuffer::allocVram(int size)
{
    int vramAddr = g_002F4230.getVram()->alloc(size, NULL);

    visible = 0;
    addr = vramAddr;
    owned = 1;
    return vramAddr;
}

// FUNC 0x00244DB0 GSBuffer::freeVram
void GSBuffer::freeVram()
{
    if (owned != 0)
        g_002F4230.getVram()->dealloc(addr);
    owned = 0;
    addr = -1;
}

// FUNC 0x00244E00 GSBuffer::download
//   Reads the whole buffer back into pixels as dstPsm (gs_download_image,
//   buffer width = width), after giving back the allocator's released
//   blocks.  Caller: CarIconMaker::virtual_2.
void GSBuffer::download(void *pixels, int dstPsm)
{
    g_002F4250.flushReleased();
    gs_download_image(addr, dstPsm, width, 0, 0, width, height, pixels);
}

// FUNC 0x00244E68 GSBuffer::upload
//   Queues pixels (as srcPsm) into the whole buffer on the GIF list; VIF1
//   waits for it.  No callers.
void GSBuffer::upload(const void *pixels, int srcPsm)
{
    int prevList = gfx_select_list(PGL_LIST_GIF);

    gs_upload_image(addr, srcPsm, width, 0, 0, width, height, pixels);
    gfx_put_irq_marker(PGL_MARK_SIGNAL);
    gfx_select_list(PGL_LIST_VIF1);
    gfx_put_irq_marker(PGL_MARK_WAIT);
    gfx_select_list(prevList);
}

// FUNC 0x00244EF0 GSBuffer::loadTex1
//   New VRAM for a Tex1 set (the old freed if addr >= 0, owned or not),
//   linked and its uploads queued on the GIF list; VIF1 waits for them.  No
//   callers.
void GSBuffer::loadTex1(Tex1Image *img)
{
    int vramAddr;
    int prevList;

    if (addr >= 0)
        g_002F4250.dealloc(addr);
    vramAddr = g_002F4250.alloc(img->vramSize << VRAM_BLOCK_SHIFT, NULL);
    addr = vramAddr;
    owned = 1;
    tex1_set_vram_base(img, vramAddr);
    prevList = gfx_select_list(PGL_LIST_GIF);
    tex1_upload_packet(img);
    gfx_put_irq_marker(PGL_MARK_SIGNAL);
    gfx_select_list(PGL_LIST_VIF1);
    gfx_put_irq_marker(PGL_MARK_WAIT);
    gfx_select_list(prevList);
    gfx_texflush();
}
