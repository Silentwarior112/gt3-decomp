/* T244 - gt2k_menu::ButtonList and List<Button>.  0x001C7FE0..0x001CA550.
 *
 * A ButtonList is a Drawable holding a List<Button>: one Button per record
 * (ButtonData) of a page.  The buttons are laid along one axis (controlType
 * != 0: x and widths, 0: y and heights), each centred at its offPoint, the
 * buttons `spacing' apart.  The cursor is a SmartIndexT<int> over the
 * buttons (it skips the buttons whose focus is disabled); the display value
 * (displayX / displayY) eases each frame towards the scroll target that keeps
 * the focused button inside the list's own w / h. */
/* Retail's .rodata (0x00336BB0..0x00336E00): the literals, the vtables, then
 * the type names ButtonList, List<Button>, SmartIndexT<int>, Cell<Button>,
 * OperatorList, List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * Operator (reverse definition order). */
#include "game/menu/framework/button_list_internal.h"
#include <stddef.h>
#include "game/menu/framework/T206_button.h"               /* first: Button (Item, Transitive, the operators) is
                                               defined before ButtonList, so the type names come
                                               after T244's own, as in retail (.rodata
                                               0x00336C88..0x00336E00, reverse definition order) */
#include "game/menu/framework/T238_slider.h"               /* gt2k_menu::Slider, SliderDesc.  Before ButtonList: its
                                               SmartIndexDoubleRange (T266) instantiates
                                               SmartIndexT<int> after Cell<Button> and before
                                               List<Button>, retail's order, also once
                                               T251_smart_index.h no longer instantiates it where
                                               it is read (ref/data/u071/pending.md) */
#include "game/menu/framework/T244_button_list.h"
#include "game/common/T017_smart_index.h"          /* __tfQ29gt2k_menut11SmartIndexT1Zi's storage */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::SelectableFuncData */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the help layers) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuLayer,
                                               MenuSliderFunc, MenuMultiFunc, MultiButtonInfo */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/common/T251_smart_index.h"          /* gt2k_menu::SmartIndexT<int> */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "game/menu/framework/T267_concurrent_button.h"    /* gt2k_menu::ConcurrentButton */
#include "game/menu/framework/T268_free_button.h"          /* gt2k_menu::FreeButton */
#include "game/menu/framework/T269_multi_button.h"         /* gt2k_menu::MultiButton */
#include "game/common/T494_text_utils.h"           /* str_equal, str_has_prefix */
#include "game/menu/framework/T495_menu_list.h"            /* gt2k_menu::pCell / pList (count, append, unlink, indexOf,
                                               sort) */
#include "game/menu/framework/menu_list.h"                 /* gt2k_menu::Cell<T>, List<T>, ListIterator<T> */

namespace gt2k_menu {

/* (? name) The record's action is a sub list.  Inline in the original: T244's
 * tests of it are a value (xori 4, then a branch on zero), not a
 * compare-and-branch (createButtons 0x001C8570, decideButton 0x001CA150).
 * Local to the file: an inline function in the shared header would be
 * compiled in every unit that includes it and shift their label numbers. */
static inline bool is_sub_list(const ButtonData *data)
{
    return (data->flags & BUTTON_ACTION_MASK) == BUTTON_SUB_LIST;
}

/* The cursor range to the buttons (clamped unless the list loops), the
 * cursor to 0. */
// FUNC 0x001C7FE0 gt2k_menu::ButtonList::resetIndex
void ButtonList::resetIndex()
{
    if (loop == 0)                          /* through the inline setter: retail keeps &index in
                                               $s0 for the store and the two calls */
        index.setClamp(true);
    else
        index.setClamp(false);
    index.setRange(0, count());
    index.set(0);
}

/* The four special buttons (by element name prefix, see createButtons).
 * Each is new'd on the menu heap (EH: Object::operator delete if the
 * constructor throws) and attached under the list.  The slider and the
 * multi button get their descriptor from the layout's function tables,
 * looked up by the element name and returned by value from the entry's
 * func(ctx) (no NULL check of the entry), after the operator new. */
// FUNC 0x001C8040 gt2k_menu::ButtonList::createSlider
Button *ButtonList::createSlider(MenuLayoutItem *elem, ButtonData *data, f32 offPoint, int index)
{
    const MenuSliderFunc *entry = layout->findSliderFunc(elem->name);
    void *ctx = layout->sliderCtx;
    Slider *button = new Slider(this, controlType, layout, offPoint, data, elem, relative, index, /* implicit: operator new, operator delete (new-expression) */
                                ctx, entry->func(ctx));
    button->attachTo(this);
    return button;
}

// FUNC 0x001C8148 gt2k_menu::ButtonList::createMultiButton
Button *ButtonList::createMultiButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                      int index)
{
    u16 flags = data->flags;
    int subControlType = (flags >> BUTTON_SUB_CONTROL_SHIFT) & 1;
    int subLoop = (flags >> BUTTON_SUB_LOOP_SHIFT) & 1;
    const MenuMultiFunc *entry = layout->findMultiFunc(elem->name);
    void *ctx = layout->multiCtx;
    MultiButton *button = new MultiButton(this, controlType, loop, layout, offPoint, data, elem, /* implicit: operator new, operator delete (new-expression) */
                                          data->subItems, data->subCount, subControlType, subLoop,
                                          relative, index, ctx, entry->func(ctx));
                                            /* func's result: a temporary at sp+0x40, bound to
                                               the const reference */
    button->attachTo(this);
    return button;
}

