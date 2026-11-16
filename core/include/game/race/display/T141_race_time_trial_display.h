/* T141_race_time_trial_display.h - ExaminationDisplayer,
 * TimeTrialExaminationMessages, RaceTimeTrialDisplay (translation unit
 * T141, unit u048), 0x00188478..0x00188FE0.
 *
 * RaceTimeTrialDisplay is the race display of the time trial.  When the
 * race posts event 0xA (the car is disqualified) it runs a sequence: the
 * BGM event 0x14 (arg 0x20), a 1 s wait, then the "disqualified" message
 * (TimeTrialExaminationMessages: a darkening band and the text fading in,
 * held 3 s, fading out; at most 30 s), a 4 s wait that the pad button 0x40
 * (CONTROL_DECIDE) cuts short, and finally event 1 (the end of the race).
 *
 * ExaminationDisplayer (the messages of an examination: the time trial's
 * disqualification, the licence tests' LicenseExaminationMessages, T146)
 * has only inline methods: T141 and T146 each emit out-of-line copies and a
 * vtable of their own (0x0032B728 here, 0x0032C7A8 in T146).  It and
 * FrameTimer are defined in T141_examination_displayer.h (included below),
 * which T146's header includes instead of this one.
 */
#ifndef GAME_T141_RACE_TIME_TRIAL_DISPLAY_H
#define GAME_T141_RACE_TIME_TRIAL_DISPLAY_H

#include "game/game.h"
#include "game/race/display/T154_race_display.h"         /* base RaceDisplay */

class GSBuffer;                             /* references only: forward declarations */
class RaceOrganization;                     /* T125 */
class Control;                              /* T474 */
class type_info;

/* The typeinfo functions, declared before the classes (see T132).  Not for
 * egcs: an earlier declaration is the one it uses, and then it does not
 * generate the functions (u048 matching; T146 emits its own
 * __tf20ExaminationDisplayer then, as retail). */
#ifndef BOOT_OLD_GCC
extern "C" {
/* (__tf20ExaminationDisplayer 0x00188FA0: T141_examination_displayer.h) */
/* the __si_type_info at 0x004B2010 (base gp-0x46B0) */
const type_info &__tf28TimeTrialExaminationMessages(void);  /* 0x00188EE0 */
/* the __si_type_info at 0x004B2000 (base __ti11RaceDisplay 0x004B21D0) */
const type_info &__tf20RaceTimeTrialDisplay(void);          /* 0x00188E70 */
}
#endif

/* FrameTimer and ExaminationDisplayer (the base of the examination
 * messages): their own header since u049's data pass (retail T146 saw them
 * without this header's race display classes), included here where they
 * were defined. */
#include "game/race/display/T141_examination_displayer.h"

/* The "disqualified" message of a time trial: a band darkening the whole
 * screen height fades in with the message on it, held 3 s, fading out.  vtable
 * 0x0032B6E0 (8 slots: [1] [2] [4] [5] its own).  typeinfo
 * __tf28TimeTrialExaminationMessages 0x00188EE0 (si, base
 * ExaminationDisplayer).  vptr +0x4.  Size 0x1C (RaceTimeTrialDisplay's
 * timer at +0xA47C + 0x1C). */
class TimeTrialExaminationMessages : public ExaminationDisplayer {
public:
    /* timer -1, state 0, no fades, no message. */
    TimeTrialExaminationMessages();                             /* 0x00188478 */
    /* One frame of the fades (1/60 per frame): fade in, hold 180 frames,
     * fade out, then finished (and timer = 120).  org is not used. */
    virtual void updateWhenDisqualifying(RaceOrganization &org);/* [1] +0x14 0x001884F8 */
    virtual void updateAlways();                                /* [2] +0x1C 0x00188620 timer-- */
    /* The band: 0xFFA0A0 (ABGR) with alpha bandFade * 64, blended
     * subtractively (gfx_blend_func 0x42: Cd - Cs * As; in 16:9 4/3 of the
     * screen width, centred), then the message (0x40C0A0, alpha textFade
     * * 128, sliding by 1 - textFade, 1.5x size; x scaled 0.75 in 16:9). */
    virtual void renderWhenDisqualifying(GSBuffer &buf);        /* [4] +0x2C 0x00188630 */
    virtual ~TimeTrialExaminationMessages();                    /* [5] +0x34 0x001884A8 */

    /* Shows msg from the start of the sequence; the first argument (the
     * display's innerPanel()) is not used. */
    void start(int unused, const char *msg);    /* 0x001884D8 */
    /* Inline: T141's out-of-line copies have no callers and sit at the end
     * of the unit, before __tf28TimeTrialExaminationMessages (egcs emits a
     * class's inline members there in reverse declaration order). */
    void setState(u8 s) { state = s; }      /* 0x00188ED8 */
    void nextState() { state++; }           /* 0x00188EC8 */
    u8   getState() { return state; }       /* 0x00188EC0 */

