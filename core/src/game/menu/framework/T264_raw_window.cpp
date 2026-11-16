#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T264_raw_window.cpp"
#pragma implementation "src/game/menu/framework/T264_raw_window.cpp"
#pragma implementation "T264_raw_window.h"
#endif
/* T264 - gt2k_menu::RawWindow (0x001D5D60..0x001D8178).
 *
 * The base window of the layout-driven menu windows.  A window is built
 * from a node of the page layout (MenuLayoutItem): its image, position and
 * size come from the node; its name ("Class_name") from the node's name.
 * Every frame the root window walks the tree twice (processInputTree:
 * children first, virtual_15 reads the menu control of the window that has
 * one; callHooksTree: virtual_10 / children / virtual_11).  Events go up the
 * parents (virtual_19 -> virtual_17 here, then virtual_18 on each parent)
 * until a handler returns 0.  Positions are centre based, relative to the
 * "position parent" (Drawable::anchor, anchorType 3); setLeft / setTop /
 * setWidth / setHeight work with edges.  Drawing is clipped to the windows
 * above that have clipChildren set (the T264 .data rectangle g_002D1028).
 */
/* Retail's .rodata (0x00339220..0x003393F8): the vtable, then the type names
 * in the reverse order of the classes' definitions: RawWindow, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder> (T216's, through
 * Item's header), SmartIndexT<int> (instantiated before them: T211's
 * PageHistory, so T211 comes first), Operator (a non-polymorphic base: its
 * name after all the others). */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (link) */
#include "game/menu/framework/T264_raw_window.h"           /* this translation unit */
#include "game/common/curves_internal.h"
#include "eekernel.h"                       /* EI() */
#include "engine/pgl/G001_flare.h"                /* pgl_color4f, gs_draw_flat_sprite */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend */
#include "runtime/newlib/G007_stdio.h"                /* strcat, strchr, strcmp, strcpy */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (up .. right, decide, cancel) */
#include "game/menu/framework/T218_color.h"                /* Color4f */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T241_link_manager.h"         /* gt2k_menu::LinkManager::findButtonData */
#include "game/menu/framework/T244_button_data.h"          /* gt2k_menu::ButtonData (the layout tree) */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (pageManager) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_clip_box */
#include "game/menu/framework/T265_raw_window_register.h"  /* createRegistered */
#include "game/menu/framework/T271_raw_anim_scroll.h"      /* gt2k_menu::RawAnimScroll */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si */
#endif

/* .data: the whole virtual screen and the current clip rectangle
 * {x0, y0, x1, y1} (see curves_internal.h) */
f32 g_002D1018[4] = { 0.0f, 0.0f, 640.0f, 448.0f };     /* 0x002D1018 */
f32 g_002D1028[4] = { 0.0f, 0.0f, 640.0f, 448.0f };     /* 0x002D1028 */

