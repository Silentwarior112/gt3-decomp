/* T331 (0x001DDFF0..0x001DE050): the timed per-car message: its kind in
 * DynCarState.msgKind, the frames left in msgTime (MSG_NO_TIMEOUT: none),
 * the durations in g_002D32F0. */
#include "game/car/physics/T331_dyn_car_message.h"
#include "game/car/physics/car_param_internal.h"

/* T331's .data (then its frame info at 0x002D3370): {kind, frames} of each
 * message kind; 0x78 a timed message, 0xFF one without a time-out. */
DynMsgEntry g_002D32F0[16] = {              /* 0x002D32F0 */
    { 0, 0 },
    { 1, 0 },
    { 2, 0 },
    { 3, 0 },
    { 4, 0 },
    { 5, 0 },
    { 6, 0x78 },
    { 7, 0x78 },
    { 8, 0x78 },
    { 9, 0x78 },
    { 10, 0 },
    { 11, 0 },
    { 12, 0x78 },
    { 13, MSG_NO_TIMEOUT },
    { 14, MSG_NO_TIMEOUT },
    { 15, 0 },
};

// FUNC 0x001DDFF0 DynCar_tickMessage
/* Counts msgTime down to 0 (MSG_NO_TIMEOUT stays). */
void DynCar_tickMessage(RaceCarState *car)
{
    DynCarState *st = &car->st;

    if (st->msgTime != 0 && st->msgTime != MSG_NO_TIMEOUT)
        st->msgTime--;
}

// FUNC 0x001DE018 DynCar_setMessage
/* A message with a duration replaces any message; one without (time 0)
 * only shows when no message is running. */
void DynCar_setMessage(RaceCarState *car, int kind)
{
    int time = g_002D32F0[kind].time;
    DynCarState *st = &car->st;

    if (time != 0 || st->msgTime == 0) {
        st->msgKind = kind;
        st->msgTime = time;
    }
}
