#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T263_raw_list.cpp"
#pragma implementation "src/game/menu/framework/T263_raw_list.cpp"
#pragma implementation "T263_raw_list.h"
#endif
/* T263 - the list windows of the menu system.  0x001D3F70..0x001D5C10.
 *
 * gt2k_menu::RawListItem is a RawWindow that can be linked into a list
 * (prev / next, the owning list, lock flags for the layout, a "hi-light"
 * image shown while it is selected but not focused).  gt2k_menu::RawList is
 * itself an item and holds a doubly linked list of items with one focus
 * item: the focus moves with the cursor events (up / down for a vertical
 * list, left / right for a horizontal one, with the cursor or the refused
 * sound), optionally wrapping around (moveLoop) and keeping the index when
 * it moves between two sub-lists (keepSubListIndex).  layout() places the
 * items one after the other (margin, spacing; sub-lists first) and resizes
 * the list to them unless its size is locked; scrollToFocus() scrolls the
 * list inside its scroll window (setScroll) so that the focus item is
 * visible, through the RawAnimScroll of RawWindow::anim.
 *
 * The variants only change flags: H = horizontal; Pos / Size / Fixed lock
 * the position / the size / both of the list when it is laid out as an
 * item of a parent list.
 *
 * Destructors: every one here is empty; egcs adds the vptr store, the base
 * destructor (flags 0) and, for the deleting form (flags & 1), the class
 * operator delete (Object's, 0x001BF458).
 */
#include "game/menu/framework/T263_raw_list.h"
#include "game/menu/framework/raw_list_internal.h"

