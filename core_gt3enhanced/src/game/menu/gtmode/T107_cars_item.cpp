/* T107 - gt2k_menu::CarsItem, TheCar and CarGetMessageItem: the cars of the
 * prize car screen ("render_model" and "car_get_message" of
 * menu/car_get.mbl).  0x0013C8F0..0x0013E918.
 *
 * CarsItem holds the cars of one prize set (1..4) on the turntable of its
 * owner (CarGetControl::turntable, T106) and runs the screen's show
 * (virtual_1, the CARS_* states of car_get_internal.h):
 *   0  timer = 210 frames (one car) or 120;
 *   1  waits; one car: straight to 3, else the roulette starts (60 frames);
 *   2  waits, then the roulette stops on the car won (picked by the cars'
 *      weights in the constructor);
 *   3  waits for it to stop;
 *   4  the other cars are covered (highlight 1 -> 0), then the table slows
 *      down;
 *   5  the other cars sink to -2 * turntable.maxHeight, then the message
 *      shows (owner->carsShown) and the page is unfrozen;
 *   6  done.
 * While the other cars are still visible virtual_2 draws them all
 * (drawAll), then only the car won, lit by the camera's light set
 * (drawSelected). */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T107's .rodata (0x00319360..0x0031964E): SimpleCameraBase::print's format
 * string (T412's inline function: its literal is written where the header
 * is read; unreferenced, hence the anchor), virtual_1's jump table, the
 * vtables of CarGetMessageItem, CarsItem and TheCar, then the type names
 * CarGetMessageItem, EventNameItem, CarsItem, TheCar, CarGetControl,
 * PrizeControlBase, ControlBaseWithPageManager, CallBackWhenPrizeRendering,
 * PrizeModelSetContext, SmartIndexT<int>, the OperatorHolder group,
 * Operator: the menu item classes first, then the page manager, then the
 * prize classes and the car_get screen (T061's header brings them in that
 * order), this file's own header last.  No "bad_alloc" (no <new>).
 * ref/data/u021/notes.md. */
// DATA_START 0x00319360
#include "game/camera/T412_simple_camera_base.h"   /* first: SimpleCameraBase::print's string (the cameras'
                                               base, T106 / T411) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the OperatorHolder group) gt2k_menu::Transitive
                                               (the factories' result) */
#include "game/menu/framework/T211_page_manager.h"         /* (SmartIndexT<int>) gt2k_menu::PageManager (setFreeze) */
#include "game/menu/gtmode/T061_car_get_control.h"      /* (PrizeBase's classes, ControlBaseWithPageManager,
                                               PrizeControlBase, CarGetControl) the owner */
#include "game/menu/gtmode/T107_cars_item.h"            /* own: TheCar, CarsItem, EventNameItem, CarGetMessageItem */
/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_enable / gfx_disable, the pgl_* matrix and light calls,
                                               gs_* clears and sprites, tex1_relocate, Tex1Image, GsTexRegs,
                                               the PGL_* constants */
#include "engine/media/G002_mpeg_frameq.h"          /* tanf */
#include "game/car/model/T184_car_icon_maker.h"       /* GsTexPacket (drawAll's environment map registers) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, MenuTexture */
#include "game/menu/gtmode/T215_showroom_data.h"        /* ShowroomData, showroom_index / showroom_value_ptr / showroom_table */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (getTexture), MenuLayoutItem */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec, menu_car_spec */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_00352758 (the screen buffer) */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar (the wheel accessors) */
#include "game/camera/T411_menu_car_camera.h"      /* MenuCarCamera (showroom_index's argument) */
#include "game/race/core/T414_race_random.h"          /* raceRandomFloat */
#include "game/car/model/T446_car_model.h"            /* CarModel, car_shadow_set_*, g_00352AC4 */
#include "game/car/model/T450_menu_tire_model.h"      /* MenuTireModel */
#include "engine/gfx/T451_model_set.h"            /* ModelSet, modelset_set_env_tex */
#include "game/car/model/T456_car_model_file.h"       /* CarModelFile, carmodelfile_relocate, carmodelfile_work_area */
#include "engine/gfx/T512_render_context.h"       /* RenderContext g_002F4230, GSBufferPair */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "engine/gfx/T515_tex_set.h"              /* TexSet */
#include "game/menu/gtmode/car_get_internal.h"

/* The prize car scene's light vectors: T107 .bss, file statics (local
 * symbols in the original), in this order. */
