/* T121_race_entry.cpp - RaceEntryInformation, RaceEntryCar, RaceEntryBase
 * (0x00177108..0x00179468).
 *
 * A RaceEntryCar is one car of a race: its car data (RaceEntryInformation =
 * CarEquipments + the id of the input that drives it), lap times and
 * records, the model (RaceCarModel, T180), the sound (CarSound, T176), a few
 * per-entry options and the driver name / date of the record.  RaceEntryBase
 * is the list of the entries; it links them by distance to a point (the draw
 * order) and by the cars' race position.
 */
/* Retail's .rodata (0x00327350..0x00327412): "bad_alloc" (<new>'s inline
 * what(), written when the header is read), the vtables of RaceEntryBase,
 * RaceEntryCar and RaceEntryInformation, then the type names in the reverse
 * order of the classes' definitions: RaceEntryBase, RaceEntryCar,
 * RaceEntryInformation, RaceCameraBase, UnitArena<64>.  So the camera and
 * course headers come before the file's own, and nothing that defines the
 * menu classes (CdClock from T519_pdicdvd.h, not T092's header). */
// DATA_START 0x00327350
#include <new>
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_camera_manager.h"       /* CameraManager, CameraView (targetInfo, camera); RaceCameraBase */
#include "game/race/core/T121_race_entry.h"           /* T482 CarEquipments, T126..T128, T180 RaceCarModel,
                                               T176 CarSound, T280 CarCondition */
#include "libc.h"                           /* strcpy */
#include "game/race/core/T123_race_setup.h"           /* the race pool g_002B3BF0 */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (st: rpm, rank, engineSnd ...), car_view_*, car_wheel_pos */
#include "game/sound/T288_car_sound.h"            /* car_sound_wind / road / gear_whine, car_crash_sound_get */
#include "game/car/physics/T287_surface_grip.h"         /* the seven wheel voice evaluators tyre_sound_squeal_0789 .. tyre_sound_squeal_2_4 */
#include "game/car/physics/T285_table.h"                /* F_001DE050, the gain table of the listening modes */
#include "game/race/dynamics/T303_race_time_events.h"     /* car_best_lap */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase (doppler, position history) */
#include "game/car/model/T446_car_model.h"            /* car_model_path, CarModel::Callback::lod */
#include "game/car/model/T456_car_model_file.h"       /* CarModelFile (RaceCarModel::data: info) */
#include "engine/pdi/T519_pdicdvd.h"              /* struct CdClock */
#include "game/sound/T465_clock.h"                /* clock_get_local */
#include "game/data/T485_car_color.h"            /* CarColorDataBase g_00352D98 */
#include "engine/pgl/G001_flare.h"                /* pgl_matrix_mode, vu0_load_identity_regs, vu0_apply_matrix */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_user */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __class_type_info::base_info */
#endif
#include "game/race/core/race_entry_internal.h"                           /* the car sound frame's constants, g_002B2CC0 */

/* A position (four floats) copied with a count-down loop (retail: li 3,
 * li -1, bne; a for over an index is reversed into bgez).  The pointers
 * are copied into locals: stepping the parameters themselves gives the
 * count the source's register (retail: count a0, source v0 / v1). */
static inline void copy_pos(f32 *dst, const f32 *src)
{
    f32 *d = dst;
    const f32 *s = src;
    int n = 4;
    while (n--) {
        *d = *s;
        d++;
        s++;
    }
}

/* A BCD byte of the CD clock as a number (an int: retail shifts with sra). */
static inline int bcd_to_int(int bcd)
{
    return (bcd & 0xF) + (bcd >> 4) * 10;
}

/* ======================================================================== */
/* RaceEntryInformation                                                     */
/* ======================================================================== */

// FUNC 0x00177108 RaceEntryInformation::RaceEntryInformation
//   CarEquipments::CarEquipments, then the vptr (+0x1DC) and inputId.
RaceEntryInformation::RaceEntryInformation()
{
    inputId = 0;

    /* implicit: CarEquipments (the base's constructor) */
}

// FUNC 0x00177140 RaceEntryInformation::~RaceEntryInformation
//   (slot 0.)  Only the vptr store: it passes its flags unchanged to
//   CarEquipments::~CarEquipments, which does the operator delete.
RaceEntryInformation::~RaceEntryInformation()
{
    /* implicit: ~CarEquipments (the base / member destructors) */
}

/* ======================================================================== */
/* RaceEntryCar                                                             */
/* ======================================================================== */

// FUNC 0x00177168 RaceEntryCar::operator new
void *RaceEntryCar::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00177190 RaceEntryCar::operator delete
void RaceEntryCar::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x001771B8 RaceEntryCar::RaceEntryCar
//   Members constructed in order: info (RaceEntryInformation), lapTime
//   (RaceLapTime), sessionBest / lapRecord (LapRecord), timeRecord
//   (TimeRecord), model (RaceCarModel), sound (CarSound), condition
//   (CarCondition); a throw destroys the ones built (~CarSound,
//   ~RaceCarModel, ~RaceLapTime, ~RaceEntryInformation).  rankPrev /
//   rankNext stay as they are.
RaceEntryCar::RaceEntryCar()
{
    distance = 0.0f;                        /* sw $zero */
    sortPrev = 0;
    sortNext = 0;
    state = 0;
    input = 0;
    pendingModel = 0;
    soundLoaded = 0;
    order = 0;
    transmission = 0;
    tyre = 0;
    steeringAssist = 0;
    vibration = 0;
    points = 0;
    topSpeed = 0.0f;                        /* sw $zero */
    number = -1;
    ghostLapTime = RACE_TIME_INVALID;
    modelEnabled = 1;                       /* this order of the three: retail's store order */
    assistAsm = 1;
    assistTcs = 1;

    /* implicit: RaceEntryInformation, RaceLapTime, LapRecord, TimeRecord,
     * RaceCarModel, CarSound, CarCondition (the members' constructors),
     * ~CarSound, ~RaceCarModel, ~RaceLapTime, ~RaceEntryInformation (their
     * cleanup on a throw) */
    strcpy(driverName, g_00352388[0]);      /* "SESSION BEST" (T187's RACEMSG text) */
    setDate(0);
}

