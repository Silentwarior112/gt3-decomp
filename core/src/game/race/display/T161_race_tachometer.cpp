/* T161 - the tachometers of the race display.  0x0019D278..0x0019DE30.
 *
 * RaceRoundTachometerBase is a RaceMeterBase that holds a RaceRoundMeter
 * (the dial, at +0x20) and forwards the display object calls to it; it adds
 * the dial image prefix and the transmission mark printed next to the dial
 * centre.  RaceOnboardTachometer / RaceRemoteTachometer only set it up
 * differently.  RaceBattleTachometer does the same around a RaceBarMeter.
 * The destructors are inline (game/race/display/T161_race_tachometer.h); the last five
 * functions are out-of-line copies of RaceMeterBase's inline members
 * (T154; never called).
 */
/* Its own header first, right after <new> (race_meters_internal.h, U056_OWN_HEADER):
 * retail's type names have RaceMeterBase's and T161's own
 * last, after the race display header's (the T154 merge;
 * T160_race_meters.h). */
#define U056_OWN_HEADER "game/race/display/T161_race_tachometer.h"
#include "game/race/display/race_meters_internal.h"

/* T161's .rodata starts with "bad_alloc" (<new>, read first), which its
 * code does not reference (the dial table 0x0032F5B8 is the first item it
 * does). */
// DATA_START 0x0032F5A8

/* ======================================================================== */
/* RaceRoundTachometerBase                                                  */
/* ======================================================================== */

// FUNC 0x0019D278 RaceRoundTachometerBase::RaceRoundTachometerBase
//   Callers: the two derived constructors.  RaceMeterBase's constructor
//   (0x0019C368), the vptr, the dial's constructor (RaceRoundMeter,
//   0x0019C928), then the defaults.  EH cleanup: ~RaceRoundMeter on the
//   dial, then RaceMeterBase's vptr and ~RaceDisplayObjectBase.
//   Retail keeps the dial's address (this + 0x20, the member constructor's
//   argument) in an s-register for the stores: the setters go through a
//   pointer to the dial (exact; written on `meter' directly they use `this').
RaceRoundTachometerBase::RaceRoundTachometerBase()
{
    /* implicit: RaceMeterBase (base constructor), RaceRoundMeter (member constructor), ~RaceRoundMeter, ~RaceDisplayObjectBase (exception cleanup) */
    RaceRoundMeter *m = &meter;

    setPrefix("");                          /* 0x003520F8 */
    m->setScaleColor(METER_SCALE_COLOR, METER_SCALE_RED_COLOR);
    m->setPanelColor(METER_PANEL_COLOR);
    m->enabled = 1;
}

/* The dial table (.rodata 0x0032F5B8; the suffixes are this unit's .sdata
 * 0x00352100..0x00352140, between the constructor's "" and drawMeter's
 * "meter#1": the table is defined here, after the constructor). */
const TachoScale g_0032F5B8[8] = {          /* 0x0032F5B8 */
    { 6000.0f, 500.0f, "06" },
    { 7000.0f, 500.0f, "07" },
    { 8000.0f, 500.0f, "08" },
    { 9000.0f, 500.0f, "09" },
    { 10000.0f, 500.0f, "10" },
    { 11000.0f, 500.0f, "11" },
    { 12000.0f, 500.0f, "12" },
    { 18000.0f, 1000.0f, "18" },
};

// FUNC 0x0019D350 RaceRoundTachometerBase::setScale
//   index: RaceRemoteTachometer::virtual_12 (slot 12 of all three vtables).
//   Caller: RaceDisplay::setupCarMeters with the car's rev limit.  Picks the
//   first g_0032F5B8 entry the limit fits in: the scale marks (length 0.03,
//   fill 0.85, its rpm step) and the dial image prefix + suffix ("rev_08").
//   The full scale is the limit even when no entry fits.  An index loop
//   (loop.c turns `i < 8' into a signed pointer compare, slt).
void RaceRoundTachometerBase::setScale(f32 maxRpm)
{
    char name[0x80];
    int i;

    for (i = 0; i < 8; i++) {
        if (maxRpm <= g_0032F5B8[i].maxRpm) {
            meter.setScaleLine(0.03f, 0.85f, g_0032F5B8[i].step);
            strcpy(name, prefix);
            strcat(name, g_0032F5B8[i].suffix);
            meter.setImage(name);           /* RaceDisplayObjectBase::setImage */
            break;
        }
    }
    meter.setScale(maxRpm);                 /* RaceMeterBase's, inline */
}

