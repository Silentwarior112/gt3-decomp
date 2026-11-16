/* T276_envmap_render.h - the dynamic environment map (translation unit T276,
 * unit u075).
 *
 * Called by RacePS2Base (virtual_8, u044: envmapInit; virtual_44 /
 * virtual_47, u045: envmapUpdate).  A DynamicEnvmapCamera
 * (T277) is put at the car the current view follows; the scene (the sky, then
 * the course parts visible from that place, VisionList) is drawn three times
 * into a 128x256 colour buffer (PSMCT32, with its own PSMZ24 Z buffer), each
 * time turned 120 degrees about z, and each picture is drawn into the target
 * buffer through the embedded "GTM1" model 0x00339540 (the map's shape),
 * turned the opposite way. */
#ifndef GAME_T276_ENVMAP_RENDER_H
#define GAME_T276_ENVMAP_RENDER_H

#include "game/game.h"

class GSBuffer;                             /* T514 (pointers only) */
class CourseData;                           /* T179 */
class CameraBase;                           /* T410 */
class CameraManager;                        /* T383 */
class DynamicEnvmapCamera;                  /* T277 */

/* Prepares (relocates) the embedded "GTM1" model. */
void envmapInit(void);                      /* 0x001D8260 */
/* Empty.  RacePS2Base::virtual_35 (T130) calls it first, with nothing in
 * $a0 of its own (? the counterpart of envmapInit, which RacePS2Base's
 * virtual_8 calls).  Lupo 2's matcher named it
 * DynamicsConductor::ReportTimeDifference, but that is T305's slot 0
 * (0x001ED988). */
void F_001D8280(void);                      /* 0x001D8280 */
/* The sky (turned by the course environment's angles), then the course: the
 * parts listed in parts ({n, part[n]}, a VisionPoint's), or all of it if
 * parts is NULL.  cam's slot 2 sets the view. */
void envmapDrawScene(CourseData *course, CameraBase *cam, u16 *parts);   /* 0x001D8288 */
/* Renders the map into target from cam's place.  vram < 0: the two work
 * buffers get VRAM of their own, else they are put at vram. */
void envmapRender(GSBuffer *target, CourseData *course, DynamicEnvmapCamera *cam, int vram); /* 0x001D83A8 */
/* envmapRender from the car cm's current view follows, at time t (the camera
 * is a local). */
void envmapUpdate(GSBuffer *target, CourseData *course, CameraManager *cm, int vram, f32 t); /* 0x001D8698 */

#endif /* GAME_T276_ENVMAP_RENDER_H */
