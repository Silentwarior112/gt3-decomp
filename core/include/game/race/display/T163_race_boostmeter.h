/* T163_race_boostmeter.h - RaceBoostmeter, the turbo boost dial of the race
 * display (translation unit T163, unit u056), 0x0019E088..0x0019E230.
 *
 * A RaceRoundMeterBase (full scale 2.0, centre (0, 0), radius 32, angles
 * -45..200.32, needle -0.1..0.95, panel colour 0x80A0A088) drawn only while
 * `visible', which RaceDisplay (T154, +0xA0BC) sets for turbo cars.  T163
 * also has unreferenced copies of RaceMeterBase's inline setScale / set
 * (0x0019E220 / 0x0019E228; T160_race_meters.h declares them).
 *
 * T154_race_display.h includes it after the tachometers (RaceDisplay holds
 * RaceBoostmeter by value); T163's source includes it first (retail's T163
 * type names: RaceBoostmeter's and RaceMeterBase's last, see
 * T160_race_meters.h).
 */
#ifndef GAME_T163_RACE_BOOSTMETER_H
#define GAME_T163_RACE_BOOSTMETER_H

#include "game/game.h"
#include "game/race/display/T160_race_meters.h"          /* base RaceRoundMeterBase (RaceMeterBase) */

class GSBuffer;                             /* T514 (references only) */
class type_info;                            /* T548 */

/* Declared before the class, for GCC 14 only: egcs generates it itself (in
 * T163, with the vtable); an earlier extern "C" declaration would be the
 * one egcs uses, and it would never define the function (see T161).  The
 * __si_type_info at 0x004B2350, base __ti18RaceRoundMeterBase 0x004B2300. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf14RaceBoostmeter(void);            /* 0x0019E1D0 */
}
#endif

/* vtable 0x0032FB60 (15 slots: [0] [5] [10] its own; index.txt names [10]
 * RaceBoostmeter::virtual_10), typeinfo __tf14RaceBoostmeter 0x0019E1D0
 * (si, base RaceRoundMeterBase).  vptr +0x14 (RaceDisplayObjectBase's).
 * Size 0x50 (RaceDisplay +0xA0BC). */
class RaceBoostmeter : public RaceRoundMeterBase {
public:
    /* Caller: RaceDisplay's constructor.  The dial's look (see above);
     * visible = 0. */
    RaceBoostmeter();                       /* 0x0019E088 */
    virtual ~RaceBoostmeter();              /* [ 0] +0x0C 0x0019E150 (index: virtual_0) */
    /* RaceRoundMeterBase::drawMeter (direct) when visible. */
    virtual void drawMeter(GSBuffer &buf);  /* [ 5] +0x34 0x0019E178 */
    /* RaceRoundMeterBase::render_2nd (direct) when visible. */
    virtual void render_2nd();              /* [10] +0x5C 0x0019E1A0 (index: virtual_10) */
    /* ? name.  Inline: RaceDisplay::setupCarMeters (T154) expands it;
     * 0x0019E1C8 is the copy egcs emits in T163. */
    void setVisible(u8 v) { visible = v; }  /* 0x0019E1C8 */

    u8 visible;                             /* 0x4C the car has a turbo (sb / lbu; u051 / u052 s8) */
    u8 unk_4D[3];                           /* 0x4D */
};
EGCS_SIZE(RaceBoostmeter, 0x50);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti14RaceBoostmeter[3];     /* 0x004B2350 __si_type_info (u056 g_004B2350) */
#endif

#endif /* GAME_T163_RACE_BOOSTMETER_H */
