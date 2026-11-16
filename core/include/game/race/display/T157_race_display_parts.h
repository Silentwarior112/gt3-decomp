/* T157_race_display_parts.h - the elements of the race head-up display
 * (translation unit T157, unit u054), 0x00196910..0x0019A148.
 *
 * Every element is a RaceDisplayObjectBase (T156: a rectangle x, y, w, h,
 * an alignment, an enabled flag, an image of the race display's pack, vptr
 * +0x14): lap counter, rank, gear (7-segment font "gearr#1"), digital
 * speedometer, odometer, time / speed values with a label, the lap time
 * list, timed messages and the 8-line event board, the VSC / TCS / brake /
 * oil lamps, back panels, the accel / brake bars, the steering wheel, the
 * tyre wear gauge, the car icon, the replay-mode banner; plus the faded
 * "gt_logo_mini" watermark (RaceDisplayFader, not polymorphic).  RaceDisplay
 * (T154) holds all of them by value.  update is the per-frame update
 * (RaceDisplay::virtual_2 calls it), draw the draw (RaceDisplay::
 * virtual_3 calls it on every enabled object).  Text goes through the race
 * display's font g_002F1968 (T496), images through the T158 helpers.
 *
 * Inline in the original: the destructors (RaceDisplay's destructor
 * 0x00191300 expands every one of them: the vptr store where there is one,
 * then ~RaceDisplayObjectBase; none has a caller) and the small accessors.
 * egcs emitted their out-of-line copies at the end of the TU, class by class
 * (accessors, __tf function, destructor).  Only four destructors store
 * their class's vptr (RaceBackPanel, RaceABMonitor, RaceSteeringDisplay,
 * RaceTireWearDisplay): they are user-declared, written inline below.  The
 * other fourteen store none, in their copies and where RaceDisplay's
 * destructor expands them: they are the implicit destructors and are not
 * declared (egcs checked).  The accessors are declared in the reverse of
 * their addresses: egcs writes a class's in-class inline members in the
 * reverse of their declaration order (DATA.md; u054 data pass: T157's frame
 * info has retail's function order only so).  The order changes no layout:
 * none of them is virtual.
 *
 * The classes are defined in the original's order (u054 matching pass):
 * egcs writes T157's vtables, type names and inline copies in reverse
 * definition order, and retail's run ReplayMode, CarIcon, Speed, TireWear ..
 * Odometer, DigitalSpeedmeter, Rank, Lap (RaceGearDisplay after
 * RaceOdometer, RaceSpeedDisplay after RaceTireWearDisplay).
 */
#ifndef GAME_T157_RACE_DISPLAY_PARTS_H
#define GAME_T157_RACE_DISPLAY_PARTS_H

#include "game/game.h"
#include "game/race/display/T156_race_display_object.h"     /* base RaceDisplayObjectBase */

struct Tex1Image;                               /* G001 (pointers only) */
struct CarIcon;                                 /* T184 */
class type_info;                                /* T548 */

/* A blink lasts this many frames, half of them hidden ((blink / 12) & 1):
 * RaceMessageDisplay / RaceCarIconDisplay::setBlink. */
#define RACE_BLINK_FRAMES 24

/* The typeinfo functions (vtable entry 0), each an __si_type_info on its
 * base's node.  Declared before the classes, for GCC 14 only: egcs declares
 * a polymorphic class's __tf<mangled> itself (C++ linkage) when the class is
 * complete and rejects a later extern "C" declaration; an earlier one is the
 * declaration it uses, and then it never defines the function (u054
 * matching pass: T157 then emitted none of its 18 __tf functions). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf21RaceReplayModeDisplay(void);         /* 0x001996C0 */
const type_info &__tf18RaceCarIconDisplay(void);            /* 0x00199780 */
const type_info &__tf16RaceSpeedDisplay(void);              /* 0x001997F8 base RaceValueDisplayBase */
const type_info &__tf19RaceTireWearDisplay(void);           /* 0x001998D8 */
const type_info &__tf19RaceSteeringDisplay(void);           /* 0x00199958 */
const type_info &__tf13RaceABMonitor(void);                 /* 0x001999E0 */
const type_info &__tf13RaceBackPanel(void);                 /* 0x00199A58 */
const type_info &__tf14RaceIndicators(void);                /* 0x00199AC8 */
const type_info &__tf16RaceEventDisplay(void);              /* 0x00199B60 */
const type_info &__tf18RaceMessageDisplay(void);            /* 0x00199CC0 */
const type_info &__tf19RaceLapTimesDisplay(void);           /* 0x00199D40 */
const type_info &__tf15RaceTimeDisplay(void);               /* 0x00199DC0 base RaceValueDisplayBase */
const type_info &__tf20RaceValueDisplayBase(void);          /* 0x00199E30 */
const type_info &__tf15RaceGearDisplay(void);               /* 0x00199ED8 */
const type_info &__tf12RaceOdometer(void);                  /* 0x00199F50 */
const type_info &__tf21RaceDigitalSpeedmeter(void);         /* 0x00199FD8 */
const type_info &__tf15RaceRankDisplay(void);               /* 0x0019A058 */
const type_info &__tf14RaceLapDisplay(void);                /* 0x0019A0D8 */
}
#endif

