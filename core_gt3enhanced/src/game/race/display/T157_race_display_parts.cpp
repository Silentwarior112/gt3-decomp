/* T157_race_display_parts.cpp - the elements of the race head-up display
 * (0x00196910..0x0019A148).
 *
 * Every element derives from RaceDisplayObjectBase (T156): a rectangle
 * (x, y, w, h, 16-bit), an alignment per axis (RACE_ALIGN_*, T158) and an
 * enabled flag.  update is the per-frame update, draw the draw
 * (RaceDisplay::virtual_2 / virtual_3 call them).  Text goes through the
 * system font g_002F1968 (styles "sw_12" / "sw_16" / "sw_20", "gearr#1",
 * "speed", "odo#1", "rank#1"); images come from the race display's image
 * pack (race_display_find_image, T154) and are drawn by the T158 helpers.
 * The updates run once per frame, 60 a second (RACE_FRAME_TIME).
 *
 * Inline in the original (T157_race_display_parts.h): every destructor and
 * the small accessors.  egcs emitted their out-of-line copies at the end of
 * the TU, class by class in reverse order of the classes (the accessors,
 * the __tf function and the destructor); the destructors have only their
 * markers here.
 *
 * <new> first: retail's T157 .rodata opens with "bad_alloc" (0x0032E810,
 * nothing references it; <new>'s inline what(), DATA.md), 6 zero bytes, then
 * RaceDisplayFader::update's jump table (u053's data pass,
 * ref/data/u054/pending.md).
 */
// DATA_START 0x0032E810
#include <new>
#include "game/race/display/race_display_parts_internal.h"

/* ======================================================================== */
/* RaceDisplayFader: the "gt_logo_mini" watermark                           */
/* ======================================================================== */

/* RaceDisplayFader::state (jump table 0x0032E820). */
enum {
    FADER_START    = 0,                     /* arms the first wait */
    FADER_WAIT     = 1,                     /* hidden until timer runs out */
    FADER_FADE_IN  = 2,
    FADER_SHOW     = 3,                     /* shown until timer runs out */
    FADER_FADE_OUT = 4,
    FADER_HIDDEN   = 5                      /* re-arms the wait (30 s) */
};
#define FADER_FIRST_WAIT  180               /* frames: 3 s */
#define FADER_HOLD        1800              /* frames: 30 s */
#define FADER_FADE_STEP   (1.0f / 120.0f)   /* per frame: 2 s fades (gp-0x7C68 / gp-0x7C64 0x3C088889) */

// FUNC 0x00196910 RaceDisplayFader::RaceDisplayFader
//   Caller: RaceDisplay::RaceDisplay.
RaceDisplayFader::RaceDisplayFader()
{
    state = FADER_START;
    timer = 0;
    alpha = 0.0f;
    logo = 0;
    aspec = 0;                              /* the enhanced engine (Lupo 2 L2 0x001943D8) */
}

// FUNC 0x00196928 RaceDisplayFader::reset
//   Caller: RaceDisplay::virtual_0.  The logo image is kept.
void RaceDisplayFader::reset()
{
    state = FADER_START;
    timer = 0;
    alpha = 0.0f;
}

// FUNC 0x00196938 RaceDisplayFader::update
//   Caller: RaceDisplay::virtual_2.  Jump table 0x0032E820 (6 cases; state
//   compared unsigned).  Wait 3 s, fade in over 2 s, stay 30 s, fade out
//   over 2 s, then (FADER_HIDDEN) wait 30 s and fade in again.
void RaceDisplayFader::update()
{
    switch ((u32)state) {
    case FADER_START:
        state = state + 1;
        timer = FADER_FIRST_WAIT;
        /* fall through */
    case FADER_WAIT:
        timer = timer - 1;
        if (timer < 0)
            state = state + 1;
        break;
    case FADER_FADE_IN:
        alpha = alpha + FADER_FADE_STEP;
        if (alpha > 1.0f) {
            alpha = 1.0f;
            state = state + 1;
            timer = FADER_HOLD;
        }
        break;
    case FADER_SHOW:
        timer = timer - 1;
        if (timer < 0)
            state = state + 1;
        break;
    case FADER_FADE_OUT:
        alpha = alpha - FADER_FADE_STEP;
        if (alpha < 0.0f) {
            alpha = 0.0f;
            state = state + 1;
        }
        break;
    case FADER_HIDDEN:
        state = FADER_WAIT;
        timer = FADER_HOLD;
        break;
    }
}

// FUNC 0x00196A38 RaceDisplayFader::draw
//   Caller: RaceDisplay::virtual_3.  The logo, white at the faded alpha, at
//   (620, 420) aligned bottom right (+ the wide screen shift).
//   The enhanced engine (ENH_HUD_LOGO_LOOK, hud.md C14; Lupo 2 L2
//   0x001944F8): drawn additively (Cs * As + Cd) at half grey, so the
//   watermark brightens the picture instead of covering it; aspec selects
//   "gt_aspec_logo_mini" (GT3's logo when the pack lacks it).
void RaceDisplayFader::draw()
{
#if ENH_HUD_LOGO_LOOK
    gfx_blend_func(PGL_BLEND_ADD, 1.0f);    /* Lupo 2: blend 0x48 */
    pgl_color4f(0.5f, 0.5f, 0.5f, alpha);
    if (logo == 0) {
        if (aspec != 0)
            logo = race_display_find_new_image("gt_aspec_logo_mini");
        if (logo == 0)
            logo = race_display_find_image("gt_logo_mini");
    }
    race_draw_image(logo, RACE_ALIGN_WIDE | RACE_ALIGN_RIGHT, RACE_ALIGN_BOTTOM, 620.0f, 420.0f);
    gfx_blend_func(PGL_BLEND_ALPHA, 1.0f);  /* back to the display's blend */
#else
    pgl_color4f(1.0f, 1.0f, 1.0f, alpha);
    if (logo == 0)
        logo = race_display_find_image("gt_logo_mini");
    race_draw_image(logo, RACE_ALIGN_WIDE | RACE_ALIGN_RIGHT, RACE_ALIGN_BOTTOM, 620.0f, 420.0f);
#endif
}

/* ======================================================================== */
/* RaceLapDisplay                                                           */
/* ======================================================================== */

// FUNC 0x00196AB0 RaceLapDisplay::RaceLapDisplay
//   Caller: RaceDisplay::RaceDisplay.  Aligned right / top.
RaceLapDisplay::RaceLapDisplay()
    : RaceDisplayObjectBase()
{
    alignX = RACE_ALIGN_RIGHT;
    labelWidth = 0.0f;
    laps = 0;
    lap = 0;
    alignY = RACE_ALIGN_TOP;
}

// FUNC 0x00196B00 RaceLapDisplay::draw
//   Draw (when lap > 0): "<lap>/<laps>" ("<lap>" when laps <= 0; lap
//   clamped to 9999) in "sw_20", ending at x (+ the wide screen shift
//   whatever the alignment), then the word g_002C43E8[36] ("Lap") in
//   "sw_16" 10 units before it.  The widths are divided by the font's
//   Font::getScaleRawX scale; the label's is measured once.
void RaceLapDisplay::draw()
{
    char text[32];

    if (lap > 0) {
        g_002F1968.select("sw_20");
        g_002F1968.setColor(RACE_COLOR_TEXT);
        f32 shift = 0.0f;
        if (g_002F424C != 0)
            shift = RACE_WIDE_SHIFT;            /* gp-0x7C60 */
        int shown = lap;
        f32 posY = (f32)y;
        if (shown >= 10000)
            shown = 9999;
        f32 right = (f32)x + shift;
        if (laps > 0)
            sprintf(text, "%d/%d", shown, laps);
        else
            sprintf(text, "%d", shown);
        f32 invScale = 1.0f / g_002F1968.getScaleRawX();
        f32 textWidth = text_string_width(&g_002F1968, text);
        g_002F1968.setPos3(right - textWidth * invScale, posY, 0.0f);
        g_002F1968.putString(text);
        g_002F1968.select("sw_16");
        const char *label = g_002C43E8[RACEDISPLAY_LAP_COUNT_LABEL];    /* "Lap" */
        if (labelWidth == 0.0f)
            labelWidth = text_string_width(&g_002F1968, label);
        g_002F1968.setPos3(right - (textWidth + 10.0f + labelWidth) * invScale, posY, 0.0f);
        g_002F1968.putString(label);
    }
}

/* ======================================================================== */
/* RaceRankDisplay                                                          */
/* ======================================================================== */

// FUNC 0x00196CC0 RaceRankDisplay::RaceRankDisplay
//   Caller: RaceDisplay::RaceDisplay.
RaceRankDisplay::RaceRankDisplay()
    : RaceDisplayObjectBase()
{
    scale = 1.0f;
    rank = 0;
}

// FUNC 0x00196D08 RaceRankDisplay::draw
//   Draw: the rank's name ("1st".., race_rank_name) in "rank#1" at the
//   object's scale with a pitch of 3, first as a shadow 4 units down-right,
//   then in the text colour at (x, y); the size and pitch are reset after.
void RaceRankDisplay::draw()
{
    g_002F1968.select("rank#1");
    g_002F1968.setSize(scale, scale);
    g_002F1968.setSpacing(3.0f, 0.0f);
    int posX = x;
    int posY = y;
    const char *name = race_rank_name(rank);
    g_002F1968.setPos(posX + 4, posY + 4);
    g_002F1968.setColor(RACE_COLOR_SHADOW);
    g_002F1968.putString(name);
    g_002F1968.setPos(posX, posY);
    g_002F1968.setColor(RACE_COLOR_TEXT);
    g_002F1968.putString(name);
    g_002F1968.setSize(1.0f, 1.0f);
    g_002F1968.setSpacing(0.0f, 0.0f);
}

/* ======================================================================== */
/* RaceGearDisplay                                                          */
/* ======================================================================== */

/* The blink phases advance by this per frame (signed bytes, lit while > 0:
 * about 3 frames on, 3 off). */
#define GEAR_BLINK_STEP 0x2A

/* Halves every channel of a GS colour unless lit.  A statement macro: its
 * loop notes give draw retail's register allocation (the font address copied
 * into $s0 once charH is dead); written as a plain if, the allocation
 * differs. */
#define RACE_DIM_UNLESS(color, lit) do { if ((lit) == 0) (color) >>= 1; } while (0)

// FUNC 0x00196E18 RaceGearDisplay::RaceGearDisplay
//   Caller: RaceDisplay::RaceDisplay.  The shift lamp one character (24)
//   left of the gear.  The blink phases are not initialised.
RaceGearDisplay::RaceGearDisplay()
    : RaceDisplayObjectBase()
{
    lampX = -0x18;
    bright = 1;
    gear = 0;
    suggestedGear = 0;
    shiftWarning = 0;
    lampY = 0;
}

// FUNC 0x00196E70 RaceGearDisplay::update
//   Update: both blink phases advance by GEAR_BLINK_STEP; the shift lamp's
//   is held at 0 (unlit) without a warning.
void RaceGearDisplay::update()
{
    if (shiftWarning)
        lampBlink = lampBlink + GEAR_BLINK_STEP;
    else
        lampBlink = 0;
    suggestPhase = suggestPhase + GEAR_BLINK_STEP;
}

const char *g_00352060 = "gearr#1";        /* 0x00352060 gp-0x5990 (the string 0x00352058): the gear font */

