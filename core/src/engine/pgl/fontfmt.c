/* fontfmt.c - unit u114, 0x0024D128..0x0024D4F8 (G001, no FDE: C).
 *
 * The end of the text renderer: packet close and the measuring twins of
 * font_putc / font_put_kanji.  The library's printf follows in
 * format_part1.c.
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/pgl_font_internal.h"

// FUNC 0x0024D128 font_flush
// Closes the open sprite run (its REGLIST GIF tag: NLOOP = sprites) and the
// text packet: DMA cnt + VIF FLUSH + VIF DIRECT of its quadwords at the
// context's write pointer, which then moves past the packet.
void font_flush(void)
{
    GsLibState *st = &g_002F72C0;
    GsContext *ctx = GS_SPR_CTX;
    u32 qwc;
    u32 *cur;

    if (st->run != NULL) {
        u32 n = ((u32)((u8 *)st->pktEnd - (u8 *)st->run) >> 5) | GIF_EOP;

        FONT_PUT_RUN_TAG(st->run, n);
        st->run = NULL;
    }
    qwc = (u32)((u8 *)st->pktEnd - (u8 *)st->pkt) >> 4;
    cur = ctx->cur;
    if (qwc != 0) {
        ctx->cur = cur + 4 + qwc * 4;
        cur[0] = DMATAG_CNT(qwc);
        cur[2] = VIF_FLUSH;
        cur[3] = VIF_DIRECT(qwc);
        cur[1] = 0;
    }
    st->pkt = NULL;
    st->pktEnd = NULL;
}

// FUNC 0x0024D1C8 font_measure_kanji
// font_put_kanji without drawing: advances the pen by the kanji cell
// (code is not used).  Returns -1 without a kanji font, else 0.
int font_measure_kanji(int code)
{
    GsLibState *st = &g_002F72C0;
    const PglFont *font = st->font;

    st->lastChar = -1;
    if (st->kfont == NULL)
        return -1;
    st->curX = st->curX + (f32)font->kanjiX * st->scaleX + (f32)font->kanjiW * st->scaleX +
               st->spacingX;
    return 0;
}

// FUNC 0x0024D230 font_measure_char
// font_putc without drawing: the same pen movement (newlines, kerning,
// maxX).  Returns -1 without a font, else 0.  Callers: font_text_width,
// font_apply_align (through fmt_vprintf).
int font_measure_char(int c)
{
    GsLibState *st = &g_002F72C0;
    GsContext *ctx = GS_SPR_CTX;
    const PglFont *font = st->font;
    const FontGlyph *g;
    f32 xScale, yScale;
    int kern;

    if (font == NULL)
        return -1;
    if (c >= 0x80) {
        int hi = c >> 8;
        int lo = c & 0xFF;

        if ((hi & 0xFF) >= FONT_KANJI_FIRST)
            return font_measure_kanji(c & 0x7F7F);
        c = kana_page_lookup(hi & 0x7F, lo);
    }
    xScale = st->scaleX;
    yScale = st->scaleY;
    if (ctx->halfHeight)
        yScale *= 0.5f;
    switch (c) {
    case '\n':
        st->maxX = u114_maxf(st->maxX, st->curX);
        st->curX = st->startX;
        st->curY = st->curY + ((f32)font->lineHeight * yScale + st->spacingY);
        st->lastChar = -1;
        break;
    case '\r':
        st->maxX = u114_maxf(st->maxX, st->curX);
        st->curX = st->startX;
        st->lastChar = -1;
        break;
    default:
        if (c < font->first || font->last < c) {
            st->lastChar = -1;
            return 0;
        }
        g = &font->glyphs[c - font->first];
        kern = 0;
        if (st->lastChar >= 0) {
            const FontGlyph *prev = &font->glyphs[st->lastChar - font->first];

            kern = u114_glyph_kerning(g->left, prev->right, font->kernRows);
        }
        st->curX = st->curX + (f32)(g->bearingX - kern) * xScale + (f32)g->advance * xScale +
                   st->spacingX;
        st->lastChar = c;
        break;
    }
    return 0;
}
