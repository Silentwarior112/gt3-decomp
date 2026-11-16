/* T499 - font metrics of a Font (T496): its current face is Font::face
 * (+0x244), a G001 PglFont.  0x0023D690..0x0023D7E0.  Callers: the race
 * displays (RaceGearDisplay, RaceLapTimesDisplay, ...), gt2k_menu::String. */
#include "engine/io/vol_internal.h"
#include "engine/text/T499_font_metrics.h"

// FUNC 0x0023D690 text_char_width
u8 text_char_width(Font *f)
{
    PglFont *face = f->face;
    if (face == NULL)
        return 0;
    return face->width;
}

// FUNC 0x0023D6B0 text_char_height
u8 text_char_height(Font *f)
{
    PglFont *face = f->face;
    if (face == NULL)
        return 0;
    return face->lineHeight;
}

// FUNC 0x0023D6D0 text_kanji_width
//   No callers.
u8 text_kanji_width(Font *f)
{
    PglFont *face = f->face;
    if (face == NULL)
        return 0;
    return face->kanjiW;
}

// FUNC 0x0023D6F0 text_kanji_height
//   No callers.
u8 text_kanji_height(Font *f)
{
    PglFont *face = f->face;
    if (face == NULL)
        return 0;
    return face->kanjiH;
}

// FUNC 0x0023D710 text_string_width
//   Pass-through to font_measure_width (same arguments).
f32 text_string_width(Font *f, const char *s)
{
    return font_measure_width(f, s);
}

// FUNC 0x0023D730 text_string_height
//   Pass-through to font_measure_height (same arguments).  No callers.
f32 text_string_height(Font *f, const char *s)
{
    return font_measure_height(f, s);
}

// FUNC 0x0023D750 font_measure_width
//   Width of `s' in the font's face (0 without a face).
f32 font_measure_width(Font *f, const char *s)
{
    PglFont *face = f->face;
    if (face == NULL)
        return 0.0f;
    font_set(face, NULL);
    return font_text_width((const u8 *)s);
}

// FUNC 0x0023D798 font_measure_height
//   Height of `s' in the font's face (0 without a face).
f32 font_measure_height(Font *f, const char *s)
{
    PglFont *face = f->face;
    if (face == NULL)
        return 0.0f;
    font_set(face, NULL);
    return font_text_height((const s8 *)s);
}
