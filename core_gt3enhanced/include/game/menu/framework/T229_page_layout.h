/* T229_page_layout.h - gt2k_menu::PageLayout and the menu layout file data
 * (translation unit T229, unit u068).
 *
 * A layout file is read whole from the volume (malloc) and relocated in place
 * (offsets -> pointers, T243 relocate_archive).  It holds pages sorted by name; a
 * page has its own items and its layers ("defaultLayer", "hidden", ...); a
 * layer has plain items (built as Items) and object items, built by the hook
 * named by their type (the hook table the screen's code sets, binary-searched
 * by name).  The objects of a page go up by 100 in z per layout level; the
 * page transitions slide a layer in from / out to the side of inputMode.
 *
 * The same records have other names in the first pass: the 0x6C-byte item is
 * u067 / u070's MenuElement (T224 keeps u067's global name as a typedef),
 * u002 / u065 / u069's MenuResource (T235 defines it again under that name,
 * the type of ButtonList's (T244) element parameters); MenuLayer is u069 /
 * u070's MenuLayer of "elements" and "groups"; PageLayout is
 * u067's MenuFile; the hook table is GtmodeHook / OptionHook / PageHookTable
 * in the screens; MenuSliderFunc / MenuMultiFunc are u070's SliderEntry /
 * MultiEntry, and MultiButtonInfo is its MultiDesc. */
#ifndef GAME_T229_PAGE_LAYOUT_H
#define GAME_T229_PAGE_LAYOUT_H

#include "game/game.h"

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Drawable;                             /* T259 */
class Transitive;                           /* T216 */
class Item;                                 /* T213 */
struct MenuTexture;                         /* T213: {u32 size; void *data;} a texture entry, 8 bytes */
struct MenuUV;                              /* T213: +0x00 the texture index (int, getTexture) */
struct ButtonData;                          /* T244 */
struct SliderDesc;                          /* T238 */

/* The text of a layout item (16 bytes). */
struct MenuText {
    u16         unk0;                       /* 0x00 */
    u16         flags;                      /* 0x02 & 0x0F: anchor 0..8 (3 x 3); 0x10 scroll,
                                               0x20 typewriter */
    u32         color;                      /* 0x04 RGBA8, R in the low byte */
    const char *text;                       /* 0x08 */
    const char *font;                       /* 0x0C NULL / "": "sw_16" */
};
EGCS_SIZE(gt2k_menu::MenuText, 0x10);

/* The object type of an object item (its hook's name). */
struct MenuItemType {
    const char *name;                       /* 0x00 "": none (createObject makes nothing) */
};

/* MenuLayoutItem::flags (u065's names for what Item reads, u069's for what
 * the T243 relocation tests; merged by the sweep). */
enum {
    ITEM_HAS_TEXTURE   = 0x00000001,        /* uv: drawn as an Image */
    ITEM_HAS_COLORS    = 0x00000002,        /* colors: drawn as a Rectangle in them */
    ITEM_HAS_TEXT      = 0x00000004,        /* text */
    ITEM_HAS_ACTIVE    = 0x00000008,        /* active: another item, shown while a button is focused */
    ITEM_BLEND_MASK    = 0x000000F0,        /* Item::setBlend: 0x00 alpha, 0x10 additive, 0x20 the
                                               alpha-tested blend (Drawable blend 0x30) */
    ITEM_BLEND_ALPHA   = 0x00000000,
    ITEM_BLEND_ADD     = 0x00000010,
    ITEM_BLEND_SPECIAL = 0x00000020,
    ITEM_FRAME_MASK    = 0x00000F00,        /* Item::drawImage: the Rectangle's frame */
    ITEM_FRAME_SHADOW  = 0x00000100,
    ITEM_FRAME_RAISED  = 0x00000200,        /* (? the names may be swapped, T256) */
    ITEM_FRAME_SUNKEN  = 0x00000300,
    ITEM_BUTTON_KIND_MASK  = 0x0000F000,    /* the button kind (Button, T206; u063's names): */
    ITEM_BUTTON_KIND_FRAME = 0x00001000,    /*   a steady focus frame */
    ITEM_BUTTON_KIND_BLINK = 0x00002000,    /*   a blinking one (cross-faded with the active picture);
                                               0: no cursor picture */
    ITEM_TYPE_MASK     = 0x00030000,        /* == ITEM_HAS_TYPE: type, an object item's type record
                                               (relocated with its name, T243) */
    ITEM_HAS_TYPE      = 0x00010000,
    ITEM_GZIPPED       = 0x00040000,        /* the texture is gzipped */
    ITEM_LINEAR_FILTER = 0x00080000,        /* Item::setFilter: own draw settings, linear filter */
    ITEM_FADE_IN_MASK  = 0x00F00000,        /* Item::appear: the fade-in script */
    ITEM_FADE_OUT_MASK = 0x0F000000         /* Item::virtual_4: the fade-out script */
};

