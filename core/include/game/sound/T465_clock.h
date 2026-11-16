/* T465_clock.h - the clock (translation unit T465, unit u104).
 *
 * Ticked by the music thread (T463) every frame: the IOP time
 * (pdicdvd_get_time, T519: low word the seconds since 2000-01-01 00:00,
 * high word the stat byte) and three sceCdCLOCK dates computed from it:
 * as read (pdicdvd_time_to_clock), GMT and local (pdicdvd_time_to_clock_gmt /
 * _local).  The readers get a copy published with interrupts off.
 */
#ifndef GAME_T465_CLOCK_H
#define GAME_T465_CLOCK_H

#include "game/game.h"

struct CdClock;                                 /* T519_pdicdvd.h (sceCdCLOCK's layout, 8 bytes) */

void clock_update(void);                        /* 0x0022BA48 */
/* The high word of the time: the stat byte. */
int  clock_stat(void);                          /* 0x0022BAF0 (clock_ticks_hi) */
/* The low word of the time: the seconds since 2000-01-01 00:00. */
int  clock_seconds(void);                       /* 0x0022BB08 */
/* The dates of this frame (read with interrupts off). */
void clock_get(struct CdClock *out);            /* 0x0022BB20 (F_0022BB20) as read */
void clock_get_gmt(struct CdClock *out);        /* 0x0022BB68 (F_0022BB68) */
void clock_get_local(struct CdClock *out);      /* 0x0022BBB0 (F_0022BBB0) */

#endif /* GAME_T465_CLOCK_H */