// FUNC 0x001C8290 gt2k_menu::ButtonList::createFreeButton
Button *ButtonList::createFreeButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint, int index)
{
    u16 flags = data->flags;
    int subControlType = (flags >> BUTTON_SUB_CONTROL_SHIFT) & 1;
    int subLoop = (flags >> BUTTON_SUB_LOOP_SHIFT) & 1;
    FreeButton *button = new FreeButton(this, controlType, loop, layout, offPoint, data, elem, /* implicit: operator new, operator delete (new-expression) */
                                        data->subItems, data->subCount, subControlType, subLoop,
                                        relative, index);
    button->attachTo(this);
    return button;
}

/* As createFreeButton; the list also remembers it (it gets the input first,
 * see doControl). */
// FUNC 0x001C8398 gt2k_menu::ButtonList::createConcurrentButton
Button *ButtonList::createConcurrentButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                           int index)
{
    u16 flags = data->flags;
    int subControlType = (flags >> BUTTON_SUB_CONTROL_SHIFT) & 1;
    int subLoop = (flags >> BUTTON_SUB_LOOP_SHIFT) & 1;
    ConcurrentButton *button = new ConcurrentButton(this, controlType, loop, layout, offPoint, /* implicit: operator new, operator delete (new-expression) */
                                                    data, elem, data->subItems, data->subCount,
                                                    subControlType, subLoop, relative, index);
    button->attachTo(this);
    concurrent = button;
    return button;
}

/* Slot 5: a plain Button, attached under the list. */
// FUNC 0x001C84A8 gt2k_menu::ButtonList::createButton
Button *ButtonList::createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint, int index)
{
    Button *button = new Button(this, controlType, layout, offPoint, data, elem, relative, index); /* implicit: operator new, operator delete (new-expression) */
    button->attachTo(this);
    return button;
}

/* Slot 6: one button per record; its rectangle is rects[i] (a table of
 * layout items) or else the page item named like the record (records
 * without one are skipped, their index too).  An item named "slider*" makes
 * a Slider; "multi*" / "free*" / "concurrent*" make the sub-list buttons
 * when the record is a sub list with entries; anything else goes through the
 * virtual createButton.  Every new button starts hidden (virtual_9(0)) and
 * is appended to the list. */
// FUNC 0x001C8570 gt2k_menu::ButtonList::createButtons
void ButtonList::createButtons(ButtonData **items, int count, MenuLayoutItem *rects)
{
    f32 pos = 0.0f;
    for (int i = 0; i < count; i++) {
        MenuLayoutItem *elem = (rects != NULL) ? &rects[i] : layout->findPageItem(items[i]->name);
        if (elem == NULL)
            continue;
        pos += ((controlType == 0) ? elem->h : elem->w) * 0.5f;
        Button *button;
        if (str_has_prefix("slider", elem->name))
            button = createSlider(elem, items[i], pos, i);
        else if (str_has_prefix("multi", elem->name)
                 && is_sub_list(items[i])
                 && items[i]->subCount > 0)
            button = createMultiButton(elem, items[i], pos, i);
        else if (str_has_prefix("free", elem->name)
                 && is_sub_list(items[i])
                 && items[i]->subCount > 0)
            button = createFreeButton(elem, items[i], pos, i);
        else if (str_has_prefix("concurrent", elem->name)
                 && is_sub_list(items[i])
                 && items[i]->subCount > 0)
            button = createConcurrentButton(elem, items[i], pos, i);
        else
            button = createButton(elem, items[i], pos, i);
        button->virtual_9(0);
        append(button);                     /* its Cell<Button> part (NULL stays NULL) */
        pos += ((controlType == 0) ? elem->h : elem->w) * 0.5f;
        pos += spacing;
    }
}