/* An item (element) of a layout: a rectangle with a texture, vertex colours
 * and a text (flags: ITEM_*).  y is measured up from the bottom of the
 * 448-line screen: an object's centre is (x + w / 2, 448 - (y + h / 2)). */
struct MenuLayoutItem {
    char            name[0x40];             /* 0x00 (the item arrays are sorted by it) */
    u32             flags;                  /* 0x40 */
    f32             x;                      /* 0x44 left */
    f32             y;                      /* 0x48 bottom (y up) */
    f32             z;                      /* 0x4C depth (the object's z) */
    f32             w;                      /* 0x50 */
    f32             h;                      /* 0x54 */
    MenuItemType   *type;                   /* 0x58 */
    MenuLayoutItem *active;                 /* 0x5C */
    MenuUV         *uv;                     /* 0x60 */
    const u8       *colors;                 /* 0x64 4 x RGBA8 */
    MenuText       *text;                   /* 0x68 */
};
EGCS_SIZE(gt2k_menu::MenuLayoutItem, 0x6C);

/* A layer of a page: plain items (built as Items) and object items (built
 * by the hook of their type). */
struct MenuLayer {
    char             name[0x40];            /* 0x00 */
    int              itemCount;             /* 0x40 */
    MenuLayoutItem **items;                 /* 0x44 sorted by name */
    int              objectCount;           /* 0x48 */
    MenuLayoutItem **objects;               /* 0x4C sorted by name */
};
EGCS_SIZE(gt2k_menu::MenuLayer, 0x50);

/* A page of the file: its own items and its layers. */
struct MenuPage {
    char             name[0x40];            /* 0x00 */
    int              itemCount;             /* 0x40 */
    MenuLayoutItem **items;                 /* 0x44 sorted by name */
    int              layerCount;            /* 0x48 */
    MenuLayer      **layers;                /* 0x4C sorted by name */
};
EGCS_SIZE(gt2k_menu::MenuPage, 0x50);

/* The header of a loaded layout file (relocated by T243 relocate_archive). */
struct MenuLayoutData {
    int          pageCount;                 /* 0x00 */
    MenuPage   **pages;                     /* 0x04 sorted by name */
    int          textureCount;              /* 0x08 */
    MenuTexture *textures;                  /* 0x0C */
};

/* Translation unit T243 (u069, the .pmb relocation) has no canonical header;
 * its one function called from outside is declared here, beside its only
 * caller (PageLayout::readFile).  Turns the file's offsets from base into
 * pointers: the pages and everything below them, the textures (and the
 * TEX1 images' own offsets).  u069 calls the record MenuArchiveFile. */
void relocate_archive(void *base, MenuLayoutData *data);   /* 0x001C7EB0 (T243) */

class PageLayout;

/* A hook: the create function of the objects whose type is `name'.  Every
 * entry has this type in the table; most functions take fewer arguments (p8
 * .. p14 are ints or function pointers, passed on as they are). */
typedef Transitive *MenuHookFn(void *ctx, PageLayout *layout, Object *parent,
                               MenuLayoutItem *item, int p8, int pC, int p10, int p14);
struct MenuHook {
    const char *name;                       /* 0x00 (the table is sorted by it) */
    MenuHookFn *create;                     /* 0x04 */
    int         p8;                         /* 0x08 */
    int         pC;                         /* 0x0C */
    int         p10;                        /* 0x10 */
    int         p14;                        /* 0x14 */
};
EGCS_SIZE(gt2k_menu::MenuHook, 0x18);