static f32 g_0049BE50[4];                   /* 0x0049BE50 the ambient colour (pgl_light_modelfv) */
static f32 g_0049BE60[4];                   /* 0x0049BE60 light 0's ambient colour (TheCar::setLights) */
static f32 g_0049BE70[4];                   /* 0x0049BE70 light 0's diffuse colour */
static f32 g_0049BE80[4];                   /* 0x0049BE80 light 0's specular colour */

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* TheCar                                                                   */
/* ------------------------------------------------------------------------ */

/* Makes this car the menu's car on show (inline, see the class). */
inline void TheCar::selectSpec()
{
    menu_car_spec()->setCar(code);
}

/* Takes the loaded model file of car `code' (relocates it), sets up the
 * body and the four tyres (sizes from the car's MenuCarSpec, front pair
 * then rear pair: tyre width, radius, sidewall; their packets built after
 * the file) and the colour. */
// FUNC 0x0013C8F0 gt2k_menu::TheCar::setup
void TheCar::setup(const char *code, CarModelFile *model, int color)
{
    this->code = code;
    this->model = model;
    CarModel *b = &body;                    /* (kept in a register for the colour) */
    carmodelfile_relocate(model);
    b->setModel(this->model->body, 1);
    selectSpec();                           /* implicit: setCar (TheCar::selectSpec, inline) */
    void *work = carmodelfile_work_area(this->model);
    for (int wheel = 0; wheel < 4; wheel++) {
        tires[wheel].setPosition(wheel);
        tires[wheel].setWheelFile(this->model->wheel);
        tires[wheel].setTireFile(this->model->tire);
        tires[wheel].setWheelModel(this->model->body);
        /* wheel >> 1: 0 the front pair, 1 the rear */
        f32 width = menu_car_spec()->car.dyn.gear.tyreWidth[wheel >> 1];
        f32 radius = menu_car_spec()->car.dyn.gear.tyreRadius[wheel >> 1];
        tires[wheel].setSizeProfile(width, radius, menu_car_spec()->car.dyn.gear.tyreSidewall[wheel >> 1]);
        work = tires[wheel].build(work, 30);    /* 30 segments */
    }
    b->color = color;
}

/* The alpha of the floor picture: 1 while selected, else fades by 0.05 a
 * frame. */
// FUNC 0x0013CA30 gt2k_menu::TheCar::updateAlpha
void TheCar::updateAlpha()
{
    if (selected) {
        alpha = 1.0f;
        return;
    }
    alpha -= 0.05f;
    if (alpha < 0.0f)
        alpha = 0.0f;
}

/* The CPU lighting on and, if the showroom record of the camera's mode has
 * a light 0, light 0 with the record's ambient, diffuse and specular
 * colours times fade and its position.  `this' is not used. */
// FUNC 0x0013CA78 gt2k_menu::TheCar::setLights
void TheCar::setLights(MenuCarCamera *cam, f32 fade)
{
    gfx_enable(PGL_CAP_CPU_LIGHTING);
    int record = showroom_index(cam);
    int i;
    pgl_push_matrix();
    if (showroom_flag(showroom_table(), SHOWROOM_FLAG_LIGHT0, record) != 0) {
        gfx_enable(PGL_CAP_LIGHT0);
        const f32 *color = showroom_value_ptr(showroom_table(), SHOWROOM_LIGHT0_AMBIENT, record);
        for (i = 0; i < 4; i++)
            g_0049BE60[i] = color[i] * fade;
        pgl_lightfv(PGL_CAP_LIGHT0, PGL_LIGHT_AMBIENT, g_0049BE60);
        color = showroom_value_ptr(showroom_table(), SHOWROOM_LIGHT0_DIFFUSE, record);
        for (i = 0; i < 4; i++)
            g_0049BE70[i] = color[i] * fade;
        pgl_lightfv(PGL_CAP_LIGHT0, PGL_LIGHT_DIFFUSE, g_0049BE70);
        color = showroom_value_ptr(showroom_table(), SHOWROOM_LIGHT0_SPECULAR, record);
        for (i = 0; i < 4; i++)
            g_0049BE80[i] = color[i] * fade;
        pgl_lightfv(PGL_CAP_LIGHT0, PGL_LIGHT_SPECULAR, g_0049BE80);
        pgl_lightfv(PGL_CAP_LIGHT0, PGL_LIGHT_POSITION,
                    showroom_value_ptr(showroom_table(), SHOWROOM_LIGHT0_POSITION, record));
    }
    pgl_pop_matrix();
}

