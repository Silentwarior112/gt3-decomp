/* T496 - the menu / race font (Font): up to 8 loaded font-set files whose
 * faces (G001 PglFont) are found by name in a 15-bucket dictionary, the
 * kanji font file, the style string "face#palette*sx,sy" (scales in
 * percent), size / scale / offset / position wrappers around G001's font
 * renderer; the global system font g_002F1968.  The dictionary (FontDict)
 * comes last (in the original a template instance emitted at the end of the
 * unit).  0x0023C768..0x0023D3CC.
 *
 * Data: no .rodata (retail has none: the headers write no type name or
 * string); .data g_002F1968 then the frame info; .lit4 setStyle's 0.01f
 * (the percent scales). */
#include "engine/pgl/G001_flare.h"                    /* PglFont, FontSet, KanjiFontFile, font_*, pgl_color */
#include "runtime/newlib/G007_stdio.h"                    /* atoi, memset, strcmp */
#include "libc.h"                               /* strlen */
#include "engine/io/T503_vol_thread.h"               /* file_read_blocking, vol_device_object */
#include "engine/pdistd/T532_heap.h"                     /* free */
#include "game/data/car_database_internal.h"                               /* hash_string, FONT_PARAM_* */
#include "engine/text/T496_font.h"

/* FontDict's inline constructor (free list = 0, then FontDict::init), the
 * eight file slots zeroed (from the last), then the fields: no kanji file,
 * no face, palette 0, no batch, every scale and size 1.0, offset 0.
 * Caller: T496_static_init. */
// FUNC 0x0023C768 Font::Font
Font::Font()
{
    int i;

    /* implicit: dict: FontDict() inline, its FontDict::init call */
    for (i = 7; i >= 0; i--)
        files[i] = 0;
    kanjiFile = 0;
    face = 0;
    defaultFace = 0;
    kanjiFont = 0;
    kanjiCluts = 0;
    palette = 0;
    active = 0;
    styleScaleX = 1.0f;
    styleScaleY = 1.0f;
    scaleX = 1.0f;
    scaleY = 1.0f;
    offsetX = 0;
    offsetY = 0;
    sizeX = 1.0f;
    sizeY = 1.0f;
}

/* Releases the eight file slots and frees the kanji font file. */
// FUNC 0x0023C800 Font::~Font
Font::~Font()
{
    /* implicit: ~FontDict, operator delete (compiler-generated: bases, members, deleting form) */
    int slot;

    for (slot = 0; slot < 8; slot++)
        releaseFile(slot);
    free(kanjiFile);
    /* then the dictionary's destructor FontDict::~FontDict (flags 2), and
     * operator delete if flags & 1 */
}

/* Loads a font-set file into the first free slot (gt3.vol, blocking) and
 * adds its faces to the dictionary (a name already there keeps its face).
 * Returns the slot, -2 if all 8 are used, -1 if the file cannot be read or
 * one of its faces is NULL (the slot then stays taken).  The count and the
 * faces are read through the loaded buffer (fontset_relocate works in
 * place). */
// FUNC 0x0023C880 Font::loadFile
int Font::loadFile(const char *path)
{
    int slot;
    FontSet *file;
    int i;

    for (slot = 0; slot < 8; slot++)
        if (files[slot] == 0)
            break;
    if (slot == 8)
        return -2;
    file = (FontSet *)file_read_blocking(path, 0, 0, 1, &vol_device_object);
    if (file == 0)
        return -1;
    files[slot] = fontset_relocate(file);
    for (i = 0; i < file->count; i++) {
        PglFont *font = file->fonts[i];

        if (font == 0)
            return -1;
        if (dict.lookup(font->name) == 0)
            dict.insert(font->name, font);
    }
    return slot;
}

/* Releases a slot (0..7, unsigned check): removes its faces from the
 * dictionary (those still pointing at them), frees the file, clears the
 * slot. */
