/* T179_course_data.cpp - CourseData, the loaded course file (T179), u059.
 *
 * A course is one file of the volume (courses/<region>/<name>) read into a
 * 13 MB unit of the course arena (course_read).  Its header is a table of
 * offsets that course_data_relocate turns into pointers (word 0 holds the address the
 * offsets were made for), relocating every sub-block with its own function.
 * The draw helpers pick the visible parts of the models from the vision
 * list point nearest to the camera.  The table of courses (code -> file
 * name) is built once from the "courses/US" directory of the volume and
 * sorted by code for a binary search.
 *
 * Data (retail): .data 0x002C2AF0 (the region folders, then the frame
 * info), .sdata 0x00352230..0x00352242 (the course table's three words,
 * course_path's "/"), .lit4 0x0034FF44; .rodata 0x00330A98..0x00330B38:
 * <new>'s "bad_alloc", the region folders' literals, "courses/.list", then
 * the type names "14RaceCameraBase", "11RaceMiniMap", "t9UnitArena1Ui64"
 * (UnitArena<64> instantiated first, then RaceMiniMap, then RaceCameraBase
 * defined; none of the other race display classes).
 */
// DATA_START 0x00330A98
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (T153 suppresses it by default) */
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's first .rodata string) */
#include "libc.h"                           /* strcpy, strcat */
#include "game/race/core/T153_race_course.h"          /* UnitArena<64> (no use) */
#include "game/race/display/T164_race_minimap.h"         /* RaceMiniMap (no use: the mini map record's class) */
#include "game/camera/T383_camera_manager.h"       /* CameraManager (course_draw): RaceCameraBase */
#include "game/course/T179_course_data.h"          /* this translation unit */
#include "game/course/T178_course_env.h"           /* CourseEnv, F_001A80E0, course_fog_apply */
#include "engine/gfx/T451_model_set.h"            /* ModelSet, modelset_* */
#include "game/course/T453_vision_list.h"          /* VisionList, VisionPoint, vision_relocate / nearest */
#include "engine/gfx/T454_sprite_model.h"         /* SpriteModel, spriteset_* */
#include "game/course/T457_runway.h"               /* runway_relocate */
#include "game/sound/T458_course_sound.h"         /* course_sound_relocate / locate */
#include "game/car/physics/T167_rigid_body.h"           /* GadgetShape, g_002C04B8 */
#include "game/camera/T172_course_camera.h"        /* CourseCamera::relocate */
#include "game/race/core/T131_race_ps2_render.h"      /* Matrix44 */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "engine/io/T500_vol_toc.h"              /* VolToc, VolDir, vol_toc_lookup */
#include "engine/io/T503_vol_thread.h"           /* vol_device_object, file_read_blocking */
#include "engine/pdistd/T532_heap.h"                 /* malloc, realloc */
#include "engine/pgl/G001_flare.h"                /* gfx_*, gs_*, pgl_*, vu0_*, tex1_relocate, PGL_* */
#include "runtime/newlib/G007_stdio.h"                /* qsort */
#include "game/sound/car_sound_internal.h"                           /* FlareSourceData, RaceMiniMapData, CourseListEntry, the course table */

/* p moved by delta bytes. */
template <class T> static inline T *reloc(T *p, int delta)
{
    return (T *)((u8 *)p + delta);
}

/* F_001A9240's scale of model58 (and of its fog). */
#define COURSE_SKY_SCALE        0.01f
/* course_draw's flags: bit 0 draws model78 (the SpriteModel) too. */
#define COURSE_DRAW_SPRITES     0x1
/* The TOC records (T500): a directory record's entry offsets start at
 * +0xC; a record's word 0 holds its name's offset in the low 24 bits. */
#define VOL_DIR_ENTRIES         0xC
#define VOL_NAME_MASK           0xFFFFFF

/* *p moved by delta bytes. */
template <class T> static inline void reloc_at(T **p, int delta)
{
    *p = (T *)((u8 *)*p + delta);
}

