/* T073 - the memory card menu classes: gt2k_menu::MemorycardMenuItem,
 * MemorycardMenuItemItem, MemorycardLayerMenuItem, MemorycardMenu,
 * MemorycardLayerMenu (and T073's copies of the inline destructors of
 * List<MemorycardMenuItem> / Cell<MemorycardMenuItem>).
 * 0x001507F8..0x00151AD0.
 *
 * The operation menus and dialogs of T074..T076 derive from these: a
 * MemorycardLayerMenu is the objects of one layer of the page's layout
 * with a list of items; each frame the open menu gets select() (the cursor
 * moves between the selectable items; decide on a MemorycardLayerMenuItem
 * makes its sub menu, which then gets the input until it is cancelled) and
 * update() (the items' images, the menu's fade in / out).  Everything is
 * allocated on the menu heap (MemorycardMenuItem's operator new).
 *
 * Note on names: index.txt calls 0x00150A88 / 0x00150CA8 / 0x00150D58
 * MemorycardLayerMenu::virtual_3 / 4 / 6; MemorycardMenu's own vtable
 * (0x0031E008) has them in slots 3, 4, 6, so they are MemorycardMenu's,
 * inherited by MemorycardLayerMenu.
 *
 * Includes (data pass): retail's T073 .rodata (0x0031DD80..0x0031E420) is
 * the fade scripts g_0031DD80, the seven vtables, then the type names of
 * T073's own four classes, List / MemorycardMenuItem / Cell<
 * MemorycardMenuItem>, MemorycardControl, GarageCarList with List /
 * GarageCar / Cell<GarageCar>, MemorycardFileManagerBase with List / Cell<
 * MemorycardFile>, SmartIndexT<int>, the operator holders and Operator last
 * (none of T076's dialogs): the first six headers complete those classes in
 * the reverse of that order (MemorycardControl through its class header,
 * without T075's screens). */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (clearOperators); OperatorHolder,
                                               OperatorList */
#include "game/menu/framework/T211_page_manager.h"         /* (SmartIndexT<int>) */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* (no use: retail's T073 has MemorycardFileManagerBase's
                                               names) */
#include "game/save/T429_garage_car_list.h"      /* (no use: retail's T073 has GarageCarList's names) */
#include "game/menu/memorycard/T068_memorycard_control_class.h" /* gt2k_menu::MemorycardControl (ControlBase's pad
                                               input control1) */
#include "game/menu/memorycard/T073_memorycard_menu.h"      /* the menu classes, MemorycardMenuResult; menu_list.h */
/* the rest */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current; pdistd::Allocator */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, disappearAll, forEach) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable (anchor, anchorType, visible) */
#include "game/menu/memorycard/memorycard_menu_internal.h"

