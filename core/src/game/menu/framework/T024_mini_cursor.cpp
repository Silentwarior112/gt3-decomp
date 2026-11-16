/* T024 - gt2k_menu::MiniCursor.  0x0010C9E0..0x0010DFF0.
 *
 * The strip of small pictures under the arcade selects (the courses, the
 * classes, the cars), built by ArcadeControl::createMiniCursor (T020) over
 * a SmartIndexT<int>, one icon per index.  update (virtual_1) keeps the
 * current icon inside the strip's width and restarts the blink when the
 * index changes; draw (virtual_2) places and tints the icons, draws the
 * `mark' picture over the marked (locked) ones and the cursor over the
 * current one. */
/* Includes: retail's .rodata (0x0030B2A0) has the vtable, then the type
 * names SmartIndexT<int>, MiniCursor, OperatorList, List<OperatorHolder>,
 * OperatorHolder (its base list), Cell<OperatorHolder>, Operator: the
 * classes T213 brings, MiniCursor, then SmartIndexT<int> (instantiated
 * where T251 is read: after this file's own header).  The others add no
 * type names. */
#include "game/menu/framework/T024_mini_cursor.h"
#include "game/common/T251_smart_index.h"          /* SmartIndexT<int> (list) */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current (the arrays) */
#include "game/menu/framework/T218_color.h"                /* Color4f (the current icon's tint) */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout::getTexture, MenuLayoutItem */
#include "game/menu/arcade/arcade_course_select_internal.h"

