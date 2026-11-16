/* T158_race_display_draw.cpp - helpers of the race display
 * (0x0019A148..0x0019AB38): textured and plain rectangles of the 2D prim
 * buffer (gfx_alloc_tail, drawn as strips by pgl_draw_strip), the alignment
 * of a position, the time / speed formatters and the rank names.  See
 * game/race/display/T158_race_display_draw.h. */
/* T158's own include set (not race_display_draw_internal.h): retail T158's .sdata has no string
 * before its own (the "" of T157's inline RaceMessageDisplay::clear() would
 * come first through T154 / T157's headers). */
#include "types.h"
#include "libc.h"                           /* strcpy, strncpy */
#include "game/race/display/T158_race_display_draw.h"
#include "engine/pgl/G001_flare.h"                /* Tex1Image, GsTexRegs, gfx_alloc_tail, gfx_set_texture_regs,
                                               pgl_draw_strip */
#include "engine/gfx/T512_render_context.h"       /* g_002F424C (wide), g_002F4250 (VramAllocator) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, RACEDISPLAY_RANK_1ST */

/* A vertex of the 2D prim buffer (T154's RaceDisplayVertex): x, y, z = 0. */
struct RaceDrawVertex {
    f32 x;                                  /* 0x0 */
    f32 y;                                  /* 0x4 */
    int z;                                  /* 0x8 */
};
/* A texture coordinate (T154's RaceDisplayUV). */
struct RaceDrawUV {
    f32 u;                                  /* 0x0 */
    f32 v;                                  /* 0x4 */
};
/* A textured quad of the prim buffer (gfx_alloc_tail(5)): the 4 vertices of
 * a strip, then their 4 UVs (5 qwords). */
struct RaceDisplaySprite {
    RaceDrawVertex v[4];                    /* 0x00 */
    RaceDrawUV     uv[4];                   /* 0x30 */
};
EGCS_SIZE(RaceDisplaySprite, 0x50);

/* The largest page side (the power-of-two loops stop past it). */
#define TEX_PAGE_MAX 0x400

// FUNC 0x0019A148 race_draw_image
//   Draws tex at its own size, aligned at (x, y).  The UVs cover
//   the texture inside its power-of-two page (the page size is doubled
//   while < the size and <= 0x400); the texture is made resident
//   (VramAllocator::loadTex1) and selected.  Callers: RaceDisplayFader::draw,
//   the draw of RaceTimeDisplay, RaceMessageDisplay, RaceABMonitor,
//   RaceSteeringDisplay, RaceTireWearDisplay, RaceReplayModeDisplay.
void race_draw_image(Tex1Image *tex, int alignX, int alignY, f32 x, f32 y)
{
    if (tex != NULL) {
        RaceDisplaySprite *quad = (RaceDisplaySprite *)gfx_alloc_tail(5);
        u64 clamp = tex->tex->clamp;
        int texW = (int)(((clamp >> 14) & 0x3FF) + 1);
        int texH = (int)(((clamp >> 34) & 0x3FF) + 1);
        f32 w = (f32)texW;
        f32 h = (f32)texH;
        f32 left = race_align_posf(x, w, alignX);
        f32 top = race_align_posf(y, h, alignY);
        f32 right = left + w;
        f32 bottom = top + h;
        int pageW, pageH;
        f32 u, v;

        quad->v[0].x = left;
        quad->v[0].y = top;
        quad->v[0].z = 0;
        quad->v[1].x = right;
        quad->v[1].y = top;
        quad->v[1].z = 0;
        quad->v[2].x = left;
        quad->v[2].y = bottom;
        quad->v[2].z = 0;
        quad->v[3].x = right;
        quad->v[3].y = bottom;
        quad->v[3].z = 0;
        pageW = 1;
        while (pageW < TEX_PAGE_MAX + 1 && pageW < texW)
            pageW <<= 1;
        u = w / (f32)pageW;
        pageH = 1;
        while (pageH < TEX_PAGE_MAX + 1 && pageH < texH)
            pageH <<= 1;
        v = h / (f32)pageH;
        quad->uv[0].u = 0.0f;
        quad->uv[0].v = 0.0f;
        quad->uv[1].u = u;
        quad->uv[1].v = 0.0f;
        quad->uv[2].u = 0.0f;
        quad->uv[2].v = v;
        quad->uv[3].u = u;
        quad->uv[3].v = v;
        g_002F4250.loadTex1(tex);
        gfx_set_texture_regs(tex->tex);
        pgl_draw_strip(quad->v, NULL, NULL, quad->uv, 4);
    }
}