namespace gt2k_menu {

/* ======================================================================== */
/* RawListItem                                                              */
/* ======================================================================== */

/* moveDir (0x1F8) is not initialised here (RawList's constructor does). */
// FUNC 0x001D3F70 gt2k_menu::RawListItem::RawListItem
RawListItem::RawListItem(const RawWindowArgs &args)
    : RawWindow(args)
{
    list = NULL;
    prevItem = NULL;
    nextItem = NULL;
    positionLocked = false;
    sizeLocked = false;
    hiLightPropertyName = NULL;
    selected = 0;
}

/* index.txt: RawListItem::virtual_0. */
// FUNC 0x001D3FC0 gt2k_menu::RawListItem::~RawListItem
RawListItem::~RawListItem()
{
    /* implicit: ~RawWindow() and, if (flags & 1), operator delete (added by egcs) */
}

/* index.txt: RawListItemFixed::virtual_21; the slot is RawListItem's own
 * (vtable 0x00338D58 slot 21), inherited by every item class.  A selected
 * item that loses the focus shows its hi-light image instead of the normal
 * unfocused look (the image only: no size, no position). */
// FUNC 0x001D4018 gt2k_menu::RawListItem::virtual_21
void RawListItem::virtual_21(int focus)
{
    if (hiLightPropertyName != NULL && selected != 0 && focus == 0) {
        changeImage(hiLightPropertyName, 0, 0);
        return;
    }
    RawWindow::virtual_21(focus);
}

/* Only PlayModeBox::virtual_29 (u037) calls it. */
// FUNC 0x001D4068 gt2k_menu::RawListItem::setSelected
void RawListItem::setSelected(int sel)
{
    if (sel != selected) {
        selected = sel;
        virtual_21(focus);
    }
}

/* ======================================================================== */
/* RawListItemFixed                                                         */
/* ======================================================================== */

// FUNC 0x001D40A8 gt2k_menu::RawListItemFixed::RawListItemFixed
RawListItemFixed::RawListItemFixed(const RawWindowArgs &args)
    : RawListItem(args)
{
    positionLocked = true;
    sizeLocked = true;
}

/* index.txt: RawListItemFixed::virtual_0. */
// FUNC 0x001D40E8 gt2k_menu::RawListItemFixed::~RawListItemFixed
RawListItemFixed::~RawListItemFixed()
{
    /* implicit: ~RawListItem() and, if (flags & 1), operator delete (added by egcs) */
}

/* ======================================================================== */
/* RawList                                                                  */
/* ======================================================================== */

// FUNC 0x001D4140 gt2k_menu::RawList::RawList
RawList::RawList(const RawWindowArgs &args)
    : RawListItem(args)
{
    /* In field order: sched1 moves the last store of the constant 1
     * (focusControl) in front of the vptr store, as in retail. */
    focusAnimation = NULL;
    firstItem = NULL;
    lastItem = NULL;
    focusItem = NULL;
    count = 0;
    margin = 0.0f;                          /* stored as integer zeros */
    spacing = 0.0f;
    scroll = NULL;
    scrollCenter = 0;
    scrollPrevMargin = 0.0f;
    scrollPrevOffset = 0.0f;
    scrollNextMargin = 0.0f;
    scrollNextOffset = 0.0f;
    vertical = 1;
    moveLoop = 0;
    keepSubListIndex = 1;
    focusControl = 1;
    childrenFocusChange = 0;
    moveDir = LIST_MOVE_NONE;
}

/* index.txt: RawList::virtual_0. */
// FUNC 0x001D41C8 gt2k_menu::RawList::~RawList
RawList::~RawList()
{
    /* implicit: ~RawListItem() and, if (flags & 1), operator delete (added by egcs) */
}

/* Slot 9, the per-frame hook: runs the focus animation. */
// FUNC 0x001D4220 gt2k_menu::RawList::virtual_9
void RawList::virtual_9()
{
    if (focusAnimation != NULL)
        focusAnimation->update();
}

/* Slot 22: a child window was built (RawWindow::loadChild calls it on the
 * parent, 0x001D766C; the first pass said createChild); a RawListItem child
 * is appended to the list, without a relayout. */
// FUNC 0x001D4248 gt2k_menu::RawList::virtual_22
void RawList::virtual_22(RawWindow *child)
{
    RawListItem *item = dynamic_cast<RawListItem *>(child);    /* NULL for NULL */
    if (item != NULL)
        addItem(item, 0);
}

/* Slot 17, the event handler.  The list's own cursor events move the focus
 * (up / down = previous / next of a vertical list, left / right of a
 * horizontal one): the cursor sound and a scroll when it moved, the refused
 * sound when it could not; either way the event is handled (0).
 * Everything else goes to RawWindow's handler. */
// FUNC 0x001D42B8 gt2k_menu::RawList::virtual_17
int RawList::virtual_17(RawWindow *sender, int event, int arg)
{
    if (sender == this) {
        int handled = 0;
        int moved = 0;
        moveDir = LIST_MOVE_NONE;
        switch (event) {
        case RAW_EVENT_UP:
            if (vertical) {
                moved = virtual_26();
                handled = 1;
            }
            break;
        case RAW_EVENT_DOWN:
            if (vertical) {
                moved = virtual_27();
                handled = 1;
            }
            break;
        case RAW_EVENT_LEFT:
            if (!vertical) {
                moved = virtual_26();
                handled = 1;
            }
            break;
        case RAW_EVENT_RIGHT:
            if (!vertical) {
                moved = virtual_27();
                handled = 1;
            }
            break;
        }
        if (handled) {
            if (moved) {
                menu_play_se(MENU_SE_CURSOR);
                scrollToFocus();
                return 0;
            }
            menu_play_se(MENU_SE_REFUSED);
            return 0;
        }
    }
    return RawWindow::virtual_17(sender, event, arg);
}

/* Slot 14 (the menu control): RawWindow's, then passed on to the focus
 * item (virtual_24). */
// FUNC 0x001D4408 gt2k_menu::RawList::virtual_14
void RawList::virtual_14(MenuControl *ctl)
{
    RawWindow::virtual_14(ctl);
    virtual_24(ctl);
}

/* Slot 20 (the focus): RawWindow's, then the focus goes to every child
 * window that is not a list item, then to the focus item; the focus
 * animation is moved onto the focus item (also when that is NULL). */
// FUNC 0x001D4450 gt2k_menu::RawList::virtual_20
void RawList::virtual_20(int focus)
{
    RawWindow *child;

    RawWindow::virtual_20(focus);
    for (child = firstChildWindow(); child != NULL; child = child->nextWindow()) {
        if (dynamic_cast<RawListItem *>(child) == NULL)
            child->virtual_20(focus);
    }
    RawListItem *item = focusItem;
    if (item != NULL)
        item->virtual_20(focus);
    if (focusAnimation != NULL)
        focusAnimation->virtual_5(item);
}

/* Slot 24: the focus item gets the menu control ctl (NULL when
 * focusControl is 0). */
// FUNC 0x001D45C8 gt2k_menu::RawList::virtual_24
void RawList::virtual_24(MenuControl *ctl)
{
    RawListItem *item = focusItem;
    if (item != NULL)
        item->virtual_14(focusControl ? ctl : NULL);
}

/* Slot 25: focusControl = on, and the focus item's menu control again. */
// FUNC 0x001D4608 gt2k_menu::RawList::virtual_25
void RawList::virtual_25(int on)
{
    focusControl = on;
    virtual_24(menuControl);
}

/* Slot 26: the focus moves to the previous item (moveDir -1); at the first
 * item it wraps to the last one (moveDir +1) if moveLoop, else nothing
 * happens (0). */
// FUNC 0x001D4638 gt2k_menu::RawList::virtual_26
int RawList::virtual_26()
{
    RawListItem *target = focusItem;
    if (target == NULL)
        return 0;
    target = target->prevItem;
    moveDir = LIST_MOVE_PREV;
    if (target == NULL) {
        if (!moveLoop) {
            moveDir = LIST_MOVE_NONE;
            return 0;
        }
        target = lastItem;
        moveDir = LIST_MOVE_NEXT;
    }
    return virtual_28(target);
}

/* Slot 27: the same towards the next item. */
// FUNC 0x001D46A0 gt2k_menu::RawList::virtual_27
int RawList::virtual_27()
{
    RawListItem *target = focusItem;
    if (target == NULL)
        return 0;
    target = target->nextItem;
    moveDir = LIST_MOVE_NEXT;
    if (target == NULL) {
        if (!moveLoop) {
            moveDir = LIST_MOVE_NONE;
            return 0;
        }
        target = firstItem;
        moveDir = LIST_MOVE_PREV;
    }
    return virtual_28(target);
}

/* Slot 28: moves the focus from the current item to item.  Two sub-lists
 * pass the focus index on (keepSubListIndex, virtual_30); the old item's
 * menu control moves to the new one.  Always 1. */
// FUNC 0x001D4708 gt2k_menu::RawList::virtual_28
int RawList::virtual_28(RawListItem *item)
{
    RawListItem *oldFocus = focusItem;
    if (oldFocus != NULL && item != NULL) {
        if (keepSubListIndex) {
            /* implicit: __dynamic_cast (getRawList is inline); egcs drops the NULL check
             * of oldFocus / item, both known to be non-NULL here */
            RawList *from = getRawList(oldFocus);
            RawList *to = getRawList(item);
            if (from != NULL && to != NULL)
                virtual_30(from, to);
        }
        MenuControl *ctl = oldFocus->menuControl;
        oldFocus->virtual_14(NULL);
        item->virtual_14(ctl);
    }
    virtual_29(item);
    return 1;
}

/* Slot 29: focusItem = item (nothing else; PlayModeBox overrides it). */
// FUNC 0x001D4840 gt2k_menu::RawList::virtual_29
void RawList::virtual_29(RawListItem *item)
{
    focusItem = item;
}

/* The out-of-line max / min / clamp helpers (below, 0x001D56D0..). */
static f32 max_f(f32 a, f32 b);
static f32 min_f(f32 a, f32 b);
static f32 clamp_f(f32 v, f32 lo, f32 hi);

/* Scrolls the list inside its scroll window (the view) so that the focus
 * item is visible.  Along the list's axis (x for a horizontal list, y for a
 * vertical one, on screen):
 *   prevRoom = how far the list may still move forward (its start lies
 *              before the view's start), + scrollPrevMargin;
 *   nextRoom = the same at the end, + scrollNextMargin;
 *   before / after = the room between the view's start and the item's
 *              start / the item's end and the view's end; after a move
 *              towards the previous (moveDir < 0) / next (> 0) item,
 *              scrollPrevOffset / scrollNextOffset (plus half the free room
 *              if scrollCenter) more is kept free;
 * and the list's scroll animation is aimed at its own position plus
 * clamp(min(0, after) - min(0, before), -nextRoom, prevRoom).  The two axes
 * are two copies of the same code. */
// FUNC 0x001D4848 gt2k_menu::RawList::scrollToFocus
void RawList::scrollToFocus()
{
    RawListItem *item = focusItem;
    if (item == NULL || scroll == NULL)
        return;

    /* anim is the RawAnimScroll the RawWindow constructor builds
     * (setAnimation, which could replace it, has no callers). */
    if (vertical == 0) {
        f32 listStart = getScreenLeft();
        f32 listEnd = listStart + w;
        f32 itemStart = item->getScreenLeft();
        f32 itemSize = item->w;
        f32 viewStart = scroll->getScreenLeft();
        f32 viewSize = scroll->w;
        f32 itemEnd = itemStart + itemSize;
        f32 before = itemStart - viewStart;
        f32 viewEnd = viewStart + viewSize;
        f32 prevRoom = max_f(0.0f, (viewStart - listStart) + scrollPrevMargin);
        f32 after = viewEnd - itemEnd;
        f32 nextRoom = max_f(0.0f, (listEnd - viewEnd) + scrollNextMargin);
        f32 center;
        if (scrollCenter)
            center = (viewSize - itemSize) * 0.5f;
        else
            center = 0.0f;
        if (moveDir < 0)
            before -= center + scrollPrevOffset;
        else if (moveDir > 0)
            after -= center + scrollNextOffset;
        /* Only the parts outside the view count; before / after are cut in
         * place (with a separate variable the result took another register
         * than retail's). */
        before = min_f(0.0f, before);
        after = min_f(0.0f, after);
        f32 move = clamp_f(after - before, -nextRoom, prevRoom);
        f32 left = getLeft() + move;
        f32 top = getTop();
        static_cast<RawAnimScroll *>(anim)->scrollTo(left, top);
    } else {
        f32 listStart = getScreenTop();
        f32 listEnd = listStart + h;
        f32 itemStart = item->getScreenTop();
        f32 itemSize = item->h;
        f32 viewStart = scroll->getScreenTop();
        f32 viewSize = scroll->h;
        f32 itemEnd = itemStart + itemSize;
        f32 before = itemStart - viewStart;
        f32 viewEnd = viewStart + viewSize;
        f32 prevRoom = max_f(0.0f, (viewStart - listStart) + scrollPrevMargin);
        f32 after = viewEnd - itemEnd;
        f32 nextRoom = max_f(0.0f, (listEnd - viewEnd) + scrollNextMargin);
        f32 center;
        if (scrollCenter)
            center = (viewSize - itemSize) * 0.5f;
        else
            center = 0.0f;
        if (moveDir < 0)
            before -= center + scrollPrevOffset;
        else if (moveDir > 0)
            after -= center + scrollNextOffset;
        before = min_f(0.0f, before);
        after = min_f(0.0f, after);
        f32 move = clamp_f(after - before, -nextRoom, prevRoom);
        f32 left = getLeft();
        f32 top = getTop() + move;
        static_cast<RawAnimScroll *>(anim)->scrollTo(left, top);
    }
}

/* Slot 30: to takes over the focus index of from (keepSubListIndex). */
// FUNC 0x001D4B30 gt2k_menu::RawList::virtual_30
void RawList::virtual_30(RawList *from, RawList *to)
{
    to->virtual_31(from->getIndex((RawListItem *)NULL));
}

/* Slot 31: the focus goes to item index (< 0: stays), the menu control
 * with it; no scroll. */
// FUNC 0x001D4B88 gt2k_menu::RawList::virtual_31
void RawList::virtual_31(int index)
{
    if (focusItem != NULL)
        focusItem->virtual_14(NULL);
    virtual_29(getItem(index));
    if (focusItem != NULL)
        focusItem->virtual_14(menuControl);
}

/* Slot 32: moves the focus to item index (moveDir from the direction) and
 * scrolls to it. */
// FUNC 0x001D4C30 gt2k_menu::RawList::virtual_32
void RawList::virtual_32(int index)
{
    int delta = index - getIndex((RawListItem *)NULL);
    moveDir = LIST_MOVE_NONE;
    if (delta < 0)
        moveDir = LIST_MOVE_PREV;
    if (delta > 0)
        moveDir = LIST_MOVE_NEXT;
    virtual_28(getItem(index));
    scrollToFocus();
}

/* The index of item (NULL: the focus item), -1 if it is not in the list. */
// FUNC 0x001D4CC0 gt2k_menu::RawList::getIndex
int RawList::getIndex(RawListItem *item)
{
    RawListItem *p;
    int index;

    if (item == NULL)
        item = focusItem;
    index = 0;
    for (p = firstItem; p != NULL; p = p->nextItem, index++) {
        if (item == p)
            return index;
    }
    return -1;
}

/* Item index (< 0: the focus item), NULL past the end. */
// FUNC 0x001D4CF8 gt2k_menu::RawList::getItem
RawListItem *RawList::getItem(int index)
{
    RawListItem *p;

    if (index < 0)
        return focusItem;
    if (index >= count)
        return NULL;
    p = firstItem;
    while (index != 0 && p != NULL) {
        p = p->nextItem;
        index--;
    }
    return p;
}

// FUNC 0x001D4D40 gt2k_menu::RawList::setVertical
void RawList::setVertical(int vertical, int relayout)
{
    this->vertical = vertical;
    if (relayout)
        layout();
}

// FUNC 0x001D4D68 gt2k_menu::RawList::addItem
void RawList::addItem(RawListItem *item, int relayout)
{
    insertItem(item, count, relayout);
}

/* Links item in before item index (at the end if index >= count), with
 * interrupts off.  The item's position becomes relative to the list.  The
 * focus is set again through slot 29: the old focus item, or the first
 * item if there was none. */
// FUNC 0x001D4D88 gt2k_menu::RawList::insertItem
void RawList::insertItem(RawListItem *item, int index, int relayout)
{
    RawListItem *prev;
    RawListItem *next;
    RawListItem *oldFocus;
    int intr;

    item->attachPosition(this);
    oldFocus = focusItem;
    intr = intr_suspend();
    if (count == 0) {
        next = NULL;
        prev = NULL;
    } else if (index < count) {
        next = getItem(index);
        prev = next->prevItem;
    } else {
        prev = getItem(count - 1);
        next = prev->nextItem;
    }
    if (prev == NULL)
        firstItem = item;
    else
        prev->nextItem = item;
    item->prevItem = prev;
    item->nextItem = next;
    if (next == NULL)
        lastItem = item;
    else
        next->prevItem = item;
    item->list = this;
    count++;
    focusItem = NULL;
    if (oldFocus == NULL)
        oldFocus = firstItem;
    intr_resume(intr);
    virtual_29(oldFocus);
    if (relayout)
        layout();
}

/* Unlinks item, with interrupts off.  If it had the focus, the focus goes
 * to its next item, else to its previous one. */
// FUNC 0x001D4EB0 gt2k_menu::RawList::removeItem
void RawList::removeItem(RawListItem *item, int relayout)
{
    RawListItem *oldFocus;
    RawListItem *prev;
    RawListItem *next;
    int intr;

    oldFocus = focusItem;
    intr = intr_suspend();
    prev = item->prevItem;
    next = item->nextItem;
    if (prev == NULL)
        firstItem = next;
    else
        prev->nextItem = next;
    if (next == NULL)
        lastItem = prev;
    else
        next->prevItem = prev;
    item->prevItem = NULL;
    item->nextItem = NULL;
    count--;
    item->list = NULL;
    focusItem = NULL;
    if (item == oldFocus)
        oldFocus = (next != NULL) ? next : prev;
    intr_resume(intr);
    virtual_29(oldFocus);
    if (relayout)
        layout();
}

/* Lays the items out one after the other along the list's axis, starting
 * at margin, spacing apart; sub-lists are laid out first.  An item with a
 * locked position keeps it (its position still counts).  Then the list is
 * resized to the bounding box of its items, unless its size is locked or
 * it is empty. */
// FUNC 0x001D4F80 gt2k_menu::RawList::layout
void RawList::layout()
{
    RawListItem *item;
    int resize = 0;
    f32 minY = 3.40282347e+38f;             /* FLT_MAX, .sdata 0x003528E8 */
    f32 maxY = -3.40282347e+38f;            /* 0x003528EC */
    f32 nextY = 0.0f;
    f32 minX = minY;
    f32 maxX = maxY;
    f32 nextX = 0.0f;

    if (vertical == 0)
        nextX = margin;
    else
        nextY = margin;

    for (item = firstItem; item != NULL; item = item->nextItem) {
        /* Quirk: reloaded and recomputed for every item.  The bool accessor
         * puts the load straight into resize's register (retail's lw s2;
         * xori s2,s2,1). */
        resize = !isLockSize();
        /* implicit: __dynamic_cast (getRawList is inline), its NULL check kept */
        RawList *subList = getRawList(item);
        if (subList != NULL)
            subList->layout();

        f32 left = nextX;
        f32 top = nextY;
        f32 width = item->w;
        f32 height = item->h;
        if (!item->isLockPosition()) {
            item->setLeft(left);
            item->setTop(top);
        } else {
            left = item->getLeft();
            top = item->getTop();
        }
        /* The next position from the item's end; PRE computes left + width and
         * top + height on both paths, as retail does. */
        if (vertical == 0)
            nextX = left + width + spacing;
        else
            nextY = top + height + spacing;
        f32 right = left + width;
        f32 bottom = top + height;
        if (left < minX)
            minX = left;
        if (top < minY)
            minY = top;
        if (maxX < right)
            maxX = right;
        if (maxY < bottom)
            maxY = bottom;
    }
    if (resize) {
        setWidth(maxX - minX);
        setHeight(maxY - minY);
    }
}

/* ======================================================================== */
/* The variants                                                             */
/* ======================================================================== */

// FUNC 0x001D5190 gt2k_menu::RawListFixed::RawListFixed
RawListFixed::RawListFixed(const RawWindowArgs &args)
    : RawList(args)
{
}

/* index.txt: RawListFixed::virtual_0. */
// FUNC 0x001D51C8 gt2k_menu::RawListFixed::~RawListFixed
RawListFixed::~RawListFixed()
{
    /* implicit: ~RawList() and, if (flags & 1), operator delete (added by egcs) */
}

// FUNC 0x001D5220 gt2k_menu::RawListH::RawListH
RawListH::RawListH(const RawWindowArgs &args)
    : RawList(args)
{
    vertical = 0;
}

/* index.txt: RawListH::virtual_0. */
// FUNC 0x001D5258 gt2k_menu::RawListH::~RawListH
RawListH::~RawListH()
{
    /* implicit: ~RawList() and, if (flags & 1), operator delete (added by egcs) */
}

// FUNC 0x001D52B0 gt2k_menu::RawListHFixed::RawListHFixed
RawListHFixed::RawListHFixed(const RawWindowArgs &args)
    : RawListH(args)
{
    positionLocked = true;
    sizeLocked = true;
}

/* index.txt: RawListHFixed::virtual_0. */
// FUNC 0x001D52F0 gt2k_menu::RawListHFixed::~RawListHFixed
RawListHFixed::~RawListHFixed()
{
    /* implicit: ~RawListH() and, if (flags & 1), operator delete (added by egcs) */
}

// FUNC 0x001D5348 gt2k_menu::RawListHPos::RawListHPos
RawListHPos::RawListHPos(const RawWindowArgs &args)
    : RawListH(args)
{
    positionLocked = true;
}

/* index.txt: RawListHPos::virtual_0. */
// FUNC 0x001D5388 gt2k_menu::RawListHPos::~RawListHPos
RawListHPos::~RawListHPos()
{
    /* implicit: ~RawListH() and, if (flags & 1), operator delete (added by egcs) */
}

// FUNC 0x001D53E0 gt2k_menu::RawListHSize::RawListHSize
RawListHSize::RawListHSize(const RawWindowArgs &args)
    : RawListH(args)
{
    sizeLocked = true;
}

/* index.txt: RawListHSize::virtual_0. */
// FUNC 0x001D5420 gt2k_menu::RawListHSize::~RawListHSize
RawListHSize::~RawListHSize()
{
    /* implicit: ~RawListH() and, if (flags & 1), operator delete (added by egcs) */
}

// FUNC 0x001D5478 gt2k_menu::RawListV::RawListV
RawListV::RawListV(const RawWindowArgs &args)
    : RawList(args)
{
}

/* index.txt: RawListV::virtual_0. */
// FUNC 0x001D54B0 gt2k_menu::RawListV::~RawListV
RawListV::~RawListV()
{
    /* implicit: ~RawList() and, if (flags & 1), operator delete (added by egcs) */
}

// FUNC 0x001D5508 gt2k_menu::RawListVFixed::RawListVFixed
RawListVFixed::RawListVFixed(const RawWindowArgs &args)
    : RawListV(args)
{
    positionLocked = true;
    sizeLocked = true;
}

/* index.txt: RawListVFixed::virtual_0. */
// FUNC 0x001D5548 gt2k_menu::RawListVFixed::~RawListVFixed
RawListVFixed::~RawListVFixed()
{
    /* implicit: ~RawListV() and, if (flags & 1), operator delete (added by egcs) */
}

// FUNC 0x001D55A0 gt2k_menu::RawListVPos::RawListVPos
RawListVPos::RawListVPos(const RawWindowArgs &args)
    : RawListV(args)
{
    positionLocked = true;
}

/* index.txt: RawListVPos::virtual_0. */
// FUNC 0x001D55E0 gt2k_menu::RawListVPos::~RawListVPos
RawListVPos::~RawListVPos()
{
    /* implicit: ~RawListV() and, if (flags & 1), operator delete (added by egcs) */
}

// FUNC 0x001D5638 gt2k_menu::RawListVSize::RawListVSize
RawListVSize::RawListVSize(const RawWindowArgs &args)
    : RawListV(args)
{
    sizeLocked = true;
}

/* index.txt: RawListVSize::virtual_0. */
// FUNC 0x001D5678 gt2k_menu::RawListVSize::~RawListVSize
RawListVSize::~RawListVSize()
{
    /* implicit: ~RawListV() and, if (flags & 1), operator delete (added by egcs) */
}

/* ======================================================================== */
/* Float helpers (static or template functions emitted out of line, not     */
/* inlined: scrollToFocus, their only user, calls them)                     */
/* ======================================================================== */

/* a > b ? a : b (as an if / else: a ?: under -ffast-math would fold to max.s,
 * which retail does not have) */
// FUNC 0x001D56D0 max_f
static f32 max_f(f32 a, f32 b)
{
    f32 r;
    if (a > b)
        r = a;
    else
        r = b;
    return r;
}

/* a < b ? a : b (as max_f) */
// FUNC 0x001D56F0 min_f
static f32 min_f(f32 a, f32 b)
{
    f32 r;
    if (a < b)
        r = a;
    else
        r = b;
    return r;
}

/* max(min(v, hi), lo) */
// FUNC 0x001D5710 clamp_f
static f32 clamp_f(f32 v, f32 lo, f32 hi)
{
    return max_f(min_f(v, hi), lo);
}

} /* namespace gt2k_menu */