namespace gt2k_menu {

/* A list link of a MemorycardMenu as its item: Cell<MemorycardMenuItem> is
 * the item's first base (+0), so the conversion is free. */
inline MemorycardMenuItem *mc_menu_item(pCell *cell)
{
    return static_cast<MemorycardMenuItem *>(static_cast<Cell<MemorycardMenuItem> *>(cell));
}

/* ======================================================================== */
/* MemorycardMenuItem: allocation                                           */
/* ======================================================================== */

/* The current menu heap, 16-aligned; on failure the heap's debug dump. */
// FUNC 0x001507F8 gt2k_menu::MemorycardMenuItem::operator new
void *MemorycardMenuItem::operator new(size_t size)
{
    void *p = menu_heap_current()->allocate(size, 0x10);

    if (p == NULL)
        menu_heap_current()->debug();
    return p;
}

/* (unreferenced) */
// FUNC 0x00150850 gt2k_menu::MemorycardMenuItem::operator new[]
void *MemorycardMenuItem::operator new[](size_t size)
{
    return MemorycardMenuItem::operator new(size);
}

// FUNC 0x00150870 gt2k_menu::MemorycardMenuItem::operator delete
void MemorycardMenuItem::operator delete(void *p)
{
    menu_heap_current()->free(p);
}

/* (unreferenced) */
// FUNC 0x001508A0 gt2k_menu::MemorycardMenuItem::operator delete[]
void MemorycardMenuItem::operator delete[](void *p)
{
    MemorycardMenuItem::operator delete(p);
}

/* ======================================================================== */
/* MemorycardMenu                                                           */
/* ======================================================================== */

/* (The bases inline: the cell's links and the list's head / tail cleared,
 * the vptrs.) */
// FUNC 0x001508C0 gt2k_menu::MemorycardMenu::MemorycardMenu
MemorycardMenu::MemorycardMenu(int vertical, int cancelable)
{
    this->vertical = vertical;
    this->cancelable = cancelable;
    active = 1;
    current = NULL;
}

// FUNC 0x00150920 gt2k_menu::MemorycardMenu::~MemorycardMenu
MemorycardMenu::~MemorycardMenu()
{
    /* The items are not deleted here.  The bases' inline destructors:
     * ~List<MemorycardMenuItem> (its vptr, gt2k_menu::pList::~pList(this +
     * 0xC, 0)) and the implicit ~MemorycardMenuItem (Cell's vptr,
     * gt2k_menu::pCell::~pCell(this, 0)), then MemorycardMenuItem::operator
     * delete if flags & 1. */
    /* implicit: ~pList, ~pCell, operator delete (the bases' destructors, flags & 1) */
}

/* The first selectable item after item (NULL if none). */
// FUNC 0x001509A8 gt2k_menu::MemorycardMenu::nextSelectable
MemorycardMenuItem *MemorycardMenu::nextSelectable(MemorycardControl &ctl, MemorycardMenuItem *item)
{
    if (item == NULL)
        return NULL;
    while ((item = mc_menu_item(item->next)) != NULL) {
        if (item->isSelectable(ctl))
            return item;
    }
    return NULL;
}

/* The first selectable item before item (NULL if none). */
// FUNC 0x00150A10 gt2k_menu::MemorycardMenu::prevSelectable
MemorycardMenuItem *MemorycardMenu::prevSelectable(MemorycardControl &ctl, MemorycardMenuItem *item)
{
    if (item == NULL)
        return NULL;
    while ((item = mc_menu_item(item->prev)) != NULL) {
        if (item->isSelectable(ctl))
            return item;
    }
    return NULL;
}

/* The cursor on the last item of the list (T074..T076, after building a
 * menu). */
// FUNC 0x00150A78 gt2k_menu::MemorycardMenu::selectLast
void MemorycardMenu::selectLast()
{
    current = mc_menu_item(tail);
}

/* The current item gets the input first; when it uses it (code != 0) its
 * result is returned and the menu is not active.  Else: cancel returns
 * {2, 0} if the menu is cancelable; up / down (left / right when not
 * vertical) move to the previous / next selectable item; a cursor left on
 * an item that is no longer selectable moves to a selectable one.  Sound 3
 * when the cursor moved, 4 when a direction did not move it. */
// FUNC 0x00150A88 gt2k_menu::MemorycardMenu::select
MemorycardMenuResult MemorycardMenu::select(MemorycardControl &ctl)
{
    const MenuControl &pad = ctl.control1;
    MemorycardMenuResult result;
    MemorycardMenuItem *old;
    MemorycardMenuItem *item;

    if (current != NULL) {
        result = current->select(ctl);
        if (result.code != MC_RESULT_NONE) {
            active = 0;
            return result;
        }
    } else {
        MemorycardMenuItem *first = mc_menu_item(head);

        if (first != NULL && first->isSelectable(ctl))
            current = mc_menu_item(head);
        else
            current = nextSelectable(ctl, mc_menu_item(head));
    }
    active = 1;
    if (pad.cancel != 0 && cancelable != 0)
        return MemorycardMenuResult(MC_RESULT_CANCELLED, 0);
    old = current;
    if (old != NULL) {
        if (vertical != 0 ? pad.up != 0 : pad.left != 0) {
            item = prevSelectable(ctl, current);
            if (item != NULL)
                current = item;
        }
        if (vertical != 0 ? pad.down != 0 : pad.right != 0) {
            item = nextSelectable(ctl, current);
            if (item != NULL)
                current = item;
        }
        if (!current->isSelectable(ctl)) {
            item = prevSelectable(ctl, current);
            if (item == NULL)
                item = nextSelectable(ctl, current);
            current = item;
        }
    }
    if (pad.up != 0 || pad.down != 0 || pad.right != 0 || pad.left != 0) {
        if (old != current)
            menu_play_se(MENU_SE_CURSOR);
        else
            menu_play_se(MENU_SE_REFUSED);
    }
    return MemorycardMenuResult(MC_RESULT_USED, 0);
}

// FUNC 0x00150CA8 gt2k_menu::MemorycardMenu::isSelectable
bool MemorycardMenu::isSelectable(MemorycardControl &ctl)
{
    return true;
}

/* update(ctl, sel && item == current) on every item. */
// FUNC 0x00150CB0 gt2k_menu::MemorycardMenu::update
void MemorycardMenu::update(MemorycardControl &ctl, bool sel)
{
    ListIterator<MemorycardMenuItem> it(this);
    MemorycardMenuItem *item;

    while ((item = it.next()) != NULL)
        item->update(ctl, sel ? item == current : false);
}

/* render(ctl, sel && item == current) on every item. */
// FUNC 0x00150D58 gt2k_menu::MemorycardMenu::render
void MemorycardMenu::render(MemorycardControl &ctl, bool sel)
{
    ListIterator<MemorycardMenuItem> it(this);
    MemorycardMenuItem *item;

    while ((item = it.next()) != NULL)
        item->render(ctl, sel ? item == current : false);
}

/* ======================================================================== */
/* MemorycardMenuItemItem                                                   */
/* ======================================================================== */

/* The images name and name + "_sel" of the layout (new Item: Object's
 * operator new, the menu heap), both under parent and placed relative to it
 * (anchor type 3), hidden until the first update; the second drawn in
 * front.  (blinkTime is cleared inside the loop, as in the original.) */
// FUNC 0x00150E00 gt2k_menu::MemorycardMenuItemItem::MemorycardMenuItemItem
MemorycardMenuItemItem::MemorycardMenuItemItem(PageLayout *layout, Object *parent, const char *name)
{
    char selName[0x80];
    int i;

    /* The landing pads: Object::operator delete of an Item whose
     * constructor threw, then the base's cleanup (Cell's vptr,
     * gt2k_menu::pCell::~pCell(this, 0)). */
    /* implicit: ~pCell (the base's cleanup on a throw) */
    images[0] = new Item(name, layout); /* implicit: Object::operator delete (if the constructor throws) */
    strcpy(selName, name);
    strcat(selName, "_sel");
    images[1] = new Item(selName, layout); /* implicit: Object::operator delete (if the constructor throws) */
    /* images[i] read anew after attachTo (retail reloads it), anchor stored
     * before anchorType */
    for (i = 0; i < 2; i++) {
        blinkTime = 0.0f;
        images[i]->visible = 0;
        images[i]->attachTo(parent);
        images[i]->anchor = static_cast<const Drawable *>(parent);
        images[i]->anchorType = MC_ANCHOR_XY;
    }
    blink = 1;
    dim = 0;
    images[1]->z += 1.0f;
}

// FUNC 0x00150F90 gt2k_menu::MemorycardMenuItemItem::select
MemorycardMenuResult MemorycardMenuItemItem::select(MemorycardControl &ctl)
{
    return MemorycardMenuResult(MC_RESULT_NONE, 0);
}

/* The image of the state at alpha 0.5 (0.1 when dimmed); blinking and
 * selected: the first image stays, the "_sel" one's alpha runs down from
 * 0.5 over 30 frames and starts again. */
// FUNC 0x00150FA8 gt2k_menu::MemorycardMenuItemItem::update
void MemorycardMenuItemItem::update(MemorycardControl &ctl, bool sel)
{
    f32 alpha = 0.5f;
    Item *image;
    int on;

    if (dim != 0)
        alpha = 0.1f;
    if (blink != 0) {
        if (sel) {
            blinkTime -= 1.0f / 30.0f;
            if (blinkTime < 0.0f)
                blinkTime = 1.0f;
            alpha *= blinkTime;
        }
        images[0]->visible = 1;
    } else {
        images[0]->visible = !sel;
    }
    images[1]->visible = sel;
    /* sel != 0 in a variable: retail has sltu; sll 2 (images[sel != 0]
     * folds to `sel ? 4 : 0', a movz) */
    on = sel != 0;
    image = images[on];
    image->color[0].a = alpha;
    image->color[1].a = alpha;
    image->color[2].a = alpha;
    image->color[3].a = alpha;
}

/* ======================================================================== */
/* MemorycardLayerMenu                                                      */
/* ======================================================================== */

/* The objects of layer `name' of the layout, under parent; with
 * anchorName, that element of the layer places the menu (setPosition) and
 * gives its depth.  (If buildLayer throws, the landing pad runs
 * ~MemorycardMenu(this, 0).) */
// FUNC 0x00151050 gt2k_menu::MemorycardLayerMenu::MemorycardLayerMenu
MemorycardLayerMenu::MemorycardLayerMenu(int vertical, int cancelable, PageLayout *layout,
                                         Object *parent, const char *name,
                                         const char *anchorName)
    : MemorycardMenu(vertical, cancelable)
{
    this->layout = layout;
    anchor = NULL;
    shown = 1;
    autoHide = 1;
    root = layout->buildLayer(name, 0);
    root->attachTo(parent);
    if (anchorName != NULL) {
        anchor = PageLayout::findItemIn(anchorName, this->layout->getLayer(name));
        if (anchor != NULL)
            setPriority(anchor->z);
    }
}

// FUNC 0x00151158 gt2k_menu::MemorycardLayerMenu::~MemorycardLayerMenu
MemorycardLayerMenu::~MemorycardLayerMenu()
{
    /* The layer's objects close (their default close scripts, through
     * disappearAll(NULL)); root itself is not released here.  Then
     * MemorycardMenu::~MemorycardMenu(this, 0) and MemorycardMenuItem::
     * operator delete if flags & 1. */
    root->disappearAll(NULL);
    /* implicit: ~MemorycardMenu, operator delete (the base, flags & 1) */
}

/* The items, then the fade: out while not active (autoHide), in while
 * active. */
// FUNC 0x001511C8 gt2k_menu::MemorycardLayerMenu::update
void MemorycardLayerMenu::update(MemorycardControl &ctl, bool sel)
{
    MemorycardMenu::update(ctl, sel);
    if (active != 0) {
        if (shown == 0)
            setShown(1, 1);
    } else if (autoHide != 0 && shown != 0) {
        setShown(0, 1);
    }
}

/* setShown's walk (an ObjectFunc): every Transitive of the layer loses its
 * running operators. */
// FUNC 0x00151240 gt2k_menu::stopOperators
static void stopOperators(Object *obj, void *a, void *b, void *c)
{
    Transitive *t = dynamic_cast<Transitive *>(obj);

    if (t != NULL)
        t->clearOperators();
}

/* One instruction of the fade scripts: code (T217's Operator::op* index),
 * the operands f10 / f14, result (1 ends the frame); the rest 0. */
#define U028_OP(code, f10, f14, result) \
    { (code), { 0 }, 0, { 0, 0, 0, 0 }, (f10), (f14), 0.0f, 0.0f, 0.0f, \
      { 0, 0, 0, 0, 0, 0, 0, 0 }, (result) }

/* setShown's scripts (.rodata, T073's first data: an extern const array is
 * written where it is defined): [0] fade in, opFadeAlpha +4 / s for 0.25 s;
 * [1] opSetAlpha 1; [2] fade out, opFadeAlpha -4 / s for 0.25 s; [3]
 * opSetAlpha 0; each then opRemove. */
const OperatorCode g_0031DD80[4][2] = {     /* 0x0031DD80 */
    { U028_OP(25 /* opFadeAlpha */,  4.0f, 0.25f, 1), U028_OP(3 /* opRemove */, 0.0f, 0.0f, 1) },
    { U028_OP(24 /* opSetAlpha */,   1.0f, 0.0f,  0), U028_OP(3 /* opRemove */, 0.0f, 0.0f, 1) },
    { U028_OP(25 /* opFadeAlpha */, -4.0f, 0.25f, 1), U028_OP(3 /* opRemove */, 0.0f, 0.0f, 1) },
    { U028_OP(24 /* opSetAlpha */,   0.0f, 0.0f,  0), U028_OP(3 /* opRemove */, 0.0f, 0.0f, 1) }
};

/* Shows / hides the layer: the running operators stopped, then the fade of
 * g_0031DD80 (animate) or the alpha set at once. */
// FUNC 0x001512A0 gt2k_menu::MemorycardLayerMenu::setShown
void MemorycardLayerMenu::setShown(int show, int animate)
{
    const OperatorCode *code;
    int fade;

    root->forEach(stopOperators, NULL, NULL, NULL);
    fade = show != 0 ? 0 : 2;
    if (animate != 0) {
        code = g_0031DD80[fade];
    } else {
        /* its own variable: retail multiplies (fade + 1) * 0x60, where
         * g_0031DD80[fade + 1] folds the + 1 into the address */
        int set = fade + 1;

        code = g_0031DD80[set];
    }
    root->disappearAll(code);
    shown = show;
}

/* Places the layer so that the anchor's centre is at x, y (y measured up
 * from the bottom of the screen). */
// FUNC 0x00151340 gt2k_menu::MemorycardLayerMenu::setPosition
void MemorycardLayerMenu::setPosition(f32 x, f32 y)
{
    if (anchor != NULL) {
        x -= anchor->x + anchor->w * 0.5f;
        y -= MC_SCREEN_HEIGHT - (anchor->y + anchor->h * 0.5f);
    }
    root->x.set(x);
    root->y.set(y);
}

// FUNC 0x001513B0 gt2k_menu::MemorycardLayerMenu::setPriority
void MemorycardLayerMenu::setPriority(f32 z)
{
    root->z = z;
}

// FUNC 0x001513C0 gt2k_menu::MemorycardLayerMenu::getWidth
f32 MemorycardLayerMenu::getWidth()
{
    if (anchor != NULL)
        return anchor->w;
    return 1.0f;
}

// FUNC 0x001513E0 gt2k_menu::MemorycardLayerMenu::getHeight
f32 MemorycardLayerMenu::getHeight()
{
    if (anchor != NULL)
        return anchor->h;
    return 1.0f;
}

/* Puts obj (an item's object, T074..T076) under the layer, placed relative
 * to it (anchor type 3). */
// FUNC 0x00151400 gt2k_menu::MemorycardLayerMenu::attachObject
void MemorycardLayerMenu::attachObject(Transitive *obj)
{
    if (obj != NULL) {
        obj->attachTo(root);
        obj->anchorType = MC_ANCHOR_XY;
        obj->anchor = root;
    }
}

/* ======================================================================== */
/* MemorycardLayerMenuItem                                                  */
/* ======================================================================== */

// FUNC 0x00151450 gt2k_menu::MemorycardLayerMenuItem::MemorycardLayerMenuItem
MemorycardLayerMenuItem::MemorycardLayerMenuItem(PageLayout *layout, Object *parent,
                                                 const char *name)
    : MemorycardMenuItemItem(layout, parent, name)
{
    subMenu = NULL;
}

// FUNC 0x00151488 gt2k_menu::MemorycardLayerMenuItem::~MemorycardLayerMenuItem
MemorycardLayerMenuItem::~MemorycardLayerMenuItem()
{
    /* The sub menu is deleted (its virtual destructor, flags 3); the bases'
     * implicit destructors leave only Cell's vptr and
     * gt2k_menu::pCell::~pCell(this, 0); then MemorycardMenuItem::operator
     * delete if flags & 1. */
    delete subMenu;
    /* implicit: ~pCell, operator delete (the bases, flags & 1) */
}

/* Without a sub menu: checkDecide.  With one: its result, except that a
 * cancelled sub menu (code 2) is deleted and gives {1, 0xB}. */
// FUNC 0x00151510 gt2k_menu::MemorycardLayerMenuItem::select
MemorycardMenuResult MemorycardLayerMenuItem::select(MemorycardControl &ctl)
{
    MemorycardMenuResult result;

    if (subMenu != NULL) {
        result = subMenu->select(ctl);
        if (result.code != MC_RESULT_USED) {
            if (result.code == MC_RESULT_CANCELLED) {
                delete subMenu;
                subMenu = NULL;
                return MemorycardMenuResult(MC_RESULT_USED, MC_ARG_SUB_MENU_CLOSED);
            }
        }
        return result;
    }
    return checkDecide(ctl);
}

/* Decide on a selectable item: the decide sound and makeSubMenu. */
// FUNC 0x001515C8 gt2k_menu::MemorycardLayerMenuItem::checkDecide
MemorycardMenuResult MemorycardLayerMenuItem::checkDecide(MemorycardControl &ctl)
{
    if (ctl.control1.decide != 0 && isSelectable(ctl)) {
        menu_play_se(MENU_SE_CONFIRM);
        return makeSubMenu(ctl);
    }
    return MemorycardMenuResult(MC_RESULT_NONE, 0);
}

/* The sub menu (its images stop blinking meanwhile), updateItem, then the
 * images. */
// FUNC 0x00151650 gt2k_menu::MemorycardLayerMenuItem::update
void MemorycardLayerMenuItem::update(MemorycardControl &ctl, bool sel)
{
    if (subMenu != NULL) {
        blink = 0;
        subMenu->update(ctl, sel);
    } else {
        blink = 1;
    }
    updateItem(ctl, sel);
    MemorycardMenuItemItem::update(ctl, sel);
}

// FUNC 0x001516E8 gt2k_menu::MemorycardLayerMenuItem::render
void MemorycardLayerMenuItem::render(MemorycardControl &ctl, bool sel)
{
    if (subMenu != NULL)
        subMenu->render(ctl, sel);
}

} /* namespace gt2k_menu */

