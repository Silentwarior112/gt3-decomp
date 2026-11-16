/* T141 - TimeTrialExaminationMessages, RaceTimeTrialDisplay,
 * ExaminationDisplayer (u048), 0x00188478..0x00188FE0.
 *
 * RaceTimeTrialDisplay is the race display of the time trial.  When the
 * race posts event 0xA (DynamicsConductorTimeTrial: the car is
 * disqualified) it runs a sequence: the BGM event 0x14 (group 0x20), a 1 s
 * wait, then the "disqualified" message (TimeTrialExaminationMessages: a
 * band darkening the screen and the text fading in, held 3 s, fading out;
 * at most 30 s), a 4 s wait that the pad's decide button cuts short, and
 * finally event 1 (the end of the race).
 *
 * ExaminationDisplayer's methods are inline in its header; their
 * out-of-line copies come last in this unit (0x00188F30..0x00188FDC).
 *
 * Includes (u048's data pass): the classes retail's T141 saw, in its order.
 * Its .rodata (0x0032B5D0..0x0032B8BA) is "bad_alloc" (<new>), the two jump
 * tables, the three vtables, then the type names in reverse definition
 * order: RaceTimeTrialDisplay, TimeTrialExaminationMessages,
 * ExaminationDisplayer, CallBackWhenPrizeRendering, PrizeModelSetContext,
 * then T154_race_display.h's (RaceMiniMap .. RaceMeterBase, RaceCameraBase,
 * UnitArena<64>, RaceValueDisplayBase, RaceDisplayBase): the race display
 * first, the prize classes after it.  Its .sdata (0x00351EA8..0x00351EB6) is
 * the "" of T157's inline RaceMessageDisplay::clear, then "sw_20".
 */