/* `this' is not used. */
// FUNC 0x0013CC20 gt2k_menu::TheCar::resetLights
void TheCar::resetLights()
{
    gfx_disable(PGL_CAP_CPU_LIGHTING);
    gfx_disable(PGL_CAP_LIGHT0);
}

/* The body's part 6 in black, then in the shadow colour in the ground pass
 * (CarModel::F_0021F328 / F_0021F258; ? the car's own shadow). */
// FUNC 0x0013CC48 gt2k_menu::TheCar::F_0013CC48
void TheCar::F_0013CC48()
{
    body.F_0021F328();
    body.F_0021F258();
}

/* The body, raised so that the front wheels touch the floor: by the tyre
 * radius less the wheel centre's height. */
// FUNC 0x0013CC78 gt2k_menu::TheCar::drawBody
void TheCar::drawBody()
{
    selectSpec();                           /* implicit: setCar (TheCar::selectSpec, inline) */
    f32 radius = menu_car_spec()->car.dyn.gear.tyreRadius[0];
#if ENH_PHYSICS_REAR_RIDE_HEIGHT
    /* The enhanced engine (from the PC port, its ROADMAP 68): pitched for
     * the rear axle's own height; drawTires follows in this matrix
     * (ShowRoomCar::restLift) */
    f32 pitch;
    pgl_translate(0.0f, menu_car_spec()->car.restLift(radius, &pitch), 0.0f);
    vu0_rotate_x(pitch);
#else
    pgl_translate(0.0f, radius - menu_car_spec()->car.wheelHeight(0, 0.0f, 0.0f), 0.0f);
#endif
    body.draw();
}

/* The four tyres at their wheel positions, turned by the steer and camber
 * angles (radians -> degrees). */
// FUNC 0x0013CCF8 gt2k_menu::TheCar::drawTires
void TheCar::drawTires()
{
    for (int wheel = 0; wheel < 4; wheel++) {
        pgl_push_matrix();
        f32 lateral = menu_car_spec()->car.wheelLateralPos(wheel, 0.0f, 0.0f);
        f32 height = menu_car_spec()->car.wheelHeight(wheel, 0.0f, 0.0f);
        pgl_translate(lateral, height, menu_car_spec()->car.wheelAxleX(wheel, 0.0f, 0.0f));
        f32 steer = menu_car_spec()->car.wheelSteerAngle(wheel, 0.0f, 0.0f) * 180.0f / 3.14159265f;
        f32 camber = menu_car_spec()->car.wheelCamber(wheel, 0.0f, 0.0f) * 180.0f / 3.14159265f;
        tires[wheel].draw(steer, camber, 0.0f, 1, 1);  /* with the tyre and its shadow */
        pgl_pop_matrix();
    }
}

/* ------------------------------------------------------------------------ */
/* CarsItem                                                                 */
/* ------------------------------------------------------------------------ */

/* n cars from owner's entries; picks the car won: a random point of the
 * sum of the weights (raceRandomFloat), the first car whose running sum
 * reaches it.  The camera shows the prize record (mode 9).
 * Matching: 99.7%, exact with the reference compiler 991111-01; with 991111b
 * two spill slots are swapped (0x1C / 0x20: the registers GCSE keeps
 * this + 0x100 (&roulette) and this + 0x11C (&cars[0]) in; their order
 * follows GCSE's hashes, which depend on the pseudo numbers: MATCHING.md,
 * Register allocation). */
// FUNC 0x0013CE58 gt2k_menu::CarsItem::CarsItem
CarsItem::CarsItem(int n, MenuLayoutItem *res, MenuTexture *tex, PageManager *pages,
                   CarGetControl *ctl, Tex1Image *image, CarGetControl *owner)
    : Item(res, tex), roulette(n, 0), ctl(ctl), pages(pages)
{
    /* The members in order: Item::Item(res, tex), the vptr, roulette
     * (CarRoulette::CarRoulette(n, 0)), cars[4] (TheCar's inline
     * constructor: code, model, CarModel::CarModel, a loop of
     * MenuTireModel::MenuTireModel, alpha, selected), ctl, pages, the camera
     * (CarGetMenuCamera's inline constructor: MenuCarCamera's mode 0, the
     * MotionValues, the vptr, state, next).  The landing pads destroy the
     * members built so far: CameraBase::~CameraBase, the cars and tyres
     * through their vtable, CarModel::~CarModel, Item::~Item. */
    /* implicit: CarRoulette, CarModel, MenuTireModel (member constructors), ~CameraBase, ~CarModel (landing pads) */
    this->tex = image;                      /* (the fields in member order) */
    this->owner = owner;
    timer = -1;
    unk_1040 = 0;
    count = n;
    state = CARS_START;
    f32 total = 0.0f;
    int i;
    for (i = 0; i < count; i++) {
        TheCar *car = &cars[i];
        CarGetEntry *e = &owner->entries[i];
        car->setup(e->code, e->model, e->color);
        total += this->owner->weights[i];
    }
    total *= raceRandomFloat(1);
    roulette.setTarget(0);
    f32 sum = 0.0f;
    for (i = 0; i < count; i++) {
        sum += this->owner->weights[i];
        if (total <= sum) {
            roulette.setTarget(i);
            break;
        }
    }
    fade = 0.0f;
    drop = 0.0f;
    highlight = 1.0f;
    tex1_relocate(this->tex);
    this->pages->setFreeze(1);
    camera.setType(MENU_CAMERA_MODE_PRIZE);
}