/* ======================================================================== */
/* The template members (T073's copies)                                     */
/* ======================================================================== */

// FUNC 0x00151720 gt2k_menu::List<gt2k_menu::MemorycardMenuItem>::~List
/* inline in game/menu/framework/menu_list.h; egcs emits this copy with the vtable of
 * List<MemorycardMenuItem>: vptr = 0x0031E048, then
 * gt2k_menu::pList::~pList(this, flags) */
/* implicit: ~pList (inline destructor's copy) */

// FUNC 0x00151748 gt2k_menu::Cell<gt2k_menu::MemorycardMenuItem>::~Cell
/* inline in game/menu/framework/menu_list.h; egcs emits this copy with the vtable of
 * Cell<MemorycardMenuItem>: vptr = 0x0031E0A0, then
 * gt2k_menu::pCell::~pCell(this, flags) */
/* implicit: ~pCell (inline destructor's copy) */

/* ======================================================================== */
/* The end of T073: egcs's deferred output, per class its inline members   */
/* (game/menu/memorycard/T073_memorycard_menu.h), its typeinfo function and its implicit   */
/* destructor                                                               */
/* ======================================================================== */

// FUNC 0x00151770 gt2k_menu::MemorycardLayerMenuItem::updateItem
/* inline in game/menu/memorycard/T073_memorycard_menu.h (empty); egcs emits this copy */