// FUNC 0x00177350 RaceEntryCar::~RaceEntryCar
//   (slot 0.)  Then ~CarSound, ~RaceCarModel, ~RaceLapTime,
//   ~RaceEntryInformation (flags 2) and, with bit 0, operator delete(this,
//   0x2148).
RaceEntryCar::~RaceEntryCar()
{
    /* implicit: ~RaceEntryInformation, ~RaceLapTime, ~CarSound, ~RaceCarModel
     * (the base / member destructors) */
    unloadModel();
}

// FUNC 0x001773D0 RaceEntryCar::unloadModel
//   The model's block given back (RaceCarModel::release); the engine sound
//   is to be loaded again and a deferred model is forgotten.  Callers: the
//   destructor, RaceOrganization::unloadModels, RaceBase::virtual_19.
void RaceEntryCar::unloadModel()
{
    model.release();
    pendingModel = 0;
    soundLoaded = 0;
}

// FUNC 0x00177400 RaceEntryCar::updateGhostLapTime
//   ghostLapTime = the car's best lap (car_best_lap: RaceCarState::bestLap).
//   Caller: RaceOrganization::F_0017AD28 on car 0, the time the ghost of
//   the next run has to beat.
void RaceEntryCar::updateGhostLapTime()
{
    ghostLapTime = car_best_lap(state);
}

// FUNC 0x00177430 RaceEntryCar::updateColorIndex
//   The model's colour index: the index of the car's colour in its colour
//   list (CarColorDataBase::getColorIndex), 0 when it is not there.
void RaceEntryCar::updateColorIndex()
{
    RaceEntryInformation *car = &info;
    int index = g_00352D98.getColorIndex(car->getModelCode(), car->colorCode);
    model.colorIndex = (index > -1) ? index : 0;
}

// FUNC 0x00177488 RaceEntryCar::copyFrom
//   Takes over another entry (RaceOrganization::setupGhost, the second car):
//   its car data (CarEquipments::operator=, inputId), lap times, options, its
//   model (now, or later by applyPendingModel when deferModel != 0) and a
//   copy of its sound.
void RaceEntryCar::copyFrom(RaceEntryCar *src, int deferModel)
{
    info = src->info;                       /* CarEquipments::operator= + inputId */
    /* implicit: CarEquipments::operator= (RaceEntryInformation's implicit
     * operator=) */
    lapTime = src->lapTime;                 /* all the data, the vptr is kept */
    info.setInputID(1);                     /* quirk: the copied input id is overwritten with 1 */
    transmission = src->transmission;
    order = src->order;
    tyre = src->tyre;
    steeringAssist = src->steeringAssist;
    assistAsm = src->assistAsm;
    assistTcs = src->assistTcs;
    if (deferModel == 0)
        model.copyFrom(src->model);
    else
        pendingModel = &src->model;
    sound = src->sound;                     /* CarSound's operator= (0x00179230): the
                                               banks' buffers, not their links */
    /* implicit: CarSound::operator= (the compiler-generated copy assignment) */
    soundLoaded = 1;
}

// FUNC 0x00177660 RaceEntryCar::applyPendingModel
//   Caller: RacePS2Base::virtual_3.  1 when a deferred model was taken over.
int RaceEntryCar::applyPendingModel()
{
    RaceCarModel *pending = pendingModel;
    int applied = 0;
    if (pending != 0) {
        model.copyFrom(*pending);
        pendingModel = 0;
        modelEnabled = 1;
        applied = 1;
    }
    return applied;
}

// FUNC 0x001776A8 RaceEntryCar::loadModel
//   Caller: RaceBase::virtual_8.  kind: car_model_path's (0 day, 1 night,
//   2 menu, 3 eve).
void RaceEntryCar::loadModel(int kind)
{
    char path[0x100];
    car_model_path(info.getModelCode(), path, kind);
    model.load(path);
}

// FUNC 0x001776F8 RaceEntryCar::loadTires
//   Caller: RaceBase::virtual_8.
void RaceEntryCar::loadTires()
{
    RaceEntryInformation *car = &info;
    int parts[2];
    parts[0] = car->getFronttirePart();
    parts[1] = car->getReartirePart();
    int kind = car->F_00236600();           /* before &model: retail keeps `this' live */
    model.loadTire(parts, kind);
}

// FUNC 0x00177758 RaceEntryCar::loadWheel
//   Caller: RaceBase::virtual_8.
void RaceEntryCar::loadWheel()
{
    RaceEntryInformation *car = &info;
    if (car->isDefaultWheel() == 0)
        model.loadWheel(car->getWheelName());
}

// FUNC 0x001777B0 RaceEntryCar::loadSound
//   Caller: RaceBase::virtual_8.  The SPU area every time, the engine sound
//   (its code and exhaust kind) only once.
void RaceEntryCar::loadSound()
{
    CarSound *snd = &sound;
    snd->reserveSpu();
    if (soundLoaded == 0) {
        RaceEntryInformation *car = &info;
        int code = car->getEngineSoundCode();       /* the engine sound code */
        snd->load(code, car->getExhaustSoundKind());
        soundLoaded = 1;
    }
}