/* The cars' sizes for the turntable: half the length and the height, in
 * metres. */
// FUNC 0x0013D368 gt2k_menu::CarsItem::setupLayout
void CarsItem::setupLayout()
{
    f32 halfSize[CAR_GET_MAX_CARS];         /* sp+0x00 */
    f32 height[CAR_GET_MAX_CARS];           /* sp+0x10 */

    for (int i = 0; i < count; i++) {
        cars[i].selectSpec();               /* implicit: setCar (TheCar::selectSpec, inline) */
        f32 length = (f32)menu_car_spec()->car.spec.length * 0.001f;
        f32 h = (f32)menu_car_spec()->car.spec.height * 0.001f;
        halfSize[i] = length * 0.5f;
        height[i] = h;
    }
    owner->turntable.setup(halfSize, height);
}

/* The cars not won have sunk below the floor (deeper than the tallest
 * car). */
// FUNC 0x0013D468 gt2k_menu::CarsItem::othersGone
int CarsItem::othersGone()
{
    if (drop < -owner->turntable.maxHeight)
        return 1;
    return 0;
}

/* One frame: Item's update, the roulette, the turntable, the cars' selected
 * flags (the slot the roulette shows), the camera, the show (jump table
 * 0x00319380, 7 cases), the car won's light, and the owner's selected
 * car. */
// FUNC 0x0013D498 gt2k_menu::CarsItem::virtual_1
void CarsItem::virtual_1()
{
    Item::virtual_1();
    roulette.update();
    owner->turntable.update();
    timer--;
    int shown = -1;
    for (int i = 0; i < count; i++) {
        u8 isShown = 0;
        /* implicit: car_roulette_slot (CarRoulette::slot, inline; here and below) */
        if (i == roulette.slot()) {
            isShown = 1;
            shown = i;
        }
        cars[i].setSelected(isShown);
        cars[i].updateAlpha();
    }
    camera.update(&owner->turntable, shown, count);

    switch (state) {
    case CARS_START:
        if (count == 1)
            timer = CARS_WAIT_ONE_CAR;
        else
            timer = CARS_WAIT_FIRST;
        state = CARS_WAIT;
        /* fall through */
    case CARS_WAIT:
        if (timer > 0)
            break;
        camera.requestNext();
        if (count == 1) {
            state = CARS_STOPPING;
            break;
        }
        roulette.start();
        timer = CARS_SPIN_TIME;
        state = CARS_SPIN;
        break;
    case CARS_SPIN:
        if (timer > 0)
            break;
        roulette.stop();
        state = CARS_STOPPING;
        break;
    case CARS_STOPPING:
        if (!roulette.isIdle())
            break;
        state = CARS_HIGHLIGHT;
        break;
    case CARS_HIGHLIGHT:
        highlight -= 0.025f;
        if (highlight < 0.0f) {
            highlight = 0.0f;
            owner->turntable.stopping = 1;
            camera.requestNext();
            state = CARS_SINK;
        }
        break;
    case CARS_SINK: {
        drop -= 0.05f;
        f32 bottom = -owner->turntable.maxHeight;
        bottom = bottom + bottom;
        if (drop < bottom) {
            owner->carsShown = 1;
            drop = bottom;                  /* (the store is in the call's delay slot) */
            pages->setFreeze(0);
            state = CARS_DONE;
        }
        break;
    }
    case CARS_DONE:
        break;
    }

    if (!othersVisible()) {
        fade += 1.0f / 30.0f;               /* 0x3D088889: 1 in half a second */
        if (1.0f < fade)
            fade = 1.0f;
    } else {
        fade = 0.0f;
    }
    owner->selected = roulette.slot();
}

