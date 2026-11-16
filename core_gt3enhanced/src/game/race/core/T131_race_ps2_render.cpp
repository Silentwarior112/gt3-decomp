/* T131 - RacePS2Base, the drawing of a race view (u045).
 *
 * render (GameObject slot 2) runs drawView (the 3D view), virtual_45 (the
 * 2D overlays, empty here: RaceBasic draws them) and virtual_46 (the error
 * view).  drawView draws a full-screen view; drawSubView a view into a
 * sub-rectangle of the target (the split screen).  Both
 *   - render the dynamic environment map of the scene (envmapUpdate),
 *   - draw the course (the vision lists of CourseData), the reflections on
 *     the road (drawReflections: drawCarReflections / drawLampReflections into a small buffer, blurred and
 *     blended over the screen), the cars (drawCars, farthest first), the
 *     rear-view mirror (drawMirror), the tyre smoke and the lens flare
 *     (FlareClass at +0x9830, lights from addCarLamps / addCourseLights),
 *   - copy the result into the target;
 * drawView also does the replay depth-of-field blur (ReplayBlur at
 * +0x97CC), the rain, the colour correction, the vignette, the afterimage
 * (captureAfterimage / drawAfterimage: the frame saved into three VRAM
 * planes and drawn over the next frames with a fading alpha) and the
 * camera's frame effect (the previous frame mixed in).
 *
 * Includes (u045's data pass): the classes retail's T131 saw, in its order.
 * Its .rodata (0x003280C8..0x003282E8) is "bad_alloc" (<new>), the error
 * view's message, then the type names in reverse definition order:
 * OperatorList, List / OperatorHolder, OperatorHolder, Cell / OperatorHolder,
 * RaceCameraBase, the GameStatus's garage lists, UnitArena<64>, and Operator
 * (the non-polymorphic base, after all the others) - T130's set without
 * T130's own three classes, whose key functions are there.  So UnitArena<64>
 * is instantiated first (T153), the GameStatus (T429) comes before RaceBase's
 * RaceCameraBase, and the menu operator classes (T216) after T130's header.
 * No race display (T154 / T157: neither their names nor the "" of
 * RaceMessageDisplay::clear in .sdata are in retail's T131).
 */
// DATA_START 0x003280C8
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* (no use: "bad_alloc", the first .rodata string) */
#include "libgraph.h"                       /* SCE_GS_PSM* */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::data */
#include "game/save/T429_game_status.h"          /* (no use: the garage classes retail's T131 saw) */
#include "game/camera/T383_race_camera_base.h"     /* (RaceCameraBase, through RaceBase) */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base (render, slots 44..55, captureAfterimage), MirrorView;
                                               + T121 RaceEntryBase / RaceEntryCar, T129 RaceBase, T159
                                               FlareClass, T173 RaceSmoke, T165 ParticleManager, T167
                                               RigidBodyManager, T182 ReplayBlur, T185 ErrorEventView */
#include "game/menu/framework/T216_menu_transitive.h"      /* (no use: the menu operator classes retail's T131 saw) */
#include "game/race/core/T131_race_ps2_render.h"      /* own: Matrix44, race_car_in_view, clear_frame_alpha / write_alpha_depth / F_00180EB0 */
#include "game/app/T002_render_thread.h"        /* GameObject */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "engine/gfx/T512_render_context.h"       /* RenderContext g_002F4230, GSBufferPair */
#include "game/camera/T383_camera_manager.h"       /* CameraManager, CameraView (CameraSet, T388) */
#include "game/camera/T410_camera_base.h"          /* CameraBase */
#include "game/effects/T277_dynamic_envmap_camera.h"    /* DynamicEnvmapCamera::followCar */
#include "game/effects/T276_envmap_render.h"        /* envmapUpdate */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (draw wrappers), RaceEntryBase::sortByDistance,
                                               F_00179460 */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, course, loggerMode, eventQueue) */
#include "game/course/T179_course_data.h"          /* CourseData, course_draw_model .. course_submit_flares */
#include "game/course/T178_course_env.h"           /* CourseEnv, CourseFog, course_env_apply .. course_env_set_reflection */
#include "game/course/T457_runway.h"               /* Runway::hasLights */
#include "game/car/model/T180_race_car_model.h"       /* RaceCarModel::getCarModel */
#include "game/car/model/T446_car_model.h"            /* CarModel::callback (lod, lodDist) */
#include "game/car/model/T181_car_lighting.h"         /* CarRenderCtx */
#include "game/effects/T183_race_vignette.h"        /* race_draw_vignette */
#include "game/car/model/T184_car_icon_maker.h"       /* GsTexPacket */
#include "game/car/physics/T289_car_mode.h"             /* car_display_level, CAR_SHOW_* */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_view_x / y / z */
#include "game/sound/T288_car_sound.h"            /* DynCarState::noController (RaceCarState::st) */
#include "game/car/model/T449_race_tire_model.h"      /* brake_textures_link / _unlink */
#include "engine/gfx/T451_model_set.h"            /* modelset_set_* */
#include "game/course/T453_vision_list.h"          /* VisionList, VisionPoint, vision_nearest */
#include "engine/text/T496_font.h"                 /* Font g_002F1968 */
#include "engine/pgl/G001_flare.h"                /* gfx_*, gs_*, pgl_*, vu0_*, pfx_*, weather_draw, box_visible */
#include "game/race/core/race_ps2_render_internal.h"                           /* the unit's constants */
#include "game/enhanced.h"                  /* ENH_RACE_AFTERIMAGE */

/* GSBufferPair's current / other buffer.  Retail takes the pair's address
 * (g_002F4230+0xAC4 / +0xA50, kept in a register) and reads cur at +0x70
 * from it: the form of an inline accessor (the original's were most likely
 * GSBufferPair members).  Local here, not in T512_render_context.h: an
 * inline function in a widely included header shifts egcs's label numbers,
 * which changes other units' .gcc_except_table (u008's notes; u054's T157
 * gets four more regions). */
