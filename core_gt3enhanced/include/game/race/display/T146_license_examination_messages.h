/* T146_license_examination_messages.h - LicenseExaminationMessages (the
 * result screen of a licence test), LicensePrize, ExamMessageLine and the
 * text helpers (translation unit T146, unit u049), 0x0018A798..0x0018BE40.
 *
 * LicenseExaminationMessages is run by RaceLicenseDisplay (T147, +0xA47C):
 *   - passed (setup(kind > 0, ...), then updateWhenPassing / renderWhenPassing every
 *     frame): a dark screen fades in, the trophy's race events and
 *     GameStatus flag are posted (if `report'), then the trophy model (gold
 *     for kinds 1 / 2, silver 3, bronze 4 / 5), its rank name and the
 *     time line (the result time) fade in;
 *   - failed (setup(-1, ...), then updateWhenDisqualifying /
 *     renderWhenDisqualifying): a darkening band (blended subtractively,
 *     gfx_blend_func 0x42: Cd - Cs * As, u048) and the failure text ("time
 *     over" with the time line / "disqualified") fade in, are held 180
 *     frames and fade out.
 * The three trophy models are loaded once by the constructor into a unit
 * of the car model arena g_002C2F50 (UnitArena<64>, T180; 0x32000 bytes
 * each, kept).
 *
 * T146 also holds out-of-line copies of ExaminationDisplayer's inline
 * methods (0x0018BD00..0x0018BD68, unused; T141's are the ones in the
 * vtables), of __tf20ExaminationDisplayer (0x0018BD70) and of Color8's
 * set(u32) (0x0018BE00, the only copy; u049's "constructor Color8(u32)",
 * h41) and setters (0x0018BE20..0x0018BE38): declared with their classes
 * (T141, T188). */
#ifndef GAME_T146_LICENSE_EXAMINATION_MESSAGES_H
#define GAME_T146_LICENSE_EXAMINATION_MESSAGES_H

#include "game/game.h"
#include "game/race/modes/T145_mini_prize.h"               /* base MiniPrize */

class GSBuffer;                                 /* T514 (references only) */
class RaceOrganization;                         /* T125 (references only) */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
/* (__si_type_info 0x004B2080, base ExaminationDisplayer 0x00353340) */
const type_info &__tf26LicenseExaminationMessages(void);   /* 0x0018BCB0 */
/* (__si_type_info 0x004B2090, base MiniPrize 0x004B2070) */
const type_info &__tf12LicensePrize(void);                 /* 0x0018BDB0 */
}
#endif

/* FrameTimer (u049's class: the timers of LicenseExaminationMessages and
 * RaceLicenseDisplay) is defined in T141_examination_displayer.h, which
 * this header includes: T141's RaceTimeTrialDisplay and
 * TimeTrialExaminationMessages hold one too (u048). */

/* (? name, u049) A line that fades in and out
 * (LicenseExaminationMessages+0x28): a time in ms, as race_format_time
 * (T158, 0x0019A7F0) writes it: the organization's result time
 * (RaceOrganization::result.time).  Not polymorphic, 0xC bytes.  The
 * constructor is inline (expanded in LicenseExaminationMessages'). */
struct ExamMessageLine {
    ExamMessageLine() : time(0), alpha(0.0f), fade(0) {}
    void init(int time, u8 on);                 /* 0x0018ACA8 show = on, time = time, alpha 0, still */
    void fadeIn();                              /* 0x0018ACC0 fade = 1 */
    void fadeOut();                             /* 0x0018ACD0 fade = 2 */
    void update();                              /* 0x0018ACE0 one frame of the fade (1/60) */
    /* While visible and used: the time centred at (x, y), 1.25x, in
     * (r, g, b) with alpha * 128, with its shadow.  buf is not used. */
    void draw(GSBuffer &buf, u8 r, u8 g, u8 b, int x, int y);  /* 0x0018AD58 */

    int time;                                   /* 0x0 the time in ms (race_format_time's argument; u049 msgId) */
    f32 alpha;                                  /* 0x4 0..1 */
    s8  fade;                                   /* 0x8 0 still, 1 fading in, 2 fading out */
    u8  show;                                   /* 0x9 the line is used */
};
EGCS_SIZE(ExamMessageLine, 0xC);

/* vtable 0x0032C7F0 (5 slots; 1 its own), typeinfo __tf12LicensePrize
 * 0x0018BDB0.  One trophy of a licence result.  No data of its own: vptr
 * +0x90 (PrizeBase's); 0x9C bytes (the stride of
 * LicenseExaminationMessages::prizes). */
class LicensePrize : public MiniPrize {
public:
    LicensePrize();                             /* 0x0018AEB0 */
    virtual ~LicensePrize();                    /* [1] +0x14 0x0018AEE8 */
    /* MiniPrize::render, then puts back the render states the 2D drawing
     * expects. */
    void render(GSBuffer &buf, f32 alpha);      /* 0x0018AF10 */
};
EGCS_SIZE(LicensePrize, 0x9C);

/* ExaminationDisplayer and FrameTimer: included after LicensePrize, whose
 * class the original defined first (egcs processes the classes in reverse
 * definition order: retail T146 emits LicenseExaminationMessages',
 * ExaminationDisplayer's, then LicensePrize's inline copies / typeinfo, and
 * its type names LicenseExaminationMessages, ExaminationDisplayer,
 * LicensePrize).  Their own header, not T141_race_time_trial_display.h:
 * retail T146 saw none of the race display classes that one reads (T154). */