// FUNC 0x0013D730 gt2k_menu::CarsItem::othersVisible
int CarsItem::othersVisible()
{
    return othersGone() == 0;
}

// FUNC 0x0013D750 gt2k_menu::CarsItem::virtual_2
void CarsItem::virtual_2()
{
    if (othersVisible())
        drawAll();
    else
        drawSelected();
}

/* The floor picture (the item's own texture set) under car i, alpha
 * `alpha': at the car's place and heading if `place' (else the caller has
 * moved there), turned back so that it does not turn with the car, 5 mm
 * below it and pulled 0.005 towards the viewer in the projection, scaled to
 * turntable.spacing / 2, blended without depth writes. */
// FUNC 0x0013D798 gt2k_menu::CarsItem::drawFloor
void CarsItem::drawFloor(s8 i, f32 alpha, int place)
{
    f32 x, z;                               /* sp+0x0, sp+0x4 */

    owner->turntable.getWorldPos(i, &x, &z);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    proj_shift_depth(0.005f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 0.0f);  /* 0x44 */
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_CULL_FACE);
    if (place != 0) {
        pgl_translate(x, 0.0f, z);
        vu0_rotate_y(owner->turntable.carAngle[i]);
    }
    pgl_push_matrix();
    pgl_translate(0.0f, -0.005f, 0.0f);
    vu0_rotate_y(-owner->turntable.carAngle[i]);
    f32 scale = owner->turntable.spacing * 0.5f;
    vu0_scale(scale, 1.0f, scale);
    pgl_color4f(1.0f, 1.0f, 1.0f, alpha);
    gfx_depth_mask(0);
    draw_table_floor(texSet);
    gfx_depth_mask(1);
    pgl_pop_matrix();
    gfx_disable(PGL_CAP_BLEND);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
}

/* The projection of the prize car scene (both draw functions), from the
 * PROJECTION matrix mode to a fresh MODELVIEW matrix: 640 x 448, near 0.1,
 * far 1000, a horizontal field of view of 40 degrees, the y scale 1.098,
 * x * 0.75 on a 16:9 screen (rc->wide).  Inline: retail keeps rc
 * (&g_002F4230) in a register of its own for the wide test. */