// FUNC 0x00151778 gt2k_menu::MemorycardLayerMenuItem::makeSubMenu
/* inline in game/menu/memorycard/T073_memorycard_menu.h ({MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED}:
 * nothing opened); egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x00151798 __tfQ29gt2k_menu23MemorycardLayerMenuItem
extern "C" void *__tfQ29gt2k_menu23MemorycardLayerMenuItem(void)
{
    if (__tiQ29gt2k_menu23MemorycardLayerMenuItem[0] == 0) {
        __tfQ29gt2k_menu22MemorycardMenuItemItem();
        __rtti_si(__tiQ29gt2k_menu23MemorycardLayerMenuItem, "Q29gt2k_menu23MemorycardLayerMenuItem",
                  (const type_info *)__tiQ29gt2k_menu22MemorycardMenuItemItem);
    }
    return __tiQ29gt2k_menu23MemorycardLayerMenuItem;
}
#endif

// FUNC 0x001517E8 gt2k_menu::MemorycardLayerMenu::getRoot
/* inline in game/menu/memorycard/T073_memorycard_menu.h (root); egcs emits this copy */

// FUNC 0x001517F0 gt2k_menu::MemorycardLayerMenu::setAutoHide
/* inline in game/menu/memorycard/T073_memorycard_menu.h (autoHide = on); egcs emits this copy */

