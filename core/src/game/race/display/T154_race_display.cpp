/* T154_race_display.cpp - RaceDisplay, the head-up display of a race
 * (translation unit T154, 0x001909E0..0x00195B88; units u051 / u052, one
 * object as in the original).  See race_display_internal.h and include/game/race/display/T154_race_display.h.
 *
 * The race display pulls its texts from the race event queue
 * (RaceOrganization::eventQueue) every frame: the poll* methods walk the
 * pending events of one display event class (lap time, check point, lap /
 * time difference, speed, information) with RaceDisplayEventBase::peek and
 * put the matching ones on the event board or the time displays.
 * virtual_3 draws the enabled objects of the enabled panels (RD_PANEL_*),
 * filtered by the display items (RD_SHOW_*).  setLayout places the objects
 * from the race mode's racedetail.db records (g_002F00C0); the two
 * tachometer layouts are the tables g_002BC080 / g_002BC120.
 *
 * <new> first: retail's T154 .rodata opens with "bad_alloc" (0x0032DF90,
 * nothing references it; <new>'s inline what(), DATA.md) and 6 zero bytes
 * before "race/%s/display.imgs" (u050's data pass, ref/data/u051/pending.md).
 */
// DATA_START 0x0032DF90
#include <new>
#include "game/race/display/race_display_internal.h"
#include "libc.h"                           /* strcpy, strcat, free, memset */
#ifndef BOOT_OLD_GCC                        /* egcs's <new> conflicts with it */
#include "cxxrt.h"                          /* __rtti_si, __rtti_user */
#endif

/* The include order is part of the match: egcs writes the type names of the
 * classes without a key function (and of those whose vtables T154 emits) in
 * the reverse of their definition order.  Retail's T154 .rodata
 * (0x0032DFA0..0x0032E610) has, after the vtables: the garage classes' names
 * (GameStatus's header, read last), GeneralTime, LapDiff, TimeDiff,
 * RaceDisplayEventBase (the display events, after the display), RaceDisplay,
 * MiniMap, OnboardSpeedmeter, the two round tachometers, MeterBase,
 * ValueDisplayBase, RaceDisplayBase (the display's header,
 * T154_race_display.h), then RaceCameraBase and UnitArena<64> (read before
 * the display's header here, unlike in its other includers), and no
 * Operator / OperatorHolder names: the file sees no menu class
 * (T216_menu_transitive.h is not included; T429_game_status.h reads only
 * the garage). */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (only the name) */
#include "game/race/core/T153_race_course.h"          /* UnitArena<64> */
#include "game/camera/T383_race_camera_base.h"     /* RaceCameraBase */

/* own translation unit */
#include "game/race/display/T154_race_display.h"         /* RaceDisplay, RaceDisplayBase, RaceMiniMap, RaceMeterLayout, the
                                               T154 data and free functions; T156, T157, T160 RaceMeterBase,
                                               T161, T162 RaceOnboardSpeedmeter, T163, T127 */
#define GAME_T194_EMITS_TYPEINFO            /* RaceDisplayEventBase's type name (only the name) */
#include "game/race/display/T194_race_display_event.h"   /* the display events (T154's difference events too) */

/* outside the unit */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar (state, info, transmission, lapTime,
                                               timeRecord) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::find / findNth / post, counts */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, raceDB, eventQueue, the records,
                                               countDown, timeLimit, timeRemain, loggerMode) */
#include "game/race/core/T126_race_lap_time.h"        /* RaceLapTime::total (RaceEntryCar::lapTime) */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getCourseName, RaceEvent (event) */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::getName1 / getName2 (RaceEntryCar::info) */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (st.rank), car_lap_display, car_lap_time,
                                               car_*_assist, car_oil_needs_change, car_handbrake_on */
#include "game/race/display/T279_meter_scale.h"          /* SelectMeterScale */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition::odometerDistance */
#include "game/race/display/T286_level_colour.h"         /* level_to_rgb */
#include "game/car/physics/T301_car_motion.h"           /* car_trip_display */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor (wear, getStartingFormat, virtual_29) */
#include "game/car/physics/T327_wear_curve.h"           /* WearCurve (DynamicsConductor::wear) */
#include "game/race/dynamics/T361_drive_params.h"         /* AIDriver (RaceCarState::ai: gear, gearSet) */
#include "game/save/T428_game_options.h"         /* GameOptions::replaySplit, displayFlags */
#include "game/data/T487_race_mode_database.h"   /* RaceModeDatabase::getModeId, RaceDetailRecord, g_002F00C0 */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, the RACEDISPLAY texts */
#include "game/sound/T176_car_sound.h"            /* race_se_play, the sound effects */
#include "game/sound/T469_sound_effects.h"        /* SE_COUNT_D / SE_COUNT_S */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon (RaceDisplay::icons) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer::setDrawBuffer */
#include "engine/text/T496_font.h"                 /* Font, g_002F1968 */
#include "engine/io/T506_image_pack.h"           /* image_pack_load / image_pack_find */
#include "game/race/display/T158_race_display_draw.h"    /* race_format_time */
#include "game/data/T516_country.h"              /* Country::code, g_00352FD8 */
#include "game/data/T518_unit_settings.h"        /* UnitSettings::convSpeed / speedName, g_002F59D8 */
#include "engine/gfx/T512_render_context.h"       /* g_002F424C (the wide / split screen mode), g_002F4250 (VRAM) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/pgl/G001_flare.h"                /* Tex1Image, GsTexRegs, gfx_alloc_tail, gfx_set_texture_regs,
                                               pgl_draw_strip, gfx_disable, gfx_tex_param, pgl_matrix_mode,
                                               pgl_ortho, vu0_load_identity_regs, pgl_color,
                                               gfx_reset_texture_state_spr, PGL_* */
/* last: the garage classes' names come first in retail's T154 type names */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::options */

// FUNC 0x001909E0 RaceDisplay::RaceDisplay
//   org and icons NULL, then the members in order (activeTacho NULL between
//   messageDF0 and onboardTacho): RaceMiniMap, RaceDisplayFader,
//   RaceLapDisplay, RaceRankDisplay, RaceLapTimesDisplay, RaceTimeDisplay x
//   8, RaceSpeedDisplay x 2, RaceEventDisplay x 2, RaceMessageDisplay x 2,
//   RaceReplayModeDisplay, RaceMessageDisplay, RaceOnboardTachometer,
//   RaceRemoteTachometer, RaceBattleTachometer, RaceOnboardSpeedmeter,
//   RaceBoostmeter, RaceOdometer x 2, RaceIndicators, RaceDigitalSpeedmeter,
//   RaceGearDisplay, RaceBackPanel, RaceMessageDisplay, RaceABMonitor,
//   RaceSteeringDisplay, RaceTireWearDisplay, RaceMessageDisplay x 2, the
//   RaceDisplayObjectBase mirror, RaceCarIconDisplay x 2, SpeedRecord.  The
//   exception landing pads undo the constructed ones (~RaceDisplayObjectBase,
//   ~RaceRoundMeter, ~RaceBoostmeter, the arrays' virtual destructors).
//   Then the flag bytes and the shared resources for the first display
//   alive.
RaceDisplay::RaceDisplay()
    : org(NULL), icons(NULL), activeTacho(NULL)
{
    /* implicit: RaceMiniMap, RaceDisplayObjectBase, RaceDisplayFader, RaceLapDisplay, RaceRankDisplay, RaceLapTimesDisplay, RaceTimeDisplay, RaceSpeedDisplay, RaceEventDisplay, RaceMessageDisplay, RaceReplayModeDisplay, RaceOnboardTachometer, RaceRemoteTachometer, RaceBattleTachometer, RaceOnboardSpeedmeter, RaceBoostmeter, RaceOdometer, RaceIndicators, RaceDigitalSpeedmeter, RaceGearDisplay, RaceBackPanel, RaceABMonitor, RaceSteeringDisplay, RaceTireWearDisplay, RaceCarIconDisplay, SpeedRecord (member constructors), ~RaceDisplayObjectBase, ~RaceRoundMeter, ~RaceBoostmeter (exception cleanup) */
    panels = RD_PANEL_TOP_LEFT | RD_PANEL_TOP_RIGHT | RD_PANEL_BOTTOM_LEFT |
             RD_PANEL_BOTTOM_RIGHT | RD_PANEL_CENTER;
    displayNumber = 0;
    inner = 1;
    /* The count is read before `live' is stored: a char store keeps every
     * later load behind it, and retail loads the count first. */
    int first = (g_00351FAC == 0);
    live = 0;
    if (first)
        loadResources();
    g_00351FAC++;
}

// FUNC 0x00191300 RaceDisplay::~RaceDisplay
//   The last display alive releases the shared resources; then the members
//   in reverse order, their inline destructors expanded (the vptr stores of
//   RaceTireWearDisplay, RaceSteeringDisplay, RaceABMonitor, RaceBackPanel,
//   RaceOnboardSpeedmeter, RaceBattleTachometer / RaceMeterBase,
//   RaceRoundTachometerBase, RaceMiniMap; ~RaceDisplayObjectBase,
//   ~RaceRoundMeter, ~RaceBoostmeter with flags 2; the arrays' elements
//   through their virtual destructors, flags 0).  Deleting form (flags bit
//   0): operator delete(this).
RaceDisplay::~RaceDisplay()
{
    if (--g_00351FAC == 0)
        releaseResources();
    /* implicit: ~RaceBoostmeter, ~RaceRoundMeter, ~RaceDisplayObjectBase (member destructors), operator delete (deleting destructor) */
}

// FUNC 0x00191830 RaceDisplay::loadResources
//   race/<country>/display.imgs (once) and the font font/menu.fnt (once).
void RaceDisplay::loadResources()
{
    char path[0x40];

    if (g_00351FA4 == NULL) {
        sprintf(path, "race/%s/display.imgs", g_00352FD8.code());
        g_00351FA4 = image_pack_load(path);
    }
    if (g_00351FA8 < 0)
        g_00351FA8 = g_002F1968.loadFile(g_00351FBC);
}

// FUNC 0x001918A0 RaceDisplay::releaseResources
//   The pack was loaded whole (image_pack_load: malloc), so free() releases
//   it.
void RaceDisplay::releaseResources()
{
    g_002F1968.releaseFile(g_00351FA8);
    g_00351FA8 = -1;
    free(g_00351FA4);
    g_00351FA4 = NULL;
}

/* T154's .sdata (after the "" the constructor emits): written here, after
 * loadResources, as the original's order shows ("race/%s/display.imgs" comes
 * before "font/menu.fnt" in .rodata, and virtual_0's "boost" after
 * g_00351FBC in .sdata). */
ImagePack  *g_00351FA4 = NULL;              /* 0x00351FA4 */
int         g_00351FA8 = -1;                /* 0x00351FA8 */
int         g_00351FAC = 0;                 /* 0x00351FAC */
int         g_00351FB0 = 0;                 /* 0x00351FB0 */
int         g_00351FB4 = 0;                 /* 0x00351FB4 */
u8          g_00351FB8 = 0;                 /* 0x00351FB8 */
const char *g_00351FBC = "font/menu.fnt";   /* 0x00351FBC */

