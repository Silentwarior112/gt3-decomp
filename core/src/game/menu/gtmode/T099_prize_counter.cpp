/* T099 - gt2k_menu::PrizeCounter and its menu items.  0x00129140..0x0012A2B0.
 *
 * The prize screens embed a PrizeCounter built with (money, money + prize)
 * - GTmodeTrophyControl (T056) and ConsolationControl (T057's methods) at
 * +0xEF8, TrophyGetControl (T060) at +0xED8 - and register three hooks each
 * under "count", "count_shadow" and "get_money_disp" (the MenuHook tables
 * g_0049BCE8 / g_0049BD60 / g_0049BDF0, filled by T056_static_init,
 * T057_static_init, T060_static_init).  The nine hooks are three copies of
 * one set (? a template on the screen class): only the screen class, the
 * counter's offset in it and the delay before counting differ. */
/* Retail T099's .rodata (0x00313A20..0x00313EF6): "%s Cr. %s" (the items'
 * inline virtual_2), the vtables, the type names ConsolationControl ..
 * TrophyGetControl, RemainItem::virtual_1's jump table, the own names
 * (RemainItem, ControlItem, ShadowItem, PrizeCounter), then
 * PrizeControlBase, ControlBaseWithPageManager, CallBackWhenPrizeRendering,
 * PrizeModelSetContext, the garage's, SmartIndexT<int>, OperatorList ..
 * Cell<OperatorHolder>, Operator.  egcs writes the names in the reverse
 * order of the class definitions (instances where they are instantiated):
 * the menu classes first (T216), SmartIndexT<int> (T211) before the
 * GameStatus (the garage), T460 (PrizeModelSetContext,
 * CallBackWhenPrizeRendering) before T060 / T059 define
 * ControlBaseWithPageManager, the screens' headers last, T060's (T059's
 * PrizeControlBase, T099's own header, TrophyGetControl) before T056's (the
 * MiniPrizeControlBase family). */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the hooks' result) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T211_page_manager.h"         /* (SmartIndexT<int>'s type name) */
#include "game/save/T429_game_status.h"          /* (the garage's type names) */
#include "game/menu/gtmode/T460_prize_base.h"           /* (PrizeModelSetContext, CallBackWhenPrizeRendering) */
#include "game/menu/gtmode/T060_trophy_get.h"           /* gt2k_menu::TrophyGetControl (hook ctx) */
#include "game/menu/gtmode/T056_gtmode_trophy_control.h" /* gt2k_menu::GTmodeTrophyControl, ConsolationControl (hook ctx) */
#include "game/menu/options/option_items_internal.h"

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* CountTick, PrizeCounter                                                  */
/* ------------------------------------------------------------------------ */

/* 1 (and restarts) once amount >= 1 and at least 3 calls were made;
 * otherwise clamps amount to 1 and frames to 3. */
// FUNC 0x00129140 gt2k_menu::CountTick::advance
int CountTick::advance(f32 add)
{
    amount = amount + add;
    frames = frames + 1;
    if (amount >= 1.0f && frames >= 3) {
        amount = 0.0f;
        frames = 0;
        return 1;
    }
    if (amount >= 1.0f)
        amount = 1.0f;
    if (frames >= 3)
        frames = 3;
    return 0;
}

/* Stores the vptr (0x00313B08) first; scale 1, the rest 0. */
// FUNC 0x001291C0 gt2k_menu::PrizeCounter::PrizeCounter
PrizeCounter::PrizeCounter(s64 cur, s64 tgt)
{
    target = tgt;
    current = cur;
    scale = 1.0f;
    frame = 0;
    fraction = 0.0f;
    speed = 0.0f;
    accel = 0.0f;
    tick.amount = 0.0f;
    tick.frames = 0;
    state = 0;
}

/* Odometer digit n (0 = units) of current, as a float: 0 for a leading
 * zero; when the units digit is 9 the fraction is added so the digit rolls
 * (only the units digit is tested).  No callers in the binary. */
// FUNC 0x00129208 gt2k_menu::PrizeCounter::digit
f32 PrizeCounter::digit(int n)
{
    int power = 1;
    s64 value = current;
    for (; n > 0; n--)
        power *= 10;                        /* 32-bit multiply */
    /* implicit: __divdi3, __moddi3, __floatdisf (s64 division / modulo / conversion) */
    value /= power;                         /* __divdi3 */
    if (value == 0)
        return 0.0f;
    f32 d = (f32)(value % 10);              /* __moddi3, then __floatdisf (0x00278800) */
    /* current read again: the libcalls are const, so PRE keeps the first
     * load in a copy ($s0) instead of reloading */
    if (current % 10 == 9)
        return d + fraction;
    return d;
}

