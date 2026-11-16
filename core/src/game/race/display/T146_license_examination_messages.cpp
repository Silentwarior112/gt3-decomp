/* T146 - LicenseExaminationMessages, LicensePrize, ExamMessageLine and the
 * text helpers of the licence result (u049).
 *
 * LicenseExaminationMessages is the result screen of a licence test, run
 * by RaceLicenseDisplay (T147):
 *   - passed (setup(kind > 0, ...), then updateWhenPassing / renderWhenPassing every
 *     frame): a dark screen fades in (fade0, 1/30 per frame), the trophy's
 *     BGM event and GameStatus small-trophy bit are posted (if `report'), a
 *     wait (180 or 30 frames), then the trophy model (gold for kinds 1 / 2,
 *     silver 3, bronze 4 / 5) is shown, its rank name fades in under it
 *     (fade1) and the time line fades in; finished 300 frames later;
 *   - failed (setup(-1, ...), then updateWhenDisqualifying /
 *     renderWhenDisqualifying): a blue band and the failure text ("time
 *     over" / "disqualified") fade in together (1/60 per frame), held 180
 *     frames, fade out, then finished.
 * The three trophy models are loaded once by the constructor into a unit of
 * the car model arena g_002C2F50 (0x32000 bytes each).
 *
 * ExaminationDisplayer's methods are inline (T141): this unit has
 * out-of-line copies of them too (0x0018BD00..0x0018BDAC), unused (the
 * vtables hold T141's).  The fade tests in double come from double
 * literals in the source (fade < 1.0).
 *
 * Data (u049's data pass): retail's .rodata 0x0032C6D0..0x0032C9B6 is
 * "bad_alloc" (<new>), the jump tables of setup, updateWhenPassing (two)
 * and renderWhenPassing, the vtables of LicenseExaminationMessages,
 * ExaminationDisplayer (T146's own copy) and LicensePrize, then the type
 * names GarageWheelList .. Cell<GarageCar> (the GameStatus),
 * RaceValueDisplayBase, LicenseExaminationMessages, ExaminationDisplayer,
 * LicensePrize, UnitArena<64>, CallBackWhenPrizeRendering,
 * PrizeModelSetContext: in reverse definition order the prize classes
 * (T145) first, UnitArena<64> (T153, emitted), T146's own classes with
 * ExaminationDisplayer between them (T146's header), the race display
 * parts (T157: RaceValueDisplayBase only, no T154) and the GameStatus last.
 * Its .sdata 0x00351F10..0x00351F1E is the "" of T157's inline
 * RaceMessageDisplay::clear, then "sw_20".  Neither "bad_alloc" nor the ""
 * is referenced: anchors.
 */
// DATA_START 0x0032C6D0
// DATA_START 0x00351F10
#include "types.h"
#include "libc.h"                           /* memset, strlen */
#include <new>                              /* "bad_alloc" (retail's first .rodata string) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si, __rtti_user (egcs's <new> and cxxrt.h conflict) */
#endif
#include "game/menu/framework/T188_color.h"                /* Color8 (before the classes: its inline copies are the
                                               last functions of the unit, reverse definition order) */
#include "game/race/modes/T145_mini_prize.h"           /* MiniPrize, PrizeId, prize_rank_name */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, after the prize classes' */
#include "game/race/core/T153_race_course.h"
#include "game/race/display/T146_license_examination_messages.h"
#include "game/race/display/T157_race_display_parts.h"   /* (no use: RaceValueDisplayBase and the "", which retail's T146 has) */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::setSmallTrophy */
#define GAME_T180_INLINE_BLOCK_ACCESSORS    /* RaceCarModel::allocBlock / freeBlock inline (retail expands them) */
#include "game/car/model/T180_race_car_model.h"       /* RaceCarModel::allocBlock / freeBlock (a block of the model
                                               arena g_002C2F50, UnitArena<64>, T153) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer (width, displayHeight) */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/text/T499_font_metrics.h"         /* text_string_width */
