/* T331_dyn_car_message.h - a timed message per car (translation unit T331,
 * unit u077), 0x001DDFF0..0x001DE050.
 *
 * The kind is in DynCarState.msgKind, the frames left in msgTime (0xFF: no
 * time-out), the durations in g_002D32F0. */
#ifndef GAME_T331_DYN_CAR_MESSAGE_H
#define GAME_T331_DYN_CAR_MESSAGE_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only; u077 DynCar) */

/* One kind of message.  Not polymorphic.  Size 8. */
class DynMsgEntry {
public:
    int kind;                               /* 0x0 = its index */
    int time;                               /* 0x4 frames: 0 (only while no message runs), 0x78, 0xFF */
};
EGCS_SIZE(DynMsgEntry, 8);

/* The kinds (u083).  RaceDisplay::showWarning (T154) shows 1 and 2 as
 * "WRONG WAY" and 4, 5, 7..10, 12 as "AUTO DRIVE"; 0 clears; 11 (the pit
 * stop) shows nothing.  The names say when T307 / T350 set them. */
enum {
    CARMSG_NONE          = 0x0,
    CARMSG_WRONG_WAY     = 0x1,             /* backwards faster than 5 km/h */
    CARMSG_WRONG_WAY_PIT = 0x2,             /* moving backwards past the pit exit */
    CARMSG_AUTO_DRIVE    = 0x4,             /* driven by the computer (ctrlMode 1, the pit exit) */
    CARMSG_AUTO_DRIVE_TH = 0x5,             /* the same with st.fullThrottle set */
    CARMSG_PIT_TOO_FAST  = 0x7,             /* 100 km/h or more 25 m before the box: no stop */
    CARMSG_PIT_RACE_DONE = 0x8,             /* the car's race is done: no stop */
    CARMSG_PIT_NO_NEED   = 0x9,             /* nothing to service: no stop */
    CARMSG_PIT_LANE      = 0xA,             /* in the pit lane */
    CARMSG_PIT_STOP      = 0xB,             /* the pit stop is running */
    CARMSG_PIT_EXIT_BACK = 0xC,             /* backed out over the pit exit gate (-3) */
    /* The enhanced engine (ref/enhanced/INTERFACES.md): the Pod race's two
     * warnings, after GT3's table of 16 kinds (0..15), with Lupo 2's own
     * numbers (its table has 18 entries, L2 0x002DEC38).  Set by the Pod
     * stop-area rule (L2 0x001F7D08 / 0x001F7D70: 16, 0x001F7D28: 17);
     * shown by RaceDisplay::showWarning (hud.md C17: RACEDISPLAY 56 / 58).
     * g_002D32F0 has Lupo 2's 18 entries for them (untimed, as Lupo 2). */
    CARMSG_POD_STOP      = 0x10,            /* in the stop area: stop and pull the handbrake (amber) */
    CARMSG_POD_RETURN    = 0x11             /* past it: go back to the stop area (red) */
};

/* T331's .data: kinds 0..15 in GT3, up to its .eh_frame at 0x002D3370
 * (u077: 17 entries; a kind 16 read the CIE there as {0x10, 0}); the
 * enhanced engine's table has 18, 0x10 bytes more (the Pod warnings). */
extern DynMsgEntry g_002D32F0[18];          /* 0x002D32F0 (16 in GT3; the enhanced engine's Pod warnings) */

void DynCar_tickMessage(RaceCarState *car); /* 0x001DDFF0 msgTime counts down to 0 (0xFF stays) */
/* A message with a duration replaces any message; one without only shows
 * when no message is running. */
void DynCar_setMessage(RaceCarState *car, int kind);    /* 0x001DE018 */

#endif /* GAME_T331_DYN_CAR_MESSAGE_H */
