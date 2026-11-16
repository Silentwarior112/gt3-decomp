/* T197 - the race menus' common helpers and RaceMenuPage.
 * 0x001AFA60..0x001B02F8.
 *
 * The race menus (pause menus, the replay option, the result board) run on
 * a 128 KB menu heap of their own, load their layouts from
 * "menu/<country>/<file>.pmb" and draw with a 640 x 448 2D projection that
 * is widened to 852.9 x 448 (and the system font squeezed by 0.75) when the
 * wide screen option (GameOptions::wide, 16:9) is set. */
#include "game/menu/race_menu/race_menu_internal.h"
#include "engine/pgl/G001_flare.h"                /* gfx_set_draw_area, gfx_blend_func, gfx_disable, gfx_set_scissor,
                                               pgl_matrix_mode, pgl_ortho, vu0_load_identity_regs */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push / menu_heap_pop */
#include "game/save/T429_game_status.h"          /* getGameStatus (GameOptions::wide); before the menu
                                               classes: retail's type names have the operator list
                                               classes, then the garage, then Operator */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive */
#include "game/menu/framework/T218_color.h"                /* Color4f */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/data/T516_country.h"              /* Country, g_00352FD8 */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/race_menu/T197_race_menu_page.h"

using namespace gt2k_menu;

/* The race menu heap (retail .sdata, first: initialised, so written where it
 * is defined, before the ".pmb" of race_menu_load_layout). */
void *g_00352398 = NULL;                    /* 0x00352398 */

/* The 16:9 projection: 640 / 0.75 wide, centred on x = 320 (T197 .sdata). */
static const f32 kWideLeft   = -106.432495f;    /* gp-0x7A28 */
static const f32 kWideRight  = 746.4325f;       /* gp-0x7A24 */
static const f32 kWideSqueeze = 0.7504553f;     /* gp-0x7A20 / gp-0x7A18 the font's / scissor's x scale */
static const f32 kWideFontX  = 106.40781f;      /* gp-0x7A1C the font's x offset */

/* Callers: race_begin (T007 game init). */
// FUNC 0x001AFA60 race_menu_heap_alloc
void race_menu_heap_alloc(void)
{
    if (g_00352398 == NULL)
        g_00352398 = memalign(0x40, RACE_MENU_HEAP_SIZE);
}

/* Callers: race_end (T007). */
// FUNC 0x001AFA90 race_menu_heap_free
void race_menu_heap_free(void)
{
    if (g_00352398 != NULL) {
        free(g_00352398);
        g_00352398 = NULL;
    }
}

/* Callers: RacePS2Base::RacePS2Base (T130). */
// FUNC 0x001AFAB8 race_menu_heap_push
void race_menu_heap_push(void)
{
    menu_heap_push(g_00352398, RACE_MENU_HEAP_SIZE);
}

/* Callers: RacePS2Base::~RacePS2Base. */
// FUNC 0x001AFAD8 race_menu_heap_pop
void race_menu_heap_pop(void)
{
    menu_heap_pop();
}

/* Reads "menu/<country>/<file>.pmb" into the layout and makes `page' its
 * current page.  1 when both succeed.  Only RaceMenuPage::load calls it. */
// FUNC 0x001AFAF8 race_menu_load_layout
static int race_menu_load_layout(PageLayout *layout, const char *file, const char *page)
{
    char path[0x100];

    sprintf(path, "menu/%s/", g_00352FD8.code());
    strcat(path, file);
    strcat(path, ".pmb");
    if (layout->readFile(path) == 0)
        return 0;
    return layout->setPage(page, 0) != 0;
}

/* The object of layer `name' of the current page (its objects attachSorted
 * by z), or an empty Transitive (Object::operator new(0xC4), Transitive();
 * Object::operator delete if the constructor throws) when the page has no
 * such layer. */
// FUNC 0x001AFBA0 race_menu_build_layer
Transitive *race_menu_build_layer(PageLayout *layout, const char *name)
{
    if (layout->getLayer(name) != NULL)
        return layout->buildLayer(name, 1);
    return new Transitive;                  /* Object::operator new; Object::operator delete on a throw */ /* implicit: operator new, operator delete (new-expression) */
}

