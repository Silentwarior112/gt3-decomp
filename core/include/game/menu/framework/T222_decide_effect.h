/* T222_decide_effect.h - gt2k_menu::DecideEffect (translation unit T222, unit
 * u067).
 *
 * The flash over a decided button (Button virtual_15, MapButton's): runs the
 * operator script at 0x00334BC0, sets *done = 1 when the script ends
 * (virtual_6), and is drawn above everything (z = 100000) as a strip of two
 * quads, colour[0] down the middle and colour[1] at the edges.  With a view
 * set (MapButton, T207) the rectangle is mapped through the zoomed map. */
#ifndef GAME_T222_DECIDE_EFFECT_H
#define GAME_T222_DECIDE_EFFECT_H

#include "game/game.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* base gt2k_menu::Transitive */

namespace gt2k_menu {

/* vtable 0x00334A80 (8 slots: [3] Object's, [4] / [5] / [7] Transitive's).
 * typeinfo __tfQ29gt2k_menu12DecideEffect 0x001C0968 (si, base Transitive;
 * __si_type_info 0x004B2550).  Size 0xD4 (operator new in T206 0x001B4598,
 * T207 0x001B5160). */
class DecideEffect : public Transitive {
public:
    DecideEffect(int *done, f32 x, f32 y, f32 width, f32 height); /* 0x001C0640 *done = 0; starts
                                               the script 0x00334BC0 */
    /*  [0] +0x0C 0x001C09B8 the implicit destructor: only ~Transitive, no vptr store */
    virtual void virtual_1();               /* [1] +0x14 0x001C0730 Transitive's */
    virtual void virtual_2();               /* [2] +0x1C 0x001C0770 the 6-vertex strip */
    virtual void virtual_6();               /* [6] +0x3C 0x001C0760 the script has ended: *done = 1 */
    void setView(float *zoom, float *x, float *y); /* 0x001C0750 */

    int   *done;                            /* 0xC4 */
    float *zoom;                            /* 0xC8 the view (all three or none): zoom about */
    float *viewX;                           /* 0xCC   (viewX, viewY) onto the 640 x 448 screen */
    float *viewY;                           /* 0xD0 */
};
EGCS_SIZE(gt2k_menu::DecideEffect, 0xD4);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu12DecideEffect(void);  /* 0x001C0968 */
#endif

#endif /* GAME_T222_DECIDE_EFFECT_H */