// FUNC 0x00177830 RaceEntryCar::setupTires
//   Caller: RaceBase::virtual_8.
void RaceEntryCar::setupTires()
{
    model.setupTires();
}

/* The draw wrappers: RaceCarModel's function while modelEnabled.  The
 * arguments after `this' pass through in their registers. */

// FUNC 0x00177850 RaceEntryCar::setReflectionPlane
void RaceEntryCar::setReflectionPlane(f32 t)
{
    if (modelEnabled != 0)
        model.setReflectionPlane(t);
}

// FUNC 0x00177878 RaceEntryCar::F_00177878
//   Not tested against modelEnabled (RaceCarModel::F_001AA568 is empty).
void RaceEntryCar::F_00177878()
{
    model.F_001AA568();
}

// FUNC 0x00177898 RaceEntryCar::drawShadow
void RaceEntryCar::drawShadow(f32 t)
{
    if (modelEnabled != 0)
        model.drawShadow(t);
}

// FUNC 0x001778C0 RaceEntryCar::F_001778C0
void RaceEntryCar::F_001778C0(f32 t)
{
    if (modelEnabled != 0)
        model.F_001AA680(t);
}

// FUNC 0x001778E8 RaceEntryCar::F_001778E8
void RaceEntryCar::F_001778E8(f32 t)
{
    if (modelEnabled != 0)
        model.F_001AA6D0(t);
}

// FUNC 0x00177910 RaceEntryCar::drawBody
void RaceEntryCar::drawBody(f32 t)
{
    if (modelEnabled != 0)
        model.drawBody(t, 1);
}

// FUNC 0x00177938 RaceEntryCar::drawTires
void RaceEntryCar::drawTires(int blur, f32 scale, f32 t, int lod)
{
    if (modelEnabled != 0)
        model.drawTires(blur, scale, t, lod);
}

// FUNC 0x00177960 RaceEntryCar::drawTireCylinders
void RaceEntryCar::drawTireCylinders(f32 unused, f32 t)
{
    if (modelEnabled != 0)
        model.drawTireCylinders(unused, t);
}

// FUNC 0x00177988 RaceEntryCar::F_00177988
void RaceEntryCar::F_00177988(f32 t)
{
    if (modelEnabled != 0)
        model.F_001AAD70(t);
}

// FUNC 0x001779B0 RaceEntryCar::F_001779B0
//   No callers.
void RaceEntryCar::F_001779B0(f32 t)
{
    if (modelEnabled != 0)
        model.F_001AAD78(t);
}

// FUNC 0x001779D8 RaceEntryCar::emitSparks
void RaceEntryCar::emitSparks(ParticleManager *pm, f32 t)
{
    if (modelEnabled != 0)
        model.emitSparks(pm, t);
}

// FUNC 0x00177A00 RaceEntryCar::addLamps
void RaceEntryCar::addLamps(FlareClass *flare, f32 t)
{
    if (modelEnabled != 0)
        model.addLamps(flare, t);
}

// FUNC 0x00177A28 RaceEntryCar::drawLampReflections
void RaceEntryCar::drawLampReflections(FlareClass *flare, f32 t)
{
    if (modelEnabled != 0)
        model.drawLampReflections(flare, t);
}

/* {0, 0, 0, 1}: the listener's own position, where the on-board voices sound
 * (.data: T121's only variable, before its frame info). */
f32 g_002B2CC0[4] = { 0.0f, 0.0f, 0.0f, 1.0f };    /* 0x002B2CC0 */