static inline GSBuffer *pair_current(GSBufferPair *p) { return &p->buf[p->cur]; }
static inline GSBuffer *pair_other(GSBufferPair *p) { return &p->buf[1 - p->cur]; }

/* The render context's fields as retail reads them: through &g_002F4230 in
 * a register (lui / addiu, then lw 0x18(reg) ...), the form of inline
 * accessors taking the context (u021's cars_projection has the same wide
 * test).  Local for the reason above. */
static inline int rc_field_mode(RenderContext *rc) { return rc->unk18; }
static inline f32 rc_field_blend(RenderContext *rc) { return rc->unk14; }
/* The y scale of every race view's projection, x * 0.75 on a 16:9 screen. */
static inline void proj_y_scale(RenderContext *rc)
{
    F_00257EF0(PROJ_Y_SCALE);
    if (rc->wide != 0)
        vu0_scale(0.75f, 1.0f, 1.0f);
}

/* min.s / max.s: egcs's <? / >? operators (T131 has no -ffast-math, under
 * which a ternary would give them); GCC 14 has none. */
#ifdef BOOT_OLD_GCC
#define MINF(a, b) ((a) <? (b))
#define MAXF(a, b) ((a) >? (b))
#else
#define MINF(a, b) ((a) < (b) ? (a) : (b))
#define MAXF(a, b) ((a) > (b) ? (a) : (b))
#endif

// FUNC 0x0017E6C0 RacePS2Base::render
//   GameObject::render: while renderEnabled, the 3D view, the 2D overlays and
//   the error view.
void RacePS2Base::render(GSBuffer &buf)
{
    if (renderEnabled != 0) {
        drawView(&buf);
        virtual_45(&buf);
        virtual_46(&buf);
    }
}

