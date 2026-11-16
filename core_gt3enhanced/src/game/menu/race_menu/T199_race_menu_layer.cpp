/* T199 - RaceMenuLayer, the RaceMenuWindow methods and the
 * RaceMenuSelectWindow cursor.  0x001B0678..0x001B0F28.
 *
 * A layer owns one object built from a layer of the page (`root'), hung
 * under its RootObject `obj'; the images of the layer are Items attached
 * under root.  A window adds a frame image whose size / position stand for
 * the window's. */
/* The include order is the original's data order: <new> first (the
 * "bad_alloc" of its inline what() opens the .rodata, 0x00331A08), then
 * the race displays before the menu classes.  egcs writes the type names
 * in the reverse order of the classes' definitions; retail's: the windows,
 * RaceMenuLayer, RaceMenuButton, the operator list classes, RaceMiniMap ..
 * RaceMeterBase, RaceCameraBase, RaceValueDisplayBase, RaceDisplayBase,
 * UnitArena<64>, then Operator (a base of OperatorHolder, always last).
 * So UnitArena<64> is instantiated first (T153 read before T154, which
 * reads it after the display parts), then the displays (T154; nothing of
 * them is used here).  T157's inline RaceMessageDisplay::clear puts its ""
 * into the .sdata, as in retail (0x003523B0). */
// DATA_START 0x00331A08
// DATA_START 0x003523B0
#include <new>                              /* retail's .rodata starts with its "bad_alloc" */
#include "game/menu/race_menu/race_menu_internal.h"
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/display/T154_race_display.h"         /* (seen: the race displays, RaceCameraBase) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object, RootObject */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "engine/input/T474_control.h"              /* Control */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* race_menu_build_layer / new_item / attach, begin / end_draw */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuButton */
#include "game/menu/race_menu/T199_race_menu_layer.h"

using namespace gt2k_menu;

/* release(), clear(), then obj's RootObject::~RootObject (flags 2); egcs
 * deleting flag: operator delete. */
// FUNC 0x001B0678 RaceMenuLayer::~RaceMenuLayer
RaceMenuLayer::~RaceMenuLayer()
{
    release();
    clear();
    /* egcs: obj's RootObject::~RootObject(2), then operator delete if
     * flags & 1 */
    /* implicit: ~RootObject (member destructor), operator delete (deleting destructor) */
}

/* root = the object of layer `name', appended to obj's children.  Callers:
 * ReplayOption::load (T196), RaceMenuWindow::load. */
// FUNC 0x001B06E0 RaceMenuLayer::load
void RaceMenuLayer::load(PageLayout *layout, const char *name)
{
    root = race_menu_build_layer(layout, name);
    root->attachTo(&obj);
}

/* A new Item of the hidden item `name' under root (NULL without a root). */
// FUNC 0x001B0720 RaceMenuLayer::newItem
Item *RaceMenuLayer::newItem(PageLayout *layout, const char *name, int sorted)
{
    if (root != NULL)
        return race_menu_new_item(layout, root, name, sorted);
    return NULL;
}

/* Attaches item under root (when there is one); returns item. */
// FUNC 0x001B0758 RaceMenuLayer::attach
Transitive *RaceMenuLayer::attach(Transitive *item, int sorted)
{
    if (root != NULL)
        return race_menu_attach(root, item, sorted);
    return item;
}

/* o->virtual_4(NULL) (slot 4, +0x2C: disappear with the default close
 * script) when o is not NULL.  No callers. */
// FUNC 0x001B0780 RaceMenuLayer::disappear
void RaceMenuLayer::disappear(Object *o)
{
    if (o != NULL)
        o->virtual_4(NULL);
}

/* item->visible = on when item is not NULL.  Callers: PauseMenu::showMain /
 * showConfirm (T169), PauseMenuWithRetry::showMain / showConfirm (T171). */
// FUNC 0x001B07B8 RaceMenuLayer::showItem
void RaceMenuLayer::showItem(Transitive *item, int on)
{
    if (item != NULL)
        item->visible = on;
}

/* Draws the tree (no projection set-up): RootObject::drawSorted (z order)
 * when sorted, else RootObject::virtual_2 (list order).  Callers:
 * ReplayOption::draw (T196), ResultBar::render. */
// FUNC 0x001B07C8 RaceMenuLayer::drawTree
void RaceMenuLayer::drawTree(int sorted)
{
    if (root != NULL && visible != 0) {
        if (sorted != 0)
            obj.drawSorted();
        else
            obj.virtual_2();
    }
}

/* drawTree inside a menu draw of buf (race_menu_begin_draw / end).
 * Callers: RacePause::virtual_3, ReplayPause::virtual_3,
 * RacePauseWithRetry::virtual_3. */
// FUNC 0x001B0818 RaceMenuLayer::draw
void RaceMenuLayer::draw(GSBuffer *buf, int sorted)
{
    if (root != NULL) {
        if (visible != 0) {
            race_menu_begin_draw(buf);
            if (sorted != 0)
                obj.drawSorted();
            else
                obj.virtual_2();
            race_menu_end_draw();
        }
    }
}

