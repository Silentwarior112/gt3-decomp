/* T179_course_data.h - CourseData, the loaded course file (translation unit
 * T179, unit u059), 0x001A8D18..0x001A9B90.
 *
 * A course is one file of the volume ("courses/<region>/<name>") read into
 * one 0xD00000-byte unit of the course arena (course_read; RaceCourse, T153,
 * keeps it as `data').  Its header is a table of offsets that course_data_relocate
 * turns into pointers (word 0 holds the address the offsets were made for),
 * relocating every sub-block with its own function: the models (ModelSet,
 * T451; SpriteModel, T454), the vision lists (VisionList, T453), the runway
 * (T457 / T181), the flare sources (T195), the enemy lines (T368), the
 * environment (CourseEnv, T178), the mini map (T164), the course cameras
 * (T172), the course sound (T458), the gadget shapes (T167) and the drive
 * parameters (T361).  The draw helpers pick the visible parts of a model
 * from the vision list point nearest to the camera.  T179 also keeps the
 * table of the courses of the volume (code -> file name, sorted by code).
 *
 * Names: u059 (the owner) where they say what a field is, else the users'
 * (u045 the vision lists, u075 the environment map's models, u050 / u090
 * the cameras, u085 the drive parameters).  The functions keep the owner's
 * F_ names (no unit found better ones). */
#ifndef GAME_T179_COURSE_DATA_H
#define GAME_T179_COURSE_DATA_H

#include "game/game.h"

class ModelSet;                             /* T451_model_set.h (pointers only) */
struct VisionList;                          /* T453_vision_list.h */
struct SpriteModel;                         /* T454_sprite_model.h */
struct Runway;                              /* T457_runway.h */
struct FlareSourceData;                     /* T195 (u055; no canonical header) */
struct EnemyLines;                          /* T368_enemy_lines.h */
class CourseEnv;                            /* T178_course_env.h */
struct RaceMiniMapData;                     /* T164 (u056; no canonical header) */
class CourseCamera;                         /* T172_course_camera.h (u092 / u093 CourseCameraData) */
struct CourseSound;                         /* T458_course_sound.h */
struct GadgetShape;                         /* T167_rigid_body.h (u059 CourseObj) */
struct CourseDriveParam;                    /* T361_drive_params.h */
struct Tex1Image;                           /* G001_flare.h */
class CameraManager;                        /* T383_camera_manager.h */
class FlareClass;                           /* T159_flare.h */

/* The gadget shape table of the course file (CourseData +0x90): word 0 is
 * the address the offsets were made for, then the offsets of the shapes,
 * themselves relative to the table; the first offset is also the end of
 * the table (the number of shapes is (obj[0] - 4) / 4).  RacePS2Base hands
 * &obj[0] to the rigid bodies (g_00352174, T130). */
struct CourseObjTable {
    u32          base;                      /* 0x00 */
    GadgetShape *obj[1];                    /* 0x04 [n] (NULL: none) */
};
EGCS_SIZE(CourseObjTable, 0x8);

/* Not polymorphic.  The header of the course file; its size is not known
 * (0xA4 bytes at least: the 41 words below). */
