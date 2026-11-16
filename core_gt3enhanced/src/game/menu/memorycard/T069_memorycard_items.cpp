/* T069 - the items of the memory card screens ("menu/memorycard.mbl").
 * 0x0014B610..0x0014E880.
 *
 * The texts of the file information pages (create_callback_text callbacks:
 * ctl, buf, size; an empty text when there is nothing to show), the hook
 * table MemorycardControl::begin (T068) hands to the page layouts, and the
 * five item classes SlotTitle, TitleView, ProgressBar, SingleIconView and
 * IconView.  The operator new / delete of every `new' is Object's (the menu
 * heap, 0x001BF3C0 / 0x001BF458); the landing pads in the binary only free
 * the object (and destroy what is built) if a constructor throws.
 *
 * Layout of the object (ref/matching/u027/notes.md, ref/data/u027/notes.md
 * have the evidence):
 *   - the five classes are defined here, as in the original (no other
 *     original saw them), each with its member bodies in the class and
 *     followed by its factory, in the order IconView, SingleIconView,
 *     ProgressBar, TitleView, SlotTitle (their type names come out in
 *     reverse).  egcs writes their strings first ("" in .sdata,
 *     "active_progress_bar" in .rodata) and emits the functions at the end of
 *     the unit, after the static initialiser: the factories first (what the
 *     initialiser references, in reverse definition order), then per class
 *     (SlotTitle's first) its virtual functions in reverse order, its
 *     typeinfo function and its implicit destructor, once the vtables are
 *     written; drawIcon (called out of line by IconView::virtual_2) last.
 *     That order is retail's .text, frame info and .lit4;
 *     T069's copy of MemorycardSlot::getManager (0x0014DAE0) comes out
 *     after scrollTarget: it is defined (inline) before IconView;
 *   - the texts (address order), with the title table before them and the
 *     licence letters after memcard_text_size, as their strings show;
 *   - the entry count, the hook table (zeros in .data, before the frame
 *     info) and the static initialiser. */

/* The headers in the order of the classes the original saw: its .rodata
 * has, after the five classes' type names (0x0031CFE0..0x0031D05F), those of
 * MemorycardControl, List / MemorycardMenuItem / Cell (T073),
 * MemorycardFileManagerBase, List / Cell<MemorycardFile>, SmartIndexT<int>,
 * the OperatorHolder group, the garage's lists, RegurationResult and
 * Operator (0x0031D068..0x0031D518): the reverse of the order below.  No
 * names of T075's or T076's classes (MemorycardControl without the screens
 * of T075_memorycard_game_slots.h). */
#include "game/menu/gtmode/T045_reguration_result.h"    /* RegurationResult (its type name only) */
#include "game/save/T429_game_status.h"          /* the garage's classes (their type names only) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, MenuTexture; the OperatorHolder group */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager; SmartIndexT<int> */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* gt2k_menu::MemorycardFileManagerBase */
#include "game/menu/memorycard/T073_memorycard_menu.h"      /* MemorycardMenuItem (its type names only) */
#include "game/menu/memorycard/T068_memorycard_control_class.h" /* gt2k_menu::MemorycardControl */
#include "game/menu/memorycard/memorycard_items_internal.h"

using namespace gt2k_menu;

/* ======================================================================== */
/* MemorycardSlot's manager                                                 */
/* ======================================================================== */

/* What T069 asks a slot's manager.  Every call of MemorycardSlot::getManager
 * in T069 (as in T068 and T071) is a `jal' followed by the manager's method:
 * the original's accessors were inline members of MemorycardSlot compiled
 * before getManager's body, so they kept the call (egcs expands a call only
 * once the callee's body is compiled, and an inline function expanded later
 * is copied as it was compiled).  These static inline helpers are that, an
 * exact form, not the original (u026's notes: T068's slot_port).  The
 * cursor's row and the last entry's row (MemorycardSlot::getLastRow,
 * 0x00150468, is the out-of-line copy of the second) are inline in
 * IconView::scrollTarget. */
static inline int slot_icon_columns(MemorycardSlot *s)
{
    return s->getManager()->getXIconNum();
}

static inline void slot_set_screen_size(MemorycardSlot *s, int n)
{
    s->getManager()->setScreenSize(n);
}

static inline bool slot_is_connect(MemorycardSlot *s)
{
    return s->getManager()->isConnect();
}

static inline int slot_cursor_row(MemorycardSlot *s)
{
    return s->cursor / s->getManager()->getXIconNum();
}

static inline int slot_last_row(MemorycardSlot *s)
{
    int entries = s->getCount();

    return (entries - 1) / s->getManager()->getXIconNum();
}

/* MemorycardSlot::getManager, an inline accessor of T071's MemorycardSlot:
 * T069's out-of-line copy (no caller binds to it: the linker keeps T068's
 * 0x0014B608, the first of the weak copies; T071's is 0x00150460, u028's
 * F_00150460).  `inline' and defined here,
 * after the helpers that keep their calls and before IconView, for egcs:
 * its copy then comes out with the deferred functions right after
 * scrollTarget, as retail's 0x0014DAE0.  T071's header declares it without
 * `inline' (the other units call it, T068 defines it); GCC 14 does without
 * this copy. */
#ifdef BOOT_OLD_GCC
namespace gt2k_menu {
// FUNC 0x0014DAE0 gt2k_menu::MemorycardSlot::getManager
inline MemorycardFileManagerBase *MemorycardSlot::getManager()
{
    return manager;
}
} /* namespace gt2k_menu */
#endif

/* ======================================================================== */
/* IconView                                                                 */
/* ======================================================================== */

namespace gt2k_menu {

/* vtable 0x0031D4B8.  typeinfo __tfQ29gt2k_menu8IconView 0x0014E590 (si,
 * base Item; storage 0x004B1A00).  Size 0xF70.  The icons of the files of
 * one card slot, cols per row (the manager's getXIconNum()), scrolled to
 * keep the cursor's row in view; the cursor's icon is drawn last, with a
 * blinking frame; "NO MEMORY CARD(PS2)" without a card.  The members in
 * the order egcs compiles them: the constructor (expanded in its factory)
 * calls scrollTarget out of line, virtual_1 expands it, virtual_2 calls
 * drawIcon out of line. */
class IconView : public Item {
public:
    /* The grid: icons of the list icon size, `cols' (getXIconNum()) per row
     * with equal gaps across the element's width; the same pitch down (from
     * the width, sic); as many rows as fit, + 2.  The view starts at row 0,
     * the manager is told how many entries are shown, the scroll starts at
     * its target.  baseColor is first Color4f() (0.502, gp-0x7E00), then the
     * layout's colour. */
    IconView(MenuLayoutItem *elem, MenuTexture *image, PageManager *p, MemorycardControl *c,
             MemorycardSlot *s, PageLayout *file)
        : Item(elem, image), pages(p), ctl(c), slot(s), sprites(*file), scroll(), baseColor()
    {
        int perRow;
        f32 gap;

        setText("");                        /* 0x003517A0 */
        baseColor = color[0];
        perRow = slot_icon_columns(s);
        iconW = sprites.sprites[1].getW();  /* +0x244 */
        cols = perRow;
        iconH = sprites.sprites[1].getH();
        gap = (elem->w - iconW * (f32)perRow) / (f32)(perRow + 1);
        pitchX = gap + iconW;
        originX = gap + iconW * 0.5f;
        pitchY = gap + iconW;
        originY = gap + iconW * 0.5f;
        rows = (int)(elem->h / (gap + iconW)) + 2;
        slot->setTopRow(0);
        {
            int n = cols * rows;

            slot_set_screen_size(slot, n);
        }
        scroll.set(scrollTarget());
        time = 0.0f;
        blink = 0.0f;
    }
    /* [0] +0x0C 0x0014E5E0 ~IconView: implicit (no vptr store) */