/* The layout's "focus" and "disable" pictures (object items of the current
 * layer), if it has them. */
// FUNC 0x001C87E0 gt2k_menu::ButtonList::createFocusItems
void ButtonList::createFocusItems()
{
    MenuLayoutItem *focus = layout->findObject("focus", NULL);   /* the current layer */
    if (focus != NULL)
        focusItem = new Item(focus, layout); /* implicit: operator new, operator delete (new-expression) */
    MenuLayoutItem *disable = layout->findObject("disable", NULL);
    if (disable != NULL)                    /* a variable each (one shared: other registers) */
        disableItem = new Item(disable, layout);
}

/* The full constructor (sub lists, PageManager pages): the Drawable part
 * (inline), position and size, the buttons (createButtons called directly,
 * not through the vtable), the cursor at `cursor'.  The parameters x, y, w,
 * h and index of the declaration are posX, posY, width, height and cursor
 * here (the members of those names). */
// FUNC 0x001C88D0 gt2k_menu::ButtonList::ButtonList
ButtonList::ButtonList(int controlType, int loop, PageLayout *layout, f32 posX, f32 posY,
                       f32 width, f32 height, f32 spacing, ButtonData **items, int count,
                       int relative, int cursor, MenuLayoutItem *rects)
    : controlType(controlType), loop(loop), layout(layout), helpLayer(NULL), helpLayer2(NULL),
      focusDisabled(0), active(1), concurrent(NULL), spacing(spacing), relative(relative),
      interpolateDiff(0.0f), decideFunc(NULL), decideCtx(NULL), focusItem(NULL),
      disableItem(NULL)
{
    /* implicit: Object (base constructor, inline Drawable), ~pList, ~Object (exception cleanup) */
    x.set(posX);
    y.set(posY);
    w = width;
    h = height;
    ButtonList::createButtons(items, count, rects);
    resetIndex();
    index.set(cursor);
    createFocusItems();
}

/* The plain constructor: no buttons yet (the derived lists call
 * createButtons and resetIndex themselves), spacing 4, relative 1. */
// FUNC 0x001C8B90 gt2k_menu::ButtonList::ButtonList
ButtonList::ButtonList(int controlType, int loop, PageLayout *layout)
    : controlType(controlType), loop(loop), layout(layout), helpLayer(NULL), helpLayer2(NULL),
      focusDisabled(0), active(1), concurrent(NULL), spacing(4.0f), relative(1),
      interpolateDiff(0.0f), decideFunc(NULL), decideCtx(NULL), focusItem(NULL),
      disableItem(NULL)
{
    /* implicit: Object (base constructor, inline Drawable), ~pList, ~Object (exception cleanup) */
    createFocusItems();
}

/* Unlinks the buttons (they are children of the list in the object tree,
 * which deletes them) and deletes the two pictures (each field cleared
 * before its delete); then ~List<Button>, ~Object (Drawable's implicit
 * destructor), Object::operator delete when (flags & 1). */
// FUNC 0x001C8DA8 gt2k_menu::ButtonList::~ButtonList
ButtonList::~ButtonList()
{
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL)
        unlink(button);
    if (focusItem != NULL) {
        Item *item = focusItem;
        focusItem = NULL;
        delete item;
    }
    if (disableItem != NULL) {
        Item *item = disableItem;
        disableItem = NULL;
        delete item;
    }
    /* implicit: operator delete (deleting destructor) */
}

/* The help layer of the focused button (Button::setFocus passes its
 * record's help layer name): a new Transitive under parent holding the
 * plain items and the objects of layer `name' of the current page; the
 * previous one (if any) disappears.  No such layer: helpLayer = NULL. */