// FUNC 0x0023C9A0 Font::releaseFile
void Font::releaseFile(int slot)
{
    if ((u32)slot < 8) {
        FontSet *file = files[slot];

        if (file != 0) {
            int i;

            for (i = 0; i < file->count; i++) {
                PglFont *font = file->fonts[i];
                PglFont *found = dict.lookup(font->name);

                if (found == font)
                    dict.remove(found->name);
            }
            free(file);
        }
        files[slot] = 0;
    }
}

/* Loads the kanji font file (replacing the previous one): relocated in place
 * by kanjifile_relocate; its record 0 and its CLUTs are kept for begin().  0, or -1
 * if it cannot be read. */
// FUNC 0x0023CA70 Font::loadKanjiFont
int Font::loadKanjiFont(const char *path)
{
    KanjiFontFile *file =
        (KanjiFontFile *)file_read_blocking(path, 0, 0, 1, &vol_device_object);

    if (file == 0)
        return -1;
    if (kanjiFile != 0)
        releaseKanjiFont(0);
    kanjiFile = kanjifile_relocate(file);
    kanjiFont = file->fonts[0];
    kanjiCluts = file->cluts;
    return 0;
}

/* Frees the kanji font file (the argument is unused). */
// FUNC 0x0023CAF8 Font::releaseKanjiFont
void Font::releaseKanjiFont(int unused)
{
    free(kanjiFile);
    kanjiFile = 0;
    kanjiFont = 0;
    kanjiCluts = 0;
}

/* The default face = the face `name'; 0, or -1 if there is none (the
 * default then becomes NULL). */
// FUNC 0x0023CB30 Font::setDefaultFace
int Font::setDefaultFace(const char *name)
{
    PglFont *font = dict.lookup(name);

    defaultFace = font;
    return font != 0 ? 0 : -1;
}

/* Copies src into dst up to NUL, '#', '*' or ','; returns the stopping
 * character's address.  Caller: setStyle. */
// FUNC 0x0023CB68 style_token
static const char *style_token(char *dst, const char *src)
{
    char c = *src++;

    while (c != 0 && c != '#' && c != '*' && c != ',') {
        *dst++ = c;
        c = *src++;
    }
    *dst = 0;
    return src - 1;
}

/* Selects a style: "face", then "#n" (palette n, negative -> 0) and / or
 * "*sx" or "*sx,sy" (scales in percent, multiplied in).  NULL selects the
 * default face.  The size is re-applied with the new scales.  Returns 0,
 * or -1 if the face is unknown (the default face is then used).  Quirk: a
 * '#' at the very end makes the loop read past the terminator. */
// FUNC 0x0023CBD0 Font::setStyle
int Font::setStyle(const char *style)
{
#ifdef BOOT_OLD_GCC
    char number[0x10];                  /* retail's frame: 0x10 bytes of locals, then 0x20
                                           unused bytes egcs keeps with the alloca */
#else
    char number[0x30];                  /* a longer token lands in those 0x20 bytes in retail */
#endif
    char *name;
    const char *p;
    char c;
    PglFont *font;

    palette = 0;
    styleScaleX = 1.0f;
    styleScaleY = 1.0f;
    if (style == 0) {
        face = defaultFace;
        setSize(sizeX, sizeY);
        return 0;
    }
    name = (char *)__builtin_alloca(strlen(style) + 1);
    p = style_token(name, style);
    c = *p++;
    while (c != 0) {
        switch (c) {
        case '#':
            c = *p++;
            palette = c - '0';
            if (palette < 0)
                palette = 0;
            break;
        case '*': {
            int percentX, percentY;

            p = style_token(number, p);
            percentX = atoi(number);
            if (*p == ',') {
                p = style_token(number, p + 1);
                percentY = atoi(number);
            } else {
                percentY = percentX;
            }
            styleScaleX = styleScaleX * ((f32)percentX * 0.01f);
            styleScaleY = styleScaleY * ((f32)percentY * 0.01f);
            break;
        }
        }
        c = *p++;
    }
    setSize(sizeX, sizeY);
    font = dict.lookup(name);
    if (font == 0) {
        face = defaultFace;
        return -1;
    }
    face = font;
    return 0;
}

/* The renderer's alignment (G001 font_set_align: bits 0-1 horizontal, 2-3
 * vertical); the font is unused. */