// FUNC 0x00177A50 RaceEntryCar::updateSound
//   Caller: RacePS2Base::virtual_29, once per frame and car (noCrashSound: the
//   replay mode).  Builds the car's CarSoundFrame and hands it to
//   CarSound::update: the voices are placed relative to the listener (the
//   current view's matrix: vu0_apply_matrix), get the gains of the listening
//   mode (F_001DE050, LISTEN_*) and a Doppler factor as their pitch.
//   noCrashSound != 0 (or another car's view) mutes the crash sounds, not skids.
//   In a split screen the view that follows the car is made current first.
//   Matching (u042): 81.7%, 738 of 792 instructions; kept honest (no
//   fakematch).  What matches: the copies as a count-down loop on local
//   pointers (copy_pos), the seven wheel voices written out with their own
//   pointers (&frame.voice[base + k]), the DynCarState pointer, one gain
//   variable per section, the volume computed before pitch / param, the
//   volatile sqrt, the first Doppler dot product in its own variable
//   (`listen': doppler $f21, camX / dx..dz $f22, $f25..$f23 as retail) and
//   the seven section pointers set together before the first section (only
//   engine0's copy shares their CSE block, so only its counter is allocated
//   before its pointers, as retail).
//   The limitation (ref/matching/u042/notes.md, follow-ups 1 and 2,
//   2026-10-17): retail tests `split' again (its stack slot) right after
//   each vu0_apply_matrix, the car's and every wheel's, and both arms
//   compute every address used after the test: for the car frame.pos, the
//   seven sections' structs, &voicePitch, &windPitch; for a wheel its
//   seven &volume / &pitch and wheel + 1.  In the not-split arm that code
//   died after GCSE, so retail's listing shows only its traces: the extra
//   branch, the 0x510 frame (ours 0x4D0), the spill slots 0x3B8..0x3F4 and
//   an 8-aligned label after the arm.  No source statement that would
//   naturally leave dead address computations in exactly that place was
//   found: an inline helper, a debug macro (`do { if (dbg) printf(..); }
//   while (0)') or live pointer variables set in both arms all give other
//   frames, slot orders or CSE blocks.  The instruction count also shows
//   what is missing: the spill slots follow GCSE's hash order of the
//   reaching registers, `(14029 + regno + const) mod T' with T = (insns at
//   GCSE / 2) | 1; solving retail's slot order gives T = 345 and the wheel
//   loop's pseudos about 107 above ours, so the original function had
//   more RTL before the wheel loop (about one dead insn per address) than
//   any form found.  An exact form exists (dead pointer sets in an if /
//   else after each transform, the view selection in a do / while (0), a
//   do-while copy loop and 107 unused locals: 792 / 792,
//   build/scratch/u042/fu2/fakematch_T121.cpp) but is not the original and
//   is not kept.  Consequence: the function is 0xD8 bytes shorter than
//   retail's, so its FDE (T121's frame info, 8 bytes) differs from the
//   original's; the full link places T121's .data anyway (its variables are
//   exact).
void RaceEntryCar::updateSound(CameraManager *cm, int noCrashSound)
{
    CarSoundFrame frame;                    /* sp+0x000 */
    f32 pos[4];                             /* sp+0x380 the car, then each wheel, seen from the listener */
    u16 windPitch;                          /* sp+0x390 car_sound_wind's pitch */
    f32 voicePitch;                         /* sp+0x394 the gear whine's / road's pitch */
    f32 crashPitch;                         /* sp+0x398 (not used) */
    int crashKind;                          /* sp+0x39C */
    int crashArg;                           /* sp+0x3A0 (not used) */

    /* split screen: the view that follows this car (else the car's number) */
    int split = cm->viewCount >= 2;
    if (split) {
        int car = number;
        if (car < cm->viewCount) {
            pgl_matrix_mode(PGL_MTX_MODELVIEW);
            vu0_load_identity_regs();
            int view = car;
            if (car != cm->views[car].targetInfo.index[0]) {
                if (car == cm->views[0].targetInfo.index[0])
                    view = 0;
                else if (car == cm->views[1].targetInfo.index[0])
                    view = 1;
            }
            cm->setCurrentView(view);
            cm->applyView();                /* the view's matrix */
        }
    }

    RaceCarState *st = state;
    CarSound *snd = &sound;
    DynCarState *run = &st->st;
    f32 x = car_view_x(st, 0.0f);
    f32 y = car_view_y(st, 0.0f);
    f32 z = car_view_z(st, 0.0f);
    pos[0] = x;
    pos[3] = 1.0f;
    pos[1] = y;
    pos[2] = z;
    vu0_apply_matrix(pos, pos);
    copy_pos(frame.pos, pos);
    frame.onboard = cm->isOwnCar(this);

    /* Doppler: (c + the listener's speed) / (c + the car's speed) along the
     * listener -> car direction, c the speed of sound per frame; the
     * listener's speed from the camera's last two eye positions. */
    f32 doppler = 1.0f;
    if (cm->views[cm->current].camera->doppler != 0) {
        f32 camX = cm->getPositionX();
        f32 dx = x - camX;
        f32 camY = cm->getPositionY();
        f32 dy = y - camY;
        f32 camZ = cm->getPositionZ();
        f32 dz = z - camZ;
        f32 dist = boot_sqrtf_v(dx * dx + dy * dy + dz * dz);
        if (dist == 0.0f)
            dist = 1.0f;
        dx = dx / dist;
        dy = dy / dist;
        dz = dz / dist;
        VariableCameraBase *cam = cm->views[cm->current].camera;
        f32 listen = dx * (camX - cam->position[1].x)       /* the listener's speed */
                   + dy * (camY - cam->position[1].y)
                   + dz * (camZ - cam->position[1].z);
        f32 vx = x - car_view_x_now(st);
        f32 vy = y - car_view_y_now(st);
        f32 vz = z - car_view_z_now(st);
        doppler = (listen + SOUND_SPEED_PER_FRAME)
                / (dx * vx + dy * vy + dz * vz + SOUND_SPEED_PER_FRAME);
    } else {
        snd->stop();                        /* no Doppler on this camera: the voices restarted */
        snd->start();
    }

    int mode = LISTEN_OUTSIDE;
    frame.scale = 1.0f;
    switch (cm->getCameraKind()) {             /* the camera kind */
    case 0:
        mode = LISTEN_INSIDE_OTHER;
        if (frame.onboard != 0) {
            mode = LISTEN_INSIDE_OWN;
            copy_pos(pos, g_002B2CC0);      /* in the car: the voices sound at the listener */
        }
        break;
    case 1:
        mode = (frame.onboard == 0) ? LISTEN_MOUNT_OTHER : LISTEN_MOUNT_OWN;
        break;
    }
    voicePitch = 0.0f;
    windPitch = 2500;
    CarSoundVoiceEx *engine0 = &frame.engine[SND_ENGINE_0];
    CarSoundVoiceEx *engine1 = &frame.engine[SND_ENGINE_1];
    CarSoundVoiceEx *wind = &frame.engine[SND_ENGINE_WIND];
    CarSoundVoice *whine = &frame.voice[SND_VOICE_GEAR_WHINE];
    CarSoundVoice *turbo = &frame.voice[SND_VOICE_TURBO];
    CarSoundVoice *road = &frame.voice[SND_VOICE_ROAD];
    CarSoundVoice *silent = &frame.voice[SND_VOICE_SILENT];

    {
        f32 gain = F_001DE050(mode, SND_GAIN_ENGINE0);
        copy_pos(engine0->pos, pos);
        engine0->volume = gain * (f32)run->engineSnd[0] * 0.015625f;    /* / 64 */
        engine0->pitch = doppler;
        engine0->param = (f32)run->rpm;
    }
    {
        f32 gain = F_001DE050(mode, SND_GAIN_ENGINE1);
        copy_pos(engine1->pos, pos);
        engine1->volume = gain * (f32)run->engineSnd[1] * 0.015625f;
        engine1->pitch = doppler;
        engine1->param = (f32)run->rpm;
    }
    {
        f32 gain = F_001DE050(mode, SND_GAIN_GEAR_WHINE);
        copy_pos(whine->pos, pos);
        whine->volume = gain * car_sound_gear_whine(st, &voicePitch);
        whine->pitch = doppler * voicePitch;
    }
    {
        f32 gain = F_001DE050(mode, SND_GAIN_TURBO);
        copy_pos(turbo->pos, pos);
        turbo->volume = gain * (f32)run->engineSnd[2] * 0.0078125f;     /* / 128 */
        turbo->pitch = (f32)run->turboSpoolMax / 6144.0f;
    }
    {
        f32 gain = F_001DE050(mode, SND_GAIN_ROAD);
        copy_pos(road->pos, pos);
        road->volume = gain * car_sound_road(st, &voicePitch);
        road->pitch = doppler * voicePitch;
    }
    {
        f32 gain = F_001DE050(mode, SND_GAIN_WIND);
        copy_pos(wind->pos, pos);
        wind->volume = gain * car_sound_wind(st, &windPitch);
        wind->pitch = doppler;
        wind->param = (f32)windPitch;       /* lhu */
    }
    {
        copy_pos(silent->pos, pos);
        silent->volume = 0.0f;              /* sw $zero */
        silent->pitch = doppler;
    }

    frame.oneShot[SND_ONESHOT_BLOWOFF] = F_001DE050(mode, SND_GAIN_BLOWOFF) * run->blowOffLevel;

    /* the loudest crash of the frame, as the one-shot of its kind; only
     * from the car's own view and outside replays */
    {
        f32 gain = F_001DE050(mode, SND_GAIN_CRASH);
        frame.oneShot[SND_ONESHOT_CRASH + 0] = 0.0f;
        frame.oneShot[SND_ONESHOT_CRASH + 1] = 0.0f;
        frame.oneShot[SND_ONESHOT_CRASH + 2] = 0.0f;
        if (frame.onboard != 0 && noCrashSound == 0) {
            gain = gain * car_crash_sound_get(st, &crashPitch, &crashKind, &crashArg);
            switch (crashKind) {
            case 0: frame.oneShot[SND_ONESHOT_CRASH + 0] = gain; break;
            case 1: frame.oneShot[SND_ONESHOT_CRASH + 1] = gain; break;
            case 2: frame.oneShot[SND_ONESHOT_CRASH + 2] = gain; break;
            }
        }
    }

    /* the wheels: seven voices each, at the wheel's position */
    f32 gainWheel0 = F_001DE050(mode, SND_GAIN_WHEEL0);
    f32 gainWheel1 = F_001DE050(mode, SND_GAIN_WHEEL1);
    f32 gainWheel2 = F_001DE050(mode, SND_GAIN_WHEEL2);
    f32 gainWheel3 = F_001DE050(mode, SND_GAIN_WHEEL3);
    f32 gainWheel4 = F_001DE050(mode, SND_GAIN_WHEEL4);
    f32 gainWheel5 = F_001DE050(mode, SND_GAIN_WHEEL5);
    f32 gainWheel6 = F_001DE050(mode, SND_GAIN_WHEEL6);
    for (int wheel = 0; wheel < 4; wheel++) {
        int base = SND_VOICE_WHEEL + SND_WHEEL_VOICES * wheel;
        CarSoundVoice *v0 = &frame.voice[base + 0];
        CarSoundVoice *v1 = &frame.voice[base + 1];
        CarSoundVoice *v2 = &frame.voice[base + 2];
        CarSoundVoice *v3 = &frame.voice[base + 3];
        CarSoundVoice *v4 = &frame.voice[base + 4];
        CarSoundVoice *v5 = &frame.voice[base + 5];
        CarSoundVoice *v6 = &frame.voice[base + 6];
        car_wheel_pos(st, pos, wheel, 0.0f);
        pos[3] = 1.0f;
        vu0_apply_matrix(pos, pos);
        copy_pos(v0->pos, pos);
        v0->volume = 0.0f;
        v0->pitch = doppler;
        copy_pos(v1->pos, pos);
        v1->volume = 0.0f;
        v1->pitch = doppler;
        copy_pos(v2->pos, pos);
        v2->volume = 0.0f;
        v2->pitch = doppler;
        copy_pos(v3->pos, pos);
        v3->volume = 0.0f;
        v3->pitch = doppler;
        copy_pos(v4->pos, pos);
        v4->volume = 0.0f;
        v4->pitch = doppler;
        copy_pos(v5->pos, pos);
        v5->volume = 0.0f;
        v5->pitch = doppler;
        copy_pos(v6->pos, pos);
        v6->volume = 0.0f;
        v6->pitch = doppler;
        /* each sets its voice when the wheel's surface is in its class */
        tyre_sound_squeal_0789(st, wheel, gainWheel1, doppler, &v1->volume, &v1->pitch);
        tyre_sound_roll_0789(st, wheel, gainWheel5, doppler, &v5->volume, &v5->pitch);
        tyre_sound_squeal_6_12_14(st, wheel, gainWheel6, doppler, &v6->volume, &v6->pitch);
        tyre_sound_squeal_5(st, wheel, gainWheel2, doppler, &v2->volume, &v2->pitch);
        tyre_sound_none(st, wheel, gainWheel0, doppler, &v0->volume, &v0->pitch);
        tyre_sound_squeal_1_13(st, wheel, gainWheel3, doppler, &v3->volume, &v3->pitch);
        tyre_sound_squeal_2_4(st, wheel, gainWheel4, doppler, &v4->volume, &v4->pitch);
    }
    {
        /* The enhanced engine (Lupo 2 L2 0x001769A4..0x001769C8,
         * cameras_sound.md S1): the Pod's voice - volume 1.0, the Doppler
         * factor, the emotion the Pod entered this frame (L2 0x001E19D0: 0
         * for any other car) */
        CarSoundPodVoice pod;

        pod.volume = 1.0f;
        pod.pitch = doppler;
        pod.emotion = (st->params.specialKind == CAR_KIND_POD) ? st->podSound : 0;
        snd->update(&frame, &pod);
    }
}