/* One relocation step of course_data_relocate: a statement macro (retail
 * has the loop notes' barriers between the steps: the header word is stored
 * before the first test, and the first step's end label is aligned; the
 * later plain `if's have no aligned labels). */
#define COURSE_RELOC(field, call)                                            \
    do {                                                                     \
        if (cd->field != 0) {                                                \
            cd->field = reloc(cd->field, delta);                             \
            call;                                                            \
        }                                                                    \
    } while (0)

// FUNC 0x001A8D18 course_data_relocate
//   Relocates a loaded course (RaceCourse::load, after course_read).
//   The gadget table is relocated in a do / while (0) block with a break
//   (its loop-depth weight gives the table s1 and delta s2, and its exit
//   label is not aligned, as retail's); the shapes through reloc_at (the
//   slot's address computed afresh, as retail's n * 4 + table + 4).
void course_data_relocate(CourseData *cd)
{
    u32 builtAt = cd->base;
    cd->base = (u32)cd;
    int delta = (int)((u32)cd - builtAt);

    COURSE_RELOC(model04, modelset_relocate(cd->model04));
    COURSE_RELOC(visionList, vision_relocate(cd->visionList));
    COURSE_RELOC(visionMirror, vision_relocate(cd->visionMirror));
    COURSE_RELOC(visionSub, vision_relocate(cd->visionSub));
    COURSE_RELOC(visionMirrorAlt, vision_relocate(cd->visionMirrorAlt));
    COURSE_RELOC(envModel, modelset_relocate(cd->envModel));
    COURSE_RELOC(envVision, vision_relocate(cd->envVision));
    COURSE_RELOC(model24, modelset_relocate(cd->model24));
    COURSE_RELOC(vis28, vision_relocate(cd->vis28));
    COURSE_RELOC(model34, modelset_relocate(cd->model34));
    COURSE_RELOC(vis38, vision_relocate(cd->vis38));
    COURSE_RELOC(model54, modelset_relocate(cd->model54));
    COURSE_RELOC(model58, modelset_relocate(cd->model58));
    COURSE_RELOC(envSky, modelset_relocate(cd->envSky));
    COURSE_RELOC(model60, modelset_relocate(cd->model60));
    COURSE_RELOC(model6C, modelset_relocate(cd->model6C));
    COURSE_RELOC(runway, runway_relocate(cd->runway));
    COURSE_RELOC(flares, cd->flares->relocate());
    COURSE_RELOC(model78, spriteset_relocate(cd->model78));
    cd->enemyLine = reloc(cd->enemyLine, delta);    /* QUIRK: not null-checked */
    COURSE_RELOC(env, F_001A80E0(cd->env));
    COURSE_RELOC(miniMap, cd->miniMap->relocate());
    COURSE_RELOC(replay, cd->replay->relocate());
    COURSE_RELOC(sound, course_sound_relocate(cd->sound));

    /* The gadget shapes: offsets relative to the table's own word 0; each
     * shape's model is an offset from the shape.  The first shape is
     * copied to g_002C04B8 (T167's template). */
    do {
        if (cd->objs == 0)
            break;
        cd->objs = reloc(cd->objs, delta);
        CourseObjTable *table = cd->objs;
        int n = 0;
        u32 end = (u32)table->obj[0];       /* the first offset: the end of the table */
        int shapeDelta = (int)((u32)table - table->base);
        for (; (u32)&table->obj[n] - (u32)table < end; n++) {
            if (table->obj[n] != 0) {
                reloc_at(&table->obj[n], shapeDelta);
                GadgetShape *shape = table->obj[n];
                reloc_at(&shape->model, (int)(u32)shape);
                modelset_relocate(shape->model);
            }
        }
        if (n > 0)
            g_002C04B8 = *table->obj[0];
    } while (0);

    if (cd->driveParam[0] != 0)
        cd->driveParam[0] = reloc(cd->driveParam[0], delta);
    if (cd->driveParam[1] != 0)
        cd->driveParam[1] = reloc(cd->driveParam[1], delta);
    if (cd->driveParam[2] != 0)
        cd->driveParam[2] = reloc(cd->driveParam[2], delta);
    if (cd->driveParam[3] != 0)
        cd->driveParam[3] = reloc(cd->driveParam[3], delta);
    if (cd->smokeTex != 0) {
        cd->smokeTex = reloc(cd->smokeTex, delta);
        tex1_relocate(cd->smokeTex);
    }
    if (cd->unk_64 != 0)
        cd->unk_64 = reloc((u8 *)cd->unk_64, delta);
}