static inline void cars_projection(RenderContext *rc)
{
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    vu0_load_identity_regs();
    pgl_frustum(-320.0f, 320.0f, -224.0f, 224.0f, 0.1f, 1000.0f, 320.0f / tanf(CARS_HALF_FOV_RAD));
    F_00257EF0(1.098f);
    if (rc->wide != 0)
        vu0_scale(0.75f, 1.0f, 1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    vu0_load_identity_regs();
}

/* All the cars, the one the roulette shows last.  The scene is drawn into
 * `back' (a PSMCT32 buffer the size of g_002F4230's current depth buffer
 * unkAC4, which is the depth buffer) over a copy of the screen, then
 * blended onto the screen (blitField, 0.8).  `work' keeps another copy of
 * the screen: before the car shown is drawn it is laid over the cars drawn
 * so far with 1 - highlight (in state 4 they fade out to the background),
 * then its VRAM is freed.  The cars reflect a black 32 x 32 texture (env),
 * their environment-map colours are off (pgl_set_scalar_param 0), the
 * ambient light is black and nothing else is lit; the others are lowered
 * by drop. */
// FUNC 0x0013D938 gt2k_menu::CarsItem::drawAll
void CarsItem::drawAll()
{
    g_0049BE50[0] = 0.0f;
    g_0049BE50[1] = 0.0f;
    g_0049BE50[2] = 0.0f;
    g_0049BE50[3] = 1.0f;
    pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B | PGL_ATTR_VIEWPORT |
                    PGL_ATTR_SCISSOR);  /* 0x887 */
    gfx_enable(PGL_CAP_COLOR_CLAMP);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_VU1_PROG_33E);
    gfx_enable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_FLAG24);
    gfx_depth_mask(1);
    gfx_color_mask(1, 1, 1, 1);
    gfx_disable(PGL_CAP_FOG);
    g_00352AC4 = 0.6f;                      /* the car shadow strength */
    car_shadow_set_color(0.0f, 0.0f, 0.0f, 1.0f);
    GSBuffer *screen = g_00352758;
    GSBufferPair &depth = g_002F4230.unkAC4;
    GSBuffer back(&depth.buf[depth.cur], 0 /* PSMCT32 */);  /* sp+0x00 */
    GSBuffer work(screen, 0 /* PSMCT32 */); /* sp+0x40 */
    GSBuffer env(0 /* PSMCT32 */, 32, 32);  /* sp+0x80 */
    env.setDrawBuffer();
    gs_set_clear_color(0x80000000);         /* black, alpha 0x80 */
    gs_clear_buffers(PGL_CLEAR_COLOR);
    GsTexPacket envRegs;                    /* sp+0xC0 (0x50 bytes, as in u045 / u061) */
    env.makeTexRegs(&envRegs.regs);
    modelset_set_env_tex(&envRegs.regs);
    back.setField(PGL_FIELD_FRAME_SHIFT);
    back.blit(screen, PGL_FILTER_NEAREST, PGL_FILTER_NEAREST);
    work.blit(screen, PGL_FILTER_NEAREST, PGL_FILTER_NEAREST);
    back.displayHeight();                   /* (result unused) */
    gfx_init_lights_material_spr();
    pgl_light_modelfv(PGL_LIGHTMODEL_AMBIENT, g_0049BE50);
    back.setDrawBuffer();
    depth.buf[depth.cur].setZBuffer();      /* the depth buffer of the scene */
    gs_set_clear_depth(1.0f);
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    /* implicit: pgl_frustum, vu0_load_identity_regs, F_00257EF0, vu0_scale, tanf (cars_projection, inline) */
    cars_projection(&g_002F4230);
    camera.applyView();                     /* the view (CarGetMenuCamera::applyView) */

    int i;
    s8 order[CAR_GET_MAX_CARS] = {0, 1, 2, 3};  /* sp+0x110 (a copy of .sdata 0x003516C0) */
    for (i = 0; i < count; i++) {
        s8 c = order[i];
        if (cars[c].selected) {             /* swapped to the end */
            order[i] = order[count - 1];
            order[count - 1] = c;
            break;
        }
    }
    for (i = 0; i < count; i++) {
        s8 c = order[i];
        pgl_push_matrix();
        f32 x, z;                           /* sp+0x120, sp+0x124 */
        owner->turntable.getWorldPos(c, &x, &z);
        pgl_translate(x, 0.0f, z);
        vu0_rotate_y(owner->turntable.carAngle[c]);
        TheCar *car = &cars[c];
        if (car->selected) {
            /* the screen before the cars over the others */
            gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
            gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
            gfx_set_texture_function(PGL_TFX_MODULATE);
            gfx_disable(PGL_CAP_DEST_ALPHA);
            gfx_disable(PGL_CAP_ALPHA_TEST);
            gfx_depth_mask(0);
            gfx_enable(PGL_CAP_BLEND);
            gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_CD),
                           1.0f - highlight);   /* 0x64 */
            pgl_color(0x80FFFFFF);
            work.setTexture();
            gs_draw_textured_sprite_f(0.0f, 0.0f, (f32)back.width, (f32)back.displayHeight(),
                                      0.0f, 0.0f, (f32)work.width, (f32)(work.height - 1));
            work.freeVram();
        } else {
            pgl_translate(0.0f, drop, 0.0f);
        }
        drawFloor(c, car->alpha, 0);
        car_shadow_set_dir(0.0f, 0.0f, 0.0f);
        pgl_set_scalar_param(0, 0.0f);      /* qAF0: no environment-map colours */
        car->F_0013CC48();
        car->drawBody();
        car->drawTires();
        pgl_set_scalar_param(0, 1.0f);
        pgl_pop_matrix();
    }
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    gs_set_clear_depth(1.0f);
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    screen->blitField(&back, 1, 0.8f);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ANTIALIAS);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    pgl_pop_attrib();                       /* (the matrix mode stays PROJECTION) */
}                                           /* GSBuffer::~GSBuffer: env, work, back (flags 2) */

/* Only the car won (the first selected one), on its floor, lit: the
 * ambient colour is the showroom record's times fade, plus light 0
 * (TheCar::setLights); its environment map is the first texture of tex
 * ("menu/prize_get.img", loaded into VRAM by envTex) and its colours fade
 * in with fade.  The scene is drawn into `back' over a copy of the screen
 * whose alpha is set to 0x80, then blended onto the screen. */