// FUNC 0x001786B0 RaceEntryCar::recordSectionTime
//   Caller: RaceOrganization::recordSectionTime.  Returns the difference to
//   the session best section (LapRecord::sectionDiff).
int RaceEntryCar::recordSectionTime(int section, u32 time)
{
    LapRecord *lap = &lapRecord;
    lap->section[section] = time;
    return lap->sectionDiff(sessionBest, section);
}

// FUNC 0x001786E8 RaceEntryCar::recordLapTime
//   Caller: RaceOrganization::recordLapTime.  Returns the difference to the
//   session best lap (LapRecord::totalDiff); update != 0 also takes the lap
//   as the best when it is better (LapRecord::update).
int RaceEntryCar::recordLapTime(u32 total, int update)
{
    LapRecord *lap = &lapRecord;
    LapRecord *best = &sessionBest;
    lap->total = total;
    int diff = lap->totalDiff(*best);
    if (update != 0)
        best->update(*lap);
    return diff;
}

// FUNC 0x00178750 RaceEntryCar::resetLapTime
//   No callers.  The lap times recorded anew (RaceLapTime::reset(0)) with
//   `time' as the first lap (RaceLapTime::add).
void RaceEntryCar::resetLapTime(u32 time)
{
    RaceLapTime *laps = &lapTime;
    laps->reset(0);
    laps->add(time);
}