// FUNC 0x001918D8 RaceDisplay::virtual_0
//   The set-up for a race: the organization, the race mode (the split
//   battles 7 / 8 by the screen mode; a race of 10 laps or more, or one with
//   a time limit, is shown as RACE_MODE_ENDULANCE, except free practice),
//   the meters, the record times with their medals, the lap counters, the
//   mode message (QUALIFY / DEMONSTRATION / LIVE / REPLAY) and, live or in a
//   replay (not in a demo), the course name in capitals as the title.  Live
//   free practice also posts the information event "attack after one
//   warm-up lap".
void RaceDisplay::virtual_0(RaceOrganization *org, int raceMode, int replay)
{
    setOrganization(*org);
    this->raceMode = raceMode;
    g_00351FB0 = 0;
    g_00351FB4 = 0;
    this->replay = replay;
    layoutVariant = -1;
    currentVariant = -1;
    unkA470 = 0;
    finished = 0;
    g_00351FB8 = 0;
    playerCar = 0;
    displayCar = -1;
    leaderCar = -1;
    /* The option and the mode are read before the flag bytes are stored
     * (retail loads both ahead of those char stores). */
    int split = getGameStatus()->getGameOptions()->replaySplit;
    int mode = this->raceMode;
    endurance = 0;
    splitReplay = split;
    unkA476 = 0;
    noLapDisplay = 0;
    finishDisplay = 1;

    if ((u32)(mode - RD_MODE_SPLIT_A) < 2) {
        this->raceMode = (g_002F424C != 0) ? RD_MODE_SPLIT_B : RD_MODE_SPLIT_A;
    } else {
        RaceEvent *race = org->raceDB->event;
        int timeLimited = race->limitTime != 0;
        int raceLaps = race->numberOfLaps;  /* (a signed compare: slti) */
        int longRace = raceLaps >= 10;
        if (this->raceMode != g_002F00C0.getModeId("RACE_MODE_FREE_PRACTICE") &&
            (longRace || timeLimited)) {
            int endu = g_002F00C0.getModeId("RACE_MODE_ENDULANCE");
            endurance = 1;
            this->raceMode = endu;
        }
    }
    displayMode = this->raceMode;

    qualify = (this->raceMode == g_002F00C0.getModeId("RACE_MODE_FREE_PRACTICE"));
    RaceMessageDisplay *t = &title;
    t->x = 0x140;                           /* the centre of the screen */
    t->y = 0xE0;
    t->clear();
    fader.reset();
    miniMap.init(org);
    onboardTacho.meter.imagePack = g_00351FA4;
    remoteTacho.meter.imagePack = g_00351FA4;
    battleTacho.bar.imagePack = g_00351FA4;
    speedometer.imagePack = g_00351FA4;
    RaceBoostmeter *boost = &boostMeter;
    boost->imagePack = g_00351FA4;
    boost->setImage("boost");
    tripMeter.setDigits(4);
    carName.setFont("sw_16");
    carName.setColor(RD_COLOR_TEXT);
    lapTimes.reset();
    lastTime.setTime(-1);

    /* the record times: min * 60000 + sec * 1000 + ms (the race record is
     * reached through org again for each one) */
    RaceTimeDisplay *first = &recordTimes[0];
    first->setTime(org->raceDB->event->time[0].min * 60000 + org->raceDB->event->time[0].sec * 1000 + org->raceDB->event->time[0].ms);
    first->mark = race_display_find_image("mark_1st");
    RaceTimeDisplay *second = &recordTimes[1];
    second->setTime(org->raceDB->event->time[1].min * 60000 + org->raceDB->event->time[1].sec * 1000 + org->raceDB->event->time[1].ms);
    second->mark = race_display_find_image("mark_2nd");
    RaceTimeDisplay *third = &recordTimes[2];
    third->setTime(org->raceDB->event->time[2].min * 60000 + org->raceDB->event->time[2].sec * 1000 + org->raceDB->event->time[2].ms);
    third->mark = race_display_find_image("mark_3rd");

    recordTime.setLabel(g_002C43E8[RACEDISPLAY_PP_TIME_LABEL]);
    SpeedRecord *top = &topSpeed;
    *top = org->speedRecord;

    DynamicsConductor *dc = org->entries->cars[0]->state->dc;
    hasTireWear = (dc->wear.x3 != 0.0f);
    int laps = dc->virtual_29();
    if (laps >= 4 || laps == 0)
        lapTimes.setLines(3);
    else
        lapTimes.setLines(laps);
    lapDisplay.laps = laps;

    setTargetCarNumber(0);
    setDisplayCar(playerCar);
    setLayout(0);
    updateDisplayMode();
    showLeaderIcon();
    events.reset();
    modeMessage.setFont("sw_20*160,160");

    const char *modeText = "";
    if (replay == RD_LIVE) {
        if (qualify)
            modeText = g_002C43E8[RACEDISPLAY_QUALIFY_MODE];
    } else if (org->demonstration != 0) {
        modeText = g_002C43E8[RACEDISPLAY_DEMO_MODE];
    } else if (live) {
        modeText = g_002C43E8[RACEDISPLAY_LIVE_MODE];
    } else {
        modeText = g_002C43E8[RACEDISPLAY_REPLAY_MODE];
    }
    modeMessage.setText(modeText, 0.0f, 0.0f);

    if ((u32)replay < 2) {                  /* RD_LIVE or RD_REPLAY */
        RaceDataBase *db = org->raceDB;
        /* Quirk: getCourseName returns NULL for an unknown course; it is
         * dereferenced without a check. */
        const char *name = db->getCourseName(org->courseCode);
        title.clear();
        modeMessage.clear();
        if (*name != '-') {
            char courseName[0x80];          /* sp+0x00 */

            title.setFont("sw_20*160,160");
            title.setColor(RD_COLOR_TEXT);
            strcpy(courseName, db->getCourseName(org->courseCode));
            for (char *p = courseName; *p != '\0'; p++) {
                int c = *p;
                if ((u32)(c - 'a') < 26)
                    c &= ~0x20;             /* to upper case */
                *p = c;
            }
            title.setText(courseName, 0.0f, 0.0f);
            modeMessage.setBlink(RD_MODE_MESSAGE_BLINKS);
            switch (dc->getStartingFormat()) {
            case START_FORMAT_STANDING:
                title.setFadeTime(0.3f, 1.0f);
                title.show(4.0f, 1.0f);
                modeMessage.setFadeTime(0.3f, 1.0f);
                modeMessage.show(4.0f, 1.0f);
                events.addEvent("", 5.0f, 0.0f);
                break;
            case START_FORMAT_ROLLING:
            case START_FORMAT_FREE_RUN:
            case START_FORMAT_FREE_PRACTICE:
                title.setFadeTime(0.1f, 0.3f);
                title.show(2.0f, 0.2f);
                modeMessage.setFadeTime(0.1f, 0.3f);
                modeMessage.show(2.0f, 0.2f);
                events.addEvent("", 3.0f, 0.0f);
                break;
            }
        }
    }

    if (replay == RD_LIVE) {
        virtual_17();
        if (qualify) {
            RaceDisplayInformationEvent ev;     /* sp+0x00; RaceDisplayInformationEvent() */
            ev.information = RD_INFO_READY_QUALIFY;
            ev.car = 0;
            ev.post(org);
        }
    }
}

// FUNC 0x00192010 RaceDisplay::updateDisplayMode
//   The mode shown: in a replay RACE_MODE_REPLAY (the split battles 7 / 8
//   keep their own mode when the split screen replay option is on; race
//   modes 4 and 9 also hide the lap display), in a demo RACE_MODE_DEMO (not
//   for the split battles).  Remembers the option.
//   Matching (83.6%): retail copies the mode into a second register (daddu
//   in the first beq's slot) for the split test and has its own allocation
//   at the top.  Tried: the 4 / 9 test on raceMode, a switch, a copy, an
//   inline split test, early-return and else orders.
void RaceDisplay::updateDisplayMode()
{
    /* The option and the replay mode are read before the char store (retail
     * loads both ahead of it). */
    int split = getGameStatus()->getGameOptions()->replaySplit;
    currentVariant = -1;
    int r = replay;
    splitReplay = split;
    switch (r) {
    case RD_LIVE:                           /* shapes retail's case tree: == 1, < 2, == 2, == 3 */
        break;
    case RD_REPLAY:
    case RD_REPLAY_ALT: {
        int mode = raceMode;
        if (mode == 4 || mode == 9)
            noLapDisplay = 1;
        if ((u32)(mode - RD_MODE_SPLIT_A) >= 2)
            displayMode = g_002F00C0.getModeId("RACE_MODE_REPLAY");
        else if (splitReplay == 0)
            displayMode = g_002F00C0.getModeId("RACE_MODE_REPLAY");
        else
            displayMode = mode;
        break;
    }
    case RD_DEMO:
        if ((u32)(raceMode - RD_MODE_SPLIT_A) >= 2)
            displayMode = g_002F00C0.getModeId("RACE_MODE_DEMO");
        break;
    }
}

// FUNC 0x00192120 RaceDisplay::virtual_1
void RaceDisplay::virtual_1()
{
}

// FUNC 0x00192128 RaceDisplay::virtual_9
//   The layout variant virtual_2 hands to setLayout, from the view camera's
//   car mount (VariableCameraBase::getMount, slot 13; the races, u046):
//   mount 0 variant 0, any other mount or none (-1) variant 1.  (The
//   header's parameter name is `on'.)
void RaceDisplay::virtual_9(int mount)
{
    if (mount == 0)
        layoutVariant = 0;
    else
        layoutVariant = 1;
}

// FUNC 0x00192140 RaceDisplay::setDisplayCar
//   Sets the meters up for car `car' of the race when it changes.
void RaceDisplay::setDisplayCar(int car)
{
    if (displayCar != car) {
        displayCar = car;
        setupCarMeters(org->entries->cars[car]);
    }
}

// FUNC 0x00192190 RaceDisplay::setupCarMeters
//   The scales of the tachometers (the dial's end, the red zone), the
//   speedometer (its unit and end) and the boost meter (shown for a turbo
//   car).  The setScale calls are direct (members by value).
//   setTransmission (T161) and setVisible (T163) are inline members: retail
//   stores through each meter's address as the integrated calls do, and
//   loads car->transmission as a word (setTransmission takes an int; u056
//   made them inline, they were stand-ins here).
//   Matching (97.8%): retail stores the battle meter's transmission first
//   and its bar scale in the setupScale call's delay slot, with `unit != 0'
//   computed before both (the two stores are independent there).  The
//   order transmission, scale (as for the round meters) gives the bar
//   scale and the unit test the other way round (93.3%); a stand-in
//   setRedZone (bar.scale through the meter's address) gives retail's order
//   but not its address (97.5%).
void RaceDisplay::setupCarMeters(RaceEntryCar *car)
{
    int unit;                               /* sp+0x0 the speed unit: 0 km/h, 1 mph */
    int speedScale;                         /* sp+0x4 */
    int redZone;                            /* sp+0x8 rpm */
    int tachoScale;                         /* sp+0xC rpm */
    RaceCarState *state = car->state;

    SelectMeterScale(state, &unit, &speedScale, &redZone, &tachoScale);

    onboardTacho.setScale((f32)tachoScale);
    onboardTacho.setTransmission(car->transmission);
    onboardTacho.meter.setRedZone((f32)redZone);
    onboardTacho.meter.updateScale();

    remoteTacho.setScale((f32)tachoScale);
    remoteTacho.setTransmission(car->transmission);
    remoteTacho.meter.setRedZone((f32)redZone);
    remoteTacho.meter.updateScale();

    battleTacho.setScale((f32)tachoScale);
    battleTacho.bar.scale = (f32)redZone;   /* +0x7038 */
    battleTacho.setTransmission(car->transmission);
    speedometer.setupScale(unit != 0, (f32)speedScale);
    speedometer.updateScale();

    boostMeter.setVisible(state->params.engine.turboMax != 0.0f);
}

