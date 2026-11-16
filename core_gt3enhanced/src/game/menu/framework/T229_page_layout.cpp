/* T229 - gt2k_menu::PageLayout: a loaded menu layout file.
 * 0x001C2318..0x001C33D8.
 *
 * The file is read whole from the volume (malloc) and relocated in place
 * (T243 relocate_archive).  It holds pages sorted by name; a page has its own
 * items and layers ("defaultLayer", "hidden", ...); a layer has plain items
 * (built as Items) and object items, built by the hook named by their type
 * (the hook table set by the screen's code, binary-searched by name).  The
 * objects go up by 100 in z per layout level.  The page transitions slide a
 * layer in from / out to the side given by inputMode.
 *
 * Data (retail .rodata 0x00335710..0x00335A58): "defaultLayer", the slide-in
 * script and openTransition's jump table, the four slide-out scripts and
 * closeTransition's jump table, then the type names SmartIndexT<int>,
 * Cell<Button>, the operator list classes (Item's), Operator: the original
 * saw the Button header (Item, Cell<Button>) and then SmartIndexT<int>, but
 * no List<Button> (no ButtonList).  T206 / T251 are read for that only.
 * .sdata 0x00352618: "hidden". */
#include "game/menu/framework/page_layout_internal.h"
#include "game/menu/framework/T206_button.h"               /* data layout only: Item's classes, Cell<Button> */
#include "game/common/T251_smart_index.h"          /* data layout only: SmartIndexT<int> */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (createItem / createObject) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (start, the layer groups) */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (the transition scripts) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, attachSorted, disappearAll,
                                               kill, operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"
#include "game/menu/framework/T259_menu_drawable.h"        /* ANCHOR_XY, MENU_SCREEN_W / _H */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue::set */
#include "game/common/T494_text_utils.h"           /* str_equal */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */

