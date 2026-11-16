/* T363 (0x001F6578..0x001F6678): the computer driver's gear for its target
 * speed (the race display, u051, shows AIDriver.gear / gearSet). */
/* Retail T363 has no .rodata: of the 18 files from T318 to T376 (17
 * "bad_alloc" / t9UnitArena1Ui64 blocks between T317's and T397's), it is
 * the one without <new> and the race course (the only u086 file that does
 * not use the DynamicsConductor; ref/data/u086/notes.md). */
#include "game/race/ai/T363_ai_gear.h"
#include "game/race/ai/ai_internal.h"

// FUNC 0x001F6578 ai_clear_gear
void ai_clear_gear(AIDriver *ai)
{
    ai->gearSet = 0;
    ai->gear = -1;
}

/* Once the car is not slower than the speed it aims at, the gear for that
 * speed; braking (state 3 / 4) also sets gearSet. */
// FUNC 0x001F6588 ai_select_gear
void ai_select_gear(AIDriver *ai)
{
    RaceCarState *car = ai->car;
    DynCarState *st = &car->st;
    f32 target;

    ai_clear_gear(ai);
    switch (ai->state) {
    case AI_BRAKE:
        target = ai->nextSpeed;
        if (st->speedX < target)
            return;
        ai->gearSet = 1;
        break;
    case AI_BRAKE_CORNER:
        target = ai->nextSpeed < ai->segSpeed ? ai->nextSpeed : ai->segSpeed;
        if (st->speedX < target)
            return;
        ai->gearSet = 1;
        break;
    default:
        target = ai->nextSpeed;
        if (st->speedX < target)
            return;
        break;
    }
    ai->gear = car_gear_for_speed(car, target);
}

/* The braking point bookkeeping (its throttle factor is dropped), then the
 * gear (car_input_update, T342). */
// FUNC 0x001F6648 ai_update_gear
void ai_update_gear(AIDriver *ai)
{
    ai_brake_factor(ai);
    ai_select_gear(ai);
}
