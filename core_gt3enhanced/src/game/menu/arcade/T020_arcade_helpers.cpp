/* T020 - helpers shared by the arcade pages.  0x00104F30..0x001060B8.
 *
 * Mostly member functions of gt2k_menu::ArcadeControl (T010), called from
 * the page units T018, T021..T025 and T031: the page fade in / out (scripts
 * on the full-screen rectangle `fade'), the left / right arrows of a
 * SmartIndexT, placing items relative to a cross bar, the three history
 * banners of "ar/top"'s choices sliding in and out on the render root, the
 * mini picture cursor and text labels built from the hidden items of the
 * page layout.  The operator scripts are T020's data (arcade_select_internal.h).
 *
 * Every `new' of a menu object below is Object::operator new (the menu
 * heap); if the constructor throws, Object::operator delete frees it.
 *
 * Data: .rodata 0x00309978..0x00309EEE = the operator scripts (no string
 * literals), then the type names SmartIndexT<int>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>, Operator: the
 * menu headers, then T010 (T251 instantiates SmartIndexT<int> where it is
 * read), no GameStatus (its garage names are not in the original's).
 * .sdata 0x00350C18..0x00350C39: g_00350C18, then the strings. */
#include "types.h"
#include <stddef.h>
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, MenuTexture */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive::start */
#include "game/menu/framework/T024_mini_cursor.h"          /* gt2k_menu::MiniCursor */
#include "game/menu/framework/T255_cross.h"                /* gt2k_menu::Cross */
#include "game/menu/framework/T258_menu_string.h"          /* gt2k_menu::String */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable, ANCHOR_* */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "game/menu/framework/T218_color.h"                /* Color4f */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuText */
#include "game/data/T435_product_config.h"       /* product_config, ProductConfig::courseLevel */
#include "game/menu/arcade/T010_arcade_control.h"       /* gt2k_menu::ArcadeControl; SmartIndexT<int> (T251) */
#include "game/menu/arcade/T020_arcade_helpers.h"
#include "game/menu/arcade/arcade_select_internal.h"

/* 1 while the page-out script runs (transitionOut waits for
 * transitionOutDone).  Initialised: .sdata at its definition, in front of
 * the strings (an uninitialised C++ global would come after them). */
int g_00350C18 = 0;                         /* 0x00350C18 gp-0x6DD8 */

/* One instruction of the scripts below: code (T217's Operator::op* index),
 * arg, the operands f10..f20, result (1 ends the frame); arg2 and the
 * unknown bytes are 0.  An opCall function goes in as (int)fn: the union's
 * first member is the int (as u004's U004_OP). */
#define U002_OP(code, arg, f10, f14, f18, f1C, f20, result) \
    { (code), (arg), 0, { 0, 0, 0, 0 }, (f10), (f14), (f18), (f1C), (f20), \
      { 0, 0, 0, 0, 0, 0, 0, 0 }, (result) }

/* Page in, on the full-screen fade rectangle: opaque black, alpha -4 / s
 * for 0.25 s, then parked at (-10, -10) with size 0; opRemove. */
const gt2k_menu::OperatorCode g_00309978[4] = {     /* 0x00309978 */
    U002_OP(18 /* opSetColors */,  0,   0.0f,   0.0f, 0.0f,  1.0f, 0.0f,  0),
    U002_OP(20 /* opFadeColors */, 0,   0.0f,   0.0f, 0.0f, -4.0f, 0.25f, 1),
    U002_OP( 7 /* opSetRect */,    0, -10.0f, -10.0f, 0.0f,  0.0f, 0.0f,  0),
    U002_OP( 3 /* opRemove */,     0,   0.0f,   0.0f, 0.0f,  0.0f, 0.0f,  1)
};

namespace gt2k_menu {

/* The page-in script g_00309978 on the fade rectangle (from black). */
// FUNC 0x00104F30 gt2k_menu::ArcadeControl::transitionIn
void ArcadeControl::transitionIn()
{
    fade->start(g_00309978);
}

} /* namespace gt2k_menu */

/* The opCall at the end of the page-out script (stored in it at
 * 0x00309ACC). */
// FUNC 0x00104F58 transitionOutDone
void transitionOutDone(void)
{
    g_00350C18 = 0;
}

/* Page out: the full screen (centre 320, 224, 640 x 448), transparent
 * black, alpha +4 / s for 0.25 s, opCall transitionOutDone; opRemove. */