// FUNC 0x0019A348 race_draw_image_rect
//   The same, stretched over the w x h rectangle aligned at
//   (x, y) (the prim buffer is taken after the alignment here).  Callers:
//   RaceBackPanel::drawImage, RaceRoundMeterBase::drawMeter,
//   RaceBarMeter::drawMeter, RaceMiniMap::drawMap.
void race_draw_image_rect(Tex1Image *tex, f32 x, f32 y, f32 w, f32 h,
                          int alignX, int alignY)
{
    if (tex != NULL) {
        f32 left = race_align_posf(x, w, alignX);
        f32 right = left + w;
        f32 top = race_align_posf(y, h, alignY);
        RaceDisplaySprite *quad = (RaceDisplaySprite *)gfx_alloc_tail(5);
        f32 bottom = top + h;
        u64 clamp = tex->tex->clamp;
        int texW = (int)(((clamp >> 14) & 0x3FF) + 1);
        int texH = (int)(((clamp >> 34) & 0x3FF) + 1);
        int pageW, pageH;
        f32 u, v;

        quad->v[0].x = left;
        quad->v[0].y = top;
        quad->v[0].z = 0;
        quad->v[1].x = right;
        quad->v[1].y = top;
        quad->v[1].z = 0;
        quad->v[2].x = left;
        quad->v[2].y = bottom;
        quad->v[2].z = 0;
        quad->v[3].x = right;
        quad->v[3].y = bottom;
        quad->v[3].z = 0;
        pageW = 1;
        while (pageW < TEX_PAGE_MAX + 1 && pageW < texW)
            pageW <<= 1;
        u = (f32)texW / (f32)pageW;
        pageH = 1;
        while (pageH < TEX_PAGE_MAX + 1 && pageH < texH)
            pageH <<= 1;
        v = (f32)texH / (f32)pageH;
        quad->uv[0].u = 0.0f;
        quad->uv[0].v = 0.0f;
        quad->uv[1].u = u;
        quad->uv[1].v = 0.0f;
        quad->uv[2].u = 0.0f;
        quad->uv[2].v = v;
        quad->uv[3].u = u;
        quad->uv[3].v = v;
        g_002F4250.loadTex1(tex);
        gfx_set_texture_regs(tex->tex);
        pgl_draw_strip(quad->v, NULL, NULL, quad->uv, 4);
    }
}

// FUNC 0x0019A548 race_fill_rect
//   An untextured w x h rectangle in the current colour
//   (pgl_color): a 4-vertex strip, 3 qwords.  Callers:
//   TimeTrialExaminationMessages::renderWhenDisqualifying,
//   LicenseExaminationMessages::renderWhenPassing / renderWhenDisqualifying,
//   RaceABMonitor::draw.
void race_fill_rect(int x, int y, int w, int h)
{
    RaceDrawVertex *vtx = (RaceDrawVertex *)gfx_alloc_tail(3);
    f32 x0 = (f32)x;
    f32 y0 = (f32)y;
    f32 x1 = (f32)(x + w);
    f32 y1 = (f32)(y + h);

    vtx[0].x = x0;
    vtx[0].y = y0;
    vtx[0].z = 0;
    vtx[1].x = x1;
    vtx[1].y = y0;
    vtx[1].z = 0;
    vtx[2].x = x0;
    vtx[2].y = y1;
    vtx[2].z = 0;
    vtx[3].x = x1;
    vtx[3].y = y1;
    vtx[3].z = 0;
    pgl_draw_strip(vtx, NULL, NULL, NULL, 4);
}

// FUNC 0x0019A608 race_fill_octagon
//   An untextured rectangle with its corners cut by `corner'
//   pixels: an 8-vertex strip (6 qwords), top to bottom.  Caller:
//   RaceBackPanel::drawFrame.
void race_fill_octagon(int x, int y, int w, int h, int corner)
{
    RaceDrawVertex *vtx = (RaceDrawVertex *)gfx_alloc_tail(6);

    vtx[0].x = (f32)(x + corner);
    vtx[0].y = (f32)y;
    vtx[0].z = 0;
    vtx[1].x = (f32)(x + w - corner);
    vtx[1].y = (f32)y;
    vtx[1].z = 0;
    vtx[2].x = (f32)x;
    vtx[2].y = (f32)(y + corner);
    vtx[2].z = 0;
    vtx[3].x = (f32)(x + w);
    vtx[3].y = (f32)(y + corner);
    vtx[3].z = 0;
    vtx[4].x = (f32)x;
    vtx[4].y = (f32)(y + h - corner);
    vtx[4].z = 0;
    vtx[5].x = (f32)(x + w);
    vtx[5].y = (f32)(y + h - corner);
    vtx[5].z = 0;
    vtx[6].x = (f32)(x + corner);
    vtx[6].y = (f32)(y + h);
    vtx[6].z = 0;
    vtx[7].x = (f32)(x + w - corner);
    vtx[7].y = (f32)(y + h);
    vtx[7].z = 0;
    pgl_draw_strip(vtx, NULL, NULL, NULL, 8);
}