// FUNC 0x001922F8 race_display_tex_uvs
//   The texture coordinates of the four corners of a w x h image in its
//   power-of-two texture (at most 0x400 + 1 steps up): (0,0) (u,0) (0,v)
//   (u,v), from the prim buffer.
RaceDisplayUV *race_display_tex_uvs(int w, int h)
{
    RaceDisplayUV *uv = (RaceDisplayUV *)gfx_alloc_tail(2);
    int texW = 1;
    while (texW < 0x401 && texW < w)
        texW <<= 1;
    f32 u = (f32)w / (f32)texW;
    int texH = 1;
    while (texH < 0x401 && texH < h)
        texH <<= 1;
    f32 v = (f32)h / (f32)texH;
    uv[0].u = 0.0f;
    uv[0].v = 0.0f;
    uv[1].u = u;
    uv[1].v = 0.0f;
    uv[2].u = 0.0f;
    uv[2].v = v;
    uv[3].u = u;
    uv[3].v = v;
    return uv;
}

// FUNC 0x001923F8 race_display_draw_tex
//   Draws a texture centred on (x, y) at its own size (CLAMP MAXU / MAXV
//   + 1).  Called by the mini map (RaceMiniMap::drawCar, 0x0019E560).
//   Matching (98.0%): retail gives x + halfW $f3 and y + halfH $f2 (ours the
//   other way round, a local-alloc priority tie).  Tried: the 24 orders of
//   four corner locals with either half first, inline sums, both operand
//   orders, the stores by vertex, by coordinate, z first / last.
void race_display_draw_tex(Tex1Image *tex, f32 x, f32 y)
{
    if (tex != NULL) {
        u64 clamp = tex->tex->clamp;
        int w = (int)(((clamp >> 14) & 0x3FF) + 1);
        int h = (int)(((clamp >> 34) & 0x3FF) + 1);
        RaceDisplayVertex *vtx = (RaceDisplayVertex *)gfx_alloc_tail(3);
        f32 halfH = (f32)h * 0.5f;
        f32 halfW = (f32)w * 0.5f;
        vtx[0].x = x - halfW;
        vtx[0].y = y - halfH;
        vtx[0].z = 0;
        vtx[1].x = x + halfW;
        vtx[1].y = y - halfH;
        vtx[1].z = 0;
        vtx[2].x = x - halfW;
        vtx[2].y = y + halfH;
        vtx[2].z = 0;
        vtx[3].x = x + halfW;
        vtx[3].y = y + halfH;
        vtx[3].z = 0;
        RaceDisplayUV *uv = race_display_tex_uvs(w, h);
        g_002F4250.loadTex1(tex);
        gfx_set_texture_regs(tex->tex);
        pgl_draw_strip(vtx, NULL, NULL, uv, 4);
    }
}

// FUNC 0x00192528 race_display_draw_rect
//   An untextured rectangle (x, y)..(x + w, y + h).  No callers.
void race_display_draw_rect(int x, int y, int w, int h)
{
    RaceDisplayVertex *vtx = (RaceDisplayVertex *)gfx_alloc_tail(3);
    f32 x0 = (f32)x;
    f32 y0 = (f32)y;
    f32 x1 = (f32)(x + w);
    f32 y1 = (f32)(y + h);
    vtx[0].x = x0;
    vtx[0].y = y0;
    vtx[0].z = 0;
    vtx[1].x = x1;
    vtx[1].y = y0;
    vtx[1].z = 0;
    vtx[2].x = x0;
    vtx[2].y = y1;
    vtx[2].z = 0;
    vtx[3].x = x1;
    vtx[3].y = y1;
    vtx[3].z = 0;
    pgl_draw_strip(vtx, NULL, NULL, NULL, 4);
}

// FUNC 0x001925E8 RaceDisplay::drawCountdown
//   (Called by virtual_3.)  While the start countdown runs, live only: at
//   each whole second below 4 the title shows "@count<n>" (the image
//   count<n>) for a second.
void RaceDisplay::drawCountdown(int frames)
{
    char text[0x10];

    if (g_00351FB0 != 0 && replay == RD_LIVE) {
        int sec = frames / 60;
        if (frames % 60 != 0)
            return;
        if (sec < 4) {
            sprintf(text, "@count%d", sec);
            title.setText(text, 1.0f, 0.0f);
        }
    }
}

// FUNC 0x00192678 RaceDisplay::drawTimeLimit
//   (Called by virtual_3.)  "Time Limit" and "<n>sec." at the bottom left
//   (red below 10 seconds); "GAME OVER" in the middle at 0.
void RaceDisplay::drawTimeLimit(int seconds)
{
    char text[0x10];

    sprintf(text, "%dsec.", seconds);
    g_002F1968.setStyle("sw_12");
    g_002F1968.setPos(8, 0x190);
    g_002F1968.setColor(0.8f, 0.8f, 0.7f, 0.9f);
    g_002F1968.print(g_002C43E8[RACEDISPLAY_TIME_LIMIT]);
    g_002F1968.setPos(8, 0x1A0);
    g_002F1968.setColor(0.8f, 0.8f, 0.7f, 0.9f);
    if (seconds < 10)
        g_002F1968.setColor(0.8f, 0.2f, 0.1f, 0.9f);
    g_002F1968.print(text);
    if (seconds == 0) {
        g_002F1968.setStyle("sw_20");
        g_002F1968.setPos(0xEA, 0xE0);
        g_002F1968.print(g_002C43E8[RACEDISPLAY_GAME_OVER]);
    }
}

// FUNC 0x001927C0 F_001927C0
//   An empty function (no callers).
void F_001927C0(void)
{
}

// FUNC 0x001927C8 F_001927C8
//   An empty function (called by T367 0x001F8780).
void F_001927C8(void)
{
}

// FUNC 0x001927D0 F_001927D0
//   An empty function (called by T367 0x001F8888 with a signed byte).
void F_001927D0(int a)
{
}

// FUNC 0x001927D8 F_001927D8
//   An empty function (called by T367 0x001F88F8, $a0 set to the value at
//   the call: an int argument, as the canonical header declares).
void F_001927D8(int a)
{
}

// FUNC 0x001927E0 RaceDisplay::showWarning
//   The car's message (DynCarState::msgKind, T331; jump table 0x0032E070):
//   1, 2 "WRONG WAY" in red; 4, 5, 7..10, 12 "AUTO DRIVE"; each for 0.2 s
//   (renewed every frame).
void RaceDisplay::showWarning(int warning)
{
    if (warning == 0)
        return;
    switch (warning) {
    case 1:
    case 2:
        warningMessage.setColor(RD_COLOR_WARNING);
        warningMessage.setText(g_002C43E8[RACEDISPLAY_WRONG_WAY], 0.2f, 0.0f);
        break;
    case 4:
    case 5:
    case 7:
    case 8:
    case 9:
    case 10:
    case 12:
        warningMessage.setColor(RD_COLOR_MESSAGE);
        warningMessage.setText(g_002C43E8[RACEDISPLAY_AUTO_DRIVE], 0.2f, 0.0f);
        break;
    }
}

// FUNC 0x00192880 race_display_poll_events
//   Called by the races every frame before the displays: takes the
//   countdown event (the beep at each second below 4 s of the countdown),
//   the start event (its sound, g_00351FB4) and the "replay record full"
//   events (bit 0 of the history g_00351FB8).
void race_display_poll_events(RaceOrganization *org)
{
    RaceEventQueue *queue = &org->eventQueue;

    if (queue->find(RACE_EVENT_COUNTDOWN, NULL, 1))
        g_00351FB0 = 1;
    if (g_00351FB0 != 0) {
        int frames = org->countDown;
        if (frames == 0)
            g_00351FB0 = 0;
        else if (frames < 4 * 60 && frames % 60 == 0)
            race_se_play(SE_COUNT_D);
    }
    if (queue->find(RACE_EVENT_GO, NULL, 1)) {
        race_se_play(SE_COUNT_S);
        g_00351FB4 = 1;
    } else {
        g_00351FB4 = 0;
    }
    g_00351FB8 = g_00351FB8 << 1;
    while (queue->find(RACE_EVENT_RECORD_FULL, NULL, 1))
        g_00351FB8 = g_00351FB8 | 1;
}

// FUNC 0x00192970 race_display_drop_events
//   Called by the races every frame after the displays: removes the display
//   events left in the queue.
void race_display_drop_events(RaceOrganization *org)
{
    RaceEventQueue *queue = &org->eventQueue;

    while (queue->find(RACE_EVENT_LAP_TIME, NULL, 1))
        ;
    while (queue->find(RACE_EVENT_CHECK_POINT, NULL, 1))
        ;
    while (queue->find(RACE_EVENT_GENERAL_TIME, NULL, 1))
        ;
    while (queue->find(RACE_EVENT_VIEW_MODE, NULL, 1))
        ;
    while (queue->find(RACE_EVENT_TIME_DIFF, NULL, 1))
        ;
    while (queue->find(RACE_EVENT_LAP_DIFF, NULL, 1))
        ;
    while (queue->find(RACE_EVENT_INFORMATION, NULL, 1))
        ;
    while (queue->find(RACE_EVENT_GOAL, NULL, 1))
        ;
}

/* min.s: egcs's <? operator (GCC 14 has none). */
#ifdef BOOT_OLD_GCC
#define RD_MINF(a, b) ((a) <? (b))
#else
#define RD_MINF(a, b) ((a) < (b) ? (a) : (b))
#endif