const gt2k_menu::OperatorCode g_00309A38[5] = {     /* 0x00309A38 */
    U002_OP( 7 /* opSetRect */,    0,                      320.0f, 224.0f, 640.0f, 448.0f, 0.0f,  0),
    U002_OP(18 /* opSetColors */,  0,                        0.0f,   0.0f,   0.0f,   0.0f, 0.0f,  0),
    U002_OP(20 /* opFadeColors */, 0,                        0.0f,   0.0f,   0.0f,   4.0f, 0.25f, 1),
    U002_OP(30 /* opCall */,       (int)transitionOutDone,   0.0f,   0.0f,   0.0f,   0.0f, 0.0f,  0),
    U002_OP( 3 /* opRemove */,     0,                        0.0f,   0.0f,   0.0f,   0.0f, 0.0f,  1)
};

namespace gt2k_menu {

/* The page-out script g_00309A38 on the fade rectangle (to black), then
 * frames until its end callback (transitionOutDone) clears the flag. */
// FUNC 0x00104F60 gt2k_menu::ArcadeControl::transitionOut
void ArcadeControl::transitionOut()
{
    g_00350C18 = 1;
    fade->start(g_00309A38);
    while (g_00350C18 != 0)
        waitFrame();
}

/* "arrow_l" / "arrow_r" under parent, shown or hidden for idx. */
// FUNC 0x00104FC8 gt2k_menu::ArcadeControl::createArrows
void ArcadeControl::createArrows(SmartIndexT<int> *idx, PageLayout *layout, Object *parent)
{
    arrowL = new Item("arrow_l", layout);  /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    arrowL->attachTo(parent);
    arrowR = new Item("arrow_r", layout);
    arrowR->attachTo(parent);
    updateArrows(idx);
}

/* A clamped index hides the left arrow at its first value and the right
 * arrow at its last one; a wrapping index shows both.  idx->clamp is read
 * twice: GCSE keeps the first load across the arrowL->visible store (int,
 * clamp a bool: T251), the `daddu $t1, $a3' copy. */
// FUNC 0x001050C8 gt2k_menu::ArcadeControl::updateArrows
void ArcadeControl::updateArrows(SmartIndexT<int> *idx)
{
    int atFirst = idx->clamp == 1 && idx->value <= idx->begin;
    if (atFirst)
        arrowL->visible = 0;
    else
        arrowL->visible = 1;

    int atLast = idx->clamp == 1 && idx->value + 1 >= idx->end;
    if (atLast)
        arrowR->visible = 0;
    else
        arrowR->visible = 1;
}

// FUNC 0x00105140 gt2k_menu::ArcadeControl::clearArrows
void ArcadeControl::clearArrows()
{
    arrowL = NULL;
    arrowR = NULL;
}

} /* namespace gt2k_menu */

/* arcade_fade_out's: alpha -4 / s for 0.25 s, alpha 0, opRelease (kill()). */
const gt2k_menu::OperatorCode g_00309B28[3] = {     /* 0x00309B28 */
    U002_OP(25 /* opFadeAlpha */, 0, -4.0f, 0.25f, 0.0f, 0.0f, 0.0f, 1),
    U002_OP(24 /* opSetAlpha */,  0,  0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0),
    U002_OP( 6 /* opRelease */,   0,  0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1)
};

/* Starts the script g_00309B28 on t: it fades out in 0.25 s and kills
 * itself.  The pages call it on an item they forget or replace. */
// FUNC 0x00105150 arcade_fade_out
void arcade_fade_out(gt2k_menu::Transitive *t)
{
    t->start(g_00309B28);
}

namespace gt2k_menu {

/* A MotionValue's value moved by v (? an inline member of the original,
 * MotionValue::add): retail adds through the inline's `this' (&d->x in a
 * register, then +8), the negated target an argument of its own (neg.s,
 * add.s).  File-local, not added to T260's header: a new inline member
 * there renumbers egcs's labels in every includer (see u004's T035,
 * motionSetValue). */
static inline void motionAdd(MotionValue *mv, f32 v)
{
    mv->value += v;
}

/* Makes d relative to the bar (in x and y): its x / y lose the
 * bar's targets (the bar's position when the page was built).  `this' is
 * not used. */
// FUNC 0x00105170 gt2k_menu::ArcadeControl::attachToBar
void ArcadeControl::attachToBar(Drawable *d, Cross *bar)
{
    d->anchorType = ANCHOR_X | ANCHOR_Y;
    d->anchor = bar;
    motionAdd(&d->x, -bar->targetX);
    motionAdd(&d->y, -bar->targetY);
}

} /* namespace gt2k_menu */