#ifndef BOOT_OLD_GCC
/* A __class_type_info (MemorycardMenu's multiple bases below), one base:
 * g_00351840. */
// FUNC 0x001517F8 __tfQ29gt2k_menu19MemorycardLayerMenu
extern "C" void *__tfQ29gt2k_menu19MemorycardLayerMenu(void)
{
    if (__tiQ29gt2k_menu19MemorycardLayerMenu[0] == 0) {
        __tfQ29gt2k_menu14MemorycardMenu();
        __rtti_class(__tiQ29gt2k_menu19MemorycardLayerMenu, "Q29gt2k_menu19MemorycardLayerMenu",
                     g_00351840, 1);
    }
    return __tiQ29gt2k_menu19MemorycardLayerMenu;
}
#endif

// FUNC 0x00151850 gt2k_menu::MemorycardMenuItemItem::setBlink
/* inline in game/menu/memorycard/T073_memorycard_menu.h (blink +0x1C = on); egcs emits this copy */

// FUNC 0x00151858 gt2k_menu::MemorycardMenuItemItem::setDim
/* inline in game/menu/memorycard/T073_memorycard_menu.h (dim +0x14 = on); egcs emits this copy */

// FUNC 0x00151860 gt2k_menu::MemorycardMenuItemItem::isSelectable
/* inline in game/menu/memorycard/T073_memorycard_menu.h (true); egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x00151868 __tfQ29gt2k_menu22MemorycardMenuItemItem
extern "C" void *__tfQ29gt2k_menu22MemorycardMenuItemItem(void)
{
    if (__tiQ29gt2k_menu22MemorycardMenuItemItem[0] == 0) {
        __tfQ29gt2k_menu18MemorycardMenuItem();
        __rtti_si(__tiQ29gt2k_menu22MemorycardMenuItemItem, "Q29gt2k_menu22MemorycardMenuItemItem",
                  (const type_info *)__tiQ29gt2k_menu18MemorycardMenuItem);
    }
    return __tiQ29gt2k_menu22MemorycardMenuItemItem;
}
#endif

// FUNC 0x001518B8 gt2k_menu::MemorycardMenuItemItem::~MemorycardMenuItemItem
/* implicit in game/menu/memorycard/T073_memorycard_menu.h; egcs emits this copy for the
 * vtable: Cell<MemorycardMenuItem>'s vptr, gt2k_menu::pCell::~pCell(this,
 * 0), then MemorycardMenuItem::operator delete if flags & 1 (the images
 * belong to the object tree) */