/* What a MultiButton (T269) needs, filled by its MenuMultiFunc.  It has
 * constructors: retail returns these 8 bytes in memory (a hidden buffer in
 * $a0, ButtonList::createMultiButton 0x001C8148 passes sp+0x40), which egcs
 * does only for a class with a constructor (a plain struct comes back in
 * $v0), and T066's functions build it in place, `return
 * MultiButtonInfo(changed, index)' (u022 notes). */
struct MultiButtonInfo {
    void (*changed)(void *ctx, int index);  /* 0x0 called with the choice every input frame */
    int   index;                            /* 0x4 the initial choice */

    MultiButtonInfo() {}
    MultiButtonInfo(void (*changed_)(void *ctx, int index), int index_)
        : changed(changed_), index(index_) {}
};
EGCS_SIZE(gt2k_menu::MultiButtonInfo, 0x8);

/* The two button function tables, sorted by name: sliders (ButtonList
 * createSlider, T238) and multi buttons (ButtonList createMultiButton).  The
 * functions return their record by value: the caller passes a stack buffer
 * as the hidden first argument ($a0, 0x001C80A0), ctx is $a1, and the
 * functions return the buffer in $v0 (T082 0x00163A04, T066 0x00141FA0). */
struct MenuSliderFunc {
    const char *name;                       /* 0x0 */
    SliderDesc (*func)(void *ctx);          /* 0x4 */
};
struct MenuMultiFunc {
    const char *name;                       /* 0x0 */
    MultiButtonInfo (*func)(void *ctx);     /* 0x4 */
};

/* Not polymorphic.  Size 0x84 (embedded in the page managers).  A loaded
 * layout file, its current page and layer, and the screen's tables. */
class PageLayout {
public:
    PageLayout(void *owner);                /* 0x001C2318 */
    void initialize();                      /* 0x001C2368 (PageManager::initialize) */
    void finalize();                        /* 0x001C2388 */
    int  loadFile(const char *file);        /* 0x001C2398 unless already loaded (1 then); 0 on error */
    int  reloadFile(const char *file);      /* 0x001C2408 unloadFile, readFile */
    int  readFile(const char *file);        /* 0x001C2448 reads and relocates; 0 on error */
    void unloadFile();                      /* 0x001C24A8 */
    void setHooks(const MenuHook *table, int count, void *ctx);             /* 0x001C24D8 */
    void setSliderFuncs(const MenuSliderFunc *table, int count, void *ctx); /* 0x001C24E8 */
    void setMultiFuncs(const MenuMultiFunc *table, int count, void *ctx);   /* 0x001C24F8 */
    int  setPage(const char *name, int level); /* 0x001C2508 the page and its "defaultLayer"; 0 if
                                               either is missing */
    void clearInputMode();                  /* 0x001C2590 */
    MenuLayer      *getLayer(const char *name);        /* 0x001C2598 of the current page; NULL if none */
    MenuLayer      *getHiddenLayer();                  /* 0x001C25E0 "hidden" */
    MenuLayoutItem *findLayerItem(const char *name);   /* 0x001C2600 plain item of the current layer */
    /* (findItem / findObject before the sweep: the static findObject made
     * `findObject(name, 0)' ambiguous with the member below.) */
    static MenuLayoutItem *findItemIn(const char *name, MenuLayer *l);   /* 0x001C2630 plain item of l */
    static MenuLayoutItem *findObjectIn(const char *name, MenuLayer *l); /* 0x001C2668 object item of l */
    MenuLayoutItem *findObject(const char *name, const char *layerName); /* 0x001C26A0 NULL layerName:
                                               the current layer */
    MenuLayoutItem *findPageItem(const char *name);    /* 0x001C26F8 item of the page itself */
    MenuLayoutItem *firstPageItem();                   /* 0x001C2738 */
    MenuLayoutItem *findHiddenItem(const char *name);  /* 0x001C2768 plain item of "hidden" (the
                                               pictures the code places itself: "pinch", "cursor") */
    MenuTexture    *getTexture(MenuLayoutItem *item);  /* 0x001C27A8 NULL without flag 1 */
    MenuText       *getText(MenuLayoutItem *item);     /* 0x001C27E0 NULL without flag 4 */
    const MenuHook       *findHook(const char *name);       /* 0x001C2800 */
    const MenuSliderFunc *findSliderFunc(const char *name); /* 0x001C2838 */
    const MenuMultiFunc  *findMultiFunc(const char *name);  /* 0x001C2870 */
    int         build(Object *parent);      /* 0x001C28A8 the current layer's items and objects under
                                               parent; 0 without a layer */
    Transitive *buildLayer(const char *name, int front); /* 0x001C2998 layer `name' in a new Transitive
                                               (front: each object attachSorted by z, else appended) */
    void        openTransition(Transitive *t);  /* 0x001C2B18 off screen on the side of inputMode,
                                               then slides in (script 0x00335720) */
    void        closeTransition(Transitive *t); /* 0x001C2C08 inputMode 0: kill(); else slides out
                                               (0x00335798 / 0x003357F8 / 0x00335858 / 0x003358B8)
                                               and disappearAll(NULL) */
    Item       *createItem(MenuLayoutItem *item, Object *parent, Drawable *anchor);   /* 0x001C2CC8 a new
                                               Item; z += level * 100; anchored (type 3) if given */
    Transitive *createObject(MenuLayoutItem *item, Object *parent, Drawable *anchor); /* 0x001C2D98 by
                                               the hook of its type (an Item if none); NULL for "" */
    MenuLayoutItem *findAnyItem(const char *name);     /* 0x001C2ED0 the page's items, then per layer
                                               its items and objects */