// FUNC 0x0019A730 race_align_posf
//   The float alignment of a position: align & 3 RACE_ALIGN_LEFT pos -
//   shift, RACE_ALIGN_RIGHT pos + shift - size, else centred (pos - size /
//   2, no shift); the shift is RACE_WIDE_SHIFT when align & RACE_ALIGN_WIDE
//   and the wide screen mode (g_002F424C) are set.  Callers: T156's
//   race_display_alignf, RaceOdometer::draw,
//   RaceMessageDisplay::draw, race_draw_image(_rect).
f32 race_align_posf(f32 pos, f32 size, int align)
{
    f32 shift = 0.0f;
    f32 half;

    if ((align & RACE_ALIGN_WIDE) && g_002F424C != 0)
        shift = RACE_WIDE_SHIFT;
    half = size * 0.5f;
    align &= 3;
    if (align == RACE_ALIGN_LEFT)
        return pos - shift;
    if (align == RACE_ALIGN_RIGHT)
        return (pos + shift) - size;
    return pos - half;
}

// FUNC 0x0019A790 race_align_pos
//   The integer one: shift RACE_WIDE_SHIFT_INT (movz on g_002F424C), centred
//   is pos - (size >> 1) (arithmetic shift).  The code tests RIGHT first,
//   then `align < 2' and LEFT (the same result for align & 3).  13 callers
//   (T156's race_display_align, the T157 displays, the dials' render_2nd).
int race_align_pos(int pos, int size, int align)
{
    int shift = 0;

    if (align & RACE_ALIGN_WIDE)
        shift = g_002F424C != 0 ? RACE_WIDE_SHIFT_INT : 0;
    switch (align & 3) {
    case RACE_ALIGN_LEFT:
        return pos - shift;
    case RACE_ALIGN_RIGHT:
        return (pos + shift) - size;
    case RACE_ALIGN_CENTER:
    default:
        return pos - (size >> 1);
    }
}

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
/* The 8 dash bytes as one unaligned 64-bit store (egcs's usd: sdl / sdr). */
struct RaceTimeDash {
    u64 text;
} __attribute__((packed));
#endif

// FUNC 0x0019A7F0 race_format_time
//   A time in ms as M'SS.mmm (1 to 3 minute digits; clamped to 999'59.999).
//   < 0 or 359999999 (0x157529FF, 6000 minutes - 1 ms: no time) gives
//   "-'--.---" with the font's dash glyph 0x7F (stored as one 8-byte word,
//   pcpyh / pextlh, then the NUL).  Callers: ExamMessageLine::draw,
//   RaceDisplay::pollCheckPointEvents / virtual_20 / showTimeDiff,
//   RaceTimeDisplay::draw, RaceLapTimesDisplay::draw.
//   The dash word is MMI asm in the original: pcpyh / pextlh are
//   instructions no egcs build emits (cc1 / cc1plus have no template for
//   them), and retail's sequence uses $v0 as a second register beside the
//   result in $v1, which an assembler `dli' macro cannot (the bundled one
//   builds a u64 constant in its destination alone: lui / ori / dsll / ori /
//   dsll / ori, and egcs gives the constant one register).  Two volatile
//   statements (retail has pcpyh before the lui / ori of 0x7F2E277F; plain
//   asm lets them move ahead of it), the u64 stored unaligned (sdl / sdr),
//   then the NUL.  GCC 14: the strcpy (the same 9 bytes).
//   ref/matching/u055/notes.md, "Follow-up (2026-10-17)".
void race_format_time(char *buf, int ms)
{
    char *p;
    int i;

    if (ms < 0 || ms == 359999999) {
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
        /* "\x7F'\x7F\x7F.\x7F\x7F\x7F": halfwords 0x277F, 0x7F7F, 0x7F2E,
         * 0x7F7F.  pcpyh makes 0x7F7F7F7F (the odd halfwords), pextlh
         * interleaves it with 0x7F2E277F (the even ones). */
        u64 dash;

        __asm__ __volatile__("pcpyh %0, %1" : "=r"(dash) : "r"(0x7F7F));
        __asm__ __volatile__("pextlh %0, %1, %2" : "=r"(dash) : "r"(dash), "r"(0x7F2E277F));
        ((RaceTimeDash *)buf)->text = dash;
        buf[8] = '\0';
#else
        strcpy(buf, "\x7F'\x7F\x7F.\x7F\x7F\x7F");
#endif
        return;
    }
    if (ms > 59999999)
        ms = 59999999;
    p = buf + 8;                            /* 1 minute digit */
    if (ms > 5999999)
        p = buf + 10;                       /* 3 */
    else if (ms > 599999)
        p = buf + 9;                        /* 2 */
    *p = '\0';
    p--;
    for (i = 0; i < 3; i++) {               /* the milliseconds */
        *p = (char)(ms % 10 + '0');
        ms /= 10;
        p--;
    }
    p -= 4;
    p[4] = '.';
    p[3] = (char)(ms % 10 + '0');           /* the seconds */
    ms /= 10;
    p[2] = (char)(ms % 6 + '0');
    ms /= 6;
    p[1] = '\'';
    /* the minutes, at least one digit, at most three */
    for (i = 0; i < 3 && (i < 1 || ms != 0); i++) {
        *p = (char)(ms % 10 + '0');
        ms /= 10;
        p--;
    }
}