/* One frame of the tree (RootObject::virtual_1: updates, steps the dying
 * nodes). */
// FUNC 0x001B0890 RaceMenuLayer::update
void RaceMenuLayer::update()
{
    obj.virtual_1();
}

/* Deletes everything under obj (RootObject::deleteChildren), then a frame. */
// FUNC 0x001B08B0 RaceMenuLayer::clear
void RaceMenuLayer::clear()
{
    obj.deleteChildren();
    obj.virtual_1();
}

/* Kills root (root = NULL), then a frame (which starts stepping it to its
 * deletion). */
// FUNC 0x001B08E0 RaceMenuLayer::release
void RaceMenuLayer::release()
{
    if (root != NULL) {
        root->kill();
        root = NULL;
    }
    obj.virtual_1();
}

/* script on root and every node below it (Object::disappearAll:
 * virtual_4(script) on each).  Callers: the result bars. */
// FUNC 0x001B0920 RaceMenuLayer::startScript
void RaceMenuLayer::startScript(const OperatorCode *script)
{
    if (root != NULL)
        root->disappearAll(script);
}

/* The callback of resetScripts: a Transitive of the tree loses its
 * operators (dynamic_cast from Object; a, b, c unused). */
// FUNC 0x001B0948 race_menu_reset_operators
static void race_menu_reset_operators(Object *node, void *a, void *b, void *c)
{
    Transitive *t = dynamic_cast<Transitive *>(node);
    if (t != NULL)
        t->clearOperators();
}

/* Removes the operators of root and of every Transitive under it
 * (Object::forEach). */