/* A new Item of the hidden item `name' (Object::operator new(0x100), Item(name,
 * layout); Object::operator delete if it throws), attached under root. */
// FUNC 0x001AFC28 race_menu_new_item
Item *race_menu_new_item(PageLayout *layout, Transitive *root, const char *name, int sorted)
{
    /* Object::operator new(0x100), Item(name, layout); Object::operator
     * delete if the constructor throws.  A statement of its own: retail's
     * cleanup region ends before the attach (no sentry flag). */
    Item *item = new Item(name, layout);    /* implicit: operator new, operator delete (new-expression) */
    return static_cast<Item *>(race_menu_attach(root, item, sorted));
}

/* item among root's children by z (sorted), else as root's next sibling;
 * its position relative to root.  Returns item. */
// FUNC 0x001AFCC8 race_menu_attach
Transitive *race_menu_attach(Transitive *root, Transitive *item, int sorted)
{
    if (sorted != 0)
        item->attachSorted(root);
    else
        item->linkAfter(root);
    item->anchor = root;
    item->anchorType = 3;                   /* bit 0 x, bit 1 y: both relative */
    return item;
}

/* Colour `index' of the hidden item `name': its RGBA bytes / 255 * 0.5 (the
 * menus' colours: 0x80 is 1.0).  0 when the item does not exist (out
 * unchanged).  Callers: ReplayOption::loadColors (T196), ResultBoard(). */
// FUNC 0x001AFD28 race_menu_get_color
int race_menu_get_color(RaceMenuColor *out, PageLayout *layout, const char *name, int index)
{
    MenuLayoutItem *item = layout->findHiddenItem(name);
    if (item == NULL)
        return 0;
    const u8 *rgba = item->colors + index * 4;
    out->r = (f32)rgba[0] / 255.0f * 0.5f;
    out->g = (f32)rgba[1] / 255.0f * 0.5f;
    out->b = (f32)rgba[2] / 255.0f * 0.5f;
    out->a = (f32)rgba[3] / 255.0f * 0.5f;
    return 1;
}

/* The four vertex colours of drawable = color.  Callers:
 * ReplayOption::setColor (T196).  Drawable::setColor(const Color4f *, int)
 * per corner (retail: &color[i] in a register, the first store folded onto
 * the base).  RaceMenuColor has Color4f's layout and default (0.502): the
 * original's colours were probably Color4f (here and below the cast). */
// FUNC 0x001AFE10 race_menu_set_colors
void race_menu_set_colors(const RaceMenuColor *color, Drawable *drawable)
{
    const Color4f *c = (const Color4f *)color;
    drawable->setColor(c, 0);
    drawable->setColor(c, 1);
    drawable->setColor(c, 2);
    drawable->setColor(c, 3);
}

/* The text colour of item: (2r, 2g, 2b, a) packed (Color4f::toRGBA8,
 * alpha scale 1), stored directly into textColor.  A copy (Color4f::
 * operator=, channel by channel) doubled in place: retail stores alpha
 * first (the copy's r / g / b stores are dead); the constructor or set()
 * with the doubled values stores r first (92 %). */
// FUNC 0x001AFEA8 race_menu_set_text_color
void race_menu_set_text_color(const RaceMenuColor *color, Item *item)
{
    Color4f text;

    text = *(const Color4f *)color;
    text.r *= 2.0f;
    text.g *= 2.0f;
    text.b *= 2.0f;
    item->textColor = text.toRGBA8(1.0f);
}

/* Starts a menu draw into buf: the draw area, an orthographic projection of
 * the 640 x 448 screen (wide: -106.43 .. 746.43, the system font then
 * squeezed by 0.75 and moved right by 106.4), identity model view.
 * Callers: ReplayOption::draw, RaceMenuLayer::draw, ResultBoard::draw. */