// FUNC 0x0013DF30 gt2k_menu::CarsItem::drawSelected
void CarsItem::drawSelected()
{
    s8 won = 0;
    for (int i = 0; i < count; i++) {
        if (cars[i].selected) {
            won = i;
            break;
        }
    }
    TheCar *car = &cars[won];
    int record = showroom_index(&camera);
    const f32 *ambient = showroom_value_ptr(showroom_table(), SHOWROOM_AMBIENT, record);
    g_0049BE50[0] = ambient[0] * getFade();
    g_0049BE50[1] = ambient[1] * getFade();
    g_0049BE50[2] = ambient[2] * getFade();
    g_0049BE50[3] = 1.0f;
    pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B | PGL_ATTR_VIEWPORT |
                    PGL_ATTR_SCISSOR);  /* 0x887 */
    gfx_enable(PGL_CAP_COLOR_CLAMP);
    gfx_enable(PGL_CAP_VU1_PROG_33E);
    gfx_enable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_FLAG24);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_depth_mask(1);
    gfx_color_mask(1, 1, 1, 1);
    gfx_disable(PGL_CAP_FOG);
    g_00352AC4 = 0.6f;                      /* the car shadow strength */
    car_shadow_set_color(0.0f, 0.0f, 0.0f, 1.0f);
    GSBuffer *screen = g_00352758;          /* sp+0x88 */
    TexSet envTex;                          /* sp+0x00 TexSet::TexSet */
    envTex.load(tex);
    modelset_set_env_tex(tex->tex);
    GSBufferPair &depth = g_002F4230.unkAC4;
    GSBuffer back(&depth.buf[depth.cur], 0 /* PSMCT32 */);  /* sp+0x40 */
    back.setField(PGL_FIELD_FRAME_SHIFT);
    back.blit(screen, PGL_FILTER_NEAREST, PGL_FILTER_NEAREST);
    back.setDrawBuffer();
    gfx_color_mask(0, 0, 0, 1);             /* the alpha only: 0x80 */
    pgl_color(0x80000000);
    int width = back.width;
    gs_draw_flat_sprite(0, 0, width, back.displayHeight());
    gfx_color_mask(1, 1, 1, 1);
    back.displayHeight();                   /* (result unused) */
    gfx_init_lights_material_spr();
    pgl_light_modelfv(PGL_LIGHTMODEL_AMBIENT, g_0049BE50);
    back.setDrawBuffer();
    depth.buf[depth.cur].setZBuffer();      /* the depth buffer of the scene */
    gs_set_clear_depth(1.0f);
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    /* implicit: pgl_frustum, vu0_load_identity_regs, F_00257EF0, vu0_scale, tanf (cars_projection, inline) */
    cars_projection(&g_002F4230);
    camera.applyView();                     /* the view (CarGetMenuCamera::applyView) */
    drawFloor(won, 1.0f, 1);
    pgl_push_matrix();
    f32 x, z;                               /* sp+0x80, sp+0x84 */
    owner->turntable.getWorldPos(won, &x, &z);
    pgl_translate(x, 0.0f, z);
    vu0_rotate_y(owner->turntable.carAngle[won]);
    pgl_set_scalar_param(0, getFade());          /* qAF0: the environment-map colours */
    car_shadow_set_dir(0.0f, 0.0f, 0.0f);
    car->setLights(&camera, getFade());
    car->F_0013CC48();
    car->drawBody();
    car->drawTires();
    car->resetLights();
    pgl_set_scalar_param(0, 1.0f);
    pgl_pop_matrix();
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    gs_set_clear_depth(1.0f);
    gs_clear_buffers(PGL_CLEAR_DEPTH);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    screen->blitField(&back, 1, 0.8f);
    gfx_disable(PGL_CAP_CPU_LIGHTING);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ANTIALIAS);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    pgl_pop_attrib();
}                                           /* GSBuffer::~GSBuffer (back), TexSet::~TexSet (envTex), flags 2 */

/* ------------------------------------------------------------------------ */
/* CarGetMessageItem                                                        */
/* ------------------------------------------------------------------------ */

/* Item::virtual_1 once the owner shows the message. */
// FUNC 0x0013E438 gt2k_menu::CarGetMessageItem::virtual_1
void CarGetMessageItem::virtual_1()
{
    if (owner != NULL && owner->carsShown != 0)
        Item::virtual_1();
}

/* Item::virtual_2 once the owner shows the message. */
// FUNC 0x0013E470 gt2k_menu::CarGetMessageItem::virtual_2
void CarGetMessageItem::virtual_2()
{
    if (owner != NULL && owner->carsShown != 0)
        Item::virtual_2();
}

} /* namespace gt2k_menu */

using namespace gt2k_menu;