/* ======================================================================== */
/* Typeinfo functions (compiler-generated, __si_type_info)                  */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
// FUNC 0x001D5748 __tfQ29gt2k_menu12RawListVSize
extern "C" void *__tfQ29gt2k_menu12RawListVSize(void)
{
    if (__tiQ29gt2k_menu12RawListVSize[0] == 0) {
        __tfQ29gt2k_menu8RawListV();
        __rtti_si(__tiQ29gt2k_menu12RawListVSize, "Q29gt2k_menu12RawListVSize",
                  (const type_info *)__tiQ29gt2k_menu8RawListV);
    }
    return __tiQ29gt2k_menu12RawListVSize;
}

// FUNC 0x001D5798 __tfQ29gt2k_menu11RawListVPos
extern "C" void *__tfQ29gt2k_menu11RawListVPos(void)
{
    if (__tiQ29gt2k_menu11RawListVPos[0] == 0) {
        __tfQ29gt2k_menu8RawListV();
        __rtti_si(__tiQ29gt2k_menu11RawListVPos, "Q29gt2k_menu11RawListVPos",
                  (const type_info *)__tiQ29gt2k_menu8RawListV);
    }
    return __tiQ29gt2k_menu11RawListVPos;
}

// FUNC 0x001D57E8 __tfQ29gt2k_menu13RawListVFixed
extern "C" void *__tfQ29gt2k_menu13RawListVFixed(void)
{
    if (__tiQ29gt2k_menu13RawListVFixed[0] == 0) {
        __tfQ29gt2k_menu8RawListV();
        __rtti_si(__tiQ29gt2k_menu13RawListVFixed, "Q29gt2k_menu13RawListVFixed",
                  (const type_info *)__tiQ29gt2k_menu8RawListV);
    }
    return __tiQ29gt2k_menu13RawListVFixed;
}