// FUNC 0x0019D408 RaceRoundTachometerBase::setPrefix
//   ? name.  Callers: the three constructors.
void RaceRoundTachometerBase::setPrefix(const char *s)
{
    strcpy(prefix, s);
}

// FUNC 0x0019D428 RaceRoundTachometerBase::drawMeter
//   index: RaceRemoteTachometer::drawMeter (slot 5 of all three vtables).
//   The dial's panel (RaceRoundMeterBase::drawMeter, a direct call; buf is
//   passed on untouched), then one character of the "meter#1" font at the
//   dial centre + (markX, markY): the transmission mark.
void RaceRoundTachometerBase::drawMeter(GSBuffer &buf)
{
    RaceRoundMeter *m = &meter;             /* retail: this + 0x20 in an s-register */
    int posX, posY;

    m->RaceRoundMeterBase::drawMeter(buf);
    g_002F1968.select("meter#1");
    g_002F1968.setColor(METER_PANEL_COLOR);
    posX = race_align_pos(m->x, m->w, m->alignX) + (m->w >> 1) + markX;
    posY = race_align_pos(m->y, m->h, m->alignY) + (m->h >> 1) + markY;
    g_002F1968.setPos3(posX, posY, 0);
    g_002F1968.putChar(transmission != 0 ? TACHO_MARK_MANUAL : TACHO_MARK_AUTOMATIC);
}

/* ======================================================================== */
/* RaceOnboardTachometer / RaceRemoteTachometer                             */
/* ======================================================================== */

// FUNC 0x0019D510 RaceOnboardTachometer::RaceOnboardTachometer
//   Caller: RaceDisplay's constructor.  RaceRoundTachometerBase's
//   constructor, the vptr, the dial's set-up calls (0x0019C8E8 /
//   0x0019C8F8 / 0x0019C908) and inline setters.  EH cleanup: the base's
//   vptr, ~RaceRoundMeter on the dial (flags 2), RaceMeterBase's vptr,
//   ~RaceDisplayObjectBase.
RaceOnboardTachometer::RaceOnboardTachometer()
{
    /* implicit: RaceRoundTachometerBase (base constructor), ~RaceRoundMeter, ~RaceDisplayObjectBase (exception cleanup, inline ~RaceRoundTachometerBase) */
    RaceRoundMeter *m = &meter;             /* as the base constructor's */

    m->setCenter(0.0f, 23.0f);
    m->setRadius(75.0f, 1.0f);
    m->setScaleRadius(0.95f);
    m->setAngles(72.5f, 287.5f);
    m->setNeedle(-0.2f, 0.95f);
    setPrefix("rev_");
    markX = -10;
    markY = -15;
}

// FUNC 0x0019D620 RaceRemoteTachometer::RaceRemoteTachometer
//   Caller: RaceDisplay's constructor.  As RaceOnboardTachometer's (the same
//   EH cleanup: ~RaceRoundMeter, ~RaceDisplayObjectBase; base constructor
//   RaceRoundTachometerBase).
RaceRemoteTachometer::RaceRemoteTachometer()
{
    /* implicit: RaceRoundTachometerBase (base constructor), ~RaceRoundMeter, ~RaceDisplayObjectBase (exception cleanup, inline ~RaceRoundTachometerBase) */
    RaceRoundMeter *m = &meter;

    m->setCenter(3.0f, 4.0f);
    m->setRadius(73.0f, 1.0f);
    m->setScaleRadius(0.95f);
    m->setAngles(29.85f, 238.34f);
    m->setNeedle(0.38f, 0.9f);
    m->setScaleColor(METER_SCALE_COLOR, METER_SCALE_RED_COLOR);    /* the base constructor's again */
    m->setPanelColor(METER_PANEL_COLOR);
    setPrefix("revb_");
    markX = -2;
    markY = 28;
}

/* ======================================================================== */
/* RaceBattleTachometer                                                     */
/* ======================================================================== */

// FUNC 0x0019D750 RaceBattleTachometer::RaceBattleTachometer
//   Caller: RaceDisplay's constructor.  RaceMeterBase's constructor, the
//   vptr, the bar's constructor (0x0019CEE0).  EH cleanup: RaceMeterBase's
//   vptr, ~RaceDisplayObjectBase.
RaceBattleTachometer::RaceBattleTachometer()
{
    /* implicit: RaceMeterBase (base constructor), RaceBarMeter (member constructor), ~RaceDisplayObjectBase (exception cleanup, inline ~RaceMeterBase) */
    transmission = 0;
}