/* "car_get_message" (ctx: the CarGetControl): Object::operator new(0x104),
 * then CarGetMessageItem's inline constructor (Item::Item(item, tex), owner,
 * the vptr); the landing pad gives the block back with Object::operator
 * delete.  parent and p8 .. p14 are not used. */
// FUNC 0x0013E4A8 create_car_get_message
Transitive *create_car_get_message(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                   int p8, int pC, int p10, int p14)
{
    CarGetControl *owner = (CarGetControl *)ctx;
    MenuTexture *tex = layout->getTexture(item);
    /* implicit: Item (inline constructor) */
    CarGetMessageItem *msg = new CarGetMessageItem(owner, item, tex);  /* Item::Item(item, tex), owner, the vptr */ /* implicit: Object::operator delete (if the constructor throws) */
    return msg;
}

// FUNC 0x0013E550 gt2k_menu::CarGetMessageItem::CarGetMessageItem
/* inline in game/menu/gtmode/T107_cars_item.h; egcs emits this copy (no callers):
 * Item::Item(res, tex), owner, the vptr */
/* implicit: Item (base constructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x0013E5A0 __tfQ29gt2k_menu17CarGetMessageItem
extern "C" void *__tfQ29gt2k_menu17CarGetMessageItem(void)
{
    if (__tiQ29gt2k_menu17CarGetMessageItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu17CarGetMessageItem, "Q29gt2k_menu17CarGetMessageItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu17CarGetMessageItem;
}
#endif

// FUNC 0x0013E5F0 gt2k_menu::CarGetMessageItem::~CarGetMessageItem
/* implicit in game/menu/gtmode/T107_cars_item.h (no vptr store); egcs emits it for the
 * vtable: Item::~Item (flags 0), then Object::operator delete if flags & 1 */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */

/* CarsItem's inline members (in-class in game/menu/gtmode/T107_cars_item.h): virtual_1
 * is CarsItem's key function, so egcs writes all three out of line here,
 * after CarGetMessageItem's group (the classes in the reverse order of their
 * definitions), in the reverse order of their bodies, then the typeinfo
 * function (retail's function order: the frame info). */

// FUNC 0x0013E638 gt2k_menu::CarsItem::create
/* "render_model" (the first pass's create_cars_item): Object::operator
 * new(0x1048), a CarsItem of the owner's cars and environment map, its
 * turntable set up from the cars' sizes; the landing pad gives the block
 * back with Object::operator delete. */
/* implicit: new, getTexture, CarsItem (constructor), setupLayout, delete (the in-class body of CarsItem::create) */

// FUNC 0x0013E710 gt2k_menu::CarsItem::~CarsItem
/* The vptr; the camera (CarGetMenuCamera's implicit destructor expanded:
 * CameraBase::~CameraBase, flags 2); cars[3..0] through their vtable
 * (flags 0); Item::~Item (flags 0); Object::operator delete if flags & 1. */
/* implicit: ~CameraBase (member destructor), ~Item (base destructor), operator delete (deleting destructor) */

// FUNC 0x0013E7C8 gt2k_menu::CarsItem::getFade
/* return fade (no callers: drawSelected expands it) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x0013E7D0 __tfQ29gt2k_menu8CarsItem
extern "C" void *__tfQ29gt2k_menu8CarsItem(void)
{
    if (__tiQ29gt2k_menu8CarsItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu8CarsItem, "Q29gt2k_menu8CarsItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu8CarsItem;
}
#endif

// FUNC 0x0013E820 gt2k_menu::TheCar::~TheCar
/* inline in game/menu/gtmode/T107_cars_item.h (TheCar has no key function); egcs emits
 * this copy with the vtable: the vptr (0x00319430); tires[3..0] through
 * their vtable (MenuTireModel::~MenuTireModel, flags 0);
 * CarModel::~CarModel (body, flags 2); the global operator delete
 * (0x0027C0C0) if flags & 1 (TheCar is not a menu Object) */
/* implicit: ~CarModel (member destructor), operator delete (deleting destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function; no bases: __rtti_user. */
// FUNC 0x0013E8D8 __tfQ29gt2k_menu6TheCar
extern "C" void *__tfQ29gt2k_menu6TheCar(void)
{
    if (__tiQ29gt2k_menu6TheCar[0] == 0)
        __rtti_user(__tiQ29gt2k_menu6TheCar, "Q29gt2k_menu6TheCar");
    return __tiQ29gt2k_menu6TheCar;
}
#endif
