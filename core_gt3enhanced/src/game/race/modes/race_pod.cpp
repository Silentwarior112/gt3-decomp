/* race_pod.cpp - the Toyota Pod race: its stop-area rules and the race
 * classes RacePOD / RacePOD2P (the enhanced engine; new code, no GT3
 * original: Lupo 2's DynamicsConductor members L2 0x001F75C8..0x001F7DBC and
 * its classes L2 0x0018D420..0x0018D7C8; race_logic.md M2).  Declared in
 * include/game/race/modes/race_pod_rules.h and race_pod.h, which say what
 * starts the mode: with GT3's content (no race record with licenseType 5) it
 * never runs.
 *
 * Callers of the rules: RaceBase::virtual_9 (pod_rules_init) and
 * RaceBase::virtual_29 (pod_rules_step), T129; the AI (T366); the conductors'
 * lap end (pod_rules_lap_check, physics area).  The two classes are
 * registered at start-up like GT3's race classes; their runners are in
 * race_pod_runners.cpp. */
#include "types.h"
#include "libc.h"                           /* printf (the log lines) */
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (the construct functions) */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* (UnitArena<64>, as the other race classes' files) */
#include "game/race/display/T154_race_display.h"         /* the race displays */
#include "game/race/display/T141_race_time_trial_display.h"  /* RaceTimeTrialDisplay */
#include "game/race/display/T194_race_display_event.h"   /* RaceDisplayInformationEvent, RD_INFO_POD_SAT_DOWN */
#include "game/race/modes/race_pod.h"       /* own: RacePOD, RacePOD2P; the rules */
#include "game/race/core/T122_race_event_queue.h"     /* RACE_EVENT_CAR_DISQUALIFIED, RACE_EVENT_END */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "game/race/core/T129_race_base.h"            /* RaceClassInfo, RaceClassNode, race_class_register */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (contact, podParking), DynCarState */
#include "game/car/physics/T331_dyn_car_message.h"      /* DynCar_setMessage, CARMSG_POD_STOP / _RETURN */

/* T341's CTRL_STOP_ROLL request (declared in car/physics's
 * car_input_internal.h only): the car rolls to a stop on line 6. */
int car_mode_stop_roll(RaceCarState *car);  /* 0x001E3DF0 */

/* ======================================================================== */
/* The stop-area rules                                                      */
/* ======================================================================== */

s16 g_podStopStart = 608;                   /* Lupo 2 .sdata L2 0x0035E010 */
u8  g_podStopLength = 33;                   /* L2 0x0035E012 */

PodRaceRules g_podRules;

/* 1 when the rules run on this conductor. */
static int pod_rules_mine(DynamicsConductor *dc)
{
    return g_podRules.dc == dc && g_podRules.on == 1;
}

/* Lupo 2 L2 0x001F75C8, from DynamicsConductor::virtual_20: the mode is on
 * for a race whose rules have licenseType 5 when the stop area (moved into
 * 0..lap) fits into the lap.  A lap length of 0 leaves it off (Lupo 2's
 * loop would not end). */
void pod_rules_init(DynamicsConductor *dc)
{
    PodRaceRules *r = &g_podRules;
    f32 start, length, lap;
    int i;

    r->dc = dc;
    r->on = 0;
    if (dc->rules.licenseType != POD_RULES_LICENSE_TYPE)
        return;
    if (g_podStopStart == 0)
        return;
    start = (f32)g_podStopStart;
    if (g_podStopLength == 0)
        return;
    length = (f32)g_podStopLength;
    lap = dc->getLapLength();
    if (!(0.0f < lap))
        return;
    while (start < 0.0f)
        start += lap;
    while (lap < start)
        start -= lap;
    if (lap < start + length)
        return;
    r->areaStart = start;
    r->areaLength = length;
    r->on = 1;
    printf("[enhanced] pod race: on, stop area %d..%d m of a %d m lap\n",
           (int)start, (int)(start + length), (int)lap);
    for (i = 0; i < 8; i++) {
        r->stops[i] = 0;
        r->stopX[i] = 0.0f;
        r->stopY[i] = 0.0f;
        r->stopDist[i] = 0.0f;
    }
}

/* Lupo 2 L2 0x001F7708 (dc +0xF088 == 1; 11 callers). */
int pod_rules_on(DynamicsConductor *dc)
{
    return pod_rules_mine(dc);
}

/* Lupo 2 L2 0x001F7720. */
int pod_rules_area(DynamicsConductor *dc, f32 *start, f32 *length)
{
    if (!pod_rules_mine(dc))
        return 0;
    *start = g_podRules.areaStart;
    *length = g_podRules.areaLength;
    return 1;
}

/* Lupo 2 L2 0x001F7788 (the height it is passed is not kept). */
int pod_rules_set_stop_point(DynamicsConductor *dc, int car, f32 x, f32 y, f32 dist)
{
    if (!pod_rules_mine(dc))
        return 0;
    if (car < 0 || !(car < dc->carCount))
        return 0;
    g_podRules.stopX[car] = x;
    g_podRules.stopY[car] = y;
    g_podRules.stopDist[car] = dist;
    return 1;
}

/* Lupo 2 L2 0x001F7840. */
int pod_rules_get_stop_point(DynamicsConductor *dc, int car, f32 *x, f32 *y, f32 *dist)
{
    if (!pod_rules_mine(dc))
        return 0;
    if (car < 0 || !(car < dc->carCount))
        return 0;
    *x = g_podRules.stopX[car];
    *y = g_podRules.stopY[car];
    *dist = g_podRules.stopDist[car];
    return 1;
}

/* Lupo 2 L2 0x001F79D0: one stop a lap. */
int pod_rules_pending(DynamicsConductor *dc, int car)
{
    if (!pod_rules_mine(dc))
        return -1;
    return dc->getCar(car)->st.lap - g_podRules.stops[car];
}

/* Lupo 2 L2 0x001F7A38: "it sat down!" (information 7, hud.md C18) for a car
 * its driver drives, unless two cars race (the 2-player race). */
int pod_rules_stop_done(DynamicsConductor *dc, int car)
{
    u8 *stops;

    if (!pod_rules_mine(dc))
        return -1;
    if (dc->carCount != 2) {
        DynCarState *st = &dc->getCar(car)->st;

        if (st->driver != DRIVER_AUTO && st->driver != DRIVER_COMPUTER && st->ctrlMode == CTRL_DRIVER) {
            RaceDisplayInformationEvent ev;

            ev.car = car;
            ev.information = RD_INFO_POD_SAT_DOWN;
            ev.post(dc->org);
        }
    }
    stops = &g_podRules.stops[car];
    if (*stops < 0xFF)
        (*stops)++;
    printf("[enhanced] pod race: car %d sat down, stop %d done\n", car, *stops);
    return *stops;
}

/* Lupo 2 L2 0x001F7B20, from DynamicsConductor::virtual_17 and
 * DynamicsConductorBattle2P::virtual_17 before anything else of the lap's
 * end.  Lupo 2 also sets the Pod's emotion 7 (sad) held (L2 0x001E1308,
 * physics' emotion machine): the caller does that here. */
int pod_rules_lap_check(DynamicsConductor *dc, int car)
{
    RaceCarState *c;
    DynCarState *st;

    if (!pod_rules_mine(dc))
        return 1;
    if (pod_rules_pending(dc, car) < 2)
        return 1;
    c = dc->getCar(car);
    st = &c->st;
    printf("[enhanced] pod race: car %d disqualified (lap %d, %d stops owed)\n",
           car, st->lap, pod_rules_pending(dc, car));
    car_mode_stop_roll(c);
    if (st->lap > 0)
        st->lap--;
    dc->org->eventQueue.post(RACE_EVENT_CAR_DISQUALIFIED, car, 0);
    return 0;
}

/* Lupo 2 L2 0x001F7BF0.  The player's car (its driver drives, and it has
 * crossed the start line: lapAdjust 0): all four wheels in the area - a
 * sitting Pod is done, else "stop and pull the side brake"; past the area -
 * "go back to the stop area".  A car the computer drives: done when it sits
 * past the area's start; in the area, the first message. */
void pod_rules_step(DynamicsConductor *dc)
{
    f32 start, length;
    int i;

    if (!pod_rules_mine(dc))
        return;
    pod_rules_area(dc, &start, &length);
    for (i = 0; i < dc->carCount; i++) {
        RaceCarState *car;
        DynCarState *st;
        f32 at;
        int w, inArea, allIn;

        if (pod_rules_pending(dc, i) <= 0)
            continue;
        car = dc->getCar(i);
        st = &car->st;
        at = st->lapDistance;
        inArea = 0;
        for (w = 0; w < 4; w++) {
            if (car->contact[w].attr2 & POD_WHEEL_IN_AREA)
                inArea++;
        }
        allIn = (inArea == 4);
        if (st->driver == DRIVER_AUTO || st->driver == DRIVER_COMPUTER || st->ctrlMode != CTRL_DRIVER) {
            if (start < at && car->podParking == POD_PARKED)
                pod_rules_stop_done(dc, i);
            else if (allIn)
                DynCar_setMessage(car, CARMSG_POD_STOP);
        } else if (st->lapAdjust == 0) {
            if (allIn) {
                if (car->podParking == POD_PARKED)
                    pod_rules_stop_done(dc, i);
                else
                    DynCar_setMessage(car, CARMSG_POD_STOP);
            } else if (start + length < at) {
                DynCar_setMessage(car, CARMSG_POD_RETURN);
            }
        }
    }
}

/* ======================================================================== */
/* RacePOD                                                                  */
/* ======================================================================== */

void *RacePOD::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

void RacePOD::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

/* Lupo 2 L2 0x0018D420. */
const char *RacePOD::virtual_24()
{
    return "RacePOD";
}

/* Lupo 2 L2 0x0018D430: new RacePOD. */
RaceBase *RacePOD_create(void)
{
    return new RacePOD;
}

/* Lupo 2 L2 0x0018D468: a RacePOD in buf (NULL: none); arg unused. */
RaceBase *RacePOD_construct(void *buf, int arg)
{
    if (buf == NULL)
        return NULL;
    return ::new (buf) RacePOD;
}

/* The registry record (Lupo 2 L2 0x002DD520). */
RaceClassInfo g_racePODInfo = {
    "RacePOD", sizeof(RacePOD), RacePOD_create, RacePOD_construct
};

/* Lupo 2 L2 0x0018D498: RaceArcade, the Pod display (RacePODDisplay, the
 * hud area's), which becomes the race display; it gets the player's pad, as
 * RaceTimeTrial's display does (the disqualification message's skip). */
RacePOD::RacePOD()
{
    setRaceDisplay(&podDisplay);
    podDisplay.setInput(&input);
}

/* Lupo 2 L2 0x0018D548. */
RacePOD::~RacePOD()
{
}

/* ======================================================================== */
/* RacePOD2P                                                                */
/* ======================================================================== */

void *RacePOD2P::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

void RacePOD2P::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

/* Lupo 2 L2 0x0018D628. */
const char *RacePOD2P::virtual_24()
{
    return "RacePOD2P";
}

/* Lupo 2 L2 0x0018D638. */
RaceBase *RacePOD2P_create(void)
{
    return new RacePOD2P;
}

/* Lupo 2 L2 0x0018D670. */
RaceBase *RacePOD2P_construct(void *buf, int arg)
{
    if (buf == NULL)
        return NULL;
    return ::new (buf) RacePOD2P;
}

/* The registry record (Lupo 2 L2 0x002DD530). */
RaceClassInfo g_racePOD2PInfo = {
    "RacePOD2P", sizeof(RacePOD2P), RacePOD2P_create, RacePOD2P_construct
};

/* Lupo 2 L2 0x0018D6A0: the split battle and the Pod's split display
 * (RacePOD2PDisplay, the hud area's), which becomes the race display and the
 * battle's (RaceSplitBattle::podDisplay; Lupo 2's getDisplay). */
RacePOD2P::RacePOD2P()
{
    setRaceDisplay(&pod2PDisplay);
    podDisplay = &pod2PDisplay;
}

/* Lupo 2 L2 0x0018D740. */
RacePOD2P::~RacePOD2P()
{
}

/* ======================================================================== */
/* The registration                                                         */
/* ======================================================================== */

static RaceClassNode podNode;
static RaceClassNode pod2PNode;

/* Registers both classes at start-up (Lupo 2's static constructors, as GT3's
 * race_arcade_static_ctor): egcs makes this initialiser the file's static
 * constructor, which the link calls after the original ones. */
static int race_pod_register(void)
{
    race_class_register(&podNode, &g_racePODInfo);
    race_class_register(&pod2PNode, &g_racePOD2PInfo);
    return 1;
}

static int podRegistered = race_pod_register();
