/* T264_raw_window.h - gt2k_menu::RawWindow (translation unit T264, unit u075).
 *
 * The base window of the layout-driven menu windows (RawList*, RawScrollBar,
 * RawCheckBox, RawLabel, the Play* and Logger* windows).  A window is built
 * from a node of the page layout (a MenuLayoutItem of T229's PageLayout): its
 * image, position and size come from the node, its name ("Class_name") from
 * the node's name.  Every frame the root window walks the tree twice:
 * processInputTree (children first; virtual_15 reads the pad of a window that
 * has a menuControl, 0 stops the walk) and callHooksTree (virtual_10, the
 * children, virtual_11).  Events (1..6: decide, cancel, up, down, left,
 * right) go up the parents (virtual_19 -> virtual_17 here, then virtual_18 on
 * each parent) until a handler returns 0.  Positions are centre based,
 * relative to the "position parent" (Drawable::posParent, attachPosition);
 * getLeft / setLeft / setWidth ... work with edges.  Drawing is clipped to
 * the windows above that have clipChildren set (the T264 .data rectangle
 * 0x002D1028, the screen 0x002D1018).
 *
 * Every class's registry boilerplate (slot 8 = the class name, the layout
 * factory, create, canCreate) is defined in T272 (u073); the registry itself
 * is T265.  The accessors at 0x001D7F38..0x001D8120 are inline members of
 * the original, emitted out of line at the end of T264 (no callers): they
 * are defined inline after the class, in reverse address order, and the
 * #pragma interface below / #pragma implementation in T264's source make
 * egcs emit their copies there (in reverse definition order).  Every use of
 * nextWindow / firstChildWindow / parentWindow expands their dynamic_cast
 * (T264, T263, T085, T087; 30-odd places).
 *
 * Virtual slots: the names stay virtual_N (as in T220_object.h and every
 * unit); [1] update, [2] draw, [3] "is active" (Object: drawn by drawSorted),
 * [4] Item's, [5] ImageGZ's, [6] / [7] Transitive's, [8]..[23] RawWindow's. */
#ifndef GAME_T264_RAW_WINDOW_H
#define GAME_T264_RAW_WINDOW_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#ifdef BOOT_OLD_GCC
/* egcs declares __dynamic_cast itself at the first dynamic_cast it compiles
 * (the inline accessors below), with C++ linkage, and then rejects T549's
 * extern "C" declaration of it; seen first, that declaration is used. */
#include "runtime/cxx/T549_tinfo2.h"               /* __dynamic_cast (egcs only) */
#endif