// FUNC 0x001C8EB8 gt2k_menu::ButtonList::showHelpLayer
void ButtonList::showHelpLayer(Object *parent, const char *name)
{
    MenuLayer *layer = layout->getLayer(name);
    Transitive *old = helpLayer;
    if (layer != NULL) {
        helpLayer = new Transitive();  /* implicit: Object::operator delete (if the constructor throws) */
        for (int i = 0; i < layer->itemCount; i++) {
            Item *item = layout->createItem(layer->items[i], helpLayer, NULL);
            if (item != NULL)
                item->attachTo(helpLayer);
        }
        for (int i = 0; i < layer->objectCount; i++) {
            Transitive *obj = layout->createObject(layer->objects[i], helpLayer, NULL);
            if (obj != NULL)
                obj->attachTo(helpLayer);
        }
        helpLayer->attachTo(parent);
    } else {
        helpLayer = NULL;
    }
    if (old != NULL)
        old->disappearAll(NULL);
}

/* The second help layer: as showHelpLayer, but its items are anchored to
 * the new Transitive, which slides in (openTransition) before it is
 * attached; the previous one slides out (closeTransition). */
// FUNC 0x001C9030 gt2k_menu::ButtonList::showHelpLayer2
void ButtonList::showHelpLayer2(Object *parent, const char *name)
{
    MenuLayer *layer = layout->getLayer(name);
    Transitive *old = helpLayer2;
    if (layer != NULL) {
        helpLayer2 = new Transitive();  /* implicit: Object::operator delete (if the constructor throws) */
        for (int i = 0; i < layer->itemCount; i++) {
            Item *item = layout->createItem(layer->items[i], helpLayer2, helpLayer2);
            if (item != NULL)
                item->attachTo(helpLayer2);
        }
        for (int i = 0; i < layer->objectCount; i++) {
            Transitive *obj = layout->createObject(layer->objects[i], helpLayer2, helpLayer2);
            if (obj != NULL)
                obj->attachTo(helpLayer2);
        }
        layout->openTransition(helpLayer2);
        helpLayer2->attachTo(parent);
    } else {
        helpLayer2 = NULL;
    }
    if (old != NULL)
        layout->closeTransition(old);
}

/* Gives every button its availability check from the page's table (the
 * last check for the buttons past the table). */
// FUNC 0x001C91B0 gt2k_menu::ButtonList::setButtonChecks
void ButtonList::setButtonChecks(const SelectableFuncData *table, void *ctx)
{
    if (table == NULL)
        return;
    int n = table->count;
    int i = 0;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL) {
        if (i >= n)
            button->setCheck(table->checks[n - 1], ctx);
        else
            button->setCheck(table->checks[i], ctx);
        i++;
    }
}

/* The decide callback (see decideButton); also given to every button
 * (virtual_10: the sub-list buttons pass it on to their own lists). */
// FUNC 0x001C9288 gt2k_menu::ButtonList::setDecideFunc
void ButtonList::setDecideFunc(void *ctx, ButtonDecideFunc func)
{
    decideCtx = ctx;
    decideFunc = func;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL)
        button->virtual_10(ctx, func);
}

/* Focuses the button at the cursor (unless the focus is disabled) and
 * unfocuses all the others. */
// FUNC 0x001C9320 gt2k_menu::ButtonList::refreshFocus
void ButtonList::refreshFocus()
{
    int i = 0;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL) {
        if (i == index.value && focusDisabled == 0)
            button->setFocus(1);
        else
            button->setFocus(0);
        i++;
    }
}

/* Slot 9: refreshFocus, then the display jumps to the scroll target. */
// FUNC 0x001C93D0 gt2k_menu::ButtonList::refresh
void ButtonList::refresh()
{
    refreshFocus();
    f32 target = scrollTarget();
    if (controlType == 0)                   /* each arm its own set(): retail's two force stores */
        displayY.set(target);
    else
        displayX.set(target);
}

// FUNC 0x001C9430 gt2k_menu::ButtonList::enableFocus
void ButtonList::enableFocus()
{
    focusDisabled = 0;
    refreshFocus();
}

// FUNC 0x001C9450 gt2k_menu::ButtonList::disableFocus
void ButtonList::disableFocus()
{
    focusDisabled = 1;
    refreshFocus();
}