#include "engine/gfx/T512_render_context.h"       /* g_002F424C: the 16:9 screen mode */
#include "game/race/display/T158_race_display_draw.h"    /* race_fill_rect, race_format_time */
#include "engine/pgl/G001_flare.h"               /* gfx_enable / disable / blend_func / color_mask / tex_param, pgl_color */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization::eventQueue */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post, RACE_EVENT_BGM */
#include "game/race/display/race_license_internal.h"

/* draw_shadow_text with the four components of c. */
void draw_shadow_text_color(const char *text, const Color8 *c, int x, int y, int d);    /* 0x0018A900 */
/* (? name, unreferenced) text centred at y, each character's alpha a ramp
 * moving with progress (0..1). */
void draw_wave_text(const char *text, u32 rgb, u8 maxAlpha, int y, f32 progress);       /* 0x0018A960 */

/* ------------------------------------------------------------------------ */
/* Text helpers                                                             */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018A798 draw_shadow_text
//   The text with a black drop shadow: the shadow (0, 0, 0, a) at (x, y),
//   then the text in (r, g, b, a) at (x + d, y - d), with the system font.
//   Callers: draw_shadow_text_color, LicenseExaminationMessages::renderWhenPassing.
void draw_shadow_text(const char *text, u8 r, u8 g, u8 b, u8 a, int x, int y, int d)
{
    Color8 color;                           /* sp+0x00 */
    Color8 shadow;                          /* sp+0x10 */

    color.set(r, g, b, a);
    shadow.set(0, 0, 0, a);
    /* implicit: setR, setG, setB, setA (Color8::set, inline) */
    g_002F1968.setColor(shadow.value());
    g_002F1968.setPos(x, y);
    g_002F1968.print(text);
    g_002F1968.setColor(color.value());
    g_002F1968.setPos(x + d, y - d);
    g_002F1968.print(text);
}

// FUNC 0x0018A900 draw_shadow_text_color
//   draw_shadow_text with the four components of c.
void draw_shadow_text_color(const char *text, const Color8 *c, int x, int y, int d)
{
    draw_shadow_text(text, c->c[0], c->c[1], c->c[2], c->c[3], x, y, d);
}

// FUNC 0x0018A938 F_0018A938
//   The x scale of the texts: 0.75 in the 16:9 mode (g_002F424C), else 1.0.
//   Also called by TimeTrialExaminationMessages::renderWhenDisqualifying.
f32 F_0018A938(void)
{
    f32 scale = 1.0f;
    if (g_002F424C != 0)
        scale = 0.75f;
    return scale;
}

// FUNC 0x0018A960 draw_wave_text
//   ? name.  Unreferenced.  Prints text centred on x 0x140 at y, one
//   character at a time; each character's alpha follows a ramp that moves
//   with progress (0..1): the first half of the string fades with
//   2 - 2 progress - 2 t, the second with 2 t - 2 progress (t = (i + 1) / n),
//   times maxAlpha, clamped to 0..0x80.  The arithmetic is in double
//   (the `* 2.0's fold to x + x: one fptodp, then dpadd).
//   98.5%: maxAlpha and the font's %hi get s7 / s6 where retail has s6 / s7
//   (global-alloc priority: tried i, length, width, x declared at the top,
//   a Font pointer, the half length in a local, (int)maxAlpha; 991111-01
//   gives the same).
void draw_wave_text(const char *text, u32 rgb, u8 maxAlpha, int y, f32 progress)
{
    char glyph[2];                          /* sp+0x00 one character and its terminator */

    if (text == NULL)
        return;
    memset(glyph, 0, 2);
    int length = strlen(text);
    f32 width = text_string_width(&g_002F1968, text);
    int x = 0x140 - (int)(width * 0.5f);
    g_002F1968.setAlign(0);
    g_002F1968.setPos(x, y);
    for (int i = 0; i < length; i++) {
        glyph[0] = text[i];
        f32 t = (f32)(i + 1) / (f32)length;
        f32 t2 = t * 2.0;                   /* fold: t + t (one fptodp, dpadd) */
        f64 ramp;
        if (i < (length >> 1))
            ramp = 2.0 - progress * 2.0 - t2;
        else
            ramp = 0.0 - progress * 2.0 + t2;
        f32 a = ramp * maxAlpha;            /* implicit: fptodp, dpadd, dpsub, litodp, dpmul, dptofp (double arithmetic) */
        int alpha;
        if (a < 0.0)                        /* implicit: dpcmp (double compare) */
            alpha = 0;
        else if (a > 128.0)
            alpha = 0x80;
        else
            alpha = (u8)(int)a;
        g_002F1968.setColor(rgb | (alpha << 24));
        g_002F1968.print(glyph);
    }
}