// FUNC 0x0023CDA0 Font::setAlign
void Font::setAlign(int align)
{
    font_set_align(align);
}

/* The renderer's character scale (font parameters 0x1100 / 0x1101). */
// FUNC 0x0023CDC0 Font::setScaleRaw
void Font::setScaleRaw(f32 w, f32 h)
{
    font_set_param(FONT_PARAM_SCALE_X, w);
    font_set_param(FONT_PARAM_SCALE_Y, h);
}

/* The character size: stored, then applied times scale and style scale. */
// FUNC 0x0023CDF8 Font::setSize
void Font::setSize(f32 w, f32 h)
{
    sizeX = w;
    sizeY = h;
    setScaleRaw(w * scaleX * styleScaleX, h * scaleY * styleScaleY);
}

/* The renderer's character spacing (font parameters 0x1102 / 0x1103). */
// FUNC 0x0023CE40 Font::setSpacingRaw
void Font::setSpacingRaw(f32 a, f32 b)
{
    font_set_param(FONT_PARAM_SPACING_X, a);
    font_set_param(FONT_PARAM_SPACING_Y, b);
}

/* The character spacing (not scaled). */
// FUNC 0x0023CE78 Font::setSpacing
void Font::setSpacing(f32 a, f32 b)
{
    setSpacingRaw(a, b);
}

/* The renderer's scale x (the font is unused). */
// FUNC 0x0023CE98 Font::getScaleRawX
f32 Font::getScaleRawX()
{
    return font_get_param(FONT_PARAM_SCALE_X);
}

/* The renderer's scale y.  No callers. */
// FUNC 0x0023CEB8 Font::getScaleRawY
f32 Font::getScaleRawY()
{
    return font_get_param(FONT_PARAM_SCALE_Y);
}

/* font_locate (an integer position, no offset / scale).  No callers. */
// FUNC 0x0023CED8 Font::setPosRaw
void Font::setPosRaw(int x, int y)
{
    font_locate(x, y);
}

/* The renderer's print position. */
// FUNC 0x0023CEF8 Font::setPosRaw
void Font::setPosRaw(f32 x, f32 y)
{
    font_locate_f(x, y);
}

/* The print position: (pos + offset) * scale. */
// FUNC 0x0023CF18 Font::setPos(int,int)
void Font::setPos(int x, int y)
{
    setPosRaw(((f32)x + offsetX) * scaleX, ((f32)y + offsetY) * scaleY);
}

/* The same from floats. */
// FUNC 0x0023CF68 Font::setPos(float,float)
void Font::setPos(f32 x, f32 y)
{
    setPosRaw((x + offsetX) * scaleX, (y + offsetY) * scaleY);
}

/* A 3D print position (no offset / scale; font_locate_3d's result is
 * dropped). */
// FUNC 0x0023CFA8 Font::setPos3(int,int,int)
void Font::setPos3(int x, int y, int z)
{
    f32 pos[3] = { (f32)x, (f32)y, (f32)z };

    font_locate_3d(pos);
}

/* The same from floats. */
// FUNC 0x0023CFE8 Font::setPos3(float,float,float)
void Font::setPos3(f32 x, f32 y, f32 z)
{
    f32 pos[3] = { x, y, z };

    font_locate_3d(pos);
}

/* The same from an array.  No callers. */
// FUNC 0x0023D010 Font::setPos3(const float*)
void Font::setPos3(const f32 *xyz)
{
    font_locate_3d(xyz);
}

/* The colour (RGBA8, pgl_color: the vertex colour of untextured
 * primitives and of the glyphs); the font is unused. */
// FUNC 0x0023D030 Font::setColor(u32)
void Font::setColor(u32 rgba)
{
    pgl_color(rgba);
}

/* The colour from floats. */
// FUNC 0x0023D050 Font::setColor(float,float,float,float)
void Font::setColor(f32 r, f32 g, f32 b, f32 a)
{
    pgl_color4f(r, g, b, a);
}

/* One scale for x and y (not re-applied to the size). */
// FUNC 0x0023D070 Font::setScale(float)
void Font::setScale(f32 s)
{
    scaleX = s;
    scaleY = s;
}

