/* T162 - RaceOnboardSpeedmeter's constructor and dial set-up.
 * 0x0019DE30..0x0019E088.  The class itself (typeinfo, destructor, the
 * vtable the constructor stores) is T154's (game/race/display/T154_race_display.h); the
 * last four functions are this unit's copies of inline members (never
 * called).
 */
/* Its own header first, right after <new> (race_meters_internal.h, U056_OWN_HEADER):
 * retail's type names have RaceMeterBase's and RaceOnboardSpeedmeter's
 * last, after the race display header's (the T154 merge;
 * T160_race_meters.h). */
#define U056_OWN_HEADER "game/race/display/T162_race_onboard_speedmeter.h"
#include "game/race/display/race_meters_internal.h"

/* T162's data starts unreferenced: "bad_alloc" (<new>, read first, the
 * first item of its .rodata) and, in .sdata, the "" of T157's inline
 * RaceMessageDisplay::clear (read with the race display headers; T161's
 * .sdata ends at 0x00352156, T163's "" is 0x00352160). */
// DATA_START 0x0032F958
// DATA_START 0x00352158

// FUNC 0x0019DE30 RaceOnboardSpeedmeter::RaceOnboardSpeedmeter
//   Caller: RaceDisplay's constructor.  RaceRoundMeter's constructor, then
//   T154's vtable 0x0032E2E8 (not this unit's copy 0x0032FA08) and the
//   dial's look; the inline setters' stores come first, then the three
//   set-up calls.  EH cleanup: ~RaceRoundMeter.  The setters' order is the
//   one that gives retail's (0.95 is the first .lit4 constant).
RaceOnboardSpeedmeter::RaceOnboardSpeedmeter()
{
    /* implicit: RaceRoundMeter (base constructor), ~RaceRoundMeter (exception cleanup) */
    setPanelColor(METER_PANEL_COLOR);
    setScaleColor(METER_SCALE_COLOR, METER_SCALE_RED_COLOR);
    setScaleRadius(0.95f);
    setNeedle(-0.1f, 0.95f);
    setCenter(0.0f, 23.0f);
    setRadius(75.0f, 1.0f);
    setAngles(72.5f, 287.5f);
}

/* The dial tables (.rodata, each after its image names: egcs writes a
 * table's string literals just before it, "speed_200mp" / "speed_320km"
 * once; external linkage (race_meters_internal.h declares them), so written here, before
 * the vtable and the type names).  The last entry of each is FLT_MAX
 * (0x7F7FFFFF). */
const SpeedScale g_0032F998[4] = {          /* 0x0032F998 mph */
    { 120.0f, "speed_120mp" },
    { 160.0f, "speed_160mp" },
    { 200.0f, "speed_200mp" },
    { 3.40282347e+38f, "speed_200mp" },
};
const SpeedScale g_0032F9E8[4] = {          /* 0x0032F9E8 km/h */
    { 180.0f, "speed_180km" },
    { 240.0f, "speed_240km" },
    { 320.0f, "speed_320km" },
    { 3.40282347e+38f, "speed_320km" },
};

// FUNC 0x0019DF08 RaceOnboardSpeedmeter::setupScale
//   ? name.  Caller: RaceDisplay::setupCarMeters (mph = the car shows mph,
//   maxSpeed = the top of its scale).  The dial image of the first entry of
//   the mph / km/h table the speed fits in (none: unchanged; the last entry
//   is FLT_MAX), scale marks every 5 mph / 10 km/h, and the full scale.
//   Each branch has its own setScaleLine (retail: two copies of the 0.03 /
//   0.85 constants in .lit4) and its own counter (one `i' for both swaps
//   the counter's and the cursor's registers).
void RaceOnboardSpeedmeter::setupScale(int mph, f32 maxSpeed)
{
    /* The enhanced engine (ENH_HUD_SPEEDO_SCALE, hud.md C9; Lupo 2
     * L2 0x0019AE18): the full scale is the chosen dial's (GT3: the car's
     * scale, so a 160 km/h scale reached the "180" mark at 160); the
     * tables' last entry (FLT_MAX) keeps the car's scale. */
    f32 full = maxSpeed;
    if (mph != 0) {
        int i;

        for (i = 0; i < 4; i++) {
            if (maxSpeed <= g_0032F998[i].maxSpeed) {
                setImage(g_0032F998[i].image);      /* RaceDisplayObjectBase::setImage */
                if (i < 3)
                    full = g_0032F998[i].maxSpeed;
                break;
            }
        }
        setScaleLine(0.03f, 0.85f, 5.0f);
    } else {
        int i;

        for (i = 0; i < 4; i++) {
            if (maxSpeed <= g_0032F9E8[i].maxSpeed) {
                setImage(g_0032F9E8[i].image);
                if (i < 3)
                    full = g_0032F9E8[i].maxSpeed;
                break;
            }
        }
        setScaleLine(0.03f, 0.85f, 10.0f);
    }
#if ENH_HUD_SPEEDO_SCALE
    scale = full;
#else
    scale = maxSpeed;
#endif
}

// FUNC 0x0019E000 RaceOnboardSpeedmeter::~RaceOnboardSpeedmeter
/* inline in game/race/display/T154_race_display.h; egcs emits this copy (of 0x00195A40):
 * the vptr (0x0032E2E8), then ~RaceRoundMeter (0x0019CA58) with the
 * caller's flags. */
/* implicit: ~RaceRoundMeter (base destructor) */

/* T162's copy of 0x00195A68 (its own name string 0x0032FB28):
 * __si_type_info at 0x004B1360, base __ti14RaceRoundMeter.  Weak so that
 * T154's copy links too. */
#ifndef BOOT_OLD_GCC
// FUNC 0x0019E028 __tf21RaceOnboardSpeedmeter
extern "C" __attribute__((weak)) const type_info &__tf21RaceOnboardSpeedmeter(void)
{
    if (__ti21RaceOnboardSpeedmeter[0] == 0) {
        __tf14RaceRoundMeter();
        __rtti_si(__ti21RaceOnboardSpeedmeter, "21RaceOnboardSpeedmeter",
                  (const type_info *)__ti14RaceRoundMeter);
    }
    return *(const type_info *)__ti21RaceOnboardSpeedmeter;
}
#endif

// FUNC 0x0019E078 RaceMeterBase::setScale
/* inline in game/race/display/T154_race_display.h (scale = v); egcs emits this copy */

// FUNC 0x0019E080 RaceMeterBase::set
/* inline in game/race/display/T154_race_display.h (value = v); egcs emits this copy */