// FUNC 0x00192A58 RaceDisplay::virtual_2
//   The per-frame update: the layout and the meters of the car shown
//   (targetCar, car 0 when negative), the display events of the frame
//   (the poll* methods, below), the start display, each goal event of the
//   frame (virtual_16), the display mode when the split screen replay option
//   changed, the messages, the car's warning (not in the last second of a
//   timed race), and the elements from the car's state: rank, lap, lap
//   times, rpm, speed (in the speed unit), boost (at most 2.0), the records,
//   gear (shift lamp above the rev limit or while the limiter cuts),
//   odometer and trip, the lamps, the accel / brake bars, steering, the tyre
//   colours, the car name; the event 0x15, the car icons and the leader.
void RaceDisplay::virtual_2()
{
    u8 rgb[3];                              /* sp+0x00 */
    char carText[0x80];                     /* sp+0x10 */
    int goalCar;                            /* sp+0x90 */
    RaceOrganization *org = this->org;
    int car = targetCar;
    if (car <= -1)
        car = 0;

    setLayout(layoutVariant);
    setDisplayCar(car);
    pollLapTimeEvents(org, car);
    recordTime.time = org->lapRecord.total;
    pollCheckPointEvents(org, car);
    if (replay == RD_LIVE && (s8)g_00351FB8 == 1)  /* a record full event now, none in the 7 frames before */
        showReplayRecorded();
    pollLapDiffEvents(org, car);
    pollTimeDiffEvents(org, car);
    pollSpeedEvents(org, car);
    pollInformationEvents(org);
    if (g_00351FB4 != 0)
        put_start_display();

    int goals = org->eventQueue.counts[RACE_EVENT_GOAL];
    for (int i = 0; i < goals; i++) {
        if (org->eventQueue.findNth(RACE_EVENT_GOAL, &goalCar, i))
            virtual_16(org, goalCar, car);
    }

    if (splitReplay != getGameStatus()->getGameOptions()->replaySplit)
        updateDisplayMode();
    fader.update();
    events.update();
    title.update();
    modeMessage.update();

    RaceEntryCar *entry = org->entries->cars[car];
    RaceCarState *state = entry->state;
    if (org->timeLimit == 0 || org->timeRemain >= 60)
        showWarning(state->st.msgKind);
    warningMessage.update();
    rankDisplay.setRank(state->st.rank);
    lapDisplay.setCurrentLaps(car_lap_display(state));
    updateLapTimes(entry);
    if (activeTacho != NULL)
        activeTacho->set((f32)state->st.rpm);

    f32 speed = g_002F59D8.convSpeed(state->speed);
    speedometer.set(speed);
    digitalSpeed.setSpeed((int)speed);
    boostMeter.value = RD_MINF(state->st.turboFactor, 2.0f);
    topSpeed.update(state->speed);
    recordSpeed.setSpeed(org->speedRecord0.speed);
    topSpeedDisplay.setSpeed(topSpeed.speed);

    AIDriver *ai = &state->ai;
    int limit = state->params.gear.revLimit;    /* (retail loads it ahead of the char stores) */
    gearDisplay.setGear(state->st.gear);
    gearDisplay.setSuggestedGear(ai->gear);
    gearDisplay.setBright(state->st.clutchState == 1);
    int overRev = limit < state->st.rpm;
    u8 shiftLamp = 0;
    if (overRev || state->st.limiterOn != 0)
        shiftLamp = 1;
    gearDisplay.setShiftWarning(shiftLamp);
    gearDisplay.setSuggestBlink(ai->gearSet != 0);
    gearDisplay.update();

    odometer.setValue(state->st.condition.odometerDistance() / 10);
    tripMeter.setValue((int)(car_trip_display(state) * 0.01f));
    indicators.setBrake(car_handbrake_on(state));
    indicators.setVSC(car_brake_assist(state));
    indicators.setTCS(car_traction_assist(state));
    indicators.setOIL(car_oil_needs_change(state));
    indicators.update();
    abMonitor.setAccel(state->st.throttle);
    abMonitor.setBrake(state->st.brake);
    steering.setAngle((f32)state->st.steering / 100.0f);

    for (int i = 0; i < 4; i++) {
        level_to_rgb(state->st.wheel[i].wearLevel, &rgb[0], &rgb[1], &rgb[2]);
        tireWear.setColor(i, rgb[0], rgb[1], rgb[2]);
    }

    strcpy(carText, entry->info.getName1());
    strcat(carText, " ");
    strcat(carText, entry->info.getName2());
    carName.setText(carText, 0.0f, 0.0f);
    carName.show(0.0f, 0.0f);               /* timer = -0.0 (neg.s) */
    carName.update();

    pollViewModeEvent();
    carIcon0.update();
    carIcon1.update();
    leaderCar = findCarByRank(1);
}

/* RaceDisplayObjectBase::drawIfEnabled (0x00196890 / 0x00196840, T156):
 * the original expands them (isEnabled, then the draw slot). */
static inline void draw_if_enabled(RaceDisplayObjectBase *obj)
{
    if (obj->isEnabled())
        obj->draw();
}
static inline void draw_if_enabled(RaceDisplayObjectBase *obj, GSBuffer &buf)
{
    if (obj->isEnabled())
        obj->drawMeter(buf);
}
/* An inline scale setter of the mini map (retail keeps the map's address
 * across race_display_apply_layout and stores y first).  A stand-in: an
 * exact form, not the original (its name and class are unknown; it stores
 * what the member would). */
static inline void map_set_scale(RaceMiniMap *map, f32 sx, f32 sy)
{
    map->scaleY = sy;
    map->scaleX = sx;
}

/* RaceDisplayObjectBase::setEnabled (0x001967A0, T156's out-of-line copy):
 * inline in the original.  disableAll calls it on every object (retail
 * keeps the objects' addresses in registers, as an inline member's `this').
 * T156 declares it out of line, so this stand-in: an exact form, not the
 * original (the same store). */
static inline void set_enabled(RaceDisplayObjectBase *obj, bool on)
{
    obj->enabled = on;
}

/* The pending events of ev's type (RaceOrganization::getEventCount, inline
 * in the original; T125 declares it out of line).  The type is a virtual
 * call in every poller: retail reads getEventType through the vptr, so the
 * original called it on a base pointer or reference, not on the object
 * (egcs calls an object's virtual directly).  An exact form; the original
 * spelling is unknown. */
static inline int event_count(RaceOrganization *org, const RaceDisplayEventBase *ev)
{
    return org->eventQueue.counts[ev->getEventType()];
}

// FUNC 0x00193040 RaceDisplay::updateLapTimes
//   Caller: virtual_2, with the car shown.  The current lap (lapTime: the
//   car's lap time so far), the total (totalTime: the laps done, plus the
//   current lap while the race runs) and the best lap (bestTime).  At the
//   finish the current lap display goes off when the lap times or the last
//   lap are shown.
//   Not exact (94%): retail's frame is 0x10 bigger (an unused slot at
//   sp+0, the saved registers above it); the code is the same.  Tried:
//   TimeRecord copies and by-value accessors, reference / pointer
//   out-parameters for the lap time, inline wrappers of car_lap_time,
//   `lap' before the call (then three saved registers); 991111-01 alike.
void RaceDisplay::updateLapTimes(RaceEntryCar *car)
{
    int current = car_lap_time(car->state);
    RaceTimeDisplay *lap = &lapTime;
    lap->time = current;
    u32 done = car->lapTime.total;
    totalTime.time = finished ? done : done + current;
    if (lap->enabled && (lapTimes.enabled || lastTime.enabled))
        lap->enabled = !finished;
    bestTime.time = car->timeRecord.time;
}

// FUNC 0x001930D0 RaceDisplay::findCarByRank
//   The index of the car whose rank (RaceCarState st.rank, signed byte) is
//   `rank', -1 if none.  Callers: virtual_2, pollTimeDiffEvents,
//   showLeaderIcon.
int RaceDisplay::findCarByRank(int rank)
{
    int count = org->entries->count;
    for (int i = 0; i < count; i++) {
        if (org->entries->cars[i]->state->st.rank == rank)
            return i;
    }
    return -1;
}

// FUNC 0x00193118 RaceDisplay::pollLapTimeEvents
//   For each pending lap time event (0xB), the general time event (0xE) of
//   its car is its lap time: the player's car adds it to the lap times and
//   the last lap, the car `car' (the one shown) shows it (virtual_20).
//   Quirk: the lap time event object is reused, so after the first one
//   peek() only accepts the events of the car last read (the event of
//   another car restores the car and is skipped).  The time event is a
//   RaceDisplayDiffEvent() with T154's vtable.
void RaceDisplay::pollLapTimeEvents(RaceOrganization *org, int car)
{
    RaceDisplayLapTimeEvent lapEvent;       /* sp+0x00 */

    for (int i = 0; i < event_count(org, &lapEvent); i++) {
        if (!lapEvent.peek(org, i))
            continue;
        int eventCar = lapEvent.car;
        RaceDisplayGeneralTimeEvent timeEvent;  /* sp+0x10 */ /* implicit: RaceDisplayDiffEvent (base constructor) */
        timeEvent.car = eventCar;
        if (!timeEvent.peek(org, 0))
            continue;
        int time = timeEvent.getTime();
        if (eventCar == playerCar) {
            lapTimes.addLapTime(time);
            lastTime.setTime(time);
        }
        if (eventCar == car)
            showLapTime(time);
    }
}

// FUNC 0x00193260 RaceDisplay::pollCheckPointEvents
//   For each check point event (0xC) of the car `car': its general time
//   event (0xE) as "T<check point + 1> <time>" on the event board.  The
//   same quirk as pollLapTimeEvents (one check point event object for the
//   whole walk); the time event is a RaceDisplayDiffEvent() with T154's
//   vtable.
//   Not exact (72%): retail's loop has no interblock scheduling (the i++
//   stays at the end, the peek arguments in the loop body), ours moves the
//   i++ into the first peek's delay slot and the arguments into the loop
//   test (haifa's region of the loop).  A local object with an empty
//   destructor in scope (EH labels make the CFG "nonregular", one block per
//   region) gives retail's loop shape but takes a stack slot; nested ifs,
//   while / for forms, inline helpers (setColor, the time and the text)
//   change nothing.  The stack layout is retail's (the time event and the
//   two texts at sp+0x10 / 0x20 / 0x60, 0x40 and 0x20 bytes).
void RaceDisplay::pollCheckPointEvents(RaceOrganization *org, int car)
{
    RaceDisplayCheckPointEvent checkPointEvent;     /* sp+0x00 */
    RaceEventDisplay *board = &events;

    for (int i = 0; i < event_count(org, &checkPointEvent); i++) {
        if (!checkPointEvent.peek(org, i) || checkPointEvent.car != car)
            continue;
        int checkPoint = checkPointEvent.checkPoint;
        RaceDisplayGeneralTimeEvent timeEvent;  /* sp+0x10 */ /* implicit: RaceDisplayDiffEvent (base constructor) */
        char text[0x40];                    /* sp+0x20 */
        char timeText[0x20];                /* sp+0x60 */
        timeEvent.car = car;
        if (!timeEvent.peek(org, 0))
            continue;
        race_format_time(timeText, timeEvent.getTime());
        sprintf(text, "T%d %s", checkPoint + 1, timeText);
        board->color = RD_COLOR_EVENT;      /* setColor, inline */
        board->setFont("sw_16");
        board->addEvent(text, 3.0f, 0.0f);
    }
}

// FUNC 0x001933D0 RaceDisplay::pollViewModeEvent
//   Takes the first replay view mode event (RACE_EVENT_VIEW_MODE): when
//   its view (bits 0..7) is this display's screen, onViewModeChanged(the mode
//   (bits 8..15), the style (bits 16..23)); otherwise it is posted again
//   (no timer) for the other display.
void RaceDisplay::pollViewModeEvent()
{
    RaceEventQueue *queue = &org->eventQueue;
    int arg;                                /* sp+0x00 */
    if (queue->find(RACE_EVENT_VIEW_MODE, &arg, 1) != 0) {
        if ((arg & 0xFF) == displayNumber) {
            onViewModeChanged((arg >> 8) & 0xFF, (arg >> 16) & 0xFF);
            return;
        }
        queue->post(RACE_EVENT_VIEW_MODE, arg, 0);
    }
}

// FUNC 0x00193468 RaceDisplay::F_00193468
//   Empty, no callers (arguments unknown).
void RaceDisplay::F_00193468()
{
}

// FUNC 0x00193470 RaceDisplay::F_00193470
//   Empty, no callers (arguments unknown).
void RaceDisplay::F_00193470()
{
}

