/* T180_race_car_model.cpp - RaceCarModel and T180's copies of the
 * UnitArena<64> members (0x001A9B90..0x001AB7D0); the class is declared in
 * include/game/car/model/T180_race_car_model.h.
 *
 * RaceCarModel is the drawn model of a race car (RaceEntryCar +0x11F8).  Its
 * car file is read asynchronously (file_req_alloc) into one 0xA8000-byte
 * block of the car model arena g_002C2F50 (7 blocks, set up by race_begin),
 * relocated in place, and its body handed to the CarModel (T446).  The tyre
 * file ("tire/n%d", the number from the car's tyre parts) and the wheel file
 * ("wheel/%s") are loaded the same way into buffers inside the car file.
 * The drawing methods place the VU0 matrix from the car state at time t
 * (the body frame, or the ground frame of T302's car_ground_* accessors)
 * and draw the body, its passes, the four tyres (with a motion blur from
 * their screen speed and the glow of a hot brake), the lamps (as lights of
 * the FlareClass) and the sparks thrown where the points under the car go
 * below the road.
 */
/* Retail T180's .rodata (0x00330B38): <new>'s "bad_alloc" (unreferenced:
 * DATA_START; T179's is at 0x00330A98, u059's data pass), g_00330B48, the
 * two vtables, then the type names RaceCarModel, UnitArena<64>; T121's
 * RaceEntryCar is not seen. */
#include <new>                              /* (unused: "bad_alloc") */
// DATA_START 0x00330B38
#define GAME_T153_EMITS_TYPEINFO           /* egcs generates its weak __tft9UnitArena1Ui64 here too */
#include "game/race/core/T153_race_course.h"          /* UnitArena<64> (its __tf: the first deferred function
                                               egcs saves here, the last it writes) */
class CarModel;
CarModel *F_00179460(CarModel *m);          /* 0x00179460 (T121_race_entry.h: T121's copy) */
#ifdef BOOT_OLD_GCC
/* T180's weak copy of T121's F_00179460 (retail 0x001AB778, no callers:
 * setModelLod's call goes to T121's 0x00179460).  Retail writes it after
 * __tf12RaceCarModel and before UnitArena<64>'s __tf (the frame info), so
 * its body was read after UnitArena<64> was instantiated and before
 * RaceCarModel was complete; yet setModelLod (defined later) calls it.  An
 * exact form, not the original: setModelLod goes through a local inline
 * helper compiled before the local inline body, so the call stays (egcs
 * inlines only bodies it has already read) and egcs writes the copy.
 * T129_race_base.cpp (u043) does the same for its copy.  GCC 14 calls
 * T121's F_00179460 directly. */
inline CarModel *race_car_model_lod_param(CarModel *m)
{
    return F_00179460(m);
}

// FUNC 0x001AB778 F_00179460
//   T180's dead weak copy (no callers; T121's 0x00179460 is the one
//   linked): returns its argument.
inline CarModel *F_00179460(CarModel *m)
{
    return m;
}
#endif
#include "game/car/model/race_car_model_internal.h"
#include "libc.h"                           /* malloc, free, memcpy, strcpy */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_user, __rtti_si (GCC 14's __tf; egcs's <new>
                                               and cxxrt.h conflict: egcs generates the __tf itself) */
#endif
#include "game/car/model/T180_race_car_model.h"       /* RaceCarModel, g_002C2F50; + T446 CarModel, T449 RaceTireModel,
                                               T447 TireModelBase / the tyre and wheel files */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/effects/T159_flare.h"                /* FlareClass::addLight / drawReflection, CarLamp */
#include "game/effects/T165_particle_manager.h"     /* ParticleManager::emitSparks */
#include "game/effects/T166_particle_params.h"      /* g_002C03B0 (the spark thresholds) */
#include "game/course/T178_course_env.h"           /* CourseEnv (blurScale, blurExp) */
#include "game/course/T179_course_data.h"          /* CourseData (env) */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar's wheel accessors */
#include "game/car/physics/T289_car_mode.h"             /* car_display_level, CAR_SHOW_* */
#include "game/car/physics/T295_tyre_heat.h"            /* car_brake_heat_sq */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState and its car_* accessors */
#include "game/car/model/T455_car_model_info.h"       /* CarModelInfo, CarAxleSpec, carmodelinfo_tire */
#include "game/car/model/T456_car_model_file.h"       /* CarModelFile, carmodelfile_relocate / _work_area */
#include "engine/io/T504_file_queue.h"           /* file_req_alloc */
#include "engine/pgl/G001_flare.h"                /* the pgl_* / gfx_* / vu0_* calls, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* expf */

/* The brake is on (st.brake > 0) as 1 / 0: ? an inline of the car state in
 * the original; addLamps / drawLampReflections keep it as a bool (retail:
 * the 1 / 0 then `sltu s4, $0, v1'). */
static inline int car_braking(RaceCarState *car)
{
    return car->st.brake > 0.0f ? 1 : 0;
}

/* drawTires' blurPasses by the blur mode (T180's .rodata starts with it). */
const int g_00330B48[4] = { 0, 16, -1, 0 };    /* 0x00330B48 */

// FUNC 0x001A9B90 RaceCarModel::onModelLoaded
//   The car file's file_req_alloc callback: setData(buf).
void RaceCarModel::onModelLoaded(void *self, void *buf)
{
    ((RaceCarModel *)self)->setData((CarModelFile *)buf);
}