// DATA_START 0x0032B5D0
// DATA_START 0x00351EA8
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's T141 has it first) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user */
#endif
#include "game/race/display/T154_race_display.h"         /* base RaceDisplay (first: retail's type-name order) */
#include "game/menu/gtmode/T460_prize_base.h"           /* no use: PrizeModelSetContext, CallBackWhenPrizeRendering
                                               (after the race display, before this unit's three) */
#include "game/race/display/T141_race_time_trial_display.h"  /* own: ExaminationDisplayer, TimeTrialExaminationMessages,
                                                   RaceTimeTrialDisplay */
#include "game/race/display/T146_license_examination_messages.h" /* F_0018A938 (the text x scale), F_0018ABC8 (centred text) */
#include "game/menu/framework/T188_color.h"                /* Color8 (the band's colour) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::find / post, RACE_EVENT_* */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn, CONTROL_DECIDE */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, the RACEDISPLAY texts */
#include "game/race/display/T158_race_display_draw.h"    /* race_fill_rect */
#include "engine/text/T496_font.h"                 /* Font::setStyle / setSize, g_002F1968 */
#include "engine/gfx/T512_render_context.h"       /* g_002F4230 (wide) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "engine/pgl/G001_flare.h"                /* gfx_blend_func, pgl_color, PGL_* */
#include "game/race/modes/race_time_trial_internal.h"
#include "engine/pdistd/T533_printf.h"               /* printf (the enhanced engine's log lines) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceQueueEvent, RACE_EVENT_CAR_DISQUALIFIED */

/* RaceTimeTrialDisplay::phase[0]: the disqualification sequence. */
enum {
    TT_PHASE_RACING     = 0,                /* waits for RACE_EVENT_DISQUALIFIED */
    TT_PHASE_DISQUALIFY = 1,                /* the BGM, then the sequence */
    TT_PHASE_SEQUENCE   = 2,                /* updateDisqualify every frame */
    TT_PHASE_DONE       = 3,                /* -> TT_PHASE_END */
    TT_PHASE_END        = 4                 /* endDisqualify */
};

/* TimeTrialExaminationMessages::state. */
enum {
    TTEM_START    = 0,
    TTEM_FADE_IN  = 1,
    TTEM_HOLD     = 2,
    TTEM_FADE_OUT = 3,
    TTEM_DONE     = 4
};

/* ------------------------------------------------------------------------ */
/* TimeTrialExaminationMessages                                             */
/* ------------------------------------------------------------------------ */

// FUNC 0x00188478 TimeTrialExaminationMessages::TimeTrialExaminationMessages
//   (ExaminationDisplayer's inline constructor: active = 0; finished is
//   left as it is.)
TimeTrialExaminationMessages::TimeTrialExaminationMessages()
    : timer(-1), state(TTEM_START), bandFade(0.0f), textFade(0.0f), message(NULL)
{
    stopper = 0;                            /* the enhanced engine (Lupo 2 L2 0x00185AB0) */
    stopped = 0;
}

// FUNC 0x001884A8 TimeTrialExaminationMessages::~TimeTrialExaminationMessages
//   Only ExaminationDisplayer's inline destructor (its vptr); deleting form:
//   operator delete(this).
TimeTrialExaminationMessages::~TimeTrialExaminationMessages()
{
    /* implicit: operator delete (flags & 1) */
}

// FUNC 0x001884D8 TimeTrialExaminationMessages::start
//   Shows msg from the start of the sequence.  The first argument (the
//   display's innerPanel()) is not used.
void TimeTrialExaminationMessages::start(int unused, const char *msg)
{
    active = 1;
    finished = 0;
    state = TTEM_START;
    bandFade = 0.0f;
    textFade = 0.0f;
    stopped = 0;                            /* the enhanced engine (Lupo 2's reset L2 0x00185B38) */
    message = msg;                          /* last (retail's store order) */
}

// FUNC 0x001884F8 TimeTrialExaminationMessages::updateWhenDisqualifying
//   (index.txt: virtual_1) One frame of the fades (1/60 per frame): fade
//   in, hold 180 frames, fade out, then finished (and timer = 120).  org is
//   not used.  The fade tests are done in double (a double literal: the
//   soft-float fptodp and dpcmp).
void TimeTrialExaminationMessages::updateWhenDisqualifying(RaceOrganization &org)
{
    /* implicit: fptodp, dpcmp (the double comparisons, soft-float) */
    switch (state) {
    case TTEM_START:
        bandFade = 0.0f;
        textFade = 0.0f;
        state++;
        /* fallthrough */
    case TTEM_FADE_IN:
        bandFade += 1.0f / 60.0f;          /* gp-0x7CDC */
        textFade += 1.0f / 60.0f;
        if (bandFade < 1.0)
            break;
        timer.set(180);
        state++;
        break;
    case TTEM_HOLD:
        if (timer.get() > 0)
            break;
        bandFade = 1.0f;
        textFade = 1.0f;
        if (stopper == 0)
            state++;
        else
            stopped = 1;                    /* the enhanced engine: held (Lupo 2 L2 0x00185C5C) */
        break;
    case TTEM_FADE_OUT:
        bandFade -= 1.0f / 60.0f;          /* gp-0x7CD8 (a second copy of the constant) */
        textFade -= 1.0f / 60.0f;
        if (bandFade >= 0.0)
            break;
        timer.set(120);
        state++;
        finished = 1;
        break;
    case TTEM_DONE:                         /* the fifth case: retail's jump table
                                               (0x0032B5E0) has 5 entries */
        break;
    }
}

// FUNC 0x00188620 TimeTrialExaminationMessages::updateAlways
//   (index.txt: virtual_2)
void TimeTrialExaminationMessages::updateAlways()
{
    timer.tick();
}

// FUNC 0x00188630 TimeTrialExaminationMessages::renderWhenDisqualifying
//   (index.txt: virtual_4) The band in 0xFFA0A0 (ABGR) with alpha bandFade
//   * 64, blended subtractively (the screen darkens, blue most) over the
//   screen height (in the 16:9 mode 4/3 of the width, centred), then the
//   normal blend again and the message in 0x40C0A0 with alpha textFade *
//   128, sliding by 1 - textFade, at 1.5x size (x scaled 0.75 in 16:9).
//   The alphas go through fptoui; 1 - textFade is computed in double
//   (fptodp, dpsub, dptofp).
void TimeTrialExaminationMessages::renderWhenDisqualifying(GSBuffer &buf)
{
    u8 bandAlpha = (u8)(u32)(bandFade * 128.0f) >> 1;
    u8 textAlpha = (u8)(u32)(textFade * 128.0f);
    /* implicit: fptoui (the alpha conversions) */
    Color8 color;                           /* sp+0x00 (stored; its value() folds to registers) */

    g_002F1968.setStyle("sw_20");
    /* (0 - Cs) * As + Cd */
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_ZERO, PGL_SEL_CS, PGL_SEL_AS, PGL_SEL_CD), 0.0f);
    color.c[0] = 0xA0;
    color.c[1] = 0xA0;
    color.c[2] = 0xFF;
    color.c[3] = bandAlpha;
    pgl_color(color.value());
    if (g_002F4230.wide != 0) {
        f32 centerX = (f32)buf.width * 0.5f;
        f32 halfWidth = centerX * 4.0f / 3.0f + 1.0f;
        int x = (int)(centerX - halfWidth);
        int w = (int)(halfWidth + halfWidth);
        race_fill_rect(x, 0, w, buf.displayHeight());
    } else {
        int w = buf.width;
        race_fill_rect(0, 0, w, buf.displayHeight());
    }
    /* (Cs - Cd) * As + Cd */
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 0.0f);
    if (message != NULL) {
        g_002F1968.setSize(F_0018A938() * 1.5f, 1.5f);
        /* The enhanced engine (hud.md C20; Lupo 2 L2 0x00185F10): centred
         * on the buffer's rectangle (its size without one), as the Pod's
         * split screen needs; the race's 640 x 448 buffer gives GT3's
         * (320, 224). */
        int w = buf.rectW;
        if (w <= 0)
            w = buf.width;
        int h = buf.rectH;
        if (h <= 0)
            h = buf.displayHeight();
        exam_draw_failure_text(message, 0x40C0A0, (f32)(1.0 - textFade), textAlpha, w, h);
        /* implicit: fptodp, dpsub, dptofp (1.0 - textFade in double, soft-
         * float) */
        g_002F1968.setSize(1.0f, 1.0f);
    }
}

