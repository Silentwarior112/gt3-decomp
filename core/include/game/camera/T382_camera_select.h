/* T382_camera_select.h - the camera direction helpers and the course camera
 * record pickers (translation unit T382, unit u091), 0x00207208..0x00207768.
 *
 * The pitch / yaw (degrees) of a direction, and the search of the course's
 * camera file (RaceCourse::F_0018FD20 lists the records over a course
 * position) for the record a course camera should take: at the target's
 * position, on a later lap, or for a car of the CarProgress (T408) that will
 * stay in it long enough.  Used by the CameraManager (T383, T409), the camera
 * modes and the race displays.
 */
#ifndef GAME_T382_CAMERA_SELECT_H
#define GAME_T382_CAMERA_SELECT_H

#include "game/game.h"

class CameraTargetInfo;                     /* T377 (pointers only) */
class CarProgress;                          /* T408 (no canonical header): the lap distance of every car */
class CourseCameraRecord;                   /* T172 */
class RaceOrganization;                     /* T125 */

/* -atan2(y, |(x, z)|) in degrees */
f32 camera_pitch_deg(f32 x, f32 y, f32 z);  /* 0x00207208 */
/* 180 - atan2(x, z) in degrees: a heading (y unused) */
f32 camera_yaw_deg(f32 x, f32 y, f32 z);    /* 0x00207248 */
/* 1: pos is between lo and hi of the way through rec (no callers outside T382) */
int camera_record_in_range(CourseCameraRecord *rec, f32 pos, f32 lo, f32 hi);    /* 0x00207278 */
/* The first record over the target's course position (distance) within its
 * first 60% (all of it when `wide'); NULL: none. */
CourseCameraRecord *camera_find_record(RaceOrganization *org, CameraTargetInfo *target, int wide);  /* 0x002072C0 */
/* The same at the target's lap position on the next lap, the lap after, ...
 * (the laps of the camera file, wrapping to 1); *lap = the lap it was found
 * on, -1: none. */
CourseCameraRecord *camera_find_next_record(RaceOrganization *org, CameraTargetInfo *target, int *lap); /* 0x00207370 */
/* As camera_find_next_record for car `car' of prog, skipping cur: a record
 * within its first 80% that the car does not leave within minTime seconds at
 * its speed and (with a cur) whose camera is 15 m or more from cur's; *lap =
 * the lap, 0: none. */
CourseCameraRecord *camera_find_next_record_for(RaceOrganization *org, CarProgress *prog, int car,
                                                CourseCameraRecord *cur, int *lap, f32 minTime); /* 0x002074A0 */

#endif /* GAME_T382_CAMERA_SELECT_H */
