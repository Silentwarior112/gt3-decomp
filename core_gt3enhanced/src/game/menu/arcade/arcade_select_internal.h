/* arcade_select_internal.h - unit u002 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001036D0..0x001060B8, second pass.
 *
 * Translation units (link order):
 *   T034  gt2k_menu::ConnectStatus (the i.LINK connection panel item)
 *   T018  gt2k_menu::ArcadeControl: the "ar/select_compulsory" page (the
 *         time trial cars tt01.., their logo / description / spec / records)
 *   T019  the FocusFuncData table g_00290940 of the arcade menu and its two
 *         focus functions (filled by a static initialiser)
 *   T020  gt2k_menu::ArcadeControl helpers shared by the arcade pages
 *         (page fade in / out, arrows, the cross bar, the history banners,
 *         the mini cursor, text labels) and three free functions
 *
 * T018 and most of T020 are member functions of ArcadeControl (T010), which
 * declares them with the class.  Every class is defined in its canonical
 * header (include/game/); this header has only what only this unit uses:
 * T020's operator scripts, a float of another unit's .sdata only T020
 * reads, named constants.
 *
 * Each file includes the canonical headers it needs itself, in the order
 * that gives the original's type names in its .rodata (reverse definition
 * order: each file's comment lists the original's).  T034 does not include
 * this header.
 */
#ifndef U002_H
#define U002_H

#include "types.h"
#include <stddef.h>
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (the scripts below) */

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* menu_play_se ids: T233's MENU_SE_* (ARCADE_SE_SCROLL is MENU_SE_LIST_MOVE). */

/* ShowRoomCar::power's unit tag (a multi-character constant; not read). */
enum { UNIT_TAG_PS = 0x7073 /* 'ps' */ };


/* Drawable::anchorType: T259's ANCHOR_*. */

/* ======================================================================== */
/* T020 data: the operator scripts (T217 format) its functions start        */
/* ======================================================================== */
/* Defined in T020 (.rodata 0x00309978..0x00309DF8, before its type names:
 * external linkage, so egcs writes each at its definition). */
/* On ArcadeControl::fade, the full-screen rectangle: */
extern const gt2k_menu::OperatorCode g_00309978[4];   /* 0x00309978 page in: opaque black, alpha
                                                         -4 / s for 0.25 s, then parked at (-10, -10)
                                                         with size 0; opRemove */
extern const gt2k_menu::OperatorCode g_00309A38[5];   /* 0x00309A38 page out: full screen (320, 224,
                                                         640 x 448), transparent black, alpha +4 / s
                                                         for 0.25 s, opCall transitionOutDone; opRemove */
/* arcade_fade_out's: alpha -4 / s for 0.25 s, alpha 0, opRelease (kill()). */
extern const gt2k_menu::OperatorCode g_00309B28[3];   /* 0x00309B28 */
/* The history banners: opMove at -2560 / -2304 / -2048 pixels per second
 * for 0.25 s (640 / 576 / 512 pixels to the left), opRemove; out: +2560 / s
 * for 0.25 s (640 to the right), opRelease (kill()). */
extern const gt2k_menu::OperatorCode g_00309BB8[2];   /* 0x00309BB8 showHistoryLevel */
extern const gt2k_menu::OperatorCode g_00309C18[2];   /* 0x00309C18 hideHistoryLevel */
extern const gt2k_menu::OperatorCode g_00309C78[2];   /* 0x00309C78 showHistoryArea */
extern const gt2k_menu::OperatorCode g_00309CD8[2];   /* 0x00309CD8 hideHistoryArea */
extern const gt2k_menu::OperatorCode g_00309D38[2];   /* 0x00309D38 showHistoryClass */
extern const gt2k_menu::OperatorCode g_00309D98[2];   /* 0x00309D98 hideHistoryClass */

/* 40.0, read through $gp by F_001051B0 only: (40 + 448) * n.  It lies in
 * the .sdata between T246's g_00352700 and T258's "sw_16" (? a T246 ..
 * T257 variable: 448 + 40 is the Cross's vertical wrap, 488). */
extern f32 g_00352704;                      /* 0x00352704 gp-0x52EC (=40.0f) */

#endif /* U002_H */