/* ------------------------------------------------------------------------ */
/* RaceTimeTrialDisplay                                                     */
/* ------------------------------------------------------------------------ */

// FUNC 0x00188828 RaceTimeTrialDisplay::RaceTimeTrialDisplay
//   RaceDisplay (0x001909E0), the message (0x00188478), timer = -1, no
//   input, reset() (which sets timer to 0 again).  passed is left as it is.
//   Unwinding: ~TimeTrialExaminationMessages, ~RaceDisplay.
RaceTimeTrialDisplay::RaceTimeTrialDisplay()
    : timer(-1)
{
    /* implicit: TimeTrialExaminationMessages, RaceDisplay (the base's /
     * members' constructors), ~TimeTrialExaminationMessages, ~RaceDisplay
     * (their cleanup on a throw) */
    input = NULL;
    reset();
}

// FUNC 0x001888D8 RaceTimeTrialDisplay::~RaceTimeTrialDisplay
//   (index.txt: virtual_21) The message (~TimeTrialExaminationMessages,
//   flags 2), then ~RaceDisplay with the caller's flags (which deletes).
RaceTimeTrialDisplay::~RaceTimeTrialDisplay()
{
    /* implicit: ~TimeTrialExaminationMessages, ~RaceDisplay (the base / member
     * destructors) */
}

// FUNC 0x00188930 RaceTimeTrialDisplay::reset
//   The message off, timer 0, the phases and frameCount 0.  (85.4%: retail
//   computes the message's address first but stores the timer first, the
//   message's pointer in $a1; with the message first, as here, its store
//   comes first and the pointer takes $v0.  Tried: every order of the five
//   stores, pointer / reference locals for both, `active = 0' directly, the
//   reference compiler: at most 85.4%.  T147's reset (u049) has the
//   message first in both, as this source.)
void RaceTimeTrialDisplay::reset()
{
    ttem.deactivate();
    timer.set(0);
    phase[0] = TT_PHASE_RACING;
    phase[1] = 0;
    frameCount = 0;
}

// FUNC 0x00188960 RaceTimeTrialDisplay::setPhase
void RaceTimeTrialDisplay::setPhase(int i, u8 v)
{
    phase[i] = v;
}

