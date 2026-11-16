/* T017_smart_index.h - gt2k_menu::SmartSkipIndex and its car / course
 * variants (translation unit T017, unit u001), 0x00103218..0x001036D0.
 *
 * A SmartIndexT<int> (T251) over [begin, end) whose positions can be
 * invalid (isValid, slot 2): get / size count only the valid ones and move
 * skips the invalid ones.  The indices of the arcade menu (ArcadeControl,
 * T010: course, car1, car2): SmartCarIndex accepts the cars of the player's
 * class that the progress opened, SmartCourseIndex the courses the arcade
 * level allows.  The bodies of get / size / move are shared by the three
 * vtables (SmartSkipIndex's).
 *
 * T017 also emits a copy of SmartIndexT<int>'s typeinfo function (T235 and
 * T240 have the others, T251_smart_index.h).
 *
 * The three constructors are inline: ArcadeControl's constructor (T010,
 * 0x00113AA0..0x00113C3C) expands them (vptr, owner, player2; no calls), and
 * T017's copies (where egcs emits the vtables) have no callers.
 *
 * SmartCourseIndex is defined before SmartCarIndex: egcs emits the classes'
 * deferred functions (constructor copy, typeinfo function and type name) in
 * the reverse order of definition, and retail has SmartCarIndex's first
 * (text 0x00103508 / 0x00103590, names 0x00309378 / 0x00309398). */
#ifndef GAME_T017_SMART_INDEX_H
#define GAME_T017_SMART_INDEX_H

#include "game/game.h"
#include "game/common/T251_smart_index.h"          /* base gt2k_menu::SmartIndexT<int> */

namespace gt2k_menu {
class ArcadeControl;                        /* T010 (pointers only) */

/* vtable 0x00309350 (4 slots: [2] SmartIndexT<int>'s 0x001CC998).  typeinfo
 * __tfQ29gt2k_menu14SmartSkipIndex 0x00103640 (si, base SmartIndexT<int>;
 * __si_type_info 0x004B1470).  vptr +0x14 (SmartIndexT's).  Size 0x1C. */
class SmartSkipIndex : public SmartIndexT<int> {
public:
    /* SmartIndexT() inline: value 0, range [0, 1), wraps. */
    SmartSkipIndex(ArcadeControl *owner) : owner(owner) {}     /* 0x00103610 */
    virtual int get();                      /* [0] +0x0C 0x00103348 the valid indices in [0, value) */
    virtual int size();                     /* [1] +0x14 0x001032C8 the valid indices in [begin, end) */
    /* SmartIndexT<int>::move until a valid index; if the base stops moving
     * on an invalid one, the old value back.  1 if the value changed. */
    virtual int move(int delta);            /* [3] +0x24 0x00103218 */

    ArcadeControl *owner;                   /* 0x18 */
};

/* vtable 0x00309328.  typeinfo __tfQ29gt2k_menu16SmartCourseIndex 0x001035C0
 * (si, base SmartSkipIndex; __si_type_info 0x004B1460).  Size 0x1C. */
class SmartCourseIndex : public SmartSkipIndex {
public:
    SmartCourseIndex(ArcadeControl *owner) : SmartSkipIndex(owner) {}  /* 0x00103590 */
    /* The course's required level (ProductConfig::courseLevel for the owner's
     * mode name) <= GameStatus::arcadeLevel. */
    virtual int isValid(int i);             /* [2] +0x1C 0x001034A0 */
};

/* vtable 0x00309300.  typeinfo __tfQ29gt2k_menu13SmartCarIndex 0x00103540
 * (si, base SmartSkipIndex; __si_type_info 0x004B1450).  Size 0x20. */
class SmartCarIndex : public SmartSkipIndex {
public:
    SmartCarIndex(ArcadeControl *owner, int player2)
        : SmartSkipIndex(owner), player2(player2) {}           /* 0x00103508 */
    /* Every index while the owner's getPlayerClassName(player) is NULL
     * (the garages); else GameStatus::isArcadeLevelOpen(ProductConfig
     * F_0021D320(i, class)) and db_car_is_locked (T489) of car i's code 0. */
    virtual int isValid(int i);             /* [2] +0x1C 0x001033C0 */

    int player2;                            /* 0x1C 0 player 1 (ArcadeControl::car1), else player 2 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::SmartSkipIndex, 0x1C);
EGCS_SIZE(gt2k_menu::SmartCarIndex, 0x20);
EGCS_SIZE(gt2k_menu::SmartCourseIndex, 0x1C);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu13SmartCarIndex(void);        /* 0x00103540 */
void *__tfQ29gt2k_menu16SmartCourseIndex(void);     /* 0x001035C0 */
void *__tfQ29gt2k_menu14SmartSkipIndex(void);       /* 0x00103640 */
/* SmartIndexT<int>'s (copies 0x001C5D00 T235, 0x001C71E0 T240). */
void *__tfQ29gt2k_menut11SmartIndexT1Zi(void);      /* 0x00103690 */
extern u32 __tiQ29gt2k_menut11SmartIndexT1Zi[2];    /* 0x003532F8 gp-0x46F8 (__user_type_info) */
extern u32 __tiQ29gt2k_menu13SmartCarIndex[3];      /* 0x004B1450 (__si_type_info) */
extern u32 __tiQ29gt2k_menu16SmartCourseIndex[3];   /* 0x004B1460 */
extern u32 __tiQ29gt2k_menu14SmartSkipIndex[3];     /* 0x004B1470 */
}
#endif

#endif /* GAME_T017_SMART_INDEX_H */