    /* owner: the PageManager's control (a ControlBase *: T219 / T092 take it
     * so); RaceMenuPage (T197) passes itself. */
    void                 *owner;            /* 0x00 */
    MenuLayoutData       *data;             /* 0x04 the file (malloc) */
    MenuPage             *page;             /* 0x08 */
    char                  fileName[0x40];   /* 0x0C */
    MenuLayer            *layer;            /* 0x4C "defaultLayer" of the page */
    ButtonData           *decided;          /* 0x50 the button that led here (jump) */
    ButtonData           *popup;            /* 0x54 the button that opened the popup */
    int                   level;            /* 0x58 the objects' z goes up by 100 per level */
    int                   inputMode;        /* 0x5C slide in from: 0 none, 1 / 2 top / bottom, 3 / 4 left / right */
    const MenuHook       *hooks;            /* 0x60 */
    int                   hookCount;        /* 0x64 */
    void                 *hookCtx;          /* 0x68 */
    const MenuSliderFunc *sliderFuncs;      /* 0x6C */
    int                   sliderCount;      /* 0x70 */
    void                 *sliderCtx;        /* 0x74 */
    const MenuMultiFunc  *multiFuncs;       /* 0x78 */
    int                   multiCount;       /* 0x7C */
    void                 *multiCtx;         /* 0x80 */
};
EGCS_SIZE(gt2k_menu::PageLayout, 0x84);

/* The binary searches by name (seven instances of one search, ? a template;
 * signed character compares, a hit is a full match).  NULL if not found. */
MenuPage       **find_page(MenuPage **table, int n, const char *name);              /* 0x001C2FA8 */
MenuLayer      **find_layer(MenuLayer **table, int n, const char *name);            /* 0x001C3040 */
MenuLayoutItem **find_layer_item(MenuLayoutItem **table, int n, const char *name);  /* 0x001C30D8 */
MenuLayoutItem **find_item(MenuLayoutItem **table, int n, const char *name);        /* 0x001C3170 */
const MenuHook       *find_hook(const MenuHook *table, int n, const char *name);    /* 0x001C3208 */
const MenuSliderFunc *find_slider_func(const MenuSliderFunc *table, int n,
                                       const char *name);                           /* 0x001C32A8 */
const MenuMultiFunc  *find_multi_func(const MenuMultiFunc *table, int n,
                                      const char *name);                            /* 0x001C3340 */

} /* namespace gt2k_menu */

#endif /* GAME_T229_PAGE_LAYOUT_H */
