/* T099_prize_counter.h - gt2k_menu::PrizeCounter, the rolling prize-money
 * counter, and the items that show it (translation unit T099, unit u014),
 * 0x00129140..0x0012A2B0.
 *
 * The prize screens embed a PrizeCounter built with (money, money + prize)
 * - GTmodeTrophyControl (T056) and ConsolationControl (T057) at +0xEF8,
 * TrophyGetControl (T060) at +0xED8 - and register three hooks each
 * (MenuHook tables, T229): "count" (PrizeCounterControlItem: "Cr. <money>",
 * runs the counter after a delay; pad 1's decide speeds it up x3, a second
 * one finishes it), "count_shadow" (PrizeCounterShadowItem: the same text)
 * and "get_money_disp" (PrizeCounterRemainItem: "<prize> Cr. <left>").
 * The nine hooks are three copies of one set (? a template on the screen
 * class); only the counter's offset in the screen and the delay differ.
 * Their ctx is the screen; parent, p8 .. p14 are not used.  Each takes the
 * item's texture (PageLayout::getTexture) before operator new (Object's,
 * the menu heap). */
#ifndef GAME_T099_PRIZE_COUNTER_H
#define GAME_T099_PRIZE_COUNTER_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
/* What the items' inline virtual functions use: */
#include "game/data/messages/T094_msg_prize_rt.h"         /* g_00351D18 (the prize money text) */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (owner->control1) */
#include "game/common/T494_text_utils.h"           /* format_number */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */

namespace gt2k_menu {
class Object;                               /* T220 */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */

/* PrizeCounter +0x24: fires once the amounts added reach 1.0 and at least
 * three frames have passed (the counting sound, menu sound 3).  Not
 * polymorphic; 8 bytes. */
struct CountTick {
    /* 1 (and restarts) once amount >= 1 and 3 calls were made; otherwise
     * amount is clamped to 1 and frames to 3. */
    int advance(f32 add);                   /* 0x00129140 */

    f32 amount;                             /* 0x0 */
    int frames;                             /* 0x4 */
};

/* vtable 0x00313B08 (1 slot; the copies 0x003176A8 in T056 and 0x00318AF0
 * in T060 are emitted with the inline destructor).  typeinfo
 * __tfQ29gt2k_menu12PrizeCounter 0x0012A270 (__user_type_info 0x00353328,
 * no bases; unreferenced copies 0x00138B98 T056, 0x0013B238 T060).  0x30
 * bytes of data (s64 first: 8-aligned), vptr +0x30; size 0x38 (the last
 * member of the 0xF30-byte GTmodeTrophyControl, at +0xEF8).  Counts
 * current up to target: state 0 idle, 1 start (5 / 21 a frame for 90
 * frames), 2 accelerating, 3 done. */
class PrizeCounter {
public:
    PrizeCounter(s64 cur, s64 tgt);         /* 0x001291C0 scale 1, the rest 0 */
    /* Inline: stores the vptr; ::operator delete when bit 0 of the flags is
     * set (PrizeCounter is not a menu Object).  0x0012A240 is T099's copy
     * (the one the vtables use); unreferenced copies 0x00138B68 T056,
     * 0x0013B208 T060. */
    virtual ~PrizeCounter() {}              /* [0] +0x0C 0x0012A240 */

    /* Odometer digit n (0 = units) of current as a float (0 for a leading
     * zero; the fraction added while the units digit is 9).  No callers. */
    f32  digit(int n);                      /* 0x00129208 */
    void update();                          /* 0x001292C0 one frame of counting */
    void setScale(f32 s);                   /* 0x00129478 clamped to [1, 3] */
    void start();                           /* 0x001294B0 state 1 */
    void finish();                          /* 0x001294C0 current = target */
    int  isDone();                          /* 0x001294D0 state == 3 */