// FUNC 0x00188978 RaceTimeTrialDisplay::updateDisqualify
//   (index.txt: virtual_22) The disqualification sequence, one frame;
//   step phase[1]: 0 timer = 60; 1 waits, then shows the TimeTrialFail text
//   for at most 1800 frames; 2 runs it until it is finished, then timer =
//   240; 3 waits (the pad's decide button skips), then phase[0] = 3.
//   timer-- every frame.  (The calls on the message go through its vtable.)
//   97.9%: cases 2 / 3 load the 0xA47C of `&ttem' into $v1 (retail $v0;
//   local-alloc), and case 2's exit computes the timer's address before
//   loading 240.  Tried: msg as a pointer / reference / TTEM pointer / one
//   variable for both cases, an inline update() on the message,
//   ttem.deactivate(), the store orders of the exit, getSubPhase() /
//   nextSubPhase() against phase[1] (no change), the reference compiler.
void RaceTimeTrialDisplay::updateDisqualify(RaceOrganization &org)
{
    const char *text = g_002C43E8[RACEDISPLAY_TIME_TRIAL_FAIL];    /* read before the switch */

    switch (getSubPhase()) {
    case 0:
        timer.set(60);
        nextSubPhase();
        /* fallthrough */
    case 1:
        if (timer.get() > 0)
            break;
        timer.set(1800);
        ttem.start(innerPanel(), text);
        nextSubPhase();
        break;
    case 2: {
        ExaminationDisplayer *msg = &ttem;
        msg->updateWhenDisqualifying(org);
        msg->updateAlways();
        /* The enhanced engine (hud.md C20; Lupo 2 L2 0x001862A8): no
         * 1800-frame cap while a stopper holds the message (the time
         * trial's message ends after about 300 frames, so the cap never
         * acted there). */
        if (msg->finished != 0 || (ttem.stopper == 0 && timer.isOver())) {
            msg->active = 0;
            timer.set(240);
            nextSubPhase();
        }
        break;
    }
    case 3: {
        ExaminationDisplayer *msg = &ttem;
        msg->updateWhenDisqualifying(org);
        msg->updateAlways();
        int skip = 0;
        if (input != NULL) {
            /* this sense: retail's `skip = 0; movn skip, 1' (the other
             * gives movz) */
            if (!(input->getButtonsOn() & CONTROL_DECIDE))
                skip = 0;
            else
                skip = 1;
        }
        if (skip != 0 || timer.isOver()) {
            msg->active = 0;
            setPhase(0, TT_PHASE_DONE);
            nextSubPhase();
        }
        break;
    }
    }
    timer.tick();
}

// FUNC 0x00188BB8 RaceTimeTrialDisplay::endDisqualify
//   (index.txt: virtual_23) Back to phase 0 and posts event 1 (the end of
//   the race).
void RaceTimeTrialDisplay::endDisqualify(RaceOrganization &org)
{
    setPhase(0, TT_PHASE_RACING);
    phase[1] = 0;
    org.eventQueue.post(RACE_EVENT_END, 0, 0);
}

// FUNC 0x00188C10 RaceTimeTrialDisplay::virtual_2
//   The update: RaceDisplay's, frameCount++, then phase[0]: 0 takes event
//   0xA (disqualified) -> 1; 1 posts the BGM event 0x14 (group 0x20) -> 2;
//   2 the sequence (slot 22); 3 -> 4; 4 the end (slot 23).  dt: the
//   enhanced engine's time step (hud.md C19), passed on.
void RaceTimeTrialDisplay::virtual_2(f32 dt)
{
    RaceDisplay::virtual_2(dt);
    RaceOrganization *o = org;
    frameCount++;
    switch (phase[0]) {
    case TT_PHASE_RACING:
        if (takeDisqualification(o) != 0)   /* the enhanced engine (hud.md C20) */
            setPhase(0, TT_PHASE_DISQUALIFY);
        break;
    case TT_PHASE_DISQUALIFY:
        passed = 0;
        setPhase(0, TT_PHASE_SEQUENCE);
        phase[1] = 0;
        if (ttem.stopper == 0)              /* the enhanced engine: no jingle with a stopper (L2 0x001865AC) */
            o->eventQueue.post(RACE_EVENT_BGM, BGM_CODE_DISQUALIFIED, 0);
        /* fallthrough */
    case TT_PHASE_SEQUENCE:
        updateDisqualify(*o);
        break;
    case TT_PHASE_DONE:
        setPhase(0, TT_PHASE_END);
        phase[1] = 0;
        /* fallthrough */
    case TT_PHASE_END:
        endDisqualify(*o);
        break;
    }
}

