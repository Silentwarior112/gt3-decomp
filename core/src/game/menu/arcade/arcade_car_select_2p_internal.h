/* arcade_car_select_2p_internal.h - unit u003 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001060B8..0x001095A8, second pass.
 *
 * Translation units (both declared with the class in
 * include/game/menu/arcade/T010_arcade_control.h, gt2k_menu::ArcadeControl):
 *   T021  "ar/select_car_2p", the two-player car select: a per-player state
 *         machine class -> car / colour -> transmission -> racing / dri
 *         option -> ready, run side by side for both pads
 *         (ArcadeControl::carSelect2P, from runTwoPlayer)
 *   T022  "ar/select_class", the one-player class select, with the i.LINK
 *         battle variant (ArcadeControl::classSelect, from runSingleRace,
 *         runTimeAttack and runILink)
 *
 * Every function of the unit is a member of ArcadeControl; several are
 * reached through pointers to members (T021 / T022 .sdata 0x00350C40 ..
 * 0x00350CF0, egcs layout {s16 delta; s16 index (0 = NULL, < 0 = not
 * virtual); pfn}).  `player' is 1 or 2 throughout; the code tests
 * player == 2 / != 2, so anything but 2 is player 1.
 *
 * Every class is defined in a canonical header.  Each source includes the
 * canonical headers it needs itself, in the order that gives the original's
 * type names (data pass, ref/data/u003/notes.md), then this header: the
 * player accessors below, which need ArcadeControl. */
#ifndef U003_H
#define U003_H

#include "types.h"

/* own translation units */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl; T247 ControlBase, T211 PageManager,
                                               T232 MenuCarSpec, T251 / T017 indices, T250 Ipic */

/* enum ArcadeClass (ARCADE_CLASS_*): game/menu/arcade/T010_arcade_control.h. */

/* The menu sound effects: T233's MENU_SE_*. */

/* The screen width of the menus (T259 MENU_SCREEN_W): player 2's logos are
 * mirrored to the right half (x' = 640 - x). */

namespace gt2k_menu {

/* Player `player''s copy of a two-player member of ArcadeControl: player
 * 2's own one, anything else player 1's.  (? Inline accessors of the
 * original: the selection goes through an inline call's return value, which
 * changes the order sched1 gives the two addresses and the registers the
 * pointers get; a plain ?: in the function does not reproduce retail, these
 * do in every T021 / T022 function.  They are not members here so that T010's
 * object, where egcs writes the out-of-line copies of the class's inline
 * members, stays as it is.) */
#define U003_PLAYER_MEMBER(T, fn, m1, m2) \
    static inline T *fn(ArcadeControl *ac, int player) \
    { return (player != 2) ? &ac->m1 : &ac->m2; }

U003_PLAYER_MEMBER(MenuControl, controlOf, control1, control2)
U003_PLAYER_MEMBER(MenuCarSpec, specOf, car1Spec, car2Spec)
U003_PLAYER_MEMBER(SmartIndexT<int>, classOf, class1, class2)
U003_PLAYER_MEMBER(SmartCarIndex, carOf, car1, car2)
U003_PLAYER_MEMBER(SmartIndexT<int>, colorOf, color1, color2)
U003_PLAYER_MEMBER(SmartIndexT<int>, racingOf, racing1, racing2)
U003_PLAYER_MEMBER(SmartIndexT<int>, transmissionOf, transmission1, transmission2)
U003_PLAYER_MEMBER(Item *, logoOf, logo, logo2)
U003_PLAYER_MEMBER(Item *, carItemOf, carItem, carItem2)
U003_PLAYER_MEMBER(Car *, carViewOf, carView, carView2)
U003_PLAYER_MEMBER(MiniCursor *, miniClassesOf, miniClasses, miniClasses2)
U003_PLAYER_MEMBER(MiniCursor *, miniCarsOf, miniCars, miniCars2)
U003_PLAYER_MEMBER(SelectBox *, colorChipsOf, colorChips, colorChips2)
U003_PLAYER_MEMBER(Item *, driveTrainOf, driveTrain, driveTrain2)
U003_PLAYER_MEMBER(String *, powerTextOf, powerText, powerText2)
U003_PLAYER_MEMBER(String *, torqueTextOf, torqueText, torqueText2)
U003_PLAYER_MEMBER(String *, handlingTextOf, handlingText, handlingText2)
U003_PLAYER_MEMBER(String *, speedTextOf, speedText, speedText2)
U003_PLAYER_MEMBER(String *, accelTextOf, accelText, accelText2)
U003_PLAYER_MEMBER(CircleGauge *, handlingGaugeOf, handlingGauge, handlingGauge2)
U003_PLAYER_MEMBER(CircleGauge *, speedGaugeOf, speedGauge, speedGauge2)
U003_PLAYER_MEMBER(CircleGauge *, accelGaugeOf, accelGauge, accelGauge2)
U003_PLAYER_MEMBER(Item *, autoNoOf, autoNo, autoNo2)
U003_PLAYER_MEMBER(Item *, autoYesOf, autoYes, autoYes2)
U003_PLAYER_MEMBER(Item *, manuNoOf, manuNo, manuNo2)
U003_PLAYER_MEMBER(Item *, manuYesOf, manuYes, manuYes2)
U003_PLAYER_MEMBER(Item *, driNoOf, driNo, driNo2)
U003_PLAYER_MEMBER(Item *, driYesOf, driYes, driYes2)
U003_PLAYER_MEMBER(Item *, racingNoOf, racingNo, racingNo2)
U003_PLAYER_MEMBER(Item *, racingYesOf, racingYes, racingYes2)

#undef U003_PLAYER_MEMBER

/* carBuffer1 / carBuffer2 indexed as the array they were in the original
 * (? void *carBuffer[2]; T010_arcade_control.h keeps the two names).  An
 * array element (ARRAY_REF) gives retail's code, the offset 4 or 0 selected
 * and added to &carBuffer1; a ?: of the two members, or pointer arithmetic
 * on &carBuffer1, selects between the two addresses instead.  The view reads
 * the same two words; GCC 14 gets the plain ?:. */
struct ArcadeCarBuffers { void *buf[2]; };
#ifdef BOOT_OLD_GCC
#define U003_CAR_BUFFER(ac, i)  (((ArcadeCarBuffers *)&(ac)->carBuffer1)->buf[i])
#else
#define U003_CAR_BUFFER(ac, i)  ((i) ? (ac)->carBuffer2 : (ac)->carBuffer1)
#endif

} /* namespace gt2k_menu */

#endif /* U003_H */