namespace gt2k_menu {
struct MenuLayoutItem;                      /* T229: a node of the page layout */
class PageLayout;                           /* T229: the page's layout file */
class PageManager;                          /* T211 */
class ControlBase;                          /* T247 */
class MenuControl;                          /* T214: the pad state (decide +0xE8, cancel +0xEC, up..right +0xC8..) */
class RawAnimation;                         /* T270 */
struct ButtonData;                          /* T244: a node of the layout tree (name, childCount +0x144,
                                               children +0x148) */

/* The construction arguments of every RawWindow: the eight arguments of a
 * layout factory (a MenuHookFn, T229: ctx, layout, parent, item, p8 .. p14)
 * as a record.  createWindow / loadChild pass owner, loader and parent on to
 * the children.  p0..p3 are the hook table's words: ints or string pointers
 * (PlayMusicBox, T085: the nodes of its check box and label).  (Planned in
 * T085, u037 having the fullest copy; it is this translation unit's.) */
struct RawWindowArgs {
    /* owner: the hook context, the controller.  RawWindow reads its
     * pageManager (+0x284); PlayListBox (T085) hands its control1 (+0) to
     * virtual_14 as the menu control. */
    ControlBase    *owner;                  /* 0x00 */
    PageLayout     *loader;                 /* 0x04 the layout the window is built from */
    Object         *parent;                 /* 0x08 the hook's parent object (only passed on) */
    MenuLayoutItem *node;                   /* 0x0C the layout node of the window */
    int             p0;                     /* 0x10 */
    int             p1;                     /* 0x14 */
    int             p2;                     /* 0x18 */
    int             p3;                     /* 0x1C */
};
EGCS_SIZE(gt2k_menu::RawWindowArgs, 0x20);

/* The events a RawWindow sends up its parents: RawWindow::virtual_16 makes
 * them of the menu control's buttons, in this order (virtual_17 / 18 / 19's
 * `event'); RawCheckBox adds RAW_EVENT_CHECK_TOGGLE (7, T275).  u074, u075. */
enum {
    RAW_EVENT_DECIDE = 1,                   /* MenuControl::decide */
    RAW_EVENT_CANCEL = 2,                   /* MenuControl::cancel */
    RAW_EVENT_UP     = 3,                   /* MenuControl::up: previous item of a vertical list */
    RAW_EVENT_DOWN   = 4,                   /* MenuControl::down: next item of a vertical list */
    RAW_EVENT_LEFT   = 5,                   /* MenuControl::left: previous item of a horizontal list */
    RAW_EVENT_RIGHT  = 6                    /* MenuControl::right: next item of a horizontal list */
};

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it): RawWindow's
 * inline copies and vtable only in T264 (#pragma implementation). */
#pragma interface
#endif

/* vtable 0x00339220 (24 slots).  typeinfo __tfQ29gt2k_menu9RawWindow
 * 0x001D8128 (si, base Item; __si_type_info 0x004B2810).  The vptr is
 * Object's (+0x1C).  Size 0x1E4 (operator new(0x1E4) in T272's create;
 * RawListItem's and RawScrollBar's data start at +0x1E4). */
class RawWindow : public Item {
public:
    RawWindow(const RawWindowArgs &args);   /* 0x001D5D98 Item(args.node, its texture); a RawAnimScroll */
    virtual ~RawWindow();                   /* [0] +0x0C 0x001D5F18 deletes anim */
    virtual void virtual_1();               /* [1] +0x14 0x001D5F90 update: Item's, anim; the root window
                                               also runs processInputTree and callHooksTree; virtual_9 */
    virtual void virtual_2();               /* [2] +0x1C 0x001D6230 draw: virtual_12 / virtual_13 inside the
                                               clip rectangle (clipToWindow), nothing if it is empty */
    virtual int  virtual_3();               /* [3] +0x24 0x001D7A78 returns active */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3B88 (T272) the class name, "RawWindow" */
    virtual void virtual_9();               /* [9] +0x54 0x001D6228 per-frame hook (empty) */
    virtual void virtual_10();              /* [10] +0x5C 0x001D6218 hook before the children (empty) */
    virtual void virtual_11();              /* [11] +0x64 0x001D6220 hook after the children (empty) */
    virtual void virtual_12();              /* [12] +0x6C 0x001D6350 draws the item (Item::virtual_2) */
    virtual void virtual_13();              /* [13] +0x74 0x001D6370 draws the highlight (white, highlightAlpha) */
    virtual void virtual_14(MenuControl *ctl);  /* [14] +0x7C 0x001D6678 menuControl = ctl (the children's too
                                               if childrenFocusChange); then virtual_20(ctl != NULL) */
    virtual int  virtual_15();              /* [15] +0x84 0x001D6560 input: virtual_16(menuControl), 1 without
                                               one; 0 stops processInputTree */
    virtual int  virtual_16(MenuControl *ctl);  /* [16] +0x8C 0x001D65A0 the first button pressed becomes event
                                               1..6, sent with virtual_19; 1 if not shown */
    virtual int  virtual_17(RawWindow *sender, int event, int arg);   /* [17] +0x94 0x001D6BD0 the event
                                               handler: 0 handled (here 1: goes on up) */
    virtual int  virtual_18(RawWindow *origin, RawWindow *sender, int event, int arg); /* [18] +0x9C 0x001D6BD8
                                               bubbling: lastOrigin = origin, virtual_17, the parent's virtual_18 */
    virtual int  virtual_19(int event, int arg);    /* [19] +0xA4 0x001D6CC0 sends an event: virtual_17(this,
                                               ...), then up the parents; 0 if handled */
    virtual void virtual_20(int focus);     /* [20] +0xAC 0x001D6788 sets the focus (the children's too if
                                               childrenFocusChange); no focus: highlightAlpha 0 */
    virtual void virtual_21(int focus);     /* [21] +0xB4 0x001D68B0 the focus look: focusProperty's image, or
                                               the node's image (brightened by focusBrightness with the focus) */
    virtual void virtual_22(RawWindow *child);  /* [22] +0xBC 0x001D77F0 a child window was built (empty) */
    virtual void virtual_23(int on);        /* [23] +0xC4 0x001D7AF0 active = on */

    /* T272: the registry entry of the class (g_003528D8, "RawWindow").  The
     * layout factory (no callers): the 8 words as a RawWindowArgs, then
     * create. */
    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3B90 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3BD0 (T272) new RawWindow(*args) if
                                               canCreate(args), else NULL */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3C48 (T272) 1 (args unused) */

    int  processInputTree();                /* 0x001D6028 the children first, then virtual_15; 0 as soon as
                                               one returns 0 */
    void callHooksTree();                   /* 0x001D6118 virtual_10, the children's, virtual_11 */
    void clipToWindow();                    /* 0x001D6410 narrows 0x002D1028 to this window (if clipChildren),
                                               then to the parents' (unless noParentClip) */
    void applyColourScale(f32 scale);       /* 0x001D6960 the node's image; rgb of the built colours, and of
                                               the text colour (res flag 4: res->text->color, into textColor),
                                               times scale */
    void setFocusPropertyIf(const char *image); /* 0x001D6BC0 focusProperty = image if not NULL */
    f32  halfWidth();                       /* 0x001D6DA0 */
    f32  halfHeight();                      /* 0x001D6DB8 */
    f32  getLeft();                         /* 0x001D6DD0 the left edge, relative to posParent's left edge */
    f32  getTop();                          /* 0x001D6E30 the top edge, relative to posParent's top edge */
    f32  getScreenLeft();                   /* 0x001D6E90 the left edge on screen */
    f32  getScreenTop();                    /* 0x001D6ED0 the top edge on screen */
    void setLeft(f32 left);                 /* 0x001D6F10 */
    void setTop(f32 top);                   /* 0x001D6F88 */
    void setWidth(f32 w);                   /* 0x001D7000 the left edge stays; the children keep their place */
    void setHeight(f32 h);                  /* 0x001D7070 the top edge stays */
    void attachPosition(RawWindow *parent); /* 0x001D70E0 posParent = parent (anchor 3), screen place kept */
    void setAnimation(RawAnimation *a);     /* 0x001D7170 deletes the old one (no callers) */
    void applyParentMove();                 /* 0x001D72E8 the children first; then the parent's moveX / moveY
                                               added to the centre */
    RawWindow *createChild(const char *cls, const char *nodeName, const RawWindowArgs *args,
                           int p0, int p1, int p2, int p3);    /* 0x001D7480 createWindow, attached to this
                                               (and its position); NULL if cls is not registered */
    RawWindow *loadChild(const char *name, const RawWindowArgs *args, ButtonData *tree); /* 0x001D74F0 the child
                                               window `name' and its children from the layout tree (NULL: the
                                               page's tree of that name); "Base_xxx" uses class Base; a child
                                               "<name>_focus" becomes the focusProperty */
    void setNode(const char *name);         /* 0x001D77F8 defaultItem = the hidden node `name' (if any), reloaded */
    void reloadNode(int takeSize, int takePos);     /* 0x001D7838 re-reads defaultItem's image; position /
                                               size kept unless takePos / takeSize */
    void changeImage(const char *name, int takeSize, int takePos); /* 0x001D7930 the same with the image of
                                               node `name' (PageLayout::findAnyItem) */
    void changeVertexColors(const char *name);  /* 0x001D7A40 Item::setVertexColors(the hidden node `name'),
                                               if there is one (no callers; F_001D7A40) */
    int  isShown();                         /* 0x001D7A80 1 if this window and every window above are active */
    RawWindow *findChild(const char *name); /* 0x001D7AF8 the first descendant (depth first) named name (isNamed) */
    RawWindow *findChildByFullName(const char *name);  /* 0x001D7BF8 the same by the full name (isFullName) */
    const char *getShortName();             /* 0x001D7CF8 windowName after the first '_', NULL if none */
    int  isFullName(const char *name);      /* 0x001D7D28 */
    int  isInClass(const char *cls);        /* 0x001D7D50 1 if this or a window above is of class cls (virtual_8) */
    int  isNamed(const char *name);         /* 0x001D7E10 the short name is name */
    int  hasResource(const char *name);     /* 0x001D7E50 Item::res is named name, 0 without one (no callers;
                                               u075: hasMaterial) */

    void setWindowName(const char *name);   /* 0x001D7F38 */
    const char *getWindowName();            /* 0x001D7F40 */
    MenuControl *getMenuControl();          /* 0x001D7F48 */
    RawWindow *getLastOrigin();             /* 0x001D7F50 */
    /* NULL for no window or one that is not a RawWindow. */
    RawWindow *nextWindow();                /* 0x001D7F58 */
    RawWindow *firstChildWindow();          /* 0x001D7FB0 */
    RawWindow *parentWindow();              /* 0x001D8008 */
    void setClipChildren(bool on);          /* 0x001D8060 */
    void changeTexture(const char *name);   /* 0x001D8068 changeImage(name, 1, 0) */
    void setDefaultItem(const MenuLayoutItem &item);   /* 0x001D8088 (Lupo 2: setDefaultItem(const MenuItem &)) */
    MenuLayoutItem *getDefaultItem();       /* 0x001D8090 */
    MenuLayoutItem *getStartItem();         /* 0x001D8098 */
    void setCY(f32 v);                      /* 0x001D80A0 the centre y */
    void setCX(f32 v);                      /* 0x001D80B8 the centre x */
    f32  getRH() const;                     /* 0x001D80D0 height */
    f32  getRW() const;                     /* 0x001D80D8 width */
    f32  getCY() const;                     /* 0x001D80E0 */
    f32  getCX() const;                     /* 0x001D80E8 */
    void setChildrenFocusChange(bool on);   /* 0x001D80F0 */
    const char *getFocusProperty();         /* 0x001D80F8 */
    void setFocusProperty(const char *image);   /* 0x001D8100 */
    int  getFocus();                        /* 0x001D8108 */
    void setUnk17C(int v);                  /* 0x001D8110 */
    f32  getMoveY();                        /* 0x001D8118 */
    f32  getMoveX();                        /* 0x001D8120 */

    PageLayout     *loader;                 /* 0x100 args.loader */
    PageManager    *pages;                  /* 0x104 args.owner's (owner +0x284) */
    MenuLayoutItem *startItem;              /* 0x108 args.node */
    MenuLayoutItem *defaultItem;            /* 0x10C the node the image is (re)loaded from (setNode, reloadNode) */
    const char     *windowName;             /* 0x110 the node's name as built: "Class_name" */
    int             noParentClip;           /* 0x114 non-zero: clipToWindow does not go on to the parents */
    int             active;                 /* 0x118 1 (virtual_3 / virtual_23; isShown) */
    int             clipChildren;           /* 0x11C the drawing of this window and below is clipped to it */
    f32             baseColor[4][4];        /* 0x120 the corner colours (Item::color) as built */
    const char     *focusProperty;          /* 0x160 the node whose image shows with the focus, NULL none */
    int             focus;                  /* 0x164 */
    f32             focusBrightness;        /* 0x168 2.0: the colour scale with the focus */
    f32             highlightAlpha;         /* 0x16C alpha of the white highlight (virtual_13), 0 none */
    int             childrenFocusChange;    /* 0x170 1: menu control and focus go to the children too */
    MenuControl    *menuControl;            /* 0x174 the pad the window reads (virtual_14) */
    int             unk_178;                /* 0x178 0 as built; non-zero: virtual_16 returns 0 (the input walk
                                               stops here) when no button is pressed */
    int             unk_17C;                /* 0x17C unk_178, copied when the pad is read (setUnk17C) */
    RawAnimation   *anim;                   /* 0x180 a RawAnimScroll as built (setAnimation); deleted with the window */
    f32             moveX;                  /* 0x184 the move of the centre handed to the children (setWidth) */
    f32             moveY;                  /* 0x188 (setHeight) */
    RawWindow      *lastOrigin;             /* 0x18C virtual_18's origin (NULL after virtual_19) */
    u8              unk_190[0x10];          /* 0x190 */
    /* clipVisible: `visible' before the sweep, which hid Drawable::visible (+0xAC). */
    int             clipVisible;            /* 0x1A0 virtual_2: the clip rectangle was not empty */
    u8              unk_1A4[0x40];          /* 0x1A4 */
};
EGCS_SIZE(gt2k_menu::RawWindow, 0x1E4);

/* The inline accessors (0x001D7F38..0x001D8120, see the top). */
inline f32 RawWindow::getMoveX() { return moveX; }  /* 0x001D8120 */
inline f32 RawWindow::getMoveY() { return moveY; }  /* 0x001D8118 */
inline void RawWindow::setUnk17C(int v) { unk_17C = v; }  /* 0x001D8110 */
inline int RawWindow::getFocus() { return focus; }  /* 0x001D8108 */
inline void RawWindow::setFocusProperty(const char *image) { focusProperty = image; }  /* 0x001D8100 */
inline const char *RawWindow::getFocusProperty() { return focusProperty; }  /* 0x001D80F8 */
inline void RawWindow::setChildrenFocusChange(bool on) { childrenFocusChange = on; }  /* 0x001D80F0 */
inline f32 RawWindow::getCX() const { return x.value; }  /* 0x001D80E8 */
inline f32 RawWindow::getCY() const { return y.value; }  /* 0x001D80E0 */
inline f32 RawWindow::getRW() const { return w; }  /* 0x001D80D8 */
inline f32 RawWindow::getRH() const { return h; }  /* 0x001D80D0 */
inline void RawWindow::setCX(f32 v) { x.set(v); }  /* 0x001D80B8 */
inline void RawWindow::setCY(f32 v) { y.set(v); }  /* 0x001D80A0 */
inline MenuLayoutItem *RawWindow::getStartItem() { return startItem; }  /* 0x001D8098 */
inline MenuLayoutItem *RawWindow::getDefaultItem() { return defaultItem; }  /* 0x001D8090 */
inline void RawWindow::setDefaultItem(const MenuLayoutItem &item) { defaultItem = (MenuLayoutItem *)&item; }  /* 0x001D8088 */
inline void RawWindow::changeTexture(const char *name) { changeImage(name, 1, 0); }  /* 0x001D8068 */
inline void RawWindow::setClipChildren(bool on) { clipChildren = on; }  /* 0x001D8060 */
inline RawWindow *RawWindow::parentWindow() { return dynamic_cast<RawWindow *>(parent); }  /* 0x001D8008 */
inline RawWindow *RawWindow::firstChildWindow() { return dynamic_cast<RawWindow *>(child); }  /* 0x001D7FB0 */
inline RawWindow *RawWindow::nextWindow() { return dynamic_cast<RawWindow *>(sibling); }  /* 0x001D7F58 */
inline RawWindow *RawWindow::getLastOrigin() { return lastOrigin; }  /* 0x001D7F50 */
inline MenuControl *RawWindow::getMenuControl() { return menuControl; }  /* 0x001D7F48 */
inline const char *RawWindow::getWindowName() { return windowName; }  /* 0x001D7F40 */
inline void RawWindow::setWindowName(const char *name) { windowName = name; }  /* 0x001D7F38 */

/* The root of w's tree (w included), then applyParentMove on it and on the
 * windows after it (setWidth / setHeight). */
void propagateMove(RawWindow *w);           /* 0x001D7200 */
/* A registered window of class cls on the node nodeName of args->loader
 * (NULL or not found: args->node), with p0..p3; NULL if cls is not
 * registered (T265 createRegistered). */
RawWindow *createWindow(const char *cls, const char *nodeName, const RawWindowArgs *args,
                        int p0, int p1, int p2, int p3);   /* 0x001D7E88 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions and nodes itself (and rejects hand
 * declarations of them). */
extern "C" {
void *__tfQ29gt2k_menu9RawWindow(void);     /* 0x001D8128 */
extern u32 __tiQ29gt2k_menu9RawWindow[3];   /* 0x004B2810 its __si_type_info, filled by it */
}
#endif

#endif /* GAME_T264_RAW_WINDOW_H */