// FUNC 0x00193478 RaceDisplay::onViewModeChanged
//   Empty: the handler of pollViewModeEvent (the new replay view mode and its
//   style; the replay mode banner is set elsewhere).
void RaceDisplay::onViewModeChanged(int mode, int style)
{
}

// FUNC 0x00193480 RaceDisplay::F_00193480
//   No callers.  1: "SOFT", 2: "HARD", else "".  `this' is not used.
//   /* ? */ the meaning of kind: no caller or table tells (a tyre
//   compound?).
const char *RaceDisplay::F_00193480(int kind)
{
    switch (kind) {
    case 1:
        return "SOFT";
    case 2:
        return "HARD";
    default:
        return "";
    }
}

// FUNC 0x001934B8 RaceDisplay::virtual_3
//   The draw into buf (nothing while the player's car is not a car of the
//   race): buf as the draw buffer (setDrawBuffer), TEX0.TCC 1, lighting,
//   culling and colour material off, the 2D state (race_display_setup_2d),
//   then a 640 x 448 projection (224 high for RD_MODE_SPLIT_A; on the 16:9
//   screen, outside the data logger, 853 wide centred, or 427 for
//   RD_MODE_SPLIT_B).  The items: all in race mode 0xF, else RD_SHOW_LIVE
//   live and GameOptions::displayFlags in a replay.  The font at 3/4 width
//   on the 16:9 screen; the watermark in a demo or with no item; the
//   countdown, the time limit (live); then the objects of the enabled
//   panels, each when enabled (RaceDisplayObjectBase::isEnabled, then its
//   draw, or drawMeter(buf) for the meters).  Callers: the virtual_3
//   of RaceTimeTrialDisplay and RaceLicenseDisplay (and the vtables).
void RaceDisplay::virtual_3(GSBuffer &buf)
{
    if (playerCar < 0)
        return;
    if (playerCar >= org->entries->count)
        return;

    buf.setDrawBuffer();
    gfx_tex_param(PGL_TEXPARAM_TCC, 1);
    gfx_disable(PGL_CAP_CPU_LIGHTING);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_FLAG27);
    gfx_disable(PGL_CAP_COLOR_MATERIAL);
    gfx_disable(PGL_CAP_FLAG24);
    gfx_reset_texture_state_spr();
    race_display_setup_2d();
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();

    f32 left = 0.0f;
    f32 right = 640.0f;
    f32 bottom = 448.0f;
    if (g_002F424C != 0 && org->loggerMode == 0) {
        if (displayMode == RD_MODE_SPLIT_B) {
            right = 426.6667f;              /* 0x43D55556 */
        } else {
            left = -106.66667f;             /* 0xC2D55556 */
            right = 746.6666f;              /* 0x443AAAAA */
        }
    } else if (displayMode == RD_MODE_SPLIT_A) {
        bottom = 224.0f;
    }
    pgl_ortho(left, right, bottom, 0.0f, -1.0f, 1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();

    int show;
    if (raceMode == RD_RACE_MODE_SHOW_ALL) {
        show = -1;
    } else if (replay != RD_LIVE) {
        show = getGameStatus()->getGameOptions()->displayFlags;
    } else {
        show = RD_SHOW_LIVE;
    }

    g_002F1968.setScale(g_002F424C != 0 ? 0.75f : 1.0f, 1.0f);
    RaceOrganization *race = org;
    if (replay == RD_DEMO || show == 0)
        fader.draw();
    pgl_color(RD_COLOR_OPAQUE);
    drawCountdown(race->countDown);
    if (replay == RD_LIVE && race->timeLimit != 0)
        drawTimeLimit(race->timeRemain / 60);
    race_display_setup_2d();

    if (panels & RD_PANEL_TOP_LEFT) {
        if (show & RD_SHOW_MAP)
            miniMap.drawMap(race, targetCar);
        if (innerPanel())
            draw_if_enabled(&rankDisplay); /* implicit: isEnabled (inline draw_if_enabled) */
    }

    if (show & RD_SHOW_TIMES) {
        if (panels & RD_PANEL_TOP_RIGHT) {
            draw_if_enabled(&lapTime);
            draw_if_enabled(&lapTimes);
            draw_if_enabled(&totalTime);
            draw_if_enabled(&lastTime);
            draw_if_enabled(&bestTime);
            draw_if_enabled(&recordSpeed);
            draw_if_enabled(&topSpeedDisplay);
            for (int i = 0; i < 3; i++)
                draw_if_enabled(&recordTimes[i]);
            draw_if_enabled(&recordTime);
            if (noLapDisplay == 0)
                draw_if_enabled(&lapDisplay);
            draw_if_enabled(&carIcon0);
        }
        if (panels & RD_PANEL_CENTER) {
            draw_if_enabled(&events);
            draw_if_enabled(&carIcon1);
        }
    }

    if (panels & RD_PANEL_CENTER) {
        if (show & RD_SHOW_METERS) {
            draw_if_enabled(&digitalSpeed);
            draw_if_enabled(&backPanel);
            if (activeTacho != NULL)
                draw_if_enabled(activeTacho, buf);
            draw_if_enabled(&speedometer, buf);
            draw_if_enabled(&boostMeter, buf);
            draw_if_enabled(&indicators);
            draw_if_enabled(&odometer);
            draw_if_enabled(&tripMeter);
            draw_if_enabled(&gearDisplay);
            if (activeTacho != NULL)
                activeTacho->render_2nd();
            speedometer.RaceRoundMeterBase::render_2nd();  /* direct calls */
            boostMeter.RaceBoostmeter::render_2nd();
        }
        draw_if_enabled(&title);
        draw_if_enabled(&modeMessage);
        if (innerPanel())
            draw_if_enabled(&warningMessage);
    }

    if (panels & RD_PANEL_BOTTOM_LEFT) {
        if (show & RD_SHOW_TIRE_WEAR)
            draw_if_enabled(&tireWear);
        if (show & RD_SHOW_AB_MONITOR)
            draw_if_enabled(&abMonitor);
        if (show & RD_SHOW_STEERING)
            draw_if_enabled(&steering);
    }

    if (panels & RD_PANEL_BOTTOM_RIGHT) {
        if (show & RD_SHOW_CAR_NAME)
            draw_if_enabled(&carName);
        draw_if_enabled(&events2);
    }

    g_002F1968.setScale(1.0f, 1.0f);
    g_002F1968.flush();
}

// FUNC 0x00193DA8 RaceDisplay::virtual_16
//   A car finished (RaceDisplay::virtual_2 for each goal event): once, when
//   it is this display's car `myCar', finished = 1 and the events
//   RACE_EVENT_BGM (BGM_CODE_FINISH) and RACE_EVENT_STOP_TIMER are
//   posted.  In the split screen battles the title (font "sw_20*150,150",
//   no fade in, 1 s fade out, amber) shows "PLAYER <car + 1> WIN!" for 4 s
//   when that car is first, and only then are the events posted; otherwise
//   put_finish_display() when finishDisplay.
void RaceDisplay::virtual_16(RaceOrganization *org, int car, int myCar)
{
    switch (displayMode) {
    case RD_MODE_SPLIT_A:
    case RD_MODE_SPLIT_B: {
        if (car != myCar || finished != 0)
            return;
        RaceMessageDisplay *message = &title;
        finished = 1;
        int rank = org->entries->cars[car]->state->st.rank;
        message->setFont("sw_20*150,150");
        message->fadeIn = 0.0f;             /* setFadeTime(0, 1), setColor: inline */
        message->color = RD_COLOR_WIN;
        message->fadeOut = 1.0f;
        if (rank == 1) {
            char text[0x20];                /* sp+0x00 */
            org->eventQueue.post(RACE_EVENT_BGM, BGM_CODE_FINISH, 0);
            org->eventQueue.post(RACE_EVENT_STOP_TIMER, 0, 0);
            sprintf(text, g_002C43E8[RACEDISPLAY_PLAYER_WIN], car + 1);
            message->setText(text, 4.0f, 0.0f);
        }
        break;
    }
    default:
        if (car != myCar || finished != 0)
            return;
        finished = 1;
        org->eventQueue.post(RACE_EVENT_BGM, BGM_CODE_FINISH, 0);
        org->eventQueue.post(RACE_EVENT_STOP_TIMER, 0, 0);
        if (finishDisplay != 0)
            put_finish_display();
        break;
    }
}

// FUNC 0x00193F58 RaceDisplay::pollLapDiffEvents
//   The lap difference events (0x10) of the car `car' (bound before each
//   peek), on the event board.  The event is a RaceDisplayDiffEvent() with
//   T154's vtable.
void RaceDisplay::pollLapDiffEvents(RaceOrganization *org, int car)
{
    RaceDisplayLapDiffEvent diffEvent;      /* sp+0x00 */ /* implicit: RaceDisplayDiffEvent (base constructor) */
    for (int i = 0; i < event_count(org, &diffEvent); i++) {
        diffEvent.car = car;
        if (diffEvent.peek(org, i))
            showLapDiff(diffEvent.diff);
    }
}

// FUNC 0x00194010 RaceDisplay::pollTimeDiffEvents
//   The time difference events (0xF) of the car `car', with the icon of the
//   pole car (qualify: leaderCar) or of the car in front (the 1st when the
//   difference is >= 0, else the 2nd).  The event is a
//   RaceDisplayDiffEvent() with T154's vtable.
void RaceDisplay::pollTimeDiffEvents(RaceOrganization *org, int car)
{
    RaceDisplayTimeDiffEvent diffEvent;     /* sp+0x00 */ /* implicit: RaceDisplayDiffEvent (base constructor) */
    for (int i = 0; i < event_count(org, &diffEvent); i++) {
        diffEvent.car = car;
        if (diffEvent.peek(org, i)) {
            int diff = diffEvent.diff;
            int iconCar;
            if (qualify != 0)
                iconCar = leaderCar;
            else
                iconCar = findCarByRank(diff >= 0 ? 1 : 2);
            showTimeDiff(diff, iconCar);
        }
    }
}

// FUNC 0x00194110 RaceDisplay::pollSpeedEvents
//   The speed events (0x11) of the car `car', on the event board.
void RaceDisplay::pollSpeedEvents(RaceOrganization *org, int car)
{
    RaceDisplaySpeedEvent speedEvent;       /* sp+0x00 */
    for (int i = 0; i < event_count(org, &speedEvent); i++) {
        speedEvent.car = car;
        if (speedEvent.peek(org, i))
            showSpeed(speedEvent.speed);
    }
}