    /* The scroll that centres the cursor's row (row * pitchY), at most that
     * of the last row at the bottom edge, at least that of the first row at
     * the top edge (which wins when the list is shorter than the view).
     * virtual_1 has it inline; this out-of-line copy is the constructor's. */
    // FUNC 0x0014D9B0 gt2k_menu::IconView::scrollTarget
    f32 scrollTarget()
    {
        f32 target = (f32)slot_cursor_row(slot) * pitchY;
        f32 first = getH() * 0.5f - originY;
        f32 last = (f32)slot_last_row(slot) * pitchY - getH() * 0.5f + originY;

        /* implicit: getManager, getCount (slot_cursor_row, slot_last_row) */

        if (last < target)
            target = last;
        if (target < first)
            target = first;
        return target;
    }

    /* update: Item's; the animation clock; the scroll eases (0.3) to its
     * target and the slot's top row follows the target; the colours dim to
     * 30% without the focus; the cursor frame fades from 1 to 0 in 30
     * frames, again and again. */
    // FUNC 0x0014E338 gt2k_menu::IconView::virtual_1
    virtual void virtual_1()                /* [1] +0x14 */
    {
        f32 target, dim;

        Item::virtual_1();
        time += 1.0f / 60.0f;               /* 0x3C888889 */
        if (86400.0f < time)
            time = 0.0f;
        target = scrollTarget();            /* implicit: getManager, getCount (scrollTarget, inline) */
        scroll.approach(target, 0.3f);
        slot->setTopRow((int)((f32)(int)(target - getH() * 0.5f) / pitchY));
        dim = 1.0f;
        if (slot->focus == 0)
            dim = 0.3f;
        const Color4f &base = baseColor;
        f32 r = base.r * dim, g = base.g * dim, b = base.b * dim, a = base.a;
        color[0].set(r, g, b, a);
        color[1].set(r, g, b, a);
        color[2].set(r, g, b, a);
        color[3].set(r, g, b, a);
        blink -= 1.0f / 30.0f;              /* 0x3D088889 */
        if (blink < 0.0f)
            blink = 1.0f;
    }

    /* draw: Item's.  Without a card: the text "NO MEMORY CARD(PS2)" and no
     * icons.  Else the visible rows (from the slot's top row, `rows' of them)
     * clipped to the element's band; with the focus, the cursor's entry is
     * drawn last (as the cursor, with a count that is never reached). */
    // FUNC 0x0014E158 gt2k_menu::IconView::virtual_2
    virtual void virtual_2()                /* [2] +0x1C */
    {
        MemorycardIconRenderInfo info;
        MemorycardSlot *s;
        int focus, cursor, count, start, end, i;
        f32 x0, y0;

        Item::virtual_2();
        if (!slot_is_connect(slot)) {      /* implicit: getManager (the inline slot_is_connect) */
            setText(g_002B1C68[MCTEXT_NO_MEMORYCARD]);
            return;
        }
        setText("");                        /* 0x003517A0 */
        menu_clip_row(y.get(), h);
        info.y = 0.0f;
        info.x = 0.0f;
        info.a = 1.0f;
        info.b = 1.0f;
        info.g = 1.0f;
        info.r = 1.0f;
        info.useSlot0 = 0;
        info.time = time;
        info.w = iconW;
        info.h = iconH;
        s = slot;
        focus = s->focus;
        x0 = x.value - w * 0.5f + originX;
        cursor = s->cursor;
        y0 = y.get() - scroll.get();
        info.sprites = &sprites;
        count = s->getCount();
        start = slot->topRow * cols;
        end = start + cols * rows;
        for (i = start; i < end; i++) {
            if (focus == 0 || i != cursor)
                drawIcon(i, x0, y0, &info, focus, 0, count);
        }
        if (focus != 0)
            drawIcon(cursor, x0, y0, &info, focus, 1, ICON_COUNT_ANY);
        menu_clip_full();
    }

    /* Entry i of the grid at column i & (cols - 1) (sic: cols taken as a
     * power of 2) and row i / cols.  The cursor's place is kept for the icon
     * views of the next pages (iconPos).  Entries of the list are grey 0.5
     * when selectable, else 0.25 (past the end of the list: 0.25,
     * "icon_blank"), times 0.3 without the focus, alpha 0.5.  The cursor gets
     * the blinking frame ("icon_cursor", drawn additively) unless the
     * operation menu is open.  The floats second: virtual_2 sets $f12 / $f13
     * right after $a1 (the EE ABI gives the same registers in any order; the
     * moves follow the parameter order). */
    // FUNC 0x0014E680 gt2k_menu::IconView::drawIcon
    void drawIcon(int i, f32 x0, f32 y0, MemorycardIconRenderInfo *info, int focus, int isCursor,
                  int count)                /* (focus: the slot's) */
    {
        int perRow = cols;
        f32 ix, iy, grey;
        int inList;

        ix = x0 + (f32)(i & (perRow - 1)) * pitchX;
        iy = y0 + (f32)(i / perRow) * pitchY;
        info->x = ix;
        info->y = iy;
        if (isCursor) {
            ctl->iconPos[ICONPOS_CURSOR_X] = ix;
            ctl->iconPos[ICONPOS_CURSOR_Y] = iy;
        }
        inList = i < count;
        if (inList && slot->isFileSelectable(i) != 0)
            grey = 0.5f;
        else
            grey = 0.25f;
        if (focus == 0)
            grey *= 0.3f;
        info->r = grey;
        info->g = grey;
        info->b = grey;
        info->a = 0.5f;
        if (inList) {
            slot->renderFile(i, *info);
        } else {
            sprites.setColor(grey, grey, grey, 0.5f);
            sprites.setSize(iconW, iconH);
            sprites.setPosition(ix, iy);
            sprites.draw(MC_ICON_BLANK);
        }
        if (isCursor && slot->menuOpen == 0) {
            gfx_blend_func(MC_BLEND_ADD, 1.0f);
            sprites.setColor(1.0f, 1.0f, 1.0f, blink * 0.25f);
            sprites.setSize(iconW, iconH);
            sprites.setPosition(ix, iy);
            sprites.draw(MC_ICON_CURSOR);
            gfx_blend_func(MC_BLEND_ALPHA, 1.0f);
        }
    }

