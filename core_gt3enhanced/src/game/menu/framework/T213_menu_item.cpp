/* T213 - gt2k_menu::Item (0x001B9318..0x001BA970).
 *
 * The image / text element of a menu page.  An Item is built from one
 * layout item (MenuLayoutItem) of a PageLayout: its rectangle, an optional
 * Tex1 texture (gzipped when the item flag 0x40000 is set; then inflated
 * onto the menu heap), optional vertex colours and an optional text, which
 * can scroll (text flags 0x10) or appear one character a frame (0x20).
 */
/* Retail's type names (T213 .rodata, reverse definition order): Item, then
 * the operator list classes (OperatorList, List / Cell<OperatorHolder>,
 * OperatorHolder) and Operator, all from T213_menu_item.h. */
#include "game/menu/framework/menu_item_internal.h"
#include "engine/pgl/G001_flare.h"                /* Tex1Image, tex1_relocate */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuText */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_clip_column, menu_clip_full */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/text/T499_font_metrics.h"         /* text_char_width, text_string_width */
#include "engine/pdistd/T537_gzip_inflate.h"         /* GZIP::InflateBase */
#include "game/menu/framework/T213_menu_item.h"
#include "game/enhanced.h"                  /* ENH_MENU_TEXT_WIDTH */

namespace gt2k_menu {

/* T213 .rodata, operator scripts (T217), defined below where retail has
 * them (around setTextPosition's jump table):
 *   fade in:  opFadeAlpha (25) +2.5 / s for 0.4 s (0 -> 1), opRemove (3):
 *             the operator ends itself;
 *   fade out: opFadeAlpha -6.667 / s for 0.15 s (1 -> 0), opRelease (6):
 *             the item is killed. */
extern const OperatorCode g_00333D80[2];    /* 0x00333D80 fade in (fadeIn) */
extern const OperatorCode g_00333E08[2];    /* 0x00333E08 fade out (virtual_4) */
/* T213 .sdata (defined below, before its strings): the vertex order of
 * MenuLayoutItem::colors, {3, 2, 0, 1}. */
extern const s8 g_00352568[4];              /* 0x00352568 gp-0x5488 */

// FUNC 0x001B9318 gt2k_menu::Item::setUVExtents
void Item::setUVExtents(MenuLayoutItem *item)
{
    MenuUV *uv = item->uv;

    /* float -> double compares (fptodp + dpcmp): the constant is a double. */
    if ((double)uv->u1 != 1.0 || (double)uv->v1 != 1.0) { /* implicit: fptodp, dpcmp (double compare) */
        type = TILED;
        f[0] = uv->u1 - uv->u0;
        f[1] = uv->v1 - uv->v0;
    }
}

// FUNC 0x001B93B8 gt2k_menu::Item::setFilter
/* Item flag 0x80000: bilinear texture filtering (and so the own render
 * state). */
void Item::setFilter(MenuLayoutItem *item)
{
    if (item->flags & ITEM_LINEAR_FILTER)
        drawFlags |= DRAW_OWN_SETTINGS | DRAW_LINEAR;
}

// FUNC 0x001B93E0 gt2k_menu::Item::setBlend
/* The item's blend (flags 0xF0) as Drawable's: alpha, additive, or the
 * alpha-tested mode 0x30.  Other values leave drawFlags alone.  The blend
 * is an int (retail compares signed, slti). */
void Item::setBlend(MenuLayoutItem *item)
{
    int blend = item->flags & ITEM_BLEND_MASK;
    switch (blend) {
    case ITEM_BLEND_ALPHA:
        drawFlags |= DRAW_OWN_SETTINGS | DRAW_BLEND_ALPHA;
        break;
    case ITEM_BLEND_ADD:
        drawFlags |= DRAW_OWN_SETTINGS | DRAW_BLEND_ADD;
        break;
    case ITEM_BLEND_SPECIAL:
        drawFlags |= DRAW_OWN_SETTINGS | DRAW_BLEND_TESTED;
        break;
    }
}

// FUNC 0x001B9448 gt2k_menu::Item::setRect
/* z, the centre (the layout's y is measured up from the bottom of the
 * screen) and the size of item: Drawable::setRect inline (retail loads all
 * four operands before the first store). */
void Item::setRect(MenuLayoutItem *item)
{
    z = item->z;
    Drawable::setRect(item->x + item->w * 0.5f, (f32)MENU_SCREEN_H - (item->y + item->h * 0.5f),
                      item->w, item->h);
}

/* The vertex order of MenuLayoutItem::colors (T213 .sdata, in front of the
 * strings: defined here, where retail has it). */
const s8 g_00352568[4] = { 3, 2, 0, 1 };   /* 0x00352568 gp-0x5488 */

// FUNC 0x001B94B0 gt2k_menu::Item::setVertexColors
/* item->colors (4 x RGBA8 in the order g_00352568) when it has them, else
 * grey 0x80808080.  Alpha 255 becomes 0.502 (the GS's 0x80 = opaque); on a
 * textured item the colours are halved too (0x80 = the texture as it is).
 * One byte pointer per channel: retail keeps colors + 1 / + 2 / + 3 in
 * registers and adds the vertex offset to each (an index expression folds
 * the channel into the load offset instead). */
void Item::setVertexColors(MenuLayoutItem *item)
{
    u32 flags = item->flags;

    if (flags & ITEM_HAS_COLORS) {
        const u8 *r = &item->colors[0];
        const u8 *g = &item->colors[1];
        const u8 *b = &item->colors[2];
        const u8 *a = &item->colors[3];
        for (int i = 0; i < 4; i++) {
            Color4f *dst = &color[i];
            int k = g_00352568[i];
            f32 scale = 1.0f;
            if (flags & ITEM_HAS_TEXTURE)
                scale = 0.5f;
            int base = k * 4;
            dst->r = (f32)r[base] * (1.0f / 255.0f) * scale;
            dst->g = (f32)g[base] * (1.0f / 255.0f) * scale;
            dst->b = (f32)b[base] * (1.0f / 255.0f) * scale;
            dst->a = (f32)a[base] * (1.0f / 255.0f) * 0.502f;
        }
        return;
    }

    /* Drawable::setColor(u32) inline: egcs folds r, g, b to 128 / 255 but
     * converts the alpha byte (u32 -> float) at run time, once per vertex. */
    Drawable::setColor(0x80808080u);
}

/* The fade-in script (T213 .rodata, in front of setTextPosition's jump
 * table): opFadeAlpha (25) +2.5 / s for 0.4 s, then opRemove (3). */
const OperatorCode g_00333D80[2] = {        /* 0x00333D80 */
    { 25 /* opFadeAlpha */, 0, 0, { 0, 0, 0, 0 }, 2.5f, 0.4f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
    { 3 /* opRemove */, 0, 0, { 0, 0, 0, 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
};

// FUNC 0x001B9798 gt2k_menu::Item::fadeIn
void Item::fadeIn()
{
    alphaRatio = 0;
    start(g_00333D80);
}

// FUNC 0x001B97C0 gt2k_menu::Item::appear
void Item::appear(MenuLayoutItem *item)
{
    if (item->flags & ITEM_FADE_IN_MASK) {
        fadeIn();
        return;
    }
    alphaRatio = 1.0f;
}

// FUNC 0x001B9800 gt2k_menu::Item::setTexture
/* Frees the previous inflated texture; a gzipped one is inflated into a
 * new block of the menu heap (its unpacked size) and relocated. */
void Item::setTexture(MenuTexture *tex)
{
    void *old = texBuf;
    if (old != NULL) {
        texBuf = NULL;
        texSet = NULL;
        menu_heap_current()->free(old);
    }

    if (res->flags & ITEM_GZIPPED) {
        void *buf = menu_heap_current()->allocate(tex->size, 0x10);
        {
            GZIP::InflateBase inflater(buf, tex->data); /* 0x3038 bytes on the stack */
        }
        tex1_relocate((Tex1Image *)buf);
        texBuf = buf;
        texSet = (Tex1Image *)buf;
    } else {
        texSet = (Tex1Image *)tex->data;
        texBuf = NULL;
    }
}

// FUNC 0x001B98C0 gt2k_menu::Item::Item(MenuLayoutItem*, MenuTexture*)
/* Transitive() (0x001BB4F8), the inline Rectangle() / Image(0) (zeros at
 * +0xC4..+0xD4), the members; hidden while setResource builds it.  The
 * landing pad runs ~Transitive (0x001BB5F8). */
Item::Item(MenuLayoutItem *item, MenuTexture *tex)
    : Image(0), res(item), texBuf(NULL), text(NULL), textColor(0), textX(0), textY(0), half(0)
{
    /* implicit: Transitive (base constructor), ~Transitive (exception cleanup) */
    visible = 0;
    setResource(item, tex);
    if (res != NULL)
        appear(res);
    visible = 1;
}

// FUNC 0x001B9998 gt2k_menu::Item::Item(MenuLayoutItem*, PageLayout*)
/* Transitive() (0x001BB4F8), the inline Rectangle() / Image(0) (zeros at
 * +0xC4..+0xD4), the members; hidden while setResource builds it.  The
 * landing pad runs ~Transitive (0x001BB5F8). */
Item::Item(MenuLayoutItem *item, PageLayout *layout)
    : Image(0), res(item), texBuf(NULL), text(NULL), textColor(0), textX(0), textY(0), half(0)
{
    /* implicit: Transitive (base constructor), ~Transitive (exception cleanup) */
    visible = 0;
    setResource(item, layout->getTexture(item));
    if (res != NULL)
        appear(res);
    visible = 1;
}

// FUNC 0x001B9A80 gt2k_menu::Item::Item(const char*, PageLayout*)
/* Transitive() (0x001BB4F8), the inline Rectangle() / Image(0) (zeros at
 * +0xC4..+0xD4), the members; hidden while setResource builds it.  The
 * landing pad runs ~Transitive (0x001BB5F8). */
Item::Item(const char *name, PageLayout *layout)
    : Image(0), res(NULL), texBuf(NULL), text(NULL), textColor(0), textX(0), textY(0), half(0)
{
    /* implicit: Transitive (base constructor), ~Transitive (exception cleanup) */
    visible = 0;
    setResource(name, layout);
    if (res != NULL)
        appear(res);
    visible = 1;
}

// FUNC 0x001B9B58 gt2k_menu::Item::Item(const char*, PageLayout*, MenuLayoutItem*)
/* As Item(name, layout), then centred on pos (the size stays the item's
 * own).  Transitive() (0x001BB4F8) first; the landing pad runs ~Transitive
 * (0x001BB5F8). */
Item::Item(const char *name, PageLayout *layout, MenuLayoutItem *pos)
    : Image(0), res(NULL), texBuf(NULL), text(NULL), textColor(0), textX(0), textY(0), half(0)
{
    /* implicit: Transitive (base constructor), ~Transitive (exception cleanup) */
    visible = 0;
    setResource(name, layout);
    if (res != NULL)
        appear(res);
    visible = 1;
    f32 cx = pos->x + pos->w * 0.5f;    /* both loaded before the first store */
    f32 cy = (f32)MENU_SCREEN_H - (pos->y + pos->h * 0.5f);
    x.set(cx);
    y.set(cy);
}

// FUNC 0x001B9C90 gt2k_menu::Item::~Item
/* Then ~Transitive (0x001BB5F8) directly: Rectangle's and Image's
 * destructors are implicit.  The deleting form ends with Object::operator
 * delete (0x001BF458). */
Item::~Item()
{
    if (texBuf != NULL) {
        menu_heap_current()->free(texBuf);
        texBuf = NULL;
        texSet = NULL;
    }
    if (text != NULL) {
        menu_heap_current()->free(text);
        text = NULL;
    }
    /* implicit: ~Transitive (base destructor), operator delete (deleting destructor) */
}

// FUNC 0x001B9D30 gt2k_menu::Item::setResource(MenuLayoutItem*, MenuTexture*)
void Item::setResource(MenuLayoutItem *item, MenuTexture *tex)
{
    res = item;
    if (item == NULL)
        return;

    if (item->flags & ITEM_HAS_TEXTURE) {
        if (tex != NULL)
            setTexture(tex);
        setUVExtents(res);
        setFilter(res);
    }
    setBlend(res);
    setRect(res);
    setVertexColors(res);

    if (!(res->flags & ITEM_HAS_TEXT))
        return;
    MenuText *t = res->text;
    if (t == NULL)
        return;

    switch (t->flags & TEXT_MODE_MASK) {
    case TEXT_SCROLL:                   /* the whole item width in 4 s */
        textPos = 0;
        textSpeed = w / 240.0f;
        if (t->font == NULL || t->font[0] == 0 || g_002F1968.setStyle(t->font) < 0)
            g_002F1968.setStyle(ITEM_DEFAULT_FONT);
        g_002F1968.setSize(1.0f, 1.0f);
        textWidth = text_string_width(&g_002F1968, t->text);
        break;
    case TEXT_TYPEWRITER:
        textPos = 0;
        textSpeed = 1.0f;
        textWidth = 0;
        break;
    default:
        textPos = 0;
        textSpeed = 0;
        textWidth = 0;
        break;
    }
}

// FUNC 0x001B9EA8 gt2k_menu::Item::setResource(const char*, PageLayout*)
void Item::setResource(const char *name, PageLayout *layout)
{
    MenuLayoutItem *item = layout->findHiddenItem(name);
    if (item != NULL)
        setResource(item, layout->getTexture(item));
}

// FUNC 0x001B9F08 gt2k_menu::Item::setText
void Item::setText(const char *s)
{
    char *old = text;
    if (old != NULL) {
        text = NULL;
        menu_heap_current()->free(old);
    }
    if (s != NULL) {
        int len = strlen(s);
        char *copy = (char *)menu_heap_current()->allocate(len + 1, 0x10);
        strcpy(copy, s);
        text = copy;
#if ENH_MENU_TEXT_WIDTH
        /* Lupo 2 (L2 0x001BF5F4..0x001BF63C; menus_a.md 31): a scrolling
         * text (TEXT_SCROLL) keeps the width of the text it scrolls; GT3
         * measures only the layout's text (setResource), so a text set here
         * scrolled with the old width.  The system font in the style it has
         * (Lupo 2 does not set it again). */
        if (res != NULL && (res->flags & ITEM_HAS_TEXT) && res->text != NULL &&
            (res->text->flags & TEXT_MODE_MASK) == TEXT_SCROLL)
            textWidth = text_string_width(&g_002F1968, copy);
#endif
    }
}

// FUNC 0x001B9F98 gt2k_menu::Item::virtual_1
/* Update: Transitive's (runs the operators), then the text animation. */
void Item::virtual_1()
{
    Transitive::virtual_1();

    if (res == NULL)
        return;
    if (!(res->flags & ITEM_HAS_TEXT))
        return;
    MenuText *t = res->text;
    if (t == NULL)
        return;

    switch (t->flags & TEXT_MODE_MASK) {
    case TEXT_SCROLL:                   /* starts over once the text has left */
        textPos = textPos + textSpeed;
        if (w + textWidth < textPos)
            textPos = 0;
        break;
    case TEXT_TYPEWRITER: {             /* one more character a frame, up to 255 */
        f32 shown = textPos;
        if (shown < 255.0f) {
            const char *s = text;
            shown = shown + 1.0f;
            textPos = shown;
            if (s == NULL)
                s = t->text;
            if (s[(int)shown] & 0x80)   /* a 2-byte (Shift-JIS) character */
                textPos = shown + 1.0f;
        } else {
            textPos = 255.0f;
        }
        break;
    }
    }
}

// FUNC 0x001BA090 gt2k_menu::Item::setTextPosition
/* textX / textY and the font alignment from the anchor: the item's left
 * edge (half a character in), centre or right edge, top, middle or
 * bottom. */
void Item::setTextPosition(MenuText *t)
{
    f32 xs[3];                          /* left, centre, right */
    f32 ys[3];                          /* top, middle, bottom */

    f32 left = getScreenX();
    xs[0] = left - w * 0.5f + (f32)text_char_width(&g_002F1968) * 0.5f;
    xs[1] = getScreenX();
    f32 right = getScreenX();
    xs[2] = right + w * 0.5f - (f32)text_char_width(&g_002F1968) * 0.5f;
    ys[0] = getScreenY() - h * 0.5f;
    ys[1] = getScreenY();
    ys[2] = getScreenY() + h * 0.5f;

    switch (t->flags & TEXT_ANCHOR_MASK) {     /* jump table 0x00333DE0 */
    case 0:
        textX = xs[0]; textY = ys[0];
        g_002F1968.setAlign(FONT_ALIGN_TOP | FONT_ALIGN_LEFT);
        break;
    case 1:
        textX = xs[1]; textY = ys[0];
        g_002F1968.setAlign(FONT_ALIGN_TOP | FONT_ALIGN_HCENTER);
        break;
    case 2:
        textX = xs[2]; textY = ys[0];
        g_002F1968.setAlign(FONT_ALIGN_TOP | FONT_ALIGN_RIGHT);
        break;
    case 3:
        textX = xs[0]; textY = ys[1];
        g_002F1968.setAlign(FONT_ALIGN_VCENTER | FONT_ALIGN_LEFT);
        break;
    case 4:
        textX = xs[1]; textY = ys[1];
        g_002F1968.setAlign(FONT_ALIGN_VCENTER | FONT_ALIGN_HCENTER);
        break;
    case 5:
        textX = xs[2]; textY = ys[1];
        g_002F1968.setAlign(FONT_ALIGN_VCENTER | FONT_ALIGN_RIGHT);
        break;
    case 6:
        textX = xs[0]; textY = ys[2];
        g_002F1968.setAlign(FONT_ALIGN_BOTTOM | FONT_ALIGN_LEFT);
        break;
    case 7:
        textX = xs[1]; textY = ys[2];
        g_002F1968.setAlign(FONT_ALIGN_BOTTOM | FONT_ALIGN_HCENTER);
        break;
    case 8:
        textX = xs[2]; textY = ys[2];
        g_002F1968.setAlign(FONT_ALIGN_BOTTOM | FONT_ALIGN_RIGHT);
        break;
    }
    g_002F1968.setPos(textX, textY);
}

// FUNC 0x001BA2A0 gt2k_menu::Item::beginText
/* The system font for the text: its face, position (a scrolling text is
 * clipped to the item's column and starts at its right edge), scale 1 and
 * colour (textColor, else the layout's; halved by halfTone; alpha scaled by
 * alphaRatio). */
void Item::beginText()
{
    MenuText *t = res->text;

    if (t->font == NULL || t->font[0] == 0 || g_002F1968.setStyle(t->font) < 0)
        g_002F1968.setStyle(ITEM_DEFAULT_FONT);

    switch (t->flags & TEXT_MODE_MASK) {
    case TEXT_SCROLL: {
        menu_clip_column(getScreenX(), w);
        g_002F1968.setAlign(FONT_ALIGN_VCENTER | FONT_ALIGN_LEFT);
        f32 left = getScreenX() + w * 0.5f - textPos;
        g_002F1968.setPos(left, getScreenY());
        break;
    }
    case TEXT_TYPEWRITER:
        setTextPosition(t);
        break;
    default:
        setTextPosition(t);
        break;
    }

    g_002F1968.setSize(1.0f, 1.0f);

    u32 rgba = textColor;
    if (rgba == 0)
        rgba = t->color;
    Color4f c(rgba);
    if (half != 0) {
        c.r = c.r * 0.5f;
        c.g = c.g * 0.5f;
        c.b = c.b * 0.5f;
    }
    g_002F1968.setColor(c.toRGBA8(alphaRatio));
}

// FUNC 0x001BA570 gt2k_menu::Item::endText
void Item::endText()
{
    if ((res->text->flags & TEXT_MODE_MASK) == TEXT_SCROLL)
        menu_clip_full();
    g_002F1968.setAlign(0);
}

// FUNC 0x001BA5B8 gt2k_menu::Item::drawImage
/* The frame from the item's flags, then Image's draw (a texture),
 * Rectangle's (vertex colours) or only Drawable's render state; half-toned,
 * the vertex colours are halved for the draw and put back.  res->flags is
 * read anew at each test (retail reloads it; a local copy of res lets GCSE
 * carry the first load over the frame stores), the frame as an int (slti). */
void Item::drawImage()
{
    int frame = res->flags & ITEM_FRAME_MASK;
    if (frame != 0) {
        switch (frame) {
        case ITEM_FRAME_SHADOW:
            frameType = FRAME_SHADOW;
            frameSize = 8.0f;
            break;
        case ITEM_FRAME_RAISED:
            frameType = FRAME_RAISED;
            frameSize = 8.0f;
            break;
        case ITEM_FRAME_SUNKEN:
            frameType = FRAME_SUNKEN;
            frameSize = 8.0f;
            break;
        }
    }

    Color4f saved[4];                   /* Color4f(): 0.502, dead stores */
    if (half != 0) {
        for (int i = 0; i < 4; i++) {
            Color4f *c = &color[i];
            saved[i] = *c;
            c->r = c->r * 0.5f;
            c->g = c->g * 0.5f;
            c->b = c->b * 0.5f;
        }
    }

    if ((res->flags & ITEM_HAS_TEXTURE) && texSet != NULL)
        Image::virtual_2();
    else if (res->flags & ITEM_HAS_COLORS)
        Rectangle::virtual_2();
    else
        Drawable::virtual_2();

    if (half != 0) {
        for (int i = 0; i < 4; i++)
            color[i] = saved[i];
    }
}

// FUNC 0x001BA780 gt2k_menu::Item::virtual_2
/* Draw: the image, then the text (a stack copy, cut at textPos in
 * typewriter mode). */
void Item::virtual_2()
{
    if (res == NULL)
        return;
    if (visible == 0)
        return;

    drawImage();
    if (!(res->flags & ITEM_HAS_TEXT))
        return;

    beginText();
    const char *s = text;
    if (s == NULL)
        s = res->text->text;
    int len = strlen(s);
    /* alloca, not a VLA: retail rounds len + 1 directly (sp -= (len + 0x10)
     * & ~0xF), a VLA's size goes through bits (sll 3; srl 3) first. */
    char *buf = (char *)__builtin_alloca(len + 1);
    strcpy(buf, s);
    if ((res->text->flags & TEXT_MODE_MASK) == TEXT_TYPEWRITER) {
        int shown = (int)textPos;
        if (shown < len)
            buf[shown] = 0;
    }
    g_002F1968.print("%s", buf);
    endText();
}

/* The fade-out script (T213 .rodata, after setTextPosition's jump table):
 * opFadeAlpha -6.667 / s for 0.15 s, then opRelease (6): the item is
 * killed. */
const OperatorCode g_00333E08[2] = {        /* 0x00333E08 */
    { 25 /* opFadeAlpha */, 0, 0, { 0, 0, 0, 0 }, -6.6666665f, 0.15f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
    { 6 /* opRelease */, 0, 0, { 0, 0, 0, 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
};

// FUNC 0x001BA898 gt2k_menu::Item::virtual_4
/* Disappear: script, else the fade-out when the item asks for one, else
 * kill() at once. */
void Item::virtual_4(const OperatorCode *script)
{
    const OperatorCode *close = NULL;

    if (script != NULL)
        close = script;
    else if (res != NULL && (res->flags & ITEM_FADE_OUT_MASK))
        close = g_00333E08;

    if (close != NULL)
        start(close);
    else
        kill();
}

// FUNC 0x001BA900 gt2k_menu::Item::halfTone(bool)
void Item::halfTone(bool on)
{
    half = on;
}

// FUNC 0x001BA908 gt2k_menu::Item::setColor(unsigned int)
void Item::setColor(u32 rgba)
{
    textColor = rgba;
}

// FUNC 0x001BA910 gt2k_menu::Item::get_current_text_y(void) const
f32 Item::get_current_text_y() const
{
    return textY;
}

// FUNC 0x001BA918 gt2k_menu::Item::get_current_text_x(void) const
f32 Item::get_current_text_x() const
{
    return textX;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (base Image). */
// FUNC 0x001BA920 __tfQ29gt2k_menu4Item
extern "C" void *__tfQ29gt2k_menu4Item(void)
{
    if (__tiQ29gt2k_menu4Item[0] == 0) {
        __tfQ29gt2k_menu5Image();
        __rtti_si(__tiQ29gt2k_menu4Item, "Q29gt2k_menu4Item",
                  (const type_info *)__tiQ29gt2k_menu5Image);
    }
    return __tiQ29gt2k_menu4Item;
}
#endif