// FUNC 0x001A9178 F_001A9178
//   Empty (called by RaceCourse::unload).
void F_001A9178(CourseData *cd)
{
}

// FUNC 0x001A9180 F_001A9180
//   RacePS2Base::virtual_8: the animations reset, then course_sound_locate
//   (the course sound's lap sections on the runway).
void F_001A9180(CourseData *cd)
{
    course_reset_anims(cd);
    if (cd->sound != 0)
        course_sound_locate(cd->sound, cd->runway);
}

// FUNC 0x001A91B8 course_draw_model54
//   Unreferenced: course_draw_model with model54.
void course_draw_model54(CourseData *cd)
{
    course_draw_model(cd, cd->model54);
}

// FUNC 0x001A91D8 course_draw_model
//   Draws a whole model with the fog range (0, 1) (`cd' is unused); the fog
//   pushed and popped (PGL_ATTR_FOG), the translation of the pushed matrix
//   reset (vu0_reset_translation: drawn around the eye).
void course_draw_model(CourseData *cd, ModelSet *model)
{
    if (model != 0) {
        pgl_push_attrib(PGL_ATTR_FOG);
        gfx_set_fog_params(0.0f, 1.0f);
        pgl_push_matrix();
        vu0_reset_translation();
        modelset_draw_all(model);
        pgl_pop_matrix();
        pgl_pop_attrib();
    }
}

/* The course fog at a scale (an inline wrapper, as the original's: its
 * argument goes straight to $f12, so egcs does not keep 0.01 in a saved
 * register for vu0_scale; retail loads the constant twice). */
static inline void env_fog(CourseEnv *env, f32 scale)
{
    course_fog_apply(&env->fog, scale);
}

// FUNC 0x001A9240 F_001A9240
//   Draws model58 scaled by 0.01 after clearing the frame (colour 0, only
//   the alpha byte written: write mask 0xFF000000), with the course fog at
//   scale 0.01.  The matrix: the current one kept, the identity scaled,
//   then multiplied by the kept one.
void F_001A9240(CourseData *cd)
{
    Matrix44 saved;
    pgl_push_attrib(PGL_ATTR_FOG);
    env_fog(cd->env, COURSE_SKY_SCALE);     /* implicit: course_fog_apply (inline env_fog) */
    gfx_set_frame_write_mask(0xFF000000);
    gs_set_clear_color(0);
    gs_clear_buffers(PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH);
    gfx_set_frame_write_mask(0xFFFFFFFF);
    pgl_push_matrix();
    pgl_get_matrix(&saved.m[0][0]);
    vu0_load_identity_regs();
    vu0_scale(COURSE_SKY_SCALE, COURSE_SKY_SCALE, COURSE_SKY_SCALE);
    pgl_mult_matrix(&saved.m[0][0]);
    modelset_draw_all(cd->model58);
    pgl_pop_matrix();
    pgl_pop_attrib();
}

