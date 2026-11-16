/* enhanced/cameras_sound.h - the cameras and sound area's switches
 * (camera, sound): 1 = Lupo 2, 0 = GT3 (game/enhanced.h).  Survey:
 * ref/enhanced/survey/cameras_sound.md (entry numbers below). */
#ifndef GAME_ENHANCED_CAMERAS_SOUND_H
#define GAME_ENHANCED_CAMERAS_SOUND_H

#define ENH_CAMERA_TV_AIM          1   /* the TV cameras' pitch damper aims with 0.14 (GT3 0.4) (cameras_sound.md C1) */
#define ENH_CAMERA_LOD_BIAS        1   /* texture LOD bias + 0 (GT3 - ln 2): coarser mip levels (cameras_sound.md C2) */
#define ENH_CAMERA_DEMO_VIEW_MODE  1   /* demo replays: 75 % traditional / 25 % avantgarde, traditional when the
                                          replay BGM is off (cameras_sound.md C4) */
#define ENH_CAMERA_ZOOM_BUTTON     1   /* Press Triangle to zoom during a replay. (cameras_sound.md C5) */
#define ENH_CAMERA_START_SEQUENCE  0   /* view mode 0 starts on the starting camera, cross-fades at 1 / 2 / 3 s,
                                          half-speed sweep (cameras_sound.md C6) */
#define ENH_CAMERA_OCCLUSION_BOX   1   /* the in-car camera's occlusion box 1.5 m tall (GT3 1.0) (cameras_sound.md C7) */
#define ENH_CAMERA_STEREO_EYES     1   /* the i.LINK screen wall's eye distance 0.063 m (GT3 0.2) (cameras_sound.md C8) */
#define ENH_SOUND_ENGINE_LOAD      1   /* engine voice levels around the engine's real load share, exhaust curve
                                          0.4 + 0.6 (1 - (1 - x)^16) (cameras_sound.md S2) */
#define ENH_SOUND_MIX_TABLE        1   /* gains per listening mode: in-car engine 2.8, exhaust 2.3 / 3.0 ...
                                          (cameras_sound.md S3) */
#define ENH_SOUND_TARMAC_SQUEAL    1   /* tarmac squeal from the longitudinal slip speed, new pitch curves
                                          (cameras_sound.md S4) */
#define ENH_SOUND_SQUEAL_DETUNE    1   /* right-hand wheels' squeal onset at 0.75 pitch for one frame
                                          (cameras_sound.md S5) */
#define ENH_SOUND_WHINE_REVERSE    1   /* the gear whine's torque share taken as |share|: the reverse gear
                                          (negative ratio) whines as the forward gears do (L2 0x001DE3E4;
                                          found while porting S6, the survey has it as unchanged) */

/* Not switches: the Pod's voice (S1: needs a Pod and sound/gt3pod.ins), the
 * gearbox type 3 and Dualnote whines and the type-3 engine silence (S6:
 * need GearCalc.gearboxType 3 / RaceCarState.hybrid) do nothing with GT3's
 * cars and files.  Ported notes: ref/enhanced/ported/cameras_sound.md. */

#endif /* GAME_ENHANCED_CAMERAS_SOUND_H */