#include "game/race/display/T141_examination_displayer.h"    /* base ExaminationDisplayer, FrameTimer */

/* vtable 0x0032C758 (9 slots; 0..5 and 7 its own, 8 new; 6 is
 * ExaminationDisplayer's activate), typeinfo __tf26LicenseExaminationMessages
 * 0x0018BCB0.  vptr +0x04 (ExaminationDisplayer's); 0x210 bytes
 * (RaceLicenseDisplay+0xA47C .. +0xA68C). */
class LicenseExaminationMessages : public ExaminationDisplayer {
public:
    /* The prizes, then the three models loaded (kept, hidden) into a block
     * of g_002C2F50; current = &prizes[0]. */
    LicenseExaminationMessages();               /* 0x0018AF98 */
    virtual void updateWhenPassing(RaceOrganization &org);      /* [0] +0x0C 0x0018B510 the pass sequence */
    virtual void updateWhenDisqualifying(RaceOrganization &org);/* [1] +0x14 0x0018B3B0 the fail sequence */
    virtual void updateAlways();                                /* [2] +0x1C 0x0018B758 timer-- */
    virtual void renderWhenPassing(GSBuffer &buf);              /* [3] +0x24 0x0018B798 the pass screen */
    virtual void renderWhenDisqualifying(GSBuffer &buf);        /* [4] +0x2C 0x0018BA48 the fail screen */
    /* Gives the model block back (deleting form: operator delete(this)). */
    virtual ~LicenseExaminationMessages();                      /* [5] +0x34 0x0018B1F8 */
    virtual void deactivate();                                  /* [7] +0x44 0x0018B380 */
    /* Starts the sequence: kind > 0 passed with that trophy, -1 failed;
     * time / showTime the time line (the organization's result time);
     * report: post the trophy's events and flags; text: the failure text. */
    virtual void setup(int kind, int time, bool report, const char *text, bool showTime);  /* [8] +0x4C 0x0018B2C0 */

    /* Inline (as TimeTrialExaminationMessages', T141): T146's out-of-line
     * copies have no callers and sit at the end of the unit, before
     * __tf26LicenseExaminationMessages (reverse declaration order). */
    void setState(u8 s) { state = s; }          /* 0x0018BCA8 */
    void nextState() { state++; }               /* 0x0018BC98 */
    u8   getState() { return state; }           /* 0x0018BC90 */

    FrameTimer      timer;                      /* 0x008 frames, counted down by updateAlways */
    u8              state;                      /* 0x00C the step of the sequence */
    f32             fade0;                      /* 0x010 pass: the dark screen; fail: the band */
    f32             fade1;                      /* 0x014 pass: the rank name; fail: the text */
    f32             fade2;                      /* 0x018 pass: the trophy */
    int             kind;                       /* 0x01C the trophy: 1, 2 gold, 3 silver, 4, 5 bronze; -1 failed */
    const char     *text;                       /* 0x020 the failure text */
    u8             *buffer;                     /* 0x024 the unit of g_002C2F50 holding the three models */
    ExamMessageLine line;                       /* 0x028 */
    LicensePrize    prizes[3];                  /* 0x034 gold, silver, bronze */
    LicensePrize   *current;                    /* 0x208 the one shown */
    u8              report;                     /* 0x20C post the trophy's events / flags */
};
EGCS_SIZE(LicenseExaminationMessages, 0x210);

/* The text with a black drop shadow in the race font (g_002F1968): the
 * shadow (0, 0, 0, a) at (x, y), the text (r, g, b, a) at (x + d, y - d). */
void draw_shadow_text(const char *text, u8 r, u8 g, u8 b, u8 a, int x, int y, int d);  /* 0x0018A798 */
/* The x scale of the race texts: 0.75 in the 16:9 mode (g_002F424C),
 * else 1.0. */
f32  F_0018A938(void);                          /* 0x0018A938 */
/* The failure text: in rgb | alpha << 24, centred on x 0x140 at y 0xE0 + d
 * and, while d = (int)(slide * 30) is not 0, again at 0xE0 - d (two copies
 * sliding together).  Nothing for a NULL text.  Also used by
 * TimeTrialExaminationMessages (T141).  slide before alpha (EABI: the
 * same registers either way; u048's caller loads $f12 before $a2, the
 * argument order, only so). */
void F_0018ABC8(const char *text, u32 rgb, f32 slide, u8 alpha);  /* 0x0018ABC8 */
/* The enhanced engine: the same centred on (w / 2, h / 2) (Lupo 2's
 * version L2 0x00188870 takes the buffer's size; hud.md C20: the Pod's
 * split screen).  F_0018ABC8 is it with (640, 448). */
void exam_draw_failure_text(const char *text, u32 rgb, f32 slide, u8 alpha, int w, int h);

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_info nodes {vptr, name, base} the typeinfo functions fill
 * (u049: g_004B2080 / g_004B2090). */
extern "C" u32 __ti26LicenseExaminationMessages[3]; /* 0x004B2080 */
extern "C" u32 __ti12LicensePrize[3];               /* 0x004B2090 */
#endif

#endif /* GAME_T146_LICENSE_EXAMINATION_MESSAGES_H */
