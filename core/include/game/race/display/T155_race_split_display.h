/* T155_race_split_display.h - RaceSplitDisplay, the display of a split
 * screen race (translation unit T155, unit u052), 0x00195B88..0x00196490.
 *
 * Two RaceDisplays, one per screen (RaceSplitBattle +0x2A358, RaceSolitaire
 * +0xCDE0).  The set-up, the update, the draw, the inner panel switch, the
 * organization and the panel switches go to both; the queries and the car
 * numbers to the "target" display (-1 after construction: the race classes
 * store it before use, one screen at a time); drawTarget draws only the
 * target one.  Every call on the displays is a virtual call.
 *
 * T155 also has a copy of __tf15RaceDisplayBase without callers
 * (0x00196450; T154's 0x00195B48 is the one called).
 *
 * Names: u052 (the owner; u046 screen, u047 current for target). */
#ifndef GAME_T155_RACE_SPLIT_DISPLAY_H
#define GAME_T155_RACE_SPLIT_DISPLAY_H

#include "game/game.h"
#include "game/race/display/T154_race_display.h"         /* base RaceDisplayBase, RaceDisplay (held by value) */

class RaceOrganization;                     /* T125_race_organization.h (pointers / references only) */
class GSBuffer;                             /* T514_gs_buffer.h */
class type_info;

/* The typeinfo function (vtable entry 0), declared before the class for
 * GCC 14 only (see T154): egcs generates it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf16RaceSplitDisplay(void);  /* 0x00196400 __si_type_info 0x004B21E0, base __ti15RaceDisplayBase */
}
#endif

#ifdef BOOT_OLD_GCC
/* From here on, with #pragma implementation in T155_race_split_display.cpp:
 * egcs emits the out-of-line copies of RaceSplitDisplay's inline accessors
 * (below) only there, at the end of the object (0x00196398..0x001963F0, in
 * reverse definition order), as retail. */
#pragma interface
#endif

/* vtable 0x0032E620 (18 slots: [16] [17] new), typeinfo
 * __tf16RaceSplitDisplay 0x00196400 (si, base RaceDisplayBase).  vptr +0
 * (RaceDisplayBase's); 0x14900 bytes (RaceSplitBattle: 0x2A358 + 0x14900,
 * rounded up, is its size 0x3EC80; u046's 0x14928 is that rounding).  Built
 * by RaceSplitBattle / RaceSolitaire (F_00183480, F_00186E10). */
class RaceSplitDisplay : public RaceDisplayBase {
public:
    /* The two RaceDisplays (destroyed again, last first, if one throws),
     * target -1, each display's number (1, then 0). */
    RaceSplitDisplay();                     /* 0x00195B88 */
    virtual void virtual_0(RaceOrganization *org, int raceMode, int replay);  /* [ 0] +0x0C 0x00195DD8 both */
    virtual void virtual_1();               /* [ 1] +0x14 0x00195E60 both */
    virtual void virtual_2();               /* [ 2] +0x1C 0x00195EB8 both */
    virtual void virtual_3(GSBuffer &buf);  /* [ 3] +0x24 0x00195F10 both */
    virtual void innerPanel(bool on);       /* [ 4] +0x2C 0x00195FC0 both (index: virtual_4) */
    virtual bool innerPanel() const;        /* [ 5] +0x34 0x00196028 the target's (index: virtual_5) */
    virtual void setOrganization(RaceOrganization &org);   /* [ 6] +0x3C 0x00195D70 both (index: virtual_6) */
    virtual void setPlayerCarNumber(int n); /* [ 7] +0x44 0x001960B8 the target's (index: virtual_7) */
    virtual void setTargetCarNumber(int n); /* [ 8] +0x4C 0x00196070 the target's (index: virtual_8) */
    virtual void virtual_9(int mount);      /* [ 9] +0x54 0x00196100 the target's */
    virtual void setTopLeftPanel(bool on);  /* [10] +0x5C 0x00196190 both (index: virtual_10) */
    virtual void setTopRightPanel(bool on); /* [11] +0x64 0x001961F8 both (index: virtual_11) */
    virtual void setBottomLeftPanel(bool on);   /* [12] +0x6C 0x00196260 both (index: virtual_12) */
    virtual void setBottomRightPanel(bool on);  /* [13] +0x74 0x001962C8 both (index: virtual_13) */
    virtual void setCenterPanel(bool on);   /* [14] +0x7C 0x00196330 both (index: virtual_14) */
    virtual void *getMirrorObject();        /* [15] +0x84 0x00196148 the target's (index: virtual_15) */
    /* The displays, last first, through their virtual destructor. */
    virtual ~RaceSplitDisplay();            /* [16] +0x8C 0x00195CC0 */
    /* The target display's virtual_3 (RaceSplitBattle / RaceSolitaire
     * virtual_45; index: virtual_17). */
    virtual void drawTarget(GSBuffer &buf); /* [17] +0x94 0x00195F78 */

    /* Inline (defined after the class): T155's virtuals reach the displays
     * through them (retail: &displays[i] as an inline call's result, then
     * the virtual call), RaceSplitBattle::render stores target with
     * setTargetDisplay (through the display's own address); the out-of-line
     * copies have no callers. */
    RaceDisplay *getTargetDisplay();        /* 0x00196398 ? name */
    const RaceDisplay *getTargetDisplay() const;    /* 0x001963B8 the same code */
    RaceDisplay *getDisplay(int i);         /* 0x001963D8 ? name (not checked) */
    void setTargetDisplay(int i);           /* 0x001963F0 ? name */

    /*  vptr                                   0x00000 */
    RaceDisplay displays[2];                /* 0x00004 one per screen */
    int         target;                     /* 0x148FC the display the queries go to, -1 (u046 screen, u047
                                               current) */
};
EGCS_SIZE(RaceSplitDisplay, 0x14900);

/* The accessors, defined in reverse address order (egcs emits the #pragma
 * implementation copies in reverse definition order). */
inline void RaceSplitDisplay::setTargetDisplay(int i) { target = i; }                   /* 0x001963F0 */
inline RaceDisplay *RaceSplitDisplay::getDisplay(int i) { return &displays[i]; }        /* 0x001963D8 */
inline const RaceDisplay *RaceSplitDisplay::getTargetDisplay() const { return &displays[target]; }  /* 0x001963B8 */
inline RaceDisplay *RaceSplitDisplay::getTargetDisplay() { return &displays[target]; }  /* 0x00196398 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti16RaceSplitDisplay[3];   /* 0x004B21E0 the __si_type_info (u052 g_004B21E0) */
#endif

#endif /* GAME_T155_RACE_SPLIT_DISPLAY_H */