static inline void buf_set_psm(GSBuffer *b, int psm) { b->psm = psm; }
static inline void blur_set_fnumber(ReplayBlur *b, f32 f) { b->fNumber = f; }
static inline void blur_set_focus(ReplayBlur *b, f32 f) { b->focus = f; }
// FUNC 0x0017E740 RacePS2Base::drawView
//   Draws the full-screen race view into buf.  screen: a colour buffer the
//   size of the render context's current Z buffer (g_002F4230.unkAC4, which
//   is the depth buffer of the view); envmap: the 256 x 256 dynamic
//   environment map.
//   Matching (99.1%): left is the frame effect's min.s, which retail
//   computes straight into $f12 (ours: min.s $f0 then mov.s $f12; the min
//   is tied to the call result).  Tried: <? with the call / a variable /
//   swapped operands / an inline min helper / a float-first prototype of
//   gfx_frame_effect; the reference compiler gives the same.
void RacePS2Base::drawView(GSBuffer *buf)
{
    CourseData *course = org->course->data;
    CourseEnv *env = course->env;
    CameraManager *cm = virtual_6();

    GSBuffer screen(pair_current(&g_002F4230.unkAC4), SCE_GS_PSMCT32);   /* sp+0x00 */
    screen.setField(rc_field_mode(&g_002F4230) != 0 ? PGL_FIELD_FRAME_SHIFT : PGL_FIELD_FRAME);
    modelset_set_tex_lod_size(env->texLodSize);
    GSBuffer envmap(SCE_GS_PSMCT16, 0x100, 0x100);                         /* sp+0x40 */
    envmapUpdate(&envmap, course, cm, screen.addr, 0.0f);

    /* the projection and the view */
    screen.setDrawBuffer();
    pair_current(&g_002F4230.unkAC4)->setZBuffer();
    gs_set_clear_color(0);
    gs_clear_buffers(PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    cm->setFarClip(env->farClip);
    cm->applyProjection();
    proj_y_scale(&g_002F4230);             /* implicit: F_00257EF0, vu0_scale (the helper's) */
    modelset_set_projection_scale(1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();
    cm->applyView();

    /* the course */
    course_env_apply(env);
    course_draw_model(course, course->model54);
    course_env_draw_sun_disc(env, &flare);
    F_001A9240(course);
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    write_alpha_depth(&screen);
    drawCourse();
    if (course->model24 != NULL)                    /* the course has road reflections */
        drawReflections(&screen, &envmap);
    if (org->loggerMode == 0) {
        RigidBodyManager::SetLightVector((const Vec3 *)env->lightB[0].pos);
        rigidBodies.draw();
    }
    clear_frame_alpha(0x80000000);

    /* the cars: LOD by distance, the tyres' motion blur */
    drawCars(&envmap, -1, 1, 0.0f);

    /* the rear-view mirror: live race, a camera that shows it, the centre
     * screen of an i.LINK wall (isCenterScreen) */
    if (replayMode == RACE_LIVE) {
        if (cm->isDrawBackMirror() != 0) {
            if (cm->isCenterScreen() != 0) {
                setMirrorRect(320, 52, 240, 64, 1.3f);
                drawMirror(&screen, 0);
            }
        }
    }

    /* the lens flare: the sun, the lamps, the course's lights; the sparks */
    flare.setNoOcclusionTest(0);
    flare.begin(screen);
    course_env_set_sun(env, &flare);
    if (env->flags & ENV_LAMPS)
        addCarLamps(0.0f);
    if (env->flags & ENV_COURSE_LIGHTS)
        addCourseLights();
    flare.flush();
    particles.draw();

    /* into the target; depth: a copy of the view's Z buffer like buf */
    GSBuffer depth(buf, SCE_GS_PSMZ24);                                     /* sp+0x80 */
    depth.blit(pair_current(&g_002F4230.unkAC4), PGL_FILTER_NEAREST, PGL_FILTER_NEAREST);
    if (replayMode != RACE_LIVE && (((const u8 *)&env->flags)[1] & 1)) {    /* lbu +0x11: ENV_REPLAY_BLUR */
        buf->setDrawBuffer();
        gfx_enable(PGL_CAP_DEPTH_TEST);
        gfx_disable(PGL_CAP_BLEND);
        /* implicit: GSBuffer::copy (the inline copy constructor) */
        GSBuffer work(g_002F4230.bufB38);   /* sp+0xC0 the blur's levels, in the 640 x 224 Z buffer's VRAM */
        buf_set_psm(&work, SCE_GS_PSMCT24);
        blur.setFilm(env->blurFilmDiag, 1.333f);
        blur_set_fnumber(&blur, env->blurFNumber);
        f32 targetDist = cm->virtual_10();  /* the camera's distance to its target */
        blur_set_focus(&blur, (targetDist < env->blurFocusMin) ? env->blurFocusMin : targetDist);
        blur.setFromProjection();
        blur.draw(buf, &screen, &depth, &work, rc_field_mode(&g_002F4230), rc_field_blend(&g_002F4230));
    } else {
        gfx_disable(PGL_CAP_DEPTH_TEST);
        buf->blitField(&screen, rc_field_mode(&g_002F4230), rc_field_blend(&g_002F4230));
    }
    screen.freeVram();
    buf->setDrawBuffer();
    depth.setZBuffer();

    /* the rain, around the eye (no translation) */
    pgl_push_matrix();
    vu0_reset_translation();
    int effects = cm->virtual_11();
    if ((env->flags & ENV_WEATHER) && (effects & CAM_EFFECT_WEATHER)) {
        gfx_set_packet_access(PGL_PKT_UNCACHED_ACCEL);
        weather_draw(weather, buf->addr);   /* addr: not read by weather_draw */
        gfx_set_packet_access(PGL_PKT_CACHED);
    }
    pgl_pop_matrix();
    smoke.render(&depth, 0);
    flare.draw();

    /* the colour correction: in replays when the camera overrides the
     * course's levels or colour, else when the course has it (lbu +0x10,
     * >> 7: ENV_COLOUR_CORRECT) */
    if (replayMode == RACE_LIVE ? (env->flags >> 7) & 1 : cm->F_0020A038()) {
        f32 cc[6];                          /* sp+0xC0 lo, hi, r, g, b, saturation */
        cm->getColourCorrection(env, cc);
        GSBuffer work(SCE_GS_PSMCT32, 0x80, 0x20);                          /* sp+0x100 the filter's work VRAM */
        buf->setDrawBuffer();
        buf->setTexture();
        pgl_color(0x808080);
        pfx_color_correction(cc[2], cc[3], cc[4], cc[0], cc[1], cc[5], work.addr);
    }
    gfx_disable(PGL_CAP_DEPTH_TEST);
    if (replayMode != RACE_LIVE && (env->flags & ENV_VIGNETTE))
        race_draw_vignette(buf, env);
    buf->setDrawBuffer();
    g_002F1968.setStyle(NULL);
    cm->begin2D(org, buf);                  /* not read */

    /* the afterimage: captured on the first frame (0.5 s) and whenever the
     * camera cross-fades */
    if (frameBits == 0) {
        afterimageTime = 0.5f;
        captureAfterimage(buf);
    } else if (cm->crossFade != 0) {
        afterimageTime = cm->crossFadeTime;
        captureAfterimage(buf);
    }
    drawAfterimage(buf);

    /* the camera's frame effect: buf (target B) = lerp(buf, the other frame
     * buffer (the draw target A), level), the colour only */
    if (cm->frameEffect != 0) {
        buf->setTargetB();
        pair_other(&g_002F4230.frame)->setDrawBuffer();
        gfx_set_frame_write_mask(0x00FFFFFF);
        gfx_depth_mask(0);
        f32 level = cm->F_00209498();
        gfx_frame_effect(PGL_FX_MIX_A_INTO_B, MINF(level, 0.8f));
        gfx_set_frame_write_mask(0xFFFFFFFF);
        gfx_depth_mask(1);
    }
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_disable(PGL_CAP_BLEND);
    frameBits = (u8)((frameBits << 1) | 1);
}

// FUNC 0x0017EFC8 RacePS2Base::virtual_45
//   Empty: the 2D overlays of the derived modes (RaceBasic::virtual_45).
void RacePS2Base::virtual_45(GSBuffer *buf)
{
}

// FUNC 0x0017EFD0 RacePS2Base::virtual_46
//   Posts the error event when the course has no vision list, then draws the
//   error view.
void RacePS2Base::virtual_46(GSBuffer *buf)
{
    RaceOrganization *o = org;
    if (o->course->data->visionList == NULL)
        o->eventQueue.post(RACE_EVENT_ERROR, (int)(size_t)"VisionList is invalid or empty", 0);
    errorView.draw(buf);
}

// FUNC 0x0017F040 RacePS2Base::drawSubView
//   Draws the race view into the rectangle of buf (rectW x rectH; the split
//   screen views): a colour buffer of its own, the depth in the 640 x 224 Z
//   buffer's VRAM.  sub != 0: the second view (half the far clip, the
//   course's sub vision list, a blank environment map if the course has
//   ENV_SUB_NO_ENVMAP, no rigid bodies, the mirror with argument 1).  zoom
//   scales the projection.  No sparks, rain or screen effects.
//   Matching (99.6%): left is the order of three loads before
//   depth.setField (retail rectW, rectH, then rc->unk18).  Tried the six
//   orders of the depth set-up, a set-up helper, the field mode in a
//   variable.
void RacePS2Base::drawSubView(GSBuffer *buf, int sub, f32 zoom)
{
    CourseData *course = org->course->data;
    CourseEnv *env = course->env;
    CameraManager *cm = virtual_6();

    GSBuffer screen;                                                        /* sp+0x00 */
    screen.psm = SCE_GS_PSMCT32;
    screen.width = buf->rectW;
    screen.height = buf->rectH;
    screen.setField(rc_field_mode(&g_002F4230) != 0 ? PGL_FIELD_FRAME_SHIFT : PGL_FIELD_FRAME);
    screen.allocVram();
    GSBuffer depth;                                                         /* sp+0x40 */
    buf_set_psm(&depth, SCE_GS_PSMZ24);
    depth.width = buf->rectW;
    depth.height = buf->rectH;
    depth.setField(rc_field_mode(&g_002F4230) != 0 ? PGL_FIELD_FRAME_SHIFT : PGL_FIELD_FRAME);
    depth.setAddr(g_002F4230.bufB38.addr);
    modelset_set_tex_lod_size(env->texLodSize);
    GSBuffer envmap(SCE_GS_PSMCT16, 0x100, 0x100);                         /* sp+0x80 */
    if (sub == 0 || !(env->flags & ENV_SUB_NO_ENVMAP)) {
        envmapUpdate(&envmap, course, cm, screen.addr, 0.0f);
    } else {
        envmap.setDrawBuffer();
        gs_set_clear_color(0x80000000);
        gs_clear_buffers(PGL_CLEAR_COLOR);
    }

    /* the projection (the rectangle's aspect, zoom) and the view */
    screen.setDrawBuffer();
    depth.setZBuffer();
    gs_set_clear_color(0);
    gs_clear_buffers(PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    cm->setFarClip(sub != 0 ? env->farClip * 0.5f : env->farClip);
    cm->applyProjection();
    proj_y_scale(&g_002F4230);             /* implicit: F_00257EF0, vu0_scale (the helper's) */
    f32 aspect;
    if (buf->hasRect != 0)
        aspect = (f32)buf->rectH / (f32)buf->rectW;
    else
        aspect = (f32)buf->displayHeight() / (f32)buf->width;
    vu0_scale(aspect * PROJ_ASPECT * zoom, zoom, 1.0f);
    modelset_set_projection_scale(1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();
    cm->applyView();

    /* the course */
    course_env_apply(env);
    course_draw_model(course, course->model54);
    course_env_draw_sun_disc(env, &flare);
    F_001A9240(course);
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    write_alpha_depth(&screen);
    modelset_set_tex_lod(1);
    modelset_set_tex_set(sub != 0 ? 2 : -1);
    if (sub != 0)
        course_draw(course, course->visionSub, cm, course->env->visionSub);
    else
        course_draw(course, course->visionList, cm, course->env->visionMain);
    modelset_set_tex_lod(0);
    if (course->model24 != NULL)                    /* the course has road reflections */
        drawReflections(&screen, &envmap);
    clear_frame_alpha(0x80000000);

    /* the cars: the second view with LOD by distance, the first with LOD 0;
     * no tyre blur */
    drawCars(&envmap, sub != 0 ? -1 : 0, 0, 0.0f);
    if (sub == 0 && org->loggerMode == 0) {
        RigidBodyManager::SetLightVector((const Vec3 *)env->lightB[0].pos);
        rigidBodies.draw();
    }

    /* the rear-view mirror (the rectangle is the one drawView set) */
    if (replayMode == RACE_LIVE) {
        if (cm->isDrawBackMirror() != 0) {
            if (cm->isCenterScreen() != 0)
                drawMirror(&screen, 1);
        }
    }

    /* the lens flare */
    flare.setNoOcclusionTest(1);
    flare.begin(screen);
    course_env_set_sun(env, &flare);
    if (env->flags & ENV_LAMPS) {
        if (!(env->flags & ENV_SPLIT_NO_LIGHTS))
            addCarLamps(0.0f);
    }
    if (env->flags & ENV_COURSE_LIGHTS) {
        if (!(env->flags & ENV_SPLIT_NO_LIGHTS))
            addCourseLights();
    }
    flare.flush();
    smoke.render(&depth, 1);
    flare.draw();

    gfx_disable(PGL_CAP_DEPTH_TEST);
    buf->blitField(&screen, rc_field_mode(&g_002F4230), rc_field_blend(&g_002F4230));
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_DEPTH_TEST);
}

// FUNC 0x0017F610 RacePS2Base::drawCourse
//   The course through the main vision list (texture sets by LOD).
void RacePS2Base::drawCourse()
{
    CourseData *course = org->course->data;
    CameraManager *cm = virtual_6();
    modelset_set_tex_lod(1);
    modelset_set_tex_set(-1);
    course_draw(course, course->visionList, cm, course->env->visionMain);
    modelset_set_tex_lod(0);
}

// FUNC 0x0017F688 race_car_in_view
//   Whether the 8 m box around the car's interpolated position (t: the
//   interpolation argument of car_view_x / y / z) is in view.
int race_car_in_view(RaceCarState *car, f32 t)
{
    f32 x = car_view_x(car, t);
    f32 y = car_view_y(car, t);
    f32 z = car_view_z(car, t);
    f32 lo[4];                                                              /* sp+0x00 */
    f32 hi[4];                                                              /* sp+0x10 */
    lo[0] = x - 4.0f;
    lo[1] = y - 4.0f;
    lo[2] = z - 4.0f;
    hi[0] = x + 4.0f;
    hi[1] = y + 4.0f;
    hi[2] = z + 4.0f;
    return box_visible(lo, hi) != 0;
}

static inline void model_set_lod_dist(RaceCarModel *m, f32 d) { F_00179460(&m->carModel)->callback.lodDist = d; }
// FUNC 0x0017F730 RacePS2Base::drawCars
//   Draws the cars, farthest first (sortByDistance from the eye, then
//   sortEnd .. sortPrev): the ones whose model is enabled, that the camera
//   lets through (F_00209370), shown at least as ghosts (a ghost with the
//   scan mask) and in view.  lod: the CarModel LOD for all of them (-1: by
//   distance); lod > 0 draws the wheels (drawTireCylinders), else the tyres with
//   motion blur (drawTires: for the camera's target tyreBlur != 0, for the
//   others 2 when tyreBlur is set).  The camera's target gets half the
//   course's LOD distance.  envBuf: the scene's dynamic environment map
//   (its own VRAM copy when the runway has lights: the glows are drawn
//   into it).  Per car: its lights (CarRenderCtx), the shadow passes
//   (drawShadow per light), the body, the wheels or tyres.
//   RaceEntryCar::drawTires takes (blur, scale, t, lod): retail loads lod
//   last, in the cross-jumped tail of the two calls (u042 revisit).
void RacePS2Base::drawCars(GSBuffer *envBuf, int lod, int tyreBlur, f32 t)
{
    modelset_set_cull_far(1);
    brake_textures_link();
    CourseData *course = org->course->data;
    CourseEnv *env = course->env;
    CameraManager *cm = virtual_6();
    RaceEntryBase *entries = org->entries;
    f32 eyeX = cm->getPositionX();
    f32 eyeY = cm->getPositionY();
    f32 eyeZ = cm->getPositionZ();
    entries->sortByDistance(eyeX, eyeY, eyeZ);

    GsTexPacket envRegs;                                                    /* sp+0x00 */
    GSBuffer envTex;                                                        /* sp+0x50 */
    envTex.copy(envBuf);
    if (course->runway->hasLights != 0)
        envTex.allocVram();
    envTex.makeTexRegs(&envRegs.regs);
    modelset_set_env_tex(&envRegs.regs);

    RaceEntryCar *target = cm->views[cm->current].targetInfo.car[0];
    for (RaceEntryCar *car = entries->sortEnd; car != NULL; car = car->sortPrev) {
        if (car->modelEnabled == 0)
            continue;
        RaceCarState *state = car->state;
        int show = car_display_level(state);
        if (cm->F_00209370(org, car) == 0)
            continue;
        if (show < CAR_SHOW_GHOST)
            continue;
        if (!race_car_in_view(state, t))
            continue;
        if (show == CAR_SHOW_GHOST) {
            car->F_00177878();
            gs_set_scanmask_mode(PGL_SCANMASK_OTHER_FIELD);
        }
        F_00179460(car->model.getCarModel())->callback.lod = lod;
        if (car == target)
            model_set_lod_dist(&car->model, env->carLodDist * 0.5f);
        else
            model_set_lod_dist(&car->model, env->carLodDist);

        CarRenderCtx lights;                                                /* sp+0x90 */
        lights.setup(course, state, 0.0f);
        if (lights.lit != 0) {
            DynamicEnvmapCamera cam;                                        /* sp+0x2E0 */
            cam.followCar(state, 0.0f);
            lights.drawGlows(&envTex, envBuf, &cam, car == target);
            /* implicit: ~CameraBase (cam's implicit destructor) */
        }   /* cam's implicit destructor: CameraBase::~CameraBase (flags 2) */
        for (int i = 0; i < lights.passes; i++) {
            lights.applyShadowPass(i);
            car->drawShadow(t);
        }
        if (F_001A8618(env) != 0)
            car->F_001778C0(t);
        lights.applyDefault();
        car->F_001778E8(t);
        lights.apply();
        car->drawBody(t);
        if (lod > 0) {
            car->drawTireCylinders(env->brakeGlowScale, t);
        } else if (car == target) {
            car->drawTires(tyreBlur != 0, env->brakeGlowScale, t, lod);
        } else {
            car->drawTires(tyreBlur != 0 ? 2 : 0, env->brakeGlowScale, t, lod);
        }
        car->F_00177988(0.0f);
        gs_set_scanmask_mode(PGL_SCANMASK_OFF);
    }
    modelset_set_env_tex(NULL);
    modelset_set_cull_far(0);
    brake_textures_unlink();
}

// FUNC 0x0017FB68 RacePS2Base::drawReflections
//   The reflections on the road (CourseEnv's shadow* fields, T178 names):
//   model34 first into the screen's alpha (? the reflecting surfaces), then
//   into a reflW x reflH buffer with its own depth the course's
//   reflected parts (model24, course_draw_reflection_a), the cars mirrored in the ground
//   (drawCarReflections) and their lamps' reflections (drawLampReflections); blurred
//   reflBlur times (+-0.5 texel, bilinear) and blended over the screen
//   with reflBlend.
void RacePS2Base::drawReflections(GSBuffer *screen, GSBuffer *envBuf)
{
    CourseData *course = org->course->data;
    CourseEnv *env = course->env;
    CameraManager *cm = virtual_6();
    if (course->model34 != NULL) {
        gfx_set_frame_write_mask(0xFF000000);
        gs_set_clear_color(0);
        gs_clear_buffers(PGL_CLEAR_COLOR);
        gfx_set_frame_write_mask(0xFFFFFFFF);
        f32 x = cm->getPositionX();
        f32 y = cm->getPositionY();
        f32 z = cm->getPositionZ();
        course_draw_reflection_b(course, x, y, z);
    }
    pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B | PGL_ATTR_FOG |
                    PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR);                  /* 0x8C7 */
    GSBuffer refl(SCE_GS_PSMCT32, env->reflW, env->reflH);              /* sp+0x00 */
    GSBuffer reflZ(&refl, SCE_GS_PSMZ24);                                   /* sp+0x40 */
    refl.setDrawBuffer();
    reflZ.setZBuffer();
    modelset_set_projection_scale(1.0f);
    gs_set_clear_color(0);
    gs_clear_buffers(PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH);
    course_fog_apply(&env->reflFog, 1.0f);
    {
        f32 x = cm->getPositionX();
        f32 y = cm->getPositionY();
        f32 z = cm->getPositionZ();
        course_draw_reflection_a(course, x, y, z);
    }
    clear_frame_alpha(0x80000000);
    if (env->flags & ENV_REFL_CARS)
        drawCarReflections(envBuf, 0.0f);
    if (env->flags & ENV_REFL_LAMPS)
        drawLampReflections(0.0f);

    /* the blur: the buffer drawn over itself, half a texel off */
    if (env->reflBlur != 0) {
        refl.setTexture();
        gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
        gfx_disable(PGL_CAP_DEPTH_TEST);
        gfx_disable(PGL_CAP_ALPHA_TEST);
        gfx_disable(PGL_CAP_BLEND);
        gfx_disable(PGL_CAP_FOG);
        pgl_color(0x80FFFFFF);
        for (int pass = 0; pass < env->reflBlur; pass++) {
            f32 off = (pass & 1) ? 0.5f : -0.5f;
            f32 x = 0.0f, y = 0.0f;
            f32 w = (f32)refl.width;
            f32 h = (f32)refl.displayHeight();
            gs_draw_textured_sprite_strips_f(x, y, w, h, x + off, y + off,
                                             (f32)refl.width + off, (f32)refl.height + off);
            gfx_texflush();
        }
    }
    pgl_pop_attrib();
    modelset_set_projection_scale(1.0f);

    /* over the screen */
    refl.setTexture();
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_FOG);
    gfx_blend_func(env->reflBlend, 0.0f);
    pgl_color(0x80FFFFFF);
    f32 off = -0.5f;
    if (env->reflBlur & 1)
        off = 0.0f;
    f32 w = (f32)screen->width;
    f32 h = (f32)screen->displayHeight();
    gs_draw_textured_sprite_strips_f(0.0f, 0.0f, w, h, off + 0.0f, off + 0.0f,
                                     (f32)refl.width + off, (f32)refl.height + off);
}

// FUNC 0x0017FFE8 RacePS2Base::drawCarReflections
//   The cars mirrored in the ground (setReflectionPlane: the front faces
//   turn clockwise) into the reflection buffer, farthest first; not the
//   ones without a controller (noController: replay / ghost cars).  The
//   normal matrix of each is the inverse of its mirror matrix times the
//   view matrix saved first.  envBuf: the environment map; t is not used.
void RacePS2Base::drawCarReflections(GSBuffer *envBuf, f32 t)
{
    modelset_set_cull_far(1);
    gfx_front_face(PGL_FRONT_CW);
    gfx_enable(PGL_CAP_NORMAL_MATRIX);
    Matrix44 view;                                                          /* sp+0x00 */
    pgl_get_matrix(&view.m[0][0]);
    CameraManager *cm = virtual_6();
    RaceEntryBase *entries = org->entries;
    CourseData *course = org->course->data;
    f32 eyeX = cm->getPositionX();
    f32 eyeY = cm->getPositionY();
    f32 eyeZ = cm->getPositionZ();
    entries->sortByDistance(eyeX, eyeY, eyeZ);
    GsTexPacket envRegs;                                                    /* sp+0x40 */
    envBuf->makeTexRegs(&envRegs.regs);
    modelset_set_env_tex(&envRegs.regs);

    for (RaceEntryCar *car = entries->sortEnd; car != NULL; car = car->sortPrev) {
        if (cm->F_00209370(org, car) == 0)
            continue;
        RaceCarState *state = car->state;
        if (state->st.noController != 0)
            continue;
        CarRenderCtx lights;                                                /* sp+0x90 */
        lights.setup(course, state, 0.0f);
        pgl_push_matrix();
        car->setReflectionPlane(0.0f);
        pgl_push_matrix();
        vu0_invert_rigid();
        pgl_mult_matrix_90(&view.m[0][0]);
        Matrix44 normal;                                                    /* sp+0x2E0 */
        pgl_get_matrix(&normal.m[0][0]);
        pgl_set_normal_matrix(&normal.m[0][0]);
        pgl_pop_matrix();
        lights.apply();
        F_00179460(car->model.getCarModel())->callback.lod = 2;
        car->drawBody(0.0f);
        car->drawTireCylinders(org->course->data->env->brakeGlowScale, 0.0f);
        pgl_pop_matrix();
    }
    modelset_set_env_tex(NULL);
    gfx_disable(PGL_CAP_NORMAL_MATRIX);
    gfx_front_face(PGL_FRONT_CCW);
    modelset_set_cull_far(0);
}

// FUNC 0x00180218 RacePS2Base::drawLampReflections
//   The reflections of the cars' lamps on the road (FlareClass's reflection
//   list), the same cars as drawCarReflections.  Quirk: modelset_set_cull_far(1) is
//   not reset to 0 at the end (drawCars / drawCarReflections do it).  t is not
//   used.
void RacePS2Base::drawLampReflections(f32 t)
{
    modelset_set_cull_far(1);
    CameraManager *cm = virtual_6();
    RaceEntryBase *entries = org->entries;
    course_env_set_reflection(org->course->data->env, &flare);
    flare.beginReflections();
    for (RaceEntryCar *car = entries->sortEnd; car != NULL; car = car->sortPrev) {
        RaceCarState *state = car->state;
        if (cm->F_00209370(org, car) == 0)
            continue;
        if (state->st.noController != 0)
            continue;
        pgl_push_matrix();
        car->setReflectionPlane(0.0f);
        car->drawLampReflections(&flare, 0.0f);
        pgl_pop_matrix();
    }
    flare.endReflections();
}

// FUNC 0x00180330 RacePS2Base::drawMirror
//   The rear-view mirror: the view behind (the view matrix mirrored with
//   vu0_scale(1, 1, -1), front faces clockwise; far clip 120, 60 when
//   sub != 0) into the rectangle `mirror' of a copy of screen, with a blank
//   64 x 64 environment map and the mirror vision list (the alternative one
//   when sub != 0 and the course has it), the cars at LOD 2 and the smoke;
//   then a black frame around it and its depth cleared to the nearest (?
//   nothing drawn later shows through).
void RacePS2Base::drawMirror(GSBuffer *screen, int sub)
{
    /* implicit: GSBuffer::copy (the inline copy constructor) */
    GSBuffer view(*screen);                                                 /* sp+0x00 */
    view.setRect((int)mirror.x, (int)mirror.y, (int)mirror.w, (int)mirror.h);
    CourseData *course = org->course->data;
    CourseEnv *env = course->env;
    CameraManager *cm = virtual_6();
    pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B | PGL_ATTR_FOG |
                    PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR);                  /* 0x8C7 */
    gfx_front_face(PGL_FRONT_CW);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    GSBuffer envmap(SCE_GS_PSMCT16, 0x40, 0x40);                           /* sp+0x40 */
    envmap.setDrawBuffer();
    gs_set_clear_color(0x80000000);
    gs_clear_buffers(PGL_CLEAR_COLOR);

    /* the projection and the view */
    view.setDrawBuffer();
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    cm->setFarClip(sub != 0 ? 60.0f : 120.0f);
    cm->applyProjection();
    proj_y_scale(&g_002F4230);             /* implicit: F_00257EF0, vu0_scale (the helper's) */
    f32 zoom = mirror.zoom;
    f32 aspect;
    if (view.hasRect != 0)
        aspect = (f32)view.rectH / (f32)view.rectW;
    else
        aspect = (f32)view.displayHeight() / (f32)view.width;
    vu0_scale(aspect * PROJ_ASPECT * zoom, zoom, 1.0f);
    modelset_set_projection_scale(1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();
    vu0_scale(1.0f, 1.0f, -1.0f);
    cm->applyView();

    /* the course */
    course_env_apply(env);
    course_fog_apply(&env->mirrorFog, 1.0f);
    gs_set_clear_color_f(0.0f, 0.0f, 0.0f, 0.0f);
    gs_clear_buffers(PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH);
    course_draw_model(course, course->model60);
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    modelset_set_tex_lod(1);
    modelset_set_tex_set(2);
    if (sub != 0 && course->visionMirrorAlt != NULL)
        course_draw(course, course->visionMirrorAlt, cm, course->env->visionMirror);
    else
        course_draw(course, course->visionMirror, cm, course->env->visionMirror);
    modelset_set_tex_lod(0);
    clear_frame_alpha(0x80000000);

    /* the cars, the smoke */
    drawCars(&envmap, 2, 0, 0.0f);
    smoke.draw(1);

    /* the frame */
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    int right = view.rectW - 1;
    int bottom = view.rectH - 1;
    pgl_color(0);
    gs_draw_line(0, 0, 0, bottom);
    gs_draw_line(0, bottom, right, bottom);
    gs_draw_line(right, bottom, right, 0);
    gs_draw_line(right, 0, 0, 0);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gs_set_clear_depth(0.0f);
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    gs_set_clear_depth(1.0f);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_pop_matrix();
    gfx_front_face(PGL_FRONT_CCW);
    pgl_pop_attrib();
}

// FUNC 0x001807B0 clear_frame_alpha
//   Clears the alpha channel of the frame to rgba's alpha (write mask
//   0xFF000000: only alpha), then all channels writable again.
void clear_frame_alpha(u32 rgba)
{
    gfx_set_frame_write_mask(0xFF000000);
    gs_set_clear_color(rgba);
    gs_clear_buffers(PGL_CLEAR_COLOR);
    gfx_set_frame_write_mask(0xFFFFFFFF);
}

// FUNC 0x001807F8 write_alpha_depth
//   Before the course: the depth cleared, then buf (the buffer being drawn)
//   as a full-size texture sprite at the depth of gs_set_sprite_zf(1, 0xFF)
//   with nothing of the colour written (write mask 0), depth test ALWAYS
//   and alpha test NOTEQUAL 0: only the depth of the pixels whose alpha is
//   set (by F_001A9240's model58) is written, just above the cleared depth
//   (? for a later depth test: the flare's occlusion test or the replay
//   blur).  Leaves the depth test LEQUAL and the write mask 0x00FFFFFF
//   (colour, not alpha).
void write_alpha_depth(GSBuffer *buf)
{
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    buf->setTexture();
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_ALPHA_TEST);
    gfx_depth_func(PGL_DEPTHFUNC_ALWAYS);
    gfx_alpha_func(PGL_ATEST_NOTEQUAL, 0.0f);
    gfx_set_frame_write_mask(0);
    gs_set_sprite_zf(1, 0xFF);
    f32 w = (f32)buf->width;
    f32 h = (f32)buf->displayHeight();
    gs_draw_textured_sprite_strips_f(0.0f, 0.0f, w, h, 0.0f, 0.0f, (f32)buf->width, (f32)buf->height);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_depth_func(PGL_DEPTHFUNC_LEQUAL);
    gfx_set_frame_write_mask(0x00FFFFFF);
}