/* ? name (u051's).  Not polymorphic, 0x10 bytes (RaceDisplay+0x50): the
 * "gt_logo_mini" watermark at (620, 420), alignment 5 / 1.  update: wait
 * 3 s, fade in over 2 s, stay 30 s, fade out over 2 s, wait 30 s, again
 * (states 0..5, jump table 0x0032E820). */
struct RaceDisplayFader {
    RaceDisplayFader();                     /* 0x00196910 all zero */
    void reset();                           /* 0x00196928 alpha, state, timer = 0 (the logo is kept) */
    void update();                          /* 0x00196938 */
    void draw();                            /* 0x00196A38 */

    int        state;                       /* 0x00 */
    int        timer;                       /* 0x04 frames */
    f32        alpha;                       /* 0x08 */
    Tex1Image *logo;                        /* 0x0C "gt_logo_mini", looked up on the first draw */
};
EGCS_SIZE(RaceDisplayFader, 0x10);

/* vtable 0x0032EF20 (10 slots: [0] and [4] its own, the others
 * RaceDisplayObjectBase's).  typeinfo __tf14RaceLapDisplay 0x0019A0D8 (si,
 * storage 0x004B22F0).  "<lap>/<laps>" (or "<lap>") in "sw_20" ending at x,
 * the word "Lap" (g_002C43E8[36]; u054 g_002C4478[0], "sw_16") in front.  Size 0x24
 * (RaceDisplay+0x60). */
class RaceLapDisplay : public RaceDisplayObjectBase {
public:
    RaceLapDisplay();                       /* 0x00196AB0 */
    /* [0] +0x0C 0x0019A128 ~RaceLapDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void draw();               /* [4] +0x2C 0x00196B00 draw */

    void setTotalLaps(int n) { laps = n; }  /* 0x0019A0D0 ? name (inline in the original) */
    void setCurrentLaps(int n) { lap = n; } /* 0x0019A0C8 (inline in the original) */

    int lap;                                /* 0x18 the current lap (0: hidden; shown up to 9999) */
    int laps;                               /* 0x1C the number of laps (0: unknown) */
    f32 labelWidth;                         /* 0x20 the width of "Lap", measured once (0: not yet) */
};
EGCS_SIZE(RaceLapDisplay, 0x24);

/* vtable 0x0032EEC8 (10 slots: [0], [4]).  typeinfo __tf15RaceRankDisplay
 * 0x0019A058 (si, storage 0x004B2250).  "1st".. (race_rank_name, T158) in
 * the font "rank#1" at the object's scale, with a drop shadow (0x60000000,
 * 4 units down-right).  Size 0x20 (RaceDisplay+0x84). */
class RaceRankDisplay : public RaceDisplayObjectBase {
public:
    RaceRankDisplay();                      /* 0x00196CC0 scale 1.0 */
    /* [0] +0x0C 0x0019A0A8 ~RaceRankDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void draw();               /* [4] +0x2C 0x00196D08 draw */

    void setRank(int r) { rank = r; }       /* 0x0019A050 ? name (inline in the original) */
    void setScale(f32 s) { scale = s; }     /* 0x0019A048 (inline in the original) */

    int rank;                               /* 0x18 1..12 */
    f32 scale;                              /* 0x1C the character scale */
};
EGCS_SIZE(RaceRankDisplay, 0x20);

/* vtable 0x0032EE70 (10 slots: [0], [4]).  typeinfo
 * __tf21RaceDigitalSpeedmeter 0x00199FD8 (si, storage 0x004B2290).  "%3d"
 * of |speed| right-aligned in the font "speed" (g_00352088), then the speed
 * unit ("sw_12", T518) at the unit offset.  Size 0x24 (RaceDisplay+0xA17C). */
class RaceDigitalSpeedmeter : public RaceDisplayObjectBase {
public:
    RaceDigitalSpeedmeter();                /* 0x001970A0 */
    /* [0] +0x0C 0x0019A028 ~RaceDigitalSpeedmeter: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void draw();               /* [4] +0x2C 0x001970E0 draw (stores |speed| back) */

