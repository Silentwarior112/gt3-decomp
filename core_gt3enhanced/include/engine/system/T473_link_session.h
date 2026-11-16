/* T473_link_session.h - the i.LINK (IEEE 1394) session of the link battle (translation unit T473, unit u104).
 *
 * A thread (link_thread) runs exchanges through the pdi1394 driver (T541,
 * over the G003 RPC core): every node sends one 0x7C-byte frame and receives
 * the frames of all eight nodes.  Two semaphores hand the frames over to the
 * game (F_0022D0F0).  Byte 0x7B of a frame carries a command: 0x80 end
 * (F_0022DA10) or 0x81 pause (F_0022CD30; u047's RaceLinkBattle sends 0x80
 * at the race's end and from the pause menu's quit, 0x81 for the pause
 * button); one received switches the session to a command exchange.  A bus reset (the driver's callback) stops the session with
 * 0x80.  F_0022D3C0 exchanges the race selections and the game options
 * before a race.  The function names are the first pass's (F_ = no evidence
 * for a name); the frames stay in the unit (gtsound_internal.h).
 */
#ifndef GAME_T473_LINK_SESSION_H
#define GAME_T473_LINK_SESSION_H

#include "game/game.h"
#include "engine/pdi/G003_pdi1394_rpc.h"          /* Pdi1394Status */

class pdi1394;                              /* T541: the driver */
struct LinkPlayerInfo;                      /* T000: what a node sends in F_0022D3C0 (5 bytes) */
struct LinkFrameSet;                        /* u104: the 8 frames of an exchange (0x3E0 bytes) */

/* LinkStatus (u002, u104, u128) is the status block pdi1394_get_status
 * (0x0026EE38, G003) copies: guid[8] (by IOP node), nodeCount +0x40 (u002
 * count, u104 unk40), nodeMask +0x44, unk48 +0x48 (non-zero after a bus
 * reset: F_0022CCB0, link_bus_reset_cb's state 3).  G003 defines it as
 * Pdi1394Status (u128's layout); one type, so &st needs no cast.  0x4C
 * bytes: the 0x50 of u104 / u128 (pad_4C) is the callers' stack slot. */
typedef Pdi1394Status LinkStatus;
EGCS_SIZE(LinkStatus, 0x4C);

void  link_create(void);                    /* 0x0022CB58 the driver (g_00352CB8 = new pdi1394(0x20002)), the callback */
/* The driver's callback (pdi1394_set_callback: a Pdi1394Callback; fno is
 * not read): a bus reset. */
void  link_bus_reset_cb(void *arg, unsigned int fno);    /* 0x0022CBD8 */
void  link_enable(void);                    /* 0x0022CC38 */
void  link_disable(void);                   /* 0x0022CC48 */
int   F_0022CC50(void);                     /* 0x0022CC50 the session runs */
u8   *F_0022CC58(void);                     /* 0x0022CC58 the data of the frame to send (this node's) */
LinkFrameSet *F_0022CC68(void);             /* 0x0022CC68 the frames received */
u8   *F_0022CC78(int slot);                 /* 0x0022CC78 the data slot `slot' sent (by its IOP node), NULL if none */
int   F_0022CCB0(void);                     /* 0x0022CCB0 LinkStatus.unk48 != 0 */
int   F_0022CCD8(void);                     /* 0x0022CCD8 the game took a command exchange's frames: the thread goes on; 1 */
int   F_0022CD18(void);                     /* 0x0022CD18 a command exchange waits for the game */
/* The session state (g_linkState, u104's .sbss 0x00352CB4; F_0022CD28 returns
 * it, u047's RaceLinkBattle reads it). */