// FUNC 0x00178798 RaceEntryCar::setDriverName
void RaceEntryCar::setDriverName(const char *name)
{
    strcpy(driverName, name);
}

// FUNC 0x001787B8 RaceEntryCar::setDate
//   date == NULL: now (clock_get_local, BCD, converted; years after 2000).
void RaceEntryCar::setDate(const RaceDate *date)
{
    CdClock clock;                          /* sp+0x00 */
    RaceDate now;                           /* sp+0x10 */
    const RaceDate *d = date;
    if (d == 0) {
        clock_get_local(&clock);
        now.zero   = 0;
        now.second = bcd_to_int(clock.second);
        now.minute = bcd_to_int(clock.minute);
        now.hour   = bcd_to_int(clock.hour);
        now.day    = bcd_to_int(clock.day);
        now.month  = bcd_to_int(clock.month);
        now.year   = bcd_to_int(clock.year) + 2000;
        /* &now taken after the fields: egcs builds the word in a register
         * (ld, and / or per field, one sd), as retail; taken first, it
         * stores each field with sb / sh */
        d = &now;
    }
    this->date = *d;
}

/* ======================================================================== */
/* RaceEntryBase                                                            */
/* ======================================================================== */

// FUNC 0x00178938 RaceEntryBase::operator new
//   No direct callers (RaceEntryPS2 has its own).
void *RaceEntryBase::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00178960 RaceEntryBase::operator delete
void RaceEntryBase::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00178988 RaceEntryBase::RaceEntryBase
//   Caller: RaceEntryPS2's constructor.  The rank list (+0x14 / +0x18) is
//   not initialised.
RaceEntryBase::RaceEntryBase()
{
    count = 0;
    capacity = 0;
    cars = 0;
    sortTop = 0;
    sortEnd = 0;
}

// FUNC 0x001789B0 RaceEntryBase::~RaceEntryBase
//   (slot 0.)  With flags bit 0: operator delete(this, 0x20).
RaceEntryBase::~RaceEntryBase()
{
    releaseEntries();
}

// FUNC 0x00178A00 RaceEntryBase::releaseEntries
//   Deletes every entry (virtual destructor, flags 3); the array stays.
void RaceEntryBase::releaseEntries()
{
    if (cars != 0) {
        for (u32 i = 0; i < (u32)count; i++) {  /* compared unsigned (sltu) */
            RaceEntryCar *car = cars[i];
            if (car != 0)
                delete car;
        }
    }
    count = 0;
}

// FUNC 0x00178A90 RaceEntryBase::addEntry
//   (slot 3.)  Appends the car and gives it its number; -1 when
//   canAddEntry(car) refuses (no bound check here).
int RaceEntryBase::addEntry(RaceEntryCar *car)
{
    if (canAddEntry(car) == 0)
        return -1;
    cars[count] = car;
    car->number = count;                    /* (an int store: count is read again) */
    return count++;
}

// FUNC 0x00178B08 RaceEntryBase::canAddEntry
//   (slot 2.)  The car is not looked at here.
int RaceEntryBase::canAddEntry(RaceEntryCar *car)
{
    return 1;
}

// FUNC 0x00178B10 RaceEntryBase::createEntry
//   (slot 1.)  RaceEntryCar::operator new(0x2148), the constructor (a throw
//   gives the memory back: RaceEntryCar::operator delete).
RaceEntryCar *RaceEntryBase::createEntry()
{
    return new RaceEntryCar; /* implicit: RaceEntryCar::operator delete (if the constructor throws) */
}

// FUNC 0x00178B68 RaceEntryBase::virtual_4
//   Slot 4, empty here (RaceSolitaireEntry: the one-car view on / off).
//   index.txt calls it RaceResultBase::cleanup(void) (a code match, lupo2m).
void RaceEntryBase::virtual_4(int on)
{
}

