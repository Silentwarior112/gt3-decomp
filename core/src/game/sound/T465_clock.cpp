/* T465 - the clock, ticked by the music thread every frame.
 * 0x0022BA48..0x0022BBF8.
 *
 * The time is the IOP clock as pdicdvd_get_time (T519) returns it: low word
 * the seconds since 2000-01-01 00:00, high word the stat byte. */
#include "game/sound/gtsound_internal.h"
#include "game/sound/T465_clock.h"

u64 g_clockTime = 0;                            /* 0x00352B80 this frame's pdicdvd_get_time */
static CdClock3 g_clockDatesNew;                /* 0x0049E460 computed by the music thread */
static CdClock3 g_clockDates;                   /* 0x0049E478 the copy the readers get */

/* Reads the time, computes the three dates and publishes them with
 * interrupts off. */
// FUNC 0x0022BA48 clock_update
void clock_update(void)
{
    int intr;

    g_clockTime = pdicdvd_get_time();
    pdicdvd_time_to_clock(&g_clockDatesNew.clock[0], g_clockTime);
    pdicdvd_time_to_clock_gmt(&g_clockDatesNew.clock[1], g_clockTime);
    pdicdvd_time_to_clock_local(&g_clockDatesNew.clock[2], g_clockTime);
    intr = intr_suspend();
    g_clockDates = g_clockDatesNew;
    intr_resume(intr);
}

// FUNC 0x0022BAF0 clock_stat
int clock_stat(void)
{
    return (int)(g_clockTime >> 32);
}

// FUNC 0x0022BB08 clock_seconds
int clock_seconds(void)
{
    return (int)(g_clockTime & 0xFFFFFFFF); /* lwu: the masked low word */
}

// FUNC 0x0022BB20 clock_get
void clock_get(CdClock *out)
{
    int intr = intr_suspend();

    *out = g_clockDates.clock[0];
    intr_resume(intr);
}

// FUNC 0x0022BB68 clock_get_gmt
void clock_get_gmt(CdClock *out)
{
    int intr = intr_suspend();

    *out = g_clockDates.clock[1];
    intr_resume(intr);
}

// FUNC 0x0022BBB0 clock_get_local
void clock_get_local(CdClock *out)
{
    int intr = intr_suspend();

    *out = g_clockDates.clock[2];
    intr_resume(intr);
}
