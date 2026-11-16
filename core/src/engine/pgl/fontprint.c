/* fontprint.c - unit u114, 0x0024B0D8..0x0024B5B0 (G001, no FDE: C).
 *
 * The text renderer's printf side: font_vprintf (with the alignment pass
 * font_apply_align), the library's sprintf (fmt_sprintf / fmt_vsprintf
 * through sprintf_putc) and the text measuring of the T499 helpers.  An
 * object of its own: after font_putc, whose kerning uses $s1 / $s3 as
 * qwords, egcs saves those registers with sq / lq in every later function
 * of the same file, retail saves them with sd / ld here.
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/pgl_font_internal.h"

// FUNC 0x0024B0D8 font_vprintf
// Prints through the library's printf engine (fmt_vprintf) with font_putc,
// inside a text packet of its own unless one is open; with an alignment the
// pen is first moved (font_apply_align, which formats the text once more to
// measure it: the same va_list value is passed to both).  Caller:
// Font::vprint (T498).
void font_vprintf(const char *fmt, va_list ap)
{
    GsLibState *st = &g_002F72C0;
    u8 wasOpen = st->open;

    if (!wasOpen)
        font_begin();
    if (st->align != 0)
        font_apply_align(st, fmt, ap);
    fmt_vprintf(font_putc, (const u8 *)fmt, ap);
    if (!wasOpen)
        font_end();
}

// FUNC 0x0024B178 font_apply_align
// Moves the pen for the alignment st->align of the text fmt / ap.  With a
// horizontal alignment the text is measured (fmt_vprintf with
// font_measure_char from the pen): its right end is max(curX, maxX), its
// bottom the pen y after it; else only the '\n's of the format string are
// counted for the height.  Right: x -= width, centre: x -= width / 2; top:
// y += ascent, bottom: y += ascent - height, centre: y += ascent - height/2
// (height = the lines' extent + one lineHeight).  The pen, the line start
// and maxX become the new position.
void font_apply_align(GsLibState *st, const char *fmt, va_list ap)
{
    const PglFont *font;
    f32 x = st->curX;
    f32 y = st->curY;
    f32 xEnd, yEnd;
    u32 vAlign;

    if (st->align & FONT_ALIGN_H_MASK) {
        st->startX = x;
        st->startY = y;
        st->maxX = x;
        st->lastChar = -1;
        fmt_vprintf(font_measure_char, (const u8 *)fmt, ap);
        xEnd = st->curX;
        yEnd = st->curY;
        if (xEnd < st->maxX)
            xEnd = st->maxX;
    } else {
        const s8 *s = (const s8 *)fmt;
        f32 yScale, lines;
        int c;

        font = st->font;
        yScale = st->scaleY;
        if (GS_SPR_CTX->halfHeight)
            yScale *= 0.5f;
        xEnd = x;
        lines = 0.0f;
        for (c = *s++; c != 0; c = *s++)
            if (c == '\n')
                lines += 1.0f;
        yEnd = y + ((f32)font->lineHeight * yScale + st->spacingY) * lines;
    }
    switch ((u32)st->align & FONT_ALIGN_H_MASK) {
    case FONT_ALIGN_LEFT:
        break;
    case FONT_ALIGN_RIGHT:
        x -= xEnd - x;
        break;
    case FONT_ALIGN_HCENTER:
        x -= (xEnd - x) * 0.5f;
        break;
    }
    vAlign = st->align & FONT_ALIGN_V_MASK;
    if (vAlign != 0) {
        f32 yScale, height, ascent;

        font = st->font;
        yScale = st->scaleY;
        if (GS_SPR_CTX->halfHeight)
            yScale *= 0.5f;
        height = (yEnd - y) + (f32)font->lineHeight * yScale;
        ascent = (f32)font->ascent * yScale;
        switch (vAlign) {
        case 0:
            break;
        case FONT_ALIGN_TOP:
            y += ascent;
            break;
        case FONT_ALIGN_BOTTOM:
            y += ascent - height;
            break;
        case FONT_ALIGN_VCENTER:
            y += ascent - height * 0.5f;
            break;
        }
    }
    st->curX = x;
    st->curY = y;
    st->startX = x;
    st->startY = y;
    st->maxX = x;
    st->lastChar = -1;
}

// FUNC 0x0024B3A0 sprintf_putc
// Output function of fmt_vsprintf: appends c at g_002F72C0.out.
int sprintf_putc(int c)
{
    GsLibState *st = &g_002F72C0;

    *st->out = c;
    st->out++;
    return 0;
}

// FUNC 0x0024B3C8 fmt_sprintf
// sprintf through the library's printf engine (50 callers).  Returns what
// fmt_vsprintf returns ($v0 is passed through).
int fmt_sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = fmt_vsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}

// FUNC 0x0024B400 fmt_vsprintf
// vsprintf: formats into buf and terminates it.  Returns fmt_vprintf's
// result ($v0 is not touched after the call).
int fmt_vsprintf(char *buf, const char *fmt, va_list ap)
{
    GsLibState *st = &g_002F72C0;
    int n;

    st->out = buf;
    n = fmt_vprintf(sprintf_putc, (const u8 *)fmt, ap);
    *st->out = 0;
    st->out = NULL;
    return n;
}

// FUNC 0x0024B440 font_text_width
// Width of a string as drawn (kerning included, max over its lines): runs
// font_measure_char from (0, 0) and restores the pen state.  Caller:
// font_measure_width (T499).  Saved in the order restored (maxX declared
// first: it gets $f20, the others' live ranges tie with it).
f32 font_text_width(const u8 *s)
{
    GsLibState *st = &g_002F72C0;
    f32 maxX, startX, startY, curX, curY;
    s32 lastChar;
    f32 width;
    int c;

    startX = st->startX;
    startY = st->startY;
    curX = st->curX;
    curY = st->curY;
    maxX = st->maxX;
    lastChar = st->lastChar;
    st->lastChar = -1;
    st->startX = 0;
    st->startY = 0;
    st->curX = 0.0f;
    st->curY = 0;
    st->maxX = 0.0f;
    while ((c = *s++) != 0) {
        if (c & 0x80)
            c = (c << 8) + *s++;
        font_measure_char(c);
    }
    width = st->curX;
    if (width < st->maxX)
        width = st->maxX;
    st->startX = startX;
    st->startY = startY;
    st->curX = curX;
    st->curY = curY;
    st->maxX = maxX;
    st->lastChar = lastChar;
    return width;
}

// FUNC 0x0024B538 font_text_height
// Height of a string: lines * (lineHeight * scaleY + spacingY) less the
// last spacing (no field-rendering halving here).  Caller: font_measure_height
// (T499).
f32 font_text_height(const s8 *s)
{
    GsLibState *st = &g_002F72C0;
    f32 lines = 0.0f;
    f32 lineHeight = (f32)st->font->lineHeight * st->scaleY;
    int c;

    for (c = *s++; c != 0; c = *s++)
        if (c == '\n')
            lines += 1.0f;
    return lineHeight + (lineHeight + st->spacingY) * lines;
}