// FUNC 0x0018ABC8 F_0018ABC8
//   The failure text: in rgb with the given alpha, centred (align 0xF) on
//   x 0x140, at y 0xE0 + d and, while d = (int)(slide * 30.0) is not 0,
//   again at 0xE0 - d (two copies sliding together as slide goes to 0).
//   Nothing for a NULL text.  Callers: LicenseExaminationMessages::
//   renderWhenDisqualifying, TimeTrialExaminationMessages::
//   renderWhenDisqualifying (T141).
void F_0018ABC8(const char *text, u32 rgb, f32 slide, u8 alpha)
{
    if (text == NULL)
        return;
    g_002F1968.setColor(rgb | (alpha << 24));
    g_002F1968.setAlign(0xF);
    int offset = (int)((f64)slide * 30.0); /* implicit: fptodp, dpmul, dptoli (double arithmetic) */
    g_002F1968.setPos(0x140, offset + 0xE0);
    g_002F1968.print(text);
    if (offset != 0) {
        g_002F1968.setPos(0x140, 0xE0 - offset);
        g_002F1968.print(text);
    }
    g_002F1968.setAlign(0);
}

/* ------------------------------------------------------------------------ */
/* ExamMessageLine                                                          */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018ACA8 ExamMessageLine::init
void ExamMessageLine::init(int time, u8 on)
{
    this->time = time;
    alpha = 0.0f;
    fade = 0;
    show = on;
}

// FUNC 0x0018ACC0 ExamMessageLine::fadeIn
void ExamMessageLine::fadeIn()
{
    fade = 1;
}

// FUNC 0x0018ACD0 ExamMessageLine::fadeOut
void ExamMessageLine::fadeOut()
{
    fade = 2;
}

// FUNC 0x0018ACE0 ExamMessageLine::update
//   One frame of the fade: 1/60 per frame up to 1 or down to 0, then still.
void ExamMessageLine::update()
{
    switch (fade) {
    case 0:                                 /* still (the empty case gives retail's range test) */
        break;
    case 1:
        alpha += 1.0f / 60.0f;              /* gp-0x7CD4 */
        if (alpha >= 1.0f) {
            alpha = 1.0f;
            fade = 0;
        }
        break;
    case 2:
        alpha -= 1.0f / 60.0f;              /* gp-0x7CD0 */
        if (alpha <= 0.0f) {
            alpha = 0.0f;
            fade = 0;
        }
        break;
    }
}

// FUNC 0x0018AD58 ExamMessageLine::draw
//   While visible (alpha != 0) and used: the time (race_format_time),
//   centred at (x, y), 1.25x (x scaled by F_0018A938), in (r, g, b) with
//   alpha alpha * 128, with its shadow.  buf is not used.
void ExamMessageLine::draw(GSBuffer &buf, u8 r, u8 g, u8 b, int x, int y)
{
    char text[0x40];                        /* sp+0x00 */
    Color8 color;                           /* sp+0x40 */

    if (alpha == 0.0f || show == 0)
        return;
    race_format_time(text, time);
    g_002F1968.setStyle("sw_20");
    g_002F1968.setAlign(0xF);
    g_002F1968.setSize(F_0018A938() * 1.25f, 1.25f);
    color.set(r, g, b, 0x80);
    color.setAlpha(alpha);                  /* setClamped(3, alpha * 128, 128) */
    /* implicit: setR, setG, setB, setA, setClamped (Color8::set, setAlpha: inline) */
    draw_shadow_text_color(text, &color, x, y, 2);
    g_002F1968.setSize(1.0f, 1.0f);
    g_002F1968.setAlign(0);
}