// FUNC 0x001A9BB0 RaceCarModel::onTireLoaded
//   The tyre file's callback: setTireData(buf).
void RaceCarModel::onTireLoaded(void *self, void *buf)
{
    ((RaceCarModel *)self)->setTireData(buf);
}

// FUNC 0x001A9BD0 RaceCarModel::onWheelLoaded
//   The wheel file's callback: setWheelData(buf).
void RaceCarModel::onWheelLoaded(void *self, void *buf)
{
    ((RaceCarModel *)self)->setWheelData(buf);
}

// FUNC 0x001A9BF0 RaceCarModel::initArena
//   The car model arena: a 0x498080-byte buffer (malloc) of 0xA8000-byte
//   blocks, 7 car files (_UnitArenaBase::init).  Caller: race_begin (u001).
void RaceCarModel::initArena()
{
    void *buf = malloc(CAR_MODEL_ARENA_SIZE);
    g_002C2F50.init(buf, CAR_MODEL_ARENA_SIZE, CAR_MODEL_BLOCK_SIZE);
}

// FUNC 0x001A9C30 RaceCarModel::freeArena
//   The arena forgets its buffer (_UnitArenaBase::reset), which is freed.
//   Caller: race_end (u001).
void RaceCarModel::freeArena()
{
    free(g_002C2F50.reset());
}

// FUNC 0x001A9C58 RaceCarModel::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).
void *RaceCarModel::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x001A9C80 RaceCarModel::operator delete
//   The size (0x6C0) is ignored (pdistd::Allocator::free).
void RaceCarModel::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x001A9CA8 RaceCarModel::RaceCarModel
//   vptr, CarModel(), the four RaceTireModel() (with the EH cleanup of the
//   constructed ones and ~CarModel), then the fields.  Caller: RaceEntryCar's
//   constructor (u042).  The state is not initialised.
RaceCarModel::RaceCarModel()
{
    /* implicit: CarModel (base constructor), RaceTireModel (constructor of each tyre), ~CarModel (exception cleanup) */
    data = NULL;
    colorIndex = 0;
    tireLoaded = 0;
    wheelLoaded = 0;
    for (int tire = 0; tire < 4; tire++)
        for (int k = 3; k >= 0; k--)
            prevScreen[tire][k] = 0.0f;
}

// FUNC 0x001A9E18 RaceCarModel::~RaceCarModel
//   (RaceCarModel::virtual_0) vptr, release(), the tyres destroyed from the
//   last (~RaceTireModel through their vtables, flags 0), ~CarModel (flags
//   2); deleting form: operator delete(this, 0x6C0).
RaceCarModel::~RaceCarModel()
{
    release();
    /* implicit: ~CarModel (base destructor) */
}

// FUNC 0x001A9ED8 RaceCarModel::copyFrom
//   /* ? */ name: it returns *this (probably the original's operator=).
//   Releases the model, takes src's load flags, state and colour, copies
//   src's whole car file block into a new block (_UnitArenaBase::alloc),
//   relocates it (setData) and sets up the tyres.  Callers:
//   RaceEntryCar::copyFrom / applyPendingModel (u042).
RaceCarModel &RaceCarModel::copyFrom(const RaceCarModel &src)
{
    release();
    tireLoaded = src.tireLoaded;
    state = src.state;
    colorIndex = src.colorIndex;
    wheelLoaded = src.wheelLoaded;
    CarModelFile *file = (CarModelFile *)g_002C2F50.alloc(0);
    data = file;
    u32 size = CAR_MODEL_BLOCK_SIZE;        /* in a variable: retail calls memcpy */
    memcpy(file, src.data, size);
    setData(file);
    setupTires();
    return *this;
}

// FUNC 0x001A9F78 RaceCarModel::release
//   Gives the car file block back (the body forgets it: CarModel::
//   clearModel; _UnitArenaBase::release) and clears the load flags.
//   Callers: the destructor, copyFrom, RaceEntryCar::unloadModel (u042).
void RaceCarModel::release()
{
    if (data != NULL) {
        carModel.clearModel();
        g_002C2F50.release(data);
    }
    data = NULL;
    tireLoaded = 0;
    wheelLoaded = 0;
}

// FUNC 0x001A9FC8 RaceCarModel::load
//   Unless a model is there: takes a block (_UnitArenaBase::alloc), copies
//   the path into it (strcpy) and queues the file's read over it
//   (file_req_alloc, done: onModelLoaded).  Caller: RaceEntryCar::loadModel
//   (u042).
void RaceCarModel::load(const char *path)
{
    if (data == NULL) {
        char *block = (char *)g_002C2F50.alloc(0);
        strcpy(block, path);
        file_req_alloc(block, block, 0, onModelLoaded, this, FILE_REQ_PLAIN);
    }
}

// FUNC 0x001AA048 RaceCarModel::setData
//   The loaded car file (NULL: the read failed): relocated in place
//   (carmodelfile_relocate), its body given to the CarModel (setModel, not
//   relocated again).
void RaceCarModel::setData(CarModelFile *file)
{
    data = file;
    if (file != NULL) {
        carmodelfile_relocate(file);
        carModel.setModel(data->body, 0);
    } else {
        g_002C2F50.release(NULL);           /* quirk: gives back a NULL block */
    }
}

