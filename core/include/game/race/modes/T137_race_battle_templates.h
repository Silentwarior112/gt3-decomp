/* T137_race_battle_templates.h - the templates RaceBattleInformationT<N> and
 * RaceBattleT<N> (translation unit T137, unit u046).  T137 emits the
 * instances <2> (RaceSplitBattle), T138 (u047) the instances <8>
 * (RaceLinkBattle).
 *
 * Split from T137_race_split_battle.h (u047's data pass), which includes it
 * where the templates were: retail's T138 has the type names of the <8>
 * instances but none of the <2> ones (tools/tnames.py --unit u047), and the
 * split header instantiated <2> in every object that included it for the
 * templates (EGCS_SIZE(RaceBattleInformationT<2>), RaceSplitBattle's base).
 * The templates themselves emit nothing until a class instantiates them.
 */
#ifndef GAME_T137_RACE_BATTLE_TEMPLATES_H
#define GAME_T137_RACE_BATTLE_TEMPLATES_H

#include "game/game.h"
#include "game/race/core/T132_race_basic.h"           /* base RaceBasic */
#include "game/race/core/T124_race_information.h"     /* base RaceInformation */
#include "game/app/T000_main.h"                 /* RaceEntry (held by value) */

class CarEquipments;                        /* T482 */

/* The race information of a battle of N cars, each with its own entry.
 * vtable <2> 0x00329F40, <8> 0x0032A8E8 (15 slots: [0] its own, the others
 * RaceInformation's).  typeinfo <2> __tft22RaceBattleInformationT1i2
 * 0x00184858, <8> __tft22RaceBattleInformationT1i8 0x00186918 (T138) (si,
 * base RaceInformation).  vptr +0x54.  Size <2> 0x468, <8> 0x1080
 * (RaceLinkBattleInformation's courseCode at +0x1080).  The entries without
 * an i.LINK node (<8>, RaceLinkBattleInformation::setupEntry) are queued in
 * missing[] for RaceInformation::assignCars: <2>::+0x460 and
 * <8>::addMissingEntry settle the layout (u046's pad_end / field_end). */
template <int N>
class RaceBattleInformationT : public RaceInformation {
public:
    /* The entries' CarEquipments (array construction, unwound on
     * exceptions), then every entry empty (no car, colour -1) and no missing
     * entry. */
    RaceBattleInformationT();                           /* <2> 0x001844D8, <8> 0x001864C8 */
    virtual ~RaceBattleInformationT();                  /* [0] +0x0C <2> 0x00184600, <8> 0x001865F0 */
    /* (setEquipments 0x001847E0 is RaceSplitBattleInformation's inline
     * member: T137_race_split_battle.h, u046's data pass.) */
    void addMissingEntry(int i);                        /* <8> 0x00186678 ? name: missing[missingCount++] = i */
    /* (? name, u017) Entry i.  Inline: T054's run_split_battle sets the two
     * entries through it (the information's address, once computed for
     * entry 1, stays in a register: retail spills it and passes it to
     * race_organization_create). */
    RaceEntry *getEntry(int i) { return &entries[i]; }

    RaceEntry entries[N];                   /* 0x058 */
    int       missing[N];                   /* 0x058 + N * 0x200 (<2> 0x458, <8> 0x1058) */
    int       missingCount;                 /* <2> 0x460, <8> 0x1078 */
};

/* The base of the battle modes of N players.  vtable <2> 0x00329CF8, <8>
 * 0x0032A5E0 (56 slots, only the destructor is its own).  typeinfo <2>
 * __tft11RaceBattleT1i2 0x00184790, <8> __tft11RaceBattleT1i8 0x001867A8
 * (T138) (si, base RaceBasic).  No data: size 0xC8C0; its deleting
 * destructor uses RaceBasic's operator delete (0xC8C0). */
template <int N>
class RaceBattleT : public RaceBasic {
public:
    RaceBattleT();                          /* <2> 0x00184688, <8> 0x00186698 */
    virtual ~RaceBattleT();                 /* [0] +0x0C <2> 0x001846C0, <8> 0x001866D0 */
};

#endif /* GAME_T137_RACE_BATTLE_TEMPLATES_H */