    void setSpeed(int v) { speed = v; }     /* 0x00199FD0 ? name (inline in the original) */
    void setUnitOffset(int dx, int dy) { unitX = dx; unitY = dy; } /* 0x00199FC0 ? name (inline in the original) */

    int speed;                              /* 0x18 */
    int unitX;                              /* 0x1C the unit, relative to the number */
    int unitY;                              /* 0x20 */
};
EGCS_SIZE(RaceDigitalSpeedmeter, 0x24);

/* vtable 0x0032EE18 (10 slots: [0], [4]).  typeinfo __tf12RaceOdometer
 * 0x00199F50 (si, storage 0x004B22A0).  A zero-padded number in the font
 * "odo#1" (g_003520A8), colour 0x80C0C062.  Size 0x28 (RaceDisplay+0xA10C,
 * +0xA134). */
class RaceOdometer : public RaceDisplayObjectBase {
public:
    RaceOdometer();                         /* 0x001971E8 value 0, setDigits(6) */
    /* [0] +0x0C 0x00199FA0 ~RaceOdometer: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void draw();               /* [4] +0x2C 0x00197258 draw */

    void setDigits(int n);                  /* 0x00197340 format "%0<n>d", width = 0 */
    void setValue(int v) { value = v; }     /* 0x00199F48 ? name (inline in the original) */

    int  value;                             /* 0x18 */
    f32  width;                             /* 0x1C the text width, measured once (0: not yet) */
    char format[8];                         /* 0x20 "%0<n>d" */
};
EGCS_SIZE(RaceOdometer, 0x28);

/* vtable 0x0032EDC0 (10 slots: [0], [3], [4]).  typeinfo
 * __tf15RaceGearDisplay 0x00199ED8 (si, storage 0x004B2230).  The gear in
 * the 7-segment font "gearr#1" (g_00352060) centred in the rectangle, over
 * dim ghost segments: 'R' below 1 (red), else the digit (amber; halved
 * colour when not bright); the suggested gear (" > " and '@' + n, red,
 * blinking when suggestBlink); the shift lamp ':' at the lamp offset (red
 * while lampBlink > 0).  Size 0x30 (RaceDisplay+0xA1A0). */
class RaceGearDisplay : public RaceDisplayObjectBase {
public:
    RaceGearDisplay();                      /* 0x00196E18 lampX -0x18, bright 1 */
    /* [0] +0x0C 0x00199F28 ~RaceGearDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void update();               /* [3] +0x24 0x00196E70 update: both blink phases += 42
                                               (lampBlink held at 0 without a warning) */
    virtual void draw();               /* [4] +0x2C 0x00196E98 draw */

    /* The inline accessors (out-of-line copies, no callers). */
    void setGear(int g) { gear = g; }       /* 0x00199ED0 ? name */
    void setSuggestedGear(int g) { suggestedGear = g; } /* 0x00199EC8 */
    void setShiftWarning(u8 on) { shiftWarning = on; } /* 0x00199EC0 ? name */
    void setLampOffset(int dx, int dy) { lampX = dx; lampY = dy; } /* 0x00199EB0 ? name */
    void setBright(u8 on) { bright = on; }  /* 0x00199EA8 ? name */
    void setSuggestBlink(bool on) { suggestBlink = on; } /* 0x00199EA0 ? name (sb +0x2A) */

    int  gear;                              /* 0x18 < 1: shown as 'R' */
    int  suggestedGear;                     /* 0x1C 0: none */
    int  lampX;                             /* 0x20 the shift lamp, relative to the gear (-0x18) */
    int  lampY;                             /* 0x24 */
    u8   shiftWarning;                      /* 0x28 the shift lamp blinks */
    u8   bright;                            /* 0x29 0: the gear at half intensity */
    /* an 8-bit bool bit-field (egcs's bool is 4 bytes): draw's !suggestBlink
     * is xori 1, a bool's not (a u8 gives sltu) */
    bool suggestBlink : 8;                  /* 0x2A the suggested gear blinks (sb / lbu) */
    s8   lampBlink;                         /* 0x2B += 42 per frame: lit while > 0 */
    s8   suggestPhase;                      /* 0x2C += 42 per frame: lit while > 0 */
    u8   unk_2D[3];                         /* 0x2D */
};
EGCS_SIZE(RaceGearDisplay, 0x30);

/* vtable 0x0032ED68 (10 slots: [0] its own, [4] __pure_virtual).  typeinfo
 * __tf20RaceValueDisplayBase 0x00199E30 (si, storage 0x004B1260).  A value
 * (drawn by the derived class) with a label in front of it, and an optional
 * mark image between the two.  0x44 bytes. */