enum {
    LINK_STATE_RUN     = 0,                 /* the race exchanges run (or no session) */
    LINK_STATE_COMMAND = 1,                 /* a command exchange (a node asked for the pause) */
    LINK_STATE_RESET   = 2,                 /* a bus reset */
    LINK_STATE_RESET48 = 3                  /* a bus reset with LinkStatus.unk48 set (the link is still up) */
};
int   F_0022CD28(void);                     /* 0x0022CD28 the state: LINK_STATE_* */
void  F_0022CD30(void);                     /* 0x0022CD30 sends 0x81 (pause) */
int   F_0022CD40(void);                     /* 0x0022CD40 0x81 received */
void  link_put_command(void);               /* 0x0022CD50 */
void  link_take_commands(void);             /* 0x0022CD78 */
void  link_thread_step(void);               /* 0x0022CE70 */
int   F_0022D0F0(void);                     /* 0x0022D0F0 hands over the frames; 1 if new ones */
void  link_player_info_clear(LinkPlayerInfo *p);    /* 0x0022D308 bytes 0, 2, 3, 4 (byte 1 is kept) */
void  F_0022D320(void);                     /* 0x0022D320 link init (game init 0x00102C00) */
void  F_0022D360(void);                     /* 0x0022D360 resets the session's player data */
pdi1394 *F_0022D3B8(void);                  /* 0x0022D3B8 the driver */
/* Exchanges the race selections (the LinkPlayerInfo bytes car .. transmission,
 * a random byte, the course: *course 0xFF takes a node's), then the
 * serialised game options (into MenuStatus::options[slot], T427); 0 if the
 * exchange failed (the options buffers then leak). */
int   F_0022D3C0(int car, int carClass, int color, int tyre, int transmission, int *course);   /* 0x0022D3C0 */
LinkPlayerInfo *F_0022D7A0(int slot);       /* 0x0022D7A0 the slot's selections, NULL if it has no node */
int   F_0022D7D0(int slot);                 /* 0x0022D7D0 the slot's state is 1 (PDI1394_STATE_PLAY) */
int   F_0022D7F0(int slot);                 /* 0x0022D7F0 the slot's state is 2 (PDI1394_STATE_LIVE) */
int   F_0022D810(int slot);                 /* 0x0022D810 the slot's state is 1 or 2 (play or live) */
int   F_0022D830(void);                     /* 0x0022D830 own slot (pdi1394::getOwnSlot), -1 if not linked */
int   F_0022D850(void);                     /* 0x0022D850 the number of slots in state 1 (play) */
void  F_0022D870(void);                     /* 0x0022D870 starts the session (semaphores, frames, the thread) */
void  link_thread(void *stack);             /* 0x0022D948 */
void  F_0022D9A8(void);                     /* 0x0022D9A8 ends the session */
int   F_0022DA08(void);                     /* 0x0022DA08 the sum of the nodes' random bytes (F_0022D3C0) */
void  F_0022DA10(void);                     /* 0x0022DA10 sends 0x80 (end) */
int   F_0022DA20(void);                     /* 0x0022DA20 0x80 received */
int   F_0022DA30(void);                     /* 0x0022DA30 0x80 received */
/* The session words other units use (T473 .sdata, initially 0; the other
 * state words stay in u104). */
extern int g_00352CA8;                      /* 0x00352CA8 gp-0x4D48 this console's view mode (0: it drives
                                               a car; ArcadeControl::F_00115C80, T010) */
extern int g_00352CB0;                      /* 0x00352CB0 gp-0x4D40 "live" mode of the session (u047: ? the
                                               race is only watched) */
extern int g_00352CDC;                      /* 0x00352CDC gp-0x4D14 nonzero: a bus reset (the link
                                               dropped; link_bus_reset_cb sets it) */

/* 0x0022DA40 (.dtors 0x0028F250: ~Control of the link's Control) and
 * 0x0022DA68 (.ctors 0x0028F17C: the random seed, the link's Control) are
 * egcs's _GLOBAL_$D$ / _GLOBAL_$I$ of T473 (compiler-generated, no
 * declaration). */

#endif /* GAME_T473_LINK_SESSION_H */
