/* race_menu_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u062,
 * 0x001AFA60..0x001B2C20, second pass.
 *
 * Translation units (link order):
 *   T197  the race menus' common helpers (their 128 KB menu heap, the layout
 *         files "menu/<country>/<file>.pmb", layers / items / colours, the
 *         640 x 448 2D projection widened in 16:9, scissor rectangles) and
 *         RaceMenuPage (a MenuControl + the PageLayout of one page)
 *   T198  RaceMenuButton, RaceMenuImageChangeButton, RaceMenuImageBlinkButton
 *   T199  RaceMenuLayer, the RaceMenuWindow and RaceMenuSelectWindow methods
 *   T200  the race result board: ResultBoard, its bars (ResultBar,
 *         ResultTitleBar, ResultRankBar, ResultTermBar) and CarIconItem
 *   T201  GT2KMSGRACEMENUTEXT::Message, the "RACEMENUTEXT" real-time texts
 *   T202  a menu sound played after a delay (a gt2k_menu::Timer)
 *   T266  gt2k_menu::SmartIndexDoubleRange
 *
 * Every class is defined in its canonical header (include/game/T197..T202,
 * T266), which the sources include themselves; this header holds what only
 * this unit uses: the race menu heap pointer, named constants, the T200
 * script opcodes, the typeinfo storage of the unit's own classes (GCC 14).
 * T200 defines its scripts and icon quad, T201 its keys, texts and
 * instance.
 */
#ifndef U062_H
#define U062_H

#include "types.h"
#include "libc.h"                           /* strcpy, strcat, memalign, free */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user (cxxrt.h clashes with <new>) */
#include <stddef.h>

/* No class headers here: each source includes the outside headers it uses,
 * then the unit's own headers, its own last.  egcs writes the type names of
 * the classes a unit sees into its .rodata in reverse class-definition
 * order, and the __tf functions' name references show retail's order (T200,
 * T201, T266: their own classes were defined last).  Only T199 / T200
 * include <new> (retail's .rodata of those two starts with its
 * "bad_alloc"). */

/* ======================================================================== */
/* T197 (only used here)                                                    */
/* ======================================================================== */
/* The race menu heap: memalign(0x40, RACE_MENU_HEAP_SIZE) by
 * race_menu_heap_alloc, NULL when freed (T197 .sdata, defined there = NULL). */
extern void *g_00352398;                    /* 0x00352398 gp-0x5658 */
enum { RACE_MENU_HEAP_SIZE = 0x20000 };     /* 128 KB */

/* The menus' virtual screen (Drawable coordinates, T259). */
enum { RACE_MENU_SCREEN_W = 640, RACE_MENU_SCREEN_H = 448 };

/* ======================================================================== */
/* T199                                                                     */
/* ======================================================================== */
/* The menu sounds of RaceMenuSelectWindow::input: T233's MENU_SE_CANCEL /
 * MENU_SE_CURSOR / MENU_SE_CONFIRM. */
/* The cursor's auto-repeat (Control::getRepeat): first after 30 frames,
 * then every 3. */
enum { RACE_MENU_REPEAT_FIRST = 30, RACE_MENU_REPEAT_INTERVAL = 3 };

/* ======================================================================== */
/* T200 (only used here)                                                    */
/* ======================================================================== */
enum { RESULT_BOARD_ROWS = 7 };             /* ResultBoard::bars */
/* ResultBoard::mode: RESULT_BOARD_* (T200). */

/* The bars' operator scripts (T200 .rodata; RaceMenuLayer::startScript):
 * opSetAlpha (24), opFadeAlpha (25) rate per second for a time, opRemove
 * (3) ends the operator.  Defined in T200 between the functions whose
 * strings precede them in retail's .rodata. */
enum {
    RESULT_OP_REMOVE     = 3,               /* opRemove: the script ends */
    RESULT_OP_SET_ALPHA  = 24,              /* opSetAlpha: alphaRatio = f10 */
    RESULT_OP_FADE_ALPHA = 25               /* opFadeAlpha: alphaRatio += f10 per second for f14 s */
};
/* One instruction: code, the operands f10 / f14, result (1 ends the frame);
 * the other fields are 0 in every T200 script. */
#define RESULT_OP(code, f10, f14, result) \
    { (code), 0, 0, { 0, 0, 0, 0 }, (f10), (f14), 0.0f, 0.0f, 0.0f, \
      { 0, 0, 0, 0, 0, 0, 0, 0 }, (result) }
/* The scripts g_00331C80 / D28 / DC8 / E88 and CarIconItem's quad
 * g_00331C30 / C50 are declared and defined in T200. */
enum { CAR_ICON_W = 64, CAR_ICON_H = 32 };  /* CarIcon::pixels, PSMCT32 */

/* ======================================================================== */
/* T201 (only used here)                                                    */
/* ======================================================================== */
/* The keys g_002C5CF0 and the instance g_002C5D20 are defined in T201. */
/* The indices of the RACEMENUTEXT texts (g_002C5D08, T201). */
enum {
    RACEMENUTEXT_LAP     = 0,               /* "lap" */
    RACEMENUTEXT_LAPS    = 1,               /* "laps" */
    RACEMENUTEXT_PT      = 2,               /* "pt" */
    RACEMENUTEXT_PTS     = 3,               /* "pts" */
    RACEMENUTEXT_RUNNING = 4,               /* "Running..." */
    RACEMENUTEXT_COUNT   = 5
};

/* ======================================================================== */
/* The typeinfo storage of the unit's own classes (.sbss / .bss, filled by  */
/* their __tf functions; the canonical headers declare only the functions)  */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __ti14RaceMenuButton[2];         /* 0x003532F0 gp-0x4700 __user_type_info */
extern u32 __ti25RaceMenuImageChangeButton[3];  /* 0x004B23E0 __si_type_info */
extern u32 __ti24RaceMenuImageBlinkButton[3];   /* 0x004B23F0 */
extern u32 __ti11CarIconItem[3];            /* 0x004B2400 */
extern u32 __ti13ResultRankBar[3];          /* 0x004B2410 */
extern u32 __ti9ResultBar[3];               /* 0x004B2420 */
extern u32 __ti14ResultTitleBar[3];         /* 0x004B1290 */
extern u32 __ti13ResultTermBar[3];          /* 0x004B12F0 */
extern u32 __tiQ219GT2KMSGRACEMENUTEXT7Message[3];      /* 0x004B2430 */
extern u32 __tiQ29gt2k_menu21SmartIndexDoubleRange[3];  /* 0x004B2440 */
}
#endif

#endif /* U062_H */