// FUNC 0x001D5838 __tfQ29gt2k_menu8RawListV
extern "C" void *__tfQ29gt2k_menu8RawListV(void)
{
    if (__tiQ29gt2k_menu8RawListV[0] == 0) {
        __tfQ29gt2k_menu7RawList();
        __rtti_si(__tiQ29gt2k_menu8RawListV, "Q29gt2k_menu8RawListV",
                  (const type_info *)__tiQ29gt2k_menu7RawList);
    }
    return __tiQ29gt2k_menu8RawListV;
}

// FUNC 0x001D5888 __tfQ29gt2k_menu12RawListHSize
extern "C" void *__tfQ29gt2k_menu12RawListHSize(void)
{
    if (__tiQ29gt2k_menu12RawListHSize[0] == 0) {
        __tfQ29gt2k_menu8RawListH();
        __rtti_si(__tiQ29gt2k_menu12RawListHSize, "Q29gt2k_menu12RawListHSize",
                  (const type_info *)__tiQ29gt2k_menu8RawListH);
    }
    return __tiQ29gt2k_menu12RawListHSize;
}

// FUNC 0x001D58D8 __tfQ29gt2k_menu11RawListHPos
extern "C" void *__tfQ29gt2k_menu11RawListHPos(void)
{
    if (__tiQ29gt2k_menu11RawListHPos[0] == 0) {
        __tfQ29gt2k_menu8RawListH();
        __rtti_si(__tiQ29gt2k_menu11RawListHPos, "Q29gt2k_menu11RawListHPos",
                  (const type_info *)__tiQ29gt2k_menu8RawListH);
    }
    return __tiQ29gt2k_menu11RawListHPos;
}

