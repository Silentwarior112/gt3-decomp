/* T306 (0x001EE638..0x001EE7E8): a data logger OneLapGhostInfo (T313) holds a
 * whole car and its CarBox record (when that fits the 0x400 bytes): saved
 * when a sector begins (car_sector_begin, T290; ghost_record_start, T313),
 * put back to restart it (car_sector_restore, T290; ghost_restart, T313).
 *
 * Retail T306's .rodata (0x0033CC48..0x0033CC69; nothing references it:
 * DATA_START): "bad_alloc", t9UnitArena1Ui64. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"
// DATA_START 0x0033CC48

// FUNC 0x001EE638 DynamicsConductor::saveSector
void DynamicsConductor::saveSector(OneLapGhostInfo *rec, int car)
{
    RaceCarState *c = getCar(car);
    CarBox *box;

    car_copy(rec->car, c);
    memset(rec->aux, 0, sizeof(rec->aux));
    box = &GetCollisionProcessor()->box[car];
    if ((u32)car_box_save_size(box) <= sizeof(rec->aux))  /* unsigned compare */
        car_box_save(box, (CarBoxSave *)rec->aux);
}

// FUNC 0x001EE6D8 DynamicsConductor::restoreSector
/* The car comes back as saved, but keeps its noController / carIndex /
 * driver bytes; its lap restarts at the sector's start time.  The CarBox
 * record is loaded when any of its 0x400 bytes is non-zero. */
void DynamicsConductor::restoreSector(int car, OneLapGhostInfo *rec)
{
    RaceCarState *c = getCar(car);
    DynCarState *st = &c->st;
    u8 noController = st->noController;
    u8 carIndex = st->carIndex;
    s8 driver = st->driver;
    CarBoxList *list;
    u32 i;
    int sum;

    car_restore(c, rec->car);
    st->noController = noController;
    st->carIndex = carIndex;
    st->driver = driver;
    st->lapStartTime = (clock + rec->frac50) / 3;
    list = GetCollisionProcessor();
    boxes_reset_car(list, car);
    sum = 0;
    for (i = 0; i < sizeof(rec->aux); i++)
        sum += rec->aux[i];
    if (sum != 0)
        car_box_load(&list->box[car], (CarBoxSave *)rec->aux);
}