/* Puts the cursor on the first button whose record links to page `link'
 * (no callers). */
// FUNC 0x001C9470 gt2k_menu::ButtonList::focusLink
void ButtonList::focusLink(const char *link)
{
    if (link == NULL)
        return;
    int i = 0;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL) {
        if (str_equal(button->linkMenu->link, link)) {
            index.set(i);
            refreshFocus();
            return;
        }
        i++;
    }
}

// FUNC 0x001C9520 gt2k_menu::ButtonList::setCursor
void ButtonList::setCursor(int cursor)
{
    index.set(cursor);
    refreshFocus();
}

/* The cursor to the button under position pos along the list (sizes plus
 * spacing summed from 0); past the end: the last one.  A list that does not
 * scroll: 0. */
// FUNC 0x001C9550 gt2k_menu::ButtonList::setCursorAt
void ButtonList::setCursorAt(f32 pos)
{
    ListIterator<Button> it(this);
    if (relative != 0) {
        f32 end = 0.0f;
        int i = 0;
        Button *button;
        while ((button = it.next()) != NULL) {
            end += (controlType == 0) ? button->h : button->w; /* this sense: retail's bnel */
            end += spacing;
            if (pos < end) {
                index.set(i);
                break;
            }
            i++;
        }
        if (i == count())
            index.set(i - 1);
    } else {
        index.set(0);
    }
}

/* The centre of the focused button along the list (relative: the sizes
 * before it plus half its own; otherwise its own x or y; 0 without one). */
// FUNC 0x001C9640 gt2k_menu::ButtonList::focusPosition
f32 ButtonList::focusPosition()
{
    Button *focused = focusButton();
    if (relative != 0) {
        f32 pos;
        if (controlType == 0) {
            pos = heightBeforeFocus();
            if (focused != NULL)
                pos += focused->h * 0.5f;
        } else {
            pos = widthBeforeFocus();
            if (focused != NULL)
                pos += focused->w * 0.5f;
        }
        return pos;
    }
    if (focused != NULL)
        return (controlType == 0) ? focused->y.value : focused->x.value;
    return 0.0f;
}

/* The display value relative to the list's leading edge (ScrollBar's
 * draw).  The trailing "- 0" is in the code (? an inlined offset that is 0). */
// FUNC 0x001C9700 gt2k_menu::ButtonList::scrollOffset
f32 ButtonList::scrollOffset()
{
    f32 offset = 0.0f;                      /* loaded before the branch, as retail's mtc1 */
    if (controlType == 0)
        return (displayY.value - (y.value - h * 0.5f)) - offset;
    return (displayX.value - (x.value - w * 0.5f)) - offset;
}

/* Slot 8: the button at the cursor; NULL while the focus is disabled. */
// FUNC 0x001C9760 gt2k_menu::ButtonList::focusButton
Button *ButtonList::focusButton()
{
    if (focusDisabled != 0)
        return NULL;
    int i = 0;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL) {
        if (i == index.value)
            return button;
        i++;
    }
    return NULL;
}

/* The record of the focused button, NULL if none. */
// FUNC 0x001C97D8 gt2k_menu::ButtonList::focusData
ButtonData *ButtonList::focusData()
{
    Button *focused = focusButton();
    if (focused != NULL)
        return focused->linkMenu;
    return NULL;
}

/* Slot 10. */
// FUNC 0x001C9810 gt2k_menu::ButtonList::getCursor
int ButtonList::getCursor()
{
    return index.value;
}

/* The widths of all the buttons plus the spacing between them. */
// FUNC 0x001C9818 gt2k_menu::ButtonList::totalWidth
f32 ButtonList::totalWidth()
{
    f32 sum = 0.0f;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL)
        sum += button->w;
    return sum + spacing * (f32)(count() - 1);
}

// FUNC 0x001C98A8 gt2k_menu::ButtonList::totalHeight
f32 ButtonList::totalHeight()
{
    f32 sum = 0.0f;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL)
        sum += button->h;
    return sum + spacing * (f32)(count() - 1);
}

/* Width plus spacing of the buttons before the focused one (all of them if
 * none is focused; 0 while the focus is disabled). */
