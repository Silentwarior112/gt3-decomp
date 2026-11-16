/* T496_font.h - Font, the menu / race font (translation unit T496, unit u109),
 * 0x0023C768..0x0023D3CC, with its text output (T498, 0x0023D470..0x0023D690).
 *
 * Up to 8 loaded font files (FontSet) whose faces (PglFont) are found by name
 * in a 15-bucket dictionary (FontDict), a texture file, the style string
 * "face#palette*sx,sy" (scales in percent), and size / scale / offset /
 * position wrappers around G001's font renderer (fontset_relocate, font_set,
 * ... in G001_flare.h).  One instance: the system font g_002F1968.
 *
 * Not the same thing as u114's `struct Font`, which is G001's PglFont (a face
 * of a font file; u109 called it FontFace). */
#ifndef GAME_T496_FONT_H
#define GAME_T496_FONT_H

#include "game/game.h"
#include <stdarg.h>                             /* va_list (Font::vprint) */

struct PglFont;                                 /* G001_flare.h: a face (u109: FontFace) */
struct FontSet;                                 /* G001_flare.h: a font file (u109: FontFile) */
struct KanjiFontFile;                           /* G001_flare.h: the kanji font file (kanjifile_relocate) */
struct KanjiFont;                               /* G001_flare.h: a record of it (font_set_kanji) */

/* A node of the face dictionary. */
struct FontDictNode {
    FontDictNode *next;                         /* 0x0 */
    const char   *key;                          /* 0x4 the face's name */
    PglFont      *value;                        /* 0x8 */
};
EGCS_SIZE(FontDictNode, 0xC);

/* The face dictionary at Font +0x000: 15 hash buckets (hash_string % 15,
 * T497) and a pool of 40 nodes.  In the original a template instance emitted
 * at the end of T496.  Not polymorphic. */
class FontDict {
public:
    FontDict() : freeList(0) { init(); }        /* inline (expanded in Font::Font) */
    ~FontDict();                                /* 0x0023D138 nothing */
    void     init();                            /* 0x0023D0C0 empty buckets, the pool chained, node 39 first */
    PglFont *lookup(const char *key);           /* 0x0023D160 NULL if absent */
    int      insert(const char *key, PglFont *value); /* 0x0023D1D8 1 new, 0 replaced */
    int      remove(const char *key);           /* 0x0023D2B8 1 removed, 0 absent */

    FontDictNode *buckets[15];                  /* 0x000 */
    FontDictNode *freeList;                     /* 0x03C */
    FontDictNode  nodes[40];                    /* 0x040 */
};
EGCS_SIZE(FontDict, 0x220);

/* Not polymorphic.  Size 0x27C (the static object g_002F1968). */
class Font {
public:
    Font();                                     /* 0x0023C768 (T496_static_init) */
    ~Font();                                    /* 0x0023C800 */
    int  loadFile(const char *path);            /* 0x0023C880 a slot; -2 none free, -1 error */
    void releaseFile(int slot);                 /* 0x0023C9A0 */
    int  loadKanjiFont(const char *path);       /* 0x0023CA70 the kanji font file; 0, -1 if not read */
    void releaseKanjiFont(int unused);          /* 0x0023CAF8 */
    int  setDefaultFace(const char *name);      /* 0x0023CB30 */
    int  setStyle(const char *style);           /* 0x0023CBD0 "face#palette*sx,sy" */
    void setAlign(int align);                   /* 0x0023CDA0 font_set_align (G001's FONT_ALIGN_* bits) */
    void setScaleRaw(f32 w, f32 h);             /* 0x0023CDC0 the renderer's scale (0x1100 / 0x1101) */
    void setSize(f32 w, f32 h);                 /* 0x0023CDF8 */
    void setSpacingRaw(f32 a, f32 b);           /* 0x0023CE40 the renderer's spacing (0x1102 / 0x1103) */
    void setSpacing(f32 a, f32 b);              /* 0x0023CE78 */
    f32  getScaleRawX();                        /* 0x0023CE98 the renderer's scale x (0x1100) */
    f32  getScaleRawY();                        /* 0x0023CEB8 its scale y (0x1101) */
    void setPosRaw(int x, int y);               /* 0x0023CED8 font_locate (integer position; no callers) */
    void setPosRaw(f32 x, f32 y);               /* 0x0023CEF8 */
    void setPos(int x, int y);                  /* 0x0023CF18 */
    void setPos(f32 x, f32 y);                  /* 0x0023CF68 */
    void setPos3(int x, int y, int z);          /* 0x0023CFA8 */
    void setPos3(f32 x, f32 y, f32 z);          /* 0x0023CFE8 */
    void setPos3(const f32 *xyz);               /* 0x0023D010 */
    void setColor(u32 rgba);                    /* 0x0023D030 */
    void setColor(f32 r, f32 g, f32 b, f32 a);  /* 0x0023D050 */
    void setScale(f32 s);                       /* 0x0023D070 */
    void setScale(f32 sx, f32 sy);              /* 0x0023D080 */
    void setOffset(f32 x, f32 y);               /* 0x0023D090 */
    f32  getScaleX();                           /* 0x0023D0A0 */
    f32  getScaleY();                           /* 0x0023D0A8 */
    f32  getSizeX();                            /* 0x0023D0B0 */
    f32  getSizeY();                            /* 0x0023D0B8 */
    /* T498 (font_print) */
    void begin();                               /* 0x0023D470 opens a glyph batch */
    void select(const char *style);             /* 0x0023D510 */
    void flush();                               /* 0x0023D568 closes the batch */
    void putChar(int c);                        /* 0x0023D5A0 */
    void putString(const char *s);              /* 0x0023D5C0 */
    void print(const char *fmt, ...);           /* 0x0023D5E0 */
    void vprint(const char *fmt, va_list ap);   /* 0x0023D618 */

    FontDict  dict;                             /* 0x000 the faces by name */
    FontSet  *files[8];                         /* 0x220 the loaded font files */
    /* The kanji font file (loadKanjiFont: relocated by kanjifile_relocate,
     * malloc'd; u114): its record 0 and its CLUTs go to font_set_kanji (begin). */
    struct KanjiFontFile *kanjiFile;            /* 0x240 */
    PglFont  *face;                             /* 0x244 the current face */
    PglFont  *defaultFace;                      /* 0x248 */
    const struct KanjiFont *kanjiFont;          /* 0x24C the file's record 0 */
    const u32 *kanjiCluts;                      /* 0x250 the file's 16-colour CLUTs (0x40 bytes each) */
    int       palette;                          /* 0x254 the style's "#n" */
    int       active;                           /* 0x258 a glyph batch is open */
    f32       styleScaleX;                      /* 0x25C the style's "*x" (percent) */
    f32       styleScaleY;                      /* 0x260 the style's "*x,y" */
    f32       scaleX;                           /* 0x264 */
    f32       scaleY;                           /* 0x268 */
    f32       offsetX;                          /* 0x26C */
    f32       offsetY;                          /* 0x270 */
    f32       sizeX;                            /* 0x274 */
    f32       sizeY;                            /* 0x278 */
};
EGCS_SIZE(Font, 0x27C);

extern Font g_002F1968;                         /* 0x002F1968 the system font (T496 .data, defined
                                                   there; its static constructor / destructor
                                                   0x0023D3B0 / 0x0023D388 are egcs's) */

#endif /* GAME_T496_FONT_H */
