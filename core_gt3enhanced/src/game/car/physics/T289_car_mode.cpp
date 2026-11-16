/* T289 - the display level of a car: 0x001E0668..0x001E0740.
 *
 * Data (retail), nothing the code references: .rodata 0x0033C4D0..
 * 0x0033C5A5, <new>'s "bad_alloc", then the race display names MiniMap,
 * OnboardSpeedmeter, RemoteTachometer, OnboardTachometer, MeterBase,
 * RaceCameraBase, UnitArena<64>, ValueDisplayBase, RaceDisplayBase (the
 * set and order of every retail includer of T154_race_display.h: the
 * original saw the displays through RaceSolitaire's headers); .sdata
 * 0x00352950, the "" of T157's inline RaceMessageDisplay::clear (8 bytes
 * with its padding, between T336's gains and T337's floats: T289 is -G8). */
// DATA_START 0x0033C4D0
// DATA_START 0x00352950
#include <new>                              /* (unused: "bad_alloc") */
#include "game/race/display/T154_race_display.h"         /* (seen first: the displays' type names, UnitArena<64>'s, "") */
#include "game/car/physics/car_engine_internal.h"
#include "game/race/modes/T139_race_solitaire.h"       /* RaceSolitaire::getShowGhost */
#include "game/car/physics/T289_car_mode.h"

/* CAR_SHOW_FULL for a car with a controller (st.noController 0: a normal
 * car).  The others (the ghosts and the data logger's other runs):
 * CAR_SHOW_MAP when neither the organization's loggerMode nor the race's
 * ghost switch (RaceSolitaire::showGhost, flipped by button 0x100) is
 * set; else CAR_SHOW_HIDDEN when the driver is 1 or the computer drives
 * (ctrlMode 1), CAR_SHOW_GHOST otherwise.  Callers: RacePS2Base::render
 * (ghosts drawn with the scan mask), RaceMiniMap::drawCar, RaceCarModel::
 * emitSparks / addLamps, CameraManager::isWatchable, F_0020E1F0 (T393),
 * car_silence_hidden. */
// FUNC 0x001E0668 car_display_level
int car_display_level(RaceCarState *car)
{
    DynCarState *st = &car->st;
    if (st->noController == 0)
        return CAR_SHOW_FULL;
    RaceOrganization *org = car->org;
    /* the running race read as a RaceSolitaire (the time trial and free run
     * races derive from it; other races have other data there); read only
     * when loggerMode is 0 (a RaceSolitaire local read first: 85.2%) */
    if (org->loggerMode == 0 && static_cast<RaceSolitaire *>(org->race)->getShowGhost() == 0)
        return CAR_SHOW_MAP;
    if (st->driver == 1 || st->ctrlMode == CTRL_COMPUTER)
        return CAR_SHOW_HIDDEN;
    return CAR_SHOW_GHOST;
}

/* Unless the car is shown in full: silences its tyres (smoke 0, volume 0,
 * pitch 1) and its three engine sound levels.  Called by car_step (T302). */
// FUNC 0x001E06D8 car_silence_hidden
void car_silence_hidden(RaceCarState *car)
{
    if (car_display_level(car) != CAR_SHOW_FULL) {
        DynCarState *st = &car->st;
        for (int i = 0; i < 4; i++) {
            DynWheel *w = &st->wheel[i];
            w->smoke = 0.0f;
            w->squealVolume = 0.0f;
            w->squealPitch = 1.0f;
        }
        st->engineSnd[1] = 0;
        st->engineSnd[2] = 0;
        st->engineSnd[0] = 0;
    }
}
