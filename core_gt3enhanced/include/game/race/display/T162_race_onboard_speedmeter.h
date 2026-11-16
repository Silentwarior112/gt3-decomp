/* T162_race_onboard_speedmeter.h - RaceOnboardSpeedmeter, the speedometer
 * dial of the race display (translation unit T162, unit u056),
 * 0x0019DE30..0x0019E088.
 *
 * T162 defines its constructor and the dial set-up:
 *     RaceOnboardSpeedmeter::RaceOnboardSpeedmeter()   0x0019DE30
 *     RaceOnboardSpeedmeter::setupScale(int mph, f32 maxSpeed)  0x0019DF08
 *         (? name; the dial image of the first entry of the mph table
 *         g_0032F998 / km/h table g_0032F9E8 the top speed fits in, marks
 *         every 5 mph / 10 km/h; caller RaceDisplay::setupCarMeters).
 * The class has no key function (only its inline destructor): its vtable,
 * destructor and typeinfo function are emitted where it is built, in T154
 * (0x0032E2E8, 0x00195A40, 0x00195A68: the ones RaceDisplay uses) and in
 * T162 (0x0032FA08, 0x0019E000, 0x0019E028, with RaceMeterBase::setScale
 * 0x0019E078 / set 0x0019E080; never called).
 *
 * Its own header (in T154_race_display.h before the T154 merge): retail's
 * T162 has RaceMeterBase's and this class's type names last, after the
 * display's (the source includes this header first), the other display
 * objects have it before the mini map's (T154_race_display.h includes it
 * after the meters; see T160_race_meters.h).
 */
#ifndef GAME_T162_RACE_ONBOARD_SPEEDMETER_H
#define GAME_T162_RACE_ONBOARD_SPEEDMETER_H

#include "game/game.h"
#include "game/race/display/T160_race_meters.h"          /* base RaceRoundMeter */

class type_info;                            /* T548 */

/* The typeinfo function, declared before the class for GCC 14 only (egcs
 * generates it; an earlier declaration would stop it): the __si_type_info
 * 0x004B1360, base __ti14RaceRoundMeter (it calls __tf14RaceRoundMeter,
 * T160).  T154's copy 0x00195A68, T162's 0x0019E028 (its own name string
 * 0x0032FB28). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf21RaceOnboardSpeedmeter(void);    /* 0x00195A68 (T162 0x0019E028) */
}
#endif

/* The speedometer dial (RaceDisplay +0x7040).  vtables 0x0032E2E8 (T154; the
 * constructor stores it) and 0x0032FA08 (T162), 15 slots (only [0] its
 * own), typeinfo __tf21RaceOnboardSpeedmeter 0x00195A68 (si, base
 * RaceRoundMeter).  No data of its own: 0x307C bytes. */
class RaceOnboardSpeedmeter : public RaceRoundMeter {
public:
    RaceOnboardSpeedmeter();                /* 0x0019DE30 */
    virtual ~RaceOnboardSpeedmeter() {}     /* [0] +0x0C 0x00195A40 (T162 copy 0x0019E000) */
    /* ? name.  The dial images (mph != 0: the mph ones) and the scale for
     * maxSpeed (RaceDisplay::setupCarMeters). */
    void setupScale(int mph, f32 maxSpeed); /* 0x0019DF08 */
};
EGCS_SIZE(RaceOnboardSpeedmeter, 0x307C);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti21RaceOnboardSpeedmeter[3];  /* 0x004B1360 */
}
#endif

#endif /* GAME_T162_RACE_ONBOARD_SPEEDMETER_H */