// FUNC 0x0019D7C8 RaceBattleTachometer::setScale
//   index: RaceBattleTachometer::virtual_12.  Caller:
//   RaceDisplay::setupCarMeters.  The rev limit is not used: the bar always
//   shows "meter_mini".
void RaceBattleTachometer::setScale(f32 maxRpm)
{
    bar.setImage("meter_mini");
}

// FUNC 0x0019D7F0 RaceBattleTachometer::drawMeter
//   The bar (RaceBarMeter::drawMeter, direct, buf passed on), then the
//   transmission mark at the bar's aligned corner, 5 pixels down.
void RaceBattleTachometer::drawMeter(GSBuffer &buf)
{
    RaceBarMeter *b = &bar;                 /* retail: this + 0x20 in an s-register */
    int posX, posY;

    b->RaceBarMeter::drawMeter(buf);
    g_002F1968.select("meter#1");
    g_002F1968.setColor(METER_PANEL_COLOR);
    posX = race_align_pos(b->x, b->w, b->alignX);
    posY = race_align_pos(b->y, b->h, b->alignY) + 5;
    g_002F1968.setPos3(posX, posY, 0);
    g_002F1968.putChar(transmission != 0 ? TACHO_MARK_MANUAL : TACHO_MARK_AUTOMATIC);
}

/* 0x0019D8B0..0x0019DA68: RaceBattleTachometer's inline members, written
 * in game/race/display/T161_race_tachometer.h (in-class, declared in the reverse of this
 * order: egcs emits them at the end of the unit, last-declared first, then
 * the class's __tf; the frame info has retail's function order only so).
 * The markers name the symbols (the copies' stab lines are in the
 * header). */

// FUNC 0x0019D8B0 RaceBattleTachometer::setImage (setImage__20RaceBattleTachometerPCc)
//   ? name.  No callers.  The bar's named image (0x0019D030).
/* implicit: RaceBarMeter::setImage (the body is in the header) */

// FUNC 0x0019D8D0 RaceBattleTachometer::setImage (setImage__20RaceBattleTachometerP9Tex1Image)
//   ? name.  No callers.  (0x0019D010.)
/* implicit: RaceBarMeter::setImage (the body is in the header) */

// FUNC 0x0019D8F0 RaceBattleTachometer::setImagePack (setImagePack__20RaceBattleTachometerP9ImagePack)

// FUNC 0x0019D8F8 RaceBattleTachometer::setRegion (setRegion__20RaceBattleTachometeriiii)
//   RaceDisplayObjectBase::setRegion (inline, T156) on the bar: setLocation
//   and setSize are virtual calls through the bar's vtable.

// FUNC 0x0019D960 RaceBattleTachometer::setAlignment (setAlignment__20RaceBattleTachometerUcUc)
//   The bar's (RaceDisplayObjectBase::setAlignment, inline).

// FUNC 0x0019D970 RaceBattleTachometer::setSize (setSize__20RaceBattleTachometerii)

// FUNC 0x0019D980 RaceBattleTachometer::setLocation (setLocation__20RaceBattleTachometerii)

// FUNC 0x0019D990 RaceBattleTachometer::setTransmission (setTransmission__20RaceBattleTachometeri)
//   transmission = t, an int (RaceDisplay::setupCarMeters expands it).

// FUNC 0x0019D998 RaceBattleTachometer::setRedZone (setRedZone__20RaceBattleTachometerf)
//   The bar's full scale (+0x3C = bar.scale).

// FUNC 0x0019D9A0 RaceBattleTachometer::set (set__20RaceBattleTachometerf)
//   The current rpm: bar.value (+0x38).

// FUNC 0x0019D9A8 RaceBattleTachometer::~RaceBattleTachometer
/* inline in game/race/display/T161_race_tachometer.h; egcs emits this copy: the vptr;
 * the bar's ~RaceBarMeter expanded (RaceMeterBase's vptr, then
 * ~RaceDisplayObjectBase with flags 2); RaceMeterBase's vptr, then
 * ~RaceDisplayObjectBase with the caller's flags. */