// FUNC 0x001808E8 RacePS2Base::addCarLamps
//   The lamps of the cars the camera is not inside of
//   (CameraManager::virtual_12) into the flare.
//   Matching (94.4%): retail reloads o->entries before the loop (a0) as
//   well as on the back edge; egcs's PRE reuses the count's load instead.
//   Only a volatile read of the count (a fakematch, not kept) stops it;
//   inline count / car accessors (also const ones) and -fno-gcse (worse
//   elsewhere) were tried.
void RacePS2Base::addCarLamps(f32 t)
{
    RaceOrganization *o = org;
    CameraManager *cm = virtual_6();
    course_env_set_lamps(org->course->data->env, &flare);
    int count = o->entries->count;     /* blez / slt: compared signed */
    for (int i = 0; i < count; i++) {
        RaceEntryCar *car = o->entries->cars[i];
        if (cm->virtual_12(o, i) != 0)
            car->addLamps(&flare, t);
    }
}

// FUNC 0x001809E8 RacePS2Base::addCourseLights
//   The course's lights into the flare: the flare source groups visible
//   from the vision list point nearest the eye (all of them when the course
//   has no vision list).
void RacePS2Base::addCourseLights()
{
    CourseData *course = org->course->data;
    CameraManager *cm = virtual_6();
    course_env_set_flare_types(org->course->data->env, &flare);
    f32 x = cm->getPositionX();
    f32 y = cm->getPositionY();
    f32 z = cm->getPositionZ();
    const u16 *groups;
    if (course->visionList == NULL)
        groups = NULL;
    else
        groups = vision_nearest(course->visionList, x, y, z)->parts;
    course_submit_flares(course, &flare, groups);
}