class RaceValueDisplayBase : public RaceDisplayObjectBase {
public:
    RaceValueDisplayBase();                 /* 0x00197378 color 0x801880D0, setLabel("") */
    /* [0] +0x0C 0x00199E80 ~RaceValueDisplayBase: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void draw() = 0;           /* [4] +0x2C __pure_virtual (draw) */

    void setLabel(const char *text);        /* 0x001973F8 race_copy_string(label, text, 0x20), labelWidth = 0 */

    u32        color;                       /* 0x18 */
    Tex1Image *mark;                        /* 0x1C an image between the value and the label (u051: the
                                               "mark_1st".. medal), NULL: none */
    f32        labelWidth;                  /* 0x20 measured once (0: not yet) */
    char       label[0x20];                 /* 0x24 */
};
EGCS_SIZE(RaceValueDisplayBase, 0x44);

/* vtable 0x0032ED10 (10 slots: [0], [4]).  typeinfo __tf15RaceTimeDisplay
 * 0x00199DC0 (si on RaceValueDisplayBase, storage 0x004B21F0).  The time
 * (race_format_time, "sw_16") right-aligned, shifted by -/+106.667 for
 * alignX 4 / 5 on the wide screen, the mark and the label ("sw_12") left
 * of a column at least 90 wide.  Size 0x48 (RaceDisplay+0xEC ..). */
class RaceTimeDisplay : public RaceValueDisplayBase {
public:
    RaceTimeDisplay();                      /* 0x00197428 time -1 */
    /* [0] +0x0C 0x00199E10 ~RaceTimeDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void draw();               /* [4] +0x2C 0x00197468 draw */

    void setTime(int t) { time = t; }       /* 0x00199DB8 (inline in the original) */
    int getImmediateTimeValue() const { return time; } /* 0x00199DB0 (inline in the original) */

    int time;                               /* 0x44 ms, -1: none */
};
EGCS_SIZE(RaceTimeDisplay, 0x48);

/* vtable 0x0032ECB8 (10 slots: [0], [4]).  typeinfo
 * __tf19RaceLapTimesDisplay 0x00199D40 (si, storage 0x004B2270).  The last
 * `lines' lap times ("sw_16", right-aligned, lineSpacing character heights
 * apart, "-" for lines past the stored ones) and their lap numbers ("sw_12")
 * in a column to the left.  Size 0x48 (RaceDisplay+0xA4). */
class RaceLapTimesDisplay : public RaceDisplayObjectBase {
public:
    RaceLapTimesDisplay();                  /* 0x001976D8 reset(), lines 3, lineSpacing 0.8 */
    /* [0] +0x0C 0x00199D90 ~RaceLapTimesDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void draw();               /* [4] +0x2C 0x00197760 draw */

    void reset();                           /* 0x00197750 stored = lapCount = 0 */
    void addLapTime(int time);              /* 0x00197A10 ? name: appended, the oldest dropped when 8 are stored */
    void setCurrentLapTime(int time);       /* 0x00197A80 ? name: the slot after the stored ones, not counted */
    void setLines(int n) { lines = n; }     /* 0x00199D38 ? name (inline in the original) */
    void setLineSpacing(f32 s) { lineSpacing = s; } /* 0x00199D30 ? name (inline in the original) */

    int lapCount;                           /* 0x18 laps added in all */
    int stored;                             /* 0x1C entries of times (at most 8) */
    int lines;                              /* 0x20 lines shown (3) */
    int times[8];                           /* 0x24 ms, the most recent last */
    f32 lineSpacing;                        /* 0x44 in character heights (0.8) */
};
EGCS_SIZE(RaceLapTimesDisplay, 0x48);

/* vtable 0x0032EC60 (10 slots: [0], [3], [4]).  typeinfo
 * __tf18RaceMessageDisplay 0x00199CC0 (si, storage 0x004B22E0).  A text (or
 * the image <name> for a text "@<name>") shown for a duration after a
 * delay, with fades and blinking; squeezed into w when w > 0.  Size 0x88
 * (RaceDisplay+0xCB4 ..; 8 in RaceEventDisplay). */
class RaceMessageDisplay : public RaceDisplayObjectBase {
public:
    RaceMessageDisplay();                   /* 0x00197AA0 disabled, setText("", timer, timer) (the just-
                                               cleared timer, read back: 0), colour 0x80D0D0D0,
                                               setFont("sw_16") */
    /* [0] +0x0C 0x00199D10 ~RaceMessageDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void update();               /* [3] +0x24 0x00197BA8 update: timer += 1/60 up to the
                                               duration; blink counts down while duration >= 0 */
    virtual void draw();               /* [4] +0x2C 0x00197BF0 draw (visible and not blinked off) */

