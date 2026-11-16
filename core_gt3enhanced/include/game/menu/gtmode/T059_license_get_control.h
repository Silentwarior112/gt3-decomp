/* T059_license_get_control.h - gt2k_menu::LicenseGetControl, the "licence
 * acquired" screen, and gt2k_menu::PrizeControlBase (translation unit T059,
 * unit u019), 0x0013A088..0x0013A8D0.
 *
 * license_get_screen (called by T053's run_license_test) runs
 * "menu/lc_get.mbl", page "license_get", on a 1 MB menu
 * heap with the controller on the stack, then records the licence trophy
 * (GameStatus largeTrophy bit 1 << licence; licences 0..5 = B, A, IB, IA,
 * S, "d").  The page shows the licence trophy model ("render_model", T058
 * create_trophy_license) and the licence's two message layers
 * (g_0029F2C8): the first is shown for 240 frames and faded out
 * (g_003185A0), the second appears 120 frames later; the page takes input
 * 60 frames after that.
 *
 * PrizeControlBase is the base of the screens with a PrizeBase model
 * (LicenseGetControl, TrophyGetControl T060).  As with the T056 screens, the
 * constructors are inline and the destructors implicit in the original
 * (stack objects of the entry functions; PrizeControlBase's vtable and destructor have a copy
 * in T059 and in T060). */
/* PrizeControlBase has a guard of its own: T060 includes only it
 * (T059_BASE_ONLY), as T056 does through T060.  egcs writes the type names
 * of the classes a unit defines in the reverse order of their definitions,
 * and retail T056 / T060 have PrizeControlBase's but not
 * LicenseGetControl's (T058 has both: it includes this header whole). */
#ifndef GAME_T059_PRIZE_CONTROL_BASE
#define GAME_T059_PRIZE_CONTROL_BASE

#include "game/game.h"
/* only ControlBaseWithPageManager: T060 must see PrizeCounter (T099) after
 * PrizeControlBase (see T056) */
#define T056_BASE_ONLY
#include "game/menu/gtmode/T056_gtmode_trophy_control.h" /* base gt2k_menu::ControlBaseWithPageManager */
#undef T056_BASE_ONLY
#include "game/menu/gtmode/T460_prize_base.h"           /* PrizeBase (prize) */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (g_0049BDD8) */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (g_003185A0) */

namespace gt2k_menu {

/* vtable 0x003186B0 (2 slots: [1] ControlBase::waitFrame; copy 0x00318B00
 * in T060).  typeinfo __tfQ29gt2k_menu16PrizeControlBase 0x0013A7C8 (si,
 * base ControlBaseWithPageManager; storage 0x004B1670, declared in T056;
 * unreferenced copy 0x0013B278 T060).  Size 0xE7C: the derived screens add
 * their own int at 0xE7C (the licence, the prize money), which their
 * constructors store after the PrizeControlBase part is built, together
 * with their vptr (the store is in the exception region of the finished
 * base, not in PrizeBase's). */
class PrizeControlBase : public ControlBaseWithPageManager {
public:
    PrizeControlBase() : prize() {}
    /* ~PrizeBase, ~ControlBase.  0x0013A818 is the copy both vtables use;
     * unreferenced copy 0x0013B2C8 T060. */
    /* [0] +0x0C 0x0013A818 ~PrizeControlBase: implicit (no vptr store) */

    /*         ControlBaseWithPageManager      0x000 .. 0xDE8 */
    PrizeBase  prize;                       /* 0xDE8 the prize model */
};
EGCS_SIZE(gt2k_menu::PrizeControlBase, 0xE7C);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu16PrizeControlBase(void);     /* 0x0013A7C8 */
/* Its __si_type_info node (T060's TrophyGetControl uses it too; the plan
 * put it in T056). */
extern u32 __tiQ29gt2k_menu16PrizeControlBase[3];   /* 0x004B1670 */
}
#endif

#endif /* GAME_T059_PRIZE_CONTROL_BASE */

#if !defined(GAME_T059_LICENSE_GET_CONTROL_H) && !defined(T059_BASE_ONLY)
#define GAME_T059_LICENSE_GET_CONTROL_H

namespace gt2k_menu {

/* vtable 0x00318698 (2 slots).  typeinfo
 * __tfQ29gt2k_menu17LicenseGetControl 0x0013A718 (si, base
 * PrizeControlBase; storage 0x004B17E0).  Size 0xED8 (sp+0x000 in
 * license_get_screen). */
class LicenseGetControl : public PrizeControlBase {
public:
    LicenseGetControl(int l) : PrizeControlBase(), licence(l), render() {}
    /* render, ~PrizeBase, ~ControlBase. */
    /* [0] +0x0C 0x0013A768 ~LicenseGetControl: implicit (no vptr store) */
    /* "menu/lc_get.mbl", the licence trophy "menu/lic_*.prm" (PrizeModelSpec)
     * into a buffer of its own. */
    void load();                            /* 0x0013A088 */
    void unload();                          /* 0x0013A100 the pages, the model */
    /* The render with a transition, the jingle, the item factory
     * g_0049BDD8. */
    void begin();                           /* 0x0013A130 */
    void end();                             /* 0x0013A200 */
    /* Page "license_get" (frozen while the two messages come in), then until
     * it is left. */
    void mainLoop();                        /* 0x0013A248 */
    void run();                             /* 0x0013A410 load, begin, mainLoop, end, unload */

    /*         PrizeControlBase                0x000 .. 0xE7C */
    int        licence;                     /* 0xE7C LicenseIndex 0..5 */
    RenderBase render;                      /* 0xE80 */
};
EGCS_SIZE(gt2k_menu::LicenseGetControl, 0xED8);

} /* namespace gt2k_menu */

/* license_get_screen (0x0013A458): its own header since u017's data pass
 * (its caller T053 saw it without the classes). */
#include "game/menu/gtmode/T059_license_get_entry.h"
/* .ctors entry 0x0028F074: g_0049BDD8 = "render_model" (T058
 * create_trophy_license). */
void T059_static_init(void);                /* 0x0013A6E0 */

/* g_0049BDD8 (0x0049BDD8, .bss) LicenseGetControl's item factory: a file
 * static of T059 (retail has it in .bss), defined there (u019's data
 * pass). */
/* The two message elements of "license_get" by licence: {"b_mess0",
 * "b_mess1"}, {"a_mess0", ..}, ib, ia, s, d. */
extern const char *g_0029F2C8[6][2];        /* 0x0029F2C8 (T059 .data) */
extern const gt2k_menu::OperatorCode g_003185A0[];  /* 0x003185A0 (T059 .rodata) the first message's
                                               fade-out (Object::disappearAll): opFadeColors, alpha
                                               -1 / s for 1 s, then opRelease (kill) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu17LicenseGetControl(void);    /* 0x0013A718 */
}
#endif

#endif /* GAME_T059_LICENSE_GET_CONTROL_H */
