/* T143_race_free_run_display.h - RaceFreeRunDisplay, the race display of the
 * free run (translation unit T143, unit u048), 0x00189998..0x00189AE0.
 *
 * RaceDisplay (T154) with the two session messages of the free run (race
 * messages 0x10 "timing starts at the next control line" and 0x11 "START
 * ends the session", on the message board) and no finish display.  Held by
 * value in RaceFreeRun (+0x2204C). */
#ifndef GAME_T143_RACE_FREE_RUN_DISPLAY_H
#define GAME_T143_RACE_FREE_RUN_DISPLAY_H

#include "game/game.h"
#include "game/race/display/T154_race_display.h"             /* base RaceDisplay */

class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* (__si_type_info 0x004B2040, base RaceDisplay 0x004B21D0) */
extern "C" const type_info &__tf18RaceFreeRunDisplay(void);    /* 0x00189A90 */
#endif

/* vtable 0x0032BDD8 (22 slots; 18, 19 and 21 its own), typeinfo
 * __tf18RaceFreeRunDisplay 0x00189A90.  No data of its own: vptr +0x00
 * (RaceDisplay's); 0xA47C bytes (RaceFreeRun+0x2204C .. +0x2C4C8). */
class RaceFreeRunDisplay : public RaceDisplay {
public:
    RaceFreeRunDisplay();                       /* 0x00189998 */
    /* RaceDisplay's, then race messages 0x10 and 0x11 on the message board
     * in 0x802BCC49 (at (3.0, 2.0) and (3.0, 0.0)).  (u048: virtual_18;
     * RaceDisplay's slot 18 is put_start_display, u052.) */
    virtual void put_start_display();           /* [18] +0x9C 0x001899D0 */
    virtual void put_finish_display();          /* [19] +0xA4 0x00189A60 nothing: a free run has no finish */
    /* Inline in the original: RaceFreeRun's destructor (T142) has it
     * expanded (the vptr store, then ~RaceDisplay); 0x00189A68 is the
     * out-of-line copy egcs emitted with the vtable. */
    virtual ~RaceFreeRunDisplay() {}            /* [21] +0xB4 0x00189A68 */
};
/* The enhanced engine: RaceDisplay's size (GT3 0xA47C; the flags and the
 * hud's Lupo 2 parts, T154_race_display.h). */
EGCS_SIZE(RaceFreeRunDisplay, 0xA8E8);

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* The __si_type_info {vptr, name, base} __tf18RaceFreeRunDisplay fills
 * (u048 g_004B2040). */
extern "C" u32 __ti18RaceFreeRunDisplay[3];     /* 0x004B2040 */
#endif

#endif /* GAME_T143_RACE_FREE_RUN_DISPLAY_H */