// RaceTimeTrialDisplay::takeDisqualification (the enhanced engine)
//   Lupo 2's SimpleExamDisplay::virtual_2, phase 0 (L2 0x001864C0..
//   0x00186560, hud.md C20): 1 when this display's player car (playerCar)
//   is disqualified, by event 0xA (RACE_EVENT_DISQUALIFIED: the time
//   trial's, arg the car's index) or the Pod race's appended
//   RACE_EVENT_CAR_DISQUALIFIED (Lupo 2's 0xB, arg the car).  Lupo 2 only
//   peeks at both; here the event 0xA found is taken from the queue as
//   GT3's time trial takes it (find(..., 1): nothing else reads it, and a
//   time trial's sequence then cannot start twice), the Pod's is left (the
//   result board reads it, menus_b.md 14).  A time trial has one car,
//   index 0 = playerCar: GT3's behaviour.
int RaceTimeTrialDisplay::takeDisqualification(RaceOrganization *o)
{
    RaceEventQueue *queue = &o->eventQueue;
    int car = playerCar;
    for (int i = 0; i < queue->count; i++) {
        RaceQueueEvent *e = &queue->events[i];
        if (e->type == RACE_EVENT_DISQUALIFIED && e->arg == car) {
            queue->remove(i);
            return 1;
        }
    }
    int arg;
    for (int i = 0; i < queue->counts[RACE_EVENT_CAR_DISQUALIFIED]; i++) {
        if (queue->findNth(RACE_EVENT_CAR_DISQUALIFIED, &arg, i) && arg == car) {
            printf("hud: car %d disqualified (the Pod rule)\n", car);
            return 1;
        }
    }
    return 0;
}

// FUNC 0x00188D40 RaceTimeTrialDisplay::virtual_3
//   The draw: RaceDisplay's, then the message while it is active and the
//   inner panel is on (renderWhenPassing if passed, else
//   renderWhenDisqualifying: ExaminationDisplayer::draw, inline).
void RaceTimeTrialDisplay::virtual_3(GSBuffer &buf)
{
    RaceDisplay::virtual_3(buf);
    if (innerPanel())
        ttem.draw(buf, passed);
}

// FUNC 0x00188DE0 RaceTimeTrialDisplay::virtual_0
//   The set-up for a race: RaceDisplay's, then reset().
void RaceTimeTrialDisplay::virtual_0(RaceOrganization *o, int raceMode, int replay)
{
    RaceDisplay::virtual_0(o, raceMode, replay);
    reset();
}

/* ------------------------------------------------------------------------ */
/* RacePODDisplay (the enhanced engine)                                     */
/* ------------------------------------------------------------------------ */

/* The event board's colour of the rules (RaceEventDisplay's default). */
#define POD_RULE_COLOR 0x802BCC49
/* RaceDisplay::replay of a live race (T154's RD_LIVE). */
#define POD_REPLAY_LIVE 0

// RacePODDisplay::RacePODDisplay (the enhanced engine)
//   Lupo 2 L2 0x001B40E8: RaceTimeTrialDisplay's (Lupo 2's
//   SimpleExamDisplay), then its vptr.  Caller: RacePOD's constructor (the
//   race area's; Lupo 2 L2 0x0018D4C8), RacePOD2PDisplay's (T155).
RacePODDisplay::RacePODDisplay()
{
    /* implicit: RaceTimeTrialDisplay (base constructor) */
}

// RacePODDisplay::put_start_display (the enhanced engine)
//   Lupo 2 L2 0x001B4118 (hud.md C21): RaceDisplay's ("@start"), then, in
//   a live race, the stop-area rules on the event board in green, "sw_16",
//   3 s each, the first 2 s after the last event's start, the others each
//   when the one before ends.
void RacePODDisplay::put_start_display()
{
    RaceDisplay::put_start_display();
    if (replay != POD_REPLAY_LIVE)
        return;
    RaceEventDisplay *board = &events;
    board->color = POD_RULE_COLOR;
    board->setFont("sw_16");
    board->addEvent(g_raceDisplayEnhTexts[RD_TEXT_POD_RULE_1], 3.0f, 2.0f);
    board->addEvent(g_raceDisplayEnhTexts[RD_TEXT_POD_RULE_2], 3.0f, 0.0f);
    board->addEvent(g_raceDisplayEnhTexts[RD_TEXT_POD_RULE_3], 3.0f, 0.0f);
    printf("hud: the Pod's stop-area rules on the event board\n");
}

// FUNC 0x00188E10 RaceTimeTrialDisplay::setInput
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (no
 * callers): input = c */