/* The y offset of screen row n (488 per row, the Cross's vertical wrap):
 * (g_00352704 + 448) * n, g_00352704 being a variable (40.0, loaded through
 * $gp, not a literal). */
// FUNC 0x001051B0 F_001051B0
f32 F_001051B0(int n)
{
    return (g_00352704 + 448.0f) * (f32)n;
}

/* The history banners: opMove at -2560 / -2304 / -2048 pixels per second
 * for 0.25 s (640 / 576 / 512 pixels to the left), opRemove; out: +2560 / s
 * for 0.25 s (640 to the right), opRelease (kill()). */
const gt2k_menu::OperatorCode g_00309BB8[2] = {     /* 0x00309BB8 showHistoryLevel */
    U002_OP(9 /* opMove */,    0, -2560.0f, 0.0f, 0.25f, 0.0f, 0.0f, 1),
    U002_OP(3 /* opRemove */,  0,     0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1)
};
const gt2k_menu::OperatorCode g_00309C18[2] = {     /* 0x00309C18 hideHistoryLevel */
    U002_OP(9 /* opMove */,    0,  2560.0f, 0.0f, 0.25f, 0.0f, 0.0f, 1),
    U002_OP(6 /* opRelease */, 0,     0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1)
};
const gt2k_menu::OperatorCode g_00309C78[2] = {     /* 0x00309C78 showHistoryArea */
    U002_OP(9 /* opMove */,    0, -2304.0f, 0.0f, 0.25f, 0.0f, 0.0f, 1),
    U002_OP(3 /* opRemove */,  0,     0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1)
};
const gt2k_menu::OperatorCode g_00309CD8[2] = {     /* 0x00309CD8 hideHistoryArea */
    U002_OP(9 /* opMove */,    0,  2560.0f, 0.0f, 0.25f, 0.0f, 0.0f, 1),
    U002_OP(6 /* opRelease */, 0,     0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1)
};
const gt2k_menu::OperatorCode g_00309D38[2] = {     /* 0x00309D38 showHistoryClass */
    U002_OP(9 /* opMove */,    0, -2048.0f, 0.0f, 0.25f, 0.0f, 0.0f, 1),
    U002_OP(3 /* opRemove */,  0,     0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1)
};
const gt2k_menu::OperatorCode g_00309D98[2] = {     /* 0x00309D98 hideHistoryClass */
    U002_OP(9 /* opMove */,    0,  2560.0f, 0.0f, 0.25f, 0.0f, 0.0f, 1),
    U002_OP(6 /* opRelease */, 0,     0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1)
};