    bool isVisible();                       /* 0x00197B50 ? name: enabled, and duration 0 or 0 <= timer <= duration */
    /* A duration <= 0 keeps the timing (else duration, timer = -delay); NULL
     * keeps the text; the width is measured again.  12 callers (RaceDisplay). */
    void setText(const char *text, f32 duration, f32 delay);   /* 0x00197E90 ? name */
    void setFont(const char *style);        /* 0x00197EE8 ? name: strcpy(fontName, style) */
    /* The inline accessors (out-of-line copies; isFinished is expanded by
     * RaceEventDisplay::addEvent, clear by RaceEventDisplay::reset). */
    void clear() { setText("", -1.0f, 0.0f); } /* 0x00199C90 ? name: setText("", -1, 0) */
    void show(f32 duration, f32 delay) { this->duration = duration; timer = -delay; } /* 0x00199C80 */
    void setColor(u32 c) { color = c; }     /* 0x00199C78 */
    void setFadeTime(f32 in, f32 out) { fadeIn = in; fadeOut = out; } /* 0x00199C68 */
    void setBlink(int n) { blink = n * RACE_BLINK_FRAMES; } /* 0x00199C58 ? name: n blinks of 24 frames */
    bool isFinished() { return timer >= duration; } /* 0x00199C30 ? name: duration <= timer */

    f32  timer;                             /* 0x18 seconds, counts up from -delay */
    f32  duration;                          /* 0x1C 0: forever, < 0: none */
    f32  fadeIn;                            /* 0x20 */
    f32  fadeOut;                           /* 0x24 used once timer > fadeIn (clamped: the last fadeOut s) */
    u32  color;                             /* 0x28 */
    int  blink;                             /* 0x2C frames; hidden 12 of every 24 */
    f32  textWidth;                         /* 0x30 measured once (0: not yet) */
    f32  scaleX;                            /* 0x34 the text squeezed into w (at most 1) */
    char fontName[0x10];                    /* 0x38 */
    char text[0x40];                        /* 0x48 */
};
EGCS_SIZE(RaceMessageDisplay, 0x88);

/* vtable 0x0032EC08 (10 slots: [0], [3], [4]).  typeinfo
 * __tf16RaceEventDisplay 0x00199B60 (si, storage 0x004B2280).  The event
 * board (u048: RaceMessageBoard): up to 8 messages queued one after the
 * other in the board's colour, font, rectangle and alignment.  update and
 * draw are those of the 8 messages (virtual calls).  Size 0x47C
 * (RaceDisplay+0x3BC, +0x838). */
class RaceEventDisplay : public RaceDisplayObjectBase {
public:
    RaceEventDisplay();                     /* 0x00197F08 reset(), colour 0x802BCC49, setFont("sw_16") */
    /* [0] +0x0C 0x00199BB0 ~RaceEventDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr; it destroys messages[7]..[0]
     * with their virtual destructors, flags 0) */
    virtual void update();               /* [3] +0x24 0x001980F8 update: the messages', then both
                                               event times count down to 0 */
    virtual void draw();               /* [4] +0x2C 0x00198188 draw: the messages' */

    void reset();                           /* 0x00198078 every message cleared, offset and times 0 */
    /* The first finished message gets the text: delay 0 starts it when the
     * last event ends, else delay (>= 0) after the last event's start.  The
     * one-shot offset is cleared.  9 callers (RaceDisplay, T143). */
    void addEvent(const char *text, f32 duration, f32 delay);  /* 0x001981E8 ? name */
    void setFont(const char *style);        /* 0x00198360 ? name: strcpy(fontName, style) */
    void setColor(u32 c) { color = c; }     /* 0x00199B58 ? name (inline in the original) */
    void setOffset(int dx, int dy) { offX = dx; offY = dy; } /* 0x00199B48 the offset of the next addEvent only */
    f32 getLastEventBegin() const { return lastEventBegin; } /* 0x00199B40 (inline in the original) */
    f32 getLastEventEnd() const { return lastEventEnd; } /* 0x00199B38 (inline in the original) */

    int  offX;                              /* 0x18 for the next event only */
    int  offY;                              /* 0x1C */
    u32  color;                             /* 0x20 the colour of the next event */
    f32  lastEventBegin;                    /* 0x24 seconds from now (counts down) */
    f32  lastEventEnd;                      /* 0x28 */
    char fontName[0x10];                    /* 0x2C */
    RaceMessageDisplay messages[8];         /* 0x3C */
};
EGCS_SIZE(RaceEventDisplay, 0x47C);

