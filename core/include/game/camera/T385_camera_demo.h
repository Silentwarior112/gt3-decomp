/* T385_camera_demo.h - the course demo cameras of CameraManager (translation
 * unit T385, unit u092), 0x0020A9C0..0x0020AB80.
 *
 * View 0's designer camera steps through the course's demo camera records
 * (CourseCamera +0x30, `records2'), one every 240 frames, with a cross-fade
 * (2 s) at each change.  The two functions are CameraManager methods,
 * declared in T383_camera_manager.h: startDemoCameras (0x0020A9C0) and
 * updateDemoCameras (0x0020AA90).
 *
 * CourseCameraData (u092, u093) is CourseCamera (T172, the course's camera
 * file; CourseData +0x88): the same offsets - u092's count24 / rec28 /
 * count2C / rec30 are its count / records / count2 / records2 (+0x24 ..
 * +0x30), u093's positions its records (CameraPosition = CourseCameraRecord,
 * 0x80 bytes).  One class, so this is a typedef.
 */
#ifndef GAME_T385_CAMERA_DEMO_H
#define GAME_T385_CAMERA_DEMO_H

#include "game/game.h"

class CourseCamera;                         /* T172 (the definition) */

typedef CourseCamera CourseCameraData;

#endif /* GAME_T385_CAMERA_DEMO_H */