// FUNC 0x00196E98 RaceGearDisplay::draw
//   Draw, in the 7-segment font g_00352060 ("gearr#1"), centred in the
//   rectangle (half a character down): the unlit cells "8\r1\r" one
//   character to the left (the font's '\r' glyphs), the gear ('R' below 1
//   in red, else '0' + gear in the text colour; both halved when not
//   bright), the unlit " > H\r" at the centre, the suggested gear (" > " and
//   '@' + n, red; black in the off phase when suggestBlink), and the shift
//   lamp ':' at the lamp offset (bright red while lampBlink > 0, else
//   black).
//   Matching (98.3%): retail branches over the dimming `srl'; egcs's jump
//   pass makes every form a `srl; movz' (if, ?:, `/= 2', an inline helper
//   with an early return, a bool bit-field `bright'; the reference compiler
//   the same).  The statement macro gives retail's register allocation.
void RaceGearDisplay::draw()
{
    g_002F1968.select(g_00352060);
    int charW = text_char_width(&g_002F1968);
    int charH = text_char_height(&g_002F1968);
    int centreX = race_align_pos(x, w, alignX) + (w >> 1);
    int baseY = race_align_pos(y, h, alignY) + (h >> 1) + (charH >> 1);
    g_002F1968.setPos3(centreX - charW, baseY, 0);
    g_002F1968.setColor(RACE_COLOR_GHOST);
    g_002F1968.begin();
    g_002F1968.putString("8\r1\r");
    u32 color = (gear >= 1) ? RACE_COLOR_TEXT : RACE_COLOR_RED;
    RACE_DIM_UNLESS(color, bright);             /* every channel halved (srl) */
    g_002F1968.setColor(color);
    g_002F1968.putChar((gear < 1) ? 'R' : gear + '0');
    g_002F1968.setPos3(centreX, baseY, 0);
    g_002F1968.setColor(RACE_COLOR_GHOST);
    g_002F1968.putString(" > H\r");
    if (suggestedGear > 0) {
        /* lit unless blinking in the off phase: (suggestBlink ^ 1) |
         * (suggestPhase > 0), a store-flag (a bool's ! is xori 1) */
        g_002F1968.setColor((!suggestBlink | (suggestPhase > 0)) ? RACE_COLOR_RED : RACE_COLOR_BLACK);
        g_002F1968.putString(" > ");
        g_002F1968.putChar(suggestedGear + '@');
    }
    g_002F1968.setPos3(centreX + lampX, baseY + lampY, 0);
    g_002F1968.setColor((lampBlink > 0) ? RACE_COLOR_BRIGHT_RED : RACE_COLOR_BLACK);
    g_002F1968.putChar(':');
}

/* ======================================================================== */
/* RaceDigitalSpeedmeter                                                    */
/* ======================================================================== */

// FUNC 0x001970A0 RaceDigitalSpeedmeter::RaceDigitalSpeedmeter
//   Caller: RaceDisplay::RaceDisplay.
RaceDigitalSpeedmeter::RaceDigitalSpeedmeter()
    : RaceDisplayObjectBase()
{
    speed = 0;
    unitX = 0;
    unitY = 0;
}

const char *g_00352088 = "speed";          /* 0x00352088 gp-0x5968 (the string 0x00352080): the speedometer's font */

// FUNC 0x001970E0 RaceDigitalSpeedmeter::draw
//   Draw: "%3d" of |speed| right-aligned at the aligned x in g_00352088
//   ("speed"), then the speed unit (UnitSettings::speedName) in "sw_12" at
//   the unit offset.  Stores the absolute value back.
void RaceDigitalSpeedmeter::draw()
{
    if (speed < 0)
        speed = -speed;
    int posX = race_align_pos(x, 0, alignX);
    g_002F1968.setColor(RACE_COLOR_TEXT);
    g_002F1968.setPos3(posX, y, 0);
    g_002F1968.setAlign(FONT_ALIGN_RIGHT);
    g_002F1968.select(g_00352088);
    g_002F1968.print("%3d", speed);
    g_002F1968.setPos3(posX + unitX, y + unitY, 0);
    g_002F1968.select("sw_12");
    g_002F1968.putString(g_002F59D8.speedName());
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
}

/* ======================================================================== */
/* RaceOdometer                                                             */
/* ======================================================================== */

// FUNC 0x001971E8 RaceOdometer::RaceOdometer
//   Callers: RaceDisplay::RaceDisplay (two odometers).  Six digits.  (EH:
//   ~RaceDisplayObjectBase when setDigits throws.)
RaceOdometer::RaceOdometer()
    : RaceDisplayObjectBase()
{
    value = 0;
    setDigits(6);
}

const char *g_003520A8 = "odo#1";          /* 0x003520A8 gp-0x5948 (the string 0x003520A0): the odometer's font */

// FUNC 0x00197258 RaceOdometer::draw
//   Draw: the value through format in g_003520A8 ("odo#1") in the
//   odometer's colour, aligned by race_align_posf on the width (measured
//   once, divided by the font's Font::getScaleRawX scale).
void RaceOdometer::draw()
{
    char text[32];

    g_002F1968.setColor(RACE_COLOR_ODOMETER);
    g_002F1968.select(g_003520A8);
    sprintf(text, format, value);
    f32 textW = width;
    if (textW == 0.0f)
        textW = width = text_string_width(&g_002F1968, text) / g_002F1968.getScaleRawX();
    g_002F1968.setPos3(race_align_posf((f32)x, textW, alignX), (f32)y, 0.0f);
    g_002F1968.putString(text);
}

// FUNC 0x00197340 RaceOdometer::setDigits
//   Callers: RaceDisplay::virtual_0, the constructor.  format = "%0<n>d";
//   the width is measured again.
void RaceOdometer::setDigits(int n)
{
    sprintf(format, "%%0%dd", n);
    width = 0.0f;
}

/* ======================================================================== */
/* RaceValueDisplayBase, RaceTimeDisplay                                    */
/* ======================================================================== */

/* The label column of the value displays: the value takes at least this
 * width, the label ends RACE_LABEL_GAP before it. */
#define RACE_VALUE_MIN_WIDTH 90.0f
#define RACE_LABEL_GAP       10.0f

/* T156's race_align_shift_x (0x001966D0, which has no callers), inline: the
 * time displays expand it (an inline copy for this unit; the out-of-line
 * function stays T156's). */
static inline f32 align_shift_x(int align)
{
    f32 shift = 0.0f;

    if (g_002F424C != 0)
        shift = RACE_WIDE_SHIFT;
    if (align == (RACE_ALIGN_WIDE | RACE_ALIGN_LEFT))
        return -shift;
    if (align == (RACE_ALIGN_WIDE | RACE_ALIGN_RIGHT))
        return shift;
    return 0.0f;
}

// FUNC 0x00197378 RaceValueDisplayBase::RaceValueDisplayBase
//   Callers: the RaceTimeDisplay and RaceSpeedDisplay constructors.  No
//   mark, the text colour, setLabel("") with the empty string 0x00352028.
//   (EH: ~RaceDisplayObjectBase when setLabel throws.)
RaceValueDisplayBase::RaceValueDisplayBase()
    : RaceDisplayObjectBase()
{
    mark = 0;
    color = RACE_COLOR_TEXT;
    setLabel("");
}

// FUNC 0x001973F8 RaceValueDisplayBase::setLabel
//   Callers: RaceDisplay::virtual_0, RaceDisplay::showCarIcon,
//   RaceDisplay::layoutTimeDisplay, the constructor.  At most 31
//   characters; the width is measured again.
void RaceValueDisplayBase::setLabel(const char *text)
{
    race_copy_string(label, text, sizeof(label));
    labelWidth = 0.0f;
}

// FUNC 0x00197428 RaceTimeDisplay::RaceTimeDisplay
//   Caller: RaceDisplay::RaceDisplay.  No time.
RaceTimeDisplay::RaceTimeDisplay()
    : RaceValueDisplayBase()
{
    time = -1;
}

// FUNC 0x00197468 RaceTimeDisplay::draw
//   Draw, right-aligned (Font::setAlign): the time (race_format_time; the
//   text is passed to Font::print as the format) in "sw_16" at x, shifted
//   by -/+RACE_WIDE_SHIFT for alignX WIDE | LEFT / WIDE | RIGHT on the wide
//   screen (T156's race_align_shift_x, expanded); then, left of a
//   column at least 90 wide, the mark image (when set: aligned right /
//   centre at the rectangle's bottom, the column moved left by its width)
//   and the label in "sw_12".
//   Matching (98.7%): retail computes max(valueW, 90) before loading the
//   10.0, ours loads both constants first (ternary, `>?', separate
//   statements tried; the reference compiler the same).
void RaceTimeDisplay::draw()
{
    char text[16];

    g_002F1968.setColor(color);
    g_002F1968.setAlign(FONT_ALIGN_RIGHT);
    g_002F1968.select("sw_16");
    race_format_time(text, time);
    f32 shiftX = align_shift_x(alignX);         /* RACE_WIDE_SHIFT gp-0x7C5C */
    f32 posY = (f32)y;
    f32 right = (f32)x + shiftX;
    g_002F1968.setPos3(right, posY, 0.0f);
    g_002F1968.print(text);
    f32 invScale = 1.0f / g_002F1968.getScaleRawX();
    f32 valueW = text_string_width(&g_002F1968, text) * invScale;
    f32 labelRight = (valueW > RACE_VALUE_MIN_WIDTH ? valueW : RACE_VALUE_MIN_WIDTH) + RACE_LABEL_GAP;   /* max.s */
    if (mark != 0) {
        g_002F1968.flush();
        pgl_color(RACE_COLOR_WHITE);
        race_draw_image(mark, RACE_ALIGN_RIGHT, RACE_ALIGN_CENTER,
                        (right - (f32)(int)labelRight) + (f32)w, (f32)(y + h));
        /* the mark's width, ((clamp >> 14) & 0x3FF) + 1 as a u64 (the
         * unsigned conversion through __floatdisf) */
        labelRight += (f32)(((mark->tex->clamp >> 14) & 0x3FF) + 1); /* implicit: __floatdisf (u64 to float) */
    }
    g_002F1968.select("sw_12");
    if (labelWidth == 0.0f)
        labelWidth = text_string_width(&g_002F1968, label) * invScale;
    g_002F1968.setColor(color);
    g_002F1968.setPos3((right - labelRight) - labelWidth, posY, 0.0f);
    g_002F1968.putString(label);
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
}

/* ======================================================================== */
/* RaceLapTimesDisplay                                                      */
/* ======================================================================== */

#define LAP_TIMES_MAX 8                     /* RaceLapTimesDisplay::times */

// FUNC 0x001976D8 RaceLapTimesDisplay::RaceLapTimesDisplay
//   Caller: RaceDisplay::RaceDisplay.  Three lines, 0.8 character heights
//   apart (gp-0x7C58).  (EH: ~RaceDisplayObjectBase when reset throws.)
RaceLapTimesDisplay::RaceLapTimesDisplay()
    : RaceDisplayObjectBase()
{
    reset();
    lines = 3;
    lineSpacing = 0.8f;
}

// FUNC 0x00197750 RaceLapTimesDisplay::reset
//   Callers: RaceDisplay::virtual_0, the constructor.
void RaceLapTimesDisplay::reset()
{
    lapCount = 0;
    stored = 0;
}

// FUNC 0x00197760 RaceLapTimesDisplay::draw
//   Draw, right-aligned (Font::setAlign) at x (+ the wide screen shift as
//   in RaceTimeDisplay): the last `lines' times in "sw_16", one per line
//   (lineSpacing character heights apart; race_format_time(-1), the dashes,
//   for lines past the stored ones), then their lap numbers ("%d",
//   "sw_12", lapCount - lines + 1 upwards, at least 1) in a column left of
//   the widest time.  Quirk: the column width is the maximum of 90 and the
//   UNscaled text widths, and it is scaled by Font::getSizeX, not by
//   1 / Font::getScaleRawX as the times are.
//   Matching (98.8%): our `line++' is scheduled into the first call's
//   block, retail's next to `lineY += lineH'.
void RaceLapTimesDisplay::draw()
{
    char text[16];

    g_002F1968.setColor(RACE_COLOR_TEXT);
    f32 maxW = RACE_VALUE_MIN_WIDTH;
    g_002F1968.setAlign(FONT_ALIGN_RIGHT);
    g_002F1968.select("sw_16");
    f32 lineH = (f32)text_char_height(&g_002F1968) * lineSpacing;
    int first = stored - lines;
    if (first < 0)
        first = 0;
    f32 invScale = 1.0f / g_002F1968.getScaleRawX();
    f32 shiftX = align_shift_x(alignX);         /* RACE_WIDE_SHIFT gp-0x7C54 */
    int line = first;
    f32 lineY = (f32)y;
    f32 right = (f32)x + shiftX;
    for (; line < first + lines; line++) {
        race_format_time(text, (line < stored) ? times[line] : -1);
        f32 textW = text_string_width(&g_002F1968, text);
        maxW = (textW > maxW) ? textW : maxW;   /* max.s maxW, textW */
        textW = textW * invScale;
        f32 thisY = lineY;
        lineY += lineH;
        g_002F1968.setPos3(right - textW, thisY, 0.0f);
        g_002F1968.putString(text);
    }
    g_002F1968.select("sw_12");
    int firstLap = lapCount - lines + 1;
    if (firstLap <= 0)
        firstLap = 1;
    maxW = (maxW + RACE_LABEL_GAP) * g_002F1968.getSizeX();
    right = right - maxW;                       /* the lap numbers' column */
    int lapNo = firstLap;
    lineY = (f32)y;
    for (; lapNo < firstLap + lines; lapNo++) {
        f32 thisY = lineY;
        lineY += lineH;
        g_002F1968.setPos3(right, thisY, 0.0f);
        g_002F1968.print("%d", lapNo);
    }
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
}