/* One frame of counting (state 0 idle, 1 start, 2 accelerating, 3 done). */
// FUNC 0x001292C0 gt2k_menu::PrizeCounter::update
void PrizeCounter::update()
{
    f32 accelStep = 0.0f;                   /* this frame's change of accel */
    /* the new speed is built on the stored one: case 1 reloads it after the
     * u32 conversion's branch, the others reuse the value just stored */
    frame++;
    switch (state) {
    case 0:                                 /* idle */
        speed = 0.0f;
        accel = 0.0f;
        frame = 0;
        break;
    case 1:                                 /* start: 0.238 (5 / 21) a frame for 90 frames */
        speed = 0.23809524f;
        if ((f32)frame > 90.0f)             /* u32 -> float */
            state++;
        break;
    case 2:                                 /* accelerate, up to the limits */
        accelStep = 5.2720418f;
        if (accel >= 474.48376f) {
            accel = 474.48376f;
            accelStep = 0.0f;
        }
        if (speed >= 142345.125f) {
            accelStep = 0.0f;
            speed = 142345.125f;
            accel = 0.0f;
        }
        break;
    case 3:                                 /* done */
        speed = 0.0f;
        current = target;
        accel = 0.0f;
        break;
    }
    accel = accel + accelStep;
    speed = (speed + accel) * scale;
    f32 total = fraction + speed;
    int whole = (int)total;                 /* cvt.w.s */
    current = current + whole;
    fraction = total - (f32)whole;
    if (current >= target) {
        current = target;
        state = 3;
    } else if (tick.advance(speed)) {
        menu_play_se(MENU_SE_CURSOR);
    }
}

// FUNC 0x00129478 gt2k_menu::PrizeCounter::setScale
void PrizeCounter::setScale(f32 s)
{
    if (s < 1.0f)
        scale = 1.0f;
    else if (3.0f < s)
        scale = 3.0f;
    else
        scale = s;
}

// FUNC 0x001294B0 gt2k_menu::PrizeCounter::start
void PrizeCounter::start()
{
    state = 1;
}

// FUNC 0x001294C0 gt2k_menu::PrizeCounter::finish
void PrizeCounter::finish()
{
    current = target;
}