/* ------------------------------------------------------------------------ */
/* LicensePrize                                                             */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018AEB0 LicensePrize::LicensePrize
//   MiniPrize, then the vptr (+0x90).
LicensePrize::LicensePrize()
{
    /* implicit: MiniPrize (base constructor) */
}

// FUNC 0x0018AEE8 LicensePrize::~LicensePrize
//   (index.txt: LicensePrize::virtual_1) The vptr, then ~MiniPrize (which
//   does the deleting form).
LicensePrize::~LicensePrize()
{
    /* implicit: ~MiniPrize (base destructor) */
}

// FUNC 0x0018AF10 LicensePrize::render
//   Draws the model (MiniPrize::render: virtual_4), then puts back the
//   render states the 2D drawing expects: no alpha test, blending, depth
//   test, destination alpha test, culling; colour clamping on; every
//   colour channel written; TCC 1, TFX modulate; the blend
//   (Cs - Cd) * As + Cd.
void LicensePrize::render(GSBuffer &buf, f32 alpha)
{
    MiniPrize::render(buf, alpha);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_enable(PGL_CAP_COLOR_CLAMP);
    gfx_color_mask(1, 1, 1, 1);
    gfx_tex_param(PGL_TEXPARAM_TCC, 1);
    gfx_tex_param(PGL_TEXPARAM_TFX, PGL_TFX_MODULATE);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);  /* 0x44 */
}

/* ------------------------------------------------------------------------ */
/* LicenseExaminationMessages                                               */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018AF98 LicenseExaminationMessages::LicenseExaminationMessages
//   (ExaminationDisplayer's inline constructor: active = 0; finished, kind
//   and report are left as they are.)  The three prizes are constructed
//   (LicensePrize::LicensePrize; destroyed again if one throws), then a
//   unit of g_002C2F50 is taken and the gold, silver and bronze
//   models (type 0, rank 0 / 1 / 2) are loaded into it, 0x32000 bytes
//   each, kept (keep 1) and hidden.  Caller: RaceLicenseDisplay's
//   constructor.
LicenseExaminationMessages::LicenseExaminationMessages()
    : timer(-1), state(0), fade0(0.0f), fade1(0.0f), fade2(0.0f), text(NULL), buffer(NULL)
{
    /* implicit: LicensePrize (constructor of each prize) */
    current = NULL;
    buffer = (u8 *)RaceCarModel::allocBlock();      /* implicit: alloc (the inline's call) */
    u8 *model = buffer;
    u32 misalign = (u32)(size_t)buffer & 0x3F;
    if (misalign != 0)
        model = buffer + misalign;          /* sic: not a 64-byte alignment (that would add 0x40 - misalign) */
    for (int i = 0; i < 3; i++) {
        PrizeId id;                         /* sp+0x00 */
        id.type = 0;
        id.rank = -1;
        switch (i) {
        case 0:
            id.rank = PRIZE_RANK_GOLD;
            break;
        case 1:
            id.rank = PRIZE_RANK_SILVER;
            break;
        case 2:
            id.rank = PRIZE_RANK_BRONZE;
            break;
        }
        prizes[i].setup(&id, 0, 0, model, LICENSE_PRIZE_MODEL_SIZE, 1);  /* implicit: loadPrize (MiniPrize::setup, inline) */
        model += LICENSE_PRIZE_MODEL_SIZE;
    }
    current = &prizes[0];
}