// FUNC 0x00197A10 RaceLapTimesDisplay::addLapTime
//   Caller: RaceDisplay::pollLapTimeEvents.  Appends a time (the oldest
//   dropped when LAP_TIMES_MAX are stored) and counts the lap.
void RaceLapTimesDisplay::addLapTime(int time)
{
    int slot = stored;
    if (slot >= LAP_TIMES_MAX) {
        for (int i = 0; i < LAP_TIMES_MAX - 1; i++)
            times[i] = times[i + 1];
        stored = stored - 1;
        slot = stored;
    }
    times[slot] = time;
    stored = stored + 1;
    lapCount = lapCount + 1;
}

// FUNC 0x00197A80 RaceLapTimesDisplay::setCurrentLapTime
//   No callers.  Writes the slot after the stored times (the last one when
//   full) without counting it.
void RaceLapTimesDisplay::setCurrentLapTime(int time)
{
    times[(stored >= LAP_TIMES_MAX) ? LAP_TIMES_MAX - 1 : stored] = time;
}

/* ======================================================================== */
/* RaceMessageDisplay                                                       */
/* ======================================================================== */

/* A blink lasts RACE_BLINK_FRAMES (T157_race_display_parts.h). */
/* A text "@<name>" is the image <name>. */
#define RACE_IMAGE_TEXT   '@'

// FUNC 0x00197AA0 RaceMessageDisplay::RaceMessageDisplay
//   Callers: RaceDisplay::RaceDisplay, RaceEventDisplay's constructor (the
//   array).  Disabled, no fades, no blink; setText's duration and delay are
//   the just-cleared timer field, read back from memory (0: setText keeps
//   the timing).  Light grey, "sw_16".  (EH: ~RaceDisplayObjectBase when
//   setText / setFont throw.)
RaceMessageDisplay::RaceMessageDisplay()
    : RaceDisplayObjectBase()
{
    timer = 0.0f;
    duration = 0.0f;
    fadeIn = 0.0f;
    fadeOut = 0.0f;
    blink = 0;
    enabled = 0;
    shadow = 0;                             /* the enhanced engine (Lupo 2 L2 0x001955F8: no shadow, */
    textImage = 0;                          /* no image) */
    setText("", timer, timer);
    color = RACE_COLOR_MESSAGE;             /* Lupo 2: 0x80C0C0C0 (ENH_HUD_TEXT_LOOK) */
    setFont("sw_16");
}

// FUNC 0x00197B50 RaceMessageDisplay::isVisible
//   ? name.  Caller: draw.  Enabled and either shown forever
//   (duration 0) or within 0 <= timer <= duration.
bool RaceMessageDisplay::isVisible()
{
    bool visible = false;
    if (enabled) {
        if (duration == 0.0f)
            visible = true;
        else if (duration > 0.0f && timer >= 0.0f && timer <= duration)
            visible = true;
    }
    return visible;
}

// FUNC 0x00197BA8 RaceMessageDisplay::update
//   Update: the timer runs (RACE_FRAME_TIME, gp-0x7C50) up to the
//   duration; the blink counter runs down while the message has a duration
//   (>= 0).
void RaceMessageDisplay::update()
{
    if (timer < duration)
        timer = timer + RACE_FRAME_TIME;
    if (duration >= 0.0f) {
        if (blink > 0)
            blink = blink - 1;
    }
}

// FUNC 0x00197BF0 RaceMessageDisplay::draw
//   Draw, while visible and not in the off half of a blink: a text
//   "@<name>" is the image <name> (race_display_find_image, aligned by the
//   object's alignment) in the colour; otherwise the text in fontName,
//   squeezed (scaleX, at most 1) into w when w > 0, aligned by
//   race_align_posf on its width (measured once, divided by
//   Font::getScaleX), its alpha faded by clamp(a * a, 0, 1): a = timer /
//   fadeIn while timer < fadeIn, then (duration - timer) / fadeOut once
//   timer > fadeIn (above 1 until the last fadeOut seconds; where the two
//   fades overlap, the fade out wins).
//   Matching (92.2%): the squeeze's min.s operand order (allocation; ?: in
//   every direction and `<?' give ours), and retail loads the clamp's 1.0
//   before squaring at the join (ours squares first, duplicated into the
//   bc1fl slots).
void RaceMessageDisplay::draw()
{
    if (!isVisible())
        return;
    if ((blink / (RACE_BLINK_FRAMES / 2)) & 1)
        return;
#if ENH_HUD_TEXT_LOOK
    drawEnhanced();
#else
    if (text[0] != RACE_IMAGE_TEXT) {
        g_002F1968.select(fontName);
        if (textWidth == 0.0f) {
            f32 width = text_string_width(&g_002F1968, text) / g_002F1968.getScaleX();
            scaleX = 1.0f;
            textWidth = width;
            if (w > 0) {
                f32 squeeze = (f32)w / width;
                squeeze = (squeeze < 1.0f) ? squeeze : 1.0f;    /* min.s */
                scaleX = squeeze;
                textWidth = width * squeeze;
            }
        }
        f32 in = fadeIn;
        f32 level = 1.0f;
        if (in > 0.0f) {
            if (timer < in)
                level = timer / in;
        }
        f32 out = fadeOut;
        if (out > 0.0f) {
            if (in < timer)
                level = (duration - timer) / out;
        }
        f32 sq = level * level;
        f32 pos = (sq > 0.0f) ? sq : 0.0f;                      /* max.s */
        f32 alpha = (pos < 1.0f) ? pos : 1.0f;                  /* min.s */
        u32 rgba = color;
        u32 rgb = rgba & 0xFFFFFF;
        /* the alpha byte as an unsigned value, scaled, truncated */
        g_002F1968.setColor(rgb | ((u32)(int)((f32)(rgba >> 24) * alpha) << 24));
        g_002F1968.setPos3(race_align_posf((f32)x, textWidth, alignX), (f32)y, 0.0f);
        g_002F1968.setSize(scaleX, 1.0f);
        g_002F1968.putString(text);
        g_002F1968.setSize(1.0f, 1.0f);
    } else {
        g_002F1968.flush();
        pgl_color(color);
        race_draw_image(race_display_find_image(text + 1), alignX, alignY, (f32)x, (f32)y);
    }
#endif
}

/* The enhanced engine: Lupo 2's text look (hud.md C12).  The shadow's alpha
 * follows the text's later: clamp((alpha - 0.3) / 0.7) (gp constants 0.3,
 * 1.42857). */
#define RACE_SHADOW_FADE_START 0.3f
#define RACE_SHADOW_FADE_SCALE 1.42857146f  /* 1 / 0.7 */
#define RACE_SHADOW_OFFSET     2.0f         /* pixels right and down, times scaleX */

// RaceMessageDisplay::drawEnhanced (the enhanced engine)
//   Lupo 2's draw (L2 0x001956F8, ENH_HUD_TEXT_LOOK, hud.md C12; called by
//   draw once the message is visible and not blinked off): the image setText
//   looked up for a text "@<name>" (textImage) in the colour, aligned by the
//   object's alignment; otherwise the text as GT3 draws it (the same fades
//   and squeeze), first in black 2 * scaleX pixels right and down when
//   shadow is 1, its alpha the text's clamped through (a - 0.3) / 0.7 so
//   the shadow fades out before the text.
void RaceMessageDisplay::drawEnhanced()
{
    if (textImage != 0) {
        g_002F1968.flush();
        pgl_color(color);
        race_draw_image(textImage, alignX, alignY, (f32)x, (f32)y);
        return;
    }
    g_002F1968.select(fontName);
    if (textWidth == 0.0f) {
        f32 width = text_string_width(&g_002F1968, text) / g_002F1968.getScaleX();
        scaleX = 1.0f;
        textWidth = width;
        if (w > 0) {
            f32 squeeze = (f32)w / width;
            squeeze = (squeeze < 1.0f) ? squeeze : 1.0f;
            scaleX = squeeze;
            textWidth = width * squeeze;
        }
    }
    g_002F1968.setSize(scaleX, 1.0f);
    f32 in = fadeIn;
    f32 level = 1.0f;
    if (in > 0.0f) {
        if (timer < in)
            level = timer / in;
    }
    f32 out = fadeOut;
    if (out > 0.0f) {
        if (in < timer)
            level = (duration - timer) / out;
    }
    f32 sq = level * level;
    f32 pos = (sq > 0.0f) ? sq : 0.0f;
    f32 alpha = (pos < 1.0f) ? pos : 1.0f;
    u32 rgba = color;
    f32 baseAlpha = (f32)(rgba >> 24);
    f32 left = race_align_posf((f32)x, textWidth, alignX);
    if (shadow == 1) {
        f32 s = (alpha - RACE_SHADOW_FADE_START) * RACE_SHADOW_FADE_SCALE;
        s = (s > 0.0f) ? s : 0.0f;
        s = (s < 1.0f) ? s : 1.0f;
        f32 offset = scaleX * RACE_SHADOW_OFFSET;
        g_002F1968.setColor((u32)(int)(baseAlpha * s) << 24);     /* black */
        g_002F1968.setPos3(left + offset, (f32)y + offset, 0.0f);
        g_002F1968.putString(text);
    }
    g_002F1968.setColor((rgba & 0xFFFFFF) | ((u32)(int)(baseAlpha * alpha) << 24));
    g_002F1968.setPos3(left, (f32)y, 0.0f);
    g_002F1968.putString(text);
    g_002F1968.setSize(1.0f, 1.0f);
}

// FUNC 0x00197E90 RaceMessageDisplay::setText
//   ? name.  12 callers (RaceDisplay).  A duration <= 0 keeps the timing
//   (else duration, timer = -delay); NULL keeps the text (at most 63
//   characters); the width is measured again.
//   The enhanced engine (ENH_HUD_TEXT_LOOK, hud.md C12; Lupo 2 L2
//   0x00195A58): a text "@<name>" is looked up here, once (textImage, the
//   text left empty), any other is copied (up to 0x5F characters) and
//   clears the image.
void RaceMessageDisplay::setText(const char *text, f32 duration, f32 delay)
{
    if (duration > 0.0f) {
        this->duration = duration;
        timer = -delay;
    }
#if ENH_HUD_TEXT_LOOK
    if (text != 0) {
        if (text[0] == RACE_IMAGE_TEXT) {
            textImage = race_display_find_image(text + 1);
            this->text[0] = '\0';
        } else {
            race_copy_string(this->text, text, RACE_MESSAGE_TEXT_MAX);
            textImage = 0;
        }
    }
#else
    if (text != 0)
        race_copy_string(this->text, text, RACE_MESSAGE_TEXT_MAX);
#endif
    textWidth = 0.0f;
}

// FUNC 0x00197EE8 RaceMessageDisplay::setFont
//   ? name.  Callers: RaceDisplay::virtual_0 / virtual_16, the constructor,
//   RaceEventDisplay::addEvent.  Unbounded strcpy into the 16 bytes.
void RaceMessageDisplay::setFont(const char *style)
{
    strcpy(fontName, style);
}

/* ======================================================================== */
/* RaceEventDisplay                                                         */
/* ======================================================================== */

#define RACE_EVENTS 8                       /* RaceEventDisplay::messages */

/* RaceDisplayObjectBase::setEnabled, inline in the original (T156 declares
 * it out of line; its copy 0x001967A0 has no callers): an exact form, not
 * the original (T154 uses the same stand-in).  Its parameter is a fresh
 * pseudo set to the constant, which loop.c hoists out of addEvent's loop
 * (retail's `li $a1, 1' before the loop). */