// FUNC 0x001AFF10 race_menu_begin_draw
void race_menu_begin_draw(GSBuffer *buf)
{
    buf->setDrawBuffer();
    int width = buf->width;
    gfx_set_draw_area(0, 0, width, buf->displayHeight());
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    if (getGameStatus()->getGameOptions()->wide != 0) {
        pgl_ortho(kWideLeft, kWideRight, (f32)RACE_MENU_SCREEN_H, 0.0f, -1.0f, 1.0f);
        g_002F1968.setScale(kWideSqueeze, 1.0f);
        g_002F1968.setOffset(kWideFontX, 0.0f);
    } else {
        pgl_ortho(0.0f, (f32)RACE_MENU_SCREEN_W, (f32)RACE_MENU_SCREEN_H, 0.0f, -1.0f, 1.0f);
    }
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();
}

/* Ends it: the font back to scale 1 / offset 0, the normal alpha blend
 * ((Cs - Cd) * As + Cd, fix 0.5), blending off. */
// FUNC 0x001B0038 race_menu_end_draw
void race_menu_end_draw(void)
{
    g_002F1968.setScale(1.0f, 1.0f);
    g_002F1968.setOffset(0.0f, 0.0f);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 0.5f);
    gfx_disable(PGL_CAP_BLEND);
}

/* The scissor rectangle of size w x h centred on (cx, cy), in the menu's
 * 640-wide coordinates (squeezed by 0.75 around x = 320 when wide).  The
 * conversions truncate (cvt.w.s).  Only race_menu_set_scissor calls it. */
// FUNC 0x001B0098 race_menu_set_scissor_centered
static void race_menu_set_scissor_centered(f32 cx, f32 cy, f32 w, f32 h)
{
    if (getGameStatus()->getGameOptions()->wide != 0) {
        w = w * kWideSqueeze;
        cx = (cx - 320.0f) * kWideSqueeze + 320.0f;
    }
    gfx_set_scissor((int)(cx - w * 0.5f), (int)(cy - h * 0.5f), (int)w, (int)h);
}

/* The scissor rectangle (x, y, w, h).  Caller: ResultBar::render. */
// FUNC 0x001B0158 race_menu_set_scissor
void race_menu_set_scissor(f32 x, f32 y, f32 w, f32 h)
{
    race_menu_set_scissor_centered(x + w * 0.5f, y + h * 0.5f, w, h);
}

/* The whole screen (0, 0, 640, 448). */
// FUNC 0x001B0190 race_menu_reset_scissor
void race_menu_reset_scissor(void)
{
    gfx_set_scissor(0, 0, RACE_MENU_SCREEN_W, RACE_MENU_SCREEN_H);
}

/* ------------------------------------------------------------------------ */
/* RaceMenuPage                                                             */
/* ------------------------------------------------------------------------ */

/* MenuControl(), PageLayout(this), loaded = 0; a throw from the layout's
 * constructor runs Control::~Control (the MenuControl has no destructor of
 * its own).  Callers: PauseMenu, ReplayPauseMenu, PauseMenuWithRetry,
 * ReplayOption, ResultBoard constructors. */
// FUNC 0x001B01B8 RaceMenuPage::RaceMenuPage
RaceMenuPage::RaceMenuPage()
    : layout(this)                          /* MenuControl(), PageLayout::PageLayout(this) */ /* implicit: MenuControl, PageLayout (member constructors), ~Control (exception cleanup) */
{
    loaded = 0;
}

/* unload(), then Control::~Control(this, 2) (the MenuControl's implicit
 * destructor); egcs deleting flag: operator delete. */
// FUNC 0x001B0220 RaceMenuPage::~RaceMenuPage
RaceMenuPage::~RaceMenuPage()
{
    unload();
    /* egcs: Control::~Control(this, 2), then operator delete if flags & 1 */
    /* implicit: ~Control (member destructor), operator delete (deleting destructor) */
}

/* Loads page `page' of file `file' once.  1 when it is loaded. */
// FUNC 0x001B0270 RaceMenuPage::load
int RaceMenuPage::load(const char *file, const char *page)
{
    if (loaded == 0) {
        if (race_menu_load_layout(&layout, file, page) == 0)
            return 0;
        loaded = 1;
    }
    return loaded;
}

// FUNC 0x001B02C0 RaceMenuPage::unload
void RaceMenuPage::unload()
{
    if (loaded != 0) {
        layout.unloadFile();
        loaded = 0;
    }
}