namespace gt2k_menu {

/* The clip-rectangle and colour copies are array assignments in the
 * original: egcs's build_vec_init loop (a pointer pair and a count-down from
 * 3 that ends at -1, `bne -1'; one element per pass, Color4f::operator=
 * copying r, g, b, a), which an element loop does not give (loop.c reverses
 * that into a `bgez' count-down).  baseColor is a f32[4][4], not a
 * Color4f[4]: a Color4f[4] member would be default-constructed (0.502, the
 * egcs array loop), which the constructor does not do; the colour copies
 * assign it as RawColors.  GCC 14 does not assign arrays: element by element
 * there (the same copy). */
typedef Color4f RawColors[4];
#ifdef BOOT_OLD_GCC
#define RAW_ASSIGN_ARRAY(dst, src)  ((dst) = (src))
#else
#define RAW_ASSIGN_ARRAY(dst, src)                                              \
    do {                                                                        \
        unsigned raw_i_;                                                        \
        for (raw_i_ = 0; raw_i_ < sizeof(dst) / sizeof((dst)[0]); raw_i_++)     \
            (dst)[raw_i_] = (src)[raw_i_];                                      \
    } while (0)
#endif

/* the scissor = the current clip rectangle */
// FUNC 0x001D5D60 applyClipRect
static void applyClipRect(void)
{
    f32 x0 = g_002D1028[0], y0 = g_002D1028[1];

    menu_clip_box(x0, y0, g_002D1028[2] - x0, g_002D1028[3] - y0);
}

// FUNC 0x001D5D98 gt2k_menu::RawWindow::RawWindow
RawWindow::RawWindow(const RawWindowArgs &args)
    : Item(args.node, args.loader->getTexture(args.node))
{
    startItem = args.node;
    loader = args.loader;
    pages = args.owner->pageManager;
    defaultItem = args.node;
    windowName = startItem->name;           /* retail: startItem's register (args.node is read again
                                               for defaultItem, after the startItem store) */
    active = 1;
    noParentClip = 0;
    clipChildren = 0;
    RAW_ASSIGN_ARRAY(*(RawColors *)baseColor, color);
    focusProperty = NULL;
    focus = 0;
    highlightAlpha = 0.0f;
    menuControl = NULL;
    unk_178 = 0;
    focusBrightness = 2.0f;
    childrenFocusChange = 1;
    anim = new RawAnimScroll();             /* implicit: Object::operator new (the menu heap); Object::operator
                                               delete if the constructor throws */
    moveX = 0.0f;
    moveY = 0.0f;
    lastOrigin = NULL;
    anim->virtual_5(this);
}

// FUNC 0x001D5F18 gt2k_menu::RawWindow::~RawWindow
/* implicit: the deleting form (flags & 1) ends with Object::operator delete */
RawWindow::~RawWindow()
{
    if (anim != NULL)
        delete anim;                        /* virtual, flags 3 */
}

/* update: the item, the animation; the root window also runs the input
 * and hook walks; then the per-frame hook */
// FUNC 0x001D5F90 gt2k_menu::RawWindow::virtual_1
void RawWindow::virtual_1()
{
    Item::virtual_1();
    anim->update();
    if (parentWindow() == NULL) {  /* implicit: __dynamic_cast (inline parentWindow) */
        processInputTree();
        callHooksTree();
    }
    virtual_9();
}

/* children first; stops (0) as soon as one returns 0 */
// FUNC 0x001D6028 gt2k_menu::RawWindow::processInputTree
int RawWindow::processInputTree()
{
    RawWindow *child;

    for (child = firstChildWindow(); child != NULL; child = child->nextWindow()) {  /* implicit: __dynamic_cast (inline firstChildWindow, nextWindow) */
        if (child->processInputTree() == 0)
            return 0;
    }
    return virtual_15();
}

// FUNC 0x001D6118 gt2k_menu::RawWindow::callHooksTree
void RawWindow::callHooksTree()
{
    RawWindow *child;

    virtual_10();
    for (child = firstChildWindow(); child != NULL; child = child->nextWindow())  /* implicit: __dynamic_cast (inline firstChildWindow, nextWindow) */
        child->callHooksTree();
    virtual_11();
}

// FUNC 0x001D6218 gt2k_menu::RawWindow::virtual_10
void RawWindow::virtual_10()
{
}

// FUNC 0x001D6220 gt2k_menu::RawWindow::virtual_11
void RawWindow::virtual_11()
{
}

// FUNC 0x001D6228 gt2k_menu::RawWindow::virtual_9
void RawWindow::virtual_9()
{
}

/* draw: clip rectangle = screen narrowed by the clipping windows; draws
 * the item and the highlight unless it is empty */
// FUNC 0x001D6230 gt2k_menu::RawWindow::virtual_2
void RawWindow::virtual_2()
{
    RAW_ASSIGN_ARRAY(g_002D1028, g_002D1018);
    clipToWindow();
    clipVisible = 1;
    if (g_002D1028[2] <= g_002D1028[0] || g_002D1028[1] >= g_002D1028[3])  /* retail loads y1 first */
        clipVisible = 0;
    if (clipVisible != 0) {
        applyClipRect();
        virtual_12();
        virtual_13();
        RAW_ASSIGN_ARRAY(g_002D1028, g_002D1018);
        applyClipRect();
    }
}

// FUNC 0x001D6350 gt2k_menu::RawWindow::virtual_12
void RawWindow::virtual_12()
{
    Item::virtual_2();
}

/* a white rectangle over the window, alpha highlightAlpha */
// FUNC 0x001D6370 gt2k_menu::RawWindow::virtual_13
void RawWindow::virtual_13()
{
    f32 left, top;

    if (highlightAlpha != 0.0f) {
        pgl_color4f(1.0f, 1.0f, 1.0f, highlightAlpha);
        left = getScreenLeft();
        top = getScreenTop();
        gs_draw_flat_sprite((int)left, (int)top, (int)(left + w), (int)(top + h));
    }
}

/* narrows g_002D1028 to this window (if clipChildren), then to the
 * parents unless noParentClip; an empty result collapses x0 / y0 */
// FUNC 0x001D6410 gt2k_menu::RawWindow::clipToWindow
void RawWindow::clipToWindow()
{
    f32 cx, cy;                             /* the centre on screen */
    f32 x0, y0, x1, y1;                     /* this window's edges */
    f32 clipX0, clipY0, clipX1, clipY1;     /* the clip rectangle as narrowed */
    RawWindow *parent;

    if (clipChildren != 0) {
        getScreenXY(&cx, &cy);
        x0 = cx - w * 0.5f;
        y0 = cy - h * 0.5f;
        x1 = x0 + w;
        y1 = y0 + h;
        clipX0 = g_002D1028[0];
        if (clipX0 < x0) {
            g_002D1028[0] = x0;
            clipX0 = x0;
        }
        clipY0 = g_002D1028[1];
        if (clipY0 < y0) {
            g_002D1028[1] = y0;
            clipY0 = y0;
        }
        clipX1 = g_002D1028[2];
        if (x1 < clipX1) {
            g_002D1028[2] = x1;
            clipX1 = x1;
        }
        clipY1 = g_002D1028[3];
        if (y1 < clipY1) {
            g_002D1028[3] = y1;
            clipY1 = y1;
        }
        if (clipX1 <= clipX0)
            g_002D1028[0] = clipX1;
        if (clipY1 <= clipY0)
            g_002D1028[1] = clipY1;
    }
    if (noParentClip == 0) {
        parent = parentWindow();  /* implicit: __dynamic_cast (inline parentWindow) */
        if (parent != NULL)
            parent->clipToWindow();
    }
}

// FUNC 0x001D6560 gt2k_menu::RawWindow::virtual_15
int RawWindow::virtual_15()
{
    if (menuControl != NULL)
        return virtual_16(menuControl);
    return 1;
}

/* the first button pressed becomes an event sent with virtual_19; with
 * none, unk_178 != 0 stops the input walk here (0).  One return per button:
 * jump2 cross-jumps the six calls into one tail, each test ending in
 * `li a1,<event>; b' (an event variable gives one `bnez' chain instead) */
// FUNC 0x001D65A0 gt2k_menu::RawWindow::virtual_16
int RawWindow::virtual_16(MenuControl *ctl)
{
    if (isShown() == 0)
        return 1;
    unk_17C = unk_178;
    if (ctl->decide != 0)
        return virtual_19(RAW_EVENT_DECIDE, 0);
    if (ctl->cancel != 0)
        return virtual_19(RAW_EVENT_CANCEL, 0);
    if (ctl->up != 0)
        return virtual_19(RAW_EVENT_UP, 0);
    if (ctl->down != 0)
        return virtual_19(RAW_EVENT_DOWN, 0);
    if (ctl->left != 0)
        return virtual_19(RAW_EVENT_LEFT, 0);
    if (ctl->right != 0)
        return virtual_19(RAW_EVENT_RIGHT, 0);
    return unk_178 ^ 1;
}

/* menuControl = ctl (and the children's, if childrenFocusChange); the
 * focus follows */
// FUNC 0x001D6678 gt2k_menu::RawWindow::virtual_14
void RawWindow::virtual_14(MenuControl *ctl)
{
    RawWindow *child;

    menuControl = ctl;
    if (childrenFocusChange != 0) {
        for (child = firstChildWindow(); child != NULL; child = child->nextWindow())  /* implicit: __dynamic_cast (inline firstChildWindow, nextWindow) */
            child->virtual_14(menuControl);
    }
    virtual_20(menuControl != NULL);
}

// FUNC 0x001D6788 gt2k_menu::RawWindow::virtual_20
void RawWindow::virtual_20(int on)
{
    RawWindow *child;

    if (on != focus)
        virtual_21(focus = on);             /* retail reads the vtable before the store (a separate
                                               `focus = on;' is scheduled first) */
    if (on == 0)
        highlightAlpha = 0.0f;
    if (childrenFocusChange != 0) {
        for (child = firstChildWindow(); child != NULL; child = child->nextWindow())  /* implicit: __dynamic_cast (inline firstChildWindow, nextWindow) */
            child->virtual_20(on);
    }
}

/* with the focus: the focusProperty node's image if there is one, else the
 * node's image brightened; without: the node's image, the built colours */
// FUNC 0x001D68B0 gt2k_menu::RawWindow::virtual_21
void RawWindow::virtual_21(int on)
{
    const char *image = focusProperty;      /* read before the test of on (retail: the load fills
                                               the first branch's delay slot) */

    if (on != 0 && image != NULL) {
        changeImage(image, 0, 0);
        return;
    }
    RAW_ASSIGN_ARRAY(color, *(RawColors *)baseColor);
    textColor = 0;
    reloadNode(0, 0);
    if (on != 0)
        applyColourScale(focusBrightness);
}

/* r, g, b (not a) of one colour times s.  (? an inline member of Color4f in
 * the original; u037's PlayMusicBox* scale the same way, src/u037/u037.h.)
 * Per element: a loop over a Color4f pointer lets GCSE share &color with the
 * copy before it, which retail recomputes. */
static inline void scaleRGB(Color4f &c, f32 s)
{
    c.r *= s;
    c.g *= s;
    c.b *= s;
}

/* the node's image with r, g, b of the built colours (and of the text
 * colour, if the node has a text) times scale */
// FUNC 0x001D6960 gt2k_menu::RawWindow::applyColourScale
void RawWindow::applyColourScale(f32 scale)
{
    int i, intrWasOn;
    MenuLayoutItem *node;

    RAW_ASSIGN_ARRAY(color, *(RawColors *)baseColor);
    textColor = 0;
    reloadNode(0, 0);
    intrWasOn = intr_suspend();
    node = res;
    for (i = 0; i < 4; i++)
        scaleRGB(color[i], scale);
    if (intrWasOn)
        EI();
    /* no NULL check of res in the original */
    if ((node->flags & 4) && node->text != NULL) {   /* item flag 4: has a text */
        Color4f text(node->text->color);
        text.r *= scale;
        text.g *= scale;
        text.b *= scale;
        textColor = text.toRGBA8(1.0f);
    }
}

// FUNC 0x001D6BC0 gt2k_menu::RawWindow::setFocusPropertyIf
void RawWindow::setFocusPropertyIf(const char *image)
{
    if (image != NULL)
        focusProperty = image;
}

/* default event handler: not handled, the event goes on up */
// FUNC 0x001D6BD0 gt2k_menu::RawWindow::virtual_17
int RawWindow::virtual_17(RawWindow *sender, int event, int arg)
{
    return 1;
}

// FUNC 0x001D6BD8 gt2k_menu::RawWindow::virtual_18
int RawWindow::virtual_18(RawWindow *origin, RawWindow *sender, int event, int arg)
{
    RawWindow *parent;

    lastOrigin = origin;
    parent = parentWindow();  /* implicit: __dynamic_cast (inline parentWindow) */
    if (virtual_17(sender, event, arg) == 0)
        return 0;
    if (parent == NULL)
        return 1;
    return parent->virtual_18(parent, sender, event, arg);
}

// FUNC 0x001D6CC0 gt2k_menu::RawWindow::virtual_19
int RawWindow::virtual_19(int event, int arg)
{
    RawWindow *parent;

    lastOrigin = NULL;
    parent = parentWindow();  /* implicit: __dynamic_cast (inline parentWindow) */
    if (virtual_17(this, event, arg) == 0)
        return 0;
    if (parent == NULL)
        return 1;
    return parent->virtual_18(this, this, event, arg);
}

/* ---- positions: x / y are the centre, relative to the anchor's centre;
 * "left" / "top" are edges relative to the anchor's left / top edge ---- */

// FUNC 0x001D6DA0 gt2k_menu::RawWindow::halfWidth
f32 RawWindow::halfWidth()
{
    return w * 0.5f;
}

// FUNC 0x001D6DB8 gt2k_menu::RawWindow::halfHeight
f32 RawWindow::halfHeight()
{
    return h * 0.5f;
}

// FUNC 0x001D6DD0 gt2k_menu::RawWindow::getLeft
f32 RawWindow::getLeft()
{
    f32 anchorLeft;                         /* the anchor's left edge, relative to its centre */
    f32 centre;

    anchorLeft = (anchor != NULL) ? anchor->w * 0.5f : 0.0f;
    centre = x.value;
    return (centre - halfWidth()) + anchorLeft;
}

// FUNC 0x001D6E30 gt2k_menu::RawWindow::getTop
f32 RawWindow::getTop()
{
    f32 anchorTop;
    f32 centre;

    anchorTop = (anchor != NULL) ? anchor->h * 0.5f : 0.0f;
    centre = y.value;
    return (centre - halfHeight()) + anchorTop;
}

// FUNC 0x001D6E90 gt2k_menu::RawWindow::getScreenLeft
f32 RawWindow::getScreenLeft()
{
    f32 centre = getScreenX();

    return centre - halfWidth();
}

// FUNC 0x001D6ED0 gt2k_menu::RawWindow::getScreenTop
f32 RawWindow::getScreenTop()
{
    f32 centre = getScreenY();

    return centre - halfHeight();
}

// FUNC 0x001D6F10 gt2k_menu::RawWindow::setLeft
void RawWindow::setLeft(f32 left)
{
    f32 anchorLeft;
    f32 half;

    anchorLeft = (anchor != NULL) ? anchor->w * 0.5f : 0.0f;
    half = halfWidth();
    x.set((left - anchorLeft) + half);
}

// FUNC 0x001D6F88 gt2k_menu::RawWindow::setTop
void RawWindow::setTop(f32 top)
{
    f32 anchorTop;
    f32 half;

    anchorTop = (anchor != NULL) ? anchor->h * 0.5f : 0.0f;
    half = halfHeight();
    y.set((top - anchorTop) + half);
}

/* the left edge stays; the children keep their place (moveX).  MotionValue::get
 * (inline): retail keeps &x in $s1 across the calls (also reloadNode,
 * changeImage, applyParentMove) */
// FUNC 0x001D7000 gt2k_menu::RawWindow::setWidth
void RawWindow::setWidth(f32 width)
{
    f32 oldCentre = x.get();
    f32 left = getLeft();

    w = width;
    setLeft(left);
    moveX = oldCentre - x.get();
    propagateMove(this);
    moveX = 0.0f;
}

// FUNC 0x001D7070 gt2k_menu::RawWindow::setHeight
void RawWindow::setHeight(f32 height)
{
    f32 oldCentre = y.get();
    f32 top = getTop();

    h = height;
    setTop(top);
    moveY = oldCentre - y.get();
    propagateMove(this);
    moveY = 0.0f;
}

/* makes the position relative to parent (anchorType 3: both axes), the
 * screen place unchanged */
// FUNC 0x001D70E0 gt2k_menu::RawWindow::attachPosition
void RawWindow::attachPosition(RawWindow *parent)
{
    f32 left = getScreenLeft();
    f32 top = getScreenTop();

    anchorType = 3;
    anchor = parent;
    if (parent != NULL) {
        left -= parent->getScreenLeft();
        top -= parent->getScreenTop();
    }
    setLeft(left);
    setTop(top);
}

/* no callers */
// FUNC 0x001D7170 gt2k_menu::RawWindow::setAnimation
void RawWindow::setAnimation(RawAnimation *a)
{
    if (anim != NULL) {
        anim->virtual_5(NULL);
        if (anim != NULL)
            delete anim;                    /* virtual, flags 3 */
    }
    if (a != NULL)
        a->virtual_5(this);
    anim = a;
}

/* the root of w's tree (the last window of the parent chain), then
 * applyParentMove on it and on the windows after it */
// FUNC 0x001D7200 gt2k_menu::propagateMove
void propagateMove(RawWindow *w)
{
    RawWindow *root = NULL;
    RawWindow *win;

    for (win = w; win != NULL; win = win->parentWindow())  /* implicit: __dynamic_cast (inline parentWindow) */
        root = win;
    for (; root != NULL; root = root->nextWindow())  /* implicit: __dynamic_cast (inline nextWindow) */
        root->applyParentMove();
}

/* children first; then adds the parent's moveX / moveY to the centre */
// FUNC 0x001D72E8 gt2k_menu::RawWindow::applyParentMove
void RawWindow::applyParentMove()
{
    RawWindow *child, *parent;

    for (child = firstChildWindow(); child != NULL; child = child->nextWindow())  /* implicit: __dynamic_cast (inline firstChildWindow, nextWindow) */
        child->applyParentMove();
    parent = parentWindow();  /* implicit: __dynamic_cast (inline parentWindow) */
    if (parent != NULL) {
        x.set(x.get() + parent->getMoveX());
        y.set(y.get() + parent->getMoveY());
    }
}

/* buf = name up to the first '_' (NULL if there is none) */
// FUNC 0x001D7440 gt2k_menu::layoutBaseName
static char *layoutBaseName(char *buf, const char *name)
{
    char *sep;

    strcpy(buf, name);
    sep = strchr(buf, '_');
    if (sep != NULL) {
        *sep = '\0';
        sep = buf;
    }
    return sep;
}

/* a window of class cls on node nodeName (NULL: args->node), as a child of
 * this */
// FUNC 0x001D7480 gt2k_menu::RawWindow::createChild
RawWindow *RawWindow::createChild(const char *cls, const char *nodeName, const RawWindowArgs *args,
                                  int p0, int p1, int p2, int p3)
{
    RawWindow *win = createWindow(cls, nodeName, args, p0, p1, p2, p3);

    if (win != NULL) {
        win->attachTo(this);
        win->attachPosition(this);
    }
    return win;
}

/* Builds the child window `name' (layout tree `tree', NULL: the current
 * page's button of that name) and, recursively, its children.  A name
 * "Base_xxx" uses class Base if it is registered, else the class `name',
 * else a plain RawWindow; the node and tree "Base" (if present) give the
 * window its size and children, the offset between the two nodes moves it.
 * A child named "<name>_focus" is not built: it becomes the focusProperty. */
// FUNC 0x001D74F0 gt2k_menu::RawWindow::loadChild
RawWindow *RawWindow::loadChild(const char *name, const RawWindowArgs *args, ButtonData *tree)
{
    char buf[0x80];
    LinkManager *links = &pages->link;
    MenuLayoutItem *node, *baseNode, *childNode;
    ButtonData *baseTree, *subTree, *child;
    const char *base;
    RawWindowArgs winArgs, childArgs;
    RawWindow *win;
    int i;
    f32 dx, dy;

    if (tree == NULL) {
        tree = links->findButtonData(name);
        if (tree == NULL)
            return NULL;
    }
    node = loader->findAnyItem(name);
    if (node == NULL)
        return NULL;
    base = layoutBaseName(buf, name);
    baseTree = NULL;
    baseNode = NULL;
    if (base != NULL) {
        baseTree = links->findButtonData(base);
        baseNode = loader->findAnyItem(base);
    }
    winArgs.owner = args->owner;
    winArgs.loader = args->loader;
    winArgs.parent = args->parent;
    winArgs.node = (baseNode != NULL) ? baseNode : node;
    winArgs.p0 = 0;
    winArgs.p1 = 0;
    winArgs.p2 = 0;
    winArgs.p3 = 0;
    win = createRegistered((base != NULL) ? base : name, &winArgs);
    if (win == NULL) {
        win = createRegistered(name, &winArgs);
        if (win == NULL)
            win = new RawWindow(winArgs);   /* implicit: Object::operator new (0x1E4 bytes); Object::operator
                                               delete if the constructor throws */
    }
    win->windowName = name;
    subTree = (baseTree != NULL) ? baseTree : tree;
    win->attachTo(this);
    win->attachPosition(this);
    virtual_22(win);
    for (i = 0; i < subTree->subCount; i++) {
        strcpy(buf, name);
        strcat(buf, "_focus");
        child = subTree->subItems[i];
        if (strcmp(buf, child->name) == 0) {
            win->focusProperty = child->name;
        } else {
            childNode = loader->findAnyItem(child->name);
            if (childNode != NULL) {
                childArgs.owner = args->owner;
                childArgs.loader = args->loader;
                childArgs.parent = args->parent;
                childArgs.node = childNode;
                childArgs.p0 = 0;
                childArgs.p1 = 0;
                childArgs.p2 = 0;
                childArgs.p3 = 0;
                win->loadChild(child->name, &childArgs, subTree->subItems[i]);
            }
        }
    }
    if (baseNode != NULL) {
        dx = baseNode->x - node->x;
        dy = baseNode->y - node->y;
        win->setLeft(win->getLeft() + dx);
        win->setTop(win->getTop() + dy);
        win->setWidth(baseNode->w);
        win->setHeight(baseNode->h);
    }
    return win;
}

// FUNC 0x001D77F0 gt2k_menu::RawWindow::virtual_22
void RawWindow::virtual_22(RawWindow *child)
{
}

/* takes the hidden node `name' (if it exists) and reloads the image from it */
// FUNC 0x001D77F8 gt2k_menu::RawWindow::setNode
void RawWindow::setNode(const char *name)
{
    MenuLayoutItem *node = loader->findHiddenItem(name);

    if (node != NULL) {
        defaultItem = node;
        reloadNode(0, 0);
    }
}

/* re-reads defaultItem's image (Drawable's visible off meanwhile); position /
 * size are kept unless takePos / takeSize */
// FUNC 0x001D7838 gt2k_menu::RawWindow::reloadNode
void RawWindow::reloadNode(int takeSize, int takePos)
{
    int wasVisible;
    f32 oldX, oldY, oldW, oldH;

    wasVisible = visible;
    visible = 0;
    oldX = x.get();
    oldY = y.get();
    oldW = w;
    oldH = h;
    setResource(defaultItem, loader->getTexture(defaultItem));
    if (res != NULL)
        appear(res);
    if (takePos == 0) {
        x.set(oldX);
        y.set(oldY);
    }
    if (takeSize == 0) {
        w = oldW;
        h = oldH;
    }
    visible = wasVisible;
}

/* the same with the image of node `name' (nothing reloaded if none) */
// FUNC 0x001D7930 gt2k_menu::RawWindow::changeImage
void RawWindow::changeImage(const char *name, int takeSize, int takePos)
{
    int wasVisible;
    f32 oldX, oldY, oldW, oldH;
    MenuLayoutItem *node;

    oldX = x.get();
    oldY = y.get();
    oldW = w;
    oldH = h;
    wasVisible = visible;
    node = loader->findAnyItem(name);
    visible = 0;
    if (node != NULL) {
        setResource(node, loader->getTexture(node));
        if (res != NULL)
            appear(res);
    }
    if (takePos == 0) {
        x.set(oldX);
        y.set(oldY);
    }
    if (takeSize == 0) {
        w = oldW;
        h = oldH;
    }
    visible = wasVisible;
}

/* no callers */
// FUNC 0x001D7A40 gt2k_menu::RawWindow::changeVertexColors
void RawWindow::changeVertexColors(const char *name)
{
    MenuLayoutItem *node = loader->findHiddenItem(name);

    if (node != NULL)
        setVertexColors(node);
}

// FUNC 0x001D7A78 gt2k_menu::RawWindow::virtual_3
int RawWindow::virtual_3()
{
    return active;
}

/* 1 if this window and every window above it are active.  Tail-recursive:
 * egcs turns the call into a jump without loop notes, so retail rebuilds the
 * dynamic_cast's typeinfo addresses on every pass (a loop hoists them) */
// FUNC 0x001D7A80 gt2k_menu::RawWindow::isShown
int RawWindow::isShown()
{
    RawWindow *parent;

    if (active == 0)
        return 0;
    parent = parentWindow();  /* implicit: __dynamic_cast (inline parentWindow) */
    if (parent != NULL)
        return parent->isShown();
    return 1;
}

// FUNC 0x001D7AF0 gt2k_menu::RawWindow::virtual_23
void RawWindow::virtual_23(int on)
{
    active = on;
}

/* the first descendant (depth first) whose short name is name */
// FUNC 0x001D7AF8 gt2k_menu::RawWindow::findChild
RawWindow *RawWindow::findChild(const char *name)
{
    RawWindow *child, *found;

    for (child = firstChildWindow(); child != NULL; child = child->nextWindow()) {  /* implicit: __dynamic_cast (inline firstChildWindow, nextWindow) */
        if (child->isNamed(name))
            return child;
        found = child->findChild(name);
        if (found != NULL)
            return found;
    }
    return NULL;
}

/* the first descendant whose full name is name */
// FUNC 0x001D7BF8 gt2k_menu::RawWindow::findChildByFullName
RawWindow *RawWindow::findChildByFullName(const char *name)
{
    RawWindow *child, *found;

    for (child = firstChildWindow(); child != NULL; child = child->nextWindow()) {  /* implicit: __dynamic_cast (inline firstChildWindow, nextWindow) */
        if (child->isFullName(name))
            return child;
        found = child->findChildByFullName(name);
        if (found != NULL)
            return found;
    }
    return NULL;
}

/* the part of the window name after the first '_', NULL if none */
// FUNC 0x001D7CF8 gt2k_menu::RawWindow::getShortName
const char *RawWindow::getShortName()
{
    const char *sep = strchr(windowName, '_');

    return (sep != NULL) ? sep + 1 : NULL;
}

// FUNC 0x001D7D28 gt2k_menu::RawWindow::isFullName
int RawWindow::isFullName(const char *name)
{
    return strcmp(name, getWindowName()) == 0;
}

/* 1 if this window or one above it is of class cls (virtual_8) */
// FUNC 0x001D7D50 gt2k_menu::RawWindow::isInClass
int RawWindow::isInClass(const char *cls)
{
    RawWindow *win = this;

    while (strcmp(cls, win->virtual_8()) != 0) {
        win = win->parentWindow();  /* implicit: __dynamic_cast (inline parentWindow) */
        if (win == NULL)
            return 0;
    }
    return 1;
}

// FUNC 0x001D7E10 gt2k_menu::RawWindow::isNamed
int RawWindow::isNamed(const char *name)
{
    const char *shortName = getShortName();

    if (shortName == NULL)
        return 0;
    return strcmp(name, shortName) == 0;
}

/* no callers */
// FUNC 0x001D7E50 gt2k_menu::RawWindow::hasResource
int RawWindow::hasResource(const char *name)
{
    if (res == NULL)
        return 0;
    return strcmp(name, res->name) == 0;
}

/* a registered window of class cls on the node nodeName of args->loader
 * (NULL or not found: args->node), with p0..p3 */
// FUNC 0x001D7E88 gt2k_menu::createWindow
RawWindow *createWindow(const char *cls, const char *nodeName, const RawWindowArgs *args,
                        int p0, int p1, int p2, int p3)
{
    RawWindowArgs winArgs;
    MenuLayoutItem *node = args->node;
    MenuLayoutItem *named;

    if (nodeName != NULL) {
        named = args->loader->findHiddenItem(nodeName);
        if (named != NULL)
            node = named;
    }
    winArgs.owner = args->owner;
    winArgs.loader = args->loader;
    winArgs.parent = args->parent;
    winArgs.node = node;
    winArgs.p0 = p0;
    winArgs.p1 = p1;
    winArgs.p2 = p2;
    winArgs.p3 = p3;
    return createRegistered(cls, &winArgs);
}

/* ---- accessors (inline members, out-of-line copies; no callers) ---- */

// FUNC 0x001D7F38 gt2k_menu::RawWindow::setWindowName(const char*)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D7F40 gt2k_menu::RawWindow::getWindowName(void)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D7F48 gt2k_menu::RawWindow::getMenuControl(void)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D7F50 gt2k_menu::RawWindow::getLastOrigin
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D7F58 gt2k_menu::RawWindow::nextWindow
/* implicit: __dynamic_cast; inline in game/menu/framework/T264_raw_window.h (dynamic_cast<RawWindow *>(sibling));
 * egcs emits this copy */

// FUNC 0x001D7FB0 gt2k_menu::RawWindow::firstChildWindow
/* implicit: __dynamic_cast; inline in game/menu/framework/T264_raw_window.h (dynamic_cast<RawWindow *>(child));
 * egcs emits this copy */

// FUNC 0x001D8008 gt2k_menu::RawWindow::parentWindow
/* implicit: __dynamic_cast; inline in game/menu/framework/T264_raw_window.h (dynamic_cast<RawWindow *>(parent));
 * egcs emits this copy */

// FUNC 0x001D8060 gt2k_menu::RawWindow::setClipChildren(bool)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8068 gt2k_menu::RawWindow::changeTexture(const char*)
/* implicit: changeImage (the inline body); inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8088 gt2k_menu::RawWindow::setDefaultItem(const MenuItem&)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8090 gt2k_menu::RawWindow::getDefaultItem(void)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8098 gt2k_menu::RawWindow::getStartItem(void)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D80A0 gt2k_menu::RawWindow::setCY
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D80B8 gt2k_menu::RawWindow::setCX
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D80D0 gt2k_menu::RawWindow::getRH(void) const
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D80D8 gt2k_menu::RawWindow::getRW(void) const
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D80E0 gt2k_menu::RawWindow::getCY(void) const
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D80E8 gt2k_menu::RawWindow::getCX(void) const
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D80F0 gt2k_menu::RawWindow::setChildrenFocusChange(bool)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D80F8 gt2k_menu::RawWindow::getFocusProperty(void)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8100 gt2k_menu::RawWindow::setFocusProperty
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8108 gt2k_menu::RawWindow::getFocus(void)
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8110 gt2k_menu::RawWindow::setUnk17C
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8118 gt2k_menu::RawWindow::getMoveY
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8120 gt2k_menu::RawWindow::getMoveX
/* inline in game/menu/framework/T264_raw_window.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x001D8128 __tfQ29gt2k_menu9RawWindow
extern "C" void *__tfQ29gt2k_menu9RawWindow(void)
{
    if (__tiQ29gt2k_menu9RawWindow[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu9RawWindow, "Q29gt2k_menu9RawWindow",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu9RawWindow;
}
#endif