// FUNC 0x001AA0A8 RaceCarModel::loadTire
//   Unless the tyre file is there: the tyre file number of the car's tyre
//   part parts[0] (carmodelinfo_tire; flag: its second table); unless it is
//   the car's default, its name (tire_file_path, "tire/n%d") goes into the
//   car file's tyre buffer and the file is read over it (file_req_alloc,
//   done: onTireLoaded).  The loop runs once.  Caller:
//   RaceEntryCar::loadTires (u042).
void RaceCarModel::loadTire(const int *parts, int flag)
{
    if (tireLoaded == 0) {
        int i = 0;
        do {
            CarModelInfo *info = data->info;
            char *buf = (char *)data->tire;
            int number = carmodelinfo_tire(info, parts[i], flag);
            if (number == info->defaultTire)
                break;
            tire_file_path(buf, number, 0);
            file_req_alloc(buf, buf, 0, onTireLoaded, this, FILE_REQ_PLAIN);
            i++;
        } while (i <= 0);
    }
}

// FUNC 0x001AA178 RaceCarModel::setTireData
//   The loaded tyre file (NULL: failed) is relocated (tire_file_relocate).
void RaceCarModel::setTireData(void *buf)
{
    if (buf != NULL) {
        tire_file_relocate((TireFile2 *)buf);
        tireLoaded = 1;
    }
}

// FUNC 0x001AA1B0 RaceCarModel::loadWheel
//   Unless the wheel file is there: "wheel/<name>" (wheel_file_path) into
//   the car file's wheel buffer, read over it (file_req_alloc, done:
//   onWheelLoaded).  Caller: RaceEntryCar::loadWheel (u042).
void RaceCarModel::loadWheel(const char *name)
{
    if (wheelLoaded == 0) {
        char *buf = (char *)data->wheel;
        wheel_file_path(buf, name, 0);
        file_req_alloc(buf, buf, 0, onWheelLoaded, this, FILE_REQ_PLAIN);
    }
}

// FUNC 0x001AA218 RaceCarModel::setWheelData
//   The loaded wheel file (NULL: failed) is relocated (wheel_file_relocate).
void RaceCarModel::setWheelData(void *buf)
{
    if (buf != NULL) {
        wheel_file_relocate((WheelFile2 *)buf);
        wheelLoaded = 1;
    }
}

// FUNC 0x001AA250 RaceCarModel::setupTires
//   Each tyre: its position, colour, the tyre and wheel files
//   (TireModelBase's inline setters), its
//   axle's brake from the car file (RaceTireModel::setBrake: disc, caliper,
//   disc radius / offset, caliper angle; wheels 0, 1 front, 2, 3 rear) and
//   its size from the state's drivetrain (TireModelBase::setSizeProfile:
//   tyreWidth, tyreRadius, tyreSidewall of the axle); the tyres take their
//   work spaces one after the other from the end of the car file's data
//   (carmodelfile_work_area, RaceTireModel::build).  (The original indexes
//   the car file's records by wheel / 2 and the state's by wheel >> 1.)
//   Callers: copyFrom, RaceEntryCar::setupTires (u042).
void RaceCarModel::setupTires()
{
    void *work = carmodelfile_work_area(data);
    RaceCarState *car = state;
    int color = colorIndex;
    CarModelInfo *info = data->info;
    for (int wheel = 0; wheel < 4; wheel++) {
        tires[wheel].setPosition(wheel);
        tires[wheel].setColor(color);
        tires[wheel].setWheelFile(data->wheel);
        tires[wheel].setTireFile(data->tire);
        RaceTireModel *tire = &tires[wheel];
        CarAxleSpec *axle = &info->axles[wheel / 2];
        /* CarAxleSpec (T455): b the disc, a the caliper, c / d the disc's
         * radius and offset, e the caliper's angle in degrees */
        tire->setBrake(axle->c, axle->d, axle->e, axle->b, axle->a);
        GearCalc *gear = &car->params.gear;
        tire->setSizeProfile(gear->tyreWidth[wheel >> 1], gear->tyreRadius[wheel >> 1],
                             gear->tyreSidewall[wheel >> 1]);
        work = tire->build(work);
    }
}

// FUNC 0x001AA3A0 RaceCarModel::loadBodyMatrix
//   Multiplies the current matrix by the body frame at t: translate to the
//   view position (car_view_x / y / z), then rotate y (car_yaw), x
//   (car_pitch), -z (car_roll), in degrees.
void RaceCarModel::loadBodyMatrix(f32 t)
{
    RaceCarState *car = state;
    f32 x = car_view_x(car, t);
    f32 y = car_view_y(car, t);
    pgl_translate(x, y, car_view_z(car, t));
    vu0_rotate_y(car_yaw(car, t) * RAD_TO_DEG);
    vu0_rotate_x(car_pitch(car, t) * RAD_TO_DEG);
    vu0_rotate_z(-(car_roll(car, t) * RAD_TO_DEG));
}