/* implicit: ~RaceDisplayObjectBase (inline ~RaceBarMeter and ~RaceMeterBase) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0019DA18 __tf20RaceBattleTachometer
//   __si_type_info at 0x004B2330, base __ti13RaceMeterBase.
extern "C" const type_info &__tf20RaceBattleTachometer(void)
{
    if (__ti20RaceBattleTachometer[0] == 0) {
        __tf13RaceMeterBase();
        __rtti_si(__ti20RaceBattleTachometer, "20RaceBattleTachometer",
                  (const type_info *)__ti13RaceMeterBase);
    }
    return *(const type_info *)__ti20RaceBattleTachometer;
}
#endif

/* ======================================================================== */
/* Destructors / typeinfo of the round tachometers                          */
/* ======================================================================== */

// FUNC 0x0019DA68 RaceRemoteTachometer::~RaceRemoteTachometer
/* inline in game/race/display/T161_race_tachometer.h; egcs emits this copy:
 * ~RaceRoundTachometerBase expanded (its vptr, ~RaceRoundMeter on the dial
 * with flags 2, RaceMeterBase's vptr, then ~RaceDisplayObjectBase with the
 * caller's flags). */
/* implicit: ~RaceRoundMeter (member destructor), ~RaceDisplayObjectBase (inline ~RaceMeterBase) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0019DAC8 __tf20RaceRemoteTachometer
//   __si_type_info at 0x004B11E0, base __ti23RaceRoundTachometerBase.
extern "C" const type_info &__tf20RaceRemoteTachometer(void)
{
    if (__ti20RaceRemoteTachometer[0] == 0) {
        __tf23RaceRoundTachometerBase();
        __rtti_si(__ti20RaceRemoteTachometer, "20RaceRemoteTachometer",
                  (const type_info *)__ti23RaceRoundTachometerBase);
    }
    return *(const type_info *)__ti20RaceRemoteTachometer;
}
#endif

// FUNC 0x0019DB18 RaceOnboardTachometer::~RaceOnboardTachometer
/* inline in game/race/display/T161_race_tachometer.h; egcs emits this copy (as
 * RaceRemoteTachometer's: ~RaceRoundMeter, ~RaceDisplayObjectBase). */
/* implicit: ~RaceRoundMeter (member destructor), ~RaceDisplayObjectBase (inline ~RaceMeterBase) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0019DB78 __tf21RaceOnboardTachometer
//   __si_type_info at 0x004B1270, base __ti23RaceRoundTachometerBase.
extern "C" const type_info &__tf21RaceOnboardTachometer(void)
{
    if (__ti21RaceOnboardTachometer[0] == 0) {
        __tf23RaceRoundTachometerBase();
        __rtti_si(__ti21RaceOnboardTachometer, "21RaceOnboardTachometer",
                  (const type_info *)__ti23RaceRoundTachometerBase);
    }
    return *(const type_info *)__ti21RaceOnboardTachometer;
}
#endif

/* ======================================================================== */
/* RaceRoundTachometerBase: forwarding to the dial                          */
/* ======================================================================== */

/* 0x0019DBC8..0x0019DD50: RaceRoundTachometerBase's inline members, written
 * in game/race/display/T161_race_tachometer.h (in-class, declared in the reverse of this
 * order, as RaceBattleTachometer's above). */

// FUNC 0x0019DBC8 RaceRoundTachometerBase::setImage (setImage__23RaceRoundTachometerBasePCc)
//   ? name.  No callers.  The dial's named image (RaceDisplayObjectBase::
//   setImage, 0x001965A8).
/* implicit: RaceDisplayObjectBase::setImage (the body is in the header) */

// FUNC 0x0019DBE8 RaceRoundTachometerBase::setImage (setImage__23RaceRoundTachometerBaseP9Tex1Image)
//   ? name.  No callers.  meter.image (+0x24).

// FUNC 0x0019DBF0 RaceRoundTachometerBase::setImagePack (setImagePack__23RaceRoundTachometerBaseP9ImagePack)
//   index: EngineSound::setMuffler(EngineSound*) (lupo2m, a name matched by
//   its code: one `sw $a1, 0x20($a0)`).  Here: meter.imagePack (+0x20), the
//   counterpart of RaceBattleTachometer::setImagePack.

// FUNC 0x0019DBF8 RaceRoundTachometerBase::updateScale (updateScale__23RaceRoundTachometerBase)
//   ? name.  No callers (RaceDisplay::setupCarMeters calls the dial's
//   RaceRoundMeter::updateScale, 0x0019CA80, directly).
/* implicit: RaceRoundMeter::updateScale (the body is in the header) */