class CourseData {
public:
    u32               base;                 /* 0x00 the address the offsets were made for */
    /* the course: the parts the vision list point lists (course_draw) */
    ModelSet         *model04;              /* 0x04 animated */
    VisionList       *visionList;           /* 0x08 the main view's (u045; u059 vis08) */
    VisionList       *visionMirror;         /* 0x0C the rear-view mirror's (u045; vis0C) */
    VisionList       *visionSub;            /* 0x10 (u045; vis10) */
    /* the course as the dynamic environment map draws it (u075 model,
     * vision; u059 model14, vis18) */
    ModelSet         *envModel;             /* 0x14 animated */
    VisionList       *envVision;            /* 0x18 */
    /* the mirror's in the sub view, when the course has it */
    VisionList       *visionMirrorAlt;      /* 0x1C (u045; vis1C) */
    u8                unk_20[4];            /* 0x20 */
    /* the road reflection's models (u045): course_draw_reflection_a's model and vision
     * list (RacePS2Base::drawReflections, the reflection pass, runs when it is
     * set; the first reading, u045 hasShadow, was "the shadows") */
    ModelSet         *model24;              /* 0x24 animated */
    VisionList       *vis28;                /* 0x28 */
    u8                unk_2C[8];            /* 0x2C */
    /* the road reflection's too, course_draw_reflection_b's (u045 f34: drawn first in
     * drawReflections, after clearing the frame's alpha) */
    ModelSet         *model34;              /* 0x34 */
    VisionList       *vis38;                /* 0x38 */
    u8                unk_3C[0x18];         /* 0x3C */
    ModelSet         *model54;              /* 0x54 drawn whole first in the main view (u045 f54) */
    ModelSet         *model58;              /* 0x58 F_001A9240's, scaled by 0.01 (animated) */
    ModelSet         *envSky;               /* 0x5C the environment map's sky (u075 sky; model5C) */
    ModelSet         *model60;              /* 0x60 drawn whole first in the mirror view (u045 f60) */
    void             *unk_64;               /* 0x64 relocated only */
    /* RaceSmoke::setTexture (T173) gets it (u059 tex68, u044 unk68) */
    Tex1Image        *smokeTex;             /* 0x68 the tyre smoke's texture */
    ModelSet         *model6C;              /* 0x6C drawn whole after the course (course_draw) */
    Runway           *runway;               /* 0x70 (u043 table: its markers) */
    FlareSourceData  *flares;               /* 0x74 the flare sources (course_submit_flares) */
    /* drawn with the course's parts when course_draw's flags bit 0 is set */
    SpriteModel      *model78;              /* 0x78 animated */
    EnemyLines       *enemyLine;            /* 0x7C relocated without a NULL test */
    CourseEnv        *env;                  /* 0x80 */
    RaceMiniMapData  *miniMap;              /* 0x84 */
    /* replay and demo cameras (u092 cameras, u093 camera) */
    CourseCamera     *replay;               /* 0x88 the course cameras */
    CourseSound      *sound;                /* 0x8C */
    /* u044 / u050 unk90: the course has rigid bodies */
    CourseObjTable   *objs;                 /* 0x90 the gadget shapes */
    /* per drive class (getDriveParam, T361) */
    CourseDriveParam *driveParam[4];        /* 0x94 (u085; u059 p94, p98, p9C, pA0) */
};

/* Relocates a loaded course in place (RaceCourse::load, after
 * course_read). */
void course_data_relocate(CourseData *cd);                                /* 0x001A8D18 */
/* Empty (RaceCourse::unload). */
void F_001A9178(CourseData *cd);                                /* 0x001A9178 */
/* RacePS2Base::virtual_8: the animated models reset, then the course
 * sound located on the runway (course_sound_locate). */
void F_001A9180(CourseData *cd);                                /* 0x001A9180 */
/* Draws a whole model with the fog range (0, 1); cd is not used. */
void course_draw_model(CourseData *cd, ModelSet *m);                   /* 0x001A91D8 */
/* model58, scaled by 0.01 with the course fog at 0.01, after clearing the
 * frame (colour 0, write mask 0xFF000000). */
void F_001A9240(CourseData *cd);                                /* 0x001A9240 */
/* The course seen from the camera: the parts of model04 (and with flags
 * bit 0 of model78) listed by the point of v nearest to the camera, all of
 * them when v is NULL; then model6C. */
void course_draw(CourseData *cd, VisionList *v, CameraManager *cm, int flags);  /* 0x001A92E8 */
/* model24 through vis28 at (x, y, z). */
void course_draw_reflection_a(CourseData *cd, f32 x, f32 y, f32 z);           /* 0x001A94C8 */
/* model34 through vis38 at (x, y, z). */
void course_draw_reflection_b(CourseData *cd, f32 x, f32 y, f32 z);           /* 0x001A9578 */
/* Submits the flare source groups {n, group[n]} (all when NULL) to fc. */
void course_submit_flares(CourseData *cd, FlareClass *fc, const u16 *groups);    /* 0x001A9628 */
/* The animations stepped by 100, released, stepped by -v, released (the
 * second argument is not used; RaceOrganization::updateStartAndTimer). */
void F_001A96B8(CourseData *cd, void *unused, f32 v);           /* 0x001A96B8 */
/* Per frame: the animations of the animated models stepped (id a, time v;
 * modelset_step_anims / spriteset_step_clut_anims). */
void course_step_anims(CourseData *cd, int a, f32 v);                  /* 0x001A9768 */
/* The animated models' CLUT animations applied (RacePS2Base::virtual_3). */
void course_apply_clut_anims(CourseData *cd);                                /* 0x001A97E8 */
/* The region whose course folder the paths use (g_002C2AF0[region]:
 * "courses/US", "courses/JP", ...; set_course_region, T050). */
void course_set_region(int region);                                    /* 0x001A98C0 */
/* Builds the course table from the directory "courses/US" of the volume
 * (game_init, T007). */
void course_table_init(void);                                          /* 0x001A98C8 */
/* Reads course `code' (nameToCode of its name, T481) into block, 0xD00000
 * bytes at most (file_read_blocking from the volume, inflated if packed);
 * returns block. */
CourseData *course_read(u64 code, void *block);                  /* 0x001A9AD8 */

#endif /* GAME_T179_COURSE_DATA_H */