static inline void set_enabled(RaceDisplayObjectBase *obj, bool on)
{
    obj->enabled = on;
}

// FUNC 0x00197F08 RaceEventDisplay::RaceEventDisplay
//   Callers: RaceDisplay::RaceDisplay (two boards).  The 8 messages are
//   constructed in a count-down loop (RaceMessageDisplay::RaceMessageDisplay;
//   EH: the constructed ones destroyed through their virtual destructors,
//   then ~RaceDisplayObjectBase).  Green, "sw_16".
RaceEventDisplay::RaceEventDisplay()
    : RaceDisplayObjectBase()
{
    /* implicit: RaceMessageDisplay (constructor of each message) */
    reset();
    color = RACE_COLOR_GREEN;
    setFont("sw_16");
}

// FUNC 0x00198078 RaceEventDisplay::reset
//   Callers: RaceDisplay::virtual_0, RaceDisplay::disableAll, the
//   constructor.  Clears every message (RaceMessageDisplay::clear, inline:
//   setText("", -1, 0), which keeps the timing).
void RaceEventDisplay::reset()
{
    offX = 0;
    offY = 0;
    for (int i = 0; i < RACE_EVENTS; i++)
        messages[i].clear(); /* implicit: setText (inline clear) */
    lastEventBegin = 0.0f;
    lastEventEnd = 0.0f;
}

// FUNC 0x001980F8 RaceEventDisplay::update
//   Update: every message's update (virtual calls through each
//   message's vptr), then both event times count down (RACE_FRAME_TIME,
//   gp-0x7C4C) to 0.
//   Matching (69.4%; the loop is exact): retail counts the two down one
//   after the other (end's load after begin's max.s) and restores the
//   registers after the last store; ours loads both first.  Tried:
//   helpers by reference / pointer / value, `>?', if-forms, temporaries;
//   a do { } while (0) around end's load gives 93.1%, not kept.
void RaceEventDisplay::update()
{
    for (int i = 0; i < RACE_EVENTS; i++)
        messages[i].update();
    f32 begin = lastEventBegin - RACE_FRAME_TIME;
    lastEventBegin = (begin > 0.0f) ? begin : 0.0f;         /* max.s */
    f32 end = lastEventEnd - RACE_FRAME_TIME;
    lastEventEnd = (end > 0.0f) ? end : 0.0f;               /* max.s */
}

// FUNC 0x00198188 RaceEventDisplay::draw
//   Draw: every message's draw (virtual calls).
void RaceEventDisplay::draw()
{
    for (int i = 0; i < RACE_EVENTS; i++)
        messages[i].draw();
}

// FUNC 0x001981E8 RaceEventDisplay::addEvent
//   ? name.  9 callers (RaceDisplay, RaceFreeRunDisplay::virtual_18).
//   Takes the first finished message (RaceMessageDisplay::isFinished,
//   inline), gives it the board's colour, font (setFont), rectangle moved
//   by the one-shot offset and alignment (setLocation / setSize /
//   setAlignment: virtual calls), and queues it (setText): delay 0 starts
//   it when the last event ends; otherwise it starts delay (>= 0) after the
//   last event's start.  The offset is cleared even when no message is
//   free.
//   Matching (97.6%): lastEventBegin's load goes into the bc1fl slot (retail
//   loads it after the max.s, bc1f; nop).
void RaceEventDisplay::addEvent(const char *text, f32 duration, f32 delay)
{
    for (int i = 0; i < RACE_EVENTS; i++) {
        RaceMessageDisplay *message = &messages[i];
        if (message->isFinished()) {
            set_enabled(message, true);
            message->color = color;
            message->setFont(fontName);
            message->setLocation(x + offX, y + offY);
            message->setSize(w, h);
            message->setAlignment(alignX, alignY);
            if (delay == 0.0f) {
                delay = lastEventEnd;
                lastEventEnd = delay + duration;
            } else {
                delay = (delay > 0.0f) ? delay : 0.0f;                /* max.s */
                delay += lastEventBegin;
                f32 end = delay + duration;
                lastEventEnd = (end > lastEventEnd) ? end : lastEventEnd;   /* max.s */
            }
            lastEventBegin = delay;
            message->setText(text, duration, delay);
            break;
        }
    }
    offX = 0;
    offY = 0;
}

// FUNC 0x00198360 RaceEventDisplay::setFont
//   ? name.  8 callers (RaceDisplay, the constructor).  Unbounded strcpy
//   into the 16 bytes.
void RaceEventDisplay::setFont(const char *style)
{
    strcpy(fontName, style);
}

/* ======================================================================== */
/* RaceIndicators                                                           */
/* ======================================================================== */

/* RaceIndicators::vsc / tcs: LAMP_* (T157_race_display_parts.h).  The lamps' blink phase advances by this per frame (a signed byte, lit
 * while > 0: about 6 frames on, 6 off). */
#define LAMP_BLINK_STEP 0x15

// FUNC 0x00198380 RaceIndicators::RaceIndicators
//   Caller: RaceDisplay::RaceDisplay.  The four lamps off; blink is left
//   uninitialised.
RaceIndicators::RaceIndicators()
    : RaceDisplayObjectBase()
{
    vsc = LAMP_OFF;
    tcs = LAMP_OFF;
    handbrake = 0;
    oil = 0;
}

// FUNC 0x001983C8 RaceIndicators::update
//   Update: the blink phase advances by LAMP_BLINK_STEP.
void RaceIndicators::update()
{
    blink = blink + LAMP_BLINK_STEP;
}

const char *g_003520B8 = "odo#1";          /* 0x003520B8 gp-0x5938 (g_003520A8's string): the lamps' font */

// FUNC 0x001983D8 RaceIndicators::draw
//   Draw "! \" # $" in g_003520B8 ("odo#1") at the aligned rectangle, one
//   lamp per glyph, lit or RACE_COLOR_UNLIT: VSC green and TCS yellow when
//   on, or blinking; handbrake and oil red when set.  w <= 0 is replaced by the
//   width of the string (divided by Font::getScaleX, truncated, stored as
//   s16).
//   Matching (86.0%): retail computes on = (state == ON), then `if (state ==
//   BLINK) on |= lit' as a branch, then branches on `on'; the `|' forms
//   that give its `or' are if-converted by egcs (movz) in every shape tried
//   (|, bool / int locals, if, if / else, ?:), so the || form is kept.
void RaceIndicators::draw()
{
    g_002F1968.select(g_003520B8);
    if (w <= 0) {
        f32 measured = text_string_width(&g_002F1968, "! \" # $");
        measured = measured / g_002F1968.getScaleX();
        w = (s16)(int)measured;
    }
    int posX = race_align_pos(x, w, alignX);
    g_002F1968.setPos3(posX, race_align_pos(y, h, alignY), 0);
    bool lit = blink > 0;
    g_002F1968.setColor((vsc == LAMP_ON || (vsc == LAMP_BLINK && lit)) ? RACE_COLOR_GREEN : RACE_COLOR_UNLIT);
    g_002F1968.putChar('!');
    g_002F1968.putChar(' ');
    g_002F1968.setColor((tcs == LAMP_ON || (tcs == LAMP_BLINK && lit)) ? RACE_COLOR_YELLOW : RACE_COLOR_UNLIT);
    g_002F1968.putChar('"');
    g_002F1968.putChar(' ');
    g_002F1968.setColor((handbrake != 0) ? RACE_COLOR_RED : RACE_COLOR_UNLIT);
    g_002F1968.putChar('#');
    g_002F1968.putChar(' ');
    g_002F1968.setColor((oil != 0) ? RACE_COLOR_RED : RACE_COLOR_UNLIT);
    g_002F1968.putChar('$');
}

/* ======================================================================== */
/* RaceBackPanel                                                            */
/* ======================================================================== */

/* RaceBackPanel::panel: below RACE_PANEL_IMAGE_MAX the image "panel_<n>". */
#define RACE_PANEL_IMAGE_MAX 0x3F           /* setPanel looks up n < 0x3F */
#define RACE_PANEL_FRAME     0x40           /* a frame 3 wide */
#define RACE_PANEL_SHAPE     0x41           /* a filled shape */

// FUNC 0x001985D0 RaceBackPanel::RaceBackPanel
//   Caller: RaceDisplay::RaceDisplay.  panel is not initialised.
RaceBackPanel::RaceBackPanel()
    : RaceDisplayObjectBase()
{
}

// FUNC 0x00198608 RaceBackPanel::setPanel
//   ? name.  Callers: RaceDisplay::setLayout, RaceDisplay::layoutMeters.
//   Panels below 0x3F are the images "panel_<n>" (setImage).
void RaceBackPanel::setPanel(int n)
{
    char name[0x80];

    panel = n;
    if (n < RACE_PANEL_IMAGE_MAX) {
        sprintf(name, "panel_%d", n);
        setImage(name);
    }
}

// FUNC 0x00198658 RaceBackPanel::draw
//   Draw (after Font::flush): below 0x40 the image (drawImage; quirk: 0x3F
//   draws whatever image is set), 0x40 a frame (drawFrame), 0x41 a filled
//   shape (drawShape), anything else nothing.
void RaceBackPanel::draw()
{
    g_002F1968.flush();
    if (panel < RACE_PANEL_FRAME) {
        drawImage();
    } else {
        switch (panel) {
        case RACE_PANEL_FRAME:
            drawFrame();
            break;
        case RACE_PANEL_SHAPE:
            drawShape();
            break;
        }
    }
}

// FUNC 0x001986D8 RaceBackPanel::drawImage
//   ? name.  Caller: draw.  The image stretched over the rectangle
//   (race_draw_image_rect; in the current colour).
void RaceBackPanel::drawImage()
{
    race_draw_image_rect(image, (f32)x, (f32)y, (f32)w, (f32)h, alignX, alignY);
}

// FUNC 0x00198730 RaceBackPanel::drawFrame
//   ? name.  Caller: draw.  The aligned rectangle with its corners cut
//   by 3 (race_fill_octagon) in RACE_COLOR_UNLIT.
void RaceBackPanel::drawFrame()
{
    pgl_color(RACE_COLOR_UNLIT);
    int left = race_align_pos(x, w, alignX);
    race_fill_octagon(left, race_align_pos(y, h, alignY), w, h, 3);
}

// FUNC 0x001987A0 RaceBackPanel::drawShape
//   ? name.  Caller: draw.  An 8-vertex strip (6 qwords of
//   gfx_alloc_tail, pgl_draw_strip) in RACE_COLOR_UNLIT: the top corners
//   cut by 3, straight sides down to 30% of the height, then slanting in by
//   15% of the width to the bottom edge.
void RaceBackPanel::drawShape()
{
    pgl_color(RACE_COLOR_UNLIT);
    f32 left = (f32)race_align_pos(x, w, alignX);
    f32 top = (f32)race_align_pos(y, h, alignY);
    f32 width = (f32)w;
    f32 height = (f32)h;
    f32 slant = width * 0.15f;                  /* gp-0x7C48 */
    f32 slantHeight = height * 0.7f;            /* gp-0x7C44 */
    RaceDisplayVertex *v = (RaceDisplayVertex *)gfx_alloc_tail(6);
    f32 right = left + width;
    f32 bottom = top + height;
    v[0].x = left + 3.0f;   v[0].y = top;                  v[0].z = 0;
    v[1].x = right - 3.0f;  v[1].y = top;                  v[1].z = 0;
    v[2].x = left;          v[2].y = top + 3.0f;           v[2].z = 0;
    v[3].x = right;         v[3].y = top + 3.0f;           v[3].z = 0;
    v[4].x = left;          v[4].y = bottom - slantHeight; v[4].z = 0;
    v[5].x = right;         v[5].y = bottom - slantHeight; v[5].z = 0;
    v[6].x = left + slant;  v[6].y = bottom;               v[6].z = 0;
    v[7].x = right - slant; v[7].y = bottom;               v[7].z = 0;
    pgl_draw_strip(v, 0, 0, 0, 8);
}

/* ======================================================================== */
/* RaceABMonitor                                                            */
/* ======================================================================== */

