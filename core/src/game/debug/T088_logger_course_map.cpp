/* T088 - the data logger's course map.  0x00171968..0x00171CA8.
 *
 * logger_load_course_map (T055) reads "logger/<course>" into the race
 * pool's 0x20000-byte buffer (RaceMemPool::courseMap) and calls relocate():
 * the file holds the address it was written at (+0x04) and the offset of a
 * Tex1Image (+0x30).  LoggerMapCourse::virtual_12 (T087, u038) draws it
 * with draw(); LoggerGraphRoot::virtual_9 (T087, u038) places the
 * cursor point on it with toMap().
 *
 * Data: retail's T088 has no .rodata (no literal, no type name: T087's type
 * names end with Operator's, 0x00324D10..0x00324D25; T089's vtables start
 * at 0x00324D28),
 * only its frame info (.data 0x002B078C) and the .lit4 constant 0x0034FC94.
 * So it includes only the headers it needs, none with a polymorphic class
 * (no <new>: "bad_alloc" is not there either).
 */
#include "types.h"
#include "game/debug/T088_logger_course_map.h"    /* own translation unit */
#include "engine/pgl/G001_flare.h"                /* gfx_enable / gfx_disable (PGL_CAP_*), gfx_blend_func,
                                               pgl_color4f, gfx_alloc_tail, pgl_draw_strip, PrimVertex,
                                               PrimUV, Tex1Image, tex1_relocate, tex1_select_variation,
                                               gfx_set_texture_regs */
#include "engine/media/G002_mpeg_frameq.h"          /* cosf, sinf */
#include "engine/gfx/T512_render_context.h"       /* g_002F4230.vram (g_002F4250, the VRAM allocator) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 */
#include "game/debug/logger_course_map_internal.h"                           /* LOGGER_BLEND_ALPHA */

/* Moves the file's pointers by (where it is - where it was written). */
// FUNC 0x00171968 LoggerCourseMap::relocate
void LoggerCourseMap::relocate()
{
    int delta = (u8 *)this - (u8 *)base;

    base = this;
    tex = (Tex1Image *)((u8 *)tex + delta);
    tex1_relocate(tex);
}

/* The picture as a w x h quad at (x, y), opaque white, without depth test,
 * fog or alpha test, alpha blended; the texture coordinates cover width x
 * height of the texture's power-of-two size (doubling from 1 while smaller,
 * at most 2048).  The texture is made resident and selected with its
 * variation index + 1 (LoggerMapCourse passes the selected sector). */
// FUNC 0x001719A0 LoggerCourseMap::draw
void LoggerCourseMap::draw(int index, f32 x, f32 y, f32 w, f32 h)
{
    PrimVertex *quad;
    PrimUV *uv;
    int picW, picH, texW, texH;
    f32 uMax, vMax;

    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_FOG);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(LOGGER_BLEND_ALPHA, 1.0f);

    /* a strip of 4 (3 quadwords): top left, top right, bottom left, bottom
     * right; the z words are stored as $zero */
    quad = (PrimVertex *)gfx_alloc_tail(3);
    quad[0].x = x;
    quad[0].y = y;
    quad[0].z = 0.0f;
    quad[1].x = x + w;
    quad[1].y = y;
    quad[1].z = 0.0f;
    quad[2].x = x;
    quad[2].y = y + h;
    quad[2].z = 0.0f;
    quad[3].x = x + w;
    quad[3].y = y + h;
    quad[3].z = 0.0f;

    uv = (PrimUV *)gfx_alloc_tail(2);
    picW = width;
    texW = 1;
    while (texW < 0x401 && texW < picW)
        texW <<= 1;
    uMax = (f32)picW / (f32)texW;
    picH = height;
    texH = 1;
    while (texH < 0x401 && texH < picH)
        texH <<= 1;
    vMax = (f32)picH / (f32)texH;
    uv[0].u = 0.0f;                         /* the zeros are stored as $zero */
    uv[0].v = 0.0f;
    uv[1].u = uMax;
    uv[1].v = 0.0f;
    uv[2].u = 0.0f;
    uv[2].v = vMax;
    uv[3].u = uMax;
    uv[3].v = vMax;

    g_002F4230.vram.loadTex1(tex);          /* g_002F4250 */
    gfx_set_texture_regs(tex->tex);
    tex1_select_variation(tex, index + 1);
    pgl_draw_strip(quad, NULL, NULL, uv, 4);
}

/* World (x, z) -> map pixels from the map's centre: rotated by `rotation'
 * degrees, minus the origin, times scale. */
// FUNC 0x00171BD0 LoggerCourseMap::toMap
void LoggerCourseMap::toMap(f32 *mapX, f32 *mapY, f32 x, f32 z)
{
    f32 angle = rotation * 3.1415927f / 180.0f;
    f32 c = cosf(angle);
    f32 s = sinf(angle);
    f32 t = x;

    x = t * c - z * s;
    z = t * s + z * c;
    x -= originX;
    z -= originZ;
    x *= scale;
    z *= scale;
    *mapX = x;
    *mapY = z;
}