// FUNC 0x00178B70 RaceEntryBase::sortByDistance
//   Callers: RacePS2Base::drawCars / drawCarReflections (the draw order).  Stores each
//   car's distance to (x, y, z) in car->distance and links the cars nearest
//   first (sortTop .. sortEnd through sortPrev / sortNext; equal ones keep
//   their order).
void RaceEntryBase::sortByDistance(f32 x, f32 y, f32 z)
{
    sortTop = 0;
    sortEnd = 0;
    for (u32 i = 0; i < (u32)count; i++) {  /* compared unsigned (sltu) */
        RaceEntryCar *car = cars[i];
        RaceCarState *st = car->state;
        f32 dx = car_view_x(st, 0.0f) - x;
        f32 dy = car_view_y(st, 0.0f) - y;
        f32 dz = car_view_z(st, 0.0f) - z;
        /* the volatile sqrt (a scheduling barrier, as retail: sortTop is
         * read after it) */
        f32 dist = boot_sqrtf_v(dx * dx + dy * dy + dz * dz);
        car->distance = dist;
        RaceEntryCar *prev = 0;
        RaceEntryCar *next = sortTop;
        while (next != 0 && !(dist < next->distance)) {
            prev = next;
            next = next->sortNext;
        }
        if (prev != 0)
            prev->sortNext = car;
        else
            sortTop = car;
        if (next != 0)
            next->sortPrev = car;
        else
            sortEnd = car;
        car->sortPrev = prev;
        car->sortNext = next;
    }
}

// FUNC 0x00178CE8 RaceEntryBase::sortByRank
//   11 callers.  Links the cars by their race position (DynCarState::rank,
//   a signed byte, 1 the leader), smallest first (rankTop .. rankEnd through
//   rankPrev / rankNext; equal ones keep their order).
void RaceEntryBase::sortByRank()
{
    u32 n = count;                          /* compared unsigned (sltu) */
    rankTop = 0;
    rankEnd = 0;
    for (u32 i = 0; i < n; i++) {
        RaceEntryCar *car = cars[i];
        RaceEntryCar *prev = 0;
        RaceEntryCar *next = rankTop;
        s8 rank = car->state->st.rank;
        while (next != 0 && !(rank < next->state->st.rank)) {
            prev = next;
            next = next->rankNext;
        }
        if (prev != 0)
            prev->rankNext = car;
        else
            rankTop = car;
        if (next != 0)
            next->rankPrev = car;
        else
            rankEnd = car;
        car->rankPrev = prev;
        car->rankNext = next;
    }
}

// FUNC 0x00178D98 RaceEntryBase::getEntry
RaceEntryCar *RaceEntryBase::getEntry(int i)
{
    return cars[i];
}

// FUNC 0x00178DB0 RaceEntryBase::getRankEnd
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178DB8 RaceEntryBase::getRankTop
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178DC0 RaceEntryBase::getSortEnd
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178DC8 RaceEntryBase::getSortTop
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178DD0 RaceEntryBase::getCount
//   index.txt calls it RaceResultBase::isStartResultSequence(void) (lupo2m,
//   matched by its code); it sits among RaceEntryBase's accessors.
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x00178DD8 __tf13RaceEntryBase
//   Builds the __user_type_info __ti13RaceEntryBase on the first call.
extern "C" const type_info &__tf13RaceEntryBase(void)
{
    if (__ti13RaceEntryBase[0] == 0)
        __rtti_user(__ti13RaceEntryBase, "13RaceEntryBase");
    return *(const type_info *)__ti13RaceEntryBase;
}
#endif

/* ======================================================================== */
/* RaceEntryCar accessors                                                   */
/* ======================================================================== */

// FUNC 0x00178E18 RaceEntryCar::setCondition
//   No callers; the same code as setCondition2.
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178E60 RaceEntryCar::setTopSpeed
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178E68 RaceEntryCar::getTopSpeed
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178E70 RaceEntryCar::setCondition2
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178EB8 RaceEntryCar::getCondition
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178EC0 RaceEntryCar::getDate
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178EC8 RaceEntryCar::getDriverName
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178ED0 RaceEntryCar::updateBodyState
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */
/* implicit: updateBodyState (RaceCarModel's) (in the header's inline body) */

// FUNC 0x00178EF0 RaceEntryCar::setModelLod
//   A fixed LOD for the body (-1: by distance), through the out-of-line
//   conversion F_00179460.
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */
/* implicit: getCarModel (RaceCarModel's), F_00179460 (in the header's inline body) */

// FUNC 0x00178F28 RaceEntryCar::getInput
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178F30 RaceEntryCar::setInput
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178F38 RaceEntryCar::stopSound
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */
/* implicit: stop (CarSound's) (in the header's inline body) */

// FUNC 0x00178F58 RaceEntryCar::startSound
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */
/* implicit: start (CarSound's) (in the header's inline body) */

// FUNC 0x00178F78 RaceEntryCar::getSound
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178F80 RaceEntryCar::setModelEnabled
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178F88 RaceEntryCar::getModelEnabled
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178F90 RaceEntryCar::getCarModel
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178F98 RaceEntryCar::getModelInfo
//   No callers.  The car file's CarModelInfo (RaceCarModel::getInfo's code;
//   no NULL check).
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FA8 RaceEntryCar::getModel
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FB0 RaceEntryCar::setGhostLapTime
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FB8 RaceEntryCar::getGhostLapTime
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FC0 RaceEntryCar::getSessionBestLapRecord
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FC8 RaceEntryCar::getLapRecord
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FD0 RaceEntryCar::getLastLapTime
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FD8 RaceEntryCar::getLastSectionTime
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FE8 RaceEntryCar::getSessionBestLapTime
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00178FF0 RaceEntryCar::getSessionBestSectionTime
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179000 RaceEntryCar::getTimeRecord
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179008 RaceEntryCar::getLapTime
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179010 RaceEntryCar::getCarName2
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */
/* implicit: getName2 (CarEquipments') (in the header's inline body) */