    PageManager          *pages;            /* 0x100 */
    MemorycardControl    *ctl;              /* 0x104 */
    MemorycardSlot       *slot;             /* 0x108 */
    MemorycardIconSprites sprites;          /* 0x10C */
    f32                   time;             /* 0xF2C animation time (s), wraps after a day */
    MotionValue           scroll;           /* 0xF30 y offset of the first row */
    f32                   iconW;            /* 0xF3C sprites[1]'s size */
    f32                   iconH;            /* 0xF40 */
    f32                   pitchX;           /* 0xF44 gap + iconW */
    f32                   pitchY;           /* 0xF48 gap + iconW (sic: the width) */
    f32                   originX;          /* 0xF4C gap + iconW / 2 */
    f32                   originY;          /* 0xF50 the same */
    int                   cols;             /* 0xF54 icons per row */
    int                   rows;             /* 0xF58 rows drawn: the rows that fit + 2 */
    f32                   blink;            /* 0xF5C the cursor frame's alpha / 0.25: 1 -> 0 in 30 frames */
    Color4f               baseColor;        /* 0xF60 the layout's colour (color[0]) */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::IconView, 0xF70);

/* "icon_view_slot1" (p8 0), "icon_view_slot2" (p8 1): the texture looked up
 * before the allocation, operator new 0xF70, the inline constructor
 * (Item::Item(item, tex), MemorycardIconSprites(*layout), Item::setText("")
 * ...).  ctx is the MemorycardControl (PageLayout::setHooks in
 * MemorycardControl::begin). */
// FUNC 0x0014D638 memcard_create_icon_view
inline Transitive *memcard_create_icon_view(void *ctx, PageLayout *layout, Object *parent,
                                            MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MemorycardControl *ctl = (MemorycardControl *)ctx;
    PageManager *pages = &ctl->pages;
    MenuTexture *tex = layout->getTexture(item);
    IconView *view = new IconView(item, tex, pages, ctl, ctl->slots.getSlot(p8), layout);
    /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item, MemorycardIconSprites, setText (the inline IconView constructor) */
    /* implicit: getManager, setTopRow, scrollTarget (the inline IconView constructor) */

    return view;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0014E590 __tfQ29gt2k_menu8IconView
extern "C" void *__tfQ29gt2k_menu8IconView(void)
{
    if (__tiQ29gt2k_menu8IconView[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu8IconView, "Q29gt2k_menu8IconView",      /* 0x0031D050 */
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu8IconView;
}
#endif

// FUNC 0x0014E5E0 gt2k_menu::IconView::~IconView
/* implicit (no vptr store); egcs emits it for the vtable: sprites (its 14
 * Items in reverse order, ~Item through their vtables), Item::~Item (flags
 * 0), then Object::operator delete if flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

/* ======================================================================== */
/* SingleIconView                                                           */
/* ======================================================================== */

namespace gt2k_menu {

/* vtable 0x0031D470.  typeinfo __tfQ29gt2k_menu14SingleIconView 0x0014E108
 * (si, base Item; storage 0x004B19D0).  Size 0xF44.  The selected file's
 * icon on the operation pages: kind 0 "op_icon_face", 1
 * "confirm_icon_face", 2 "working_icon_face" (with a clone of the file of
 * its own).  It flies from where the previous page showed it to its place
 * (MemorycardControl::iconPos) and grows to the element's size. */
class SingleIconView : public Item {
public:
    /* Where the icon starts: the "op" view at the list cursor's icon (at the
     * element's size if that is already the "op" place, else at the list
     * icon size), the "confirm" view at the "op" place, the "working" view
     * at the "confirm" place (other kinds: not set).  The icon is the
     * current slot's selected entry if the slot has the focus; the
     * "working" view takes a clone of it.  sprites(*file). */
    SingleIconView(MenuLayoutItem *elem, MenuTexture *image, MemorycardControl *c,
                   PageLayout *file, int k)
        : Item(elem, image), ctl(c), sprites(*file)
    {
        MemorycardSlot *cur;
        f32 opX, opY;

        icon = NULL;
        ownsIcon = 0;
        kind = k;
        switch (k) {
        case ICON_FACE_OP:
            opX = ctl->iconPos[ICONPOS_OP_X];
            opY = ctl->iconPos[ICONPOS_OP_Y];
            iconX = ctl->iconPos[ICONPOS_CURSOR_X];
            iconY = ctl->iconPos[ICONPOS_CURSOR_Y];
            if (iconX == opX && iconY == opY) {
                iconW = w;
                iconH = h;
            } else {
                iconW = sprites.sprites[1].getW();  /* +0x23C */
                iconH = sprites.sprites[1].getH();
            }
            break;
        case ICON_FACE_CONFIRM:
            iconX = ctl->iconPos[ICONPOS_OP_X];
            iconY = ctl->iconPos[ICONPOS_OP_Y];
            iconW = w;
            iconH = h;
            break;
        case ICON_FACE_WORKING:
            iconX = ctl->iconPos[ICONPOS_CONFIRM_X];
            iconY = ctl->iconPos[ICONPOS_CONFIRM_Y];
            iconW = w;
            iconH = h;
            break;
        }
        time = 0.0f;
        cur = ctl->getCurrentSlot();
        if (cur != NULL && cur->focus != 0) {
            MemorycardFile *file = cur->getFile(cur->cursor);

            if (file != NULL && kind == ICON_FACE_WORKING) {
                icon = file->clone();
                ownsIcon = 1;
            } else {
                icon = file;
                ownsIcon = 0;
            }
        }
    }

    /* An owned icon (the "working" view's clone) is released (noWait) and
     * deleted; then the implicit part: sprites (its 14 Items in reverse
     * order, ~Item through their vtables), Item::~Item (flags 0), and
     * Object::operator delete if flags & 1. */
    // FUNC 0x0014E010 gt2k_menu::SingleIconView::~SingleIconView
    virtual ~SingleIconView()               /* [0] +0x0C */
    {
        if (icon != NULL && ownsIcon != 0) {
            icon->releaseIcon(true);
            delete icon;                    /* virtual, flags 3 */
        }
        /* implicit: operator delete (the deleting destructor, flags & 1) */
    }

    /* update: Item's, the animation clock (1/60 s a frame, wraps after a
     * day), and the rectangle eases (0.1) to the view's place: the "op"
     * place for the "op" view, the "confirm" place for the other two (else
     * (0, 0)), at the element's size. */
    // FUNC 0x0014DF18 gt2k_menu::SingleIconView::virtual_1
    virtual void virtual_1()                /* [1] +0x14 */
    {
        f32 tx, ty;

        Item::virtual_1();
        time += 1.0f / 60.0f;               /* 0x3C888889 */
        if (86400.0f < time)
            time = 0.0f;
        tx = 0.0f;
        ty = 0.0f;
        switch (kind) {
        case ICON_FACE_OP:
            tx = ctl->iconPos[ICONPOS_OP_X];
            ty = ctl->iconPos[ICONPOS_OP_Y];
            break;
        case ICON_FACE_CONFIRM:
        case ICON_FACE_WORKING:
            tx = ctl->iconPos[ICONPOS_CONFIRM_X];
            ty = ctl->iconPos[ICONPOS_CONFIRM_Y];
            break;
        }
        iconX += (tx - iconX) * 0.1f;
        iconY += (ty - iconY) * 0.1f;
        iconW += (w - iconW) * 0.1f;
        iconH += (h - iconH) * 0.1f;
    }

    /* draw: Item's, then the icon at the view's animated rectangle, grey
     * (0.5), with half the view's alpha, once that is above 1/128.  A
     * borrowed icon is dropped as soon as it is no longer the selected
     * file. */
    // FUNC 0x0014DE38 gt2k_menu::SingleIconView::virtual_2
    virtual void virtual_2()                /* [2] +0x1C */
    {
        MemorycardIconRenderInfo info;
        f32 alpha;

        Item::virtual_2();
        alpha = alphaRatio * 0.5f;
        info.sprites = &sprites;
        info.time = time;
        info.useSlot0 = kind != ICON_FACE_OP;
        info.w = iconW;
        info.h = iconH;
        info.x = iconX;
        info.y = iconY;
        info.r = 0.5f;
        info.g = 0.5f;
        info.b = 0.5f;
        info.a = alpha;
        if (ownsIcon == 0 && ctl->getSelectedFile() != icon)
            icon = NULL;
        if (0.0078125f < alpha && icon != NULL)
            icon->render(info);
    }

    MemorycardControl    *ctl;              /* 0x100 */
    MemorycardIconSprites sprites;          /* 0x104 */
    MemorycardFile       *icon;             /* 0xF24 the file whose icon is drawn */
    int                   ownsIcon;         /* 0xF28 icon is a clone (kind 2), deleted with the view */
    f32                   iconX;            /* 0xF2C */
    f32                   iconY;            /* 0xF30 */
    f32                   iconW;            /* 0xF34 */
    f32                   iconH;            /* 0xF38 */
    f32                   time;             /* 0xF3C animation time (s), wraps after a day */
    int                   kind;             /* 0xF40 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::SingleIconView, 0xF44);

/* "op_icon_face" (p8 0), "confirm_icon_face" (1), "working_icon_face" (2):
 * operator new 0xF44, the inline constructor (Item::Item(item, tex), ctl,
 * MemorycardIconSprites(*layout): the layout copied to the stack first). */
// FUNC 0x0014D2F0 memcard_create_icon_face
inline Transitive *memcard_create_icon_face(void *ctx, PageLayout *layout, Object *parent,
                                            MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MemorycardControl *ctl = (MemorycardControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    SingleIconView *view = new SingleIconView(item, tex, ctl, layout, p8);
    /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item, MemorycardIconSprites (the inline SingleIconView constructor) */
    /* implicit: getCurrentSlot, getFile (the inline SingleIconView constructor) */

    return view;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0014E108 __tfQ29gt2k_menu14SingleIconView
extern "C" void *__tfQ29gt2k_menu14SingleIconView(void)
{
    if (__tiQ29gt2k_menu14SingleIconView[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu14SingleIconView, "Q29gt2k_menu14SingleIconView", /* 0x0031D030 */
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu14SingleIconView;
}
#endif

/* ======================================================================== */
/* ProgressBar                                                              */
/* ======================================================================== */

namespace gt2k_menu {

/* vtable 0x0031D428 ([2] Item's draw).  typeinfo __tfQ29gt2k_menu11ProgressBar
 * 0x0014DDA0 (si, base Item; storage 0x004B19F0).  Size 0x10C.  The element
 * is the frame; bar ("active_progress_bar", a child anchored to it) grows
 * from the left edge to width * progress. */
class ProgressBar : public Item {
public:
    /* bar: "active_progress_bar" of the layout (an Item, 0x100 bytes),
     * width 0, one step in front of the frame, a child of it, anchored to
     * it. */
    ProgressBar(MenuLayoutItem *elem, MenuTexture *image, MemorycardControl *c, PageLayout *file)
        : Item(elem, image), ctl(c)
    {
        bar = new Item("active_progress_bar", file);    /* 0x0031CBA0 */
        bar->w = 0.0f;
        bar->h = h;
        bar->x.set(w * 0.5f);
        bar->y.set(0.0f);
        bar->z = z + 1.0f;
        bar->attachTo(this);
        progress = 0.0f;
        bar->anchorType = ANCHOR_XY;
        bar->anchor = this;
    }
    /* [0] +0x0C 0x0014DDF0 ~ProgressBar: implicit (no vptr store; bar is the
     * frame's child: not deleted here) */

    /* update (no Item::virtual_1 here).  The shown progress eases towards
     * the one reported last (MemorycardControl::progress): fast (0.4) while
     * it is still below the report before (progressPrev), slowly (0.01)
     * after; it snaps to it once both reports are equal and >= 0.9999
     * (done).  The bar keeps its left edge on the frame's and takes the
     * frame's alpha. */
    // FUNC 0x0014DCF8 gt2k_menu::ProgressBar::virtual_1
    virtual void virtual_1()                /* [1] +0x14 */
    {
        f32 shown = progress;
        f32 prev = ctl->progressPrev;
        f32 reported = ctl->progress;

        if (shown < prev)
            progress = shown + (reported - shown) * 0.4f;
        else
            progress = shown + (reported - shown) * 0.01f;
        if (prev == reported && 0.9999f <= prev)
            progress = reported;
        bar->w = w * progress;
        bar->x.set((bar->w - w) * 0.5f);
        bar->alphaRatio = alphaRatio;
    }

    Item              *bar;                 /* 0x100 */
    f32                progress;            /* 0x104 shown, 0..1 */
    MemorycardControl *ctl;                 /* 0x108 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::ProgressBar, 0x10C);

/* "progress_bar": operator new 0x10C, the inline constructor
 * (Item::Item(item, tex), then Item::Item("active_progress_bar", layout)
 * for the bar). */
// FUNC 0x0014D168 memcard_create_progress_bar
inline Transitive *memcard_create_progress_bar(void *ctx, PageLayout *layout, Object *parent,
                                               MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MemorycardControl *ctl = (MemorycardControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    ProgressBar *bar = new ProgressBar(item, tex, ctl, layout);
    /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item, attachTo (the inline ProgressBar constructor: bar's new-expression, bar->attachTo) */

    return bar;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0014DDA0 __tfQ29gt2k_menu11ProgressBar
extern "C" void *__tfQ29gt2k_menu11ProgressBar(void)
{
    if (__tiQ29gt2k_menu11ProgressBar[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu11ProgressBar, "Q29gt2k_menu11ProgressBar",   /* 0x0031D010 */
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu11ProgressBar;
}
#endif

// FUNC 0x0014DDF0 gt2k_menu::ProgressBar::~ProgressBar
/* implicit (no vptr store); egcs emits it for the vtable: Item::~Item
 * (flags 0), then Object::operator delete if flags & 1.  The bar is not
 * deleted here (it is a child of the frame). */
/* implicit: operator delete (compiler-generated destructor) */

/* ======================================================================== */
/* TitleView                                                                */
/* ======================================================================== */

namespace gt2k_menu {

/* vtable 0x0031D3E0 ([1] Item's update).  typeinfo __tfQ29gt2k_menu9TitleView
 * 0x0014DC50 (si, base Item; storage 0x004B19C0).  Size 0x200.  The screen
 * title: the image "title_<save / load>_<replay / ghost / setting / game>"
 * or "title_file_operation" of the mode (g_002A4DD8; mode 0x0C as 0x11) in
 * the element's place and size; the element's own image is not drawn. */
class TitleView : public Item {
public:
    /* Defined below, after title_view_new (pages: not used). */
    inline TitleView(MenuLayoutItem *elem, MenuTexture *image, PageManager *pages, PageLayout *file,
                     MemorycardControl *ctl);
    /* [0] +0x0C 0x0014DCA0 ~TitleView: implicit (no vptr store) */

    /* draw: the title only (Item::virtual_2 called directly), not the
     * element's own image. */
    // FUNC 0x0014DC30 gt2k_menu::TitleView::virtual_2
    virtual void virtual_2()                /* [2] +0x1C */
    {
        title.virtual_2();
    }

    Item title;                             /* 0x100 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::TitleView, 0x200);

/* The new-expression of the factory.  Retail's factory calls the
 * constructor (0x0014D040, right after the factory) although egcs had
 * compiled its body before the factory's (the constructor comes out in the
 * same pass, after the factory): the factory expanded something compiled
 * before the constructor's body, which kept the call.  This static inline
 * helper is that, an exact form, not the original. */
static inline TitleView *title_view_new(MenuLayoutItem *elem, MenuTexture *image, PageManager *pages,
                                        PageLayout *file, MemorycardControl *ctl)
{
    return new TitleView(elem, image, pages, file, ctl);
}

namespace gt2k_menu {

/* The title image of the mode (g_002A4DD8 from MODE_REPLAY_SAVE on; the
 * demo replays as MODE_REPLAY_LOAD) in the element's place and size. */
// FUNC 0x0014D040 gt2k_menu::TitleView::TitleView
inline TitleView::TitleView(MenuLayoutItem *elem, MenuTexture *image, PageManager *pages,
                            PageLayout *file, MemorycardControl *ctl)
    : Item(elem, image), title()
{
    int mode = ctl->mode == MemorycardControl::MODE_DEMO_REPLAY_LOAD
             ? MemorycardControl::MODE_REPLAY_LOAD : ctl->mode;

    title.setResource(g_002A4DD8[mode - MemorycardControl::MODE_REPLAY_SAVE], file);
    title.setX(x.value);
    title.setY(y.value);
    title.setW(w);
    title.setH(h);
}

} /* namespace gt2k_menu */

/* "title_base": operator new 0x200, the constructor out of line. */
// FUNC 0x0014CF90 memcard_create_title_view
inline Transitive *memcard_create_title_view(void *ctx, PageLayout *layout, Object *parent,
                                             MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MemorycardControl *ctl = (MemorycardControl *)ctx;
    PageManager *pages = &ctl->pages;
    MenuTexture *tex = layout->getTexture(item);
    TitleView *view = title_view_new(item, tex, pages, layout, ctl);
    /* implicit: new, TitleView (the inline title_view_new: operator new, the constructor) */
    /* implicit: Object::operator delete (if the constructor throws) */

    return view;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0014DC50 __tfQ29gt2k_menu9TitleView
extern "C" void *__tfQ29gt2k_menu9TitleView(void)
{
    if (__tiQ29gt2k_menu9TitleView[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu9TitleView, "Q29gt2k_menu9TitleView",    /* 0x0031CFF8 */
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu9TitleView;
}
#endif

// FUNC 0x0014DCA0 gt2k_menu::TitleView::~TitleView
/* implicit (no vptr store); egcs emits it for the vtable: title.~Item()
 * (flags 2), Item::~Item (flags 0), then Object::operator delete if
 * flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

/* ======================================================================== */
/* SlotTitle                                                                */
/* ======================================================================== */

namespace gt2k_menu {

/* vtable 0x0031D398 ([2] Item's draw).  typeinfo __tfQ29gt2k_menu9SlotTitle
 * 0x0014DB98 (si, base Item; storage 0x004B19E0).  Size 0x114.  The title
 * bar of a card slot: the layout's colours, dimmed to 30% while the slot
 * does not have the focus. */
class SlotTitle : public Item {
public:
    /* baseColor is first Color4f() (0.502, gp-0x7E04), then the layout's
     * colour. */
    SlotTitle(MenuLayoutItem *elem, MenuTexture *image, MemorycardSlot *s)
        : Item(elem, image), baseColor(), slot(s)
    {
        baseColor = color[0];
    }
    /* [0] +0x0C 0x0014DBE8 ~SlotTitle: implicit (no vptr store) */

    /* update: Item's, then the four vertex colours: the layout's, 30% while
     * the slot does not have the focus (alpha kept). */
    // FUNC 0x0014DAE8 gt2k_menu::SlotTitle::virtual_1
    virtual void virtual_1()                /* [1] +0x14 */
    {
        f32 dim;

        Item::virtual_1();
        dim = 1.0f;
        if (slot->focus == 0)
            dim = 0.3f;
        const Color4f &base = baseColor;
        f32 r = base.r * dim, g = base.g * dim, b = base.b * dim, a = base.a;
        color[0].set(r, g, b, a);
        color[1].set(r, g, b, a);
        color[2].set(r, g, b, a);
        color[3].set(r, g, b, a);
    }

    /*              Item                       0x000 .. 0x100 (vptr +0x1C) */
    Color4f         baseColor;              /* 0x100 the layout's colour (color[0]) */
    MemorycardSlot *slot;                   /* 0x110 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::SlotTitle, 0x114);

/* "title_bar_slot1" (p8 0), "title_bar_slot2" (p8 1): operator new 0x114,
 * the inline constructor (Item::Item(item, tex), the vptr, slot,
 * baseColor). */
// FUNC 0x0014CE88 memcard_create_slot_title
inline Transitive *memcard_create_slot_title(void *ctx, PageLayout *layout, Object *parent,
                                             MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MemorycardControl *ctl = (MemorycardControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    SlotTitle *title = new SlotTitle(item, tex, ctl->slots.getSlot(p8));
    /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (the inline SlotTitle constructor) */

    return title;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0014DB98 __tfQ29gt2k_menu9SlotTitle
extern "C" void *__tfQ29gt2k_menu9SlotTitle(void)
{
    if (__tiQ29gt2k_menu9SlotTitle[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu9SlotTitle, "Q29gt2k_menu9SlotTitle",    /* 0x0031CFE0 */
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu9SlotTitle;
}
#endif

// FUNC 0x0014DBE8 gt2k_menu::SlotTitle::~SlotTitle
/* implicit (no vptr store); egcs emits it for the vtable: Item::~Item
 * (flags 0), then Object::operator delete if flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

/* ======================================================================== */
/* The texts                                                                */
/* ======================================================================== */

/* The title images of the modes from MODE_REPLAY_SAVE on (TitleView; the
 * strings at 0x0031CBB8..0x0031CC70, after "active_progress_bar"). */
const char *g_002A4DD8[11] = {             /* 0x002A4DD8 */
    "title_save_replay", "title_load_replay", "title_save_ghost", "title_load_ghost",
    "title_save_ghost", "title_load_ghost", "title_save_setting", "title_load_setting",
    "title_file_operation", "title_save_game", "title_load_game"
};

/* ------------------------------------------------------------------------ */
/* Any file                                                                  */
/* ------------------------------------------------------------------------ */

/* "?_pt_size": the selected file's size in KB ("Reading..." while it is
 * being computed, "Broken", "----").  An if chain (a switch gives egcs's
 * binary decision tree); no file: an empty text (the block at the end, as
 * in all the texts of this kind). */
// FUNC 0x0014B610 memcard_text_size
void memcard_text_size(MemorycardControl *ctl, char *buf, int size)
{
    MemorycardFile *file = ctl->getSelectedFile();

    if (file != NULL) {
        int kb = file->sizeKB;

        if (kb == MC_SIZE_READING)
            strcpy(buf, g_002B1C68[MCTEXT_READING]);
        else if (kb == MC_SIZE_BROKEN)
            strcpy(buf, g_002B1C68[MCTEXT_BROKEN]);
        else if (kb == MC_SIZE_INVALID)
            strcpy(buf, "----");
        else {
            format_number(buf, kb);
            strcat(buf, "KB");
        }
    } else
        buf[0] = 0;
}

/* A card date: day, month, the 4-digit year. */
// FUNC 0x0014B6D8 memcard_format_date
void memcard_format_date(char *buf, const McDateTime *date)
{
    format_date(buf, date->day, date->month, date->year, 1);
}

/* "?_pt_date", "o_pt_update": the date the save was last written. */
// FUNC 0x0014B708 memcard_text_date
void memcard_text_date(MemorycardControl *ctl, char *buf, int size)
{
    MemorycardFile *file = ctl->getSelectedFile();

    if (file != NULL) {
        memcard_format_date(buf, &file->dir.entry.modify);  /* file + 0x20 */
        return;
    }
    buf[0] = 0;
}

/* "?_pt_comment": the comment of a GT3 save (16 characters).  The NULL
 * test before the cast is the dynamic_cast's own. */
// FUNC 0x0014B748 memcard_text_comment
void memcard_text_comment(MemorycardControl *ctl, char *buf, int size)
{
    MemorycardFile *file = ctl->getSelectedFile();
    MemorycardGT3File *save = dynamic_cast<MemorycardGT3File *>(file);

    if (save == NULL) {
        buf[0] = 0;
        return;
    }
    if (!file->infoValid()) {
        buf[0] = 0;
        return;
    }
    strncpy(buf, save->thumbnail.comment, 0x10);    /* +0x258 */
    buf[0x10] = 0;
}

/* "?_pt_mode": the licence name (and test) a GT3 save was made in. */
// FUNC 0x0014B7F8 memcard_text_mode
void memcard_text_mode(MemorycardControl *ctl, char *buf, int size)
{
    MemorycardFile *file = ctl->getSelectedFile();
    MemorycardGT3File *save = dynamic_cast<MemorycardGT3File *>(file);

    if (save == NULL) {
        buf[0] = 0;
        return;
    }
    if (!file->infoValid()) {
        buf[0] = 0;
        return;
    }
    strncpy(buf, save->thumbnail.licenceName, 0x18);    /* +0x269 */
    buf[0x18] = 0;
}

/* "o_pt_create": the date the save was created. */
// FUNC 0x0014B8A8 memcard_text_create_date
void memcard_text_create_date(MemorycardControl *ctl, char *buf, int size)
{
    MemorycardFile *file = ctl->getSelectedFile();

    if (file != NULL) {
        memcard_format_date(buf, &file->dir.entry.create);  /* file + 0x18 */
        return;
    }
    buf[0] = 0;
}

/* "o_pt_copy_protect": "yes" when the directory may not be copied, else
 * "no". */
// FUNC 0x0014B8E8 memcard_text_copy_protect
void memcard_text_copy_protect(MemorycardControl *ctl, char *buf, int size)
{
    MemorycardFile *file = ctl->getSelectedFile();

    if (file != NULL) {
        /* file + 0x2C; the bool is tested as (attr >> 3) & 1 */
        bool prot = (file->dir.entry.attr & sceMcFileAttrDupProhibit) != 0;

        if (prot)
            strcpy(buf, g_002B1C68[MCTEXT_CP_YES]);
        else
            strcpy(buf, g_002B1C68[MCTEXT_CP_NO]);
    } else
        buf[0] = 0;
}

/* "o_pt_title1": the first line of the save's title. */
// FUNC 0x0014B958 memcard_text_title1
void memcard_text_title1(MemorycardControl *ctl, char *buf, int size)
{
    MemorycardFile *file = ctl->getSelectedFile();

    if (file != NULL) {
        int firstLine = file->getTitleFirstLineBytes();

        strncpy(buf, file->getTitle(), firstLine);
        buf[firstLine] = 0;
    } else
        buf[0] = 0;
}

/* "o_pt_title2": the rest of the title (at most 32 bytes), if any.  The
 * length compare is signed. */
// FUNC 0x0014B9E0 memcard_text_title2
void memcard_text_title2(MemorycardControl *ctl, char *buf, int size)
{
    MemorycardFile *file = ctl->getSelectedFile();

    if (file != NULL) {
        int firstLine = file->getTitleFirstLineBytes();
        char *title = file->getTitle();

        if (firstLine < (int)strlen(title)) {
            strncpy(buf, title + firstLine, 0x20);
            buf[0x20] = 0;
            return;
        }
    }
    buf[0] = 0;
}

/* ------------------------------------------------------------------------ */
/* Game saves ("g_pt_*")                                                     */
/* ------------------------------------------------------------------------ */

/* The licence letters (thumbnail.game.licence; the strings in .sdata after
 * memcard_text_size's "----" / "KB"). */
const char *g_002A4E08[MC_LICENCE_COUNT] = { "B", "A", "IB", "IA", "S", "D" };   /* 0x002A4E08 */

/* "g_pt_license": the letter of the highest licence passed (unsigned
 * index, < 6). */
// FUNC 0x0014BA80 memcard_text_game_licence
void memcard_text_game_licence(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_game_thumbnail(ctl, buf);

    if (thumb != NULL) {
        u32 licence = thumb->game.licence;

        if (licence < MC_LICENCE_COUNT)
            strcpy(buf, g_002A4E08[licence]);
        else
            buf[0] = 0;
    }
}

// FUNC 0x0014BAE0 memcard_text_game_days
void memcard_text_game_days(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_game_thumbnail(ctl, buf);

    if (thumb != NULL)
        format_number(buf, thumb->game.days);
}

// FUNC 0x0014BB18 memcard_text_game_money
void memcard_text_game_money(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_game_thumbnail(ctl, buf);

    if (thumb != NULL)
        format_number(buf, thumb->game.money);      /* 64-bit */
}

// FUNC 0x0014BB50 memcard_text_game_cars
void memcard_text_game_cars(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_game_thumbnail(ctl, buf);

    if (thumb != NULL)
        format_number(buf, thumb->game.cars);
}

/* "g_pt_g_achievement": "<n>.<tenth>%". */
// FUNC 0x0014BB88 memcard_text_game_achievement
void memcard_text_game_achievement(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_game_thumbnail(ctl, buf);

    if (thumb != NULL) {
        int tenths = (int)(thumb->game.achievement * 10.0f);

        sprintf(buf, "%d.%01d%%", tenths / 10, tenths % 10);   /* 0x0031CC80 */
    }
}

/* ------------------------------------------------------------------------ */
/* Replays ("r_pt_*")                                                        */
/* ------------------------------------------------------------------------ */

// FUNC 0x0014BBF8 memcard_text_replay_time
void memcard_text_replay_time(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_replay_thumbnail(ctl, buf);

    if (thumb != NULL)
        format_time(buf, thumb->replay.time);
}

/* "r_pt_laps": "<n>lap" / "<n>laps". */
// FUNC 0x0014BC30 memcard_text_replay_laps
void memcard_text_replay_laps(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_replay_thumbnail(ctl, buf);

    if (thumb != NULL) {
        int laps = thumb->replay.laps;

        sprintf(buf, "%d", laps);           /* 0x003517E8 */
        strcat(buf, laps < 2 ? g_002B1C68[MCTEXT_LAP] : g_002B1C68[MCTEXT_LAPS]);
    }
}

/* "r_pt_car": "<name> <second name>" of the car. */
// FUNC 0x0014BCA0 memcard_text_replay_car
void memcard_text_replay_car(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_replay_thumbnail(ctl, buf);

    if (thumb != NULL) {
        strcpy(buf, db_car_name(thumb->replay.car));
        strcat(buf, " ");                   /* 0x003517F0 */
        strcat(buf, db_car_name2(thumb->replay.car));
    }
}

/* "r_pt_course": the course's name, through a RaceDataBase on the stack
 * (RaceDataBase(), getCourseName, ~RaceDataBase). */
// FUNC 0x0014BD10 memcard_text_replay_course
void memcard_text_replay_course(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_replay_thumbnail(ctl, buf);

    if (thumb != NULL) {
        RaceDataBase db;

        strcpy(buf, db.getCourseName(thumb->replay.course));
    }
}

/* "r_pt_power": "<power><unit>" in the user's unit. */
// FUNC 0x0014BDA0 memcard_text_replay_power
void memcard_text_replay_power(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_replay_thumbnail(ctl, buf);

    if (thumb != NULL)
        sprintf(buf, "%d%s", (int)g_002F59D8.convPower(thumb->replay.power), g_002F59D8.powerName());
}

/* ------------------------------------------------------------------------ */
/* Best lap replays / ghosts ("b_pt_*")                                      */
/* ------------------------------------------------------------------------ */

/* "b_pt_time": the lap time, or the top speed of the speed test. */
// FUNC 0x0014BE20 memcard_text_bestlap_time
void memcard_text_bestlap_time(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_bestlap_thumbnail(ctl, buf);

    if (thumb != NULL) {
        if (thumb->category == MC_CATEGORY_MAX_SPEED)
            format_speed(buf, thumb->bestlap.speed);
        else
            format_time(buf, thumb->bestlap.time);
    }
}

// FUNC 0x0014BE78 memcard_text_bestlap_car
void memcard_text_bestlap_car(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_bestlap_thumbnail(ctl, buf);

    if (thumb != NULL) {
        strcpy(buf, db_car_name(thumb->bestlap.car));
        strcat(buf, " ");                   /* 0x003517F0 */
        strcat(buf, db_car_name2(thumb->bestlap.car));
    }
}

/* (RaceDataBase(), getCourseName, ~RaceDataBase.) */
// FUNC 0x0014BEE8 memcard_text_bestlap_course
void memcard_text_bestlap_course(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_bestlap_thumbnail(ctl, buf);

    if (thumb != NULL) {
        RaceDataBase db;

        strcpy(buf, db.getCourseName(thumb->bestlap.course));
    }
}

// FUNC 0x0014BF78 memcard_text_bestlap_power
void memcard_text_bestlap_power(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_bestlap_thumbnail(ctl, buf);

    if (thumb != NULL) {
        sprintf(buf, "%d%s", (int)g_002F59D8.convPower(thumb->bestlap.power), g_002F59D8.powerName());
    }
}

// FUNC 0x0014BFF8 memcard_text_bestlap_mass
void memcard_text_bestlap_mass(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_bestlap_thumbnail(ctl, buf);

    if (thumb != NULL)
        sprintf(buf, "%dkg", (int)thumb->bestlap.mass);     /* 0x00351800 */
}

/* ------------------------------------------------------------------------ */
/* Setting files ("s_pt_*")                                                  */
/* ------------------------------------------------------------------------ */

// FUNC 0x0014C040 memcard_text_setting_car
void memcard_text_setting_car(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_setting_thumbnail(ctl, buf);

    if (thumb != NULL) {
        strcpy(buf, db_car_name(thumb->setting.car));
        strcat(buf, " ");                   /* 0x003517F0 */
        strcat(buf, db_car_name2(thumb->setting.car));
    }
}

/* (RaceDataBase(), getCourseName, ~RaceDataBase.) */
// FUNC 0x0014C0B0 memcard_text_setting_course
void memcard_text_setting_course(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_setting_thumbnail(ctl, buf);

    if (thumb != NULL) {
        RaceDataBase db;

        strcpy(buf, db.getCourseName(thumb->setting.course));
    }
}

// FUNC 0x0014C140 memcard_text_setting_power
void memcard_text_setting_power(MemorycardControl *ctl, char *buf, int size)
{
    gt2k_game::GameThumbnail *thumb = memcard_setting_thumbnail(ctl, buf);

    if (thumb != NULL) {
        sprintf(buf, "%d%s", (int)g_002F59D8.convPower(thumb->setting.power), g_002F59D8.powerName());
    }
}

/* ------------------------------------------------------------------------ */
/* The cards                                                                 */
/* ------------------------------------------------------------------------ */

/* The free space of card `id' (T501: port * 4 + slot): "<KB - 2>KB Free"
 * (never below 0) on a formatted PS2 memory card, "Unformatted" on an
 * unformatted one, nothing without a PS2 card.  ctl and size are not used. */
// FUNC 0x0014C1C0 memcard_text_free_space
void memcard_text_free_space(int id, MemorycardControl *ctl, char *buf, int size)
{
    if (mc_slot_type(id) == sceMcTypePS2) {
        if (mc_slot_formatted(id) != 0) {
            int freeKB = mc_slot_free(id) - 2;

            format_number(buf, freeKB > -1 ? freeKB : 0);
            strcat(buf, g_002B1C68[MCTEXT_KB_FREE]);
        } else
            strcpy(buf, g_002B1C68[MCTEXT_UNFORMATTED]);
    } else
        buf[0] = 0;
}

// FUNC 0x0014C260 memcard_text_free_slot1
void memcard_text_free_slot1(MemorycardControl *ctl, char *buf, int size)
{
    memcard_text_free_space(MC_ID_SLOT1, ctl, buf, size);
}

// FUNC 0x0014C290 memcard_text_free_slot2
void memcard_text_free_slot2(MemorycardControl *ctl, char *buf, int size)
{
    memcard_text_free_space(MC_ID_SLOT2, ctl, buf, size);
}

/* "pt_message": always empty. */
// FUNC 0x0014C2C0 memcard_text_message
void memcard_text_message(MemorycardControl *ctl, char *buf, int size)
{
    buf[0] = 0;
}

/* ------------------------------------------------------------------------ */
/* The thumbnails of the four kinds of GT3 save                              */
/* ------------------------------------------------------------------------ */

// FUNC 0x0014C2C8 memcard_game_thumbnail
gt2k_game::GameThumbnail *memcard_game_thumbnail(MemorycardControl *ctl, char *buf)
{
    MemorycardFile *file = ctl->getSelectedFile();
    MemorycardGameFile *save = dynamic_cast<MemorycardGameFile *>(file);

    if (save == NULL || !file->infoValid()) {
        buf[0] = 0;
        return NULL;
    }
    return &save->thumbnail;                /* +0x230 */
}

// FUNC 0x0014C368 memcard_replay_thumbnail
gt2k_game::GameThumbnail *memcard_replay_thumbnail(MemorycardControl *ctl, char *buf)
{
    MemorycardFile *file = ctl->getSelectedFile();
    MemorycardReplayFile *save = dynamic_cast<MemorycardReplayFile *>(file);

    if (save == NULL || !file->infoValid()) {
        buf[0] = 0;
        return NULL;
    }
    return &save->thumbnail;
}

// FUNC 0x0014C408 memcard_bestlap_thumbnail
gt2k_game::GameThumbnail *memcard_bestlap_thumbnail(MemorycardControl *ctl, char *buf)
{
    MemorycardFile *file = ctl->getSelectedFile();
    MemorycardBestlapReplayFile *save = dynamic_cast<MemorycardBestlapReplayFile *>(file);

    if (save == NULL || !file->infoValid()) {
        buf[0] = 0;
        return NULL;
    }
    return &save->thumbnail;
}

// FUNC 0x0014C4A8 memcard_setting_thumbnail
gt2k_game::GameThumbnail *memcard_setting_thumbnail(MemorycardControl *ctl, char *buf)
{
    MemorycardFile *file = ctl->getSelectedFile();
    MemorycardSettingFile *save = dynamic_cast<MemorycardSettingFile *>(file);

    if (save == NULL || !file->infoValid()) {
        buf[0] = 0;
        return NULL;
    }
    return &save->thumbnail;
}

/* ======================================================================== */
/* The hook table                                                           */
/* ======================================================================== */

/* The entries of g_002A4E20 (51), read by MemorycardControl::begin (T068):
 * a constant .sdata word after the texts' strings ("%dkg"), where the
 * original defined it with the table. */
int g_00351808 = sizeof(g_002A4E20) / sizeof(g_002A4E20[0]);    /* 0x00351808 */

/* The hook table (T069_static_init fills it).  Zero in the file: the
 * original built its MenuHook entries with a constructor at run time, so
 * egcs left the table's 0x4C8 bytes zero in .data, after the two tables
 * above and before the frame info (retail 0x002A4E20..0x002A52E8).  Without
 * an initialiser egcs writes it there too (a C++ global without one is
 * zeros at the end of the file's .data, u022's tables). */
MenuHook g_002A4E20[51];                    /* 0x002A4E20 */

/* One entry, as the constructor of the original's MenuHook stores it (see
 * T069_static_init).  Not a function of the binary; it only shortens the
 * source. */
static inline void set_entry(MenuHook *e, const char *name, MenuHookFn *create,
                             int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* An entry through the walking cursor (build_vec_init's), all six words
 * stored.  HOOK: a factory (p8 its parameter), TEXT: an entry of
 * create_callback_text (T239; p8 the text callback, pC 0: the text is made
 * again before every draw). */
#define ENTRY(name, create, p8) set_entry(base, name, create, p8, 0, 0, 0); base++; (void)0
#define HOOK(name, create, p8)  ENTRY(name, create, p8)
#define TEXT(name, text)        ENTRY(name, create_callback_text, (int)(MemcardTextFn *)text)

/* Static initialiser of the hook table g_002A4E20 (.ctors entry
 * 0x0028F094).  Compiler-generated in the original (_GLOBAL_$I$...): the
 * original defined the table in T069 with MenuHook objects built by a
 * constructor (MenuHook("b_pt_car", create_callback_text, (int)f, 0, 0, 0),
 * ...); egcs builds such a table at run time, in this function (cp/init.c
 * build_vec_init: a cursor stepped by 0x18), and leaves .data zero, as
 * retail's 0x4C8 zero bytes at 0x002A4E20 (before T069's frame block) show.
 * The decomp defines the table without an initialiser (above; MenuHook in
 * T229 is a plain struct shared by other units, without a constructor), so
 * the function is written out as the walk it expands to: an exact form, not
 * the original (u006's
 * T030_static_init has the same form; here the cursor copy and the
 * iterator that T030 / T036 need make no difference).  It stores the same
 * 51 entries as any other form.  The names are sorted (PageLayout::findHook
 * binary-searches them; the strings at 0x0031CC90..0x0031CFC8).  The
 * library hooks (create_callback_text, create_text_box) take fewer or typed
 * parameters: cast to MenuHookFn. */
// FUNC 0x0014C548 T069_static_init
void T069_static_init(void)
{
    MenuHook *base = g_002A4E20;

    TEXT("b_pt_car",           memcard_text_bestlap_car);
    TEXT("b_pt_comment",       memcard_text_comment);
    TEXT("b_pt_course",        memcard_text_bestlap_course);
    TEXT("b_pt_date",          memcard_text_date);
    TEXT("b_pt_mass",          memcard_text_bestlap_mass);
    TEXT("b_pt_mode",          memcard_text_mode);
    TEXT("b_pt_power",         memcard_text_bestlap_power);
    TEXT("b_pt_size",          memcard_text_size);
    TEXT("b_pt_time",          memcard_text_bestlap_time);
    HOOK("confirm_icon_face",  memcard_create_icon_face, ICON_FACE_CONFIRM);
    TEXT("g_pt_cars",          memcard_text_game_cars);
    TEXT("g_pt_date",          memcard_text_date);
    TEXT("g_pt_day",           memcard_text_game_days);
    TEXT("g_pt_g_achievement", memcard_text_game_achievement);
    TEXT("g_pt_license",       memcard_text_game_licence);
    TEXT("g_pt_money",         memcard_text_game_money);
    TEXT("g_pt_size",          memcard_text_size);
    HOOK("icon_view_slot1",    memcard_create_icon_view, 0);
    HOOK("icon_view_slot2",    memcard_create_icon_view, 1);
    TEXT("o_pt_copy_protect",  memcard_text_copy_protect);
    TEXT("o_pt_create",        memcard_text_create_date);
    TEXT("o_pt_size",          memcard_text_size);
    TEXT("o_pt_title1",        memcard_text_title1);
    TEXT("o_pt_title2",        memcard_text_title2);
    TEXT("o_pt_update",        memcard_text_date);
    HOOK("op_icon_face",       memcard_create_icon_face, ICON_FACE_OP);
    HOOK("progress_bar",       memcard_create_progress_bar, 0);
    TEXT("pt_free_slot1",      memcard_text_free_slot1);
    TEXT("pt_free_slot2",      memcard_text_free_slot2);
    TEXT("pt_message",         memcard_text_message);
    TEXT("r_pt_car",           memcard_text_replay_car);
    TEXT("r_pt_comment",       memcard_text_comment);
    TEXT("r_pt_course",        memcard_text_replay_course);
    TEXT("r_pt_date",          memcard_text_date);
    TEXT("r_pt_laps",          memcard_text_replay_laps);
    TEXT("r_pt_mode",          memcard_text_mode);
    TEXT("r_pt_power",         memcard_text_replay_power);
    TEXT("r_pt_size",          memcard_text_size);
    TEXT("r_pt_time",          memcard_text_replay_time);
    TEXT("s_pt_car",           memcard_text_setting_car);
    TEXT("s_pt_comment",       memcard_text_comment);
    TEXT("s_pt_course",        memcard_text_setting_course);
    TEXT("s_pt_date",          memcard_text_date);
    TEXT("s_pt_mode",          memcard_text_mode);
    TEXT("s_pt_power",         memcard_text_setting_power);
    TEXT("s_pt_size",          memcard_text_size);
    HOOK("text_box",           create_text_box, 0);
    HOOK("title_bar_slot1",    memcard_create_slot_title, 0);
    HOOK("title_bar_slot2",    memcard_create_slot_title, 1);
    HOOK("title_base",         memcard_create_title_view, 0);
    HOOK("working_icon_face",  memcard_create_icon_face, ICON_FACE_WORKING);
}