// FUNC 0x00180AE0 RacePS2Base::captureAfterimage
//   Splits the frame buffer shown last (the other one of g_002F4230.frame)
//   into the three VRAM planes at vramB74 (red; green +0x23000; blue
//   +0x46000; 640 x 224) and starts the afterimage: afterimage = max(1 -
//   (1/60) / afterimageTime, 0).  buf is not used.
//   The enhanced engine (race_logic.md D1, ENH_RACE_AFTERIMAGE): it keeps
//   the GS field mode of the capture (Lupo 2 L2 0x0017EC80).
void RacePS2Base::captureAfterimage(GSBuffer *buf)
{
    GSBuffer work(SCE_GS_PSMCT32, 0x40, 0x80);                              /* sp+0x00 3 pages of work VRAM */
    u32 planeR = g_002F4230.vramB74;
    u32 planeG = planeR + 0x23000;
    u32 planeB = planeR + 0x46000;
    pair_other(&g_002F4230.frame)->setTexture();
    pfx_split_rgb_planes(planeR, planeG, planeB, 0x280, 0xE0, work.addr);
#if ENH_RACE_AFTERIMAGE
    afterimageField = (u8)gs_get_field_mode();     /* Lupo 2 (L2 0x0017ED14) */
#endif
    f32 level = 1.0f - 0.0166666676f / afterimageTime;     /* 0x3C888889 0x1.111112p-6f = 1/60: one frame (race_ps2_render_internal.h) */
    afterimage = MAXF(level, 0.0f);
}