// FUNC 0x001988F8 RaceABMonitor::RaceABMonitor
//   Caller: RaceDisplay::RaceDisplay.
RaceABMonitor::RaceABMonitor()
    : RaceDisplayObjectBase()
{
    accelImage = 0;
    brakeImage = 0;
    accel = 0.0f;
    brake = 0.0f;
    accelSign = 0;                          /* the enhanced engine (Lupo 2 L2 0x00196518) */
    brakeSign = 0;
    lastAccel = 0.0f;
    lastBrake = 0.0f;
}

// RaceABMonitor::update (the enhanced engine)
//   Lupo 2's override (L2 0x00196570): the values become last frame's, the
//   start of the bars' trail (RaceDisplay::virtual_3 calls it after the
//   draw, hud.md C5).
void RaceABMonitor::update()
{
    lastAccel = accel;
    lastBrake = brake;
}

/* The enhanced engine: Lupo 2's look (hud.md C5).  The bars' colours and
 * the dark wells. */
#define AB_COLOR_BRAKE  RACE_COLOR_RED      /* 0x800000F0 */
#define AB_COLOR_ACCEL  RACE_COLOR_GREEN    /* 0x802BCC49 */
#define AB_SIGN_W       22                  /* a sign's height is (w - 4) * 16 / 22 */
#define AB_SIGN_H       16

// RaceABMonitor::drawBar (the enhanced engine)
//   Lupo 2's bar (L2 0x00196828): a 6-vertex strip in the w x h well at
//   (x, y), solid colour from the bottom up to the lower of now and last
//   (each clamped to 0..1), then fading to transparent up to the higher of
//   the two: the trail of the change since the last frame.
void RaceABMonitor::drawBar(u32 color, f32 x, f32 y, f32 w, f32 h, f32 now, f32 last)
{
    RaceDisplayVertex *v = (RaceDisplayVertex *)gfx_alloc_tail(5);
    u32 *c = (u32 *)gfx_alloc_tail(2);
    now = (now > 0.0f) ? now : 0.0f;
    now = (now < 1.0f) ? now : 1.0f;
    last = (last > 0.0f) ? last : 0.0f;
    last = (last < 1.0f) ? last : 1.0f;
    f32 hi = (now > last) ? now : last;
    f32 lo = (now < last) ? now : last;
    f32 bottom = y + h;
    f32 loY = bottom - h * lo;
    f32 hiY = bottom - h * hi;
    f32 right = x + w;

    v[0].x = x;     v[0].y = hiY;    v[0].z = 0;
    v[1].x = right; v[1].y = hiY;    v[1].z = 0;
    v[2].x = x;     v[2].y = loY;    v[2].z = 0;
    v[3].x = right; v[3].y = loY;    v[3].z = 0;
    v[4].x = x;     v[4].y = bottom; v[4].z = 0;
    v[5].x = right; v[5].y = bottom; v[5].z = 0;
    c[0] = 0;                               /* transparent at the trail's top */
    c[1] = 0;
    c[2] = color;
    c[3] = color;
    c[4] = color;
    c[5] = color;
    pgl_draw_strip(v, 0, c, 0, 6);
}

// FUNC 0x00198940 RaceABMonitor::draw
//   Draw two vertical bars growing up from y (race_fill_rect): the throttle
//   at x + w (green), the brake at x (red), each on a dark background the
//   size of the "accel_gage" image and under its frame image (white,
//   aligned left / top at the background's corner).  Quirk: both bars use
//   accel_gage's size (CLAMP MAXU / MAXV + 1, as int).
//   The enhanced engine (ENH_HUD_AB_MONITOR, hud.md C5; Lupo 2 L2
//   0x00196588), when the pack has both signs ("replay_sign_brake",
//   "replay_sign_accel"; else GT3's look): two columns w wide in the
//   object's aligned rectangle, 4 apart, each a translucent black box with
//   its sign at the bottom ((w - 4) * 16 / 22 high) and a dark well above
//   it; the brake's bar (red) left, the throttle's (green) right, each
//   trailing last frame's value (drawBar).  GT3's look also for a layout
//   that gives the monitor no size (GT3's records with h 0).
void RaceABMonitor::draw()
{
#if ENH_HUD_AB_MONITOR
    if (accelSign == 0)
        accelSign = race_display_find_new_image("replay_sign_accel");
    if (brakeSign == 0)
        brakeSign = race_display_find_new_image("replay_sign_brake");
    /* Lupo 2's look needs the record's size: GT3's racedetail.db gives the
     * monitor no height in some layouts (h 0: GT3's look ignores it), Lupo
     * 2's own records have 50 or 100 there; those keep GT3's look. */
    if (accelSign != 0 && brakeSign != 0 && w > 4 && h > 0) {
        g_002F1968.flush();
        gfx_enable(PGL_CAP_BLEND);
        gfx_blend_func(PGL_BLEND_ALPHA, 1.0f);
        int left = race_align_pos(x, w, alignX);
        int top = race_align_pos(y, h, alignY);
        int brakeX = left + 2;              /* the inner left of each column */
        int accelX = brakeX + w + 4;
        int innerY = top + 2;
        int innerW = w - 4;
        int signH = innerW * AB_SIGN_H / AB_SIGN_W;
        int signY = innerY + h - signH - 4;
        int wellH = h - signH - 8;
        pgl_color(RACE_COLOR_UNLIT);        /* 0x40000000 */
        race_fill_rect(left, top, w, h);
        race_fill_rect(left + w + 4, top, w, h);
        pgl_color(RACE_COLOR_WHITE);
        race_draw_image_rect(brakeSign, (f32)brakeX, (f32)signY, (f32)innerW, (f32)signH,
                             RACE_ALIGN_LEFT, RACE_ALIGN_TOP);
        race_draw_image_rect(accelSign, (f32)accelX, (f32)signY, (f32)innerW, (f32)signH,
                             RACE_ALIGN_LEFT, RACE_ALIGN_TOP);
        pgl_color(RACE_COLOR_DARK);
        race_fill_rect(brakeX, innerY, innerW, wellH);
        race_fill_rect(accelX, innerY, innerW, wellH);
        drawBar(AB_COLOR_BRAKE, (f32)brakeX, (f32)innerY, (f32)innerW, (f32)wellH, brake, lastBrake);
        drawBar(AB_COLOR_ACCEL, (f32)accelX, (f32)innerY, (f32)innerW, (f32)wellH, accel, lastAccel);
        return;
    }
#endif
    g_002F1968.flush();
    if (accelImage == 0)
        accelImage = race_display_find_image("accel_gage");
    u64 clamp = accelImage->tex->clamp;
    int gaugeW = (int)(((clamp >> 14) & 0x3FF) + 1);
    int gaugeH = (int)(((clamp >> 34) & 0x3FF) + 1);
    f32 fullH = (f32)gaugeH;

    /* each bar's locals in a block of their own (retail allocates them
     * ahead of `this') */
    {
        pgl_color(RACE_COLOR_DARK);
        int barX = race_align_pos(x + w, 0, alignX);
        int baseY = y;
        int barTop = baseY - gaugeH;
        int fillH = (int)(fullH * accel);
        race_fill_rect(barX, barTop, gaugeW, gaugeH);
        pgl_color(RACE_COLOR_GREEN);
        race_fill_rect(barX, baseY - fillH, gaugeW, fillH);
        pgl_color(RACE_COLOR_WHITE);
        race_draw_image(accelImage, RACE_ALIGN_LEFT, RACE_ALIGN_TOP, (f32)barX, (f32)barTop);
    }
    if (brakeImage == 0)
        brakeImage = race_display_find_image("brake_gage");
    {
        pgl_color(RACE_COLOR_DARK);
        int barX = race_align_pos(x, 0, alignX);
        int baseY = y;
        int barTop = baseY - gaugeH;
        int fillH = (int)(fullH * brake);
        race_fill_rect(barX, barTop, gaugeW, gaugeH);
        pgl_color(RACE_COLOR_RED);
        race_fill_rect(barX, baseY - fillH, gaugeW, fillH);
        pgl_color(RACE_COLOR_WHITE);
        race_draw_image(brakeImage, RACE_ALIGN_LEFT, RACE_ALIGN_TOP, (f32)barX, (f32)barTop);
    }
}

/* ======================================================================== */
/* RaceSteeringDisplay                                                      */
/* ======================================================================== */

// FUNC 0x00198B50 RaceSteeringDisplay::RaceSteeringDisplay
//   Caller: RaceDisplay::RaceDisplay.
RaceSteeringDisplay::RaceSteeringDisplay()
    : RaceDisplayObjectBase()
{
    wheel = 0;
    angle = 0.0f;
    sign = 0;                               /* the enhanced engine (Lupo 2 L2 0x00196960) */
    lastAngle = RACE_NO_ANGLE;
}

// RaceSteeringDisplay::update (the enhanced engine)
//   Lupo 2's override (L2 0x001969B8): the angle (in degrees, as Lupo 2
//   keeps it) becomes last frame's, the start of the motion blur
//   (RaceDisplay::virtual_3 calls it after the draw, hud.md C6).
void RaceSteeringDisplay::update()
{
    lastAngle = angle * -90.0f;
}

// RaceSteeringDisplay::resetTrail (the enhanced engine)
//   Lupo 2 L2 0x001969A8: no previous angle, so no blur (disableAll).
void RaceSteeringDisplay::resetTrail()
{
    lastAngle = RACE_NO_ANGLE;
}

/* 0x0032E878 (.rodata, between "brake_gage" and "steering") */
const RaceLine g_0032E878[1] = {
    { { 0.0f, -1.0f, 0.0f }, { 0.0f, -1.79f, 0.0f } },
};

// FUNC 0x00198B90 RaceSteeringDisplay::draw
//   Draw at (aligned x, y) (pgl_push_matrix, pgl_translate): the centre
//   marker line (g_0032E878, pgl_draw_line_3d scaled by 24 under a second
//   pushed matrix; a one-entry table walked by a count-down loop) in light
//   grey, then (pgl_pop_matrix, vu0_rotate_z) the "steering" image rotated
//   by angle * -90 degrees, white, centred horizontally with its top edge
//   43 above the pivot; pgl_pop_matrix.
//   The enhanced engine (ENH_HUD_STEERING_DISPLAY, hud.md C6): Lupo 2's
//   look (drawEnhanced) when the pack has "replay_sign_gtforce" and the
//   layout gives the display a size, else GT3's (drawGT3).
void RaceSteeringDisplay::draw()
{
#if ENH_HUD_STEERING_DISPLAY
    if (sign == 0)
        sign = race_display_find_new_image("replay_sign_gtforce");
    /* Lupo 2's wheel is the record's size: GT3's racedetail.db gives the
     * display none in some layouts (w = h = 0: GT3's look uses its image's
     * size), Lupo 2's own records have one; those keep GT3's look. */
    if (sign != 0 && w > 0 && h > 0) {
        drawEnhanced();
        return;
    }
#endif
    drawGT3();
}

/* The enhanced engine: Lupo 2's steering display (hud.md C6). */
#define STEER_PASSES   8                    /* the blurred copies of the wheel and the marker */
#define STEER_BLUR     0.125f               /* the spread between copies: (angle - last) / 8 */
#define STEER_MARK_ALPHA 0.16666667f        /* each marker copy blended FIX 1/6 (gp-0x7C14) */
#define STEER_SIGN_TOP (-83.0f)             /* the wheel image's top edge, in its own pixels */

/* The wheel's sub-pixel steps (Lupo 2 L2 0x003421C8: the needle's offsets,
 * GT3's g_0032F568, relative, summing to 0). */
static const f32 s_steerJitter[STEER_PASSES][2] = {
    {  0.5f,  0.0f  },
    { -0.5f,  0.5f  },
    {  0.5f,  0.0f  },
    { -0.5f, -0.25f },
    {  0.5f,  0.0f  },
    { -0.5f,  0.5f  },
    {  0.5f,  0.0f  },
    { -0.5f, -0.75f },
};

/* A tick / the marker: a thin quad from radius 0.85 to 1.15 straight up, in
 * units of the object's w / h (Lupo 2 L2 0x003422D0). */
static const RaceDisplayVertex s_steerTick[4] = {
    { -0.015f, -0.85f, 0 },
    { -0.015f, -1.15f, 0 },
    {  0.015f, -0.85f, 0 },
    {  0.015f, -1.15f, 0 },
};

