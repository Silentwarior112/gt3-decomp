/* T498 - the font's text output (Font's methods, declared in
 * engine/text/T496_font.h): a glyph batch is opened on demand (the face's kanjiFile
 * made resident, the face with its palette and the kanji font selected) and
 * closed by flush().  0x0023D470..0x0023D68C.
 *
 * Data: the frame info only (retail has no .rodata here). */
#include <stdarg.h>                             /* va_list */
#include "engine/pgl/G001_flare.h"                    /* PglFont, font_set, font_set_kanji, font_begin / end,
                                                   font_putc / puts / vprintf */
#include "engine/gfx/T512_render_context.h"           /* g_002F4230 (vram = g_002F4250, unkB70) */
#include "engine/gfx/T513_vram_allocator.h"           /* VramAllocator::loadTex1 */
#include "game/data/car_database_internal.h"
#include "engine/text/T496_font.h"

/* Opens a glyph batch for the current face (nothing without a face or if
 * one is open): the face's Tex1 set resident in VRAM (VramAllocator::
 * loadTex1 of g_002F4250), font_set with the GS registers of kanjiFile
 * `palette` of the set, font_set_kanji with the kanji font, its CLUT
 * `palette` (0x40 bytes each) and the render context's 0x800-word VRAM
 * block, then font_begin. */
// FUNC 0x0023D470 Font::begin
void Font::begin()
{
    if (face == 0 || active != 0)
        return;
    g_002F4250.loadTex1(face->tex);
    font_set(face, &face->tex->tex[palette]);
    font_set_kanji(kanjiFont, kanjiCluts + palette * 0x10, g_002F4230.unkB70);
    font_begin();
    active = 1;
}

/* setStyle(style); an open batch is closed and reopened if the face
 * changed; a batch is opened if none was. */
// FUNC 0x0023D510 Font::select
void Font::select(const char *style)
{
    PglFont *oldFace = face;

    setStyle(style);
    if (active != 0) {
        if (oldFace == face)
            return;
        flush();
    }
    begin();
}

/* Closes the open glyph batch (font_end). */
// FUNC 0x0023D568 Font::flush
void Font::flush()
{
    if (active != 0) {
        font_end();
        active = 0;
    }
}

/* Prints one character (the font is unused; font_putc's result is
 * dropped). */
// FUNC 0x0023D5A0 Font::putChar
void Font::putChar(int c)
{
    font_putc(c);
}

/* Prints a string (the font is unused). */
// FUNC 0x0023D5C0 Font::putString
void Font::putString(const char *s)
{
    font_puts((const u8 *)s);
}

/* printf: $a2..$a7 are spilled next to the stack arguments, vprint. */
// FUNC 0x0023D5E0 Font::print
void Font::print(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
}

/* font_vprintf if there is a face; a batch opened here is closed again. */
// FUNC 0x0023D618 Font::vprint
void Font::vprint(const char *fmt, va_list ap)
{
    int wasActive;

    if (face == 0)
        return;
    wasActive = active;
    if (wasActive == 0)
        begin();
    font_vprintf(fmt, ap);
    if (wasActive == 0)
        flush();
}