// FUNC 0x001AA460 RaceCarModel::setReflectionPlane
//   The rotation of the ground frame at t (car_yaw2, car_ground_pitch,
//   car_ground_roll) gives its up vector (vf29, read between
//   pgl_push_matrix / pgl_pop_matrix); mat_reflect builds the mirror matrix
//   of the plane through the ground position (car_ground_view_x / y / z)
//   with that normal.  Caller: RaceEntryCar::setReflectionPlane (u042).
void RaceCarModel::setReflectionPlane(f32 t)
{
    RaceCarState *car = state;
    f32 up[3];                              /* sp+0x00 */
    f32 x = car_ground_view_x(car, t);
    f32 y = car_ground_view_y(car, t);
    f32 z = car_ground_view_z(car, t);
    pgl_push_matrix();
    vu0_load_identity_regs();
    vu0_rotate_y(car_yaw2(car, t) * RAD_TO_DEG);
    vu0_rotate_x(car_ground_pitch(car, t) * RAD_TO_DEG);
    vu0_rotate_z(-(car_ground_roll(car, t) * RAD_TO_DEG));
    vu0_get_y_axis(up);
    pgl_pop_matrix();
    mat_reflect(x, y, z, up[0], up[1], up[2]);
}

// FUNC 0x001AA568 RaceCarModel::F_001AA568
//   Empty.  Caller: RaceEntryCar::F_00177878 (u042).
void RaceCarModel::F_001AA568()
{
}

// FUNC 0x001AA570 RaceCarModel::loadGroundMatrix
//   loadBodyMatrix with the ground frame: car_ground_view_x / y / z,
//   car_yaw2, car_ground_pitch, car_ground_roll.
void RaceCarModel::loadGroundMatrix(f32 t)
{
    RaceCarState *car = state;
    f32 x = car_ground_view_x(car, t);
    f32 y = car_ground_view_y(car, t);
    pgl_translate(x, y, car_ground_view_z(car, t));
    vu0_rotate_y(car_yaw2(car, t) * RAD_TO_DEG);
    vu0_rotate_x(car_ground_pitch(car, t) * RAD_TO_DEG);
    vu0_rotate_z(-(car_ground_roll(car, t) * RAD_TO_DEG));
}

// FUNC 0x001AA630 RaceCarModel::drawShadow
//   The body's shadow (CarModel::drawShadow) in the ground frame.  Caller:
//   RaceEntryCar::drawShadow (u042; once per CarRenderCtx pass, T181: the
//   shadow of each light).
void RaceCarModel::drawShadow(f32 t)
{
    pgl_push_matrix();
    loadGroundMatrix(t);
    carModel.drawShadow();
    pgl_pop_matrix();
}

// FUNC 0x001AA680 RaceCarModel::F_001AA680
//   The body's CarModel::F_0021F2E0 (part 5, a ground pass) in the ground
//   frame.  Caller: RaceEntryCar::F_001778C0 (u042).
void RaceCarModel::F_001AA680(f32 t)
{
    pgl_push_matrix();
    loadGroundMatrix(t);
    carModel.F_0021F2E0();
    pgl_pop_matrix();
}

// FUNC 0x001AA6D0 RaceCarModel::F_001AA6D0
//   The body's CarModel::F_0021F258 (part 6 in the shadow colour, a ground
//   pass) in the ground frame.  Caller: RaceEntryCar::F_001778E8 (u042).
void RaceCarModel::F_001AA6D0(f32 t)
{
    pgl_push_matrix();
    loadGroundMatrix(t);
    carModel.F_0021F258();
    pgl_pop_matrix();
}

// FUNC 0x001AA720 RaceCarModel::drawBody
//   The body (CarModel::draw) with the model's colour between beginFade /
//   endFade, in the body frame at t when withMatrix != 0 (else in the
//   current matrix).  Callers: RaceEntryCar::drawBody (u042, withMatrix 1),
//   CarIconMaker::drawIcon (u061, 0).
void RaceCarModel::drawBody(f32 t, int withMatrix)
{
    CarModel *body = &carModel;
    body->color = colorIndex;
    pgl_push_matrix();
    body->beginFade();
    if (withMatrix != 0)
        loadBodyMatrix(t);
    body->draw();
    body->endFade();
    pgl_pop_matrix();
}