// FUNC 0x00180BF8 RacePS2Base::drawAfterimage
//   While afterimage > 0: the three saved planes (PSMT8H through an
//   identity CLUT) drawn over buf, each into its colour channel (write
//   masks 0xFF, 0xFF00, 0xFF0000), 640 x 448 from 640 x 224, blended
//   (Cs - Cd) x afterimage^3 + Cd.
//   The enhanced engine (race_logic.md D1, ENH_RACE_AFTERIMAGE; Lupo 2 L2
//   0x0017EDB0): nothing while the race runs in the background (behind a
//   menu), and when the current field is not the capture's the planes are
//   read half a line up (the odd field) or down, so the trail does not
//   jitter between the fields.  (Lupo 2 also draws it later, after the 2D
//   overlays, RacePS2Base::virtual_46: not taken, GT3's trail leaves the
//   HUD sharp.)
void RacePS2Base::drawAfterimage(GSBuffer *buf)
{
#if ENH_RACE_AFTERIMAGE
    if (org->backgroundMode != 0)           /* Lupo 2 (L2 0x0017EDDC) */
        return;
#endif
    if (0.0f < afterimage) {
        buf->setDrawBuffer();
        GSBuffer clut(SCE_GS_PSMCT32, 0x40, 0x20);                          /* sp+0x00 */
        u32 clutAddr = clut.addr;
        pfx_build_identity_clut32(clutAddr);
        gfx_texflush();
        pgl_color(0x80FFFFFF);
        gs_set_clut(clutAddr, SCE_GS_PSMCT32);
        gs_set_clut_csa(0);
        gfx_set_clut_mode(PGL_CLUT_T8);
        gfx_disable(PGL_CAP_DEPTH_TEST);
        gfx_disable(PGL_CAP_ALPHA_TEST);
        gfx_disable(PGL_CAP_DEST_ALPHA);
        gfx_disable(PGL_CAP_FOG);
        gfx_enable(PGL_CAP_BLEND);
        f32 a = afterimage;
        gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_CD), a * a * a);  /* 0x64 */
        gfx_depth_mask(0);
        gfx_set_texture_size(0x280, 0xE0);
        gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REGION_CLAMP);
        gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REGION_CLAMP);
        gfx_set_texture_region(PGL_REGION_U, 0, 0x27F);
        gfx_set_texture_region(PGL_REGION_V, 0, 0xDF);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
        gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
        u32 planeR = g_002F4230.vramB74;
        u32 planeG = planeR + 0x23000;
        u32 planeB = planeR + 0x46000;
        f32 v0 = 0.0f;                      /* the planes' first line */