/* vtable 0x0032EBB0 (10 slots: [0], [3], [4]).  typeinfo
 * __tf14RaceIndicators 0x00199AC8 (si, storage 0x004B2240).  Four lamps in
 * the font "odo#1" (g_003520B8): '!' VSC (green), '"' TCS (yellow): 1 on,
 * 2 blinking; '#' handbrake and '$' oil (red) when set; dim 0x40000000 when
 * off.  w <= 0 becomes the width of the string.  Size 0x20
 * (RaceDisplay+0xA15C). */
/* RaceIndicators' VSC / TCS lamp states (u054). */
enum { LAMP_OFF = 0, LAMP_ON = 1, LAMP_BLINK = 2 };

class RaceIndicators : public RaceDisplayObjectBase {
public:
    RaceIndicators();                       /* 0x00198380 the four lamps off (blink is not set) */
    /* [0] +0x0C 0x00199B18 ~RaceIndicators: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void update();               /* [3] +0x24 0x001983C8 update: blink += 21 */
    virtual void draw();               /* [4] +0x2C 0x001983D8 draw */

    void setVSC(int state) { vsc = state; } /* 0x00199AC0 (inline in the original) */
    void setTCS(int state) { tcs = state; } /* 0x00199AB8 (inline in the original) */
    void setBrake(int on) { handbrake = on; }   /* 0x00199AB0 (inline in the original; Lupo 2 name) */
    void setOIL(int on) { oil = on; }       /* 0x00199AA8 (inline in the original) */

    s8 vsc;                                 /* 0x18 LAMP_*: car_brake_assist (DynCarState::brakeAssist) */
    s8 tcs;                                 /* 0x19 LAMP_*: car_traction_assist */
    s8 handbrake;                           /* 0x1A car_handbrake_on (u051; u054 brake) */
    s8 oil;                                 /* 0x1B */
    s8 blink;                               /* 0x1C += 21 per frame: lit while > 0 */
    u8 unk_1D[3];                           /* 0x1D */
};
EGCS_SIZE(RaceIndicators, 0x20);

/* vtable 0x0032EB58 (10 slots: [0], [4]).  typeinfo __tf13RaceBackPanel
 * 0x00199A58 (si, storage 0x004B2260).  A panel behind the meters: below
 * 0x40 the image "panel_<n>" stretched over the rectangle (0x3F: whatever
 * image is set), 0x40 a frame 3 wide, 0x41 a filled shape (corners cut,
 * slanted sides), all 0x40000000.  Size 0x1C (RaceDisplay+0xA1D0). */
class RaceBackPanel : public RaceDisplayObjectBase {
public:
    RaceBackPanel();                        /* 0x001985D0 */
    virtual ~RaceBackPanel() {}             /* [0] +0x0C 0x00199A30 (out-of-line copy) */
    virtual void draw();               /* [4] +0x2C 0x00198658 draw */

    void setPanel(int n);                   /* 0x00198608 ? name: n < 0x3F: setImage("panel_<n>") */
    void drawImage();                       /* 0x001986D8 ? name */
    void drawFrame();                       /* 0x00198730 ? name: race_fill_octagon(.., 3) */
    void drawShape();                       /* 0x001987A0 ? name: an 8-vertex strip */

    int panel;                              /* 0x18 */
};
EGCS_SIZE(RaceBackPanel, 0x1C);

/* vtable 0x0032EB00 (10 slots: [0], [4]).  typeinfo __tf13RaceABMonitor
 * 0x001999E0 (si, storage 0x004B2200).  The throttle (at x + w, green) and
 * brake (at x, red) bars, growing up from y on a dark background the size
 * of "accel_gage", under the frame images (quirk: both bars use
 * accel_gage's size).  Size 0x28 (RaceDisplay+0xA274). */
class RaceABMonitor : public RaceDisplayObjectBase {
public:
    RaceABMonitor();                        /* 0x001988F8 */
    virtual ~RaceABMonitor() {}             /* [0] +0x0C 0x001999B8 (out-of-line copy) */
    virtual void draw();               /* [4] +0x2C 0x00198940 draw */

    void setAccel(f32 v) { accel = v; }     /* 0x001999B0 (inline in the original) */
    void setBrake(f32 v) { brake = v; }     /* 0x001999A8 (inline in the original) */

    Tex1Image *accelImage;                  /* 0x18 "accel_gage", looked up on the first draw */
    Tex1Image *brakeImage;                  /* 0x1C "brake_gage" */
    f32        accel;                       /* 0x20 0..1 */
    f32        brake;                       /* 0x24 0..1 */
};
EGCS_SIZE(RaceABMonitor, 0x28);