// FUNC 0x00188E20 RaceTimeTrialDisplay::getSubPhase
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (no
 * callers): phase[1] */

// FUNC 0x00188E30 RaceTimeTrialDisplay::nextSubPhase
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (no
 * callers): phase[1]++ */

// FUNC 0x00188E48 RaceTimeTrialDisplay::setSubPhase
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (no
 * callers): phase[1] = v */

// FUNC 0x00188E58 RaceTimeTrialDisplay::getPhase
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (no
 * callers): phase[i] */

#ifndef BOOT_OLD_GCC
// FUNC 0x00188E70 __tf20RaceTimeTrialDisplay
//   Builds the __si_type_info __ti20RaceTimeTrialDisplay (base RaceDisplay,
//   __ti11RaceDisplay) on the first call.
extern "C" const type_info &__tf20RaceTimeTrialDisplay(void)
{
    if (__ti20RaceTimeTrialDisplay[0] == 0) {
        __tf11RaceDisplay();
        __rtti_si(__ti20RaceTimeTrialDisplay, "20RaceTimeTrialDisplay", (const type_info *)__ti11RaceDisplay);
    }
    return *(const type_info *)__ti20RaceTimeTrialDisplay;
}
#endif

// FUNC 0x00188EC0 TimeTrialExaminationMessages::getState
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (no
 * callers): state */

// FUNC 0x00188EC8 TimeTrialExaminationMessages::nextState
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (no
 * callers): state++ */

// FUNC 0x00188ED8 TimeTrialExaminationMessages::setState
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (no
 * callers): state = s */

#ifndef BOOT_OLD_GCC
// FUNC 0x00188EE0 __tf28TimeTrialExaminationMessages
//   Builds the __si_type_info __ti28TimeTrialExaminationMessages (base
//   ExaminationDisplayer, gp-0x46B0) on the first call.
extern "C" const type_info &__tf28TimeTrialExaminationMessages(void)
{
    if (__ti28TimeTrialExaminationMessages[0] == 0) {
        __tf20ExaminationDisplayer();
        __rtti_si(__ti28TimeTrialExaminationMessages, "28TimeTrialExaminationMessages",
                  (const type_info *)__ti20ExaminationDisplayer);
    }
    return *(const type_info *)__ti28TimeTrialExaminationMessages;
}
#endif

/* ------------------------------------------------------------------------ */
/* ExaminationDisplayer (out-of-line copies of its inline methods)          */
/* ------------------------------------------------------------------------ */

// FUNC 0x00188F30 ExaminationDisplayer::deactivate
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy
 * (index.txt: TimeTrialExaminationMessages::virtual_7; slot 7 of both
 * ExaminationDisplayer vtables): active = 0 */

// FUNC 0x00188F38 ExaminationDisplayer::activate
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy
 * (index.txt: TimeTrialExaminationMessages::virtual_6): finished = 0,
 * active = 1 */

// FUNC 0x00188F48 ExaminationDisplayer::~ExaminationDisplayer
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy (its
 * vptr; deleting form: operator delete(this)) */
/* implicit: operator delete (flags & 1) */

// FUNC 0x00188F78 ExaminationDisplayer::renderWhenDisqualifying
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy
 * (empty) */

// FUNC 0x00188F80 ExaminationDisplayer::renderWhenPassing
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy
 * (index.txt: TimeTrialExaminationMessages::renderWhenPassing; empty) */

// FUNC 0x00188F88 ExaminationDisplayer::updateAlways
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy
 * (empty) */

// FUNC 0x00188F90 ExaminationDisplayer::updateWhenDisqualifying
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy
 * (empty) */

// FUNC 0x00188F98 ExaminationDisplayer::updateWhenPassing
/* inline in game/race/display/T141_race_time_trial_display.h; egcs emits this copy
 * (index.txt: TimeTrialExaminationMessages::updateWhenPassing; empty) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00188FA0 __tf20ExaminationDisplayer
//   Builds the __user_type_info __ti20ExaminationDisplayer (gp-0x46B0) on
//   the first call.  Also called by __tf26LicenseExaminationMessages
//   (T146).
extern "C" const type_info &__tf20ExaminationDisplayer(void)
{
    if (__ti20ExaminationDisplayer[0] == 0)
        __rtti_user(__ti20ExaminationDisplayer, "20ExaminationDisplayer");
    return *(const type_info *)__ti20ExaminationDisplayer;
}
#endif
