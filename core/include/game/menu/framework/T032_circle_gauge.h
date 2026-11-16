/* T032_circle_gauge.h - gt2k_menu::CircleGauge, the round gauge of a car
 * statistic (translation unit T032, unit u007), 0x00115F70..0x00116508.
 *
 * The handling / speed / acceleration gauges of the arcade car selects
 * (ArcadeControl, T021 / T023).  A ring segment from angleZero (230) towards
 * angleFull (30): a shadow ring over the whole range in colour 0, the value
 * in colour 1, the part past 80% in colour 2.  Once started (virtual_6) the
 * value rises to 1.0 by 0.04 a frame, then falls to the target by 0.02.
 * The target is a statistic of the car's spec record (MenuCarSpec's first
 * part, T278): kind 0 / 2 the bytes +0x0C / +0x0D as tenths (0 shown as 7),
 * kind 1 the top speed in km/h (T278 F_001D9778 'kmph') / 350, at most 1. */
#ifndef GAME_T032_CIRCLE_GAUGE_H
#define GAME_T032_CIRCLE_GAUGE_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

struct MenuCarSpec;                         /* T232 (pointers only) */

namespace gt2k_menu {
class PageLayout;                           /* T229 */

/* The statistic a gauge shows (the constructor's / setKind's kind; T021 /
 * T023 build the handling, speed and acceleration gauges with 0, 1, 2). */
enum CircleGaugeKind {
    CIRCLE_GAUGE_HANDLING     = 0,          /* spec byte +0x0C, tenths (0 shown as 7) */
    CIRCLE_GAUGE_SPEED        = 1,          /* the top speed in km/h / 350, at most 1 */
    CIRCLE_GAUGE_ACCELERATION = 2           /* spec byte +0x0D, tenths (0 shown as 7) */
};

/* vtable 0x0030DE48 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [7] Transitive's 0x001BBA00).
 * typeinfo __tfQ29gt2k_menu11CircleGauge 0x00116470 (si, base Item;
 * storage 0x004B1520).  Size 0x124 (operator new in its four callers).
 * The destructor is implicit in the original: 0x001164C0 does not store the
 * vptr (egcs stores it in every user-declared destructor, not in the one it
 * generates for a class with one base): ~Item, then the deleting part. */
class CircleGauge : public Item {
public:
    /* Item(name, layout); angles 230 -> 30, maxValue 1, then setKind. */
    CircleGauge(MenuCarSpec *car, const char *name, PageLayout *layout,
                int kind);                  /* 0x00115F70 */
    /*  [0] +0x0C 0x001164C0 ~CircleGauge: implicit (see above) */
    virtual void virtual_1();               /* [1] +0x14 0x00116140 Item's, then the animation step
                                               (state 0 .. 4) */
    virtual void virtual_2();               /* [2] +0x1C 0x001161F8 draws the rings */
    virtual void virtual_6();               /* [6] +0x3C 0x00116040 starts the animation (hold = 0) */
    /* The target from statistic `kind' of the car (other kinds keep it);
     * hold, state and value 0. */
    void setKind(int kind);                 /* 0x00116048 */

    MenuCarSpec *car;                       /* 0x100 */
    f32 angleFull;                          /* 0x104 30.0: the angle of value 1 */
    f32 angleZero;                          /* 0x108 230.0: the angle of value 0 */
    f32 value;                              /* 0x10C the value shown */
    f32 maxValue;                           /* 0x110 1.0 */
    f32 target;                             /* 0x114 the statistic, 0 .. 1 */
    int hold;                               /* 0x118 nonzero: the animation waits */
    int state;                              /* 0x11C 0 .. 4 */
    int unk120;                             /* 0x120 */
};
EGCS_SIZE(gt2k_menu::CircleGauge, 0x124);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu11CircleGauge(void);  /* 0x00116470 */
extern u32 __tiQ29gt2k_menu11CircleGauge[3];    /* 0x004B1520 its __si_type_info */
}
#endif

#endif /* GAME_T032_CIRCLE_GAUGE_H */