    s64       target;                       /* 0x00 */
    s64       current;                      /* 0x08 */
    u32       frame;                        /* 0x10 frames in state 1 */
    f32       fraction;                     /* 0x14 the fractional part of current */
    f32       speed;                        /* 0x18 added each frame */
    f32       accel;                        /* 0x1C added to speed each frame */
    f32       scale;                        /* 0x20 speed factor, 1 .. 3 */
    CountTick tick;                         /* 0x24 */
    u8        state;                        /* 0x2C */
    /*        vptr                             0x30 */
};
EGCS_SIZE(gt2k_menu::CountTick, 0x8);
EGCS_SIZE(gt2k_menu::PrizeCounter, 0x38);

/* The three items are defined Shadow, Control, Remain (u014's revisit):
 * egcs writes the type names of keyless classes in reverse definition
 * order, and every retail copy has RemainItem's, ControlItem's,
 * ShadowItem's, then PrizeCounter's name (T099 0x00313BF8; T104, T056,
 * T057, T058, T060); T099's vtables and out-of-line copies come in the same
 * order. */

/* "count_shadow".  vtable 0x00313AC0 (8 slots: [1] Item's update
 * 0x001B9F98, [3] .. [7] as Item's).  typeinfo
 * __tfQ29gt2k_menu22PrizeCounterShadowItem 0x0012A1A8 (si, base Item;
 * storage 0x004B16D0).  Size 0x104.  The destructor is implicit in the
 * original (0x0012A1F8, no vptr store). */
class PrizeCounterShadowItem : public Item {
public:
    /* Inline (the hooks expand it). */
    PrizeCounterShadowItem(MenuLayoutItem *res, MenuTexture *tex, PrizeCounter *c)
        : Item(res, tex), counter(c)
    {
        setText("");
    }
    /*  [0] +0x0C 0x0012A1F8 ~PrizeCounterShadowItem: implicit (see above) */
    /* draw: the same text as PrizeCounterControlItem's. */
    virtual void virtual_2()                /* [2] +0x1C 0x0012A140 */
    {
        char num[0x40];                     /* sp+0x00 */
        char text[0x40];                    /* sp+0x40 */
        format_number(num, counter->current);
        sprintf(text, "Cr. %s", num);
        setText(text);
        Item::virtual_2();
    }

    PrizeCounter *counter;                  /* 0x100 */
};
EGCS_SIZE(gt2k_menu::PrizeCounterShadowItem, 0x104);

/* "count".  vtable 0x00313A78 (8 slots: [3] .. [7] as Item's).  typeinfo
 * __tfQ29gt2k_menu23PrizeCounterControlItem 0x0012A0A8 (si, base Item;
 * storage 0x004B16B0).  Size 0x114.  The destructor is implicit in the
 * original (0x0012A0F8, no vptr store). */
class PrizeCounterControlItem : public Item {
public:
    /* Inline (the hooks expand it): starts the counter. */
    PrizeCounterControlItem(MenuLayoutItem *res, MenuTexture *tex, PrizeCounter *c,
                            ControlBase *o, int d)
        : Item(res, tex), counter(c), owner(o), frame(0), delay(d), presses(0), state(0)
    {
        setText("");
        counter->start();
    }
    /*  [0] +0x0C 0x0012A0F8 ~PrizeCounterControlItem: implicit (see above) */
    /* update: after `delay' frames the counter runs (owner->control1's
     * decide: setScale(3), then finish()); then Item's. */
    virtual void virtual_1()                /* [1] +0x14 0x00129FB8 */
    {
        switch (state) {
        case 0:                             /* waiting */
            if (delay < frame)
                state = 1;
            break;
        case 1:                             /* counting */
            if (owner->control1.decide != 0) {
                switch (presses) {
                case 0:
                    counter->setScale(3.0f);
                    break;
                case 1:
                    counter->finish();
                    break;
                }
                presses++;
            }
            counter->update();
            if (counter->isDone()) {
                frame = 0;
                state = 2;
            }
            break;
        case 2:                             /* done (the case gives retail's == 1, < 2,
                                               != 0 test tree) */
            break;
        }
        frame++;
        Item::virtual_1();
    }
    /* draw: "Cr. <current>", Item's. */
    virtual void virtual_2()                /* [2] +0x1C 0x00129F50 */
    {
        char num[0x40];                     /* sp+0x00 */
        char text[0x40];                    /* sp+0x40 */
        format_number(num, counter->current);
        sprintf(text, "Cr. %s", num);
        setText(text);
        Item::virtual_2();
    }