namespace gt2k_menu {

// FUNC 0x001C2318 gt2k_menu::PageLayout::PageLayout
PageLayout::PageLayout(void *owner_)
{
    owner = owner_;
    data = 0;
    page = 0;
    layer = 0;
    decided = 0;
    popup = 0;
    level = 0;
    inputMode = PAGE_SLIDE_NONE;
    hooks = 0;
    hookCount = 0;
    hookCtx = 0;
    sliderFuncs = 0;
    sliderCount = 0;
    sliderCtx = 0;
    multiFuncs = 0;
    multiCount = 0;
    multiCtx = 0;
    fileName[0] = '\0';
}

/* PageManager::initialize (0x001B6338). */
// FUNC 0x001C2368 gt2k_menu::PageLayout::initialize
void PageLayout::initialize()
{
    page = 0;
    fileName[0] = '\0';
    layer = 0;
    decided = 0;
    level = 0;
    inputMode = PAGE_SLIDE_NONE;
}

/* PageManager::finalize (0x001B6438). */
// FUNC 0x001C2388 gt2k_menu::PageLayout::finalize
void PageLayout::finalize()
{
    level = 0;
    inputMode = PAGE_SLIDE_NONE;
}

/* Loads `file' unless it is the one loaded already (1 then).  The name is
 * kept even when the file could not be read. */
// FUNC 0x001C2398 gt2k_menu::PageLayout::loadFile
int PageLayout::loadFile(const char *file)
{
    if (str_equal(file, fileName) != 0)
        return 1;
    int ok = reloadFile(file);
    strcpy(fileName, file);
    return ok;
}

// FUNC 0x001C2408 gt2k_menu::PageLayout::reloadFile
int PageLayout::reloadFile(const char *file)
{
    unloadFile();
    return readFile(file);
}

/* The whole file, unpacked, into a new block of the volume reader (no
 * buffer, size 0); 0 if it cannot be read. */
// FUNC 0x001C2448 gt2k_menu::PageLayout::readFile
int PageLayout::readFile(const char *file)
{
    MenuLayoutData *file_data =
        (MenuLayoutData *)file_read_blocking(file, 0, 0, 1, &vol_device_object);
    if (file_data == 0)
        return 0;
    data = file_data;
    relocate_archive(file_data, file_data);
    return 1;
}

// FUNC 0x001C24A8 gt2k_menu::PageLayout::unloadFile
void PageLayout::unloadFile()
{
    MenuLayoutData *file_data = data;
    if (file_data != 0) {
        data = 0;
        free(file_data);
    }
}

// FUNC 0x001C24D8 gt2k_menu::PageLayout::setHooks
void PageLayout::setHooks(const MenuHook *table, int count, void *ctx)
{
    hooks = table;
    hookCount = count;
    hookCtx = ctx;
}

// FUNC 0x001C24E8 gt2k_menu::PageLayout::setSliderFuncs
void PageLayout::setSliderFuncs(const MenuSliderFunc *table, int count, void *ctx)
{
    sliderFuncs = table;
    sliderCount = count;
    sliderCtx = ctx;
}

// FUNC 0x001C24F8 gt2k_menu::PageLayout::setMultiFuncs
void PageLayout::setMultiFuncs(const MenuMultiFunc *table, int count, void *ctx)
{
    multiFuncs = table;
    multiCount = count;
    multiCtx = ctx;
}

/* Selects page `name' and its "defaultLayer"; 0 if either is missing (the
 * page stays selected when only the layer is). */
// FUNC 0x001C2508 gt2k_menu::PageLayout::setPage
int PageLayout::setPage(const char *name, int level_)
{
    MenuPage **found_page = find_page(data->pages, data->pageCount, name);
    if (found_page != 0) {
        level = level_;
        page = *found_page;
        MenuLayer **found_layer = find_layer(page->layers, page->layerCount, "defaultLayer");
        if (found_layer == 0)
            return 0;
        inputMode = PAGE_SLIDE_NONE;
        layer = *found_layer;
        return 1;
    }
    page = 0;
    return 0;
}

// FUNC 0x001C2590 gt2k_menu::PageLayout::clearInputMode
void PageLayout::clearInputMode()
{
    inputMode = PAGE_SLIDE_NONE;
}

/* Layer `name' of the current page (which must be set), NULL if none. */
// FUNC 0x001C2598 gt2k_menu::PageLayout::getLayer
MenuLayer *PageLayout::getLayer(const char *name)
{
    MenuLayer *l;
    if (name != 0) {
        MenuLayer **found = find_layer(page->layers, page->layerCount, name);
        if (found != 0)
            l = *found;
        else
            l = 0;
    } else
        l = 0;
    return l;
}

// FUNC 0x001C25E0 gt2k_menu::PageLayout::getHiddenLayer
MenuLayer *PageLayout::getHiddenLayer()
{
    return getLayer("hidden");
}

/* Plain item `name' of the current layer. */
// FUNC 0x001C2600 gt2k_menu::PageLayout::findLayerItem
MenuLayoutItem *PageLayout::findLayerItem(const char *name)
{
    if (layer == 0)
        return 0;
    return findItemIn(name, layer);
}

// FUNC 0x001C2630 gt2k_menu::PageLayout::findItemIn
MenuLayoutItem *PageLayout::findItemIn(const char *name, MenuLayer *l)
{
    MenuLayoutItem **found = find_layer_item(l->items, l->itemCount, name);
    if (found != 0)
        return *found;
    return 0;
}

// FUNC 0x001C2668 gt2k_menu::PageLayout::findObjectIn
MenuLayoutItem *PageLayout::findObjectIn(const char *name, MenuLayer *l)
{
    MenuLayoutItem **found = find_item(l->objects, l->objectCount, name);
    if (found != 0)
        return *found;
    return 0;
}

/* Object item `name' of layer `layerName' (NULL: the current layer). */
// FUNC 0x001C26A0 gt2k_menu::PageLayout::findObject
MenuLayoutItem *PageLayout::findObject(const char *name, const char *layerName)
{
    if (page == 0)
        return 0;
    MenuLayer *l = (layerName != 0) ? getLayer(layerName) : layer;
    if (l == 0)
        return 0;
    return findObjectIn(name, l);
}

/* Item `name' of the page itself. */
// FUNC 0x001C26F8 gt2k_menu::PageLayout::findPageItem
MenuLayoutItem *PageLayout::findPageItem(const char *name)
{
    MenuPage *p = page;
    if (p != 0) {
        MenuLayoutItem **found = find_item(p->items, p->itemCount, name);
        if (found != 0)
            return *found;
    }
    return 0;
}

// FUNC 0x001C2738 gt2k_menu::PageLayout::firstPageItem
MenuLayoutItem *PageLayout::firstPageItem()
{
    MenuPage *p = page;
    if (p == 0)
        return 0;
    if (p->itemCount > 0)
        return p->items[0];
    return 0;
}

/* Plain item `name' of the layer "hidden" (the pictures the code places
 * itself: "pinch", "cursor", ...). */
// FUNC 0x001C2768 gt2k_menu::PageLayout::findHiddenItem
MenuLayoutItem *PageLayout::findHiddenItem(const char *name)
{
    MenuLayer *hidden = getHiddenLayer();
    if (hidden != 0)
        return findItemIn(name, hidden);
    return 0;
}

// FUNC 0x001C27A8 gt2k_menu::PageLayout::getTexture
MenuTexture *PageLayout::getTexture(MenuLayoutItem *item)
{
    if (item->flags & ITEM_HAS_TEXTURE)
        return &data->textures[item->uv->texture];
    return 0;
}

// FUNC 0x001C27E0 gt2k_menu::PageLayout::getText
MenuText *PageLayout::getText(MenuLayoutItem *item)
{
    if (item->flags & ITEM_HAS_TEXT)
        return item->text;
    return 0;
}

// FUNC 0x001C2800 gt2k_menu::PageLayout::findHook
const MenuHook *PageLayout::findHook(const char *name)
{
    if (hooks == 0)
        return 0;
    return find_hook(hooks, hookCount, name);
}

// FUNC 0x001C2838 gt2k_menu::PageLayout::findSliderFunc
const MenuSliderFunc *PageLayout::findSliderFunc(const char *name)
{
    if (sliderFuncs == 0)
        return 0;
    return find_slider_func(sliderFuncs, sliderCount, name);
}

// FUNC 0x001C2870 gt2k_menu::PageLayout::findMultiFunc
const MenuMultiFunc *PageLayout::findMultiFunc(const char *name)
{
    if (multiFuncs == 0)
        return 0;
    return find_multi_func(multiFuncs, multiCount, name);
}

/* Builds the current layer under parent: its items, then its objects.
 * PageManager::startPage (0x001B7268). */
// FUNC 0x001C28A8 gt2k_menu::PageLayout::build
int PageLayout::build(Object *parent)
{
    MenuLayer *l = layer;
    if (l == 0)
        return 0;
    for (int i = 0; i < l->itemCount; i++) {
        Item *item = createItem(l->items[i], parent, 0);
        if (item != 0)
            item->attachTo(parent);
    }
    for (int i = 0; i < l->objectCount; i++) {
        Transitive *object = createObject(l->objects[i], parent, 0);
        if (object != 0)
            object->attachTo(parent);
    }
    return 1;
}

/* Builds layer `name' of the page in a new Transitive (the objects placed
 * relative to it); `front': each one is put among the children by its z
 * (attachSorted) instead of after them. */
// FUNC 0x001C2998 gt2k_menu::PageLayout::buildLayer
Transitive *PageLayout::buildLayer(const char *name, int front)
{
    if (page == 0)
        return 0;
    MenuLayer *l = getLayer(name);
    if (l == 0)
        return 0;
    Transitive *group = new Transitive(); /* implicit: operator new, operator delete (new-expression) */
    int i;
    for (i = 0; i < l->itemCount; i++) {
        Item *item = createItem(l->items[i], group, group);
        if (item != 0) {
            if (front != 0)
                item->attachSorted(group);
            else
                item->attachTo(group);
        }
    }
    for (i = 0; i < l->objectCount; i++) {
        Transitive *object = createObject(l->objects[i], group, group);
        if (object != 0) {
            if (front != 0)
                object->attachSorted(group);
            else
                object->attachTo(group);
        }
    }
    return group;
}

/* Places t off screen on the side of inputMode, then slides it in.
 * ButtonList::showHelpLayer2 (0x001C9030). */
// FUNC 0x001C2B18 gt2k_menu::PageLayout::openTransition
void PageLayout::openTransition(Transitive *t)
{
    /* 0x00335720: opSpringTo (16) the centre to (0, 0) (600, 30, 0.5), then
     * opRemove (3): the operator ends, the layer stays.  A function-local
     * static: egcs writes it into .rodata when it compiles this function,
     * in front of the switch's jump table (0x00335780), where retail has it. */
    static const OperatorCode page_slide_in[] = {
        { 16, { 0 }, 0, { 0 }, 0.0f, 0.0f, 600.0f, 30.0f, 0.5f, { 0 }, 1 },
        {  3, { 0 }, 0, { 0 }, 0.0f, 0.0f,   0.0f,  0.0f, 0.0f, { 0 }, 1 },
    };
    switch (inputMode) {
    case PAGE_SLIDE_NONE:
        t->x.set(0.0f);
        t->y.set(0.0f);
        break;
    case PAGE_SLIDE_FROM_TOP:
        t->x.set(0.0f);
        t->y.set(-MENU_SCREEN_H);
        break;
    case PAGE_SLIDE_FROM_BOTTOM:
        t->x.set(0.0f);
        t->y.set(MENU_SCREEN_H);
        break;
    case PAGE_SLIDE_FROM_LEFT:
        t->x.set(-MENU_SCREEN_W);
        t->y.set(0.0f);
        break;
    case PAGE_SLIDE_FROM_RIGHT:
        t->x.set(MENU_SCREEN_W);
        t->y.set(0.0f);
        break;
    }
    t->start(page_slide_in);
}

/* inputMode 0: releases t at once; else slides it out to the other side and
 * closes the subtree (disappearAll(NULL): every object's default close). */
// FUNC 0x001C2C08 gt2k_menu::PageLayout::closeTransition
void PageLayout::closeTransition(Transitive *t)
{
    /* opSpringTo (16) the centre off screen (600, 30, 0.5), then opRelease
     * (6): the layer is released.  Function-local statics, in front of the
     * switch's jump table (0x00335920) as in retail. */
    static const OperatorCode page_slide_out_down[] = {     /* 0x00335798 mode 1: to (0, 448) */
        { 16, { 0 }, 0, { 0 }, 0.0f, MENU_SCREEN_H, 600.0f, 30.0f, 0.5f, { 0 }, 1 },
        {  6, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 1 },
    };
    static const OperatorCode page_slide_out_up[] = {       /* 0x003357F8 mode 2: to (0, -448) */
        { 16, { 0 }, 0, { 0 }, 0.0f, -MENU_SCREEN_H, 600.0f, 30.0f, 0.5f, { 0 }, 1 },
        {  6, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 1 },
    };
    static const OperatorCode page_slide_out_right[] = {    /* 0x00335858 mode 3: to (640, 0) */
        { 16, { 0 }, 0, { 0 }, MENU_SCREEN_W, 0.0f, 600.0f, 30.0f, 0.5f, { 0 }, 1 },
        {  6, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 1 },
    };
    static const OperatorCode page_slide_out_left[] = {     /* 0x003358B8 mode 4: to (-640, 0) */
        { 16, { 0 }, 0, { 0 }, -MENU_SCREEN_W, 0.0f, 600.0f, 30.0f, 0.5f, { 0 }, 1 },
        {  6, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 1 },
    };
    switch (inputMode) {
    case PAGE_SLIDE_NONE:
        t->kill();
        break;
    case PAGE_SLIDE_FROM_TOP:
        t->start(page_slide_out_down);
        t->disappearAll(0);
        break;
    case PAGE_SLIDE_FROM_BOTTOM:
        t->start(page_slide_out_up);
        t->disappearAll(0);
        break;
    case PAGE_SLIDE_FROM_LEFT:
        t->start(page_slide_out_right);
        t->disappearAll(0);
        break;
    case PAGE_SLIDE_FROM_RIGHT:
        t->start(page_slide_out_left);
        t->disappearAll(0);
        break;
    }
}

/* A plain item: an Item (the menu heap, 0x100 bytes); parent is unused. */
// FUNC 0x001C2CC8 gt2k_menu::PageLayout::createItem
Item *PageLayout::createItem(MenuLayoutItem *item, Object *parent, Drawable *anchor)
{
    MenuTexture *tex = getTexture(item);
    Item *object = new Item(item, tex);  /* implicit: Object::operator delete (if the constructor throws) */
    object->z = object->z + (f32)level * 100.0f;
    if (anchor != 0) {
        object->anchor = anchor;
        object->anchorType = ANCHOR_XY;
    }
    return object;
}

/* An object item: built by the hook of its type (ctx = the hook context),
 * an Item if no hook has that name; NULL for an empty type or when the
 * hook makes nothing. */
// FUNC 0x001C2D98 gt2k_menu::PageLayout::createObject
Transitive *PageLayout::createObject(MenuLayoutItem *item, Object *parent, Drawable *anchor)
{
    const char *type = item->type->name;
    if (type[0] == '\0')
        return 0;
    Transitive *object;
    const MenuHook *hook = findHook(type);
    if (hook != 0) {
        object = hook->create(hookCtx, this, parent, item, hook->p8, hook->pC, hook->p10, hook->p14);
        if (object == 0)
            return 0;
    } else {
        MenuTexture *tex = getTexture(item);
        object = new Item(item, tex);  /* implicit: Object::operator delete (if the constructor throws) */
    }
    object->z = object->z + (f32)level * 100.0f;
    if (anchor != 0) {
        object->anchor = anchor;
        object->anchorType = ANCHOR_XY;
    }
    return object;
}

/* Item `name' anywhere in the page: its own items, then per layer the
 * plain items and the object items (all with the same search). */
// FUNC 0x001C2ED0 gt2k_menu::PageLayout::findAnyItem
MenuLayoutItem *PageLayout::findAnyItem(const char *name)
{
    MenuPage *p = page;
    if (p == 0)
        return 0;
    MenuLayoutItem **found = find_item(p->items, p->itemCount, name);
    if (found == 0) {
        for (int i = 0; i < p->layerCount; i++) {
            found = find_item(p->layers[i]->items, p->layers[i]->itemCount, name);
            if (found != 0)
                break;
            found = find_item(p->layers[i]->objects, p->layers[i]->objectCount, name);
            if (found != 0)
                break;
        }
    }
    return found ? *found : 0;
}

/* The seven binary searches by name below are the same code (instances of
 * one search for each table type; ? a template).  The characters compare
 * signed (lb); a hit is a full match up to the terminator; a miss goes left
 * when the name's first differing character is the smaller one.  Written as
 * retail's code shows them: tail-recursive (egcs turns the self-calls into
 * jumps), each half's count in its own variable (the name-search pattern of
 * ref/MATCHING.md, ref/matching/u064/notes.md). */

// FUNC 0x001C2FA8 gt2k_menu::find_page
MenuPage **find_page(MenuPage **table, int count, const char *key)
{
    if (count <= 0)
        return 0;
    MenuPage **entry = &table[count / 2];
    const char *name = (*entry)->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return find_page(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return find_page(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

// FUNC 0x001C3040 gt2k_menu::find_layer
MenuLayer **find_layer(MenuLayer **table, int count, const char *key)
{
    if (count <= 0)
        return 0;
    MenuLayer **entry = &table[count / 2];
    const char *name = (*entry)->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return find_layer(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return find_layer(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

/* Used for the plain items of a layer (findItem). */
// FUNC 0x001C30D8 gt2k_menu::find_layer_item
MenuLayoutItem **find_layer_item(MenuLayoutItem **table, int count, const char *key)
{
    if (count <= 0)
        return 0;
    MenuLayoutItem **entry = &table[count / 2];
    const char *name = (*entry)->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return find_layer_item(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return find_layer_item(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

/* Used for the page's items, the layers' objects and findAnyItem. */
// FUNC 0x001C3170 gt2k_menu::find_item
MenuLayoutItem **find_item(MenuLayoutItem **table, int count, const char *key)
{
    if (count <= 0)
        return 0;
    MenuLayoutItem **entry = &table[count / 2];
    const char *name = (*entry)->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return find_item(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return find_item(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

// FUNC 0x001C3208 gt2k_menu::find_hook
const MenuHook *find_hook(const MenuHook *table, int count, const char *key)
{
    if (count <= 0)
        return 0;
    const MenuHook *entry = &table[count / 2];
    const char *name = entry->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return find_hook(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return find_hook(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

// FUNC 0x001C32A8 gt2k_menu::find_slider_func
const MenuSliderFunc *find_slider_func(const MenuSliderFunc *table, int count, const char *key)
{
    if (count <= 0)
        return 0;
    const MenuSliderFunc *entry = &table[count / 2];
    const char *name = entry->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return find_slider_func(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return find_slider_func(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

// FUNC 0x001C3340 gt2k_menu::find_multi_func
const MenuMultiFunc *find_multi_func(const MenuMultiFunc *table, int count, const char *key)
{
    if (count <= 0)
        return 0;
    const MenuMultiFunc *entry = &table[count / 2];
    const char *name = entry->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return find_multi_func(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return find_multi_func(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

} /* namespace gt2k_menu */