namespace gt2k_menu {

/* The banner of the level: historyLevel[level] (a local copy of the table
 * is indexed, no bound check) as a new Item on the render root, slid in by
 * g_00309BB8, fully opaque. */
// FUNC 0x001051D0 gt2k_menu::ArcadeControl::showHistoryLevel
void ArcadeControl::showHistoryLevel()
{
    MenuLayoutItem *table[4] = { historyLevel[0], historyLevel[1], historyLevel[2],
                                 historyLevel[3] };
    MenuLayoutItem *res = table[level];
    MenuTexture *tex = NULL;
    if (res != NULL)
        tex = pages.current->getTexture(res);
    historyLevelItem = new Item(res, tex);  /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    historyLevelItem->start(g_00309BB8);
    historyLevelItem->attachTo(&render.root);
    historyLevelItem->alphaRatio = 1.0f;
}

/* Slides the level banner out (g_00309C18 kills it) and forgets it. */
// FUNC 0x001052E8 gt2k_menu::ArcadeControl::hideHistoryLevel
void ArcadeControl::hideHistoryLevel()
{
    historyLevelItem->start(g_00309C18);
    historyLevelItem = NULL;
}

/* The banner of the course's area: ProductConfig::courseLevel (the arcade
 * level the course needs, for the mode's course list) indexes
 * {historyArea[0..5], NULL} (no bound check past 6); slid in by
 * g_00309C78, fully opaque. */
// FUNC 0x00105320 gt2k_menu::ArcadeControl::showHistoryArea
void ArcadeControl::showHistoryArea()
{
    MenuLayoutItem *table[7] = { historyArea[0], historyArea[1], historyArea[2],
                                 historyArea[3], historyArea[4], historyArea[5], NULL };
    MenuTexture *tex = NULL;
    ProductConfig *config = product_config();
    int courseIndex = course.value;
    int area = config->courseLevel(courseIndex, getModeName());
    MenuLayoutItem *res = table[area];
    if (res != NULL)
        tex = pages.current->getTexture(res);
    historyAreaItem = new Item(res, tex);   /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    historyAreaItem->start(g_00309C78);
    historyAreaItem->attachTo(&render.root);
    historyAreaItem->alphaRatio = 1.0f;
}

/* Slides the area banner out (g_00309CD8 kills it) and forgets it. */
// FUNC 0x00105488 gt2k_menu::ArcadeControl::hideHistoryArea
void ArcadeControl::hideHistoryArea()
{
    historyAreaItem->start(g_00309CD8);
    historyAreaItem = NULL;
}

/* The banner of player 1's class: historyClass[class1.value]; slid in by
 * g_00309D38 (its alphaRatio is left as the Item set it). */
// FUNC 0x001054C0 gt2k_menu::ArcadeControl::showHistoryClass
void ArcadeControl::showHistoryClass()
{
    MenuLayoutItem *table[7] = { historyClass[0], historyClass[1], historyClass[2],
                                 historyClass[3], historyClass[4], historyClass[5],
                                 historyClass[6] };
    MenuLayoutItem *res = table[class1.value];
    MenuTexture *tex = NULL;
    if (res != NULL)
        tex = pages.current->getTexture(res);
    historyClassItem = new Item(res, tex);  /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    historyClassItem->start(g_00309D38);
    historyClassItem->attachTo(&render.root);
}

/* Slides the class banner out (g_00309D98 kills it) and forgets it. */
// FUNC 0x001055F8 gt2k_menu::ArcadeControl::hideHistoryClass
void ArcadeControl::hideHistoryClass()
{
    historyClassItem->start(g_00309D98);
    historyClassItem = NULL;
}

/* A new MiniCursor (0x138 bytes) on the hidden item
 * `name' over idx, with one icon per position 0 .. end - begin - 1: the
 * hidden item whose name nameOf writes (missing ones are skipped).  NULL if
 * `name' is missing.  The member pointer is called the egcs way: index > 0
 * a virtual (vtable entry index - 1, the vptr at delta2), else pfn; `this'
 * adjusted by delta (and the entry's delta). */
// FUNC 0x00105630 gt2k_menu::ArcadeControl::createMiniCursor
MiniCursor *ArcadeControl::createMiniCursor(const char *name, SmartIndexT<int> *idx, NameFn nameOf,
                                            int arg)
{
    char iconName[0x40];
    PageLayout *layout = pages.current;
    MenuLayoutItem *res = layout->findHiddenItem(name);
    if (res == NULL)
        return NULL;

    MiniCursor *cursor = new MiniCursor(res, idx);  /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    int count = idx->end - idx->begin;
    for (int i = 0; i < count; i++) {
        (this->*nameOf)(iconName, i, arg);
        MenuLayoutItem *icon = layout->findHiddenItem(iconName);
        if (icon != NULL)
            cursor->setItem(i, icon, layout);
    }
    return cursor;
}

/* A new String (0x114 bytes) of the hidden item
 * `name': its text and font ("sw_16" when it has none; "" when the item has
 * no text), centred on the item's rectangle (y flipped: 448 - centre), size
 * `size', the item's text colour on all four corners (grey 0x80808080
 * without a text record) and the item's z.  A missing item gives an empty,
 * unplaced String.  `this' is not used.  The colours are Drawable's inline
 * setColor(u32) (four Color4f temporaries); for the grey one egcs folded r,
 * g, b to 128 / 255 and still converts a.  The missing item is the else
 * branch (retail: the code at the end, "sw_16" before "" in .sdata). */
// FUNC 0x001057B8 gt2k_menu::ArcadeControl::createLabel
String *ArcadeControl::createLabel(const char *name, PageLayout *layout, f32 size)
{
    MenuLayoutItem *res = layout->findHiddenItem(name);
    String *label;

    if (res != NULL) {
        if ((res->flags & ITEM_HAS_TEXT) != 0 && res->text != NULL && res->text->text != NULL) {
            if (res->text->font != NULL)
                label = new String(res->text->text, res->text->font);   /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
            else
                label = new String(res->text->text, "sw_16");
        } else {
            label = new String("", "sw_16");
        }

        f32 cx = res->x + res->w * 0.5f;
        f32 cy = res->y + res->h * 0.5f;
        label->x.set(cx);
        label->y.set(448.0f - cy);
        label->w = size;
        label->h = size;

        MenuText *text = res->text;
        if (text != NULL)
            label->setColor(text->color);
        else
            label->setColor(0x80808080u);
        label->z = res->z;
    } else {
        label = new String("", "sw_16");
    }
    return label;
}

} /* namespace gt2k_menu */