// FUNC 0x001C9938 gt2k_menu::ButtonList::widthBeforeFocus
f32 ButtonList::widthBeforeFocus()
{
    if (focusDisabled != 0)
        return 0.0f;
    f32 sum = 0.0f;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL && button->focused == 0)
    {
        sum += button->w;                   /* then spacing, both into sum (retail's two add.s) */
        sum += spacing;
    }
    return sum;
}

/* Width plus spacing of the buttons after the focused one (0 if none is
 * focused; does not test focusDisabled). */
// FUNC 0x001C99B8 gt2k_menu::ButtonList::widthAfterFocus
f32 ButtonList::widthAfterFocus()
{
    f32 sum = 0.0f;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL && button->focused == 0)
        ;
    while ((button = it.next()) != NULL)
    {
        sum += button->w;                   /* then spacing, both into sum (retail's two add.s) */
        sum += spacing;
    }
    return sum;
}

// FUNC 0x001C9A48 gt2k_menu::ButtonList::heightBeforeFocus
f32 ButtonList::heightBeforeFocus()
{
    if (focusDisabled != 0)
        return 0.0f;
    f32 sum = 0.0f;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL && button->focused == 0)
    {
        sum += button->h;
        sum += spacing;
    }
    return sum;
}

// FUNC 0x001C9AC8 gt2k_menu::ButtonList::heightAfterFocus
f32 ButtonList::heightAfterFocus()
{
    f32 sum = 0.0f;
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL && button->focused == 0)
        ;
    while ((button = it.next()) != NULL)
    {
        sum += button->h;
        sum += spacing;
    }
    return sum;
}

/* Slot 7: the display value that shows the focused button (relative to
 * the list's centre).  The buttons fit: centred if horizontal, from the top
 * if vertical; otherwise the start while the focused one is in the first
 * half, the end while it is in the last half, else the focused one centred. */
// FUNC 0x001C9B58 gt2k_menu::ButtonList::scrollTarget
f32 ButtonList::scrollTarget()
{
    f32 size, total, before, after;
    if (controlType == 0) {
        size = h;
        total = totalHeight();
        before = heightBeforeFocus();
        after = heightAfterFocus();
    } else {
        size = w;
        total = totalWidth();
        before = widthBeforeFocus();
        after = widthAfterFocus();
    }
    f32 target;
    if (size < total) {
        f32 half = size * 0.5f;
        if (before < half) {
            target = 0.0f;
        } else if (after < half) {
            target = size - total;
        } else {
            Button *focused = focusButton();
            f32 focusedSize;
            if (focused != NULL)
                focusedSize = (controlType == 0) ? focused->h : focused->w;
            else
                focusedSize = 0.0f;
            target = size * 0.5f - (before + focusedSize * 0.5f);
        }
    } else if (controlType == 0) {
        target = 0.0f;
    } else {
        target = (size - total) * 0.5f;
    }
    return target - size * 0.5f;
}

/* Slot 1 (the per-frame update): updates the two pictures, then eases the
 * display towards the scroll target (rate 0.3; at once when the cursor has
 * just wrapped around) and keeps |target - display| for getInterpolateDiff
 * (0 for a list that does not scroll).  The difference goes through double
 * (soft-float calls). */
// FUNC 0x001C9CA8 gt2k_menu::ButtonList::virtual_1
void ButtonList::virtual_1()
{
    if (focusItem != NULL)
        focusItem->virtual_1();
    if (disableItem != NULL)
        disableItem->virtual_1();
    f32 target = scrollTarget();
    MotionValue *display = (controlType != 0) ? &displayX : &displayY;
    if (index.wrapped != 0) {
        display->force = 0.0f;              /* not set(): the stores in the order +0, +4, +8 */
        display->velocity = 0.0f;
        display->value = target;
    } else {
        display->approach(target, 0.3f);
    }
    if (relative != 0) {
        double diff = target - display->value; /* implicit: fptodp, dpcmp, dpsub, dptofp (double arithmetic) */
        if (diff < 0.0)
            diff = 0.0 - diff;              /* (? fabs) __subdf3(0, diff) */
        interpolateDiff = (f32)diff;
    } else {
        interpolateDiff = 0.0f;
    }
}

/* Shows (hide 0) or hides (hide 1) every button. */
// FUNC 0x001C9DC0 gt2k_menu::ButtonList::hideButtons
void ButtonList::hideButtons(int hide)
{
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL)
        button->virtual_9(hide ^ 1);        /* in the call: loop.c hoists it (retail's xori in the
                                               b's delay slot) */
}