/* vtable 0x0032EAA8 (10 slots: [0], [4]).  typeinfo
 * __tf19RaceSteeringDisplay 0x00199958 (si, storage 0x004B2220).  The centre
 * marker (g_0032E878) and the "steering" image rotated by angle * -90
 * degrees.  Size 0x20 (RaceDisplay+0xA29C). */
class RaceSteeringDisplay : public RaceDisplayObjectBase {
public:
    RaceSteeringDisplay();                  /* 0x00198B50 */
    virtual ~RaceSteeringDisplay() {}       /* [0] +0x0C 0x00199930 (out-of-line copy) */
    virtual void draw();               /* [4] +0x2C 0x00198B90 draw */

    void setAngle(f32 a) { angle = a; }     /* 0x00199928 ? name (inline in the original; index.txt:
                                               RaceRankDisplay::setScale, a code match) */

    Tex1Image *wheel;                       /* 0x18 "steering", looked up on the first draw */
    f32        angle;                       /* 0x1C -1..1 */
};
EGCS_SIZE(RaceSteeringDisplay, 0x20);

/* vtable 0x0032EA50 (10 slots: [0], [4]).  typeinfo
 * __tf19RaceTireWearDisplay 0x001998D8 (si, storage 0x004B2210).  The tyre
 * gauge "tire_gage" drawn twice per wheel i, its alpha selecting the parts
 * (gfx_alpha_func_raw): alpha == 5 (i + 1) in the wheel's colour, then
 * alpha >= 5 (i + 1) + 40 in red (warning) or dark.  Size 0x30
 * (RaceDisplay+0xA2BC). */
class RaceTireWearDisplay : public RaceDisplayObjectBase {
public:
    RaceTireWearDisplay();                  /* 0x00198CC0 colours 0x80202020, no warnings */
    virtual ~RaceTireWearDisplay() {}       /* [0] +0x0C 0x001998B0 (out-of-line copy) */
    virtual void draw();               /* [4] +0x2C 0x00198D30 draw */

    void setColor(int wheel, int r, int g, int b) { color[wheel] = 0x80000000 | (b << 16) | (g << 8) | r; } /* 0x00199888 ? name: 0x80BBGGRR (inline in the original) */
    /* 0x00199868 ? name (inline in the original).  The array's address is
     * formed first, then the index added (retail: addiu 0x2C; addu): an
     * exact form, not the original (&warning[wheel] adds 0x2C to the
     * index). */
    void setWarning(int wheel, int on) {
        s8 *p = warning;
        p += wheel;
        if (on)
            *p = 1;
        else
            *p = 0;
    }

    Tex1Image *gauge;                       /* 0x18 "tire_gage", looked up on the first draw */
    u32        color[4];                    /* 0x1C one per wheel */
    s8         warning[4];                  /* 0x2C */
};
EGCS_SIZE(RaceTireWearDisplay, 0x30);

/* vtable 0x0032E9F8 (10 slots: [0], [4]).  typeinfo __tf16RaceSpeedDisplay
 * 0x001997F8 (si on RaceValueDisplayBase, storage 0x004B22B0).  The speed
 * unit ("sw_12"), the speed in that unit (T518, race_format_speed, "sw_16")
 * left of it, the label left of a column at least 90 wide.  Size 0x48
 * (RaceDisplay+0x32C, +0x374). */
class RaceSpeedDisplay : public RaceValueDisplayBase {
public:
    RaceSpeedDisplay();                     /* 0x00198EB0 speed -1.0 */
    /* [0] +0x0C 0x00199848 ~RaceSpeedDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void draw();               /* [4] +0x2C 0x00198EF0 draw */

    void setSpeed(f32 v) { speed = v; }     /* 0x001997F0 (inline in the original) */

    f32 speed;                              /* 0x44 */
};
EGCS_SIZE(RaceSpeedDisplay, 0x48);

/* vtable 0x0032E9A0 (10 slots: [0], [3], [4]).  typeinfo
 * __tf18RaceCarIconDisplay 0x00199780 (si, storage 0x004B22C0).  A 64 x 32
 * car icon (CarIcon::pixels, uploaded through T513) at the rectangle's far
 * corner, timed and blinking like a message.  Size 0x28
 * (RaceDisplay+0xA414, +0xA43C). */
class RaceCarIconDisplay : public RaceDisplayObjectBase {
public:
    RaceCarIconDisplay();                   /* 0x001990F0 */
    /* [0] +0x0C 0x001997D0 ~RaceCarIconDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void update();               /* [3] +0x24 0x00199190 update (as RaceMessageDisplay's) */
    virtual void draw();               /* [4] +0x2C 0x001991D8 draw (with an icon, visible, not blinked off) */

