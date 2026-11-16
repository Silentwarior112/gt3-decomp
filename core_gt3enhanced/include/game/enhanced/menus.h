/* enhanced/menus.h - the menus area's switches (menu, app, common, save,
 * data except T482, the display-offset file T512): 1 = Lupo 2, 0 = GT3
 * (game/enhanced.h); every one defaults to 1.  The first four are PLAN.md's
 * optional items.  Surveys: ref/enhanced/survey/menus_a.md, menus_b.md,
 * backend.md (entry numbers below); ref/enhanced/ported/menus.md.
 *
 * The ports that act only on data GT3 does not have (a CVT gear row, the
 * Pod race, the disqualification event, a "gear_max_speed" / "multi_assist"
 * / "screen_adjust" layout item, non-zero screen-adjust bytes) have no
 * switch: with GT3's data and layouts they do nothing
 * (ref/enhanced/ported/menus.md).  The switches below guard the ports that
 * change what GT3's own content does (PLAN.md rule 3). */
#ifndef GAME_ENHANCED_MENUS_H
#define GAME_ENHANCED_MENUS_H

#define ENH_MENU_ATTRACT_REPLAYS   0   /* one attract demo per idle; GCF demo kind 1000 plays a replay file
                                          (menus_a.md 4) */
#define ENH_MENU_BUSY_MARK         0   /* GT Concept's spinning oval instead of the blinking white square during loading (menus_a.md 11) */
#define ENH_MENU_REMEMBER_CURSOR   1   /* the top menu's page and cursor kept across races, in memory only
                                          (menus_a.md 15) */
#define ENH_MENU_WIDE_FROM_CONSOLE 1   /* wide screen from the console's 16:9 setting at boot and in the option
                                          defaults (menus_b.md 10, backend.md B7) */

/* Ports that change GT3's own behaviour (PLAN.md rule 3). */
#define ENH_MENU_ARCADE_DIRT_TYRES 1   /* the settings refuse a tyre change in an arcade event ("A...") on a
                                          dirt course, as GT3 does in GT mode's dirt events (menus_b.md 5) */
#define ENH_MENU_WHEEL_CALIBRATION 1   /* reset controller options give the GT Force and C294 wheels measured
                                          pedal ranges, marked valid (menus_b.md 11, backend.md B4) */
#define ENH_MENU_RESULT_LAP_GAPS   1   /* laps-only result boards: the leader shows laps, the cars on its lap
                                          their time gap (menus_b.md 14 b) */
#define ENH_MENU_LINK_RESULT_END   1   /* the i.LINK battle result ends when all cars but one have finished,
                                          not at the first goal (menus_b.md 15) */
#define ENH_MENU_FREE_RUN_ASSISTS  0   /* free run: the options' ASM / TCS for every car, not only the arcade
                                          cars (menus_a.md 8) */
#define ENH_MENU_TEXT_WIDTH        1   /* Item::setText measures centred texts (menus_a.md 31) */
#define ENH_MENU_MOVIE_WAIT        1   /* MenuBgMovie::start waits for the first decoded frame (menus_a.md 32,
                                          backend.md B8) */

#endif /* GAME_ENHANCED_MENUS_H */