// RaceSteeringDisplay::drawEnhanced (the enhanced engine)
//   Lupo 2's draw (L2 0x001969C8 / 0x00196C90): at the aligned (x, y), the
//   wheel sprite scaled to 2w x 2h (by its own size) and rotated by the
//   steering angle (angle * -90 degrees), drawn 8 times in white at alpha
//   0x10 with each copy turned a further (angle - last) / 8 and shifted by
//   the sub-pixel steps (the motion blur; none when there is no last
//   angle); then grey ticks at -90, 0 and +90 degrees and the red marker
//   at the angle, also 8 copies spread the same way, each blended at 1/6.
void RaceSteeringDisplay::drawEnhanced()
{
    g_002F1968.flush();
    f32 degrees = angle * -90.0f;
    f32 spread = 0.0f;
    if (lastAngle != RACE_NO_ANGLE)
        spread = (degrees - lastAngle) * STEER_BLUR;
    pgl_push_matrix();
    pgl_translate((f32)race_align_pos(x, 0, alignX), (f32)y, 0.0f);
    gfx_blend_func(PGL_BLEND_ALPHA, 1.0f);
    pgl_color(RACE_COLOR_WHEEL);
    pgl_push_matrix();
    {
        u64 clamp = sign->tex->clamp;
        f32 texW = (f32)(int)(((clamp >> 14) & 0x3FF) + 1);
        f32 texH = (f32)(int)(((clamp >> 34) & 0x3FF) + 1);
        f32 sx = (f32)w / texW;
        f32 sy = (f32)h / texH;
        vu0_scale(sx + sx, sy + sy, 1.0f);
    }
    vu0_rotate_z(degrees);
    for (int i = 0; i < STEER_PASSES; i++) {
        vu0_rotate_z(spread);
        race_draw_image(sign, RACE_ALIGN_CENTER, RACE_ALIGN_TOP, 0.0f, STEER_SIGN_TOP);
        gfx_offset_xy(s_steerJitter[i][0], s_steerJitter[i][1]);
    }
    pgl_pop_matrix();
    vu0_scale((f32)w, (f32)h, 1.0f);
    pgl_push_matrix();
    pgl_color(RACE_COLOR_PANEL_GREY);
    vu0_rotate_z(-90.0f);
    pgl_draw_strip(s_steerTick, 0, 0, 0, 4);
    vu0_rotate_z(90.0f);
    pgl_draw_strip(s_steerTick, 0, 0, 0, 4);
    vu0_rotate_z(90.0f);
    pgl_draw_strip(s_steerTick, 0, 0, 0, 4);
    pgl_pop_matrix();
    vu0_rotate_z(degrees);
    gfx_blend_func(PGL_BLEND_FIX, STEER_MARK_ALPHA);
    pgl_color(RACE_COLOR_MARKER);
    for (int i = 0; i < STEER_PASSES; i++) {
        vu0_rotate_z(spread);
        pgl_draw_strip(s_steerTick, 0, 0, 0, 4);
    }
    gfx_blend_func(PGL_BLEND_ALPHA, 1.0f);
    pgl_pop_matrix();
}

// RaceSteeringDisplay::drawGT3
//   GT3's draw (0x00198B90; the enhanced engine moved it here, see draw).
void RaceSteeringDisplay::drawGT3()
{
    g_002F1968.flush();
    if (wheel == 0)
        wheel = race_display_find_image("steering");
    pgl_push_matrix();
    pgl_color(RACE_COLOR_GREY);
    pgl_translate((f32)race_align_pos(x, 0, alignX), (f32)y, 0.0f);
    pgl_push_matrix();
    vu0_scale(24.0f, 24.0f, 1.0f);
    const RaceLine *line = g_0032E878;
    for (int i = 0; i < 1; i++, line++)
        pgl_draw_line_3d(line->from, line->to);
    pgl_pop_matrix();
    vu0_rotate_z(angle * -90.0f);
    pgl_color(RACE_COLOR_WHITE);
    race_draw_image(wheel, RACE_ALIGN_CENTER, RACE_ALIGN_TOP, 0.0f, -43.0f);
    pgl_pop_matrix();
}

/* ======================================================================== */
/* RaceTireWearDisplay                                                      */
/* ======================================================================== */

#define RACE_WHEELS 4                       /* RaceTireWearDisplay::color / warning */

// FUNC 0x00198CC0 RaceTireWearDisplay::RaceTireWearDisplay
//   Caller: RaceDisplay::RaceDisplay.  Every wheel dark, no warnings.
RaceTireWearDisplay::RaceTireWearDisplay()
    : RaceDisplayObjectBase()
{
    gauge = 0;
    for (int i = 0; i < RACE_WHEELS; i++) {
        color[i] = RACE_COLOR_DARK;
        warning[i] = 0;
    }
}

// FUNC 0x00198D30 RaceTireWearDisplay::draw
//   Draw (after Font::flush) with the alpha test on, blending off and
//   nearest texture filtering: the "tire_gage" image twice per wheel, its
//   alpha selecting the wheel's parts: alpha == 5 (i + 1) in the wheel's
//   colour, then alpha >= 5 (i + 1) + 40 in red (warning) or dark (the
//   later wheels draw over the earlier ones' >= bands).  The state is
//   restored after (alpha test off, blending on, linear filtering).
void RaceTireWearDisplay::draw()
{
    g_002F1968.flush();
    gfx_enable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    if (gauge == 0)
        gauge = race_display_find_image("tire_gage");
    u8 alphaRef = 5;
    for (int wheel = 0; wheel < RACE_WHEELS; wheel++) {
        gfx_alpha_func_raw(PGL_ATEST_EQUAL, alphaRef);
        pgl_color(color[wheel]);
        race_draw_image(gauge, alignX, alignY, (f32)x, (f32)y);
        gfx_alpha_func_raw(PGL_ATEST_GEQUAL, (u8)(alphaRef + 0x28));
        pgl_color(warning[wheel] ? RACE_COLOR_RED : RACE_COLOR_DARK);
        race_draw_image(gauge, alignX, alignY, (f32)x, (f32)y);
        alphaRef = (u8)(alphaRef + 5);
    }
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
}

/* ======================================================================== */
/* RaceSpeedDisplay                                                         */
/* ======================================================================== */

// FUNC 0x00198EB0 RaceSpeedDisplay::RaceSpeedDisplay
//   Caller: RaceDisplay::RaceDisplay (two displays).  No speed (-1).
RaceSpeedDisplay::RaceSpeedDisplay()
    : RaceValueDisplayBase()
{
    speed = -1.0f;
}

// FUNC 0x00198EF0 RaceSpeedDisplay::draw
//   Draw, right-aligned (Font::setAlign) at the aligned x: the speed unit
//   (UnitSettings::speedName, "sw_12"; passed to Font::print as the
//   format), the speed in that unit (UnitSettings::convSpeed,
//   race_format_speed; "sw_16") left of it, and the label left of a
//   column at least 90 wide (value and unit together).
//   Matching (98.4%): as RaceTimeDisplay::draw, retail computes the max
//   before loading the 10.0.
void RaceSpeedDisplay::draw()
{
    char text[32];

    g_002F1968.setColor(color);
    g_002F1968.setAlign(FONT_ALIGN_RIGHT);
    f32 right = (f32)race_align_pos(x, 0, alignX);
    f32 posY = (f32)y;
    g_002F1968.setPos3(right, posY, 0.0f);
    g_002F1968.select("sw_12");
    const char *unit = g_002F59D8.speedName();
    g_002F1968.print(unit);
    f32 invScale = 1.0f / g_002F1968.getScaleRawX();
    f32 width = text_string_width(&g_002F1968, unit) * invScale;   /* the unit's */
    g_002F1968.select("sw_16");
    race_format_speed(text, g_002F59D8.convSpeed(speed));
    g_002F1968.setPos3(right - width, posY, 0.0f);
    g_002F1968.print(text);
    f32 valueW = width + text_string_width(&g_002F1968, text) * invScale;
    /* the same variable, now the label column (retail keeps both in $f21) */
    width = (valueW > RACE_VALUE_MIN_WIDTH ? valueW : RACE_VALUE_MIN_WIDTH) + RACE_LABEL_GAP;   /* max.s */
    g_002F1968.select("sw_12");
    if (labelWidth == 0.0f)
        labelWidth = text_string_width(&g_002F1968, label) * invScale;
    g_002F1968.setPos3((right - width) - labelWidth, posY, 0.0f);
    g_002F1968.putString(label);
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
}

/* ======================================================================== */
/* RaceCarIconDisplay                                                       */
/* ======================================================================== */

#define CAR_ICON_W 64                       /* CarIcon: 64 x 32 x 4 bytes */
#define CAR_ICON_H 32

/* draw's 5 qwords of gfx_alloc_tail: the strip's vertices, then its uvs
 * (+0x30; retail stores them through the vertices' base). */
struct CarIconStrip {
    RaceDisplayVertex v[4];                 /* 0x00 */
    RaceDisplayUV     uv[4];                /* 0x30 */
};

// FUNC 0x001990F0 RaceCarIconDisplay::RaceCarIconDisplay
//   Caller: RaceDisplay::RaceDisplay (two icons).
RaceCarIconDisplay::RaceCarIconDisplay()
    : RaceDisplayObjectBase()
{
    icon = 0;
    timer = 0.0f;
    duration = 0.0f;
    blink = 0;
}

// FUNC 0x00199138 RaceCarIconDisplay::isVisible
//   ? name.  Caller: draw.  As RaceMessageDisplay::isVisible.
bool RaceCarIconDisplay::isVisible()
{
    bool visible = false;
    if (enabled) {
        if (duration == 0.0f)
            visible = true;
        else if (duration > 0.0f && timer >= 0.0f && timer <= duration)
            visible = true;
    }
    return visible;
}

// FUNC 0x00199190 RaceCarIconDisplay::update
//   Update: as RaceMessageDisplay::update (RACE_FRAME_TIME, gp-0x7C40).
void RaceCarIconDisplay::update()
{
    if (timer < duration)
        timer = timer + RACE_FRAME_TIME;
    if (duration >= 0.0f) {
        if (blink > 0)
            blink = blink - 1;
    }
}

// FUNC 0x001991D8 RaceCarIconDisplay::draw
//   Draw (with an icon, visible, not in the off half of a blink; after
//   Font::flush): a textured 4-vertex strip (5 qwords of gfx_alloc_tail:
//   the vertices, the uvs at +0x30) of the icon's 64 x 32 pixels at the
//   rectangle's far corner (x + w, y + h) aligned for 64 x 32.  The uvs
//   cover the image in its power-of-two texture (doubled from 1 while
//   <= 0x400 and below the size: 64 x 32 itself, so 1.0); the pixels are
//   made resident and selected (VramAllocator::loadImage, PSMCT32), the
//   texture region clamped to 0..63 x 0..31 (gfx_set_texture_region, wrap
//   REGION_CLAMP, left set), filtering nearest while drawing, white.
void RaceCarIconDisplay::draw()
{
    if (icon == 0)
        return;
    if (!isVisible())
        return;
    if ((blink / (RACE_BLINK_FRAMES / 2)) & 1)
        return;
    g_002F1968.flush();
    CarIconStrip *s = (CarIconStrip *)gfx_alloc_tail(5);
    /* the icon's size in variables: the loops keep both tests (with
     * constants fold merges them into one), and (f32)iconW is the one
     * 64.0 of the corner and the uv */
    int iconW = CAR_ICON_W;
    int iconH = CAR_ICON_H;
    f32 left = (f32)race_align_pos(x + w, iconW, alignX);
    f32 top = (f32)race_align_pos(y + h, iconH, alignY);
    f32 right = left + (f32)iconW;
    f32 bottom = top + (f32)iconH;
    s->v[0].x = left;  s->v[0].y = top;    s->v[0].z = 0;
    s->v[1].x = right; s->v[1].y = top;    s->v[1].z = 0;
    s->v[2].x = left;  s->v[2].y = bottom; s->v[2].z = 0;
    s->v[3].x = right; s->v[3].y = bottom; s->v[3].z = 0;
    int texW = 1;
    while (texW < 0x401 && texW < iconW)
        texW <<= 1;
    f32 maxU = (f32)iconW / (f32)texW;
    int texH = 1;
    while (texH < 0x401 && texH < iconH)
        texH <<= 1;
    f32 maxV = (f32)iconH / (f32)texH;
    s->uv[0].u = 0.0f; s->uv[0].v = 0.0f;
    s->uv[1].u = maxU; s->uv[1].v = 0.0f;
    s->uv[2].u = 0.0f; s->uv[2].v = maxV;
    s->uv[3].u = maxU; s->uv[3].v = maxV;
    g_002F4250.loadImage(SCE_GS_PSMCT32, iconW, iconH, icon->pixels);
    gfx_set_texture_region(PGL_REGION_U, 0, CAR_ICON_W - 1);
    gfx_set_texture_region(PGL_REGION_V, 0, CAR_ICON_H - 1);
    gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REGION_CLAMP);
    gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REGION_CLAMP);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    pgl_color(RACE_COLOR_WHITE);
    pgl_draw_strip(s->v, 0, 0, s->uv, 4);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
}