    bool isVisible();                       /* 0x00199138 ? name (as RaceMessageDisplay::isVisible) */
    void setIcon(CarIcon *p) { icon = p; }  /* 0x00199778 ? name (inline in the original; index.txt:
                                               RaceLapDisplay::setCurrentLaps, a code match) */
    void show(f32 duration, f32 delay) { this->duration = duration; timer = -delay; } /* 0x00199768 ? name (inline in the original) */
    void setBlink(int n) { blink = n * RACE_BLINK_FRAMES; } /* 0x00199758 ? name: n blinks of 24 frames (inline in the original) */
    bool isFinished() { return timer >= duration; } /* 0x00199730 ? name (inline in the original) */

    CarIcon *icon;                          /* 0x18 NULL: nothing drawn */
    f32      timer;                         /* 0x1C seconds, counts up from -delay */
    f32      duration;                      /* 0x20 0: forever, < 0: none */
    int      blink;                         /* 0x24 frames; hidden 12 of every 24 */
};
EGCS_SIZE(RaceCarIconDisplay, 0x28);

/* vtable 0x0032E948 (10 slots: [0], [3], [4]).  typeinfo
 * __tf21RaceReplayModeDisplay 0x001996C0 (si, storage 0x004B22D0).
 * "[L2] <mode> [R2]" in replays (the three images side by side), alpha
 * min(4 t^2, 1) as the time runs out.  Size 0x2C (RaceDisplay+0xDC4). */
class RaceReplayModeDisplay : public RaceDisplayObjectBase {
public:
    RaceReplayModeDisplay();                /* 0x00199420 mode -1 */
    /* [0] +0x0C 0x00199710 ~RaceReplayModeDisplay: the implicit destructor (the copy
     * emitted with the vtable stores no vptr) */
    virtual void update();               /* [3] +0x24 0x00199470 update: time counts down to 0 */
    virtual void draw();               /* [4] +0x2C 0x00199490 draw (time > 0 and a mode image) */

    /* No `this': the image of a replay camera mode ("replay_mode_traditional",
     * "_avantgarde", "_broadcast", "_crsline", else "_driver"). */
    static const char *modeImageName(int mode);    /* 0x00199600 ? name */
    void setMode(int m);                    /* 0x00199670 ? name (inline in the original; expanded in RaceDisplay) */
    void show(f32 t) { time = t; }          /* 0x001996B8 ? name (inline in the original) */

    Tex1Image *l2;                          /* 0x18 "replay_mode_l2" */
    Tex1Image *r2;                          /* 0x1C "replay_mode_r2" */
    Tex1Image *modeImage;                   /* 0x20 */
    f32        time;                        /* 0x24 seconds left */
    int        mode;                        /* 0x28 -1: none */
};
EGCS_SIZE(RaceReplayModeDisplay, 0x2C);

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_info nodes (.bss) the __tf functions build. */
extern "C" u32 __ti20RaceValueDisplayBase[3];  /* 0x004B1260 */
extern "C" u32 __ti15RaceTimeDisplay[3];       /* 0x004B21F0 */
extern "C" u32 __ti13RaceABMonitor[3];         /* 0x004B2200 */
extern "C" u32 __ti19RaceTireWearDisplay[3];   /* 0x004B2210 */
extern "C" u32 __ti19RaceSteeringDisplay[3];   /* 0x004B2220 */
extern "C" u32 __ti15RaceGearDisplay[3];       /* 0x004B2230 */
extern "C" u32 __ti14RaceIndicators[3];        /* 0x004B2240 */
extern "C" u32 __ti15RaceRankDisplay[3];       /* 0x004B2250 */
extern "C" u32 __ti13RaceBackPanel[3];         /* 0x004B2260 */
extern "C" u32 __ti19RaceLapTimesDisplay[3];   /* 0x004B2270 */
extern "C" u32 __ti16RaceEventDisplay[3];      /* 0x004B2280 */
extern "C" u32 __ti21RaceDigitalSpeedmeter[3]; /* 0x004B2290 */
extern "C" u32 __ti12RaceOdometer[3];          /* 0x004B22A0 */
extern "C" u32 __ti16RaceSpeedDisplay[3];      /* 0x004B22B0 */
extern "C" u32 __ti18RaceCarIconDisplay[3];    /* 0x004B22C0 */
extern "C" u32 __ti21RaceReplayModeDisplay[3]; /* 0x004B22D0 */
extern "C" u32 __ti18RaceMessageDisplay[3];    /* 0x004B22E0 */
extern "C" u32 __ti14RaceLapDisplay[3];        /* 0x004B22F0 */
#endif

#endif /* GAME_T157_RACE_DISPLAY_PARTS_H */