// FUNC 0x00179030 RaceEntryCar::getCarName
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */
/* implicit: getName1 (CarEquipments') (in the header's inline body) */

// FUNC 0x00179050 RaceEntryCar::getCarCode
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */
/* implicit: getModelCode (CarEquipments') (in the header's inline body) */

// FUNC 0x00179070 RaceEntryCar::getEquipments
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179078 RaceEntryCar::loadCar
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */
/* implicit: setCar (CarEquipments') (in the header's inline body) */

// FUNC 0x00179098 RaceEntryCar::IsValidAutomobile
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790A8 RaceEntryCar::getState
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790B0 RaceEntryCar::setState
//   The model keeps its own copy of the state pointer.
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790C0 RaceEntryCar::getPoints
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790C8 RaceEntryCar::setPoints
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790D0 RaceEntryCar::getVibration
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790D8 RaceEntryCar::setVibration
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790E0 RaceEntryCar::getAssistTcs
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790E8 RaceEntryCar::setAssistTcs
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001790F8 RaceEntryCar::getAssistAsm
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179100 RaceEntryCar::setAssistAsm
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179108 RaceEntryCar::getSteeringAssist
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179110 RaceEntryCar::setSteeringAssist
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179118 RaceEntryCar::getTyre
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179120 RaceEntryCar::setTyre
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179128 RaceEntryCar::getTransmission
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179130 RaceEntryCar::setTransmission
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179138 RaceEntryCar::getColorIndex
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179140 RaceEntryCar::setColorIndex
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179148 RaceEntryCar::getOrder
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179150 RaceEntryCar::setOrder
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179158 RaceEntryCar::getNumber
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179160 RaceEntryCar::setNumber
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179168 RaceEntryCar::getRankNext
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179170 RaceEntryCar::getRankPrev
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179178 RaceEntryCar::getSortNext
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x00179180 RaceEntryCar::getSortPrev
//   index.txt calls it RaceResultBase::getCarNumber(void) (lupo2m, matched
//   by its code); it is the RaceEntryCar accessor of +0x4.
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x00179188 __tf12RaceEntryCar
//   Builds the __user_type_info __ti12RaceEntryCar on the first call.
extern "C" const type_info &__tf12RaceEntryCar(void)
{
    if (__ti12RaceEntryCar[0] == 0)
        __rtti_user(__ti12RaceEntryCar, "12RaceEntryCar");
    return *(const type_info *)__ti12RaceEntryCar;
}
#endif

// FUNC 0x001791C8 RaceEntryInformation::getInputID
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

// FUNC 0x001791D0 RaceEntryInformation::setInputID
/* inline in game/race/core/T121_race_entry.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x001791D8 __tf20RaceEntryInformation
//   Builds the __class_type_info __ti20RaceEntryInformation (one base,
//   CarEquipments: __tf13CarEquipments first; base list g_00351DD8) on the
//   first call.
extern "C" const type_info &__tf20RaceEntryInformation(void)
{
    if (__ti20RaceEntryInformation[0] == 0) {
        __tf13CarEquipments();
        __rtti_class(__ti20RaceEntryInformation, "20RaceEntryInformation", g_00351DD8, 1);
    }
    return *(const type_info *)__ti20RaceEntryInformation;
}
#endif

// FUNC 0x00179230 CarSound::operator= (__as__8CarSoundRC8CarSound)
//   The compiler-generated copy assignment, emitted here for copyFrom's
//   `sound = src->sound', memberwise: pan; each of the 3 engines: its pan,
//   its voice (GtSoundVoice::operator=, 0x0022C280) and its bank
//   (EngineSound::operator=, 0x001A6570: only the 0x100-byte header buffer);
//   each of the 32 slots: its pan and its handle (GtSoundHandle::operator=,
//   0x0022BEF8); field_7CC, spuBase, path0, path1, playing; returns *this.
//   egcs assigns an array of class type with its vec-init loop, cleanup
//   included: when an engine's assignment throws, the engines already done
//   are destroyed (~EngineSound, flags 2).  Retail's shape (that cleanup
//   but no member cleanup across the slots, which a constructor has; the
//   path pointers set up before the slots' loop; `this' returned; callees
//   that stop the target's old voice first) is egcs's synthesized
//   operator=: exact (u042; the first pass took it for the copy
//   constructor, 40.5%).  Both compilers generate it from the members'
//   operator= (T176, T468), so GCC 14 copies only the banks' buffers too.
/* implicit: GtSoundVoice::operator=, EngineSound::operator=,
 * GtSoundHandle::operator= (the members' copy assignments), ~EngineSound
 * (the engines' cleanup on a throw) */

// FUNC 0x00179460 F_00179460
//   Returns its argument (an out-of-line conversion; 6 callers).  Inline in
//   the original: T121's copy comes after CarSound's operator=, the
//   last of the deferred functions, and T129 / T131 / T180 / T184 have weak
//   copies of their own (no callers: the link took T121's), so their
//   callers did not expand it either: inline functions compiled before its
//   body (as setModelLod here, ref/data/u026/notes.md).  Defined inline here
//   under egcs, after the classes (the other units declare it plain and call
//   T121's copy); a plain function for GCC 14.
#ifdef BOOT_OLD_GCC
inline
#endif
CarModel *F_00179460(CarModel *m)
{
    return m;
}