/* The scale (+0x264 / +0x268). */
// FUNC 0x0023D080 Font::setScale(float,float)
void Font::setScale(f32 sx, f32 sy)
{
    scaleX = sx;
    scaleY = sy;
}

/* The offset (+0x26C / +0x270) added to the positions. */
// FUNC 0x0023D090 Font::setOffset
void Font::setOffset(f32 x, f32 y)
{
    offsetX = x;
    offsetY = y;
}

// FUNC 0x0023D0A0 Font::getScaleX
f32 Font::getScaleX()
{
    return scaleX;
}

// FUNC 0x0023D0A8 Font::getScaleY
f32 Font::getScaleY()
{
    return scaleY;
}

// FUNC 0x0023D0B0 Font::getSizeX
f32 Font::getSizeX()
{
    return sizeX;
}

// FUNC 0x0023D0B8 Font::getSizeY
f32 Font::getSizeY()
{
    return sizeY;
}

/* Clears the buckets and the node pool, chains the pool (node i -> node
 * i - 1) and makes node 39 the free-list head. */
// FUNC 0x0023D0C0 FontDict::init
void FontDict::init()
{
    u32 i;

    memset(buckets, 0, sizeof(buckets));
    memset(nodes, 0, sizeof(nodes));
    for (i = 1; i < 40; i++)
        nodes[i].next = &nodes[i - 1];
    freeList = &nodes[39];
}

// FUNC 0x0023D138 FontDict::~FontDict
FontDict::~FontDict()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* nothing but operator delete if flags & 1 (Font's destructor passes 2) */
}

/* The face of `key', NULL if absent. */
// FUNC 0x0023D160 FontDict::lookup
PglFont *FontDict::lookup(const char *key)
{
    FontDictNode *node = buckets[hash_string(key) % 15];

    for (; node != 0; node = node->next) {
        if (strcmp(node->key, key) == 0)
            return node->value;
    }
    return 0;
}

/* Sets key -> value: replaces the value of an existing key (0), else takes
 * a node from the free list (not checked for exhaustion) and appends it to
 * the bucket (1). */
// FUNC 0x0023D1D8 FontDict::insert
int FontDict::insert(const char *key, PglFont *value)
{
    u32 bucket = hash_string(key) % 15;
    FontDictNode *last = 0;
    FontDictNode *node;

    for (node = buckets[bucket]; node != 0; node = node->next) {
        if (strcmp(node->key, key) == 0) {
            node->value = value;
            return 0;
        }
        last = node;
    }
    FontDictNode *n = freeList;

    n->key = key;
    freeList = n->next;
    n->value = value;
    n->next = 0;
    if (last == 0)
        buckets[bucket] = n;
    else
        last->next = n;
    return 1;
}

/* Removes `key': its node goes back on the free list (1); 0 if absent. */
// FUNC 0x0023D2B8 FontDict::remove
int FontDict::remove(const char *key)
{
    FontDictNode *prev = 0;
    u32 b = hash_string(key) % 15;
    FontDictNode *node;

    for (node = buckets[b]; node != 0; node = node->next) {
        if (strcmp(node->key, key) == 0) {
            if (prev == 0)
                buckets[b] = node->next;
            else
                prev->next = node->next;
            node->key = 0;
            node->value = 0;
            node->next = freeList;
            freeList = node;
            return 1;
        }
        prev = node;
    }
    return 0;
}

/* The system font (.data 0x002F1968: egcs puts a public object that has a
 * constructor in .data, zero-filled, not in .bss). */
Font g_002F1968;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002F1968 (.dtors entry 0x0028F270, .ctors entry 0x0028F19C), emitted at
 * the end of the unit, the destructor first: ~Font(2) / Font() on it.
 * Named after the unit's first global, the constructor. */
// FUNC 0x0023D388 T496_static_destroy (_GLOBAL_$D$__4Font)
/* implicit: ~Font (the static destructor of g_002F1968) */

// FUNC 0x0023D3B0 T496_static_init (_GLOBAL_$I$__4Font)
/* implicit: Font (the static initialiser of g_002F1968) */