/* Slot 12: one frame of input.  The concurrent button, else the focused
 * one, sees the input first; unless it returns 0, left / up moves the cursor
 * back, right / down forward (with the cursor sound when it moved), decide
 * runs the virtual decide().  Nothing without buttons. */
// FUNC 0x001C9E38 gt2k_menu::ButtonList::doControl
void ButtonList::doControl(const MenuControl &ctl)
{
    if (count() == 0)
        return;
    int handle = 1;
    if (concurrent != NULL) {               /* two calls (jump2 cross-jumps them into retail's one;
                                               handle's third set makes it outrank moved: $s2) */
        handle = concurrent->virtual_11(ctl);
    } else {
        Button *focused = focusButton();
        if (focused != NULL)
            handle = focused->virtual_11(ctl);
    }
    int moved = -1;
    if (handle != 0) {
        if (ctl.getPrev(controlType))
            moved = movePrev();
        else if (ctl.getNext(controlType))
            moved = moveNext();
        else if (ctl.decide != 0)
            decide();
    }
    if (moved == 1)
        menu_play_se(MENU_SE_LIST_MOVE);
}

/* Moves the cursor back, over the buttons whose focus is disabled.  When it
 * cannot move any more and stopped on such a button, it steps forward again
 * (once: an earlier skipped run is not undone).  Returns the last move's
 * result (0: did not move). */
// FUNC 0x001C9F48 gt2k_menu::ButtonList::movePrev
int ButtonList::movePrev()
{
    Button *button = NULL;
    int moved;
    /* Through the inline accessors (getIndex, getFocusButton, isDisableFocus):
     * their expansion makes the loop test longer than the 30 insns stmt.c
     * scans for exits, so the disableFocus test stays at the loop top behind
     * retail's `b' into the moves. */
    while ((moved = getIndex()->move(-1)) != 0 && (button = getFocusButton()) != NULL
           && button->isDisableFocus())
        ;
    if (moved != 0)
        refreshFocus();
    else if (button != NULL && button->isDisableFocus())
        getIndex()->move(1);
    return moved;
}

// FUNC 0x001CA020 gt2k_menu::ButtonList::moveNext
int ButtonList::moveNext()
{
    Button *button = NULL;
    int moved;
    while ((moved = getIndex()->move(1)) != 0 && (button = getFocusButton()) != NULL
           && button->isDisableFocus())
        ;
    if (moved != 0)
        refreshFocus();
    else if (button != NULL && button->isDisableFocus())
        getIndex()->move(-1);
    return moved;
}

/* Slot 11: decides the focused button with the decide effect (no NULL check
 * of it). */
// FUNC 0x001CA0F8 gt2k_menu::ButtonList::decide
void ButtonList::decide()
{
    if (count() != 0)
        decideButton(focusButton()->linkMenu, 1);
}

/* Marks the focused button decided; unless `data' is a sub list: if the
 * button is available, calls the decide callback (effect: play the decide
 * effect, PageManager::onButton), else the refused sound. */
// FUNC 0x001CA150 gt2k_menu::ButtonList::decideButton
void ButtonList::decideButton(ButtonData *data, int effect)
{
    Button *focused = focusButton();
    if (focused == NULL)
        return;
    focused->decide = 1;
    if (is_sub_list(data))
        return;
    if (!focused->isAvailable())
        menu_play_se(MENU_SE_REFUSED);
    else if (decideFunc != NULL)
        decideFunc(decideCtx, data, index.value, this, effect);
}

/* Sorts the buttons with cmp (List<Button> slot 1, pList::sort), lays them
 * out again from 0, keeps the cursor on the focused one and puts the display
 * on the scroll target at once.  Nothing for a list that does not scroll. */
// FUNC 0x001CA1F8 gt2k_menu::ButtonList::sortButtons
void ButtonList::sortButtons(int (*cmp)(const void *, const void *))
{
    if (relative == 0)
        return;
    f32 pos = 0.0f;
    Button *focused = NULL;
    sort(cmp);
    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL) {
        pos += ((controlType == 0) ? button->h : button->w) * 0.5f;
        button->setOffPoint(pos);           /* implicit: placeOffPoint (setOffPoint, inline) */
        pos += ((controlType == 0) ? button->h : button->w) * 0.5f;
        pos += spacing;
        if (button->isFocused())
            focused = button;
    }
    if (focused != NULL) {
        int i = indexOf(focused);           /* first: retail sets &index in set's delay slot */
        index.set(i);
    }
    f32 target = scrollTarget();
    if (controlType == 0)                   /* as refresh */
        displayY.set(target);
    else
        displayX.set(target);
}