// FUNC 0x001294D0 gt2k_menu::PrizeCounter::isDone
int PrizeCounter::isDone()
{
    return state == 3;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* The hooks (no direct callers; the screens' MenuHook tables)              */
/* ------------------------------------------------------------------------ */
/* ctx is the screen; parent and p8 .. p14 are not used.  The item's texture
 * is looked up before Object::operator new (the menu heap); the items'
 * constructors are inline (game/menu/gtmode/T099_prize_counter.h).  The landing pads
 * run ~Item (flags 0) if the constructor body throws, Object::operator
 * delete if Item's constructor does.  The counter's address is taken
 * before operator new (retail computes it in getTexture's delay slot) and
 * the new item is returned through a local (the EH region layout). */
using namespace gt2k_menu;

/* "count" of GTmodeTrophyControl (g_0049BCE8). */
// FUNC 0x001294E0 create_count_T056
Transitive *create_count_T056(void *ctx, PageLayout *layout, Object *parent,
                              MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    GTmodeTrophyControl *screen = (GTmodeTrophyControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x114); inline: Item(item, tex), the members,
     * setText(""), counter->start(); on a throw ~Item(0) / Object::operator
     * delete */
    /* implicit: Item, ~Item, setText, start (inline constructor, landing pad) */
    Transitive *made = new PrizeCounterControlItem(item, tex, counter, screen, 120); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

/* "count" of ConsolationControl (g_0049BD60).  Same code as
 * create_count_T056. */
// FUNC 0x001295E8 create_count_T057
Transitive *create_count_T057(void *ctx, PageLayout *layout, Object *parent,
                              MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    ConsolationControl *screen = (ConsolationControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x114); inline: Item(item, tex), the members,
     * setText(""), counter->start(); on a throw ~Item(0) / Object::operator
     * delete */
    /* implicit: Item, ~Item, setText, start (inline constructor, landing pad) */
    Transitive *made = new PrizeCounterControlItem(item, tex, counter, screen, 120); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

/* "count" of TrophyGetControl (g_0049BDF0): a longer delay. */
// FUNC 0x001296F0 create_count_T060
Transitive *create_count_T060(void *ctx, PageLayout *layout, Object *parent,
                              MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    TrophyGetControl *screen = (TrophyGetControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x114); inline: Item(item, tex), the members,
     * setText(""), counter->start(); on a throw ~Item(0) / Object::operator
     * delete */
    /* implicit: Item, ~Item, setText, start (inline constructor, landing pad) */
    Transitive *made = new PrizeCounterControlItem(item, tex, counter, screen, 360); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

/* "count_shadow" of GTmodeTrophyControl. */
// FUNC 0x001297F8 create_count_shadow_T056
Transitive *create_count_shadow_T056(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    GTmodeTrophyControl *screen = (GTmodeTrophyControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x104); inline: Item(item, tex), counter,
     * setText(""); on a throw ~Item(0) / Object::operator delete */
    /* implicit: Item, ~Item, setText (inline constructor, landing pad) */
    Transitive *made = new PrizeCounterShadowItem(item, tex, counter); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

/* "count_shadow" of ConsolationControl. */
// FUNC 0x001298D0 create_count_shadow_T057
Transitive *create_count_shadow_T057(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    ConsolationControl *screen = (ConsolationControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x104); inline: Item(item, tex), counter,
     * setText(""); on a throw ~Item(0) / Object::operator delete */
    /* implicit: Item, ~Item, setText (inline constructor, landing pad) */
    Transitive *made = new PrizeCounterShadowItem(item, tex, counter); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

/* "count_shadow" of TrophyGetControl. */
// FUNC 0x001299A8 create_count_shadow_T060
Transitive *create_count_shadow_T060(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    TrophyGetControl *screen = (TrophyGetControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x104); inline: Item(item, tex), counter,
     * setText(""); on a throw ~Item(0) / Object::operator delete */
    /* implicit: Item, ~Item, setText (inline constructor, landing pad) */
    Transitive *made = new PrizeCounterShadowItem(item, tex, counter); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

/* "get_money_disp" of TrophyGetControl. */
// FUNC 0x00129A80 create_get_money_disp_T060
Transitive *create_get_money_disp_T060(void *ctx, PageLayout *layout, Object *parent,
                                       MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    TrophyGetControl *screen = (TrophyGetControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x114); inline: Item(item, tex), the members,
     * setText(""); on a throw ~Item(0) / Object::operator delete */
    /* implicit: Item, ~Item, setText (inline constructor, landing pad) */
    Transitive *made = new PrizeCounterRemainItem(item, tex, counter, screen); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

/* "get_money_disp" of GTmodeTrophyControl. */
// FUNC 0x00129B78 create_get_money_disp_T056
Transitive *create_get_money_disp_T056(void *ctx, PageLayout *layout, Object *parent,
                                       MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    GTmodeTrophyControl *screen = (GTmodeTrophyControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x114); inline: Item(item, tex), the members,
     * setText(""); on a throw ~Item(0) / Object::operator delete */
    /* implicit: Item, ~Item, setText (inline constructor, landing pad) */
    Transitive *made = new PrizeCounterRemainItem(item, tex, counter, screen); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

/* "get_money_disp" of ConsolationControl. */
// FUNC 0x00129C70 create_get_money_disp_T057
Transitive *create_get_money_disp_T057(void *ctx, PageLayout *layout, Object *parent,
                                       MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    ConsolationControl *screen = (ConsolationControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    PrizeCounter *counter = &screen->counter;
    /* Object::operator new(0x114); inline: Item(item, tex), the members,
     * setText(""); on a throw ~Item(0) / Object::operator delete */
    /* implicit: Item, setText (inline constructor) */
    Transitive *made = new PrizeCounterRemainItem(item, tex, counter, screen); /* implicit: Object::operator delete (if the constructor throws) */
    return made;
}

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* PrizeCounterRemainItem                                                   */
/* ------------------------------------------------------------------------ */

/* The items' virtual functions are inline in their class bodies
 * (game/menu/gtmode/T099_prize_counter.h): egcs emits these copies at the end of the
 * unit with the vtables (the hooks above build the items), per class v2,
 * v1, __tf, destructor. */

/* draw: "<prize money> Cr. <target - current>", the alpha from fade. */
// FUNC 0x00129D68 gt2k_menu::PrizeCounterRemainItem::virtual_2
/* implicit: format_number, sprintf, setText, virtual_2 (inline in game/menu/gtmode/T099_prize_counter.h) */

/* update: Item's, then fade in over 60 frames, wait for the counter, hold
 * 61 frames, fade out. */
// FUNC 0x00129DE0 gt2k_menu::PrizeCounterRemainItem::virtual_1
/* implicit: virtual_1, isDone (inline in game/menu/gtmode/T099_prize_counter.h) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x00129EB8 __tfQ29gt2k_menu22PrizeCounterRemainItem
extern "C" void *__tfQ29gt2k_menu22PrizeCounterRemainItem(void)
{
    if (__tiQ29gt2k_menu22PrizeCounterRemainItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu22PrizeCounterRemainItem,
                  "Q29gt2k_menu22PrizeCounterRemainItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu22PrizeCounterRemainItem;
}
#endif

// FUNC 0x00129F08 gt2k_menu::PrizeCounterRemainItem::~PrizeCounterRemainItem
/* the implicit destructor (game/menu/gtmode/T099_prize_counter.h), no vptr store:
 * Item::~Item(this, 0), then Object::operator delete if flags & 1 */
/* implicit: operator delete (deleting destructor) */

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* PrizeCounterControlItem                                                  */
/* ------------------------------------------------------------------------ */

/* draw: "Cr. <current>". */
// FUNC 0x00129F50 gt2k_menu::PrizeCounterControlItem::virtual_2
/* implicit: format_number, sprintf, setText, virtual_2 (inline in game/menu/gtmode/T099_prize_counter.h) */

/* update: waits `delay' frames, then runs the counter; pad 1's decide
 * speeds it up (x3), a second one finishes it.  Then Item's. */
// FUNC 0x00129FB8 gt2k_menu::PrizeCounterControlItem::virtual_1
/* implicit: setScale, finish, update, isDone, virtual_1 (inline in game/menu/gtmode/T099_prize_counter.h) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x0012A0A8 __tfQ29gt2k_menu23PrizeCounterControlItem
extern "C" void *__tfQ29gt2k_menu23PrizeCounterControlItem(void)
{
    if (__tiQ29gt2k_menu23PrizeCounterControlItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu23PrizeCounterControlItem,
                  "Q29gt2k_menu23PrizeCounterControlItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu23PrizeCounterControlItem;
}
#endif

// FUNC 0x0012A0F8 gt2k_menu::PrizeCounterControlItem::~PrizeCounterControlItem
/* the implicit destructor (game/menu/gtmode/T099_prize_counter.h), no vptr store:
 * Item::~Item(this, 0), then Object::operator delete if flags & 1 */
/* implicit: operator delete (deleting destructor) */

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* PrizeCounterShadowItem                                                   */
/* ------------------------------------------------------------------------ */

/* draw: the same text as PrizeCounterControlItem's. */
// FUNC 0x0012A140 gt2k_menu::PrizeCounterShadowItem::virtual_2
/* implicit: format_number, sprintf, setText, virtual_2 (inline in game/menu/gtmode/T099_prize_counter.h) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x0012A1A8 __tfQ29gt2k_menu22PrizeCounterShadowItem
extern "C" void *__tfQ29gt2k_menu22PrizeCounterShadowItem(void)
{
    if (__tiQ29gt2k_menu22PrizeCounterShadowItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu22PrizeCounterShadowItem,
                  "Q29gt2k_menu22PrizeCounterShadowItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu22PrizeCounterShadowItem;
}
#endif

// FUNC 0x0012A1F8 gt2k_menu::PrizeCounterShadowItem::~PrizeCounterShadowItem
/* the implicit destructor (game/menu/gtmode/T099_prize_counter.h), no vptr store:
 * Item::~Item(this, 0), then Object::operator delete if flags & 1 */
/* implicit: ~Item, operator delete (base destructor, deleting destructor) */

// FUNC 0x0012A240 gt2k_menu::PrizeCounter::~PrizeCounter
/* inline in game/menu/gtmode/T099_prize_counter.h; egcs emits this copy (the one the
 * three vtables use): stores the vptr, then the global operator delete
 * (0x0027C0C0) if flags & 1 - PrizeCounter is not a menu Object */
/* implicit: operator delete (deleting destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__user_type_info, no bases). */
// FUNC 0x0012A270 __tfQ29gt2k_menu12PrizeCounter
extern "C" void *__tfQ29gt2k_menu12PrizeCounter(void)
{
    if (__tiQ29gt2k_menu12PrizeCounter[0] == 0)
        __rtti_user(__tiQ29gt2k_menu12PrizeCounter, "Q29gt2k_menu12PrizeCounter");
    return __tiQ29gt2k_menu12PrizeCounter;
}
#endif
