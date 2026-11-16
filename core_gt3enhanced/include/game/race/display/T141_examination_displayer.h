/* T141_examination_displayer.h - FrameTimer and ExaminationDisplayer, the
 * base of the examination messages (split from
 * T141_race_time_trial_display.h, which includes this header where the two
 * classes were: its includers see the same).
 *
 * The split is u049's data pass (ref/data/u049/notes.md): retail T146
 * (LicenseExaminationMessages) saw ExaminationDisplayer but none of the race
 * display classes T141's header brings (T154: RaceMiniMap .. RaceDisplayBase);
 * its type names are LicenseExaminationMessages, ExaminationDisplayer,
 * LicensePrize, UnitArena<64>, the prize classes (T145) and
 * RaceValueDisplayBase (T157), so T146's header includes this one and not
 * T141's.
 */
#ifndef GAME_T141_EXAMINATION_DISPLAYER_H
#define GAME_T141_EXAMINATION_DISPLAYER_H

#include "game/game.h"

class GSBuffer;                             /* references only: forward declarations */
class RaceOrganization;                     /* T125 */
class type_info;

/* The typeinfo function, declared before the class (see T132).  Not for
 * egcs: an earlier declaration is the one it uses, and then it does not
 * generate the function (u048 matching; T146 emits its own
 * __tf20ExaminationDisplayer then, as retail). */
#ifndef BOOT_OLD_GCC
extern "C" {
/* the __user_type_info __ti20ExaminationDisplayer gp-0x46B0 (0x00353340,
 * declared at the end).  Also called by __tf26LicenseExaminationMessages
 * (T146). */
const type_info &__tf20ExaminationDisplayer(void);          /* 0x00188FA0 */
}
#endif

/* (? name, u049) A count of frames down to 0: the waits of the examination
 * sequences (RaceTimeTrialDisplay::timer and TimeTrialExaminationMessages::
 * timer (T141); T146's LicenseExaminationMessages::timer, T147's
 * RaceLicenseDisplay::timer).  Inline only, nothing out of line anywhere.
 * The evidence: the displays keep `this + 0xA498' (T141) / `this + 0xA68C'
 * (T147) in a register for every use of their timer (an inline member's
 * `this'), and isOver's two returns are retail's if-converted `li 1;
 * slt 0 < count; movn; beqz' where a plain `count <= 0' is a branch.
 * (Moved to T141's header from T146's by u048, then here by u049.) */
class FrameTimer {
public:
    FrameTimer(int n) : count(n) {}
    void set(int n) { count = n; }
    int  get() { return count; }
    bool isOver() { if (count <= 0) return true; return false; }
    void tick() { count--; }

    int count;                                  /* 0x0 frames left */
};
EGCS_SIZE(FrameTimer, 4);

/* vtable 0x0032B728 (8 slots; a copy 0x0032C7A8 in T146; index.txt names
 * slots 0, 3, 6, 7 TimeTrialExaminationMessages::virtual_N: they are this
 * class's, both of its vtables hold them; slots 1, 2, 4, 5 are named by the
 * symbol table).  typeinfo __tf20ExaminationDisplayer 0x00188FA0 (user).
 * vptr +0x4 (after its data).  Size 8.  Every method is inline: the
 * addresses are T141's copies, then T146's. */
class ExaminationDisplayer {
public:
    ExaminationDisplayer() : active(0) {}   /* finished is left as it is */
    /* The update of a pass: LicenseExaminationMessages (T146) overrides it
     * with its pass sequence (u048 / u049; virtual_0 before the sweep). */
    virtual void updateWhenPassing(RaceOrganization &org) {}    /* [0] +0x0C 0x00188F98, 0x0018BD68 */
    virtual void updateWhenDisqualifying(RaceOrganization &org) {}  /* [1] +0x14 0x00188F90, 0x0018BD60 */
    virtual void updateAlways() {}                              /* [2] +0x1C 0x00188F88, 0x0018BD58 */
    /* The draw of a pass: LicenseExaminationMessages' pass screen; the
     * displays draw it instead of slot 4 when their passed flag is set
     * (u048 / u049; virtual_3 before the sweep). */
    virtual void renderWhenPassing(GSBuffer &buf) {}            /* [3] +0x24 0x00188F80, 0x0018BD50 */
    virtual void renderWhenDisqualifying(GSBuffer &buf) {}      /* [4] +0x2C 0x00188F78, 0x0018BD48 */
    virtual ~ExaminationDisplayer() {}                          /* [5] +0x34 0x00188F48, 0x0018BD18 */
    virtual void activate() { finished = 0; active = 1; }       /* [6] +0x3C 0x00188F38, 0x0018BD08 ? name */
    virtual void deactivate() { active = 0; }                   /* [7] +0x44 0x00188F30, 0x0018BD00 ? name */
    /* (? name, u048) While shown: renderWhenPassing if passed, else
     * renderWhenDisqualifying (through the vtable).  Inline, no copy
     * anywhere: RaceTimeTrialDisplay::virtual_3 expands it (retail keeps
     * `this' in a register for the active test and both calls, and
     * computes the address of its caller's passed flag before the test:
     * the argument's MEM is used in place, loaded after it). */
    void draw(GSBuffer &buf, u8 passed)
    {
        if (active) {
            if (passed)
                renderWhenPassing(buf);
            else
                renderWhenDisqualifying(buf);
        }
    }

    u8 active;                              /* 0x0 the message is shown */
    u8 finished;                            /* 0x1 its sequence has ended */
    /*  vptr                                   0x4 */
};
EGCS_SIZE(ExaminationDisplayer, 8);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* ExaminationDisplayer's __user_type_info {vptr, name}, filled by
 * __tf20ExaminationDisplayer (T141's, or T146's copy); the base node of
 * the examination message classes (the plan had put it in T140). */
extern "C" u32 __ti20ExaminationDisplayer[2];   /* 0x00353340 gp-0x46B0 */
#endif

#endif /* GAME_T141_EXAMINATION_DISPLAYER_H */