/* x clamped to lo..hi: max.s then min.s, volatile asm statements as every
 * max.s / min.s of the image behaves (none is in a delay slot; u018, u032,
 * u044).  GCC 14: the C comparisons (the same but for -0.0 against 0.0). */
static inline f32 race_clampf(f32 x, f32 lo, f32 hi)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 t;

    __asm__ __volatile__("max.s %0, %1, %2" : "=f"(t) : "f"(x), "f"(lo));
    __asm__ __volatile__("min.s %0, %1, %2" : "=f"(x) : "f"(t), "f"(hi));
    return x;
#else
    x = x > lo ? x : lo;
    return x < hi ? x : hi;
#endif
}

// FUNC 0x0019A968 race_format_speed
//   "%.1f" of the speed clamped to 0..999.9 (the clamp to 0 is redundant
//   after the test; the float goes to sprintf as a double: fptodp); <= 0
//   or FLT_MAX (0x003520D8) gives "---.-" in the dash glyph (0x003520C8, a
//   6-byte copy).  Caller: RaceSpeedDisplay::draw.
void race_format_speed(char *buf, f32 speed)
{
    /* A 6-byte static: .sdata, gp-relative, copied with its NUL (retail
     * 0x003520C8; an inline copy of a string literal addresses it %hi / %lo). */
    static const char dash[] = "\x7F\x7F\x7F.\x7F";

    if (speed <= 0.0f || speed == 3.40282347e+38f) {
        memcpy(buf, dash, sizeof(dash));
        return;
    }
    speed = race_clampf(speed, 0.0f, 999.9f);   /* the clamp to 0 is redundant */
    sprintf(buf, "%.1f", (double)speed);    /* implicit: fptodp ((double) conversion) */
}

/* The rank names "1st".."12th", set at start-up from the RACEDISPLAY texts
 * (T186's .data g_002C43E8[37..48]): a dynamic initialiser, egcs's
 * _GLOBAL_$I$ function T158_static_init (.ctors entry 0x0028F134) below.
 * A file static: retail has the table in .bss (0x0049C3B0) and T158's .data
 * is its frame info alone; egcs puts a dynamically initialised global in
 * .data (zeros), a static in .bss (u055's data pass). */
static const char *g_0049C3B0[12] = {       /* 0x0049C3B0 */
    g_002C43E8[RACEDISPLAY_RANK_1ST + 0],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 1],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 2],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 3],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 4],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 5],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 6],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 7],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 8],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 9],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 10],
    g_002C43E8[RACEDISPLAY_RANK_1ST + 11],
};

// FUNC 0x0019AA08 race_rank_name
//   "1st".."12th" (g_0049C3B0) for 1..12 (one unsigned compare), else ""
//   (0x003520E0).  Callers: ResultImplementRecords::render,
//   RaceRankDisplay::draw.
const char *race_rank_name(int rank)
{
    if ((u32)(rank - 1) < 12)
        return g_0049C3B0[rank - 1];
    return "";
}

// FUNC 0x0019AA38 race_copy_string
//   Copies src (NULL: "") to dst; size < 0: unbounded, else at most size - 1
//   characters and dst[size - 1] = 0.  Callers:
//   RaceValueDisplayBase::setLabel, RaceMessageDisplay::setText.
void race_copy_string(char *dst, const char *src, int size)
{
    if (src == NULL)
        src = "";
    if (size < 0) {
        strcpy(dst, src);
        return;
    }
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

// FUNC 0x0019AA98 T158_static_init (_GLOBAL_$I$race_draw_image__FP9Tex1Imageiiff)
//   (.ctors entry 0x0028F134) the dynamic initialiser of g_0049C3B0.
/* implicit: the initialiser of g_0049C3B0 */