// FUNC 0x001941C0 RaceDisplay::pollInformationEvents
//   The information events (0x12) of any car as messages on the event
//   board (RD_INFO_*): 1 / 2 with the car's maker name (2 with its icon),
//   3 / 4 for the player's car, 5 for the player's car, 6 the start display
//   and SE_COUNT_S for the player's car; an empty message when the car does
//   not match.  The walk stops at the first missing event or an unknown
//   information.  Quirks: the maker name is looked up for every event; the
//   `car < 0' tests never hold (unpack gives the car 0..15).
void RaceDisplay::pollInformationEvents(RaceOrganization *org)
{
    RaceDisplayInformationEvent infoEvent;  /* sp+0x00 */
    char text[0x80];                        /* sp+0x10 */

    for (int i = 0; i < event_count(org, &infoEvent); ) {
        infoEvent.car = -1;
        if (!infoEvent.peek(org, i))
            return;
        int car = infoEvent.car;
        const char *maker = org->entries->cars[car]->info.getName1();
        text[0] = 0;
        switch (infoEvent.information) {
        case RD_INFO_PIT_IN:
            sprintf(text, g_002C43E8[RACEDISPLAY_PIT_IN], maker);
            break;
        case RD_INFO_POLE_POSITION:
            showCarIcon(car);
            sprintf(text, g_002C43E8[RACEDISPLAY_POLE_POSITION], maker);
            break;
        case RD_INFO_START_QUALIFY:
            if (car < 0 || car == playerCar)
                strcpy(text, g_002C43E8[RACEDISPLAY_START_QUALIFY]);
            break;
        case RD_INFO_READY_QUALIFY:
            if (car < 0 || car == playerCar)
                strcpy(text, g_002C43E8[RACEDISPLAY_READY_QUALIFY]);
            break;
        case RD_INFO_ENDURANCE_FINAL:
            if (car == playerCar)
                strcpy(text, g_002C43E8[RACEDISPLAY_ENDURANCE_FINAL]);
            break;
        case RD_INFO_START:
            if (car < 0 || car == playerCar) {
                put_start_display();
                race_se_play(SE_COUNT_S);
            }
            break;
        default:
            return;
        }
        RaceEventDisplay *board = &events;
        board->setFont("sw_16");
        i++;
        board->color = RD_COLOR_EVENT;
        board->addEvent(text, 3.0f, 0.0f);
    }
}

// FUNC 0x001943E8 RaceDisplay::showLeaderIcon
//   Caller: virtual_0.  carIcon0 shows the icon of the car in first place.
void RaceDisplay::showLeaderIcon()
{
    showCarIcon(findCarByRank(1));
}

// FUNC 0x00194420 RaceDisplay::showCarIcon
//   When the icons are set up and car is a car of the race: carIcon0 shows
//   its icon, blinking 8 times, for good (the inline setIcon, setBlink(8),
//   show(0, 0): timer = -0.0), and recordTime's label is cleared (the icon
//   takes its place).  Callers: pollInformationEvents (the pole car),
//   showLeaderIcon.
void RaceDisplay::showCarIcon(int car)
{
    CarIcon *carIcons = icons;
    if (carIcons == NULL || car < 0 || car >= org->entries->count)
        return;
    CarIcon *icon = &carIcons[car];
    RaceCarIconDisplay *display = &carIcon0;
    display->setIcon(icon);
    display->show(0.0f, 0.0f);              /* timer = -0.0 (neg.s) */
    display->setBlink(8);
    recordTime.setLabel("");
}

// FUNC 0x001944A0 RaceDisplay::showLapTime
//   The lap time of the car shown on the event board in the big font
//   ("sw_20").  Callers: pollLapTimeEvents (through the vtable),
//   RaceLicenseDisplay::showLapTime (its override: only in replays).
void RaceDisplay::showLapTime(int time)
{
    char text[0x20];                        /* sp+0x00 */
    race_format_time(text, time);
    RaceEventDisplay *board = &events;
    board->color = RD_COLOR_EVENT;
    board->setFont("sw_20");
    board->addEvent(text, 3.0f, 0.0f);
}

// FUNC 0x00194500 RaceDisplay::showLapDiff
//   "+<n> Laps" (red, >= 0) or "<minus><n> Laps" (blue) on the event board,
//   3 s from 3 s before the last event's start, offset (0, 25).  Caller:
//   pollLapDiffEvents.
void RaceDisplay::showLapDiff(int laps)
{
    char text[0x20];                        /* sp+0x00 */
    RaceEventDisplay *board = &events;
    board->color = laps >= 0 ? RD_COLOR_BEHIND : RD_COLOR_AHEAD;
    text[0] = laps >= 0 ? '+' : RD_CHAR_MINUS;
    sprintf(text + 1, g_002C43E8[RACEDISPLAY_LAP_DIFF_INFO], laps > -1 ? laps : -laps);
    board->setFont("sw_16");
    board->offX = 0;                        /* setOffset(0, 25), inline */
    board->offY = RD_DIFF_OFFSET_Y;
    board->addEvent(text, 3.0f, -3.0f);
}

// FUNC 0x001945B0 RaceDisplay::showTimeDiff
//   "+<time>" (red, >= 0) or "<minus><time>" (blue) on the event board, 3 s
//   from 3 s before the last event's start.  With an icon car (and icons):
//   carIcon1 shows its icon 62 pixels left (>= 0) or right of the board, 20
//   lower, for 3 s from the last event's start, blinking 3 times, with the
//   board's enable, and the text moves 37 the other way.  Caller:
//   pollTimeDiffEvents.
void RaceDisplay::showTimeDiff(int diff, int iconCar)
{
    char text[0x20];                        /* sp+0x00 */
    RaceEventDisplay *board = &events;
    int textX = 0;
    int iconX = board->x;
    if (iconCar >= 0 && icons != NULL) {
        textX = RD_DIFF_OFFSET_X;
        if (diff >= 0) {
            iconX -= RD_DIFF_ICON_DX;
        } else {
            iconX += RD_DIFF_ICON_DX;
            textX = -RD_DIFF_OFFSET_X;
        }
        RaceCarIconDisplay *display = &carIcon1;
        display->setIcon(&icons[iconCar]);
        /* T156's inline setLocation on the member object (its type is
         * fixed, so egcs integrates the virtual: x, y as the inline call's
         * arguments, board->y loaded after the icon store; through
         * `display' it would be a vtable call) */
        carIcon1.setLocation(iconX, board->y + RD_DIFF_ICON_DY);
        display->show(3.0f, board->getLastEventBegin());
        display->setBlink(3);
        display->enabled = board->enabled;
    }
    board->offX = textX;
    board->offY = RD_DIFF_OFFSET_Y;
    board->setFont("sw_16");
    board->color = diff >= 0 ? RD_COLOR_BEHIND : RD_COLOR_AHEAD;
    text[0] = diff >= 0 ? '+' : RD_CHAR_MINUS;
    race_format_time(text + 1, diff >= 0 ? diff : -diff);
    board->addEvent(text, 3.0f, -3.0f);
}

// FUNC 0x001946E0 RaceDisplay::showSpeed
//   "<speed>.<1 decimal><unit>" on the event board (the float widened to
//   double by fptodp for the varargs).  Caller: pollSpeedEvents.
void RaceDisplay::showSpeed(f32 speed)
{
    char text[0x20];                        /* sp+0x00 */
    const char *unit = g_002F59D8.speedName();
    RaceEventDisplay *board = &events;
    sprintf(text, "%.1f%s", (double)speed, unit); /* implicit: fptodp ((double) conversion) */
    board->color = RD_COLOR_EVENT;
    board->setFont("sw_16");
    board->addEvent(text, 3.0f, 0.0f);
}

// FUNC 0x00194788 RaceDisplay::showReplayRecorded
//   Caller: virtual_2 (live, on a fresh event 0x16: the replay record is
//   full): "replay recording finished" on the event board.
void RaceDisplay::showReplayRecorded()
{
    RaceEventDisplay *board = &events;
    board->color = RD_COLOR_EVENT;
    board->setFont("sw_16");
    board->addEvent(g_002C43E8[RACEDISPLAY_RECORD_END], 3.0f, 0.0f);
}

// FUNC 0x001947E0 RaceDisplay::virtual_17
//   Empty.
void RaceDisplay::virtual_17()
{
}

// FUNC 0x001947E8 RaceDisplay::put_start_display
//   (index: virtual_18.)  Live only: the image "start" ("@start") on the
//   title for 1 second.  Callers: pollInformationEvents,
//   RaceFreeRunDisplay::virtual_18.
void RaceDisplay::put_start_display()
{
    if (replay == RD_LIVE) {
        RaceMessageDisplay *message = &title;
        message->color = RD_COLOR_TEXT;
        message->setText("@start", 1.0f, 0.0f);
    }
}

// FUNC 0x00194838 RaceDisplay::put_finish_display
//   (index: virtual_19.)  Live only: "@finish" on the title for 4 seconds.
void RaceDisplay::put_finish_display()
{
    if (replay == RD_LIVE) {
        RaceMessageDisplay *message = &title;
        message->color = RD_COLOR_TEXT;
        message->setText("@finish", 4.0f, 0.0f);
    }
}

// FUNC 0x00194888 RaceDisplay::setLayout
//   Callers: virtual_0, virtual_2 (with layoutVariant).  1 at once when the
//   variant is the one loaded; 0 when the display mode has no layout
//   (RaceModeDatabase::selectModeById).  Else every object is disabled and
//   each detail record of the mode of this variant (or of all variants,
//   RD_VARIANT_ALL) places and enables its object (RD_ITEM_*; the record
//   is read again after each call).  The cases are in the order of
//   retail's code (case 0 has its own jump table entry); the three
//   tachometer cases share their tail (cross-jumped).
int RaceDisplay::setLayout(int variant)
{
    if (currentVariant == variant)
        return 1;
    currentVariant = variant;
    if (g_002F00C0.selectModeById(displayMode) == 0)
        return 0;
    disableAll();

    for (int i = 0; i < g_002F00C0.getNumDetails(); i++) {
        g_002F00C0.selectDetail(i);
        int mode = g_002F00C0.detail->mode;
        if (mode != RD_VARIANT_ALL && mode != variant)
            continue;
        switch (g_002F00C0.detail->type) {
        case RD_ITEM_RANK:
            race_display_apply_layout(&rankDisplay);
            rankDisplay.setScale((f32)g_002F00C0.detail->width * 0.01f);
            break;
        case RD_ITEM_LAP:
            race_display_apply_layout(&lapDisplay);
            break;
        case RD_ITEM_WARNING:
            race_display_apply_layout(&warningMessage);
            break;
        case RD_ITEM_EVENTS:
            race_display_apply_layout(&events);
            break;
        case RD_ITEM_TIME:
            layoutTimeDisplay(variant);
            break;
        case RD_ITEM_RECORD_TIME:
            race_display_apply_layout(&recordTimes[g_002F00C0.detail->subType]);
            break;
        case RD_ITEM_MAP: {
            race_display_apply_layout(&miniMap);
            RaceDetailRecord *record = g_002F00C0.detail;
            map_set_scale(&miniMap, (f32)record->width * 0.01f, (f32)record->height * 0.01f);
            break;
        }
        case RD_ITEM_MIRROR:
            race_display_apply_layout(&mirror);
            break;
        case RD_ITEM_ONBOARD_TACHO:
            race_display_apply_layout(&onboardTacho);
            activeTacho = &onboardTacho;
            break;
        case RD_ITEM_REMOTE_TACHO:
            race_display_apply_layout(&remoteTacho);
            activeTacho = &remoteTacho;
            break;
        case RD_ITEM_BATTLE_TACHO:
            race_display_apply_layout(&battleTacho);
            activeTacho = &battleTacho;
            break;
        case RD_ITEM_SPEEDOMETER:
            race_display_apply_layout(&speedometer);
            break;
        case RD_ITEM_BOOST:
            race_display_apply_layout(&boostMeter);
            break;
        case RD_ITEM_GEAR:
            race_display_apply_layout(&gearDisplay);
            if ((g_002F00C0.detail->subType & 1) == 0)
                gearDisplay.setLampOffset(-0x23, 0);
            else
                gearDisplay.setLampOffset(-4, -0xC);
            break;
        case RD_ITEM_DIGITAL_SPEED: {
            race_display_apply_layout(&digitalSpeed);
            RaceDetailRecord *record = g_002F00C0.detail;
            digitalSpeed.setUnitOffset(record->width, record->height);
            break;
        }
        case RD_ITEM_ODOMETER:
            race_display_apply_layout(&odometer);
            break;
        case RD_ITEM_TRIP_METER:
            race_display_apply_layout(&tripMeter);
            break;
        case RD_ITEM_INDICATORS:
            race_display_apply_layout(&indicators);
            break;
        case RD_ITEM_METERS: {
            int subType = g_002F00C0.detail->subType;
            if (subType < RD_PANEL_IMAGES) {
                layoutMeters(subType);
            } else {
                race_display_apply_layout(&backPanel);
                backPanel.setPanel(g_002F00C0.detail->subType);
            }
            break;
        }
        case RD_ITEM_CAR_NAME:
            race_display_apply_layout(&carName);
            break;
        case RD_ITEM_REPLAY_MODE:
            race_display_apply_layout(&replayMode);
            break;
        case RD_ITEM_EVENTS2:
            race_display_apply_layout(&events2);
            break;
        case RD_ITEM_TITLE:
            if (g_002F00C0.detail->subType == 0)
                race_display_apply_layout(&title);
            else
                race_display_apply_layout(&modeMessage);
            break;
        case RD_ITEM_AB_MONITOR:
            race_display_apply_layout(&abMonitor);
            break;
        case RD_ITEM_STEERING:
            race_display_apply_layout(&steering);
            break;
        case RD_ITEM_TIRE_WEAR:
            if (hasTireWear != 0)
                race_display_apply_layout(&tireWear);
            break;
        case RD_ITEM_NONE:
            break;
        }
    }
    return 1;
}