// FUNC 0x001AA7A0 RaceCarModel::drawTires
//   The four tyres in the body frame at t, each at its wheel position
//   (car_wheel_lateral_pos, car_wheel_spring, car_wheel_axle_x) turned by
//   its steer / camber / rotation (degrees), with RaceTireModel::draw at
//   the CarModel LOD lod and blurPasses g_00330B48[blur] ({0, 16, -1, 0}).
//   With blur != 0 each tyre's screen position (VU0: vf24..vf27, clamped by
//   vf21 after vu0_load_screen_regs, divided by w) is compared with the
//   last frame's (prevScreen); its screen speed v gives the blur angle:
//   the wheel's rotation at blurScale * expf(blurExp * v) less its rotation
//   at t (radians).  The glow of a heat brake: scale (CourseEnv::brakeGlowScale,
//   T131) x car_brake_heat_sq.  Caller: RaceEntryCar::drawTires (u042).
//   Matching (98.4%, the same with 991111-01): only the three scratch
//   registers of the prevScreen addresses differ (retail: wheel * 16 in $v0,
//   ours $a0; a local-alloc order).  Tried: a prev pointer (hoists this +
//   0x66C, 70%), the statement macro around the deltas / the whole block,
//   all 90 orders of the delta / prevScreen statements (the natural one is
//   best).
void RaceCarModel::drawTires(int blur, f32 scale, f32 t, int lod)
{
    CarModel *body = &carModel;
    RaceCarState *car = state;
    int blurPasses = g_00330B48[blur];
    CourseEnv *env = car->course->data->env;
    f32 blurAngle = 0.0f;
    pgl_push_matrix();
    body->beginFade();
    loadBodyMatrix(t);
    for (int wheel = 0; wheel < 4; wheel++) {
        f32 pos[4];                         /* sp+0x00 */
        pos[0] = car_wheel_lateral_pos(car, wheel, t);
        pos[1] = car_wheel_spring(car, wheel, t);
        pos[2] = car_wheel_axle_x(car, wheel, t);     /* t: not read */
        pos[3] = 1.0f;
        f32 rotation = car_wheel_rotation(car, wheel, t);
        if (blur != 0) {
            f32 delta[3];                   /* sp+0x10 (stored, not read again) */
            f32 screen[4];                  /* sp+0x20 */
            __asm__ volatile("vnop");
            vu0_load_screen_regs();
            do { vu0_project(screen, pos); } while (0);    /* (a statement macro: retail
                                                               aligns the label after it) */
            delta[0] = screen[0] - prevScreen[wheel][0];
            delta[1] = screen[1] - prevScreen[wheel][1];
            delta[2] = screen[2] - prevScreen[wheel][2];
            prevScreen[wheel][0] = screen[0];
            prevScreen[wheel][1] = screen[1];
            prevScreen[wheel][2] = screen[2];
            f32 timeScale = env->blurScale;
            f32 speed = boot_sqrtf_v(delta[0] * delta[0] + delta[1] * delta[1]);
            blurAngle = car_wheel_rotation(car, wheel, timeScale * expf(env->blurExp * speed)) - rotation;
        }
        pgl_push_matrix();
        f32 spin = rotation * RAD_TO_DEG;
        pgl_translate(pos[0], pos[1], pos[2]);
        RaceTireModel *tire = &tires[wheel];
        f32 steer = car_wheel_steer(car, wheel, t) * RAD_TO_DEG;
        f32 camber = car_wheel_camber(car, wheel, t) * RAD_TO_DEG;
        f32 heat = car_brake_heat_sq(car, wheel, t);
        tire->draw(lod, steer, camber, spin, heat * scale, blurPasses, blurAngle);
        pgl_pop_matrix();
    }
    body->endFade();
    pgl_pop_matrix();
}

// FUNC 0x001AAA90 RaceCarModel::drawTireCylinders
//   (u060 drawWheels.)  The four tyres as plain cylinders
//   (RaceTireModel::drawCylinder, the lowest level; RacePS2Base::drawCars
//   with lod > 0), not the wheel models.  In the body frame at t, at the
//   wheel positions, steer / camber / rotation in degrees; the first float
//   argument (u045: CourseEnv::brakeGlowScale) is not used.  Caller:
//   RaceEntryCar::drawTireCylinders (u042).
void RaceCarModel::drawTireCylinders(f32 unused, f32 t)
{
    CarModel *body = &carModel;
    RaceTireModel *tire = tires;
    RaceCarState *car = state;
    pgl_push_matrix();
    body->beginFade();
    loadBodyMatrix(t);
    for (int wheel = 0; wheel < 4; wheel++) {
        pgl_push_matrix();
        f32 x = car_wheel_lateral_pos(car, wheel, t);
        f32 y = car_wheel_spring(car, wheel, t);
        pgl_translate(x, y, car_wheel_axle_x(car, wheel, t));
        f32 steer = car_wheel_steer(car, wheel, t) * RAD_TO_DEG;
        f32 camber = car_wheel_camber(car, wheel, t) * RAD_TO_DEG;
        f32 rotation = car_wheel_rotation(car, wheel, t);
        tire->drawCylinder(steer, camber, rotation * RAD_TO_DEG);
        tire++;
        pgl_pop_matrix();
    }
    body->endFade();
    pgl_pop_matrix();
}

// FUNC 0x001AABE8 RaceCarModel::drawMenuTires
//   The four tyres of a showroom car (the ShowRoomCar record of
//   CarIconMaker::drawIcon, u061) at its wheel positions (wheelLateralPos,
//   wheelHeight, wheelAxleX; steer and stroke 0): first
//   RaceTireModel::drawCylinder into the alpha only (no depth writes,
//   gfx_set_frame_write_mask 0xFF000000), then RaceTireModel::draw at lod 0
//   (no view test) with all writes, no blur and no glow.
void RaceCarModel::drawMenuTires(ShowRoomCar *car)
{
    pgl_push_matrix();
    for (int wheel = 0; wheel < 4; wheel++) {
        pgl_push_matrix();
        f32 x = car->wheelLateralPos(wheel, 0.0f, 0.0f);
        f32 y = car->wheelHeight(wheel, 0.0f, 0.0f);
        pgl_translate(x, y, car->wheelAxleX(wheel, 0.0f, 0.0f));
        f32 steer = car->wheelSteerAngle(wheel, 0.0f, 0.0f) * RAD_TO_DEG;
        f32 camber = car->wheelCamber(wheel, 0.0f, 0.0f) * RAD_TO_DEG;
        gfx_depth_mask(0);
        gfx_set_frame_write_mask(FRAME_WRITE_ALPHA);
        RaceTireModel *tire = &tires[wheel];
        tire->drawCylinder(steer, camber, 0.0f);
        gfx_depth_mask(1);
        gfx_set_frame_write_mask(FRAME_WRITE_RGBA);
        tire->draw(0, steer, camber, 0.0f, 0.0f, 0, 0.0f);
        pgl_pop_matrix();
    }
    pgl_pop_matrix();
}