    FrameTimer  timer;                      /* 0x08 frames, counted down by updateAlways */
    u8          state;                      /* 0x0C 0 / 1 fade in, 2 hold, 3 fade out, 4 done */
    f32         bandFade;                   /* 0x10 0..1 the band's alpha */
    f32         textFade;                   /* 0x14 0..1 the message's alpha */
    const char *message;                    /* 0x18 */
};
EGCS_SIZE(TimeTrialExaminationMessages, 0x1C);

/* vtable 0x0032B618 (24 slots: [0] [2] [3] [21] its own, [22] [23] new,
 * the others RaceDisplay's).  typeinfo __tf20RaceTimeTrialDisplay
 * 0x00188E70 (si, base RaceDisplay).  vptr +0 (RaceDisplayBase's).  Size
 * 0xA4AC (RaceTimeTrial: display +0x30214, the conductor +0x3A6C0). */
class RaceTimeTrialDisplay : public RaceDisplay {
public:
    /* RaceDisplay, the message, no input, reset(). */
    RaceTimeTrialDisplay();                                     /* 0x00188828 */
    virtual void virtual_0(RaceOrganization *org, int raceMode, int replay);  /* [ 0] +0x0C 0x00188DE0 RaceDisplay's, reset() */
    /* RaceDisplay's, frameCount++, then phase[0]: 0 takes event 0xA
     * (disqualified) -> 1; 1 the BGM event 0x14 -> 2; 2 the sequence
     * (virtual 22); 3 -> 4; 4 the end (virtual 23). */
    virtual void virtual_2();                                   /* [ 2] +0x1C 0x00188C10 */
    /* RaceDisplay's, then the message while it is active and the inner
     * panel is on (renderWhenPassing if passed, else renderWhenDisqualifying). */
    virtual void virtual_3(GSBuffer &buf);                      /* [ 3] +0x24 0x00188D40 */
    virtual ~RaceTimeTrialDisplay();                            /* [21] +0xB4 0x001888D8 */
    /* ? name.  One frame of the sequence, step phase[1]: 0 timer = 60; 1
     * waits, then shows message 8 for at most 1800 frames; 2 runs it until
     * it is finished, then timer = 240; 3 waits (the pad button 0x40
     * skips), then phase[0] = 3. */
    virtual void updateDisqualify(RaceOrganization &org);       /* [22] +0xBC 0x00188978 */
    /* ? name.  Back to phase 0 and event 1 (the end of the race). */
    virtual void endDisqualify(RaceOrganization &org);          /* [23] +0xC4 0x00188BB8 */

    void reset();                           /* 0x00188930 timer, frameCount, the phases 0; the message off */
    void setPhase(int i, u8 v);             /* 0x00188960 */
    /* Inline: T141's out-of-line copies have no callers and sit at the end
     * of the unit, before __tf20RaceTimeTrialDisplay (reverse declaration
     * order); RaceTimeTrial's constructor expands setInput (the store goes
     * through &display). */
    u8   getPhase(int i) { return phase[i]; }       /* 0x00188E58 */
    void setSubPhase(u8 v) { phase[1] = v; }        /* 0x00188E48 */
    void nextSubPhase() { phase[1]++; }             /* 0x00188E30 */
    u8   getSubPhase() { return phase[1]; }         /* 0x00188E20 */
    void setInput(Control *c) { input = c; }        /* 0x00188E10 */

    TimeTrialExaminationMessages ttem;      /* 0xA47C the "disqualified" message */
    FrameTimer timer;                       /* 0xA498 frames (every use through its inline members) */
    int      frameCount;                    /* 0xA49C */
    u8       phase[2];                      /* 0xA4A0 [0] the sequence, [1] its step */
    Control *input;                         /* 0xA4A4 the pad (RaceSolitaire::input0) */
    /* Draw ttem with renderWhenPassing instead of renderWhenDisqualifying
     * (RaceLicenseDisplay's name; a time trial only clears it; u048 fA4A8). */
    u8       passed;                        /* 0xA4A8 */
};
EGCS_SIZE(RaceTimeTrialDisplay, 0xA4AC);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* (__ti20ExaminationDisplayer 0x00353340: T141_examination_displayer.h) */
/* The __si_type_info {vptr, name, base} the other two __tf functions fill
 * (u048 g_004B2010 / g_004B2000). */
extern "C" u32 __ti28TimeTrialExaminationMessages[3];  /* 0x004B2010 */
extern "C" u32 __ti20RaceTimeTrialDisplay[3];   /* 0x004B2000 */
#endif

#endif /* GAME_T141_RACE_TIME_TRIAL_DISPLAY_H */