/* implicit: ~pCell, operator delete (compiler-generated destructor) */

// FUNC 0x00151910 gt2k_menu::MemorycardMenu::isActive
/* inline in game/menu/memorycard/T073_memorycard_menu.h (active); egcs emits this copy */

#ifndef BOOT_OLD_GCC
/* Two bases (g_0031E158): MemorycardMenuItem at 0, List<MemorycardMenuItem>
 * at 0xC; the typeinfo functions are called List first. */
// FUNC 0x00151918 __tfQ29gt2k_menu14MemorycardMenu
extern "C" void *__tfQ29gt2k_menu14MemorycardMenu(void)
{
    if (__tiQ29gt2k_menu14MemorycardMenu[0] == 0) {
        __tfQ29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem();
        __tfQ29gt2k_menu18MemorycardMenuItem();
        __rtti_class(__tiQ29gt2k_menu14MemorycardMenu, "Q29gt2k_menu14MemorycardMenu", g_0031E158, 2);
    }
    return __tiQ29gt2k_menu14MemorycardMenu;
}

// FUNC 0x00151978 __tfQ29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem
extern "C" void *__tfQ29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem(void)
{
    if (__tiQ29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem[0] == 0) {
        __tfQ29gt2k_menu5pList();
        __rtti_si(__tiQ29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem,
                  "Q29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem",
                  (const type_info *)__tiQ29gt2k_menu5pList);
    }
    return __tiQ29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem;
}
#endif