#if ENH_RACE_AFTERIMAGE
        {
            int field = gs_get_field_mode();

            /* Lupo 2: half a line when the field is not the capture's (L2
             * 0x0017EF00..0x0017EF34) */
            if (field != afterimageField)
                v0 = (field == PGL_FIELD_HALF_ODD) ? -0.5f : 0.5f;
        }
#endif
        gfx_set_texture_buffer(0, planeR, SCE_GS_PSMT8H, 0x280);
        gfx_set_frame_write_mask(0x000000FF);
        gs_draw_textured_sprite_strips_f(0.0f, 0.0f, 640.0f, 448.0f, 0.0f, v0, 640.0f, 224.0f + v0);
        gfx_set_texture_buffer(0, planeG, SCE_GS_PSMT8H, 0x280);
        gfx_set_frame_write_mask(0x0000FF00);
        gs_draw_textured_sprite_strips_f(0.0f, 0.0f, 640.0f, 448.0f, 0.0f, v0, 640.0f, 224.0f + v0);
        gfx_set_texture_buffer(0, planeB, SCE_GS_PSMT8H, 0x280);
        gfx_set_frame_write_mask(0x00FF0000);
        gs_draw_textured_sprite_strips_f(0.0f, 0.0f, 640.0f, 448.0f, 0.0f, v0, 640.0f, 224.0f + v0);
        gfx_set_frame_write_mask(0xFFFFFFFF);
        gfx_depth_mask(1);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
        gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    }
}

// FUNC 0x00180EB0 F_00180EB0
//   Returns its argument (no callers: an inline function emitted here).
void *F_00180EB0(void *p)
{
    return p;
}
