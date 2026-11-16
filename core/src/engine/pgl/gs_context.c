/* gs_context.c - small getters / setters of the scratchpad drawing
 * context (G001), 0x00258150..0x002582F8.
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/gs_driver_internal.h"

// FUNC 0x00258150 pgl_set_morph_weight
/* GsContext.morphWeight = weight: the morph weight.  Its caller modelset_run_part
 * (opcode 0x35) draws packet first + k of a morph sequence and passes the
 * fraction towards packet k + 1; vifmodel_send_lighting (u115) unpacks it
 * into VU1 for the vertex formats 1 and 2. */
void pgl_set_morph_weight(f32 weight)
{
    GS_SPR_CTX->morphWeight = weight;
}

// FUNC 0x00258160 gs_get_scanmask_state
/* The gs_set_scanmask_mode mode of the SCANMSK shadow's MSK field: 2 (even
 * rows masked) -> PGL_SCANMASK_EVEN, 3 (odd rows) -> PGL_SCANMASK_ODD, else
 * PGL_SCANMASK_OFF, so a caller can save and restore the mask.  Caller
 * GSBuffer::captureDrawBuffer.  (Cases 0, 2, 3 and a return after the
 * switch: egcs's decision tree for {0, 2, 3} is retail's; a `case 0:
 * default:' label pair builds another.) */
int gs_get_scanmask_state(void)
{
    switch ((s32)(GS_SPR_CTX->gen[GS_GEN_SCANMSK] & 3)) {
    case 0:
        return PGL_SCANMASK_OFF;
    case 2:                             /* SCANMSK.MSK: even rows not drawn */
        return PGL_SCANMASK_EVEN;
    case 3:                             /* odd rows not drawn */
        return PGL_SCANMASK_ODD;
    }
    return PGL_SCANMASK_OFF;            /* 1: (reserved) */
}

// FUNC 0x002581C8 gs_get_fog_color
/* The FOGCOL shadow's FCR | FCG << 8 | FCB << 16 (three byte loads): the
 * gfx_set_fog_color value.  Callers modelset_run_part, gfx_set_fog_color_f,
 * gfx_set_fog_color.  (The byte view written at each use: retail addresses
 * each byte off the context, 0x48..0x4A; a `fogcol' pointer local is a
 * pseudo of its own, ctx + 0x48.) */
u32 gs_get_fog_color(void)
{
    GsContext *ctx = GS_SPR_CTX;

    return ((u8 *)&ctx->gen[GS_GEN_FOGCOL])[0]
         | (((u8 *)&ctx->gen[GS_GEN_FOGCOL])[1] << 8)
         | (((u8 *)&ctx->gen[GS_GEN_FOGCOL])[2] << 16);
}

// FUNC 0x002581F0 pgl_get_fog_constants
/* out = { viewCenter.w, viewScale.w, limits.w }: the VU1 fog constants of
 * gfx_set_fog_params (fog + 0.1, 255 - fog, the fog start), for the
 * smoke particles drawn on the CPU (caller RaceSmoke::draw, u058). */
void pgl_get_fog_constants(f32 *out)
{
    GsContext *ctx = GS_SPR_CTX;

    out[0] = ctx->viewCenter.w;
    out[1] = ctx->viewScale.w;
    out[2] = ctx->limits.w;
}

// FUNC 0x00258218 gs_get_field_mode
/* The gfx_set_field_mode mode of the current field: PGL_FIELD_FRAME unless
 * the display is in frame mode (g_gfx.ffmd == 1); then PGL_FIELD_FRAME_SHIFT
 * if that mode is set (fieldShift), else PGL_FIELD_HALF_ODD for an odd field,
 * PGL_FIELD_HALF_EVEN for an even one.  (bootstrap 0x0100E430) */
int gs_get_field_mode(void)
{
    GsContext *ctx = GS_SPR_CTX;

    if (g_gfx.ffmd != SCE_GS_FRAME)
        return PGL_FIELD_FRAME;
    if (ctx->fieldShift)
        return PGL_FIELD_FRAME_SHIFT;
    return (ctx->field == 0) ? PGL_FIELD_HALF_EVEN : PGL_FIELD_HALF_ODD;
}

// FUNC 0x00258260 gfx_get_draw_buffer
/* The inverse of gfx_set_draw_buffer: tgtA's word address (base << 6),
 * PSM, width, height (stored w, h, address, psm in that order).
 * Caller GSBuffer::captureDrawBuffer.  (base is read through a union, alias
 * set 0: retail keeps its lhu after the stores of *w and *h, which a plain
 * u16 field read, of its own alias set, lets sched1 hoist above them; a
 * bitfield view gives the same.  The original's field was probably such a
 * union or bitfield word; this local view is an exact form, not the
 * original - it reads the same halfword.) */
void gfx_get_draw_buffer(u32 *fbWordAddr, u32 *psm, s32 *w, s32 *h)
{
    GsContext *ctx = GS_SPR_CTX;

    *w = ctx->tgtA.width;
    *h = ctx->tgtA.height;
    *fbWordAddr = (u32)((union { GsRenderTarget t; u16 base; } *)&ctx->tgtA)->base << 6;
    *psm = ctx->tgtA.psm;
}

// FUNC 0x00258290 gfx_get_cull_face
/* Returns GsContext.cullFace (+0x1C4): the culled faces, PGL_CULL_*
 * (gfx_cull_face is glCullFace).  The tyre models save it and swap
 * FRONT / BACK while drawn mirrored.  index.txt calls it gfx_get_packet_ptr
 * from a bootstrap match, but that one reads ctx->cur (+0x118): the match
 * masked the lw offset as a relocation.  Callers RaceTirePacket::draw,
 * MenuTireModel::draw. */
u32 gfx_get_cull_face(void)
{
    return GS_SPR_CTX->cullFace;
}

// FUNC 0x002582A0 gfx_get_draw_area
/* The inverse of gfx_set_draw_area: the window, y and height doubled in
 * field rendering (halfHeight).  Caller GSBuffer::captureDrawBuffer. */
void gfx_get_draw_area(s32 *x, s32 *y, s32 *w, s32 *h)
{
    GsContext *ctx = GS_SPR_CTX;

    *x = ctx->winX;
    *y = ctx->winY;
    *w = ctx->winW;
    *h = ctx->winH;
    if (ctx->halfHeight) {
        *y = *y << 1;
        *h = *h << 1;
    }
}
