/* enhanced/hud.h - the HUD area's switches (race/display): 1 = Lupo 2,
 * 0 = GT3 (game/enhanced.h).  Survey: ref/enhanced/survey/hud.md (entry
 * numbers below); port notes: ref/enhanced/ported/hud.md.  Every new sprite
 * falls back to GT3's drawing when it is missing from race/<country>/
 * display.imgs, whatever the switch (race_display_find_new_image, T154).
 *
 * Not switched (they act only on data GT3 does not have, so GT3's content
 * is unchanged): C1 the Dualnote battery gauge (hybrid cars only), C17 /
 * C18 the Pod's warnings and "sat down", C20 the time trial display's
 * generalisation (the time trial behaves as before), C21 / C22 the Pod
 * displays (built by the race area's Pod races only), C19 the time step
 * (the races pass one frame), C26 the Pod texts. */
#ifndef GAME_ENHANCED_HUD_H
#define GAME_ENHANCED_HUD_H

#define ENH_HUD_DISPLAY_RPM            1   /* the tachometer's needle on the car's display rpm, a spring-damper
                                              after the engine rpm (RaceCarState.displayRpm, physics) (hud.md C2) */
#define ENH_HUD_TELEMETRY_CYCLE        1   /* the live items are RaceDisplay.showMask (race/core's SELECT cycles the
                                              accel / brake monitor and the steering display); either forces the
                                              remote meter layout, which now carries both; its boost meter 6 px
                                              higher; the mirror record subType 0 only (hud.md C3 + C4) */
#define ENH_HUD_AB_MONITOR             1   /* the accel / brake monitor redrawn: framed columns, the
                                              replay_sign_brake / _accel signs, bars trailing last frame's value
                                              (hud.md C5) */
#define ENH_HUD_STEERING_DISPLAY       1   /* the steering display redrawn: the replay_sign_gtforce wheel with a
                                              motion blur, ticks at -90 / 0 / +90, a red marker (hud.md C6) */
#define ENH_HUD_TACHO_DIALS            1   /* a 14,000 rpm dial (rev_14 / revb_14); the needle's full scale is the
                                              dial's (hud.md C8) */
#define ENH_HUD_SPEEDO_SCALE           1   /* the speedometer's full scale is the dial's (hud.md C9) */
#define ENH_HUD_NEEDLE_LOOK            0   /* needle full red, blur 1/6, the angle clamped (hud.md C10) */
#define ENH_HUD_MINIMAP_POINTER        1   /* one car_pointer sprite tinted per car on the mini map (hud.md C11) */
#define ENH_HUD_TEXT_LOOK              1   /* HUD text: drop shadow on the title and mode label, cached images,
                                              0x60 characters, grey 0x80C0C0C0, the course name's case, the
                                              re-cased labels shown under it, clear() hides (hud.md C12, C26) */
#define ENH_HUD_CAR_SIGN               1   /* the replay_sign_car icon left of the car name (hud.md C13) */
#define ENH_HUD_LOGO_LOOK              1   /* the logo watermark additive and half bright (gt_aspec_logo_mini by
                                              the fader's flag); the mini-logo flag gates logo and car name, the
                                              car-name flag shows it (hud.md C14) */
#define ENH_HUD_DEMO_CAR_NAME          1   /* a demo shows the car name only (hud.md C15) */
#define ENH_HUD_TIME_LIMIT_LOOK        1   /* the time limit at (16, 320), red below 10 s, GAME OVER red and
                                              centred; 3 s low only with race.h's ENH_RACE_TIME_UP_ENDS
                                              (hud.md C16) */
#define ENH_HUD_FREE_RUN_LIVE_MESSAGES 1   /* the free run's start messages only live, not in replays (hud.md C23) */
#define ENH_HUD_LICENSE_RESULT_TIMING  1   /* licence results: pass wait 5 frames, fail tail 47 (hud.md C24) */

#endif /* GAME_ENHANCED_HUD_H */