// FUNC 0x001A92E8 course_draw
//   Draws the course from the camera position: the parts listed by the
//   point of vision list `v' nearest to the camera (all of them when v is
//   NULL) to course_draw_parts.
void course_draw(CourseData *cd, VisionList *v, CameraManager *cm, int flags)
{
    f32 x = cm->getPositionX();
    f32 y = cm->getPositionY();
    f32 z = cm->getPositionZ();
    const u16 *parts;
    if (v == 0)
        parts = 0;
    else
        parts = vision_nearest(v, x, y, z)->parts;
    course_draw_parts(cd, parts, flags);
}

// FUNC 0x001A93B0 course_draw_parts
//   With a part list {n, part[n]}: draws those parts of model04 (and, with
//   flags bit 0, of model78); without: all of model04 / model78 (the second
//   not NULL-tested).  Then all of model6C.
void course_draw_parts(CourseData *cd, const u16 *parts, int flags)
{
    if (parts != 0) {
        int n = *parts++;
        modelset_begin_draw(cd->model04);
        if (n != 0) {
            const u16 *p = parts;
            for (int k = n; k != 0; k--) {
                modelset_run_part(cd->model04, *p);
                p++;
            }
        }
        modelset_end_draw(cd->model04);
        if (cd->model78 != 0 && (flags & COURSE_DRAW_SPRITES)) {
            spriteset_begin_draw(cd->model78);
            if (n != 0) {
                const u16 *p = parts;
                for (int k = n; k != 0; k--) {
                    spriteset_draw_part(cd->model78, *p);
                    p++;
                }
            }
            spriteset_end_draw(cd->model78);
        }
    } else {
        modelset_draw_all(cd->model04);
        spriteset_draw_all(cd->model78);
    }
    if (cd->model6C != 0)
        modelset_draw_all(cd->model6C);
}

// FUNC 0x001A94C8 course_draw_reflection_a
//   The road reflection's model24 through vision list vis28 at x, y, z
//   (RacePS2Base::drawReflections).
void course_draw_reflection_a(CourseData *cd, f32 x, f32 y, f32 z)
{
    const u16 *parts;
    if (cd->vis28 == 0)
        parts = 0;
    else
        parts = vision_nearest(cd->vis28, x, y, z)->parts;
    if (parts != 0) {
        int n = *parts++;
        modelset_begin_draw(cd->model24);
        if (n != 0) {
            const u16 *p = parts;
            for (int k = n; k != 0; k--) {
                modelset_run_part(cd->model24, *p);
                p++;
            }
        }
        modelset_end_draw(cd->model24);
        return;
    }
    modelset_draw_all(cd->model24);
}

// FUNC 0x001A9578 course_draw_reflection_b
//   The same with model34 / vis38 (RacePS2Base::drawReflections).
void course_draw_reflection_b(CourseData *cd, f32 x, f32 y, f32 z)
{
    const u16 *parts;
    if (cd->vis38 == 0)
        parts = 0;
    else
        parts = vision_nearest(cd->vis38, x, y, z)->parts;
    if (parts != 0) {
        int n = *parts++;
        modelset_begin_draw(cd->model34);
        if (n != 0) {
            const u16 *p = parts;
            for (int k = n; k != 0; k--) {
                modelset_run_part(cd->model34, *p);
                p++;
            }
        }
        modelset_end_draw(cd->model34);
        return;
    }
    modelset_draw_all(cd->model34);
}


// FUNC 0x001A9628 course_submit_flares
//   Submits the flare source groups listed in {n, group[n]} (all when the
//   list is NULL) to the FlareClass (RacePS2Base::addCourseLights).
void course_submit_flares(CourseData *cd, FlareClass *fc, const u16 *groups)
{
    FlareSourceData *flares = cd->flares;
    if (groups != 0) {
        int n = *groups++;
        if (n != 0) {
            const u16 *p = groups;
            for (int k = n; k != 0; k--) {
                flares->submit(fc, *p);
                p++;
            }
        }
    } else {
        flares->submit(fc, -1);
    }
}