// FUNC 0x0018B1F8 LicenseExaminationMessages::~LicenseExaminationMessages
//   Gives the model unit back, then the prizes are destroyed (last first,
//   through their vtables, flags 0) and ExaminationDisplayer's inline
//   destructor (the vptr); deleting form: operator delete(this).  Callers:
//   RaceLicenseDisplay's constructor (EH) and destructor.
LicenseExaminationMessages::~LicenseExaminationMessages()
{
    RaceCarModel::freeBlock(buffer);        /* implicit: release (the inline's call) */
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x0018B2C0 LicenseExaminationMessages::setup
//   (index.txt: virtual_8) Starts the sequence: kind_ > 0 passed with that
//   trophy (1, 2 gold, 3 silver, 4, 5 and above bronze), -1 failed.  time /
//   showTime: the time line; report_: post the trophy's BGM event and
//   GameStatus bit (RaceDisplay::innerPanel()); text_: the failure text.
//   The chosen prize restarts its animation, is updated once and hidden.
void LicenseExaminationMessages::setup(int kind_, int time, bool report_, const char *text_, bool showTime)
{
    kind = kind_;
    finished = 0;
    state = 0;
    fade0 = 0.0f;
    fade1 = 0.0f;
    fade2 = 0.0f;
    active = 1;
    text = text_;
    report = report_;
    line.init(time, showTime);
    if (kind <= 0)
        return;
    switch (kind) {
    case LICENSE_RESULT_GOLD_1:             /* retail: two blocks for 1 and 2 (jump table 0x0032C6E0) */
        current = &prizes[PRIZE_RANK_GOLD];
        break;
    case LICENSE_RESULT_GOLD_2:
        current = &prizes[PRIZE_RANK_GOLD];
        break;
    case LICENSE_RESULT_SILVER:
        current = &prizes[PRIZE_RANK_SILVER];
        break;
    case LICENSE_RESULT_BRONZE_1:
    case LICENSE_RESULT_BRONZE_2:
    default:
        current = &prizes[PRIZE_RANK_BRONZE];
        break;
    }
    current->restart();
    current->update();
    current->dispFlag = 0;
}

// FUNC 0x0018B380 LicenseExaminationMessages::deactivate
//   (index.txt: virtual_7) Not shown any more; the prize is hidden.
void LicenseExaminationMessages::deactivate()
{
    active = 0;
    if (current != NULL)
        current->hide();
}

// FUNC 0x0018B3B0 LicenseExaminationMessages::updateWhenDisqualifying
//   (index.txt: virtual_1) The fail sequence, one frame: the band and the
//   text fade in together (1/60 per frame); at 1 the time line fades in
//   and 180 frames are waited; then both fade out and at 0 the time line
//   fades out, finished is set (timer = 120).  org is not used.
void LicenseExaminationMessages::updateWhenDisqualifying(RaceOrganization &org)
{
    switch (state) {
    case 0:
        fade0 = 0.0f;
        fade1 = 0.0f;
        state++;
        /* fallthrough */
    case 1:
        fade0 += 1.0f / 60.0f;              /* gp-0x7CCC */
        fade1 += 1.0f / 60.0f;
        if (fade0 < 1.0) /* implicit: fptodp, dpcmp (double compare) */
            break;
        timer.set(180);
        line.fadeIn();
        state++;
        break;
    case 2:
        if (!timer.isOver())                /* retail: li 1; slt; movn; beqz */
            break;
        fade0 = 1.0f;                       /* before state++: retail reloads state */
        fade1 = 1.0f;
        state++;
        break;
    case 3:
        fade0 -= 1.0f / 60.0f;              /* gp-0x7CC8 */
        fade1 -= 1.0f / 60.0f;
        if (fade0 >= 0.0)
            break;
        timer.set(120);
        line.fadeOut();
        finished = 1;
        state++;
        break;
    }
}

// FUNC 0x0018B510 LicenseExaminationMessages::updateWhenPassing
//   (? the update after a pass) The pass sequence, one frame:
//   0 / 1  the dark screen fades in (1/30 per frame).  At 1: with report,
//          the trophy's BGM event (0x14 with 0x1D / 0x1E / 0x1F) and its
//          GameStatus small-trophy bit (0x20000 / 0x40000 / 0x80000); a wait
//          of 180 frames (gold) or 30 (silver, bronze); kinds outside 1..5
//          set no wait.
//   2      after the wait the prize is shown, its fade starts at 0 and the
//          time line fades in;
//   3      the rank name fades in (1/30 per frame), then 300 frames;
//   4      finished.
//   From state 3 on the prize fades in (fade2, 1/60 per frame).
void LicenseExaminationMessages::updateWhenPassing(RaceOrganization &org)
{
    switch (state) {
    case 0:
        fade0 = 0.0f;
        fade1 = 0.0f;
        state++;
        /* fallthrough */
    case 1:
        fade0 += 1.0f / 30.0f;              /* gp-0x7CC4 */
        if (fade0 < 1.0) /* implicit: fptodp, dpcmp (double compare) */
            break;
        fade0 = 1.0f;
        switch (kind) {
        case LICENSE_RESULT_GOLD_1:
        case LICENSE_RESULT_GOLD_2:
            if (report) {
                org.eventQueue.post(RACE_EVENT_BGM, BGM_CODE_PRIZE_GOLD, 0);
                getGameStatus()->setSmallTrophy(SMALL_TROPHY_LICENSE_GOLD);
            }
            timer.set(180);
            break;
        case LICENSE_RESULT_SILVER:
            if (report) {
                org.eventQueue.post(RACE_EVENT_BGM, BGM_CODE_PRIZE_SILVER, 0);
                getGameStatus()->setSmallTrophy(SMALL_TROPHY_LICENSE_SILVER);
            }
            timer.set(30);
            break;
        case LICENSE_RESULT_BRONZE_1:
        case LICENSE_RESULT_BRONZE_2:
            if (report) {
                org.eventQueue.post(RACE_EVENT_BGM, BGM_CODE_PRIZE_BRONZE, 0);
                getGameStatus()->setSmallTrophy(SMALL_TROPHY_LICENSE_BRONZE);
            }
            timer.set(30);
            break;
        }
        state++;
        break;
    case 2:
        if (timer.get() > 0)
            break;
        state++;
        current->show();
        fade2 = 0.0f;
        line.fadeIn();
        break;
    case 3:
        fade1 += 1.0f / 30.0f;              /* gp-0x7CC0 */
        if (fade1 > 1.0f) {
            fade1 = 1.0f;
            state++;
            timer.set(300);
        }
        break;
    case 4:
        if (timer.get() > 0)
            break;
        finished = 1;
        state++;
        break;
    case 5:                                 /* retail: a 6-entry jump table (0x0032C700) */
        break;
    }
    if (state >= 3) {
        fade2 += 1.0f / 60.0f;              /* gp-0x7CBC */
        if (fade2 > 1.0f)
            fade2 = 1.0f;
    }
}

// FUNC 0x0018B758 LicenseExaminationMessages::updateAlways
//   (index.txt: virtual_2) The prize's animation, timer--, the time line's
//   fade.
void LicenseExaminationMessages::updateAlways()
{
    current->update();
    timer.tick();
    line.update();
}

// FUNC 0x0018B798 LicenseExaminationMessages::renderWhenPassing
//   (? the draw after a pass) A black rectangle over the screen (wider and
//   centred in the 16:9 mode) blended by fade0 / 3, the prize (alpha
//   fade2), then its rank name ("GOLD" ...) at (0x140, 0x150) in the
//   trophy's colour (gold, silver, bronze; else light green) with alpha
//   fade1 * 128, and the time line at (0x140, 0x70) in the same colour.
void LicenseExaminationMessages::renderWhenPassing(GSBuffer &buf)
{
    u8 nameAlpha = (u8)(u32)(fade1 * 128.0f); /* implicit: fptoui (float to unsigned) */
    Color8 dark;                            /* sp+0x00 */

    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_CD), fade0 / 3.0f);  /* 0x64 */
    dark.c[0] = 0;
    dark.c[1] = 0;
    dark.c[2] = 0;
    dark.c[3] = 0x80;
    pgl_color(dark.value());
    if (g_002F424C != 0) {
        f32 centre = (f32)buf.width * 0.5f;
        f32 halfWidth = centre * 4.0f / 3.0f + 1.0f;
        int x = (int)(centre - halfWidth);
        int w = (int)(halfWidth + halfWidth);
        race_fill_rect(x, 0, w, buf.displayHeight());
    } else {
        int w = buf.width;
        race_fill_rect(0, 0, w, buf.displayHeight());
    }
    gfx_enable(PGL_CAP_ALPHA_TEST);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 0.0f);    /* 0x44 */
    gfx_disable(PGL_CAP_FOG);
    current->render(buf, fade2);

    Color8 color(LICENSE_COLOR_DEFAULT);    /* sp+0x10 */
    /* implicit: set (Color8(u32), inline, calls it out of line; also for the temporaries below) */
    if (kind > 0)                           /* retail: blez, then the (kind - 1) < 5 range test */
    switch (kind) {
    case LICENSE_RESULT_GOLD_1:
    case LICENSE_RESULT_GOLD_2:
        color = Color8(LICENSE_COLOR_GOLD); /* the temporary at sp+0x20 */
        break;
    case LICENSE_RESULT_SILVER:
        color = Color8(LICENSE_COLOR_SILVER);
        break;
    case LICENSE_RESULT_BRONZE_1:
    case LICENSE_RESULT_BRONZE_2:
        color = Color8(LICENSE_COLOR_BRONZE);
        break;
    }
    const char *rankName = prize_rank_name(current->loaded.rank);
    g_002F1968.setStyle("sw_20");
    g_002F1968.setAlign(0xF);
    g_002F1968.setSize(F_0018A938() * 1.25f, 1.25f);
    draw_shadow_text(rankName, color.c[0], color.c[1], color.c[2], nameAlpha, 0x140, 0x150, 2);
    g_002F1968.setAlign(0);
    g_002F1968.setSize(1.0f, 1.0f);
    line.draw(buf, color.c[0], color.c[1], color.c[2], 0x140, 0x70);
}

