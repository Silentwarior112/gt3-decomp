/* T147_race_license_display.h - RaceLicenseDisplay, the race display of a
 * licence test (translation unit T147, unit u049), 0x0018BE40..0x0018C7E8.
 *
 * RaceDisplay (T154) plus the result sequence of LicenseExaminationMessages
 * (T146); none of the race panels (the panel setters do nothing).  Held by
 * value in RaceLicense (+0x229C8).  update (virtual_2) runs phase[0]:
 *   0  racing: event 9 (the test is failed) -> 3; a result > 0 in the
 *      organization (the trophy) -> 1;
 *   1, 2  passed: the pass sequence, then updatePass every frame;
 *   3, 4  failed: the BGM event 0x14, then updateFail every frame ("time
 *      over" and the time when the result is 0, else "disqualified");
 *   5, 6  the end: endExamination (event 1: the race ends).
 * phase[1] is the step of updatePass / updateFail.
 *
 * The virtual methods keep RaceDisplay's slot names. */
#ifndef GAME_T147_RACE_LICENSE_DISPLAY_H
#define GAME_T147_RACE_LICENSE_DISPLAY_H

#include "game/game.h"
#include "game/race/display/T154_race_display.h"             /* base RaceDisplay */
#include "game/race/display/T146_license_examination_messages.h" /* LicenseExaminationMessages (held by value) */

class GSBuffer;                                 /* T514 (references only) */
class Control;                                  /* T474 (pointers only) */
class RaceOrganization;                         /* T125 (pointers only) */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* (__si_type_info 0x004B20A0, base RaceDisplay 0x004B21D0) */
extern "C" const type_info &__tf18RaceLicenseDisplay(void);    /* 0x0018C798 */
#endif

/* vtable 0x0032C9F0 (22 slots; 0, 2, 3, 10..14, 16, 20 and 21 its own),
 * typeinfo __tf18RaceLicenseDisplay 0x0018C798.  vptr +0x00 (RaceDisplay's);
 * 0xA69C bytes (RaceLicense+0x229C8 .. +0x2D064). */
class RaceLicenseDisplay : public RaceDisplay {
public:
    /* The messages, timer -1, then reset(). */
    RaceLicenseDisplay();                       /* 0x0018BE40 */
    /* RaceDisplay's (the arguments passed on), then reset(). */
    virtual void virtual_0(RaceOrganization *org, int raceMode, int replay);  /* [ 0] +0x0C 0x0018C660 */
    virtual void virtual_2(f32 dt);             /* [ 2] +0x1C 0x0018C458 update: the phases (dt: hud.md C19) */
    /* RaceDisplay's draw, then the messages (renderWhenPassing when passed, else
     * slot 4). */
    virtual void virtual_3(GSBuffer &buf);      /* [ 3] +0x24 0x0018C5C0 */
    virtual void setTopLeftPanel(bool on);      /* [10] +0x5C 0x0018C730 nothing */
    virtual void setTopRightPanel(bool on);     /* [11] +0x64 0x0018C728 nothing */
    virtual void setBottomLeftPanel(bool on);   /* [12] +0x6C 0x0018C720 nothing */
    virtual void setBottomRightPanel(bool on);  /* [13] +0x74 0x0018C718 nothing */
    virtual void setCenterPanel(bool on);       /* [14] +0x7C 0x0018C710 nothing */
    /* (RaceDisplay's is not called.)  Only when replay == 1 (RaceDisplay
     * +0xC; u049: mode): the BGM event 0x14 (arg 0x13) and event 8. */
    virtual void virtual_16(RaceOrganization *org, int car, int myCar);   /* [16] +0x8C 0x0018C690 */
    /* RaceDisplay's (the time on the event board), only when replay != 0. */
    virtual void showLapTime(int time);         /* [20] +0xAC 0x0018C6E8 */
    virtual ~RaceLicenseDisplay();              /* [21] +0xB4 0x0018BEF8 */

    /* The messages are not shown, timer 0, phase 0 / step 0. */
    void reset();                               /* 0x0018BF50 */
    void setPhase(int i, u8 v);                 /* 0x0018BF80 */
    void updatePass(RaceOrganization *org);     /* 0x0018BF98 */
    void updateFail(RaceOrganization *org);     /* 0x0018C1E0 */
    /* (? name) event 1: the race ends. */
    void endExamination(RaceOrganization *org); /* 0x0018C400 */
    /* Inline (as RaceTimeTrialDisplay's, T141): T147's out-of-line copies
     * have no callers and sit at the end of the unit, before
     * __tf18RaceLicenseDisplay (egcs emits a class's inline members there
     * in reverse declaration order); RaceLicense's constructor expands
     * setInput. */
    u8   getPhase(int i) { return phase[i]; }   /* 0x0018C780 */
    void setSubPhase(u8 v) { phase[1] = v; }    /* 0x0018C770 */
    void nextSubPhase() { phase[1]++; }         /* 0x0018C758 */
    u8   getSubPhase() { return phase[1]; }     /* 0x0018C748 */
    void setInput(Control *c) { input = c; }    /* 0x0018C738 */

    LicenseExaminationMessages msgs;            /* 0xA47C */
    FrameTimer timer;                           /* 0xA68C frames (T146) */
    u8       phase[2];                          /* 0xA690 [0] the phase, [1] its step */
    Control *input;                             /* 0xA694 the pad (RaceSolitaire::input0) */
    u8       passed;                            /* 0xA698 draw msgs with slot 3 (passed) instead of 4 */
};
/* The enhanced engine: RaceDisplay 0x46C longer (0xA8E8: the flags and the
 * hud's Lupo 2 parts): its own members sit 0x46C later than shown (GT3
 * 0xA69C). */
EGCS_SIZE(RaceLicenseDisplay, 0xAB08);

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* The __si_type_info {vptr, name, base} __tf18RaceLicenseDisplay fills
 * (u049: g_004B20A0). */
extern "C" u32 __ti18RaceLicenseDisplay[3];     /* 0x004B20A0 */
#endif

#endif /* GAME_T147_RACE_LICENSE_DISPLAY_H */
