/* T163 - RaceBoostmeter, the turbo boost dial (drawn only when `visible',
 * which RaceDisplay::setupCarMeters sets for turbo cars).
 * 0x0019E088..0x0019E230.  The last two functions are copies of
 * RaceMeterBase's inline members (never called).
 */
/* Its own header first, right after <new> (race_meters_internal.h, U056_OWN_HEADER):
 * retail's type names have RaceMeterBase's and RaceBoostmeter's
 * last, after the race display header's (the T154 merge;
 * T160_race_meters.h). */
#define U056_OWN_HEADER "game/race/display/T163_race_boostmeter.h"
#include "game/race/display/race_meters_internal.h"

/* T163's data starts unreferenced: "bad_alloc" (<new>, the first item of
 * its .rodata) and the "" of T157's inline RaceMessageDisplay::clear (its
 * only .sdata). */
// DATA_START 0x0032FB50
// DATA_START 0x00352160

// FUNC 0x0019E088 RaceBoostmeter::RaceBoostmeter
//   Caller: RaceDisplay's constructor.  RaceRoundMeterBase's constructor
//   (0x0019C3B8), the vptr, then the dial's look (full scale 2.0).  EH
//   cleanup: ~RaceRoundMeterBase.
RaceBoostmeter::RaceBoostmeter()
{
    /* implicit: RaceRoundMeterBase (base constructor), ~RaceRoundMeterBase (exception cleanup) */
    scale = 2.0f;
    setCenter(0.0f, 0.0f);
    setRadius(32.0f, 1.0f);
    setAngles(-45.0f, 200.32f);
    setNeedle(-0.1f, 0.95f);                /* inline */
    setPanelColor(METER_PANEL_COLOR);       /* inline */
    visible = 0;
}

// FUNC 0x0019E150 RaceBoostmeter::~RaceBoostmeter
//   index: RaceBoostmeter::virtual_0.  Callers: RaceDisplay's constructor
//   (EH cleanup) and destructor.  The vptr, then ~RaceRoundMeterBase
//   (0x0019C4A0) with the caller's flags.
RaceBoostmeter::~RaceBoostmeter()
{
    /* implicit: ~RaceRoundMeterBase (base destructor) */
}

// FUNC 0x0019E178 RaceBoostmeter::drawMeter
//   The panel (RaceRoundMeterBase::drawMeter, direct, buf passed on) when
//   visible.
void RaceBoostmeter::drawMeter(GSBuffer &buf)
{
    if (visible)
        RaceRoundMeterBase::drawMeter(buf);
}

// FUNC 0x0019E1A0 RaceBoostmeter::render_2nd
//   index: RaceBoostmeter::virtual_10.  Caller: RaceDisplay::virtual_3.
//   The dial (RaceRoundMeterBase::render_2nd, direct) when visible.
void RaceBoostmeter::render_2nd()
{
    if (visible)
        RaceRoundMeterBase::render_2nd();
}

// FUNC 0x0019E1C8 RaceBoostmeter::setVisible
/* inline in game/race/display/T163_race_boostmeter.h (visible = v; RaceDisplay::
 * setupCarMeters expands it); egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x0019E1D0 __tf14RaceBoostmeter
//   __si_type_info at 0x004B2350, base __ti18RaceRoundMeterBase.
extern "C" const type_info &__tf14RaceBoostmeter(void)
{
    if (__ti14RaceBoostmeter[0] == 0) {
        __tf18RaceRoundMeterBase();
        __rtti_si(__ti14RaceBoostmeter, "14RaceBoostmeter",
                  (const type_info *)__ti18RaceRoundMeterBase);
    }
    return *(const type_info *)__ti14RaceBoostmeter;
}
#endif

// FUNC 0x0019E220 RaceMeterBase::setScale
/* inline in game/race/display/T154_race_display.h (scale = v); egcs emits this copy */

// FUNC 0x0019E228 RaceMeterBase::set
/* inline in game/race/display/T154_race_display.h (value = v); egcs emits this copy */