// FUNC 0x001D5928 __tfQ29gt2k_menu13RawListHFixed
extern "C" void *__tfQ29gt2k_menu13RawListHFixed(void)
{
    if (__tiQ29gt2k_menu13RawListHFixed[0] == 0) {
        __tfQ29gt2k_menu8RawListH();
        __rtti_si(__tiQ29gt2k_menu13RawListHFixed, "Q29gt2k_menu13RawListHFixed",
                  (const type_info *)__tiQ29gt2k_menu8RawListH);
    }
    return __tiQ29gt2k_menu13RawListHFixed;
}

// FUNC 0x001D5978 __tfQ29gt2k_menu8RawListH
extern "C" void *__tfQ29gt2k_menu8RawListH(void)
{
    if (__tiQ29gt2k_menu8RawListH[0] == 0) {
        __tfQ29gt2k_menu7RawList();
        __rtti_si(__tiQ29gt2k_menu8RawListH, "Q29gt2k_menu8RawListH",
                  (const type_info *)__tiQ29gt2k_menu7RawList);
    }
    return __tiQ29gt2k_menu8RawListH;
}

// FUNC 0x001D59C8 __tfQ29gt2k_menu12RawListFixed
extern "C" void *__tfQ29gt2k_menu12RawListFixed(void)
{
    if (__tiQ29gt2k_menu12RawListFixed[0] == 0) {
        __tfQ29gt2k_menu7RawList();
        __rtti_si(__tiQ29gt2k_menu12RawListFixed, "Q29gt2k_menu12RawListFixed",
                  (const type_info *)__tiQ29gt2k_menu7RawList);
    }
    return __tiQ29gt2k_menu12RawListFixed;
}
#endif /* !BOOT_OLD_GCC */