/* ======================================================================== */
/* RaceReplayModeDisplay                                                    */
/* ======================================================================== */

// FUNC 0x00199420 RaceReplayModeDisplay::RaceReplayModeDisplay
//   Caller: RaceDisplay::RaceDisplay.  No mode.
RaceReplayModeDisplay::RaceReplayModeDisplay()
    : RaceDisplayObjectBase()
{
    mode = -1;
    l2 = 0;
    r2 = 0;
    modeImage = 0;
    time = 0.0f;
}

// FUNC 0x00199470 RaceReplayModeDisplay::update
//   Update: time counts down (RACE_FRAME_TIME, gp-0x7C3C) to 0.
//   Matching (78.6%): retail loads the literal before `time'; in sched2
//   our load of `time' has one more dependent (the store's anti-dependence)
//   and wins the tie.  15 forms tried (operators, helpers by reference /
//   value with the step as an argument, locals, compares); the reference
//   compiler the same.  (The data reference the scorer shows is that
//   misaligned pair of loads.)
void RaceReplayModeDisplay::update()
{
    f32 left = time - RACE_FRAME_TIME;
    time = (left > 0.0f) ? left : 0.0f;         /* max.s */
}

// FUNC 0x00199490 RaceReplayModeDisplay::draw
//   Draw (after Font::flush), while time > 0 and a mode image is set:
//   "replay_mode_l2", the mode image, "replay_mode_r2" (looked up on the
//   first draw) in a row at y aligned by alignX / top, the x advancing by
//   each image's width (CLAMP MAXU + 1, 64-bit sum truncated to int), in
//   white at alpha min(4 t^2, 1) as the time runs out.
//   Matching (92.7%): retail keeps the 1.0 in $f12 and takes the min before
//   the argument moves; ours moves first and puts the min.s in the jal
//   slot (10 min / mul forms tried).
void RaceReplayModeDisplay::draw()
{
    g_002F1968.flush();
    f32 left = time;
    if (left <= 0.0f)
        return;
    if (modeImage == 0)
        return;
    left *= left;
    left *= 4.0f;
    left = (left < 1.0f) ? left : 1.0f;         /* min.s */
    pgl_color4f(1.0f, 1.0f, 1.0f, left);
    int posY = y;
    int posX = x;
    if (l2 == 0)
        l2 = race_display_find_image("replay_mode_l2");
    if (r2 == 0)
        r2 = race_display_find_image("replay_mode_r2");
    f32 rowY = (f32)posY;
    race_draw_image(l2, alignX, RACE_ALIGN_TOP, (f32)posX, rowY);
    posX += ((l2->tex->clamp >> 14) & 0x3FF) + 1;          /* in 64 bits, truncated */
    race_draw_image(modeImage, alignX, RACE_ALIGN_TOP, (f32)posX, rowY);
    posX += ((modeImage->tex->clamp >> 14) & 0x3FF) + 1;
    race_draw_image(r2, alignX, RACE_ALIGN_TOP, (f32)posX, rowY);
}

/* ======================================================================== */
/* The inline members of the class bodies, emitted at the end of the TU     */
/* (per class, in reverse order of the classes): the accessors, the         */
/* typeinfo function and the destructor (the four user-declared destructors */
/* before the __tf function, the implicit ones after it).  None of them has */
/* a caller.  The __tf functions build an __si_type_info on the base's node */
/* (__ti21RaceDisplayObjectBase, gp-0x4650, or __ti20RaceValueDisplayBase). */
/* ======================================================================== */

/* ---- RaceReplayModeDisplay ---------------------------------------------- */

// FUNC 0x00199600 RaceReplayModeDisplay::modeImageName
//   ? name (no `this': a static member or a file-level inline).  Caller:
//   setMode.  The image of a replay camera mode.  The cases in retail's
//   block and string order (0, 2, 1, 3).
const char *RaceReplayModeDisplay::modeImageName(int mode)
{
    switch (mode) {
    case 0:
        return "replay_mode_traditional";
    case 2:
        return "replay_mode_broadcast";
    case 1:
        return "replay_mode_avantgarde";
    case 3:
        return "replay_mode_crsline";
    default:
        return "replay_mode_driver";
    }
}

// FUNC 0x00199670 RaceReplayModeDisplay::setMode
//   ? name.  No callers (expanded in RaceDisplay).  A new mode looks its
//   image up (modeImageName, race_display_find_image).
void RaceReplayModeDisplay::setMode(int m)
{
    if (mode != m) {
        mode = m;
        modeImage = race_display_find_image(modeImageName(m));
    }
}

/* ======================================================================== */
/* RaceCapacityMonitor (the enhanced engine)                                */
/* ======================================================================== */

/* The gauge's colours (Lupo 2 gp constants 0.8, 0.2745, 0.55, 0.1, 0.25):
 * full charge (1) blue, the motor drawing (0) magenta, lerped. */
#define CAPACITY_BLUE_R      0.0f
#define CAPACITY_BLUE_G      0.27450982f
#define CAPACITY_BLUE_B      0.8f
#define CAPACITY_MAGENTA_R   0.55f
#define CAPACITY_MAGENTA_G   0.1f
#define CAPACITY_MAGENTA_B   0.25f
/* The sign is (w - 4) * 5 / 7 high (Lupo 2: * 20 / 28). */
#define CAPACITY_SIGN_W      28
#define CAPACITY_SIGN_H      20

// RaceCapacityMonitor::RaceCapacityMonitor (the enhanced engine)
//   Lupo 2 L2 0x001976D0.  Caller: RaceDisplay::RaceDisplay (a member).
RaceCapacityMonitor::RaceCapacityMonitor()
    : RaceDisplayObjectBase()
{
    sign = 0;
    hybrid = 0;
    capacity = 0.0f;
    charge = 0.0f;
}

// RaceCapacityMonitor::draw (the enhanced engine)
//   Lupo 2 L2 0x00197720 (hud.md C1).  Nothing unless the car shown is a
//   hybrid.  In the aligned w x h rectangle: a translucent black box, a dark
//   inner box 2 pixels in, the "meter_sign_dualnote" sign in blue at its
//   bottom ((w - 4) * 5 / 7 high; without the sprite the bar takes its
//   place), and above it the bar of the battery level (capacity) in the
//   colour of the flow (charge: 1 blue, 0 magenta).
void RaceCapacityMonitor::draw()
{
    if (hybrid == 0)
        return;
    if (sign == 0)
        sign = race_display_find_new_image("meter_sign_dualnote");
    g_002F1968.flush();
    int innerW = w - 4;
    int signH = 0;
    if (sign != 0)
        signH = innerW * CAPACITY_SIGN_H / CAPACITY_SIGN_W;
    int left = race_align_pos(x, w, alignX);
    int innerX = left + 2;
    int top = race_align_pos(y, h, alignY);
    int signY = top + h - signH;            /* the sign's top + 2 */
    int barBottom = signY - 4;
    int barH = (int)((f32)(barBottom - top - 2) * capacity);
    signY -= 2;
    pgl_color(RACE_COLOR_UNLIT);            /* 0x40000000 */
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(PGL_BLEND_ALPHA, 1.0f);
    race_fill_rect(left, top, w, h);
    pgl_color(RACE_COLOR_DARK);             /* 0x80202020 */
    race_fill_rect(innerX, top + 2, innerW, h - 4);
    if (sign != 0) {
        pgl_color(RACE_COLOR_SIGN_BLUE);
        race_draw_image_rect(sign, (f32)innerX, (f32)signY, (f32)innerW, (f32)signH,
                             RACE_ALIGN_LEFT, RACE_ALIGN_TOP);
    }
    f32 c = charge;
    f32 d = 1.0f - c;
    pgl_color4f(c * CAPACITY_BLUE_R + d * CAPACITY_MAGENTA_R,
                c * CAPACITY_BLUE_G + d * CAPACITY_MAGENTA_G,
                c * CAPACITY_BLUE_B + d * CAPACITY_MAGENTA_B, 1.0f);
    race_fill_rect(innerX, barBottom - barH, innerW, barH);
}