// FUNC 0x001B09A8 RaceMenuLayer::resetScripts
void RaceMenuLayer::resetScripts()
{
    if (root != NULL)
        root->forEach(race_menu_reset_operators, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------------ */
/* RaceMenuWindow                                                           */
/* ------------------------------------------------------------------------ */

/* The layer `name', and its frame image `frameName' when not NULL.
 * Callers: the pause menus ("defaultLayer", NULL), the result bars. */
// FUNC 0x001B09E0 RaceMenuWindow::load
void RaceMenuWindow::load(PageLayout *layout, const char *name, const char *frameName)
{
    RaceMenuLayer::load(layout, name);
    if (frameName != NULL)
        loadFrame(layout, frameName);
}

// FUNC 0x001B0A30 RaceMenuWindow::loadFrame
void RaceMenuWindow::loadFrame(PageLayout *layout, const char *name)
{
    frame = newItem(layout, name, 1);
}

// FUNC 0x001B0A60 RaceMenuWindow::setFrame
void RaceMenuWindow::setFrame(Transitive *f)
{
    frame = f;
}

/* The frame's width, -1 without a frame or a root.  No callers. */
// FUNC 0x001B0A68 RaceMenuWindow::frameWidth
f32 RaceMenuWindow::frameWidth()
{
    if (frame == NULL || root == NULL)
        return -1.0f;
    return frame->w;
}

/* The frame's height, -1 without a frame or a root. */
// FUNC 0x001B0A98 RaceMenuWindow::frameHeight
f32 RaceMenuWindow::frameHeight()
{
    if (frame == NULL || root == NULL)
        return -1.0f;
    return frame->h;
}

/* The frame's centre x (its x plus the root's), -1 without them.  No
 * callers. */
// FUNC 0x001B0AC8 RaceMenuWindow::frameX
f32 RaceMenuWindow::frameX()
{
    if (frame == NULL || root == NULL)
        return -1.0f;
    return frame->x.value + root->x.value;
}

/* The frame's centre y (its y plus the root's), -1 without them. */
// FUNC 0x001B0B00 RaceMenuWindow::frameY
f32 RaceMenuWindow::frameY()
{
    if (frame == NULL || root == NULL)
        return -1.0f;
    return frame->y.value + root->y.value;
}

/* Puts root at rest where the frame is at x (MotionValue::set inline: egcs
 * -O2 gives the listing's code, &root->x then force, velocity, value).  No
 * callers. */
// FUNC 0x001B0B38 RaceMenuWindow::setFrameX
void RaceMenuWindow::setFrameX(f32 x)
{
    if (frame != NULL) {
        Transitive *r = root;
        if (r != NULL)
            r->x.set(x - frame->x.value);
    }
}

/* Puts root at rest where the frame is at y. */
// FUNC 0x001B0B70 RaceMenuWindow::setFrameY
void RaceMenuWindow::setFrameY(f32 y)
{
    if (frame != NULL) {
        Transitive *r = root;
        if (r != NULL)
            r->y.set(y - frame->y.value);
    }
}

/* ------------------------------------------------------------------------ */
/* RaceMenuSelectWindow                                                     */
/* ------------------------------------------------------------------------ */

/* One frame of the window's input: decided (pressed CONTROL_DECIDE), else
 * cancelled (CONTROL_CANCEL); up / left and down / right with auto-repeat
 * (30, 3) move the cursor by one (down / right only onto an enabled
 * button; up / left without that check); anyInput = any of them.  Sounds
 * when `sound': decide / cancel; the cursor sound always on a move.
 * Callers: RacePause::virtual_2, ReplayPause::virtual_2,
 * RacePauseWithRetry::virtual_2. */
// FUNC 0x001B0BA8 RaceMenuSelectWindow::input
void RaceMenuSelectWindow::input(Control *pad)
{
    int moved, up, down, left, right, back, forward, oldCursor, cur;

    update();
    decided = pad->getOn(CONTROL_DECIDE);
    int cancel = 0;
    if (decided == 0)
        cancel = pad->getOn(CONTROL_CANCEL);
    cancelled = cancel;
    moved = 0;
    up = pad->getRepeat(CONTROL_UP, RACE_MENU_REPEAT_FIRST, RACE_MENU_REPEAT_INTERVAL);
    down = pad->getRepeat(CONTROL_DOWN, RACE_MENU_REPEAT_FIRST, RACE_MENU_REPEAT_INTERVAL);
    left = pad->getRepeat(CONTROL_LEFT, RACE_MENU_REPEAT_FIRST, RACE_MENU_REPEAT_INTERVAL);
    right = pad->getRepeat(CONTROL_RIGHT, RACE_MENU_REPEAT_FIRST, RACE_MENU_REPEAT_INTERVAL);
    back = (up != 0) | (left != 0);
    forward = (down != 0) | (right != 0);
    anyInput = (decided != 0) | (cancelled != 0) | back | forward;
    oldCursor = cursor;                     /* after anyInput: local-alloc then gives up / back
                                               $s1 and down / forward $s2 as retail */
    cur = oldCursor;
    if (sound != 0) {
        if (decided != 0) {
            menu_play_se(MENU_SE_CONFIRM);
            cur = cursor;
        } else if (cancelled != 0) {
            menu_play_se(MENU_SE_CANCEL);
            cur = cursor;
        }
    }
    if (cur > 0 && back != 0) {
        moved = 1;
        cursor = cur - 1;
    } else if (cur < count - 1 && forward != 0) {
        cursor = cur + 1;
        if (items[cursor]->enabled == 0)
            cursor = cur;
        else
            moved = 1;
    }
    if (moved != 0) {
        items[oldCursor]->setSelected(0);
        items[cursor]->setSelected(1);
        menu_play_se(MENU_SE_CURSOR);
    }
    refreshItems();
}

/* selected = (i == cursor) for every item (refresh() when it changes),
 * then each item's update(). */
// FUNC 0x001B0DD0 RaceMenuSelectWindow::refreshItems
void RaceMenuSelectWindow::refreshItems()
{
    for (int i = 0; i < count; i++) {
        RaceMenuButton *button = items[i];
        int sel = cursor == i;
        if (button->selected != sel) {
            button->selected = sel;
            button->refresh();
        }
        items[i]->update();
    }
}

/* ------------------------------------------------------------------------ */
/* RaceMenuLayer's inline members (no callers), typeinfo                    */
/* ------------------------------------------------------------------------ */

// FUNC 0x001B0E80 RaceMenuLayer::isVisible
/* inline in game/menu/race_menu/T199_race_menu_layer.h; egcs emits this copy */

// FUNC 0x001B0E88 RaceMenuLayer::setVisible
/* inline in game/menu/race_menu/T199_race_menu_layer.h; egcs emits this copy */

// FUNC 0x001B0E90 RaceMenuLayer::getRoot (getRoot__13RaceMenuLayer)
/* inline in game/menu/race_menu/T199_race_menu_layer.h; egcs emits this copy */

// FUNC 0x001B0E98 RaceMenuLayer::getRoot_const (getRoot__C13RaceMenuLayer)
/* inline in game/menu/race_menu/T199_race_menu_layer.h (the const overload, the same
 * code); egcs emits this copy */

// FUNC 0x001B0EA0 RaceMenuLayer::RaceMenuLayer
/* inline in game/menu/race_menu/T199_race_menu_layer.h; egcs emits this copy (root = NULL,
 * vptr, obj's RootObject::RootObject, visible = 1); the derived
 * constructors (T169, T170, T196, T200) expand it */
/* implicit: RootObject (member constructor) */

#ifndef BOOT_OLD_GCC
/* The __user_type_info __ti13RaceMenuLayer (gp-0x4610): no base. */
// FUNC 0x001B0EE8 __tf13RaceMenuLayer
extern "C" const type_info &__tf13RaceMenuLayer(void)
{
    if (__ti13RaceMenuLayer[0] == 0)
        __rtti_user(__ti13RaceMenuLayer, "13RaceMenuLayer");
    return *(const type_info *)__ti13RaceMenuLayer;
}
#endif