// FUNC 0x00194BF8 RaceDisplay::disableAll
//   Caller: setLayout.  Disables every display object (setEnabled(false),
//   inline) and clears the warning and car name messages (the inline
//   clear(): setText("", -1, 0)) and the second event board.
void RaceDisplay::disableAll()
{
    set_enabled(&miniMap, false);
    set_enabled(&lapDisplay, false);
    set_enabled(&rankDisplay, false);
    set_enabled(&lapTimes, false);
    set_enabled(&totalTime, false);
    set_enabled(&bestTime, false);
    set_enabled(&recordTime, false);
    set_enabled(&lastTime, false);
    set_enabled(&lapTime, false);
    for (int i = 0; i < 3; i++)
        set_enabled(&recordTimes[i], false);
    set_enabled(&recordSpeed, false);
    set_enabled(&topSpeedDisplay, false);
    set_enabled(&events, false);
    set_enabled(&warningMessage, false);
    warningMessage.clear();                 /* implicit: setText (the inline clear()) */
    set_enabled(&carIcon0, false);
    set_enabled(&carIcon1, false);
    set_enabled(&onboardTacho, false);
    set_enabled(&remoteTacho, false);
    set_enabled(&battleTacho, false);
    set_enabled(&speedometer, false);
    set_enabled(&boostMeter, false);
    set_enabled(&odometer, false);
    set_enabled(&tripMeter, false);
    set_enabled(&indicators, false);
    set_enabled(&digitalSpeed, false);
    set_enabled(&gearDisplay, false);
    set_enabled(&backPanel, false);
    set_enabled(&mirror, false);
    set_enabled(&tireWear, false);
    set_enabled(&abMonitor, false);
    set_enabled(&steering, false);
    set_enabled(&carName, false);
    carName.clear();
    set_enabled(&events2, false);
    events2.reset();
    set_enabled(&replayMode, false);
    set_enabled(&title, false);
    set_enabled(&modeMessage, false);
}

// FUNC 0x00194DF0 RaceDisplay::layoutTimeDisplay
//   setLayout's RD_ITEM_TIME: the current record's subType picks a time /
//   speed display and its label (the variant is not used); other values do
//   nothing.
void RaceDisplay::layoutTimeDisplay(int variant)
{
    switch (g_002F00C0.detail->subType) {
    case 1:                                 /* the lap times */
        race_display_apply_layout(&lapTimes);
        break;
    case 2:
        race_display_apply_layout(&totalTime);
        totalTime.setLabel(g_002C43E8[RACEDISPLAY_TOTAL_TIME_LABEL]);
        break;
    case 3:
    case 6:
        race_display_apply_layout(&bestTime);
        bestTime.setLabel(g_002C43E8[RACEDISPLAY_BEST_TIME_LABEL]);
        break;
    case 4:
        race_display_apply_layout(&lastTime);
        lastTime.setLabel(g_002C43E8[RACEDISPLAY_LAST_TIME_LABEL]);
        break;
    case 5:
        race_display_apply_layout(&lapTime);
        lapTime.setLabel(g_002C43E8[RACEDISPLAY_LAP_TIME_LABEL]);
        break;
    case 7:
        race_display_apply_layout(&recordTime);
        recordTime.setLabel(g_002C43E8[RACEDISPLAY_RECORD_TIME_LABEL]);
        break;
    case 8:
        race_display_apply_layout(&recordTime);
        recordTime.setLabel(g_002C43E8[RACEDISPLAY_RECORD_LAP_LABEL]);
        break;
    case 9:                                 /* the record with the record holder's icon */
        race_display_apply_layout(&recordTime);
        race_display_apply_layout(&carIcon0);
        break;
    case 10:
        race_display_apply_layout(&lapTime);
        lapTime.setLabel(g_002C43E8[RACEDISPLAY_TIME_LABEL]);
        break;
    case 11:
        race_display_apply_layout(&recordSpeed);
        recordSpeed.setLabel(g_002C43E8[RACEDISPLAY_RECORD_SPEED_LABEL]);
        break;
    case 12:
        race_display_apply_layout(&topSpeedDisplay);
        topSpeedDisplay.setLabel(g_002C43E8[RACEDISPLAY_SESSION_SPEED_LABEL]);
        break;
    case 13:
        race_display_apply_layout(&recordTime);
        recordTime.setLabel(g_002C43E8[RACEDISPLAY_FASTEST_TIME_LABEL]);
        break;
    case 14:
        race_display_apply_layout(&totalTime);
        totalTime.setLabel(g_002C43E8[RACEDISPLAY_TRIAL_TIME_LABEL]);
        break;
    default:
        break;
    }
}

// FUNC 0x00194FF8 RaceDisplay::layoutMeters
//   setLayout's RD_ITEM_METERS below 0x40: the meter set RD_METERS_ONBOARD
//   (the onboard tachometer, layout g_002BC080, back panel RD_BACK_SHAPE)
//   or RD_METERS_REMOTE (the remote tachometer, g_002BC120, RD_BACK_FRAME),
//   with the digital speedometer's unit and the gear's lamp offsets (the
//   inline setUnitOffset / setLampOffset).  Every object of the table gets
//   its region and alignment (virtual calls) and is enabled: this->*member,
//   read again for each use.
void RaceDisplay::layoutMeters(int kind)
{
    RaceMeterLayout *entry;
    switch (kind) {
    case RD_METERS_ONBOARD:
        digitalSpeed.setUnitOffset(-0x2C, 0x14);
        entry = g_002BC080;
        activeTacho = &onboardTacho;
        gearDisplay.setLampOffset(-0x23, 0);
        backPanel.setPanel(RD_BACK_SHAPE);
        break;
    case RD_METERS_REMOTE:
        digitalSpeed.setUnitOffset(5, 0);
        entry = g_002BC120;
        activeTacho = &remoteTacho;
        gearDisplay.setLampOffset(-4, -0xC);
        backPanel.setPanel(RD_BACK_FRAME);
        break;
    default:
        return;
    }
    for (; entry->member != 0; entry++) {
        (this->*entry->member).setRegion(entry->x, entry->y, entry->w, entry->h);
        (this->*entry->member).setAlignment(entry->alignX, entry->alignY);
        (this->*entry->member).enabled = 1;
    }
}

// FUNC 0x00195168 race_display_apply_layout
//   Places obj by the current detail record (location, size, alignment:
//   virtual calls; the record is read again after each) and enables it;
//   nothing for NULL.  Callers: setLayout, layoutTimeDisplay.
void race_display_apply_layout(RaceDisplayObjectBase *obj)
{
    if (obj == NULL)
        return;
    obj->setLocation(g_002F00C0.detail->x, g_002F00C0.detail->y);
    obj->setSize(g_002F00C0.detail->width, g_002F00C0.detail->height);
    obj->setAlignment(g_002F00C0.detail->alignX, g_002F00C0.detail->alignY);   /* the low bytes (lbu) */
    obj->enabled = 1;
}

// FUNC 0x00195208 race_display_find_image
//   The named image of the display images (u051 F_00195208).  9 callers
//   (RaceDisplay::virtual_0, the T157 draws).
Tex1Image *race_display_find_image(const char *name)
{
    return image_pack_find(g_00351FA4, name);
}

/* The two meter layouts (layoutMeters).  egcs does not take the pointers to
 * members as constants: the tables go to .bss and the static initializer
 * below builds them at run time, each entry memset to zero, then its
 * fields (the members are RaceDisplayObjectBase-derived objects of
 * RaceDisplay; egcs's pointer is the member's offset + 1). */
#define RD_METER(m)     ((RaceDisplayObjectBase RaceDisplay::*)&RaceDisplay::m)
RaceMeterLayout g_002BC080[10] = {          /* 0x002BC080 */
    { RD_METER(onboardTacho), 0x15E, 0x1B4, 0xAA, 0x6E, 0, 1 },
    { RD_METER(speedometer), 0x122, 0x1B4, 0xAA, 0x6E, 1, 1 },
    { RD_METER(digitalSpeed), 0x15E, 0x19A, -0x2C, 0x14, 1, 1 },
    { RD_METER(boostMeter), 0x212, 0x1B4, 0x48, 0x40, 0, 1 },
    { RD_METER(gearDisplay), 0x140, 0x168, 0x49, 0x1F, 2, 2 },
    { RD_METER(odometer), 0xCD, 0x1AC, 0, 0, 2, 2 },
    { RD_METER(tripMeter), 0xCD, 0x172, 0, 0, 2, 2 },
    { RD_METER(indicators), 0x1B3, 0x1AE, 0, 0, 2, 2 },
    { RD_METER(backPanel), 0x140, 0x168, 0x4E, 0x1F, 2, 2 },
    { 0 }
};
RaceMeterLayout g_002BC120[9] = {           /* 0x002BC120 */
    { RD_METER(remoteTacho), 0x26C, 0x140, 0xA0, 0x8C, 5, 2 },
    { RD_METER(digitalSpeed), 0x244, 0x181, 5, 0, 5, 2 },
    { RD_METER(boostMeter), 0x1DB, 0x1B4, 0x48, 0x40, 5, 1 },
    { RD_METER(gearDisplay), 0x26C, 0x145, 0x96, 0x8C, 5, 2 },
    { RD_METER(odometer), 0x25D, 0x1A8, 0, 0, 5, 2 },
    { RD_METER(tripMeter), 0x20D, 0x1A8, 0, 0, 5, 2 },
    { RD_METER(indicators), 0x26C, 0x19F, 0x7D, 0xA, 5, 2 },
    { RD_METER(backPanel), 0x26C, 0x1AE, 0x88, 0x1E, 5, 1 },
    { 0 }
};