// FUNC 0x0019DC18 RaceRoundTachometerBase::setRegion (setRegion__23RaceRoundTachometerBaseiiii)
//   RaceDisplayObjectBase::setRegion (inline, T156) on the dial: two
//   virtual calls through the dial's vtable.

// FUNC 0x0019DC80 RaceRoundTachometerBase::setAlignment (setAlignment__23RaceRoundTachometerBaseUcUc)

// FUNC 0x0019DC90 RaceRoundTachometerBase::setSize (setSize__23RaceRoundTachometerBaseii)

// FUNC 0x0019DCA0 RaceRoundTachometerBase::setLocation (setLocation__23RaceRoundTachometerBaseii)

// FUNC 0x0019DCB0 RaceRoundTachometerBase::setTransmission (setTransmission__23RaceRoundTachometerBasei)
//   transmission = t, an int (RaceDisplay::setupCarMeters expands it on
//   both round tachometers).

// FUNC 0x0019DCB8 RaceRoundTachometerBase::setRedZone (setRedZone__23RaceRoundTachometerBasef)
//   ? name.  meter.redZone (+0x70).

// FUNC 0x0019DCC0 RaceRoundTachometerBase::set (set__23RaceRoundTachometerBasef)
//   The current rpm: meter.value (+0x38).

// FUNC 0x0019DCC8 RaceRoundTachometerBase::render_2nd (render_2nd__23RaceRoundTachometerBase)
//   The dial (RaceRoundMeterBase::render_2nd, direct) when this object (not
//   the dial) is enabled.
/* implicit: RaceRoundMeterBase::render_2nd (the body is in the header) */

// FUNC 0x0019DCF0 RaceRoundTachometerBase::~RaceRoundTachometerBase
/* inline in game/race/display/T161_race_tachometer.h; egcs emits this copy: the vptr,
 * ~RaceRoundMeter on the dial (flags 2), RaceMeterBase's vptr, then
 * ~RaceDisplayObjectBase with the caller's flags. */
/* implicit: ~RaceRoundMeter (member destructor), ~RaceDisplayObjectBase (inline ~RaceMeterBase) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0019DD50 __tf23RaceRoundTachometerBase
//   Callers: the two derived __tf functions.  __si_type_info at 0x004B2340,
//   base __ti13RaceMeterBase.
extern "C" const type_info &__tf23RaceRoundTachometerBase(void)
{
    if (__ti23RaceRoundTachometerBase[0] == 0) {
        __tf13RaceMeterBase();
        __rtti_si(__ti23RaceRoundTachometerBase, "23RaceRoundTachometerBase",
                  (const type_info *)__ti13RaceMeterBase);
    }
    return *(const type_info *)__ti23RaceRoundTachometerBase;
}
#endif

/* ======================================================================== */
/* Out-of-line copies of RaceMeterBase's inline members (never called; the  */
/* vtables use T154's 0x00195AB8..0x00195AF8)                               */
/* ======================================================================== */

// FUNC 0x0019DDA0 RaceMeterBase::setScale
/* inline in game/race/display/T154_race_display.h (scale = v); egcs emits this copy */

// FUNC 0x0019DDA8 RaceMeterBase::set
/* inline in game/race/display/T154_race_display.h (value = v); egcs emits this copy */

// FUNC 0x0019DDB0 RaceMeterBase::render_2nd
/* inline in game/race/display/T154_race_display.h (empty); egcs emits this copy.
 * index: VehicleModel::delayLoadDriverModel(const char*) (lupo2m, matched
 * by its code: a bare `jr $ra`); it sits between RaceMeterBase::set and
 * ~RaceMeterBase as in T154. */

// FUNC 0x0019DDB8 RaceMeterBase::~RaceMeterBase
/* inline in game/race/display/T154_race_display.h; egcs emits this copy: the vptr, then
 * ~RaceDisplayObjectBase with the caller's flags. */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* T161's copy (its own name string 0x0032F948): __si_type_info at
 * 0x004B11D0, base __ti21RaceDisplayObjectBase (gp-0x4650).  Weak so that
 * T154's copy (0x00195AF8) links too. */
#ifndef BOOT_OLD_GCC
// FUNC 0x0019DDE0 __tf13RaceMeterBase
extern "C" __attribute__((weak)) const type_info &__tf13RaceMeterBase(void)
{
    if (__ti13RaceMeterBase[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti13RaceMeterBase, "13RaceMeterBase",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti13RaceMeterBase;
}
#endif