// FUNC 0x001519C8 gt2k_menu::MemorycardMenuItem::render
/* inline in game/menu/memorycard/T073_memorycard_menu.h (empty); egcs emits this copy */

// FUNC 0x001519D0 gt2k_menu::MemorycardMenuItem::update
/* inline in game/menu/memorycard/T073_memorycard_menu.h (empty); egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x001519D8 __tfQ29gt2k_menu18MemorycardMenuItem
extern "C" void *__tfQ29gt2k_menu18MemorycardMenuItem(void)
{
    if (__tiQ29gt2k_menu18MemorycardMenuItem[0] == 0) {
        __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem();
        __rtti_si(__tiQ29gt2k_menu18MemorycardMenuItem, "Q29gt2k_menu18MemorycardMenuItem",
                  (const type_info *)__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem);
    }
    return __tiQ29gt2k_menu18MemorycardMenuItem;
}
#endif

// FUNC 0x00151A28 gt2k_menu::MemorycardMenuItem::~MemorycardMenuItem
/* implicit in game/menu/memorycard/T073_memorycard_menu.h; egcs emits this copy for the
 * vtable: Cell<MemorycardMenuItem>'s vptr, gt2k_menu::pCell::~pCell(this,
 * 0), then MemorycardMenuItem::operator delete if flags & 1 */
/* implicit: ~pCell, operator delete (compiler-generated destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00151A80 __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem
extern "C" void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem(void)
{
    if (__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem[0] == 0) {
        __tfQ29gt2k_menu5pCell();
        __rtti_si(__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem,
                  "Q29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem;
}
#endif