namespace gt2k_menu {

/* ======================================================================== */
/* RawList's inline members, emitted out of line (no callers)               */
/* ======================================================================== */

/* Lupo 2 name (index.txt: unnamed). */
// FUNC 0x001D5A18 gt2k_menu::RawList::getRawList
/* implicit: __dynamic_cast; inline in game/menu/framework/T263_raw_list.h (layout and virtual_28 expand it); egcs emits this copy:
 * dynamic_cast<RawList *>(w), NULL for NULL */

/* index.txt: setScrollNextMargin(float) (Lupo 2 map one off). */
// FUNC 0x001D5A70 gt2k_menu::RawList::setScrollNextOffset
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: setScrollPrevOffset(float). */
// FUNC 0x001D5A78 gt2k_menu::RawList::setScrollNextMargin
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: setScrollPrevMargin(float). */
// FUNC 0x001D5A80 gt2k_menu::RawList::setScrollPrevOffset
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5A88 gt2k_menu::RawList::setScrollPrevMargin
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: setScroll(RawWindow*). */
// FUNC 0x001D5A90 gt2k_menu::RawList::setScrollCenter
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5A98 gt2k_menu::RawList::setScroll
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5AA0 gt2k_menu::RawList::setFocusAnimation
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: getFocusItem(void) (it returns the count). */
// FUNC 0x001D5AA8 gt2k_menu::RawList::getItemCount
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5AB0 gt2k_menu::RawList::getIndex(RawListItem&) (getIndex__Q29gt2k_menu7RawListRQ29gt2k_menu11RawListItem)
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */
/* implicit: getIndex (the inline body) */

/* index.txt: setMoveLoop(bool) (it sets 0x240, used by slot 28 for the
 * sub-list index; slots 26 / 27 wrap on 0x23C). */
// FUNC 0x001D5AD0 gt2k_menu::RawList::setKeepSubListIndex
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5AD8 gt2k_menu::RawList::setMoveLoop
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5AE0 gt2k_menu::RawList::getFocusItem
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x001D5AE8 __tfQ29gt2k_menu7RawList
extern "C" void *__tfQ29gt2k_menu7RawList(void)
{
    if (__tiQ29gt2k_menu7RawList[0] == 0) {
        __tfQ29gt2k_menu11RawListItem();
        __rtti_si(__tiQ29gt2k_menu7RawList, "Q29gt2k_menu7RawList",
                  (const type_info *)__tiQ29gt2k_menu11RawListItem);
    }
    return __tiQ29gt2k_menu7RawList;
}

// FUNC 0x001D5B38 __tfQ29gt2k_menu16RawListItemFixed
extern "C" void *__tfQ29gt2k_menu16RawListItemFixed(void)
{
    if (__tiQ29gt2k_menu16RawListItemFixed[0] == 0) {
        __tfQ29gt2k_menu11RawListItem();
        __rtti_si(__tiQ29gt2k_menu16RawListItemFixed, "Q29gt2k_menu16RawListItemFixed",
                  (const type_info *)__tiQ29gt2k_menu11RawListItem);
    }
    return __tiQ29gt2k_menu16RawListItemFixed;
}
#endif /* !BOOT_OLD_GCC */

namespace gt2k_menu {

/* ======================================================================== */
/* RawListItem's inline members, emitted out of line (no callers)           */
/* ======================================================================== */

/* index.txt: unnamed. */
// FUNC 0x001D5B88 gt2k_menu::RawListItem::setHiLightPropertyName
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: getPrevItem(void) (Lupo 2 map one off; it returns 0x1EC). */
// FUNC 0x001D5B90 gt2k_menu::RawListItem::getNextItem
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5B98 gt2k_menu::RawListItem::getPrevItem
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5BA0 gt2k_menu::RawListItem::lockSize
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: unnamed. */
// FUNC 0x001D5BA8 gt2k_menu::RawListItem::lockPosition
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: isLockPosition(void). */
// FUNC 0x001D5BB0 gt2k_menu::RawListItem::isLockSize
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

/* index.txt: getNextItem(void). */
// FUNC 0x001D5BB8 gt2k_menu::RawListItem::isLockPosition
/* inline in game/menu/framework/T263_raw_list.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x001D5BC0 __tfQ29gt2k_menu11RawListItem
extern "C" void *__tfQ29gt2k_menu11RawListItem(void)
{
    if (__tiQ29gt2k_menu11RawListItem[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu11RawListItem, "Q29gt2k_menu11RawListItem",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu11RawListItem;
}
#endif /* !BOOT_OLD_GCC */
