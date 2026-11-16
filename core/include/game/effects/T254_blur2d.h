/* T254_blur2d.h - gt2k_menu::Blur2D (translation unit T254, unit u071),
 * 0x001CD6A0..0x001CDA48.
 *
 * A Transitive whose w / h are a blur radius: its draw renders the screen
 * buffer being drawn 8 times, offset by the sample pattern g_003372B0 scaled
 * by (w, h), at 1/8 alpha into a copy of the buffer, then draws that copy
 * back over the screen.  Nothing when w and h are both 0. */
#ifndef GAME_T254_BLUR2D_H
#define GAME_T254_BLUR2D_H

#include "game/game.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* base gt2k_menu::Transitive */

namespace gt2k_menu {

/* vtable 0x00337178 (8 slots: [1] / [4]..[7] Transitive's, [3] Object's).
 * typeinfo __tfQ29gt2k_menu6Blur2D 0x001CD9B0 (__si_type_info 0x004B2690,
 * base Transitive).  No data of its own: size 0xC4 (operator new in F_0010C028,
 * T023, which expands the constructor).  The destructor is implicit: slot [0]
 * 0x001CDA00 only calls ~Transitive and stores no vptr. */
class Blur2D : public Transitive {
public:
    Blur2D() {}                             /* 0x001CD978 (inline: F_0010C028 expands it; this
                                               out-of-line copy has no callers) */
    /*  [0] +0x0C 0x001CDA00 the implicit destructor */
    virtual void virtual_2();               /* [2] +0x1C 0x001CD6A0 draw: the blur pass (u071: draw) */
};
EGCS_SIZE(gt2k_menu::Blur2D, 0xC4);

} /* namespace gt2k_menu */

/* The 8 sample offsets of the blur, g_003372B0 (0x003372B0, T254 .rodata):
 * {-0.875, 0.875}, {-0.625, -0.375}, {-0.375, 0.375}, {-0.125, -0.875},
 * {0.125, -0.125}, {0.375, 0.625}, {0.625, -0.625}, {0.875, 0.125}.  A
 * static const of T254 (retail has it after T254's type names, where egcs
 * writes a const table of internal linkage), so not declared here (u071's
 * data pass; only T254 reads it). */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu6Blur2D(void);        /* 0x001CD9B0 */
}
#endif

#endif /* GAME_T254_BLUR2D_H */