// FUNC 0x001A96B8 F_001A96B8
//   Called by RaceOrganization::updateStartAndTimer (the second argument is
//   ignored): steps the animations by 100, then by -v, each followed by
//   course_release_clut_anims(cd, 1).
void F_001A96B8(CourseData *cd, void *unused, f32 v)
{
    course_step_anims(cd, 1, 100.0f);
    course_release_clut_anims(cd, 1);
    course_step_anims(cd, 1, -v);
    course_release_clut_anims(cd, 1);
}

// FUNC 0x001A9720 course_reset_anims
//   The animated models' animations reset (modelset_reset_anims /
//   spriteset_reset_clut_anims).
void course_reset_anims(CourseData *cd)
{
    modelset_reset_anims(cd->model04);
    modelset_reset_anims(cd->model58);
    modelset_reset_anims(cd->envModel);
    modelset_reset_anims(cd->model24);
    spriteset_reset_clut_anims(cd->model78);
}

// FUNC 0x001A9768 course_step_anims
//   Per frame (RacePS2Base::virtual_28): the animated models' animations
//   stepped (animation id, time).  The callees take the time before the id
//   (T451 / T454): retail sets $f12 before $a1 for each call.
void course_step_anims(CourseData *cd, int a, f32 v)
{
    modelset_step_anims(cd->model04, v, a);
    modelset_step_anims(cd->model58, v, a);
    modelset_step_anims(cd->envModel, v, a);
    modelset_step_anims(cd->model24, v, a);
    spriteset_step_clut_anims(cd->model78, v, a);
}

// FUNC 0x001A97E8 course_apply_clut_anims
//   RacePS2Base::virtual_3: the animated models' CLUT animations applied.
void course_apply_clut_anims(CourseData *cd)
{
    modelset_apply_clut_anims(cd->model04);
    modelset_apply_clut_anims(cd->model58);
    modelset_apply_clut_anims(cd->envModel);
    modelset_apply_clut_anims(cd->model24);
    spriteset_apply_clut_anims(cd->model78);
}

// FUNC 0x001A9830 course_release_clut_anims
void course_release_clut_anims(CourseData *cd, int id)
{
    modelset_release_clut_anims(cd->model04, id);
    modelset_release_clut_anims(cd->model58, id);
    modelset_release_clut_anims(cd->envModel, id);
    modelset_release_clut_anims(cd->model24, id);
    spriteset_release_clut_anims(cd->model78, id);
}

/* ------------------------------------------------------------------------ */
/* The course table                                                         */
/* ------------------------------------------------------------------------ */

/* The table (.sdata, -G8: explicit initialisers, so written here, before
 * course_path's "/"; uninitialised globals would come after it). */
int              g_00352230 = 0;            /* 0x00352230 the number of courses */
CourseListEntry *g_00352234 = 0;            /* 0x00352234 the table, sorted by code */
int              g_00352238 = 0;            /* 0x00352238 the region (index into g_002C2AF0) */
/* The region folders (.data; their literals .rodata, written just before
 * the table, "courses/US" once: course_table_init's COURSE_DIR is the same
 * literal). */
const char *g_002C2AF0[5] = {               /* 0x002C2AF0 */
    "courses/US", "courses/JP", "courses/US", "courses/UK", "courses/EU",
};
/* Nothing references it (no code of the image loads 0x00330AE8): a list
 * file of the course folder, kept as data at its place (after the folders'
 * literals, before the type names: an extern const is written where it is
 * defined).  Its use in the original is not known. */
extern const char g_00330AE8[];
const char g_00330AE8[] = "courses/.list";  /* 0x00330AE8 */

// FUNC 0x001A9898 course_code_compare
//   The qsort comparison of CourseListEntry (unsigned 64-bit codes).
int course_code_compare(const void *a, const void *b)
{
    u64 x = *(const u64 *)a;
    u64 y = *(const u64 *)b;
    if (x < y)
        return -1;
    return y < x;
}