/* Compiler-generated: egcs's static initializer of T154 (.ctors entry
 * 0x0028F130), named after the file's first global function (the
 * constructor, __11RaceDisplay) and emitted at the end of the unit's own
 * functions, before the inline copies. */
// FUNC 0x00195230 T154_static_init (_GLOBAL_$I$__11RaceDisplay)
/* implicit: memset (the run-time initializers of g_002BC080 / g_002BC120) */

/* ======================================================================== */
/* The T154 display events (T194_race_display_event.h)                      */
/* ======================================================================== */

/* Inline only: egcs emits their vtables, getEventType copies and typeinfo
 * functions here (the pollers build them), in the reverse of their
 * definition order. */

// FUNC 0x001956D8 RaceDisplayGeneralTimeEvent::getEventType
/* inline in game/race/display/T194_race_display_event.h (0xE); egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x001956E0 __tf27RaceDisplayGeneralTimeEvent
//   __si_type_info at 0x004B2190, base __ti20RaceDisplayDiffEvent (copy
//   T303 0x001ECAA8, weak there).
extern "C" const type_info &__tf27RaceDisplayGeneralTimeEvent(void)
{
    if (__ti27RaceDisplayGeneralTimeEvent[0] == 0) {
        __tf20RaceDisplayDiffEvent();
        __rtti_si(__ti27RaceDisplayGeneralTimeEvent, "27RaceDisplayGeneralTimeEvent",
                  (const type_info *)__ti20RaceDisplayDiffEvent);
    }
    return *(const type_info *)__ti27RaceDisplayGeneralTimeEvent;
}
#endif

// FUNC 0x00195730 RaceDisplayLapDiffEvent::getEventType
/* inline in game/race/display/T194_race_display_event.h (0x10); egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x00195738 __tf23RaceDisplayLapDiffEvent
//   __si_type_info at 0x004B21C0, base __ti20RaceDisplayDiffEvent (copy
//   T303 0x001ECB00, weak there).
extern "C" const type_info &__tf23RaceDisplayLapDiffEvent(void)
{
    if (__ti23RaceDisplayLapDiffEvent[0] == 0) {
        __tf20RaceDisplayDiffEvent();
        __rtti_si(__ti23RaceDisplayLapDiffEvent, "23RaceDisplayLapDiffEvent",
                  (const type_info *)__ti20RaceDisplayDiffEvent);
    }
    return *(const type_info *)__ti23RaceDisplayLapDiffEvent;
}
#endif

// FUNC 0x00195788 RaceDisplayTimeDiffEvent::getEventType
/* inline in game/race/display/T194_race_display_event.h (0xF); egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x00195790 __tf24RaceDisplayTimeDiffEvent
//   __si_type_info at 0x004B2170, base __ti20RaceDisplayDiffEvent (copies
//   T303 0x001ECB58, T315 0x001F26F8).
extern "C" const type_info &__tf24RaceDisplayTimeDiffEvent(void)
{
    if (__ti24RaceDisplayTimeDiffEvent[0] == 0) {
        __tf20RaceDisplayDiffEvent();
        __rtti_si(__ti24RaceDisplayTimeDiffEvent, "24RaceDisplayTimeDiffEvent",
                  (const type_info *)__ti20RaceDisplayDiffEvent);
    }
    return *(const type_info *)__ti24RaceDisplayTimeDiffEvent;
}
#endif

/* ======================================================================== */
/* RaceDisplay's inline members                                             */
/* ======================================================================== */

/* The accessors are inline in game/race/display/T154_race_display.h (defined in the class
 * in the reverse of this order); egcs emits their out-of-line copies here,
 * with RaceDisplay's vtable (this object has the key function virtual_0),
 * before the class's typeinfo function.  No callers but the vtables. */

// FUNC 0x001957E0 RaceDisplay::getImages (getImages__11RaceDisplay)
/* inline (static): the display images g_00351FA4 */

// FUNC 0x001957E8 RaceDisplay::setCarIcons (setCarIcons__11RaceDisplayP7CarIcon)
/* inline: icons = i */

// FUNC 0x001957F0 RaceDisplay::getMirrorObject (getMirrorObject__11RaceDisplay)
/* inline (slot 15): &mirror */

// FUNC 0x00195800 RaceDisplay::setDisplayNumber (setDisplayNumber__11RaceDisplayi)
/* inline: displayNumber = n (RaceSplitDisplay's constructor) */

// FUNC 0x00195810 RaceDisplay::setFinishDisplay (setFinishDisplay__11RaceDisplayb)
/* inline: finishDisplay = on (RaceMachineTest::onExit) */

// FUNC 0x00195820 RaceDisplay::ackReplayRecorded (ackReplayRecorded__11RaceDisplay)
/* inline (static): g_00351FB8 = 3, the event 0x16 seen (no "replay
 * recording finished" from virtual_2) */

// FUNC 0x00195830 RaceDisplay::isLive (isLive__11RaceDisplay)
/* inline: live (the mode message says "LIVE", not "REPLAY") */

// FUNC 0x00195840 RaceDisplay::setLive (setLive__11RaceDisplayb)
/* inline: live = on (RaceLinkBattle::virtual_8) */

// FUNC 0x00195850 RaceDisplay::setTargetCarNumber (setTargetCarNumber__11RaceDisplayi)
/* inline (slot 8): targetCar = n */

// FUNC 0x00195860 RaceDisplay::setPlayerCarNumber (setPlayerCarNumber__11RaceDisplayi)
/* inline (slot 7): playerCar = n */

// FUNC 0x00195870 RaceDisplay::getPanels (getPanels__11RaceDisplay)
/* inline: the panel bits, read as a signed byte (lb) */

// FUNC 0x00195880 RaceDisplay::setCenterPanel (setCenterPanel__11RaceDisplayb)
/* inline (slot 14) */

// FUNC 0x001958A8 RaceDisplay::setBottomRightPanel (setBottomRightPanel__11RaceDisplayb)
/* inline (slot 13) */

// FUNC 0x001958D0 RaceDisplay::setBottomLeftPanel (setBottomLeftPanel__11RaceDisplayb)
/* inline (slot 12) */

// FUNC 0x001958F8 RaceDisplay::setTopRightPanel (setTopRightPanel__11RaceDisplayb)
/* inline (slot 11) */

// FUNC 0x00195920 RaceDisplay::setTopLeftPanel (setTopLeftPanel__11RaceDisplayb)
/* inline (slot 10) */

// FUNC 0x00195948 RaceDisplay::innerPanel (innerPanel__C11RaceDisplay)
/* inline (slot 5, the const getter): inner */

// FUNC 0x00195958 RaceDisplay::innerPanel (innerPanel__11RaceDisplayb)
/* inline (slot 4, the setter): inner = on */

// FUNC 0x00195968 RaceDisplay::getOrganization (getOrganization__11RaceDisplay)
/* inline: org */

// FUNC 0x00195970 RaceDisplay::setOrganization (setOrganization__11RaceDisplayR16RaceOrganization)
/* inline (slot 6): org = &o */

/* From here to 0x00195B88: egcs emits these with the vtables of
 * RaceDisplay, RaceMiniMap, RaceOnboardSpeedmeter and RaceMeterBase (the
 * constructor and the destructor store them).  The typeinfo functions are
 * egcs's own (the definitions below are for GCC 14). */

#ifndef BOOT_OLD_GCC
// FUNC 0x00195978 __tf11RaceDisplay
//   __si_type_info at 0x004B21D0, base __ti15RaceDisplayBase (gp-0x4708).
//   Callers: the __tf of RaceTimeTrialDisplay, RaceFreeRunDisplay,
//   RaceLicenseDisplay.
extern "C" const type_info &__tf11RaceDisplay(void)
{
    if (__ti11RaceDisplay[0] == 0) {
        __tf15RaceDisplayBase();
        __rtti_si(__ti11RaceDisplay, "11RaceDisplay", (const type_info *)__ti15RaceDisplayBase);
    }
    return *(const type_info *)__ti11RaceDisplay;
}
#endif

/* ======================================================================== */
/* Inline members of the display object classes, emitted in T154            */
/* ======================================================================== */

// FUNC 0x001959C8 RaceMiniMap::~RaceMiniMap
/* inline in game/race/display/T154_race_display.h; egcs emits this copy (the vptr, then
 * ~RaceDisplayObjectBase with the caller's flags) */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001959F0 __tf11RaceMiniMap
//   __si_type_info at 0x004B12A0, base __ti21RaceDisplayObjectBase (copy
//   T164 0x0019E6A0, weak there).
extern "C" const type_info &__tf11RaceMiniMap(void)
{
    if (__ti11RaceMiniMap[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti11RaceMiniMap, "11RaceMiniMap", (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti11RaceMiniMap;
}
#endif

// FUNC 0x00195A40 RaceOnboardSpeedmeter::~RaceOnboardSpeedmeter
/* inline in game/race/display/T162_race_onboard_speedmeter.h; egcs emits this copy (the
 * vptr, then ~RaceRoundMeter 0x0019CA58 with the caller's flags) */
/* implicit: ~RaceRoundMeter (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00195A68 __tf21RaceOnboardSpeedmeter
//   __si_type_info at 0x004B1360, base __ti14RaceRoundMeter (copy T162
//   0x0019E028, weak there).
extern "C" const type_info &__tf21RaceOnboardSpeedmeter(void)
{
    if (__ti21RaceOnboardSpeedmeter[0] == 0) {
        __tf14RaceRoundMeter();
        __rtti_si(__ti21RaceOnboardSpeedmeter, "21RaceOnboardSpeedmeter",
                  (const type_info *)__ti14RaceRoundMeter);
    }
    return *(const type_info *)__ti21RaceOnboardSpeedmeter;
}
#endif

// FUNC 0x00195AB8 RaceMeterBase::setScale
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x00195AC0 RaceMeterBase::set
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x00195AC8 RaceMeterBase::render_2nd
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy (empty) */

// FUNC 0x00195AD0 RaceMeterBase::~RaceMeterBase
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy (the vptr, then
 * ~RaceDisplayObjectBase with the caller's flags) */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00195AF8 __tf13RaceMeterBase
//   __si_type_info at 0x004B11D0, base __ti21RaceDisplayObjectBase.
//   Callers: the __tf of RaceBarMeter, RaceRoundMeterBase,
//   RaceBattleTachometer, RaceRoundTachometerBase (copies T160 / T161,
//   weak there).
extern "C" const type_info &__tf13RaceMeterBase(void)
{
    if (__ti13RaceMeterBase[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti13RaceMeterBase, "13RaceMeterBase", (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti13RaceMeterBase;
}

// FUNC 0x00195B48 __tf15RaceDisplayBase
//   __user_type_info at 0x003532E8 (gp-0x4708).  RaceDisplayBase has no
//   vtable of its own, so the function is emitted in every unit that needs
//   it (also T155's 0x00196450); this copy is the one called (by
//   __tf11RaceDisplay and __tf16RaceSplitDisplay).  Weak: both copies link.
extern "C" __attribute__((weak)) const type_info &__tf15RaceDisplayBase(void)
{
    if (__ti15RaceDisplayBase[0] == 0)
        __rtti_user(__ti15RaceDisplayBase, "15RaceDisplayBase");
    return *(const type_info *)__ti15RaceDisplayBase;
}
#endif