    PrizeCounter *counter;                  /* 0x100 */
    ControlBase  *owner;                    /* 0x104 the screen: control1.decide */
    int           frame;                    /* 0x108 */
    int           delay;                    /* 0x10C frames before counting (120 or 360) */
    s8            presses;                  /* 0x110 decide presses while counting */
    s8            state;                    /* 0x111 0 waiting, 1 counting, 2 done */
};
EGCS_SIZE(gt2k_menu::PrizeCounterControlItem, 0x114);

/* "get_money_disp".  vtable 0x00313A30 (8 slots: [3] .. [7] as Item's).
 * typeinfo __tfQ29gt2k_menu22PrizeCounterRemainItem 0x00129EB8 (si, base
 * Item; storage 0x004B1690).  Size 0x114 (operator new in the hooks).  The
 * destructor is implicit in the original: 0x00129F08 does not store the
 * vptr (~Item with 0, Object's operator delete). */
class PrizeCounterRemainItem : public Item {
public:
    /* Inline (the hooks expand it). */
    PrizeCounterRemainItem(MenuLayoutItem *res, MenuTexture *tex, PrizeCounter *c,
                           ControlBase *o)
        : Item(res, tex), fade(0), counter(c), owner(o), timer(0), state(0)
    {
        setText("");
    }
    /*  [0] +0x0C 0x00129F08 ~PrizeCounterRemainItem: implicit (see above) */
    /* The three items' virtual functions are inline in their class bodies
     * (u014's revisit; u019 had declared them inline and defined them in
     * T099).  No class has a key function, so every unit that sees them
     * writes their type names, and every unit that includes this header
     * writes their strings: retail T104, T056, T057, T058 and T060 each
     * start their .rodata with "%s Cr. %s" (after "bad_alloc") and have
     * "Cr. %s" in .sdata, as T099 does.  T099, which builds the items,
     * emits them with the vtables at the end of the unit, per class v2, v1,
     * __tf, destructor, the classes in reverse definition order (Remain,
     * Control, Shadow, PrizeCounter: retail's text, vtable and type name
     * order, the jump table of RemainItem::virtual_1 after the screens'
     * names).  The out-of-line copies' FUNC markers are in T099. */
    /* update: Item's, then fades in over 60 frames, waits for the counter,
     * holds 61 frames and fades out. */
    virtual void virtual_1()                /* [1] +0x14 0x00129DE0 */
    {
        Item::virtual_1();
        switch (state) {
        case 0:                             /* fade in */
            fade = fade + 0.016666668f;     /* 1 / 60 */
            if (fade >= 1.0f) {
                fade = 1.0f;
                state = 1;
            }
            break;
        case 1:                             /* until the counter is done */
            if (counter->isDone())
                state = 2;
            break;
        case 2:                             /* hold */
            timer++;
            if (timer >= 61)
                state = 3;
            break;
        case 3:                             /* fade out */
            fade = fade - 0.016666668f;
            if (fade <= 0.0f) {
                fade = 0.0f;
                state = 4;
            }
            break;
        case 4:                             /* gone */
            break;
        }
    }
    /* draw: "<g_00351D18[0]> Cr. <target - current>" (T494 format_number, ','
     * every 3 digits), alpha = fade, Item's draw. */
    virtual void virtual_2()                /* [2] +0x1C 0x00129D68 */
    {
        char num[0x40];                     /* sp+0x00 */
        char text[0x40];                    /* sp+0x40 */
        format_number(num, counter->target - counter->current);
        sprintf(text, "%s Cr. %s", g_00351D18[0], num);  /* EUC-JP "shoukin" (prize money) */
        setText(text);
        alphaRatio = fade;
        Item::virtual_2();
    }

    f32           fade;                     /* 0x100 copied to the alpha */
    PrizeCounter *counter;                  /* 0x104 */
    ControlBase  *owner;                    /* 0x108 the screen (not used) */
    int           timer;                    /* 0x10C frames held */
    s8            state;                    /* 0x110 0 fade in, 1 wait, 2 hold, 3 fade out, 4 gone */
};
EGCS_SIZE(gt2k_menu::PrizeCounterRemainItem, 0x114);

} /* namespace gt2k_menu */

/* The hooks (MenuHookFn, T229).  ctx: the screen, GTmodeTrophyControl
 * (T056) / ConsolationControl (T057) with the counter at +0xEF8, or
 * TrophyGetControl (T060) with it at +0xED8 (the first pass typed them
 * MiniPrizeControlBase / PrizeControlBase).  "count" delays: 120 frames,
 * 360 for _T060.  Users: the hook tables of T056 / T057 (u019) and T060
 * (u020). */
gt2k_menu::Transitive *create_count_T056(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001294E0 */
gt2k_menu::Transitive *create_count_T057(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001295E8 (the same code) */
gt2k_menu::Transitive *create_count_T060(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001296F0 */
gt2k_menu::Transitive *create_count_shadow_T056(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001297F8 */
gt2k_menu::Transitive *create_count_shadow_T057(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001298D0 */
gt2k_menu::Transitive *create_count_shadow_T060(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001299A8 */
gt2k_menu::Transitive *create_get_money_disp_T060(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00129A80 */
gt2k_menu::Transitive *create_get_money_disp_T056(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00129B78 */
gt2k_menu::Transitive *create_get_money_disp_T057(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00129C70 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu22PrizeCounterRemainItem(void);  /* 0x00129EB8 */
void *__tfQ29gt2k_menu23PrizeCounterControlItem(void); /* 0x0012A0A8 */
void *__tfQ29gt2k_menu22PrizeCounterShadowItem(void);  /* 0x0012A1A8 */
void *__tfQ29gt2k_menu12PrizeCounter(void);            /* 0x0012A270 */
/* The __user_type_info of gt2k_menu::PrizeCounter (2 words), filled by
 * __tfQ29gt2k_menu12PrizeCounter.  T099, T056 and T060 use it. */
extern u32 __tiQ29gt2k_menu12PrizeCounter[2];          /* 0x00353328 gp-0x46C8 */
}
#endif

#endif /* GAME_T099_PRIZE_COUNTER_H */