// FUNC 0x001A98C0 course_set_region
//   The region whose course folder course_path uses (called by
//   set_course_region, T050).
void course_set_region(int region)
{
    g_00352238 = region;
}

/* A word of a TOC image (retail adds the record offset to the image
 * last: image + (off + 4), image + (off + i * 4 + 0xC)). */
static inline u32 toc_word(const u8 *image, u32 off)
{
    return *(const u32 *)(image + off);
}

// FUNC 0x001A98C8 course_table_init
//   Builds the course table from the "courses/US" directory of the volume
//   (its record: word 1 = entries + 1, entry record offsets from word 3; an
//   entry record's low 24 bits locate its name, T500) and sorts it by code
//   (called by game_init, T007).
void course_table_init(void)
{
    VolDir dir;
    int i = 0;
    VolDevice *dev = &vol_device_object;    /* retail adds the toc offset to the
                                               address in a register */
    vol_toc_lookup(&dir, &dev->toc, COURSE_DIR);
    u8 *image = dir.toc->data;
    int n = toc_word(image, dir.off + 4) - 1;
    g_00352230 = n;
    g_00352234 = (CourseListEntry *)realloc(g_00352234, n * sizeof(CourseListEntry));
    for (; i < g_00352230; i++) {
        VolDir entry;                       /* a dead copy on the stack (sp+0x10) */
        u8 *data = dir.toc->data;
        CourseListEntry *ent = &g_00352234[i];
        entry.toc = dir.toc;
        entry.off = toc_word(data, dir.off + i * 4 + VOL_DIR_ENTRIES);
        const char *name = (const char *)(data + (toc_word(data, entry.off) & VOL_NAME_MASK));
        ent->name = name;
        ent->code = nameToCode(name);
    }
    qsort(g_00352234, g_00352230, sizeof(CourseListEntry), course_code_compare);
}

// FUNC 0x001A99D8 course_find
//   Binary search of the course table; the index of `code', 0 when absent.
int course_find(u64 code)
{
    int lo = 0;
    int hi = g_00352230;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        CourseListEntry *e = &g_00352234[mid];
        if (code == e->code)
            return mid;
        if (code < e->code)
            hi = mid;
        else
            lo = mid + 1;
    }
    return 0;
}

// FUNC 0x001A9A30 course_file_name
//   The file name of course `code' (the first course's when absent).
const char *course_file_name(u64 code)
{
    int i = course_find(code);
    return g_00352234[i].name;
}

// FUNC 0x001A9A60 course_path
//   buf = "<region folder>/<name>"; returns buf.
char *course_path(u64 code, char *buf)
{
    strcpy(buf, g_002C2AF0[g_00352238]);
    strcat(buf, "/");
    strcat(buf, course_file_name(code));
    return buf;
}

// FUNC 0x001A9AD8 course_read
//   Reads course `code' from the volume into `block' (at most 0xD00000
//   bytes, inflated if packed: unpack 1); called by RaceCourse::load.
CourseData *course_read(u64 code, void *block)
{
    char path[0x40];
    return (CourseData *)file_read_blocking(course_path(code, path), block,
                                            COURSE_FILE_MAX, 1, &vol_device_object);
}

// FUNC 0x001A9B20 course_list_names
//   Unreferenced: *names = a malloc'ed array of the course names; returns
//   their number.
int course_list_names(const char ***names)
{
    const char **v = (const char **)malloc(g_00352230 * sizeof(const char *));
    for (int i = 0; i < g_00352230; i++)
        v[i] = g_00352234[i].name;
    *names = v;
    return g_00352230;
}

/* .sdata 0x00352244: an initialised word (4) nothing in the program reads,
 * between T179's "/" and T184's pool; which of T179..T184 defined it the
 * bytes cannot tell (ref/data/u061/notes.md).  Defined here, after the last
 * function, so that the image's word comes from the C. */
int g_00352244 = 4;                         /* 0x00352244 unreferenced */