// FUNC 0x0018BA48 LicenseExaminationMessages::renderWhenDisqualifying
//   (index.txt: virtual_4) As TimeTrialExaminationMessages's: a band in
//   0xFFA0A0 (light blue) with alpha fade0 * 64 over the screen height
//   (wider and centred in the 16:9 mode), then the failure text in
//   0x40C0A0 with alpha fade1 * 128 sliding by 1 - fade1 at 1.5x, and the
//   time line at (0x140, 0x8C).
void LicenseExaminationMessages::renderWhenDisqualifying(GSBuffer &buf)
{
    u8 bandAlpha = (u8)(u32)(fade0 * 128.0f) >> 1; /* implicit: fptoui (float to unsigned) */
    u8 textAlpha = (u8)(u32)(fade1 * 128.0f);
    Color8 band;                            /* sp+0x00 LICENSE_COLOR_BAND, set byte by byte */

    g_002F1968.setStyle("sw_20");
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_ZERO, PGL_SEL_CS, PGL_SEL_AS, PGL_SEL_CD), 0.0f);  /* 0x42 */
    band.c[0] = 0xA0;
    band.c[1] = 0xA0;
    band.c[2] = 0xFF;
    band.c[3] = bandAlpha;
    pgl_color(band.value());
    if (g_002F424C != 0) {
        f32 centre = (f32)buf.width * 0.5f;
        f32 halfWidth = centre * 4.0f / 3.0f + 1.0f;
        int x = (int)(centre - halfWidth);
        int w = (int)(halfWidth + halfWidth);
        race_fill_rect(x, 0, w, buf.displayHeight());
    } else {
        int w = buf.width;
        race_fill_rect(0, 0, w, buf.displayHeight());
    }
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 0.0f);    /* 0x44 */
    Color8 color(LICENSE_COLOR_FAILED);     /* sp+0x10 */
    /* implicit: set (Color8(u32), inline, calls it out of line) */
    g_002F1968.setSize(F_0018A938() * 1.5f, 1.5f);
    F_0018ABC8(text, color.value(), (f32)(1.0 - fade1), textAlpha); /* implicit: fptodp, dpsub, dptofp (double arithmetic) */
    g_002F1968.setSize(1.0f, 1.0f);
    line.draw(buf, color.c[0], color.c[1], color.c[2], 0x140, 0x8C);
}