// FUNC 0x001AAD70 RaceCarModel::F_001AAD70
//   Empty.  Caller: RaceEntryCar::F_00177988 (u042).
void RaceCarModel::F_001AAD70(f32 t)
{
}

// FUNC 0x001AAD78 RaceCarModel::F_001AAD78
//   The body's CarModel::F_0021F010 (part 4) in the body frame.  Caller:
//   RaceEntryCar::F_001779B0 (u042, itself uncalled).
void RaceCarModel::F_001AAD78(f32 t)
{
    CarModel *body = &carModel;
    pgl_push_matrix();
    loadBodyMatrix(t);
    body->F_0021F010();
    pgl_pop_matrix();
}

// FUNC 0x001AADD0 RaceCarModel::emitSparks
//   When the car is drawn (car_display_level >= CAR_SHOW_GHOST), the car file has
//   spark points and the car is faster than g_002C03B0.sparkSpeed km/h
//   (|v|^2 against (speed / 3.6)^2): the road plane under the body's origin
//   (RaceCourse::getPlane from the state's sector hint), for the surfaces
//   0, 1, 7 and 9 only (the ones ground_surface_type, T347, turns into wet
//   codes: ? the paved ones); each spark point of the car, in the body
//   frame, that is below the road (or always, sparkAlways) throws
//   clamp((road - y) * sparkPerDepth, sparkMin, sparkMax) sparks
//   (ParticleManager::emitSparks) from 0.05 above the road.  The direction
//   is the velocity in view axes (v.x, v.z, -v.y).  Caller:
//   RaceEntryCar::emitSparks (u042).
//   Matching (98.5%; 991111-01 93.6%): after the first translation read,
//   retail sets $a0 before loading RAD_TO_DEG (ours: the load first: a
//   sched2 tie behind the volatile asm); in the loop, retail loads pos[1]
//   late and reuses $f3 for it (ours: sched1 issues that load first, $f5),
//   with plane[0] / pos[0] the other way round.  Tried: the products'
//   operand orders (24 forms), inline plane-height helpers, pos[1] in a
//   local, the comparison reversed or negated, a flag variable, the asm
//   non-volatile (worse) or with a memory clobber (no change).
void RaceCarModel::emitSparks(ParticleManager *pm, f32 t)
{
    RaceCarState *car = state;
    RaceCourse *course = car->course;
    CarModelInfo *info = data->info;
    f32 pos[3];                             /* sp+0x00 */
    f32 plane[4];                           /* sp+0x10 */
    f32 dir[3];                             /* sp+0x20 */
    int sector;                             /* sp+0x30 */
    int attr;                               /* sp+0x34 */

    if (car_display_level(car) < CAR_SHOW_GHOST)
        return;
    if (info->sparkCount == 0)
        return;
    f32 *vel = car->st.vel;
    ParticleParams *pp = &g_002C03B0;
    dir[0] = vel[0];
    dir[1] = vel[2];
    dir[2] = -vel[1];
    if (dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]
        < pp->sparkSpeed * pp->sparkSpeed * KMH_TO_MS_SQ)
        return;

    pgl_push_matrix();
    vu0_load_identity_regs();
    f32 x = car_view_x(car, t);
    f32 y = car_view_y(car, t);
    pgl_translate(x, y, car_view_z(car, t));
    vu0_get_translation(pos);
    vu0_rotate_y(car_yaw(car, t) * RAD_TO_DEG);
    vu0_rotate_x(car_pitch(car, t) * RAD_TO_DEG);
    vu0_rotate_z(-(car_roll(car, t) * RAD_TO_DEG));
    attr = 0;
    sector = car->st.sector;
    course->getPlane(pos, &sector, &attr, plane);
    if (sector < 0) {
        pgl_pop_matrix();
        return;
    }
    do {                                    /* (a statement macro in the original:
                                               retail aligns the case tree's label
                                               as the first one in a loop) */
        switch (attr) {
        case 0:
        case 1:
        case 7:
        case 9:
            break;
        default:
            pgl_pop_matrix();
            return;
        }
    } while (0);
    for (int i = 0; i < info->sparkCount; i++) {
        pgl_push_matrix();
        f32 *point = info->sparks[i];
        pgl_translate(point[0], point[1], point[2]);
        vu0_read_translation(pos);          /* (no vnop here in the original) */
        f32 road = -(pos[0] * plane[0] + pos[2] * plane[2] + plane[3]) / plane[1];
        if (pos[1] < road || pp->sparkAlways != 0) {
            f32 depth = road - pos[1];
            pos[1] = road + SPARK_LIFT;
            int count = (int)(depth * pp->sparkPerDepth);
            if (count < pp->sparkMin)
                count = pp->sparkMin;
            if (pp->sparkMax < count)
                count = pp->sparkMax;
            pm->emitSparks(pos, dir, plane, count);
        }
        pgl_pop_matrix();
    }
    pgl_pop_matrix();
}

// FUNC 0x001AB0F8 RaceCarModel::updateBodyState
//   (u060 updateLamps.)  The state to the body's model switches, not only
//   the lamps: CarModel::pushLamps(the brake is on: st.brake > 0, the brake
//   lamps), setWheelShake(wheel, car_wheel_shake_at) for the four wheels,
//   setDirt(car_dirt) and setSteer(car_steer_view).  Callers:
//   RaceEntryCar::updateBodyState (u042), RacePS2Base::virtual_29 (u044).
void RaceCarModel::updateBodyState()
{
    RaceCarState *car = state;
    CarModel *body = &carModel;
    body->pushLamps(car->st.brake > 0.0f ? 1 : 0);
    for (int wheel = 0; wheel < 4; wheel++)
        body->setWheelShake(wheel, car_wheel_shake_at(car, wheel, 0.0f));
    body->setDirt(car_dirt(car));
    body->setSteer(car_steer_view(car, 0.0f));
}