// FUNC 0x001CA358 gt2k_menu::List<gt2k_menu::Button>::~List
/* inline in game/menu/framework/menu_list.h; egcs emits this copy (vtable 0x00336C48 slot
 * 0): the List<Button> vptr, then pList::~pList with the flags */
/* implicit: ~pList (base destructor) */

// FUNC 0x001CA380 gt2k_menu::ButtonList::getDisableItem
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA388 gt2k_menu::ButtonList::getFocusItem
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

/* No callers (the virtual focusButton, slot 8). */
// FUNC 0x001CA390 gt2k_menu::ButtonList::getFocusButton
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA3B8 gt2k_menu::ButtonList::isEnabledFocus
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA3C8 gt2k_menu::ButtonList::getInterpolateDiff
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA3D0 gt2k_menu::ButtonList::getDisplayY
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA3D8 gt2k_menu::ButtonList::getDisplayX
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA3E0 gt2k_menu::ButtonList::getIndex
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA3E8 gt2k_menu::ButtonList::getValue
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA3F0 gt2k_menu::ButtonList::getActive
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

// FUNC 0x001CA3F8 gt2k_menu::ButtonList::setActive
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

/* The list's own size along its axis (no callers). */
// FUNC 0x001CA400 gt2k_menu::ButtonList::getSize
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

/* The buttons' total size along the axis (no callers). */
// FUNC 0x001CA420 gt2k_menu::ButtonList::getTotalSize
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy (totalHeight /
 * totalWidth) */
/* implicit: totalWidth, totalHeight (the inline body) */

// FUNC 0x001CA458 gt2k_menu::ButtonList::getControlType
/* inline in game/menu/framework/T244_button_list.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo functions (egcs emits them by itself). */

/* __class_type_info, two bases (list 0x00336CA8). */
// FUNC 0x001CA460 __tfQ29gt2k_menu10ButtonList
extern "C" void *__tfQ29gt2k_menu10ButtonList(void)
{
    if (__tiQ29gt2k_menu10ButtonList[0] == 0) {
        __tfQ29gt2k_menut4List1ZQ29gt2k_menu6Button();
        __tfQ29gt2k_menu8Drawable();
        __rtti_class(__tiQ29gt2k_menu10ButtonList, "Q29gt2k_menu10ButtonList", g_00336CA8, 2);
    }
    return __tiQ29gt2k_menu10ButtonList;
}

/* __si_type_info, base pList. */
// FUNC 0x001CA4C0 __tfQ29gt2k_menut4List1ZQ29gt2k_menu6Button
extern "C" void *__tfQ29gt2k_menut4List1ZQ29gt2k_menu6Button(void)
{
    if (__tiQ29gt2k_menut4List1ZQ29gt2k_menu6Button[0] == 0) {
        __tfQ29gt2k_menu5pList();
        __rtti_si(__tiQ29gt2k_menut4List1ZQ29gt2k_menu6Button,
                  "Q29gt2k_menut4List1ZQ29gt2k_menu6Button",
                  (const type_info *)__tiQ29gt2k_menu5pList);
    }
    return __tiQ29gt2k_menut4List1ZQ29gt2k_menu6Button;
}

/* T244's copy of the typeinfo function of the template instance
 * SmartIndexT<int> (__user_type_info; T017's 0x00103690 is the one called,
 * all copies build the node 0x003532F8).  No callers.  Weak so that the
 * copies can be linked. */
// FUNC 0x001CA510 __tfQ29gt2k_menut11SmartIndexT1Zi
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut11SmartIndexT1Zi(void)
{
    if (__tiQ29gt2k_menut11SmartIndexT1Zi[0] == 0)
        __rtti_user(__tiQ29gt2k_menut11SmartIndexT1Zi, "Q29gt2k_menut11SmartIndexT1Zi");
    return __tiQ29gt2k_menut11SmartIndexT1Zi;
}
#endif /* !BOOT_OLD_GCC */