// FUNC 0x0018BC90 LicenseExaminationMessages::getState
/* inline in game/race/display/T146_license_examination_messages.h; egcs emits this copy
 * (no callers): state */

// FUNC 0x0018BC98 LicenseExaminationMessages::nextState
/* inline in game/race/display/T146_license_examination_messages.h; egcs emits this copy
 * (no callers): state++ */

// FUNC 0x0018BCA8 LicenseExaminationMessages::setState
/* inline in game/race/display/T146_license_examination_messages.h; egcs emits this copy
 * (no callers): state = s */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0018BCB0 __tf26LicenseExaminationMessages
//   Builds the __si_type_info at 0x004B2080 (base: ExaminationDisplayer,
//   gp-0x46B0; T141's __tf20ExaminationDisplayer is called) on the first
//   call.
extern "C" const type_info &__tf26LicenseExaminationMessages(void)
{
    if (__ti26LicenseExaminationMessages[0] == 0) {
        __tf20ExaminationDisplayer();
        __rtti_si(__ti26LicenseExaminationMessages, "26LicenseExaminationMessages",
                  (const type_info *)__ti20ExaminationDisplayer);
    }
    return *(const type_info *)__ti26LicenseExaminationMessages;
}
#endif

/* ------------------------------------------------------------------------ */
/* ExaminationDisplayer: T146's out-of-line copies of its inline methods    */
/* (unreferenced; T141's copies are the ones in both vtables)               */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018BD00 ExaminationDisplayer::deactivate
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (T141's: 0x00188F30) */

// FUNC 0x0018BD08 ExaminationDisplayer::activate
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (T141's: 0x00188F38) */

// FUNC 0x0018BD18 ExaminationDisplayer::~ExaminationDisplayer
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (T141's: 0x00188F48;
   only the vptr, deleting form: operator delete(this)) */
/* implicit: operator delete (deleting destructor) */

// FUNC 0x0018BD48 ExaminationDisplayer::renderWhenDisqualifying
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (T141's: 0x00188F78;
   the five empty slots are told apart by their order only, as in T141) */

// FUNC 0x0018BD50 ExaminationDisplayer::renderWhenPassing
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (T141's: 0x00188F80) */

// FUNC 0x0018BD58 ExaminationDisplayer::updateAlways
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (T141's: 0x00188F88) */

// FUNC 0x0018BD60 ExaminationDisplayer::updateWhenDisqualifying
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (T141's: 0x00188F90) */

// FUNC 0x0018BD68 ExaminationDisplayer::updateWhenPassing
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (T141's: 0x00188F98) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0018BD70 __tf20ExaminationDisplayer
//   T146's copy of 0x00188FA0 (T141; with its own copy of the name string,
//   0x0032C940): builds the __user_type_info at gp-0x46B0 on the first
//   call.  Weak: T141 (u048) defines the other copy.
extern "C" __attribute__((weak)) const type_info &__tf20ExaminationDisplayer(void)
{
    if (__ti20ExaminationDisplayer[0] == 0)
        __rtti_user(__ti20ExaminationDisplayer, "20ExaminationDisplayer");
    return *(const type_info *)__ti20ExaminationDisplayer;
}

// FUNC 0x0018BDB0 __tf12LicensePrize
//   Builds the __si_type_info at 0x004B2090 (base: MiniPrize, 0x004B2070)
//   on the first call.
extern "C" const type_info &__tf12LicensePrize(void)
{
    if (__ti12LicensePrize[0] == 0) {
        __tf9MiniPrize();
        __rtti_si(__ti12LicensePrize, "12LicensePrize", (const type_info *)__ti9MiniPrize);
    }
    return *(const type_info *)__ti12LicensePrize;
}
#endif

/* ------------------------------------------------------------------------ */
/* Color8: T146's out-of-line copies of its inline members (T188_color.h)  */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018BE00 Color8::set
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the Color8(u32)
 * constructor calls it out of line: renderWhenPassing,
 * renderWhenDisqualifying): from a 32-bit value, r = bits 0..7, g = 8..15,
 * b = 16..23, a = 24..31 (u049's "constructor Color8(u32)"). */

// FUNC 0x0018BE20 Color8::setA
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (Color8::set(r, g, b,
 * a) calls it out of line; the calls reach T188's copy 0x00174AC8, linked
 * first): c[3] = v */

// FUNC 0x0018BE28 Color8::setB
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (as setA; T188's
 * 0x00174AD0): c[2] = v */

// FUNC 0x0018BE30 Color8::setG
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (as setA; T188's
 * 0x00174AD8): c[1] = v */

// FUNC 0x0018BE38 Color8::setR
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (as setA; T188's
 * 0x00174AE0): c[0] = v */