// FUNC 0x001AB1B8 RaceCarModel::addLamps
//   When the car is drawn (car_display_level >= CAR_SHOW_GHOST), in the body frame
//   at t, the car file's lamps go to the flare (FlareClass::addLight): list
//   A (directional, 1.0); list C, the brake lamps, when the body's brake
//   lamp history is partly set (CarModelLamps::fraction > 0): the flagged
//   (bit 0) lamps at that fraction, the others at 1.0 while braking now;
//   list B (not directional, 1.0).  Caller: RaceEntryCar::addLamps (u042).
void RaceCarModel::addLamps(FlareClass *flare, f32 t)
{
    bool braking = car_braking(state);
    CarModelInfo *info = data->info;
    if (car_display_level(state) < CAR_SHOW_GHOST)
        return;
    pgl_push_matrix();
    loadBodyMatrix(t);
    for (int i = 0; i < info->lampCountA; i++)
        flare->addLight(&info->lampsA[i], 1.0f, 1);
    f32 lit = carModel.lamps.fraction();
    if (lit > 0.0f) {
        for (int i = 0; i < info->lampCountC; i++) {
            CarLamp *lamp = &info->lampsC[i];
            if (lamp->flags & 1)
                flare->addLight(lamp, lit, 1);
            else if (braking)
                flare->addLight(lamp, 1.0f, 1);
        }
    }
    for (int i = 0; i < info->lampCountB; i++)
        flare->addLight(&info->lampsB[i], 1.0f, 0);
    pgl_pop_matrix();
}

// FUNC 0x001AB370 RaceCarModel::drawLampReflections
//   addLamps with FlareClass::drawReflection, no display level test, list
//   B at 0.3.  (The original leaves list C's count in $a2 for the list C
//   calls; drawReflection takes no such argument.)  Caller:
//   RaceEntryCar::drawLampReflections (u042).
void RaceCarModel::drawLampReflections(FlareClass *flare, f32 t)
{
    bool braking = car_braking(state);
    CarModelInfo *info = data->info;
    pgl_push_matrix();
    loadBodyMatrix(t);
    for (int i = 0; i < info->lampCountA; i++)
        flare->drawReflection(&info->lampsA[i], 1.0f);
    f32 lit = carModel.lamps.fraction();
    if (lit > 0.0f) {
        for (int i = 0; i < info->lampCountC; i++) {
            CarLamp *lamp = &info->lampsC[i];
            if (lamp->flags & 1)
                flare->drawReflection(lamp, lit);
            else if (braking)
                flare->drawReflection(lamp, 1.0f);
        }
    }
    for (int i = 0; i < info->lampCountB; i++)
        flare->drawReflection(&info->lampsB[i], LAMP_B_REFLECTION);
    pgl_pop_matrix();
}

/* The car model arena (.data 0x002C2F50: egcs puts a public object that
 * has a constructor in .data, zero-filled). */
UnitArena<64> g_002C2F50;                   /* 0x002C2F50 */

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002C2F50, emitted at the end of the unit, the destructor first. */
// FUNC 0x001AB508 race_car_model_static_dtor (_GLOBAL_$D$g_00330B48)
//   .dtors entry (0x0028F220): UnitArena<64>::~UnitArena of the car model
//   arena, inlined (the vptr 0x0032DD60, then _UnitArenaBase::
//   ~_UnitArenaBase with flags 2).
/* implicit: ~_UnitArenaBase (base destructor, inlined ~UnitArena) */

// FUNC 0x001AB538 race_car_model_static_ctor (_GLOBAL_$I$g_00330B48)
//   .ctors entry (0x0028F140): the car model arena (UnitArena<64>'s inline
//   constructor: _UnitArenaBase::_UnitArenaBase, then the vptr 0x0032DD60).
/* implicit: _UnitArenaBase (base constructor, inlined UnitArena constructor) */

// FUNC 0x001AB570 UnitArena<64>::buildFreeList
/* template code in game/race/core/T153_race_course.h (UnitArena<N>::buildFreeList,
 * index: virtual_1); egcs emits this dead weak copy in T180 (T153's
 * 0x0018FE98 is the one linked; the vtable copy 0x00330B68 points to T153's
 * members too) */

// FUNC 0x001AB5E8 UnitArena<64>::~UnitArena
/* inline in game/race/core/T153_race_course.h (index: virtual_0: the vptr, then
 * _UnitArenaBase::~_UnitArenaBase with the caller's flags); egcs emits this
 * dead weak copy in T180 (T153's is 0x0018FF10) */
/* implicit: ~_UnitArenaBase (base destructor) */

/* The accessors: inline in the original, defined here (out of the class:
 * the other units call getCarModel out of line, T121 / T131 / T184, so
 * they did not see the bodies).  The class's key function is here, so egcs
 * writes every one of them, after the static destructor / constructor and
 * UnitArena<64>'s members, in the reverse of their definition order
 * (retail's frame info): defined from setState (0x001AB730) back to
 * releaseArena (0x001AB610).  setModelLod is defined before getCarModel,
 * so its call to getCarModel stays (retail calls 0x001AB6E0).  GCC 14:
 * plain functions (the other units call them), except allocBlock /
 * freeBlock (the header's inline bodies, below). */