// FUNC 0x001996B8 RaceReplayModeDisplay::show
//   ? name.  Shown for t seconds.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001996C0 __tf21RaceReplayModeDisplay
//   __si_type_info __ti21RaceReplayModeDisplay (0x004B22D0, name
//   0x0032F010), base __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf21RaceReplayModeDisplay(void)
{
    if (__ti21RaceReplayModeDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti21RaceReplayModeDisplay, "21RaceReplayModeDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti21RaceReplayModeDisplay;
}
#endif

// FUNC 0x00199710 RaceReplayModeDisplay::~RaceReplayModeDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceCarIconDisplay ------------------------------------------------- */

// FUNC 0x00199730 RaceCarIconDisplay::isFinished
//   ? name.  duration <= timer.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199758 RaceCarIconDisplay::setBlink
//   ? name.  n blinks of RACE_BLINK_FRAMES.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199768 RaceCarIconDisplay::show
//   ? name (as RaceMessageDisplay::show).
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199778 RaceCarIconDisplay::setIcon
//   ? name.  index.txt names this RaceLapDisplay::setCurrentLaps(int)
//   (lupo2m, a code match: the same two instructions, sw $a1, 0x18($a0)),
//   but it sits in RaceCarIconDisplay's group (before
//   __tf18RaceCarIconDisplay); RaceLapDisplay's own copy is 0x0019A0C8.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199780 __tf18RaceCarIconDisplay
//   __si_type_info __ti18RaceCarIconDisplay (0x004B22C0, name 0x0032F028),
//   base __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf18RaceCarIconDisplay(void)
{
    if (__ti18RaceCarIconDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti18RaceCarIconDisplay, "18RaceCarIconDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti18RaceCarIconDisplay;
}
#endif

// FUNC 0x001997D0 RaceCarIconDisplay::~RaceCarIconDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceSpeedDisplay --------------------------------------------------- */

// FUNC 0x001997F0 RaceSpeedDisplay::setSpeed
//   index.txt gives this name to 0x00199D30; this copy is the one in
//   RaceSpeedDisplay's group (swc1 $f12, 0x44($a0) in both).
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001997F8 __tf16RaceSpeedDisplay
//   __si_type_info __ti16RaceSpeedDisplay (0x004B22B0, name 0x0032F040),
//   base __tf20RaceValueDisplayBase's node (0x004B1260).
extern "C" const type_info &__tf16RaceSpeedDisplay(void)
{
    if (__ti16RaceSpeedDisplay[0] == 0) {
        __tf20RaceValueDisplayBase();
        __rtti_si(__ti16RaceSpeedDisplay, "16RaceSpeedDisplay",
                  (const type_info *)__ti20RaceValueDisplayBase);
    }
    return *(const type_info *)__ti16RaceSpeedDisplay;
}
#endif

// FUNC 0x00199848 RaceSpeedDisplay::~RaceSpeedDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags (RaceValueDisplayBase
 * has no destructor of its own) */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceTireWearDisplay ------------------------------------------------ */

// FUNC 0x00199868 RaceTireWearDisplay::setWarning
//   ? name.  Stored as 0 / 1.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199888 RaceTireWearDisplay::setColor
//   ? name.  An opaque GS colour 0x80BBGGRR.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x001998B0 RaceTireWearDisplay::~RaceTireWearDisplay
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy: the
 * vptr store (_vt$19RaceTireWearDisplay 0x0032EA50), then
 * ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001998D8 __tf19RaceTireWearDisplay
//   __si_type_info __ti19RaceTireWearDisplay (0x004B2210, name
//   0x0032F058), base __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf19RaceTireWearDisplay(void)
{
    if (__ti19RaceTireWearDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti19RaceTireWearDisplay, "19RaceTireWearDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti19RaceTireWearDisplay;
}
#endif

/* ---- RaceSteeringDisplay ------------------------------------------------ */

// FUNC 0x00199928 RaceSteeringDisplay::setAngle
//   ? name.  index.txt names this RaceRankDisplay::setScale(float) (lupo2m,
//   a code match: swc1 $f12, 0x1C($a0)), but it sits in
//   RaceSteeringDisplay's group (before ~RaceSteeringDisplay), whose +0x1C
//   is the float angle; RaceRankDisplay's own copy is 0x0019A048.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199930 RaceSteeringDisplay::~RaceSteeringDisplay
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy: the
 * vptr store (_vt$19RaceSteeringDisplay 0x0032EAA8), then
 * ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199958 __tf19RaceSteeringDisplay
//   __si_type_info __ti19RaceSteeringDisplay (0x004B2220, name
//   0x0032F070), base __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf19RaceSteeringDisplay(void)
{
    if (__ti19RaceSteeringDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti19RaceSteeringDisplay, "19RaceSteeringDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti19RaceSteeringDisplay;
}
#endif

/* ---- RaceABMonitor ------------------------------------------------------ */

// FUNC 0x001999A8 RaceABMonitor::setBrake
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x001999B0 RaceABMonitor::setAccel
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x001999B8 RaceABMonitor::~RaceABMonitor
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy: the
 * vptr store (_vt$13RaceABMonitor 0x0032EB00), then ~RaceDisplayObjectBase
 * with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001999E0 __tf13RaceABMonitor
//   __si_type_info __ti13RaceABMonitor (0x004B2200, name 0x0032F088), base
//   __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf13RaceABMonitor(void)
{
    if (__ti13RaceABMonitor[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti13RaceABMonitor, "13RaceABMonitor",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti13RaceABMonitor;
}
#endif

/* ---- RaceBackPanel ------------------------------------------------------ */

// FUNC 0x00199A30 RaceBackPanel::~RaceBackPanel
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy: the
 * vptr store (_vt$13RaceBackPanel 0x0032EB58), then ~RaceDisplayObjectBase
 * with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199A58 __tf13RaceBackPanel
//   __si_type_info __ti13RaceBackPanel (0x004B2260, name 0x0032F098), base
//   __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf13RaceBackPanel(void)
{
    if (__ti13RaceBackPanel[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti13RaceBackPanel, "13RaceBackPanel",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti13RaceBackPanel;
}
#endif

/* ---- RaceIndicators ----------------------------------------------------- */

// FUNC 0x00199AA8 RaceIndicators::setOIL
//   Stored as a byte.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199AB0 RaceIndicators::setBrake
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199AB8 RaceIndicators::setTCS
//   LAMP_OFF / LAMP_ON / LAMP_BLINK.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199AC0 RaceIndicators::setVSC
//   LAMP_OFF / LAMP_ON / LAMP_BLINK.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199AC8 __tf14RaceIndicators
//   __si_type_info __ti14RaceIndicators (0x004B2240, name 0x0032F0A8), base
//   __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf14RaceIndicators(void)
{
    if (__ti14RaceIndicators[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti14RaceIndicators, "14RaceIndicators",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti14RaceIndicators;
}
#endif

// FUNC 0x00199B18 RaceIndicators::~RaceIndicators
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceEventDisplay --------------------------------------------------- */

// FUNC 0x00199B38 RaceEventDisplay::getLastEventEnd
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199B40 RaceEventDisplay::getLastEventBegin
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199B48 RaceEventDisplay::setOffset
//   The offset of the next addEvent only.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199B58 RaceEventDisplay::setColor
//   ? name.  The colour of the next events.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199B60 __tf16RaceEventDisplay
//   __si_type_info __ti16RaceEventDisplay (0x004B2280, name 0x0032F0C0),
//   base __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf16RaceEventDisplay(void)
{
    if (__ti16RaceEventDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti16RaceEventDisplay, "16RaceEventDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti16RaceEventDisplay;
}
#endif

// FUNC 0x00199BB0 RaceEventDisplay::~RaceEventDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: messages[7]..[0] through their virtual destructors
 * (~RaceMessageDisplay, vtable slot 0, flags 0), then
 * ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceMessageDisplay ------------------------------------------------- */

// FUNC 0x00199C30 RaceMessageDisplay::isFinished
//   ? name (expanded in RaceEventDisplay::addEvent).  duration <= timer.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199C58 RaceMessageDisplay::setBlink
//   ? name.  n blinks of RACE_BLINK_FRAMES.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199C68 RaceMessageDisplay::setFadeTime
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199C78 RaceMessageDisplay::setColor
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199C80 RaceMessageDisplay::show
//   Shown for duration seconds after delay.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199C90 RaceMessageDisplay::clear
//   ? name (expanded in RaceEventDisplay::reset).  setText with the empty
//   text (0x00352028); the timing is kept (duration -1 is not > 0).
/* implicit: setText (the inline body) */
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199CC0 __tf18RaceMessageDisplay
//   __si_type_info __ti18RaceMessageDisplay (0x004B22E0, name 0x0032F0D8),
//   base __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf18RaceMessageDisplay(void)
{
    if (__ti18RaceMessageDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti18RaceMessageDisplay, "18RaceMessageDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti18RaceMessageDisplay;
}
#endif

// FUNC 0x00199D10 RaceMessageDisplay::~RaceMessageDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceLapTimesDisplay ------------------------------------------------ */

// FUNC 0x00199D30 RaceLapTimesDisplay::setLineSpacing
//   ? name.  index.txt names this RaceSpeedDisplay::setSpeed(float) (lupo2m,
//   a code match: swc1 $f12, 0x44($a0)), but it sits in
//   RaceLapTimesDisplay's group (before __tf19RaceLapTimesDisplay), whose
//   +0x44 is the float lineSpacing; RaceSpeedDisplay's own copy is
//   0x001997F0.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199D38 RaceLapTimesDisplay::setLines
//   ? name.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199D40 __tf19RaceLapTimesDisplay
//   __si_type_info __ti19RaceLapTimesDisplay (0x004B2270, name
//   0x0032F0F0), base __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf19RaceLapTimesDisplay(void)
{
    if (__ti19RaceLapTimesDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti19RaceLapTimesDisplay, "19RaceLapTimesDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti19RaceLapTimesDisplay;
}
#endif

// FUNC 0x00199D90 RaceLapTimesDisplay::~RaceLapTimesDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceTimeDisplay ---------------------------------------------------- */

// FUNC 0x00199DB0 RaceTimeDisplay::getImmediateTimeValue
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199DB8 RaceTimeDisplay::setTime
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199DC0 __tf15RaceTimeDisplay
//   __si_type_info __ti15RaceTimeDisplay (0x004B21F0), base
//   __tf20RaceValueDisplayBase's node (0x004B1260).
extern "C" const type_info &__tf15RaceTimeDisplay(void)
{
    if (__ti15RaceTimeDisplay[0] == 0) {
        __tf20RaceValueDisplayBase();
        __rtti_si(__ti15RaceTimeDisplay, "15RaceTimeDisplay",
                  (const type_info *)__ti20RaceValueDisplayBase);
    }
    return *(const type_info *)__ti15RaceTimeDisplay;
}
#endif

// FUNC 0x00199E10 RaceTimeDisplay::~RaceTimeDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags (RaceValueDisplayBase
 * has no destructor of its own) */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceValueDisplayBase ----------------------------------------------- */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199E30 __tf20RaceValueDisplayBase
//   __si_type_info __ti20RaceValueDisplayBase (0x004B1260), base
//   __tf21RaceDisplayObjectBase's node.  Callers: the two derived __tf
//   functions.
extern "C" const type_info &__tf20RaceValueDisplayBase(void)
{
    if (__ti20RaceValueDisplayBase[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti20RaceValueDisplayBase, "20RaceValueDisplayBase",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti20RaceValueDisplayBase;
}
#endif

// FUNC 0x00199E80 RaceValueDisplayBase::~RaceValueDisplayBase
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceGearDisplay ---------------------------------------------------- */

// FUNC 0x00199EA0 RaceGearDisplay::setSuggestBlink
//   ? name.  Stored as a byte (sb).
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199EA8 RaceGearDisplay::setBright
//   ? name.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199EB0 RaceGearDisplay::setLampOffset
//   ? name.  The shift lamp relative to the gear.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199EC0 RaceGearDisplay::setShiftWarning
//   ? name.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199EC8 RaceGearDisplay::setSuggestedGear
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199ED0 RaceGearDisplay::setGear
//   ? name.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199ED8 __tf15RaceGearDisplay
//   __si_type_info __ti15RaceGearDisplay (0x004B2230), base
//   __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf15RaceGearDisplay(void)
{
    if (__ti15RaceGearDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti15RaceGearDisplay, "15RaceGearDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti15RaceGearDisplay;
}
#endif

// FUNC 0x00199F28 RaceGearDisplay::~RaceGearDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceOdometer ------------------------------------------------------- */

// FUNC 0x00199F48 RaceOdometer::setValue
//   ? name.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199F50 __tf12RaceOdometer
//   __si_type_info __ti12RaceOdometer (0x004B22A0), base
//   __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf12RaceOdometer(void)
{
    if (__ti12RaceOdometer[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti12RaceOdometer, "12RaceOdometer",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti12RaceOdometer;
}
#endif

// FUNC 0x00199FA0 RaceOdometer::~RaceOdometer
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceDigitalSpeedmeter ---------------------------------------------- */

// FUNC 0x00199FC0 RaceDigitalSpeedmeter::setUnitOffset
//   ? name.  The speed unit relative to the number.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x00199FD0 RaceDigitalSpeedmeter::setSpeed
//   ? name.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x00199FD8 __tf21RaceDigitalSpeedmeter
//   __si_type_info __ti21RaceDigitalSpeedmeter (0x004B2290), base
//   __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf21RaceDigitalSpeedmeter(void)
{
    if (__ti21RaceDigitalSpeedmeter[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti21RaceDigitalSpeedmeter, "21RaceDigitalSpeedmeter",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti21RaceDigitalSpeedmeter;
}
#endif

// FUNC 0x0019A028 RaceDigitalSpeedmeter::~RaceDigitalSpeedmeter
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceRankDisplay ---------------------------------------------------- */

// FUNC 0x0019A048 RaceRankDisplay::setScale
//   index.txt gives this name to 0x00199928; this copy is the one in
//   RaceRankDisplay's group.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x0019A050 RaceRankDisplay::setRank
//   ? name.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0019A058 __tf15RaceRankDisplay
//   __si_type_info __ti15RaceRankDisplay (0x004B2250), base
//   __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf15RaceRankDisplay(void)
{
    if (__ti15RaceRankDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti15RaceRankDisplay, "15RaceRankDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti15RaceRankDisplay;
}
#endif

// FUNC 0x0019A0A8 RaceRankDisplay::~RaceRankDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* ---- RaceLapDisplay ----------------------------------------------------- */

// FUNC 0x0019A0C8 RaceLapDisplay::setCurrentLaps
//   index.txt gives this name to 0x00199778; this copy is the one in
//   RaceLapDisplay's group.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

// FUNC 0x0019A0D0 RaceLapDisplay::setTotalLaps
//   ? name.
/* inline in game/race/display/T157_race_display_parts.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0019A0D8 __tf14RaceLapDisplay
//   __si_type_info __ti14RaceLapDisplay (0x004B22F0), base
//   __tf21RaceDisplayObjectBase's node.
extern "C" const type_info &__tf14RaceLapDisplay(void)
{
    if (__ti14RaceLapDisplay[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti14RaceLapDisplay, "14RaceLapDisplay",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti14RaceLapDisplay;
}
#endif

// FUNC 0x0019A128 RaceLapDisplay::~RaceLapDisplay
/* the implicit destructor (game/race/display/T157_race_display_parts.h); egcs emits
 * this copy: ~RaceDisplayObjectBase with the same flags */
/* implicit: ~RaceDisplayObjectBase (base destructor) */