namespace gt2k_menu {

/* Item(res, NULL) (operator new is the caller's), then: no icons yet; both
 * arrays from the menu heap (16-aligned), zeroed.  `scroll' is built by
 * MotionValue's inline constructor (the +0, +8, +4 zero stores).  The
 * members are set in declaration order; the flags are bools (4 bytes in
 * egcs: the second loop keeps `count' in a register as the first does,
 * which an int array would not allow). */
// FUNC 0x0010C9E0 gt2k_menu::MiniCursor::MiniCursor
MiniCursor::MiniCursor(MenuLayoutItem *res, SmartIndexT<int> *list)
    : Item(res, (MenuTexture *)0)
{
    this->list = list;
    gap = 4.0f;
    itemW = 16.0f;
    itemH = 16.0f;
    lastValue = -1;
    blink = 1.0f;
    count = list->end - list->begin;
    cursor = NULL;
    mark = NULL;
    items = (Item **)menu_heap_current()->allocate(count * sizeof(Item *), 0x10);
    for (int i = 0; i < count; i++)
        items[i] = NULL;
    marked = (bool *)menu_heap_current()->allocate(count * sizeof(bool), 0x10);
    for (int i = 0; i < count; i++)
        marked[i] = false;
}

/* The icons, the cursor and the mark through their virtual (deleting)
 * destructors, the arrays back to the menu heap. */
// FUNC 0x0010CB50 gt2k_menu::MiniCursor::~MiniCursor
MiniCursor::~MiniCursor()
{
    /* Implied at the end: ~Item (flags 0) and, for a deleting call,
     * Object::operator delete. */
    /* implicit: operator delete (deleting destructor) */
    if (marked != NULL) {
        menu_heap_current()->free(marked);
        marked = NULL;
    }
    if (items != NULL) {
        for (int i = 0; i < count; i++) {
            if (items[i] != NULL) {
                delete items[i];
                items[i] = NULL;
            }
        }
        menu_heap_current()->free(items);
        items = NULL;
    }
    if (cursor != NULL) {
        delete cursor;
        cursor = NULL;
    }
    if (mark != NULL) {
        delete mark;
        mark = NULL;
    }
}

/* Icon i (0 <= i < count) = a new Item of res (the old one deleted); the
 * strip takes its size.  The texture is looked up before the allocation. */
// FUNC 0x0010CCA0 gt2k_menu::MiniCursor::setItem
void MiniCursor::setItem(int i, MenuLayoutItem *res, PageLayout *layout)
{
    if (i >= 0 && i < count) {
        if (items[i] != NULL)
            delete items[i];
        MenuTexture *tex = layout->getTexture(res);
        /* implicit: operator new, operator delete (new-expression) */
        items[i] = new Item(res, tex);      /* Object::operator new; operator delete if Item() throws */
        itemW = items[i]->w;
        itemH = items[i]->h;
    }
}

/* No callers.  The hidden item `name' (the old cursor deleted). */
// FUNC 0x0010CDA8 gt2k_menu::MiniCursor::setCursor
void MiniCursor::setCursor(const char *name, PageLayout *layout)
{
    if (cursor != NULL)
        delete cursor;
    /* implicit: operator new, operator delete (new-expression) */
    cursor = new Item(name, layout);        /* Object::operator new; operator delete if Item() throws */
}

/* The hidden item `name' (the old mark deleted). */
// FUNC 0x0010CE50 gt2k_menu::MiniCursor::setMark
void MiniCursor::setMark(const char *name, PageLayout *layout)
{
    if (mark != NULL)
        delete mark;
    /* implicit: operator new, operator delete (new-expression) */
    mark = new Item(name, layout);          /* Object::operator new; operator delete if Item() throws */
}

// FUNC 0x0010CEF8 gt2k_menu::MiniCursor::setMarked
void MiniCursor::setMarked(int i, bool on)
{
    if (i >= 0 && i < count)
        marked[i] = on;
}

/* size() icons with the gaps between them (the index's virtual size). */
// FUNC 0x0010CF28 gt2k_menu::MiniCursor::totalWidth
f32 MiniCursor::totalWidth()
{
    int icons = list->size();
    return (itemW + gap) * (f32)icons - gap;
}

/* x of the current icon from the strip's start (the index's virtual get:
 * the valid indices before it). */
// FUNC 0x0010CF80 gt2k_menu::MiniCursor::widthBefore
f32 MiniCursor::widthBefore()
{
    int before = list->get();
    return (f32)before * (itemW + gap);
}

/* The room the icons after the current one take. */
// FUNC 0x0010CFD0 gt2k_menu::MiniCursor::widthAfter
f32 MiniCursor::widthAfter()
{
    int icons = list->size();
    int current = list->get();
    return (f32)(icons - 1 - current) * (itemW + gap);
}

/* update.  A strip wider than the item keeps the current icon centred,
 * stopped at both ends; a narrower one is centred in it.  The offset is then
 * moved by half the screen's room around the item (640 - w).  The first
 * icon's x jumps there when the index has just wrapped, else eases (0.3 a
 * frame).  The blink runs 1.0 down to 0.25 by 1/15 a frame and restarts at
 * 1.0 when the index changes. */
// FUNC 0x0010D050 gt2k_menu::MiniCursor::virtual_1
void MiniCursor::virtual_1()
{
    Item::virtual_1();
    f32 width = w;
    f32 total = totalWidth();
    f32 before = widthBefore();
    f32 after = widthAfter();
    f32 target;
    if (width < total) {
        f32 half = width * 0.5f;
        if (before < half)
            target = 0.0f;
        else if (after < half)
            target = width - total;
        else
            target = half - (before + itemW * 0.5f);
    } else {
        target = (width - total) * 0.5f;
    }
    target = target + (MENU_SCREEN_W - w) * 0.5f;
    if (list->wrapped != 0)
        scroll.set(target);
    else
        scroll.approach(target, 0.3f);
    blink = blink - 0.06666667f;            /* 1/15 */
    if (blink < 0.25f)
        blink = 1.0f;
    if (lastValue != list->value) {
        lastValue = list->value;
        blink = 1.0f;
    }
}

/* draw.  Gives the sub-items this alphaRatio, then walks the valid indices
 * from scroll's value in steps of itemW + gap, on this y (plus the
 * anchor's).  Icon colours (the inline Drawable::setColor(u32): four
 * Color4f(rgba) temporaries copied into color[]): unmarked (or marked with a
 * mark picture): the current one blinking grey (blink, blink, blink and the
 * 0.502 alpha of Color4f(), packed by toRGBA8), the others 0x80808080;
 * marked without a mark picture: 0x80000080 the current one, 0x80000040 the
 * others (dark red).  Only the icons whose box reaches into [0, 640) are
 * drawn, the mark over the marked ones.  The cursor is drawn last at the
 * current icon (x 0 if it is not valid), with drawFlags |= 0x11 (own
 * settings, additive).
 *
 * scroll.get() / y.get(): retail keeps &scroll and &y in registers (inline
 * MotionValue members).  99.3%: left is the loop's register, set from the
 * scroll value at the loop (hence startX and the two steps); what is left
 * is the copy of `list' in the loop (retail loads it into $v1 at the latch
 * and copies it to $a2, ours the reverse) and the two hoisted colour
 * constants 128.0 / 64/255 in $f26 / $f27 swapped (the same with the
 * reference compiler 991111-01; statement orders, a local for list->value,
 * while / if-valid loop forms tried). */
// FUNC 0x0010D1B8 gt2k_menu::MiniCursor::virtual_2
void MiniCursor::virtual_2()
{
    Item::virtual_2();
    for (int i = 0; i < count; i++)
        if (items[i] != NULL)
            items[i]->alphaRatio = alphaRatio;
    if (cursor != NULL)
        cursor->alphaRatio = alphaRatio;
    if (mark != NULL)
        mark->alphaRatio = alphaRatio;

    f32 startX = scroll.get();
    f32 rowY = y.get();
    if (anchor != NULL)
        rowY += anchor->getY();
    f32 cursorX = 0.0f;
    f32 left = startX;                      /* the left edge of icon i */
    for (int i = list->begin; i < list->end; i++) {
        if (list->isValid(i) == 0)
            continue;
        f32 centreX = itemW * 0.5f + left;
        if (i == list->value)
            cursorX = centreX;
        if (items[i] != NULL) {
            if (marked[i] == 0 || mark != NULL) {
                if (i == list->value) {
                    Color4f tint;           /* 0.502 each: only r, g, b are set */
                    tint.r = blink;
                    tint.g = blink;
                    tint.b = blink;
                    items[i]->Drawable::setColor(tint.toRGBA8(1.0f));
                } else {
                    items[i]->Drawable::setColor(0x80808080);
                }
            } else {
                if (i == list->value)
                    items[i]->Drawable::setColor(0x80000080);
                else
                    items[i]->Drawable::setColor(0x80000040);
            }
        }
        f32 halfW = itemW * 0.5f;
        if (centreX + halfW >= 0.0f && centreX - halfW < MENU_SCREEN_W) {
            if (items[i] != NULL) {
                items[i]->x.set(centreX);
                items[i]->y.set(rowY);
                items[i]->virtual_2();
            }
            if (marked[i] != 0 && mark != NULL) {
                mark->x.set(centreX);
                mark->y.set(rowY);
                mark->virtual_2();
            }
        }
        left += itemW;
        left += gap;
    }
    if (cursor != NULL) {
        cursor->x.set(cursorX);
        cursor->drawFlags |= 0x11;
        cursor->y.set(rowY);
        cursor->virtual_2();
    }
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated: builds the __si_type_info (base Item) on the first
 * call. */
// FUNC 0x0010DFA0 __tfQ29gt2k_menu10MiniCursor
extern "C" void *__tfQ29gt2k_menu10MiniCursor(void)
{
    if (__tiQ29gt2k_menu10MiniCursor[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu10MiniCursor, "Q29gt2k_menu10MiniCursor",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu10MiniCursor;
}
#endif