#ifdef BOOT_OLD_GCC
#define T180_INLINE inline
#else
#define T180_INLINE
#endif

// FUNC 0x001AB730 RaceCarModel::setState
//   No callers.
T180_INLINE void RaceCarModel::setState(RaceCarState *s)
{
    state = s;
}

// FUNC 0x001AB728 RaceCarModel::getState
//   No callers.
T180_INLINE RaceCarState *RaceCarModel::getState()
{
    return state;
}

// FUNC 0x001AB720 RaceCarModel::setColorIndex
//   No callers.
T180_INLINE void RaceCarModel::setColorIndex(int c)
{
    colorIndex = c;
}

// FUNC 0x001AB718 RaceCarModel::getColorIndex
//   No callers.
T180_INLINE int RaceCarModel::getColorIndex()
{
    return colorIndex;
}

// FUNC 0x001AB6E8 RaceCarModel::setModelLod
//   (u060 setDrawMode.)  No callers: it sets the body's fixed LOD
//   (CarModel::Callback::lod, +0x08; u045 / u061 CarDrawParam::mode), as
//   RaceEntryCar::setModelLod does, through getCarModel and T121's
//   F_00179460 (egcs: through the helper above, see there).
T180_INLINE void RaceCarModel::setModelLod(int lod)
{
#ifdef BOOT_OLD_GCC
    race_car_model_lod_param(getCarModel())->callback.lod = lod;
#else
    F_00179460(getCarModel())->callback.lod = lod;
#endif
}

// FUNC 0x001AB6E0 RaceCarModel::getCarModel
//   Callers: RaceEntryCar::setModelLod (u042), RacePS2Base::drawCars /
//   virtual_51 (u045), setDrawMode, CarIconMaker::drawIcon (u061).
T180_INLINE CarModel *RaceCarModel::getCarModel()
{
    return &carModel;
}

// FUNC 0x001AB6C8 RaceCarModel::getTire
//   No callers.
T180_INLINE RaceTireModel *RaceCarModel::getTire(int i)
{
    return &tires[i];
}

// FUNC 0x001AB6B8 RaceCarModel::getInfo
//   No callers (RaceEntryCar::getModelInfo has the same code).
T180_INLINE CarModelInfo *RaceCarModel::getInfo()
{
    return data->info;
}

/* allocBlock / freeBlock: inline in the header too, for T139 (RaceSolitaire's
 * virtual_57 / destructor) and T146 (LicenseExaminationMessages' constructor
 * / destructor), which expand them (GAME_T180_INLINE_BLOCK_ACCESSORS).
 * egcs: defined here, not from the header (T180 does not define the macro):
 * the header's bodies would be read before getInfo .. setState, and their
 * copies would come out after setState's.  GCC 14: the header's. */
#ifdef BOOT_OLD_GCC
// FUNC 0x001AB690 RaceCarModel::allocBlock
//   A block of the arena (_UnitArenaBase::alloc).  This copy has no
//   callers (T139 / T146 expand the body).
inline void *RaceCarModel::allocBlock()
{
    return g_002C2F50.alloc(0);
}

// FUNC 0x001AB668 RaceCarModel::freeBlock
//   The block back to the arena (_UnitArenaBase::release).  This copy has
//   no callers (T139 / T146 expand the body).
inline void RaceCarModel::freeBlock(void *block)
{
    g_002C2F50.release(block);
}
#endif

// FUNC 0x001AB630 RaceCarModel::setArena
//   No callers.  The arena over buf (0xA8000-byte blocks,
//   _UnitArenaBase::init); returns the previous buffer.
T180_INLINE void *RaceCarModel::setArena(void *buf, u32 size)
{
    return g_002C2F50.init(buf, size, CAR_MODEL_BLOCK_SIZE);
}

// FUNC 0x001AB610 RaceCarModel::releaseArena
//   No callers.  The arena forgets its buffer (_UnitArenaBase::reset) and
//   returns it.
T180_INLINE void *RaceCarModel::releaseArena()
{
    return g_002C2F50.reset();
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001AB738 __tf12RaceCarModel
//   The __user_type_info __ti12RaceCarModel (gp-0x4620), name
//   "12RaceCarModel".
extern "C" const type_info &__tf12RaceCarModel(void)
{
    if (__ti12RaceCarModel[0] == 0)
        __rtti_user(__ti12RaceCarModel, "12RaceCarModel");
    return *(const type_info *)__ti12RaceCarModel;
}
#endif

/* (0x001AB778: T180's weak copy of F_00179460, its marker at the top.) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001AB780 __tft9UnitArena1Ui64
//   T180's dead weak copy of T153's 0x00190148: the __si_type_info
//   __tit9UnitArena1Ui64, base __ti14_UnitArenaBase (__tf14_UnitArenaBase
//   first; gp-0x4580).
extern "C" __attribute__((weak)) const type_info &__tft9UnitArena1Ui64(void)
{
    if (__tit9UnitArena1Ui64[0] == 0) {
        __tf14_UnitArenaBase();
        __rtti_si(__tit9UnitArena1Ui64, "t9UnitArena1Ui64",
                  (const type_info *)__ti14_UnitArenaBase);
    }
    return *(const type_info *)__tit9UnitArena1Ui64;
}
#endif
