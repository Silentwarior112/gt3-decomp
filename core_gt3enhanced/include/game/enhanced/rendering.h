/* enhanced/rendering.h - the rendering area's switches (car/model, effects,
 * course, T451, tex1): 1 = Lupo 2, 0 = GT3 (game/enhanced.h).  Survey:
 * ref/enhanced/survey/rendering.md (entry numbers below); port notes:
 * ref/enhanced/ported/rendering.md.
 *
 * Entry 1 (every car model's animations) has a switch because one GT3 car
 * model does carry a live animation: no GT3 car model has CLUT animation
 * records, but la0008 (cars/day, night and eve) has a model animation track
 * (kind 7, rate 1) on a morph position (command 0x34 of its part 0), which
 * Lupo 2's per-frame CarModel::animate would start cycling.  The CLUT
 * records (the Pod's face) run either way: GT3's car models have none. */
#ifndef GAME_ENHANCED_RENDERING_H
#define GAME_ENHANCED_RENDERING_H

#define ENH_RENDER_CAR_ANIMS      1   /* a car model's animation tracks step with its CLUT records every frame
                                         (CarModel::animate, L2 0x0022DAE8 -> modelset_step_anims); 0: only the
                                         CLUT records step, GT3's car models stay still (rendering.md 1) */
#define ENH_RENDER_TYRE_SHOULDER  1   /* the inside of an ordinary tyre also gets its shoulder strip
                                         (TirePacket::virtual_4) (rendering.md 9) */
#define ENH_RENDER_FLARE_NO_FOG   1   /* fog disabled while the flare buffers are set up (FlareClass::begin)
                                         (rendering.md 10) */
#define ENH_RENDER_SHOWROOM_TABLES 1  /* ShowRoomCar::SetupShowRoomCar builds the surface / slipstream tables
                                         first (surface_tables_init, L2 0x001D9934), so the menus' top speed
                                         is the race one even before the first race; GT3 computes it with the
                                         file's defaults until a race has run (rendering.md "For other areas",
                                         menus_b 18) */

#endif /* GAME_ENHANCED_RENDERING_H */
