/* T482_car_equipments.cpp - CarEquipments (translation unit T482,
 * 0x00231AB0..0x00237860; one object in retail, the work units u106
 * 0x00231AB0.., u107 0x00234020.. and u108 0x00236600.. hold its addresses):
 * loading a car, the default settings of the parts and the car parameter
 * block (CarSpec) built from the records of the fitted parts (u106); the
 * records of the fitted parts, part ids and names, the spec sheet values,
 * fitting a part, colours (u107); the tyre helpers, the save block copies,
 * the class-inline accessors (setting values, ranges and levels, the part
 * setters; their out-of-line copies, emitted with the key function
 * ~CarEquipments) and the typeinfo functions (u108).
 *
 * Names in u108's range: Lupo 2's where index.txt has them.  The min / max
 * and level getters without a symbol (getSettingRearLSDparam3Max ..
 * getMaxLevelParam4WD) are named by the pattern: they run in the reverse
 * order of the setting getters (getSettingASCC .. getSettingFinalGearRatio,
 * Lupo 2), each reads the record field of its setting (min before max, front
 * before rear), and the named ones among them (getSettingRearLSDparamMax,
 * getSettingFrontLSDparamMin, getSettingRearSpringrateMax, ...) fix the
 * spelling. */
#include "game/data/car_equipments_internal.h"
#include "game/data/T482_car_equipments.h"

/* The CarDataBase base is constructed first (its clear() zeroes it), then
 * the vptr, reset(), and a copy of the global database's file pointers.
 * (An exception destroys the CarDataBase base again, ~CarDataBase: the EH
 * landing pad.) */
// FUNC 0x00231AB0 CarEquipments::CarEquipments
CarEquipments::CarEquipments()
    : CarDataBase()
{
    reset();
    copy(g_002EBD88);
}

// FUNC 0x00231B38 CarEquipments::~CarEquipments
CarEquipments::~CarEquipments()
{
    /* implicit: ~CarDataBase, operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr, then ~CarDataBase() of the base; the deleting form (flags &
     * 1) calls operator delete(this) */
}

/* The whole object, vptr included.  A real memcpy call in retail: the size
 * in a variable (a constant size is expanded inline, u023). */
// FUNC 0x00231B90 CarEquipments::operator=
CarEquipments &CarEquipments::operator=(const CarEquipments &src)
{
    u32 size = sizeof(CarEquipments);

    memcpy((void *)this, &src, size);
    return *this;
}

/* Note: the steer part (+0x68) is not cleared, nor the settings.
 * presetCar is cleared last (retail's store order: the scheduler moves the
 * last store of the block up, here to third). */
// FUNC 0x00231BC0 CarEquipments::reset
void CarEquipments::reset()
{
    this->colorIndex = -1;
    this->colorCode = (u32)-1;          /* u32 (u041): lui / ori, as retail */
    this->carCode = 0;
    this->carType = 0;
    this->brake = 0;
    this->brakeController = 0;
    this->chassis = 0;
    this->engine = 0;
    this->drivetrain = 0;
    this->gear = 0;
    this->suspension = 0;
    this->lsd = 0;
    this->frontTire = 0;
    this->rearTire = 0;
    this->lightweight = 0;
    this->racingModify = 0;
    this->portPolish = 0;
    this->engineBalance = 0;
    this->displacement = 0;
    this->computer = 0;
    this->naTune = 0;
    this->turbine = 0;
    this->flywheel = 0;
    this->clutch = 0;
    this->propellerShaft = 0;
    this->muffler = 0;
    this->intercooler = 0;
    this->ascc = 0;
    this->tcsc = 0;
    this->wheel = 0;
    this->unk_1B8 = 0;
    this->presetCar = 0;
}

/* A preset car (table 0x23 record): its parts, their default settings, its
 * car.  The colour, carType and presetCar are left as they are. */
// FUNC 0x00231C50 CarEquipments::setPreset
int CarEquipments::setPreset(const PresetCarRecord *rec)
{
    this->brake = rec->parts.brake;
    this->brakeController = rec->parts.brakeController;
    this->chassis = rec->parts.chassis;
    this->engine = rec->parts.engine;
    this->drivetrain = rec->parts.drivetrain;
    this->gear = rec->parts.gear;
    this->suspension = rec->parts.suspension;
    this->lsd = rec->parts.lsd;
    this->frontTire = rec->parts.frontTire;
    this->rearTire = rec->parts.rearTire;
    this->steer = rec->parts.steer;
    this->lightweight = rec->parts.lightweight;
    this->racingModify = rec->parts.racingModify;
    this->portPolish = rec->parts.portPolish;
    this->engineBalance = rec->parts.engineBalance;
    this->displacement = rec->parts.displacement;
    this->computer = rec->parts.computer;
    this->naTune = rec->parts.naTune;
    this->turbine = rec->parts.turbine;
    this->flywheel = rec->parts.flywheel;
    this->clutch = rec->parts.clutch;
    this->propellerShaft = rec->parts.propellerShaft;
    this->muffler = rec->parts.muffler;
    this->intercooler = rec->parts.intercooler;
    this->ascc = rec->parts.ascc;
    this->tcsc = rec->parts.tcsc;
    this->wheel = rec->parts.wheel;
    resetGear();
    resetDrivetrain();
    resetBrakeController();
    resetRacingModify();
    resetTurbineKit();
    resetSuspension();
    resetLsd();
    resetAscc();
    resetTcsc();
    F_002369A8();                       /* the gear ratios of the auto level */
    this->carCode = rec->car;
    return 1;
}

/* A preset car of that code (table 0x23), else the car of table 0x1E with
 * its stock parts and colour index 0 (0 if the model has no colour 0). */
// FUNC 0x00231DB0 CarEquipments::setCar
int CarEquipments::setCar(u64 code)
{
    const CarRecord *car;

    if (loadPreset(code) != 0)
        return 1;
    car = (const CarRecord *)getRecord(CARDB_CAR, code);
    if (car == NULL)
        return 0;
    this->brake = car->parts.brake;
    this->brakeController = car->parts.brakeController;
    this->chassis = car->parts.chassis;
    this->engine = car->parts.engine;
    this->drivetrain = car->parts.drivetrain;
    this->gear = car->parts.gear;
    this->suspension = car->parts.suspension;
    this->lsd = car->parts.lsd;
    this->frontTire = car->parts.frontTire;
    this->rearTire = car->parts.rearTire;
    this->steer = car->parts.steer;
    this->lightweight = car->parts.lightweight;
    this->racingModify = car->parts.racingModify;
    this->portPolish = car->parts.portPolish;
    this->engineBalance = car->parts.engineBalance;
    this->displacement = car->parts.displacement;
    this->computer = car->parts.computer;
    this->naTune = car->parts.naTune;
    this->turbine = car->parts.turbine;
    this->flywheel = car->parts.flywheel;
    this->clutch = car->parts.clutch;
    this->propellerShaft = car->parts.propellerShaft;
    this->muffler = car->parts.muffler;
    this->intercooler = car->parts.intercooler;
    this->ascc = car->parts.ascc;
    this->tcsc = car->parts.tcsc;
    this->wheel = car->parts.wheel;
    if (setColorIndex(0) == 0)
        return 0;
    resetGear();
    resetDrivetrain();
    resetBrakeController();
    resetRacingModify();
    resetTurbineKit();
    resetSuspension();
    resetLsd();
    resetAscc();
    resetTcsc();
    F_002369A8();                       /* the gear ratios of the auto level */
    this->carCode = code;
    this->carType = car->unk_0F1;
    return 1;
}

/* Table 0x23 record `code': its parts (setPreset), then its own settings
 * over the parts' defaults (loadPresetSettings); 0 if there is none. */
// FUNC 0x00231F60 CarEquipments::loadPreset
int CarEquipments::loadPreset(u64 code)
{
    const PresetCarRecord *preset = (const PresetCarRecord *)getRecord(CARDB_PRESET_CAR, code);

    if (preset == NULL || setPreset(preset) == 0)
        return 0;
    loadPresetSettings(preset);
    return 1;
}

/* ---- the default settings of the adjustable parts (the record's default,
 * clamped into its range; a level setting lies in 1..count) -------------- */

// FUNC 0x00231FC8 CarEquipments::resetGear
void CarEquipments::resetGear()
{
    const TransmissionRecord *rec = (const TransmissionRecord *)getRecord(CARDB_GEAR, this->gear);

    this->gearRatio[0] = rec->gearRatio[0];
    this->gearRatio[1] = rec->gearRatio[1];
    this->gearRatio[2] = rec->gearRatio[2];
    this->gearRatio[3] = rec->gearRatio[3];
    this->gearRatio[4] = rec->gearRatio[4];
    this->gearRatio[5] = rec->gearRatio[5];
    this->gearRatio[6] = rec->gearRatio[6];
    this->gearRatio[7] = rec->gearRatio[7];
    this->finalGearRatio = clamp(rec->finalGearRatioMin, rec->finalGearRatio, rec->finalGearRatioMax);
    this->maxSpeed = clamp(rec->maxSpeedMin, rec->maxSpeed, rec->maxSpeedMax);
    this->lastFinalGearRatio = this->finalGearRatio;
}

// FUNC 0x00232080 CarEquipments::resetDrivetrain
void CarEquipments::resetDrivetrain()
{
    const DrivetrainRecord *rec = (const DrivetrainRecord *)getRecord(CARDB_DRIVETRAIN, this->drivetrain);

    this->param4WD = clamp(rec->vcdMin, rec->vcd, rec->vcdMax);
}

/* Both brake levels start at the record's one default. */
// FUNC 0x002320C8 CarEquipments::resetBrakeController
void CarEquipments::resetBrakeController()
{
    const BrakeControllerRecord *rec =
        (const BrakeControllerRecord *)getRecord(CARDB_BRAKE_CONTROLLER, this->brakeController);

    this->frontABS = clamp(1, rec->level, rec->countF);
    this->rearABS = clamp(1, rec->level, rec->countR);
}

// FUNC 0x00232138 CarEquipments::resetRacingModify
void CarEquipments::resetRacingModify()
{
    const RacingModifyRecord *rec =
        (const RacingModifyRecord *)getRecord(CARDB_RACING_MODIFY, this->racingModify);

    this->clF = clamp(rec->aeroFMin, rec->aeroF, rec->aeroFMax);
    this->clR = clamp(rec->aeroRMin, rec->aeroR, rec->aeroRMax);
}

// FUNC 0x002321A8 CarEquipments::resetTurbineKit
void CarEquipments::resetTurbineKit()
{
    const TurbineKitRecord *rec = (const TurbineKitRecord *)getRecord(CARDB_TURBINE, this->turbine);

    this->boost1 = rec->setting[0];
    this->peekRPM1 = rec->setting[1];
    this->response1 = rec->setting[2];
    this->boost2 = rec->setting[3];
    this->peekRPM2 = rec->setting[4];
    this->response2 = rec->setting[5];
}

/* The toe settings take the record's defaults unclamped (toeFMin..toeRMax
 * are not used here). */
// FUNC 0x00232208 CarEquipments::resetSuspension
void CarEquipments::resetSuspension()
{
    const SuspensionRecord *rec = (const SuspensionRecord *)getRecord(CARDB_SUSPENSION, this->suspension);
    int count;                          /* each damper count, kept for its two stages (as specSuspension) */

    this->frontToe = rec->toeF;
    this->rearToe = rec->toeR;
    this->frontCamber = clamp(rec->camberFMin, rec->camberF, rec->camberFMax);
    this->rearCamber = clamp(rec->camberRMin, rec->camberR, rec->camberRMax);
    this->frontRideheight = clamp(rec->heightFMin, rec->heightF, rec->heightFMax);
    this->rearRideheight = clamp(rec->heightRMin, rec->heightR, rec->heightRMax);
    this->frontSpringrate = clamp(rec->springRateFMin, rec->springRateF, rec->springRateFMax);
    this->rearSpringrate = clamp(rec->springRateRMin, rec->springRateR, rec->springRateRMax);
    this->unk_164 = rec->unk_27;
    this->unk_168 = rec->unk_28;
    count = rec->damperFCount;
    this->frontDamperF1B = clamp(1, rec->damperF, count);
    this->frontDamperF2B = clamp(1, rec->damperF2, count);
    count = rec->reboundFCount;
    this->frontDamperF1R = clamp(1, rec->reboundF, count);
    this->frontDamperF2R = clamp(1, rec->reboundF2, count);
    count = rec->damperRCount;
    this->rearDamperF1B = clamp(1, rec->damperR, count);
    this->rearDamperF2B = clamp(1, rec->damperR2, count);
    count = rec->reboundRCount;
    this->rearDamperF1R = clamp(1, rec->reboundR, count);
    this->rearDamperF2R = clamp(1, rec->reboundR2, count);
    this->frontStabilizer = clamp(1, rec->stabilizerF, rec->stabilizerFCount);
    this->rearStabilizer = clamp(1, rec->stabilizerR, rec->stabilizerRCount);
}

/* LSDparam / param2 / param3 = the initial torque, the acceleration and the
 * deceleration sensitivity. */
// FUNC 0x00232400 CarEquipments::resetLsd
void CarEquipments::resetLsd()
{
    const LsdRecord *rec = (const LsdRecord *)getRecord(CARDB_LSD, this->lsd);

    this->frontLSDparam = clamp(rec->initialFMin, rec->initialF, rec->initialFMax);
    this->rearLSDparam = clamp(rec->initialRMin, rec->initialR, rec->initialRMax);
    this->frontLSDparam2 = clamp(rec->accelFMin, rec->accelF, rec->accelFMax);
    this->rearLSDparam2 = clamp(rec->accelRMin, rec->accelR, rec->accelRMax);
    this->frontLSDparam3 = clamp(rec->decelFMin, rec->decelF, rec->decelFMax);
    this->rearLSDparam3 = clamp(rec->decelRMin, rec->decelR, rec->decelRMax);
}

// FUNC 0x002324D0 CarEquipments::resetAscc
void CarEquipments::resetAscc()
{
    const AsccRecord *rec = (const AsccRecord *)getRecord(CARDB_ASCC, this->ascc);

    this->settingASCC = clamp(1, rec->level0, rec->count0);
    this->asccParam[0] = clamp(1, rec->level1, rec->count1);
    this->asccParam[1] = clamp(1, rec->level2, rec->count2);
    this->asccParam[2] = clamp(1, rec->level3, rec->count3);
}

// FUNC 0x00232570 CarEquipments::resetTcsc
void CarEquipments::resetTcsc()
{
    const TcsRecord *rec = (const TcsRecord *)getRecord(CARDB_TCSC, this->tcsc);

    this->settingTCSC = clamp(1, rec->level, rec->count);
}

/* ---- the car parameter block ------------------------------------------- */

/* Starts the torque factors at 1 (torqueTop 1000 = x 0.001, torqueBottom
 * 100 = x 0.01) and no turbo, then each part puts its values in or scales
 * the ones before it.  Quirk: port polish (table 7) is applied twice, after
 * the turbine kit and again after the clutch.  The parts done without a test
 * of their code (brake .. rear tyres, racing modify) have a record whatever
 * the car; the records are not checked for NULL. */
// FUNC 0x002325B8 CarEquipments::getSpec
void CarEquipments::getSpec(CarSpec *spec)
{
    int i;

    spec->torqueBottom = 100;
    spec->turboMax = 0;
    spec->torqueTop = 1000;
    specBrake(spec, (const BrakeRecord *)getRecord(CARDB_BRAKE, this->brake));
    specBrakeController(spec, (const BrakeControllerRecord *)
                        getRecord(CARDB_BRAKE_CONTROLLER, this->brakeController));
    specSteer(spec, (const SteerRecord *)getRecord(CARDB_STEER, this->steer));
    specChassis(spec, (const ChassisRecord *)getRecord(CARDB_CHASSIS, this->chassis));
    specEngine(spec, (const EngineRecord *)getRecord(CARDB_ENGINE, this->engine));
    specDrivetrain(spec, (const DrivetrainRecord *)getRecord(CARDB_DRIVETRAIN, this->drivetrain));
    specGear(spec, (const TransmissionRecord *)getRecord(CARDB_GEAR, this->gear));
    specSuspension(spec, (const SuspensionRecord *)getRecord(CARDB_SUSPENSION, this->suspension));
    specLsd(spec, (const LsdRecord *)getRecord(CARDB_LSD, this->lsd));
    specTiresFront(spec, (const TiresRecord *)getRecord(CARDB_FRONT_TIRE, this->frontTire));
    specTiresRear(spec, (const TiresRecord *)getRecord(CARDB_REAR_TIRE, this->rearTire));
    if (this->lightweight != 0)
        specLightweight(spec, (const LightweightRecord *)getRecord(CARDB_LIGHTWEIGHT, this->lightweight));
    specRacingModify(spec, (const RacingModifyRecord *)getRecord(CARDB_RACING_MODIFY, this->racingModify));
    if (this->portPolish != 0)
        specPortPolish(spec, (const PowerRecord *)getRecord(CARDB_PORT_POLISH, this->portPolish));
    if (this->engineBalance != 0)
        specEngineBalance(spec, (const EngineTuneRecord *)getRecord(CARDB_ENGINE_BALANCE, this->engineBalance));
    if (this->displacement != 0)
        specDisplacement(spec, (const PowerRecord *)getRecord(CARDB_DISPLACEMENT, this->displacement));
    if (this->computer != 0)
        specComputer(spec, (const PowerRecord *)getRecord(CARDB_COMPUTER, this->computer));
    if (this->naTune != 0)
        specNaTune(spec, (const EngineTuneRecord *)getRecord(CARDB_NA_TUNE, this->naTune));
    if (this->turbine != 0)
        specTurbineKit(spec, (const TurbineKitRecord *)getRecord(CARDB_TURBINE, this->turbine));
    if (this->flywheel != 0)
        specFlywheel(spec, (const InertiaRecord *)getRecord(CARDB_FLYWHEEL, this->flywheel));
    if (this->clutch != 0)
        specClutch(spec, (const InertiaRecord *)getRecord(CARDB_CLUTCH, this->clutch));
    if (this->portPolish != 0)          /* again (see above) */
        specPortPolish(spec, (const PowerRecord *)getRecord(CARDB_PORT_POLISH, this->portPolish));
    if (this->propellerShaft != 0)
        specPropellerShaft(spec, (const PropellerShaftRecord *)
                           getRecord(CARDB_PROPELLER_SHAFT, this->propellerShaft));
    if (this->muffler != 0)
        specMuffler(spec, (const PowerRecord *)getRecord(CARDB_MUFFLER, this->muffler));
    if (this->intercooler != 0)
        specIntercooler(spec, (const PowerRecord *)getRecord(CARDB_INTERCOOLER, this->intercooler));
    if (this->ascc != 0)
        specAscc(spec, (const AsccRecord *)getRecord(CARDB_ASCC, this->ascc));
    if (this->tcsc != 0)
        specTcsc(spec, (const TcsRecord *)getRecord(CARDB_TCSC, this->tcsc));
    if (this->wheel != 0)
        specWheel(spec, (const WheelRecord *)getRecord(CARDB_WHEEL, this->wheel));
    spec->rollInertia = spec->yawInertia / 2;   /* `/ 2' (a u8): two loads, as retail */
    spec->pitchInertia = spec->yawInertia;
    /* [0] is the reverse gear, [1..gearCount] the forward gears */
    for (i = spec->gearCount + 1; i < 8; i++)
        spec->gearRatio[i] = -1;
    for (i = spec->curveCount; i < 16; i++) {
        spec->curveRpm[i] = 0xFF;
        spec->curveTorque[i] = 0;
    }
    if (this->unk_1B8 != 0)
        spec->torqueGain = this->unk_1B8;
    spec->unk_00F = 9;
}

// FUNC 0x00232A68 CarEquipments::specBrake
void CarEquipments::specBrake(CarSpec *spec, const BrakeRecord *rec)
{
    spec->brakeTorque[0] = rec->unk_11;
    spec->brakeTorque[1] = rec->unk_12;
    spec->brakeTorque[2] = rec->unk_13;
}

// FUNC 0x00232A88 CarEquipments::specBrakeController
void CarEquipments::specBrakeController(CarSpec *spec, const BrakeControllerRecord *rec)
{
    spec->brakeBalance[0] = interpolate(rec->fromF, rec->toF, rec->countF, this->frontABS);
    spec->brakeBalance[1] = interpolate(rec->fromR, rec->toR, rec->countR, this->rearABS);
}

/* The record's three 12-byte curves go to the spec's counted curves 2, 0,
 * 1, element by element (retail has one lbu / sb per byte: a struct copy
 * is a block move, lwl / lwr).  steerLock first: its load is scheduled
 * ahead of the count store. */
// FUNC 0x00232AF8 CarEquipments::specSteer
void CarEquipments::specSteer(CarSpec *spec, const SteerRecord *rec)
{
    spec->steerLock = rec->unk_11;
    spec->steer[2].count = 6;
    spec->steer[2].c.x.v[0] = rec->curve[0].x.v[0]; spec->steer[2].c.x.v[1] = rec->curve[0].x.v[1];
    spec->steer[2].c.x.v[2] = rec->curve[0].x.v[2]; spec->steer[2].c.x.v[3] = rec->curve[0].x.v[3];
    spec->steer[2].c.x.v[4] = rec->curve[0].x.v[4]; spec->steer[2].c.x.v[5] = rec->curve[0].x.v[5];
    spec->steer[2].c.y.v[0] = rec->curve[0].y.v[0]; spec->steer[2].c.y.v[1] = rec->curve[0].y.v[1];
    spec->steer[2].c.y.v[2] = rec->curve[0].y.v[2]; spec->steer[2].c.y.v[3] = rec->curve[0].y.v[3];
    spec->steer[2].c.y.v[4] = rec->curve[0].y.v[4]; spec->steer[2].c.y.v[5] = rec->curve[0].y.v[5];
    spec->steer[0].count = 6;
    spec->steer[0].c.x.v[0] = rec->curve[1].x.v[0]; spec->steer[0].c.x.v[1] = rec->curve[1].x.v[1];
    spec->steer[0].c.x.v[2] = rec->curve[1].x.v[2]; spec->steer[0].c.x.v[3] = rec->curve[1].x.v[3];
    spec->steer[0].c.x.v[4] = rec->curve[1].x.v[4]; spec->steer[0].c.x.v[5] = rec->curve[1].x.v[5];
    spec->steer[0].c.y.v[0] = rec->curve[1].y.v[0]; spec->steer[0].c.y.v[1] = rec->curve[1].y.v[1];
    spec->steer[0].c.y.v[2] = rec->curve[1].y.v[2]; spec->steer[0].c.y.v[3] = rec->curve[1].y.v[3];
    spec->steer[0].c.y.v[4] = rec->curve[1].y.v[4]; spec->steer[0].c.y.v[5] = rec->curve[1].y.v[5];
    spec->steer[1].count = 6;
    spec->steer[1].c.x.v[0] = rec->curve[2].x.v[0]; spec->steer[1].c.x.v[1] = rec->curve[2].x.v[1];
    spec->steer[1].c.x.v[2] = rec->curve[2].x.v[2]; spec->steer[1].c.x.v[3] = rec->curve[2].x.v[3];
    spec->steer[1].c.x.v[4] = rec->curve[2].x.v[4]; spec->steer[1].c.x.v[5] = rec->curve[2].x.v[5];
    spec->steer[1].c.y.v[0] = rec->curve[2].y.v[0]; spec->steer[1].c.y.v[1] = rec->curve[2].y.v[1];
    spec->steer[1].c.y.v[2] = rec->curve[2].y.v[2]; spec->steer[1].c.y.v[3] = rec->curve[2].y.v[3];
    spec->steer[1].c.y.v[4] = rec->curve[2].y.v[4]; spec->steer[1].c.y.v[5] = rec->curve[2].y.v[5];
}

// FUNC 0x00232C38 CarEquipments::specChassis
void CarEquipments::specChassis(CarSpec *spec, const ChassisRecord *rec)
{
    spec->frontWeight = rec->unk_10;
    spec->axleGrip[0] = rec->unk_12 + 124;
    spec->axleGrip[1] = rec->unk_1E + 124;
    spec->wheelBase = rec->wheelBase;
    spec->weight = rec->weight;
    spec->yawInertia = rec->unk_1C;
    spec->cgOffset = rec->unk_1D;
    spec->length = rec->length;
}

/* The torque curve (all 16 points; getSpec fills those past curveCount). */
// FUNC 0x00232C88 CarEquipments::specEngine
void CarEquipments::specEngine(CarSpec *spec, const EngineRecord *rec)
{
    spec->curveTorque[0] = rec->curveTorque[0];
    spec->curveTorque[1] = rec->curveTorque[1];
    spec->curveTorque[2] = rec->curveTorque[2];
    spec->curveTorque[3] = rec->curveTorque[3];
    spec->curveTorque[4] = rec->curveTorque[4];
    spec->curveTorque[5] = rec->curveTorque[5];
    spec->curveTorque[6] = rec->curveTorque[6];
    spec->curveTorque[7] = rec->curveTorque[7];
    spec->curveTorque[8] = rec->curveTorque[8];
    spec->curveTorque[9] = rec->curveTorque[9];
    spec->curveTorque[10] = rec->curveTorque[10];
    spec->curveTorque[11] = rec->curveTorque[11];
    spec->curveTorque[12] = rec->curveTorque[12];
    spec->curveTorque[13] = rec->curveTorque[13];
    spec->curveTorque[14] = rec->curveTorque[14];
    spec->curveTorque[15] = rec->curveTorque[15];
    spec->torqueGain = rec->unk_42 * 10;
    spec->idleRpm = rec->idleRpm;
    spec->idle = rec->unk_44;
    spec->autoGearRpm = rec->autoGearRpm;
    spec->revLimit = rec->revLimit;
    spec->curveRpm[0] = rec->curveRpm[0];
    spec->curveRpm[1] = rec->curveRpm[1];
    spec->curveRpm[2] = rec->curveRpm[2];
    spec->curveRpm[3] = rec->curveRpm[3];
    spec->curveRpm[4] = rec->curveRpm[4];
    spec->curveRpm[5] = rec->curveRpm[5];
    spec->curveRpm[6] = rec->curveRpm[6];
    spec->curveRpm[7] = rec->curveRpm[7];
    spec->curveRpm[8] = rec->curveRpm[8];
    spec->curveRpm[9] = rec->curveRpm[9];
    spec->curveRpm[10] = rec->curveRpm[10];
    spec->curveRpm[11] = rec->curveRpm[11];
    spec->curveRpm[12] = rec->curveRpm[12];
    spec->curveRpm[13] = rec->curveRpm[13];
    spec->curveRpm[14] = rec->curveRpm[14];
    spec->curveRpm[15] = rec->curveRpm[15];
    spec->curveCount = rec->curveCount;
}

/* awdSplit gets the record's VCD default, not the param4WD setting. */
// FUNC 0x00232DC8 CarEquipments::specDrivetrain
void CarEquipments::specDrivetrain(CarSpec *spec, const DrivetrainRecord *rec)
{
    spec->awdSplit = rec->vcd;
    spec->layout = rec->layout;
    spec->awdType = rec->awdType;
    spec->torqueUnit = rec->unk_16;
    spec->unk_0D2[0] = rec->unk_18[0];
    spec->unk_0D2[1] = rec->unk_18[1];
    spec->unk_0CE[0] = rec->unk_1C[0];
    spec->unk_0CE[1] = rec->unk_1C[1];
    spec->flywheel = rec->flywheel;
}

/* The ratios and the final gear from the settings (low halves), the record
 * gives only the number of gears; manual ratios (autoGear 0). */
// FUNC 0x00232E18 CarEquipments::specGear
void CarEquipments::specGear(CarSpec *spec, const TransmissionRecord *rec)
{
    int i;

    for (i = 0; i < 8; i++)
        spec->gearRatio[i] = this->gearRatio[i];
    spec->finalGear = this->finalGearRatio;
    spec->autoGearLevel = this->maxSpeed;
    spec->gearCount = rec->gears;
    spec->autoGear = 0;
}

/* Quirk: the front rebound values are interpolated from the front bound
 * settings (frontDamperF1B / F2B), not from frontDamperF1R / F2R; the rear
 * ones use the rear rebound settings. */
// FUNC 0x00232E68 CarEquipments::specSuspension
void CarEquipments::specSuspension(CarSpec *spec, const SuspensionRecord *rec)
{
    int count;

    spec->strokeLimit[0] = rec->unk_1D[0];
    spec->strokeLimit[1] = rec->unk_1D[1];
    spec->strokeExt[0] = rec->unk_1D[2];
    spec->strokeExt[1] = rec->unk_1D[3];
    spec->susp[0].bumpStop = rec->unk_29;
    spec->susp[1].bumpStop = rec->unk_2A;
    spec->camberX[0][0] = rec->unk_60[0];
    spec->camberX[0][1] = rec->unk_60[1];
    spec->camberX[0][2] = rec->unk_60[2];
    spec->camberX[0][3] = rec->unk_60[3];
    spec->camberY[0][0] = rec->unk_64[0];
    spec->camberY[0][1] = rec->unk_64[1];
    spec->camberY[0][2] = rec->unk_64[2];
    spec->camberY[0][3] = rec->unk_64[3];
    spec->camberX[1][0] = rec->unk_68[0];
    spec->camberX[1][1] = rec->unk_68[1];
    spec->camberX[1][2] = rec->unk_68[2];
    spec->camberX[1][3] = rec->unk_68[3];
    spec->camberY[1][0] = rec->unk_6C[0];
    spec->camberY[1][1] = rec->unk_6C[1];
    spec->camberY[1][2] = rec->unk_6C[2];
    spec->camberY[1][3] = rec->unk_6C[3];
    spec->camberGain[0] = rec->unk_5E[0];
    spec->camberGain[1] = rec->unk_5E[1];
    spec->susp[0].unsprungMass = rec->unk_5C[0];
    spec->susp[1].unsprungMass = rec->unk_5C[1];
    spec->susp[0].bumpSlowSpeed = rec->unk_70[0];
    spec->susp[0].bumpFastSpeed = rec->unk_70[1];
    spec->susp[0].reboundSlowSpeed = rec->unk_70[2];
    spec->susp[0].reboundFastSpeed = rec->unk_70[3];
    spec->susp[1].bumpSlowSpeed = rec->unk_74[0];
    spec->susp[1].bumpFastSpeed = rec->unk_74[1];
    spec->susp[1].reboundSlowSpeed = rec->unk_74[2];
    spec->susp[1].reboundFastSpeed = rec->unk_74[3];
    spec->susp[0].bumpStopStroke = rec->unk_78[0];
    spec->susp[1].bumpStopStroke = rec->unk_78[1];
    spec->susp[0].bumpStopDamp = rec->unk_7A[0];
    spec->susp[1].bumpStopDamp = rec->unk_7A[1];
    spec->camber[0] = this->frontCamber;
    spec->camber[1] = this->rearCamber;
    spec->rideHeight[0] = this->frontRideheight;
    spec->rideHeight[1] = this->rearRideheight;
    spec->toe[0] = this->frontToe;
    spec->toe[1] = this->rearToe;
    spec->susp[0].springRate = this->frontSpringrate;
    spec->susp[1].springRate = this->rearSpringrate;
    spec->suspRate[0] = this->unk_164;
    spec->suspRate[1] = this->unk_168;
    count = rec->damperFCount;
    spec->susp[0].damper = interpolate(rec->damperFFrom, rec->damperFTo, count, this->frontDamperF1B);
    spec->susp[0].damper2 = interpolate(rec->damperF2From, rec->damperF2To, count, this->frontDamperF2B);
    count = rec->reboundFCount;
    spec->susp[0].rebound = interpolate(rec->reboundFFrom, rec->reboundFTo, count, this->frontDamperF1B);
    spec->susp[0].rebound2 = interpolate(rec->reboundF2From, rec->reboundF2To, count, this->frontDamperF2B);
    count = rec->damperRCount;
    spec->susp[1].damper = interpolate(rec->damperRFrom, rec->damperRTo, count, this->rearDamperF1B);
    spec->susp[1].damper2 = interpolate(rec->damperR2From, rec->damperR2To, count, this->rearDamperF2B);
    count = rec->reboundRCount;
    spec->susp[1].rebound = interpolate(rec->reboundRFrom, rec->reboundRTo, count, this->rearDamperF1R);
    spec->susp[1].rebound2 = interpolate(rec->reboundR2From, rec->reboundR2To, count, this->rearDamperF2R);
    spec->susp[0].stabilizer = interpolate(rec->stabilizerFFrom, rec->stabilizerFTo,
                                           rec->stabilizerFCount, this->frontStabilizer);
    spec->susp[1].stabilizer = interpolate(rec->stabilizerRFrom, rec->stabilizerRTo,
                                           rec->stabilizerRCount, this->rearStabilizer);
}

// FUNC 0x00233150 CarEquipments::specLsd
void CarEquipments::specLsd(CarSpec *spec, const LsdRecord *rec)
{
    spec->diffCode[0] = rec->diffCodeF;
    spec->diffCode[1] = rec->diffCodeR;
    spec->diffInitial[0] = this->frontLSDparam;
    spec->diffInitial[1] = this->rearLSDparam;
    spec->diffAccel[0] = this->frontLSDparam2;
    spec->diffAccel[1] = this->rearLSDparam2;
    spec->diffDecel[0] = this->frontLSDparam3;
    spec->diffDecel[1] = this->rearLSDparam3;
}

/* Tyre blocks 0..2 from the part's three compounds (each with its force
 * volume); the size; and both camberPoints (front and rear) = 4.  The two
 * code lists are walked with pointers, the tyre block by index (retail's
 * register order: the pointers start before the loop, the block's pointer
 * is a loop giv initialised last). */
// FUNC 0x00233198 CarEquipments::specTiresFront
void CarEquipments::specTiresFront(CarSpec *spec, const TiresRecord *rec)
{
    const TireSizeRecord *size;
    int i;

    const u64 *compoundCode = rec->compound;
    const u64 *forceVolCode = rec->forceVol;

    for (i = 0; i < 3; i++) {
        const TireCompoundRecord *compound =
            (const TireCompoundRecord *)getRecord(CARDB_TIRE_COMPOUND, *compoundCode++);
        const TireForceVolRecord *forceVol =
            (const TireForceVolRecord *)getRecord(CARDB_TIRE_FORCE_VOL, *forceVolCode++);

        specTire(&spec->tyre[i], compound, forceVol);
    }
    size = (const TireSizeRecord *)getRecord(CARDB_TIRE_SIZE, rec->size);
    spec->tyreRim[0] = size->rim;
    spec->tyreWidth[0] = size->width;
    spec->tyreAspect[0] = size->aspect;
    spec->camberPoints[0] = 4;
    spec->camberPoints[1] = 4;
}

/* Tyre blocks 3..5 and the rear size (as specTiresFront). */
// FUNC 0x002332A0 CarEquipments::specTiresRear
void CarEquipments::specTiresRear(CarSpec *spec, const TiresRecord *rec)
{
    const TireSizeRecord *size;
    int i;

    const u64 *compoundCode = rec->compound;
    const u64 *forceVolCode = rec->forceVol;

    for (i = 0; i < 3; i++) {
        const TireCompoundRecord *compound =
            (const TireCompoundRecord *)getRecord(CARDB_TIRE_COMPOUND, *compoundCode++);
        const TireForceVolRecord *forceVol =
            (const TireForceVolRecord *)getRecord(CARDB_TIRE_FORCE_VOL, *forceVolCode++);

        specTire(&spec->tyre[3 + i], compound, forceVol);
    }
    size = (const TireSizeRecord *)getRecord(CARDB_TIRE_SIZE, rec->size);
    spec->tyreRim[1] = size->rim;
    spec->tyreWidth[1] = size->width;
    spec->tyreAspect[1] = size->aspect;
}

/* One tyre block: the compound's curves with their point counts, the force
 * volume's factors; element by element (one lbu / sb per byte in retail, as
 * specSteer). */
// FUNC 0x002333A0 CarEquipments::specTire
void CarEquipments::specTire(CarSpecTyre *tyre, const TireCompoundRecord *compound,
                             const TireForceVolRecord *forceVol)
{
    tyre->grip = compound->grip;
    tyre->maxAngle = compound->maxAngle;
    tyre->unk_6B = compound->unk_52;
    tyre->curve0Count = 8;
    tyre->curve0.x[0] = compound->curve0.x[0]; tyre->curve0.x[1] = compound->curve0.x[1];
    tyre->curve0.x[2] = compound->curve0.x[2]; tyre->curve0.x[3] = compound->curve0.x[3];
    tyre->curve0.x[4] = compound->curve0.x[4]; tyre->curve0.x[5] = compound->curve0.x[5];
    tyre->curve0.x[6] = compound->curve0.x[6]; tyre->curve0.x[7] = compound->curve0.x[7];
    tyre->curve0.y[0] = compound->curve0.y[0]; tyre->curve0.y[1] = compound->curve0.y[1];
    tyre->curve0.y[2] = compound->curve0.y[2]; tyre->curve0.y[3] = compound->curve0.y[3];
    tyre->curve0.y[4] = compound->curve0.y[4]; tyre->curve0.y[5] = compound->curve0.y[5];
    tyre->curve0.y[6] = compound->curve0.y[6]; tyre->curve0.y[7] = compound->curve0.y[7];
    tyre->curve1Count = 6;
    tyre->curve1.v[0][0] = compound->curve1.v[0][0]; tyre->curve1.v[0][1] = compound->curve1.v[0][1];
    tyre->curve1.v[0][2] = compound->curve1.v[0][2]; tyre->curve1.v[0][3] = compound->curve1.v[0][3];
    tyre->curve1.v[0][4] = compound->curve1.v[0][4]; tyre->curve1.v[0][5] = compound->curve1.v[0][5];
    tyre->curve1.v[1][0] = compound->curve1.v[1][0]; tyre->curve1.v[1][1] = compound->curve1.v[1][1];
    tyre->curve1.v[1][2] = compound->curve1.v[1][2]; tyre->curve1.v[1][3] = compound->curve1.v[1][3];
    tyre->curve1.v[1][4] = compound->curve1.v[1][4]; tyre->curve1.v[1][5] = compound->curve1.v[1][5];
    tyre->curve1.v[2][0] = compound->curve1.v[2][0]; tyre->curve1.v[2][1] = compound->curve1.v[2][1];
    tyre->curve1.v[2][2] = compound->curve1.v[2][2]; tyre->curve1.v[2][3] = compound->curve1.v[2][3];
    tyre->curve1.v[2][4] = compound->curve1.v[2][4]; tyre->curve1.v[2][5] = compound->curve1.v[2][5];
    tyre->curve2Count = 6;
    tyre->curve2.v[0][0] = compound->curve2.v[0][0]; tyre->curve2.v[0][1] = compound->curve2.v[0][1];
    tyre->curve2.v[0][2] = compound->curve2.v[0][2]; tyre->curve2.v[0][3] = compound->curve2.v[0][3];
    tyre->curve2.v[0][4] = compound->curve2.v[0][4]; tyre->curve2.v[0][5] = compound->curve2.v[0][5];
    tyre->curve2.v[1][0] = compound->curve2.v[1][0]; tyre->curve2.v[1][1] = compound->curve2.v[1][1];
    tyre->curve2.v[1][2] = compound->curve2.v[1][2]; tyre->curve2.v[1][3] = compound->curve2.v[1][3];
    tyre->curve2.v[1][4] = compound->curve2.v[1][4]; tyre->curve2.v[1][5] = compound->curve2.v[1][5];
    tyre->curve2.v[2][0] = compound->curve2.v[2][0]; tyre->curve2.v[2][1] = compound->curve2.v[2][1];
    tyre->curve2.v[2][2] = compound->curve2.v[2][2]; tyre->curve2.v[2][3] = compound->curve2.v[2][3];
    tyre->curve2.v[2][4] = compound->curve2.v[2][4]; tyre->curve2.v[2][5] = compound->curve2.v[2][5];
    tyre->curve3Count = 4;
    tyre->curve3.x[0] = compound->curve3.x[0]; tyre->curve3.x[1] = compound->curve3.x[1];
    tyre->curve3.x[2] = compound->curve3.x[2]; tyre->curve3.x[3] = compound->curve3.x[3];
    tyre->curve3.y[0] = compound->curve3.y[0]; tyre->curve3.y[1] = compound->curve3.y[1];
    tyre->curve3.y[2] = compound->curve3.y[2]; tyre->curve3.y[3] = compound->curve3.y[3];
    tyre->curve4Count = 6;
    tyre->curve4.x.v[0] = compound->curve4.x.v[0]; tyre->curve4.x.v[1] = compound->curve4.x.v[1];
    tyre->curve4.x.v[2] = compound->curve4.x.v[2]; tyre->curve4.x.v[3] = compound->curve4.x.v[3];
    tyre->curve4.x.v[4] = compound->curve4.x.v[4]; tyre->curve4.x.v[5] = compound->curve4.x.v[5];
    tyre->curve4.y.v[0] = compound->curve4.y.v[0]; tyre->curve4.y.v[1] = compound->curve4.y.v[1];
    tyre->curve4.y.v[2] = compound->curve4.y.v[2]; tyre->curve4.y.v[3] = compound->curve4.y.v[3];
    tyre->curve4.y.v[4] = compound->curve4.y.v[4]; tyre->curve4.y.v[5] = compound->curve4.y.v[5];
    tyre->unk_4B[0] = forceVol->unk_08[0]; tyre->unk_4B[1] = forceVol->unk_08[1];
    tyre->unk_4B[2] = forceVol->unk_08[2]; tyre->unk_4B[3] = forceVol->unk_08[3];
    tyre->unk_4B[4] = forceVol->unk_08[4]; tyre->unk_4B[5] = forceVol->unk_08[5];
    tyre->unk_4B[6] = forceVol->unk_08[6]; tyre->unk_4B[7] = forceVol->unk_08[7];
    tyre->unk_4B[8] = forceVol->unk_08[8]; tyre->unk_4B[9] = forceVol->unk_08[9];
    tyre->unk_4B[12] = forceVol->unk_12;
    tyre->unk_4B[13] = forceVol->unk_13;
    tyre->unk_4B[14] = forceVol->unk_14;
    tyre->unk_5B = 0;
    tyre->unk_5C = 0;
    tyre->unk_5D = 0;
}

// FUNC 0x00233690 CarEquipments::specLightweight
void CarEquipments::specLightweight(CarSpec *spec, const LightweightRecord *rec)
{
    spec->weight = spec->weight * rec->weightRate / 1000;
    spec->yawInertia = spec->yawInertia * rec->rate / 100;
}

/* The float sizes are truncated (cvt.w.s). */
// FUNC 0x002336D8 CarEquipments::specRacingModify
void CarEquipments::specRacingModify(CarSpec *spec, const RacingModifyRecord *rec)
{
    spec->weight = spec->weight * rec->weightRate / 1000;
    spec->yawInertia = spec->yawInertia * rec->rate / 100;
    spec->dragShare = rec->unk_39;
    spec->treadFront = rec->unk_32;
    spec->treadRear = rec->unk_34;
    spec->aeroF = this->clF;
    spec->aeroR = this->clR;
    spec->trackOffset[0] = rec->unk_3D;
    spec->trackOffset[1] = rec->unk_3E;
    spec->width = (int)rec->unk_28;
    spec->height = (int)rec->unk_2C;
    spec->bodyFront = (int)rec->unk_20;
    spec->bodyRear = (int)rec->unk_24;
    spec->swingArm[0] = rec->unk_3F;
    spec->swingArm[1] = rec->unk_40;
    spec->frontalArea = (int)(rec->unk_44 * 10000.0f);
}

/* The engine power parts scale the torque factors by their rates (%). */
// FUNC 0x002337C0 CarEquipments::specPortPolish
void CarEquipments::specPortPolish(CarSpec *spec, const PowerRecord *rec)
{
    spec->torqueTop = spec->torqueTop * rec->powerRate / 100;
    spec->torqueBottom = spec->torqueBottom * rec->rate / 100;
}

/* The same as specNaTune: the curve's rpm points and the auto gears' rpm
 * shifted up, the torque factors scaled. */
// FUNC 0x00233808 CarEquipments::specEngineBalance
void CarEquipments::specEngineBalance(CarSpec *spec, const EngineTuneRecord *rec)
{
    spec->curveRpm[0] += rec->curveRpmAdd;
    spec->curveRpm[1] += rec->curveRpmAdd;
    spec->curveRpm[2] += rec->curveRpmAdd;
    spec->curveRpm[3] += rec->curveRpmAdd;
    spec->curveRpm[4] += rec->curveRpmAdd;
    spec->curveRpm[5] += rec->curveRpmAdd;
    spec->curveRpm[6] += rec->curveRpmAdd;
    spec->curveRpm[7] += rec->curveRpmAdd;
    spec->curveRpm[8] += rec->curveRpmAdd;
    spec->curveRpm[9] += rec->curveRpmAdd;
    spec->curveRpm[10] += rec->curveRpmAdd;
    spec->curveRpm[11] += rec->curveRpmAdd;
    spec->curveRpm[12] += rec->curveRpmAdd;
    spec->curveRpm[13] += rec->curveRpmAdd;
    spec->curveRpm[14] += rec->curveRpmAdd;
    spec->curveRpm[15] += rec->curveRpmAdd;
    spec->autoGearRpm += rec->autoGearRpmAdd;
    spec->torqueTop = spec->torqueTop * rec->powerRate / 100;
    spec->torqueBottom = spec->torqueBottom * rec->rate / 100;
}

// FUNC 0x00233960 CarEquipments::specDisplacement
void CarEquipments::specDisplacement(CarSpec *spec, const PowerRecord *rec)
{
    spec->torqueTop = spec->torqueTop * rec->powerRate / 100;
    spec->torqueBottom = spec->torqueBottom * rec->rate / 100;
}

// FUNC 0x002339A8 CarEquipments::specComputer
void CarEquipments::specComputer(CarSpec *spec, const PowerRecord *rec)
{
    spec->torqueTop = spec->torqueTop * rec->powerRate / 100;
    spec->torqueBottom = spec->torqueBottom * rec->rate / 100;
}

// FUNC 0x002339F0 CarEquipments::specNaTune
void CarEquipments::specNaTune(CarSpec *spec, const EngineTuneRecord *rec)
{
    spec->curveRpm[0] += rec->curveRpmAdd;
    spec->curveRpm[1] += rec->curveRpmAdd;
    spec->curveRpm[2] += rec->curveRpmAdd;
    spec->curveRpm[3] += rec->curveRpmAdd;
    spec->curveRpm[4] += rec->curveRpmAdd;
    spec->curveRpm[5] += rec->curveRpmAdd;
    spec->curveRpm[6] += rec->curveRpmAdd;
    spec->curveRpm[7] += rec->curveRpmAdd;
    spec->curveRpm[8] += rec->curveRpmAdd;
    spec->curveRpm[9] += rec->curveRpmAdd;
    spec->curveRpm[10] += rec->curveRpmAdd;
    spec->curveRpm[11] += rec->curveRpmAdd;
    spec->curveRpm[12] += rec->curveRpmAdd;
    spec->curveRpm[13] += rec->curveRpmAdd;
    spec->curveRpm[14] += rec->curveRpmAdd;
    spec->curveRpm[15] += rec->curveRpmAdd;
    spec->autoGearRpm += rec->autoGearRpmAdd;
    spec->torqueTop = spec->torqueTop * rec->powerRate / 100;
    spec->torqueBottom = spec->torqueBottom * rec->rate / 100;
}

/* The torque factors, the boost cap, the rpm limits, and the two turbo
 * stages from the turbo settings. */
// FUNC 0x00233B48 CarEquipments::specTurbineKit
void CarEquipments::specTurbineKit(CarSpec *spec, const TurbineKitRecord *rec)
{
    spec->torqueTop = spec->torqueTop * rec->powerRate / 100;
    spec->torqueBottom = spec->torqueBottom * rec->rate / 100;
    spec->turboMax = rec->turbo;
    spec->autoGearRpm += rec->autoGearRpmAdd;
    spec->revLimit += rec->revLimitAdd;
    spec->turboBoost[0] = this->boost1;
    spec->turboRpm[0] = this->peekRPM1;
    spec->turboRate[0] = this->response1;
    spec->turboBoost[1] = this->boost2;
    spec->turboRpm[1] = this->peekRPM2;
    spec->turboRate[1] = this->response2;
}

// FUNC 0x00233BE8 CarEquipments::specFlywheel
void CarEquipments::specFlywheel(CarSpec *spec, const InertiaRecord *rec)
{
    spec->torqueUnit = spec->torqueUnit * rec->rate06C / 100;
    spec->flywheel = spec->flywheel * rec->rateFlywheel / 100;
    spec->unk_0D2[0] = spec->unk_0D2[0] * rec->rate0D2[0] / 100;
    spec->unk_0D2[1] = spec->unk_0D2[1] * rec->rate0D2[1] / 100;
}

// FUNC 0x00233C68 CarEquipments::specClutch
void CarEquipments::specClutch(CarSpec *spec, const InertiaRecord *rec)
{
    spec->torqueUnit = spec->torqueUnit * rec->rate06C / 100;
    spec->flywheel = spec->flywheel * rec->rateFlywheel / 100;
    spec->unk_0D2[0] = spec->unk_0D2[0] * rec->rate0D2[0] / 100;
    spec->unk_0D2[1] = spec->unk_0D2[1] * rec->rate0D2[1] / 100;
    spec->unk_0CC = rec->unk_15;
}

// FUNC 0x00233CF0 CarEquipments::specPropellerShaft
void CarEquipments::specPropellerShaft(CarSpec *spec, const PropellerShaftRecord *rec)
{
    spec->torqueUnit = spec->torqueUnit * rec->rate06C / 100;
    spec->unk_0D2[0] = spec->unk_0D2[0] * rec->rate0D2[0] / 100;
    spec->unk_0D2[1] = spec->unk_0D2[1] * rec->rate0D2[1] / 100;
    spec->unk_0CE[0] = spec->unk_0CE[0] * rec->rate0CE[0] / 100;
    spec->unk_0CE[1] = spec->unk_0CE[1] * rec->rate0CE[1] / 100;
}

// FUNC 0x00233D88 CarEquipments::specMuffler
void CarEquipments::specMuffler(CarSpec *spec, const PowerRecord *rec)
{
    spec->torqueTop = spec->torqueTop * rec->powerRate / 100;
    spec->torqueBottom = spec->torqueBottom * rec->rate / 100;
}

// FUNC 0x00233DD0 CarEquipments::specIntercooler
void CarEquipments::specIntercooler(CarSpec *spec, const PowerRecord *rec)
{
    spec->torqueTop = spec->torqueTop * rec->powerRate / 100;
    spec->torqueBottom = spec->torqueBottom * rec->rate / 100;
}

/* The four ASCC levels interpolated; ascFlags packs two bits of each flag
 * byte (bits 0-1 +0x17, 2-3 +0x16, 4-5 +0x1D, 6-7 +0x1C). */
// FUNC 0x00233E18 CarEquipments::specAscc
void CarEquipments::specAscc(CarSpec *spec, const AsccRecord *rec)
{
    int flags;

    spec->asc0 = interpolate(rec->from0, rec->to0, rec->count0, this->settingASCC);
    spec->brakeSlip = rec->unk_13;
    spec->asc1 = interpolate(rec->from1, rec->to1, rec->count1, this->asccParam[0]);
    spec->asc2 = interpolate(rec->from2, rec->to2, rec->count2, this->asccParam[1]);
    spec->brakeSlip2 = rec->unk_22;
    spec->asc3 = interpolate(rec->from3, rec->to3, rec->count3, this->asccParam[2]);
    flags = rec->flag17 & 3;
    flags |= (rec->flag16 & 3) << 2;
    flags |= (rec->flag1D & 3) << 4;
    flags |= (rec->flag1C & 3) << 6;
    spec->ascFlags = flags;
}

// FUNC 0x00233F08 CarEquipments::specTcsc
void CarEquipments::specTcsc(CarSpec *spec, const TcsRecord *rec)
{
    spec->tcsThrottle = rec->unk_13;
    spec->fadeGain = rec->unk_14;
    spec->fadeStart = rec->unk_17;
    spec->tcs = interpolate(rec->from, rec->to, rec->count, this->settingTCSC);
}

/* The wheel changes nothing in the spec. */
// FUNC 0x00233F58 CarEquipments::specWheel
void CarEquipments::specWheel(CarSpec *spec, const WheelRecord *rec)
{
}

/* Level 1..count mapped linearly onto from..to (from when count < 2). */
// FUNC 0x00233F60 CarEquipments::interpolate
int CarEquipments::interpolate(int from, int to, int count, int level)
{
    if (count < 2)
        return from;
    level--;
    count--;
    return from + (to - from) * level / count;
}

/* A linear search of table `table' for a record of this car (+0x08) and of
 * the kind (+0x10); its code, or 0. */
// FUNC 0x00233F98 CarEquipments::findPart
u64 CarEquipments::findPart(int table, int kind)
{
    DataBlock *block = getDataBlock(table);
    int n = block->numElements;
    int i;

    for (i = 0; i < n; i++) {
        const PartRecord *rec = (const PartRecord *)block->getElement(i);

        if (rec->car == this->carCode && rec->kind == kind)
            return rec->code;
    }
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Records of the car                                                        */
/* ------------------------------------------------------------------------ */

/* The car whose model (and colour list) the car uses: the racing
 * modification record's model (no NULL check). */
// FUNC 0x00234020 CarEquipments::getModelCode
u64 CarEquipments::getModelCode()
{
    return ((RacingModifyRecord *)getRecord(CARDB_RACING_MODIFY, racingModify))->model;
}

/* 1 if the fitted turbine kit makes a turbo engine. */
// FUNC 0x00234048 CarEquipments::isTurbo
int CarEquipments::isTurbo()
{
    u64 code = turbine;
    if (code == 0)
        return 0;
    return ((TurbineKitRecord *)getRecord(CARDB_TURBINE, code))->turbo != 0;
}

/* The same for the car's stock turbine kit. */
// FUNC 0x00234080 CarEquipments::isTurboByDefault
int CarEquipments::isTurboByDefault()
{
    u64 code = getDefaultPartCode(CARDB_TURBINE);
    if (code == 0)
        return 0;
    return ((TurbineKitRecord *)getRecord(CARDB_TURBINE, code))->turbo != 0;
}

/* The engine record's +0x16 (no NULL check): the engine sound, the code of
 * "sound/engine/%05d" (the only caller, RaceEntryCar::loadSound 0x001777B0,
 * passes it to CarSound::load as `code'). */
// FUNC 0x002340D0 CarEquipments::getEngineSoundCode
u16 CarEquipments::getEngineSoundCode()
{
    return ((EngineRecord *)getRecord(CARDB_ENGINE, engine))->soundCode;
}

/* The exhaust sound: the muffler's kind (0 without one), + 4 for a turbo
 * engine, the index of "normal0".."normal3", "turbo0".."turbo3"
 * (g_002C2088: RaceEntryCar::loadSound passes it to CarSound::load as
 * `kind', "sound/<kind>/%05d"). */
// FUNC 0x002340F8 CarEquipments::getExhaustSoundKind
int CarEquipments::getExhaustSoundKind()
{
    u64 code = muffler;
    u8 mufflerKind = 0;
    if (code != 0)
        mufflerKind = ((PowerRecord *)getRecord(CARDB_MUFFLER, code))->kind;
    return isTurbo() ? mufflerKind + 4 : mufflerKind;
}

/* Takes the settings of a preset car (called by loadPreset 0x00231F60 and
 * T484 0x00238550 with the record of table 0x23 / 0x1F). */
// FUNC 0x00234150 CarEquipments::loadPresetSettings
void CarEquipments::loadPresetSettings(const PresetCarRecord *rec)
{
    carType = 3;                                /* a preset car's settings */
    finalGearRatio = rec->finalGearRatio;
    maxSpeed = rec->maxSpeed;
    frontLSDparam = rec->frontLSDparam;
    frontLSDparam2 = rec->frontLSDparam2;
    frontLSDparam3 = rec->frontLSDparam3;
    rearLSDparam = rec->rearLSDparam;
    rearLSDparam2 = rec->rearLSDparam2;
    rearLSDparam3 = rec->rearLSDparam3;
    clF = rec->clF;
    clR = rec->clR;
    frontDamperF1B = rec->frontDamperF1B;
    frontDamperF2B = rec->frontDamperF2B;
    frontDamperF1R = rec->frontDamperF1R;
    frontDamperF2R = rec->frontDamperF2R;
    rearDamperF1B = rec->rearDamperF1B;
    rearDamperF2B = rec->rearDamperF2B;
    rearDamperF1R = rec->rearDamperF1R;
    rearDamperF2R = rec->rearDamperF2R;
    frontCamber = rec->frontCamber;
    rearCamber = rec->rearCamber;
    frontToe = rec->frontToe;
    rearToe = rec->rearToe;
    /* from here in the record's order (retail's load / store schedule) */
    frontStabilizer = rec->frontStabilizer;
    rearStabilizer = rec->rearStabilizer;
    frontRideheight = rec->frontRideheight;
    rearRideheight = rec->rearRideheight;
    frontSpringrate = rec->frontSpringrate;
    rearSpringrate = rec->rearSpringrate;
    settingTCSC = rec->tcsc;
    settingASCC = rec->ascc;
    unk_1B8 = rec->unk_10A;
}

/* Engine texts (uniStrings; the engine record without a NULL check).  The
 * uniStrings texts go through CarDataBase::getMultiByteString (class-inline,
 * T481): retail keeps the base's `this' (+0x1D0) in a register for
 * getRecord and the string table (`uniStrings->getString' in place folds
 * the offset into each load). */
// FUNC 0x00234258 CarEquipments::getAspirationText
const char *CarEquipments::getAspirationText()
{
    EngineRecord *rec = (EngineRecord *)getRecord(CARDB_ENGINE, engine);
    return getMultiByteString(rec->aspirationText);  /* implicit: getString */
}

// FUNC 0x00234298 CarEquipments::getEngineTypeText1
const char *CarEquipments::getEngineTypeText1()
{
    EngineRecord *rec = (EngineRecord *)getRecord(CARDB_ENGINE, engine);
    return getMultiByteString(rec->typeText1);  /* implicit: getString */
}

// FUNC 0x002342D8 CarEquipments::getEngineTypeText2
const char *CarEquipments::getEngineTypeText2()
{
    EngineRecord *rec = (EngineRecord *)getRecord(CARDB_ENGINE, engine);
    return getMultiByteString(rec->typeText2);  /* implicit: getString */
}

/* The car's own record (table 0x1E). */
// FUNC 0x00234318 CarEquipments::getCarRecord
CarRecord *CarEquipments::getCarRecord()
{
    return (CarRecord *)getRecord(CARDB_CAR, carCode);
}

// FUNC 0x00234340 CarEquipments::getMakerIndex
u16 CarEquipments::getMakerIndex()
{
    return getCarRecord()->maker;
}

// FUNC 0x00234360 CarEquipments::getBrandIndex
u16 CarEquipments::getBrandIndex()
{
    return getCarRecord()->brand;
}

// FUNC 0x00234380 CarEquipments::getShopFlags
/* int, not u8: the callers' shifts are sra (lbu here either way). */
int CarEquipments::getShopFlags()
{
    return getCarRecord()->flags;
}

/* The two parts of the car's name (uniStrings), by region: the region 0 /
 * region 2 names (isJapan: Japan, isUK: the UK), else the default. */
// FUNC 0x002343A0 CarEquipments::getName1
const char *CarEquipments::getName1()
{
    u16 nameText;
    if (isJapan() != 0)
        nameText = getCarRecord()->name1R0;
    else if (isUK() != 0)
        nameText = getCarRecord()->name1R2;
    else
        nameText = getCarRecord()->name1;
    return getMultiByteString(nameText);  /* implicit: getString */
}

// FUNC 0x00234420 CarEquipments::getName2
const char *CarEquipments::getName2()
{
    u16 nameText;
    if (isJapan() != 0)
        nameText = getCarRecord()->name2R0;
    else if (isUK() != 0)
        nameText = getCarRecord()->name2R2;
    else
        nameText = getCarRecord()->name2;
    return getMultiByteString(nameText);  /* implicit: getString */
}

/* The car record's shortName, a string of `strings': the car's short code
 * name. */
// FUNC 0x002344A0 CarEquipments::getShortName
const char *CarEquipments::getShortName()
{
    return strings->getString(getCarRecord()->shortName);
}

/* The car record's makerName, a string of `uniStrings': the maker's name as
 * shown (the garage's maker column). */
// FUNC 0x002344D0 CarEquipments::getMakerDisplayName
const char *CarEquipments::getMakerDisplayName()
{
    return getMultiByteString(getCarRecord()->makerName);  /* implicit: getString */
}

/* LAYOUT_FR, FF, 4WD, MR, RR. */
// FUNC 0x00234500 CarEquipments::getDriveTrain
u8 CarEquipments::getDriveTrain()
{
    return ((DrivetrainRecord *)getRecord(CARDB_DRIVETRAIN, drivetrain))->layout;
}

/* (no caller) */
// FUNC 0x00234528 CarEquipments::getAwdType
u8 CarEquipments::getAwdType()
{
    return ((DrivetrainRecord *)getRecord(CARDB_DRIVETRAIN, drivetrain))->awdType;
}

/* Forward gears of the transmission (no caller). */
// FUNC 0x00234550 CarEquipments::getGearCount
u8 CarEquipments::getGearCount()
{
    return ((TransmissionRecord *)getRecord(CARDB_GEAR, gear))->gears;
}

// FUNC 0x00234578 CarEquipments::isRacingModified
int CarEquipments::isRacingModified()
{
    return ((RacingModifyRecord *)getRecord(CARDB_RACING_MODIFY, racingModify))->kind != 0;
}

/* 1 if both tyres are dirt tyres.  No NULL checks.  `others++' and the
 * `if (others != 0) return 0;' end give retail's movn / movz with the 1 in
 * $s1 (`others = 1' and `return others == 0' give store-flags). */
// FUNC 0x002345A8 CarEquipments::hasDirtTires
int CarEquipments::hasDirtTires()
{
    int others = 0;
    if (getFronttireRecord()->kind != TIRE_KIND_DIRT)
        others++;
    if (getReartireRecord()->kind != TIRE_KIND_DIRT)
        others++;
    if (others != 0)
        return 0;
    return 1;
}

/* ------------------------------------------------------------------------ */
/* Part ids                                                                  */
/* ------------------------------------------------------------------------ */

/* The category (the record table) of part id `part', -1 if none. */
// FUNC 0x00234608 CarEquipments::getPartCategory
int CarEquipments::getPartCategory(int part)
{
    int category = -1;
    switch (part) {
    case PART_ASCC_NONE: case PART_ASCC:
        category = CARDB_ASCC; break;
    case PART_BRAKE: case PART_BRAKE_RACING:
        category = CARDB_BRAKE; break;
    case PART_BRAKE_CONTROLLER_NONE: case PART_BRAKE_CONTROLLER:
        category = CARDB_BRAKE_CONTROLLER; break;
    case PART_CLUTCH ... PART_CLUTCH_TRIPLE:
        category = CARDB_CLUTCH; break;
    case PART_COMPUTER: case PART_COMPUTER_ROM:
        category = CARDB_COMPUTER; break;
    case PART_DISPLACEMENT_NONE ... PART_DISPLACEMENT_2:
        category = CARDB_DISPLACEMENT; break;
    case PART_ENGINE_BALANCE_NONE: case PART_ENGINE_BALANCE:
        category = CARDB_ENGINE_BALANCE; break;
    case PART_FLYWHEEL ... PART_FLYWHEEL_RACING:
        category = CARDB_FLYWHEEL; break;
    case PART_GEAR ... PART_GEAR_FULL_CUSTOM:
        category = CARDB_GEAR; break;
    case PART_INTERCOOLER ... PART_INTERCOOLER_RACING:
        category = CARDB_INTERCOOLER; break;
    case PART_LIGHTWEIGHT_NONE ... PART_LIGHTWEIGHT_STAGE3:
        category = CARDB_LIGHTWEIGHT; break;
    case PART_LSD ... PART_LSD_AYC:
        category = CARDB_LSD; break;
    case PART_MUFFLER ... PART_MUFFLER_RACING:
        category = CARDB_MUFFLER; break;
    case PART_NA_TUNE_NONE ... PART_NA_TUNE_STAGE3:
        category = CARDB_NA_TUNE; break;
    case PART_PORT_POLISH_NONE: case PART_PORT_POLISH:
        category = CARDB_PORT_POLISH; break;
    case PART_PROPELLER_SHAFT: case PART_PROPELLER_SHAFT_CARBON:
        category = CARDB_PROPELLER_SHAFT; break;
    case PART_RACING_MODIFY_NONE: case PART_RACING_MODIFY:
        category = CARDB_RACING_MODIFY; break;
    case PART_SUSPENSION ... PART_SUSPENSION_FULL_CUSTOM:
        category = CARDB_SUSPENSION; break;
    case PART_TCSC_NONE: case PART_TCSC:
        category = CARDB_TCSC; break;
    case PART_FRONT_TIRE ... PART_FRONT_TIRE_DIRT:
        category = CARDB_FRONT_TIRE; break;
    case PART_REAR_TIRE ... PART_REAR_TIRE_DIRT:
        category = CARDB_REAR_TIRE; break;
    case PART_TURBINE ... PART_TURBINE_STAGE4:
        category = CARDB_TURBINE; break;
    case PART_DRIVETRAIN: case PART_DRIVETRAIN_VCD:
        category = CARDB_DRIVETRAIN; break;
    case PART_COUNT:
        break;      /* an explicit case: the jump table runs to 0x57 */
    }
    return category;
}

/* The kind (the record's grade) of part id `part' within its category, -1
 * if none: the inverse of the kind -> id tables. */
// FUNC 0x002346F0 CarEquipments::getPartKind
int CarEquipments::getPartKind(int part)
{
    int kind = -1;
    switch (part) {
    case PART_BRAKE: case PART_BRAKE_CONTROLLER_NONE: case PART_CLUTCH:
    case PART_COMPUTER: case PART_DISPLACEMENT_NONE: case PART_ENGINE_BALANCE_NONE:
    case PART_FLYWHEEL: case PART_GEAR: case PART_INTERCOOLER:
    case PART_LIGHTWEIGHT_NONE: case PART_LSD: case PART_MUFFLER:
    case PART_NA_TUNE_NONE: case PART_PORT_POLISH_NONE: case PART_PROPELLER_SHAFT:
    case PART_RACING_MODIFY_NONE: case PART_SUSPENSION: case PART_TCSC_NONE:
    case PART_FRONT_TIRE: case PART_REAR_TIRE: case PART_TURBINE:
    case PART_ASCC_NONE: case PART_DRIVETRAIN:
        kind = 0; break;
    case PART_LSD_AYC: case PART_FRONT_TIRE + 5: case PART_REAR_TIRE + 5:
        kind = 5; break;
    case PART_FRONT_TIRE + 6: case PART_REAR_TIRE + 6:
        kind = 6; break;
    case PART_FRONT_TIRE + 7: case PART_REAR_TIRE + 7:
        kind = 7; break;
    case PART_FRONT_TIRE + 8: case PART_REAR_TIRE + 8:
        kind = 8; break;
    case PART_FRONT_TIRE + 9: case PART_REAR_TIRE + 9:
        kind = 9; break;
    case PART_FRONT_TIRE_DIRT: case PART_REAR_TIRE_DIRT:
        kind = TIRE_KIND_DIRT; break;
    case PART_CLUTCH_TWIN: case PART_DISPLACEMENT_2: case PART_FLYWHEEL_SEMI_RACING:
    case PART_GEAR_SUPER_CLOSE: case PART_INTERCOOLER_RACING: case PART_LIGHTWEIGHT_STAGE2:
    case PART_LSD_2WAY: case PART_MUFFLER_SEMI_RACING: case PART_NA_TUNE_STAGE2:
    case PART_SUSPENSION_SEMI_RACING: case PART_FRONT_TIRE + 2: case PART_REAR_TIRE + 2:
    case PART_TURBINE_STAGE2:
        kind = 2; break;
    case PART_CLUTCH_TRIPLE: case PART_FLYWHEEL_RACING: case PART_GEAR_FULL_CUSTOM:
    case PART_LIGHTWEIGHT_STAGE3: case PART_LSD_1_5WAY: case PART_MUFFLER_RACING:
    case PART_NA_TUNE_STAGE3: case PART_SUSPENSION_FULL_CUSTOM: case PART_FRONT_TIRE + 3:
    case PART_REAR_TIRE + 3: case PART_TURBINE_STAGE3:
        kind = 3; break;
    case PART_LSD_FULL_CUSTOM: case PART_FRONT_TIRE + 4: case PART_REAR_TIRE + 4:
    case PART_TURBINE_STAGE4:
        kind = 4; break;
    case PART_BRAKE_RACING: case PART_BRAKE_CONTROLLER: case PART_CLUTCH_SPORTS:
    case PART_COMPUTER_ROM: case PART_DISPLACEMENT_UP: case PART_ENGINE_BALANCE:
    case PART_FLYWHEEL_SPORTS: case PART_GEAR_CLOSE: case PART_INTERCOOLER_SPORTS:
    case PART_LIGHTWEIGHT_STAGE1: case PART_LSD_1WAY: case PART_MUFFLER_SPORTS:
    case PART_NA_TUNE_STAGE1: case PART_PORT_POLISH: case PART_PROPELLER_SHAFT_CARBON:
    case PART_RACING_MODIFY: case PART_SUSPENSION_SPORTS: case PART_TCSC:
    case PART_FRONT_TIRE + 1: case PART_REAR_TIRE + 1: case PART_TURBINE_STAGE1:
    case PART_ASCC: case PART_DRIVETRAIN_VCD:
        kind = 1; break;
    case PART_COUNT:
        break;      /* an explicit case: the jump table runs to 0x57 */
    }
    return kind;
}

/* The code of this car's version of part id `part', 0 if the car has none. */
// FUNC 0x00234778 CarEquipments::findPartCode
u64 CarEquipments::findPartCode(int part)
{
    int category = getPartCategory(part);
    return findPart(category, getPartKind(part));
}

/* The code of the car's stock part of `category' (from its record, no NULL
 * check), 0 for the categories it has no field for (steer, chassis, engine,
 * drive train, the tyre size / force / compound tables). */
// FUNC 0x002347D0 CarEquipments::getDefaultPartCode
u64 CarEquipments::getDefaultPartCode(int category)
{
    CarRecord *car = (CarRecord *)getRecord(CARDB_CAR, carCode);
    u64 code = 0;
    switch (category) {
    case CARDB_ASCC:            code = car->parts.ascc; break;
    case CARDB_BRAKE:           code = car->parts.brake; break;
    case CARDB_BRAKE_CONTROLLER: code = car->parts.brakeController; break;
    case CARDB_CLUTCH:          code = car->parts.clutch; break;
    case CARDB_COMPUTER:        code = car->parts.computer; break;
    case CARDB_DISPLACEMENT:    code = car->parts.displacement; break;
    case CARDB_ENGINE_BALANCE:  code = car->parts.engineBalance; break;
    case CARDB_FLYWHEEL:        code = car->parts.flywheel; break;
    case CARDB_WHEEL:           code = car->parts.wheel; break;
    case CARDB_GEAR:            code = car->parts.gear; break;
    case CARDB_INTERCOOLER:     code = car->parts.intercooler; break;
    case CARDB_LIGHTWEIGHT:     code = car->parts.lightweight; break;
    case CARDB_LSD:             code = car->parts.lsd; break;
    case CARDB_MUFFLER:         code = car->parts.muffler; break;
    case CARDB_NA_TUNE:         code = car->parts.naTune; break;
    case CARDB_PORT_POLISH:     code = car->parts.portPolish; break;
    case CARDB_PROPELLER_SHAFT: code = car->parts.propellerShaft; break;
    case CARDB_RACING_MODIFY:   code = car->parts.racingModify; break;
    case CARDB_SUSPENSION:      code = car->parts.suspension; break;
    case CARDB_TCSC:            code = car->parts.tcsc; break;
    case CARDB_FRONT_TIRE:      code = car->parts.frontTire; break;
    case CARDB_REAR_TIRE:       code = car->parts.rearTire; break;
    case CARDB_TURBINE:         code = car->parts.turbine; break;
    }
    return code;
}

/* ------------------------------------------------------------------------ */
/* The records of the fitted parts: NULL without the part                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x002348E8 CarEquipments::getEngineRecord
EngineRecord *CarEquipments::getEngineRecord()
{
    u64 code = engine;
    if (code == 0)
        return 0;
    return (EngineRecord *)getRecord(CARDB_ENGINE, code);
}

// FUNC 0x00234918 CarEquipments::getSuspensionRecord
SuspensionRecord *CarEquipments::getSuspensionRecord()
{
    u64 code = suspension;
    if (code == 0)
        return 0;
    return (SuspensionRecord *)getRecord(CARDB_SUSPENSION, code);
}

// FUNC 0x00234948 CarEquipments::getBrakecontrollerRecord
BrakeControllerRecord *CarEquipments::getBrakecontrollerRecord()
{
    u64 code = brakeController;
    if (code == 0)
        return 0;
    return (BrakeControllerRecord *)getRecord(CARDB_BRAKE_CONTROLLER, code);
}

// FUNC 0x00234978 CarEquipments::getGearRecord
TransmissionRecord *CarEquipments::getGearRecord()
{
    u64 code = gear;
    if (code == 0)
        return 0;
    return (TransmissionRecord *)getRecord(CARDB_GEAR, code);
}

// FUNC 0x002349A8 CarEquipments::getDrivetrainRecord
DrivetrainRecord *CarEquipments::getDrivetrainRecord()
{
    u64 code = drivetrain;
    if (code == 0)
        return 0;
    return (DrivetrainRecord *)getRecord(CARDB_DRIVETRAIN, code);
}

// FUNC 0x002349D8 CarEquipments::getRacingmodifyRecord
RacingModifyRecord *CarEquipments::getRacingmodifyRecord()
{
    u64 code = racingModify;
    if (code == 0)
        return 0;
    return (RacingModifyRecord *)getRecord(CARDB_RACING_MODIFY, code);
}

/* (no NULL check: every car has a chassis) */
// FUNC 0x00234A08 CarEquipments::getChassisRecord
ChassisRecord *CarEquipments::getChassisRecord()
{
    return (ChassisRecord *)getRecord(CARDB_CHASSIS, chassis);
}

// FUNC 0x00234A30 CarEquipments::getLSDRecord
LsdRecord *CarEquipments::getLSDRecord()
{
    u64 code = lsd;
    if (code == 0)
        return 0;
    return (LsdRecord *)getRecord(CARDB_LSD, code);
}

// FUNC 0x00234A60 CarEquipments::getASCCRecord
AsccRecord *CarEquipments::getASCCRecord()
{
    u64 code = ascc;
    if (code == 0)
        return 0;
    return (AsccRecord *)getRecord(CARDB_ASCC, code);
}

// FUNC 0x00234A90 CarEquipments::getTCSCRecord
TcsRecord *CarEquipments::getTCSCRecord()
{
    u64 code = tcsc;
    if (code == 0)
        return 0;
    return (TcsRecord *)getRecord(CARDB_TCSC, code);
}

// FUNC 0x00234AC0 CarEquipments::getBrakeRecord
BrakeRecord *CarEquipments::getBrakeRecord()
{
    u64 code = brake;
    if (code == 0)
        return 0;
    return (BrakeRecord *)getRecord(CARDB_BRAKE, code);
}

// FUNC 0x00234AF0 CarEquipments::getLightweightRecord
LightweightRecord *CarEquipments::getLightweightRecord()
{
    u64 code = lightweight;
    if (code == 0)
        return 0;
    return (LightweightRecord *)getRecord(CARDB_LIGHTWEIGHT, code);
}

// FUNC 0x00234B20 CarEquipments::getPortpolishRecord
PowerRecord *CarEquipments::getPortpolishRecord()
{
    u64 code = portPolish;
    if (code == 0)
        return 0;
    return (PowerRecord *)getRecord(CARDB_PORT_POLISH, code);
}

// FUNC 0x00234B50 CarEquipments::getEnginebalanceRecord
EngineTuneRecord *CarEquipments::getEnginebalanceRecord()
{
    u64 code = engineBalance;
    if (code == 0)
        return 0;
    return (EngineTuneRecord *)getRecord(CARDB_ENGINE_BALANCE, code);
}

// FUNC 0x00234B80 CarEquipments::getDisplacementRecord
PowerRecord *CarEquipments::getDisplacementRecord()
{
    u64 code = displacement;
    if (code == 0)
        return 0;
    return (PowerRecord *)getRecord(CARDB_DISPLACEMENT, code);
}

// FUNC 0x00234BB0 CarEquipments::getComputerRecord
PowerRecord *CarEquipments::getComputerRecord()
{
    u64 code = computer;
    if (code == 0)
        return 0;
    return (PowerRecord *)getRecord(CARDB_COMPUTER, code);
}

// FUNC 0x00234BE0 CarEquipments::getNATuneRecord
EngineTuneRecord *CarEquipments::getNATuneRecord()
{
    u64 code = naTune;
    if (code == 0)
        return 0;
    return (EngineTuneRecord *)getRecord(CARDB_NA_TUNE, code);
}

// FUNC 0x00234C10 CarEquipments::getTurbineRecord
TurbineKitRecord *CarEquipments::getTurbineRecord()
{
    u64 code = turbine;
    if (code == 0)
        return 0;
    return (TurbineKitRecord *)getRecord(CARDB_TURBINE, code);
}

// FUNC 0x00234C40 CarEquipments::getFlywheelRecord
InertiaRecord *CarEquipments::getFlywheelRecord()
{
    u64 code = flywheel;
    if (code == 0)
        return 0;
    return (InertiaRecord *)getRecord(CARDB_FLYWHEEL, code);
}

// FUNC 0x00234C70 CarEquipments::getClutchRecord
InertiaRecord *CarEquipments::getClutchRecord()
{
    u64 code = clutch;
    if (code == 0)
        return 0;
    return (InertiaRecord *)getRecord(CARDB_CLUTCH, code);
}

// FUNC 0x00234CA0 CarEquipments::getPropellershaftRecord
PropellerShaftRecord *CarEquipments::getPropellershaftRecord()
{
    u64 code = propellerShaft;
    if (code == 0)
        return 0;
    return (PropellerShaftRecord *)getRecord(CARDB_PROPELLER_SHAFT, code);
}

// FUNC 0x00234CD0 CarEquipments::getIntercoolerRecord
PowerRecord *CarEquipments::getIntercoolerRecord()
{
    u64 code = intercooler;
    if (code == 0)
        return 0;
    return (PowerRecord *)getRecord(CARDB_INTERCOOLER, code);
}

// FUNC 0x00234D00 CarEquipments::getMufflerRecord
PowerRecord *CarEquipments::getMufflerRecord()
{
    u64 code = muffler;
    if (code == 0)
        return 0;
    return (PowerRecord *)getRecord(CARDB_MUFFLER, code);
}

// FUNC 0x00234D30 CarEquipments::getWheelRecord
WheelRecord *CarEquipments::getWheelRecord()
{
    u64 code = wheel;
    if (code == 0)
        return 0;
    return (WheelRecord *)getRecord(CARDB_WHEEL, code);
}

// FUNC 0x00234D60 CarEquipments::getFronttireRecord
TiresRecord *CarEquipments::getFronttireRecord()
{
    u64 code = frontTire;
    if (code == 0)
        return 0;
    return (TiresRecord *)getRecord(CARDB_FRONT_TIRE, code);
}

// FUNC 0x00234D90 CarEquipments::getReartireRecord
TiresRecord *CarEquipments::getReartireRecord()
{
    u64 code = rearTire;
    if (code == 0)
        return 0;
    return (TiresRecord *)getRecord(CARDB_REAR_TIRE, code);
}

/* The tyre kinds (TIRE_KIND_*).  No NULL checks. */
// FUNC 0x00234DC0 CarEquipments::getFronttireKind
u8 CarEquipments::getFronttireKind()
{
    return getFronttireRecord()->kind;
}

// FUNC 0x00234DE0 CarEquipments::getReartireKind
u8 CarEquipments::getReartireKind()
{
    return getReartireRecord()->kind;
}

/* ------------------------------------------------------------------------ */
/* Part ids of the fitted parts (the kind -> id tables; without the part,   */
/* the category's base part, kind 0)                                         */
/* ------------------------------------------------------------------------ */

/* The part tables (u106's data pass; values from retail's .data / .sdata,
 * none changed).  Initialised and not const, so egcs writes each where it
 * is defined: the .data ones (> 8 bytes) before the frame info, the .sdata
 * ones (-G8) before the spec sheet's literals ("FR" ..).  The categories
 * in part-id order, except the drive train table: retail's .sdata has it
 * after the brake controller's. */

/* part id -> index of g_002F1048, the short name (getPartName) */
u16 g_002EC3C0[PART_COUNT] = {   /* 0x002EC3C0 */
    78,  1, 79, 73, 78,  4,  5,  6,  8,  9, 76, 74, 74, 76, 73, 78,
    13, 14, 15, 78, 17, 18, 19, 78, 21, 22, 76, 69, 70, 71, 78, 28,
    29, 30, 31, 32, 78, 34, 35, 36, 79, 69, 70, 71, 76, 74, 78, 43,
    76, 74, 78, 46, 47, 48, 75, 73, 51, 52, 53, 54, 55, 56, 57, 58,
    59, 60, 61, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 78, 69,
    70, 71, 72, 79, 73, 78, 77,
};

/* part id -> index of g_002F1048, the full name (getPartFullName) */
u16 g_002EC470[PART_COUNT] = {   /* 0x002EC470 */
    78,  1, 79,  2, 78,  4,  5,  6, 79,  7, 76, 10, 79, 79, 11, 78,
    13, 14, 15, 78, 17, 18, 19, 78, 21, 22, 76, 24, 25, 26, 78, 28,
    29, 30, 31, 32, 78, 34, 35, 36, 79, 38, 39, 40, 76, 41, 78, 42,
    76, 74, 78, 46, 47, 48, 75, 49, 51, 52, 53, 54, 55, 56, 57, 58,
    59, 60, 61, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 79, 64,
    65, 66, 67, 79, 68, 75, 62,
};

int g_00352CF8[2] = { PART_BRAKE, PART_BRAKE_RACING };   /* 0x00352CF8 */
int g_00352D00[2] = { PART_BRAKE_CONTROLLER_NONE, PART_BRAKE_CONTROLLER };   /* 0x00352D00 */
int g_00352D08[2] = { PART_DRIVETRAIN, PART_DRIVETRAIN_VCD };   /* 0x00352D08 */
int g_002EC520[4] = {   /* 0x002EC520 */
    PART_CLUTCH, PART_CLUTCH_SPORTS, PART_CLUTCH_TWIN, PART_CLUTCH_TRIPLE,
};
int g_00352D10[2] = { PART_COMPUTER, PART_COMPUTER_ROM };   /* 0x00352D10 */
int g_002EC530[3] = {   /* 0x002EC530 */
    PART_DISPLACEMENT_NONE, PART_DISPLACEMENT_UP, PART_DISPLACEMENT_2,
};
int g_00352D18[2] = { PART_ENGINE_BALANCE_NONE, PART_ENGINE_BALANCE };   /* 0x00352D18 */
int g_002EC540[4] = {   /* 0x002EC540 */
    PART_FLYWHEEL, PART_FLYWHEEL_SPORTS, PART_FLYWHEEL_SEMI_RACING, PART_FLYWHEEL_RACING,
};
int g_002EC550[4] = {   /* 0x002EC550 */
    PART_GEAR, PART_GEAR_CLOSE, PART_GEAR_SUPER_CLOSE, PART_GEAR_FULL_CUSTOM,
};
int g_002EC560[3] = {   /* 0x002EC560 */
    PART_INTERCOOLER, PART_INTERCOOLER_SPORTS, PART_INTERCOOLER_RACING,
};
int g_002EC570[4] = {   /* 0x002EC570 */
    PART_LIGHTWEIGHT_NONE, PART_LIGHTWEIGHT_STAGE1, PART_LIGHTWEIGHT_STAGE2,
    PART_LIGHTWEIGHT_STAGE3,
};
int g_002EC580[6] = {   /* 0x002EC580 */
    PART_LSD, PART_LSD_1WAY, PART_LSD_2WAY, PART_LSD_1_5WAY, PART_LSD_FULL_CUSTOM,
    PART_LSD_AYC,
};
int g_002EC598[4] = {   /* 0x002EC598 */
    PART_MUFFLER, PART_MUFFLER_SPORTS, PART_MUFFLER_SEMI_RACING, PART_MUFFLER_RACING,
};
int g_002EC5A8[4] = {   /* 0x002EC5A8 */
    PART_NA_TUNE_NONE, PART_NA_TUNE_STAGE1, PART_NA_TUNE_STAGE2, PART_NA_TUNE_STAGE3,
};
int g_00352D20[2] = { PART_PORT_POLISH_NONE, PART_PORT_POLISH };   /* 0x00352D20 */
int g_00352D28[2] = { PART_PROPELLER_SHAFT, PART_PROPELLER_SHAFT_CARBON };   /* 0x00352D28 */
int g_00352D30[2] = { PART_RACING_MODIFY_NONE, PART_RACING_MODIFY };   /* 0x00352D30 */
int g_002EC5B8[4] = {   /* 0x002EC5B8 */
    PART_SUSPENSION, PART_SUSPENSION_SPORTS, PART_SUSPENSION_SEMI_RACING,
    PART_SUSPENSION_FULL_CUSTOM,
};
int g_00352D38[2] = { PART_TCSC_NONE, PART_TCSC };   /* 0x00352D38 */
int g_002EC5C8[25] = {   /* 0x002EC5C8 */
    PART_FRONT_TIRE + 0, PART_FRONT_TIRE + 1, PART_FRONT_TIRE + 2, PART_FRONT_TIRE + 3,
    PART_FRONT_TIRE + 4, PART_FRONT_TIRE + 5, PART_FRONT_TIRE + 6, PART_FRONT_TIRE + 7,
    PART_FRONT_TIRE + 8, PART_FRONT_TIRE + 9, PART_NONE, PART_NONE, PART_FRONT_TIRE_DIRT,
    PART_FRONT_TIRE + 0, PART_FRONT_TIRE + 1, PART_FRONT_TIRE + 2, PART_FRONT_TIRE + 3,
    PART_FRONT_TIRE + 4, PART_FRONT_TIRE + 5, PART_FRONT_TIRE + 6, PART_FRONT_TIRE + 7,
    PART_FRONT_TIRE + 8, PART_FRONT_TIRE + 9, PART_NONE, PART_NONE,
};
int g_002EC630[25] = {   /* 0x002EC630 */
    PART_REAR_TIRE + 0, PART_REAR_TIRE + 1, PART_REAR_TIRE + 2, PART_REAR_TIRE + 3,
    PART_REAR_TIRE + 4, PART_REAR_TIRE + 5, PART_REAR_TIRE + 6, PART_REAR_TIRE + 7,
    PART_REAR_TIRE + 8, PART_REAR_TIRE + 9, PART_NONE, PART_NONE, PART_REAR_TIRE_DIRT,
    PART_REAR_TIRE + 0, PART_REAR_TIRE + 1, PART_REAR_TIRE + 2, PART_REAR_TIRE + 3,
    PART_REAR_TIRE + 4, PART_REAR_TIRE + 5, PART_REAR_TIRE + 6, PART_REAR_TIRE + 7,
    PART_REAR_TIRE + 8, PART_REAR_TIRE + 9, PART_NONE, PART_NONE,
};
int g_002EC698[5] = {   /* 0x002EC698 */
    PART_TURBINE, PART_TURBINE_STAGE1, PART_TURBINE_STAGE2, PART_TURBINE_STAGE3,
    PART_TURBINE_STAGE4,
};
int g_00352D40[2] = { PART_ASCC_NONE, PART_ASCC };   /* 0x00352D40 */

/* The NULL test first, its return on the fall-through path (retail's
 * `bnel; b' shape; the table lookup first gives `beql' to the end). */
// FUNC 0x00234E00 CarEquipments::getBrakePart
int CarEquipments::getBrakePart()
{
    BrakeRecord *rec = getBrakeRecord();
    if (rec == 0)
        return PART_BRAKE;
    return g_00352CF8[rec->kind];
}

// FUNC 0x00234E40 CarEquipments::getBrakecontrollerPart
int CarEquipments::getBrakecontrollerPart()
{
    BrakeControllerRecord *rec = getBrakecontrollerRecord();
    if (rec == 0)
        return PART_BRAKE_CONTROLLER_NONE;
    return g_00352D00[rec->kind];
}

// FUNC 0x00234E80 CarEquipments::getDrivetrainPart
int CarEquipments::getDrivetrainPart()
{
    DrivetrainRecord *rec = getDrivetrainRecord();
    if (rec == 0)
        return PART_DRIVETRAIN;
    return g_00352D08[rec->kind];
}

// FUNC 0x00234EC0 CarEquipments::getGearPart
int CarEquipments::getGearPart()
{
    TransmissionRecord *rec = getGearRecord();
    if (rec == 0)
        return PART_GEAR;
    return g_002EC550[rec->kind];
}

// FUNC 0x00234F00 CarEquipments::getSuspensionPart
int CarEquipments::getSuspensionPart()
{
    SuspensionRecord *rec = getSuspensionRecord();
    if (rec == 0)
        return PART_SUSPENSION;
    return g_002EC5B8[rec->kind];
}

/* (kinds 13..24, the second set: the table's own entries 13..24) */
// FUNC 0x00234F40 CarEquipments::getFronttirePart
int CarEquipments::getFronttirePart()
{
    TiresRecord *rec = getFronttireRecord();
    if (rec == 0)
        return PART_FRONT_TIRE;
    return g_002EC5C8[rec->kind];
}

// FUNC 0x00234F80 CarEquipments::getReartirePart
int CarEquipments::getReartirePart()
{
    TiresRecord *rec = getReartireRecord();
    if (rec == 0)
        return PART_REAR_TIRE;
    return g_002EC630[rec->kind];
}

// FUNC 0x00234FC0 CarEquipments::getLSDPart
int CarEquipments::getLSDPart()
{
    LsdRecord *rec = getLSDRecord();
    if (rec == 0)
        return PART_LSD;
    return g_002EC580[rec->kind];
}

/* (PART_NONE, not the base part, without a racing modification record) */
// FUNC 0x00235000 CarEquipments::getRacingmodifyPart
int CarEquipments::getRacingmodifyPart()
{
    RacingModifyRecord *rec = getRacingmodifyRecord();
    if (rec == 0)
        return PART_NONE;
    return g_00352D30[rec->kind];
}

// FUNC 0x00235040 CarEquipments::getASCCPart
int CarEquipments::getASCCPart()
{
    AsccRecord *rec = getASCCRecord();
    if (rec == 0)
        return PART_ASCC_NONE;
    return g_00352D40[rec->kind];
}

// FUNC 0x00235080 CarEquipments::getTCSCPart
int CarEquipments::getTCSCPart()
{
    TcsRecord *rec = getTCSCRecord();
    if (rec == 0)
        return PART_TCSC_NONE;
    return g_00352D38[rec->kind];
}

// FUNC 0x002350C0 CarEquipments::getLightweightPart
int CarEquipments::getLightweightPart()
{
    LightweightRecord *rec = getLightweightRecord();
    if (rec == 0)
        return PART_LIGHTWEIGHT_NONE;
    return g_002EC570[rec->kind];
}

// FUNC 0x00235100 CarEquipments::getPortpolishPart
int CarEquipments::getPortpolishPart()
{
    PowerRecord *rec = getPortpolishRecord();
    if (rec == 0)
        return PART_PORT_POLISH_NONE;
    return g_00352D20[rec->kind];
}

// FUNC 0x00235140 CarEquipments::getEnginebalancePart
int CarEquipments::getEnginebalancePart()
{
    EngineTuneRecord *rec = getEnginebalanceRecord();
    if (rec == 0)
        return PART_ENGINE_BALANCE_NONE;
    return g_00352D18[rec->kind];
}

// FUNC 0x00235180 CarEquipments::getDisplacementPart
int CarEquipments::getDisplacementPart()
{
    PowerRecord *rec = getDisplacementRecord();
    if (rec == 0)
        return PART_DISPLACEMENT_NONE;
    return g_002EC530[rec->kind];
}

// FUNC 0x002351C0 CarEquipments::getComputerPart
int CarEquipments::getComputerPart()
{
    PowerRecord *rec = getComputerRecord();
    if (rec == 0)
        return PART_COMPUTER;
    return g_00352D10[rec->kind];
}

// FUNC 0x00235200 CarEquipments::getNATunePart
int CarEquipments::getNATunePart()
{
    EngineTuneRecord *rec = getNATuneRecord();
    if (rec == 0)
        return PART_NA_TUNE_NONE;
    return g_002EC5A8[rec->kind];
}

// FUNC 0x00235240 CarEquipments::getTurbinePart
int CarEquipments::getTurbinePart()
{
    TurbineKitRecord *rec = getTurbineRecord();
    if (rec == 0)
        return PART_TURBINE;
    return g_002EC698[rec->kind];
}

// FUNC 0x00235280 CarEquipments::getFlywheelPart
int CarEquipments::getFlywheelPart()
{
    InertiaRecord *rec = getFlywheelRecord();
    if (rec == 0)
        return PART_FLYWHEEL;
    return g_002EC540[rec->kind];
}

// FUNC 0x002352C0 CarEquipments::getClutchPart
int CarEquipments::getClutchPart()
{
    InertiaRecord *rec = getClutchRecord();
    if (rec == 0)
        return PART_CLUTCH;
    return g_002EC520[rec->kind];
}

/* PART_NONE for the FF, MR and RR layouts (no propeller shaft). */
// FUNC 0x00235300 CarEquipments::getPropellershaftPart
int CarEquipments::getPropellershaftPart()
{
    switch (getDriveTrain()) {
    case LAYOUT_FF: case LAYOUT_MR: case LAYOUT_RR:
        return PART_NONE;
    }
    PropellerShaftRecord *rec = getPropellershaftRecord();
    if (rec == 0)
        return PART_PROPELLER_SHAFT;
    return g_00352D28[rec->kind];
}

/* PART_NONE unless the car is a turbo in stock form. */
// FUNC 0x00235378 CarEquipments::getIntercoolerPart
int CarEquipments::getIntercoolerPart()
{
    if (isTurboByDefault() == 0)
        return PART_NONE;
    PowerRecord *rec = getIntercoolerRecord();
    if (rec == 0)
        return PART_INTERCOOLER;
    return g_002EC560[rec->kind];
}

// FUNC 0x002353D0 CarEquipments::getMufflerPart
int CarEquipments::getMufflerPart()
{
    PowerRecord *rec = getMufflerRecord();
    if (rec == 0)
        return PART_MUFFLER;
    return g_002EC598[rec->kind];
}

/* ------------------------------------------------------------------------ */
/* The spec sheet                                                            */
/* ------------------------------------------------------------------------ */

// FUNC 0x00235410 CarEquipments::getDriveTrainText
const char *CarEquipments::getDriveTrainText()
{
    switch (getDriveTrain()) {
    case LAYOUT_FR:  return "FR";
    case LAYOUT_FF:  return "FF";
    case LAYOUT_4WD: return "4WD";
    case LAYOUT_MR:  return "MR";
    case LAYOUT_RR:  return "RR";
    }
    return 0;
}

// FUNC 0x00235498 CarEquipments::getWheelBase
u16 CarEquipments::getWheelBase()
{
    return getChassisRecord()->wheelBase;
}

/* The engine getters below do not check for a NULL engine record.  Power
 * and torque in the user's units (g_002F59D8). */
// FUNC 0x002354B8 CarEquipments::getMaxPower
int CarEquipments::getMaxPower()
{
    EngineRecord *rec = getEngineRecord();
    return (int)g_002F59D8.convPower((f32)rec->power);
}

// FUNC 0x002354F8 CarEquipments::getMaxPowerRpmText
const char *CarEquipments::getMaxPowerRpmText()
{
    return getMultiByteString(getEngineRecord()->powerRpmText);  /* implicit: getString */
}

// FUNC 0x00235528 CarEquipments::getMaxTorque
int CarEquipments::getMaxTorque()
{
    EngineRecord *rec = getEngineRecord();
    return (int)g_002F59D8.convTorque((f32)rec->torque);
}

// FUNC 0x00235568 CarEquipments::getMaxTorqueRpmText
const char *CarEquipments::getMaxTorqueRpmText()
{
    return getMultiByteString(getEngineRecord()->torqueRpmText);  /* implicit: getString */
}

// FUNC 0x00235598 CarEquipments::getDisplacementText
const char *CarEquipments::getDisplacementText()
{
    return strings->getString(getEngineRecord()->displacementText);
}

// FUNC 0x002355C8 CarEquipments::getLength
u16 CarEquipments::getLength()
{
    return getChassisRecord()->length;
}

// FUNC 0x002355E8 CarEquipments::getHeight
u16 CarEquipments::getHeight()
{
    return getChassisRecord()->height;
}

/* (from the racing modification record, no NULL check) */
// FUNC 0x00235608 CarEquipments::getWidth
u16 CarEquipments::getWidth()
{
    return getRacingmodifyRecord()->width;
}

/* ------------------------------------------------------------------------ */
/* Names                                                                     */
/* ------------------------------------------------------------------------ */

/* The EUC-JP name of part id `part''s category ("no text" for the
 * categories without one, "error text" for none and the wheel / tyre
 * tables).  No caller. */
// FUNC 0x00235628 CarEquipments::getCategoryName
const char *CarEquipments::getCategoryName(int part)
{
    const char *name = "no text";
    switch (getPartCategory(part)) {
    case CARDB_BRAKE:           name = g_002F1048[0];  break;   /* brake */
    case CARDB_BRAKE_CONTROLLER: name = g_002F1048[2]; break;   /* brake balance controller */
    case CARDB_LIGHTWEIGHT:     name = g_002F1048[23]; break;   /* lightweight */
    case CARDB_RACING_MODIFY:   name = g_002F1048[44]; break;   /* racing modify */
    case CARDB_PORT_POLISH:     name = g_002F1048[41]; break;   /* port polish */
    case CARDB_ENGINE_BALANCE:  name = g_002F1048[11]; break;   /* engine balance tune */
    case CARDB_DISPLACEMENT:    name = g_002F1048[10]; break;   /* displacement up */
    case CARDB_COMPUTER:        name = g_002F1048[7];  break;   /* tuned ROM */
    case CARDB_NA_TUNE:         name = g_002F1048[37]; break;   /* NA tune */
    case CARDB_TURBINE:         name = g_002F1048[63]; break;   /* turbine kit */
    case CARDB_FLYWHEEL:        name = g_002F1048[12]; break;   /* flywheel */
    case CARDB_CLUTCH:          name = g_002F1048[3];  break;   /* clutch */
    case CARDB_PROPELLER_SHAFT: name = g_002F1048[42]; break;   /* propeller shaft */
    case CARDB_GEAR:            name = g_002F1048[16]; break;   /* transmission */
    case CARDB_SUSPENSION:      name = g_002F1048[45]; break;   /* suspension */
    case CARDB_INTERCOOLER:     name = g_002F1048[20]; break;   /* intercooler */
    case CARDB_MUFFLER:         name = g_002F1048[33]; break;   /* muffler */
    case CARDB_LSD:             name = g_002F1048[27]; break;   /* LSD */
    case CARDB_TCSC:            name = g_002F1048[49]; break;   /* TCS controller */
    case CARDB_ASCC:            name = g_002F1048[68]; break;   /* active stability management system */
    case CARDB_FRONT_TIRE:
    case CARDB_REAR_TIRE:       name = g_002F1048[50]; break;   /* tyres */
    case -1:
    case CARDB_WHEEL: case CARDB_TIRE_SIZE: case CARDB_TIRE_FORCE_VOL: case CARDB_TIRE_COMPOUND:
        name = "error text";
        break;
    }
    return name;
}

/* (static: no `this') */
// FUNC 0x00235780 CarEquipments::getPartFullName
const char *CarEquipments::getPartFullName(int part)
{
    return g_002F1048[g_002EC470[part]];
}

/* (static: no `this').  PART_NONE reads past g_002EC3C0, into
 * g_002EC470[0] (-> "ノーマル", normal). */
// FUNC 0x002357B0 CarEquipments::getPartName
const char *CarEquipments::getPartName(int part)
{
    return g_002F1048[g_002EC3C0[part]];
}

// FUNC 0x002357E0 CarEquipments::getBrakeName
const char *CarEquipments::getBrakeName()
{
    return getPartName(getBrakePart());
}

// FUNC 0x00235808 CarEquipments::getBrakecontrollerName
const char *CarEquipments::getBrakecontrollerName()
{
    return getPartName(getBrakecontrollerPart());
}

// FUNC 0x00235830 CarEquipments::getClutchName
const char *CarEquipments::getClutchName()
{
    return getPartName(getClutchPart());
}

// FUNC 0x00235858 CarEquipments::getComputerName
const char *CarEquipments::getComputerName()
{
    return getPartName(getComputerPart());
}

// FUNC 0x00235880 CarEquipments::getDisplacementName
const char *CarEquipments::getDisplacementName()
{
    return getPartName(getDisplacementPart());
}

// FUNC 0x002358A8 CarEquipments::getEnginebalanceName
const char *CarEquipments::getEnginebalanceName()
{
    return getPartName(getEnginebalancePart());
}

// FUNC 0x002358D0 CarEquipments::getFlywheelName
const char *CarEquipments::getFlywheelName()
{
    return getPartName(getFlywheelPart());
}

// FUNC 0x002358F8 CarEquipments::getGearName
const char *CarEquipments::getGearName()
{
    return getPartName(getGearPart());
}

/* "-" without an intercooler (not a turbo in stock form). */
// FUNC 0x00235920 CarEquipments::getIntercoolerName
const char *CarEquipments::getIntercoolerName()
{
    if (getIntercoolerPart() == PART_NONE)
        return g_002F1048[75];                  /* "-" (0x002F1174) */
    return getPartName(getIntercoolerPart());
}

// FUNC 0x00235968 CarEquipments::getLightweightName
const char *CarEquipments::getLightweightName()
{
    return getPartName(getLightweightPart());
}

// FUNC 0x00235990 CarEquipments::getLSDName
const char *CarEquipments::getLSDName()
{
    return getPartName(getLSDPart());
}

// FUNC 0x002359B8 CarEquipments::getMufflerName
const char *CarEquipments::getMufflerName()
{
    return getPartName(getMufflerPart());
}

// FUNC 0x002359E0 CarEquipments::getNATuneName
const char *CarEquipments::getNATuneName()
{
    return getPartName(getNATunePart());
}

// FUNC 0x00235A08 CarEquipments::getPortpolishName
const char *CarEquipments::getPortpolishName()
{
    return getPartName(getPortpolishPart());
}

/* "-" without a propeller shaft (FF, MR, RR). */
// FUNC 0x00235A30 CarEquipments::getPropellershaftName
const char *CarEquipments::getPropellershaftName()
{
    if (getPropellershaftPart() == PART_NONE)
        return g_002F1048[75];                  /* "-" (0x002F1174) */
    return getPartName(getPropellershaftPart());
}

/* No caller.  No check for PART_NONE (no racing modification record): see
 * getPartName. */
// FUNC 0x00235A78 CarEquipments::getRacingmodifyName
const char *CarEquipments::getRacingmodifyName()
{
    return getPartName(getRacingmodifyPart());
}

// FUNC 0x00235AA0 CarEquipments::getSuspensionName
const char *CarEquipments::getSuspensionName()
{
    return getPartName(getSuspensionPart());
}

// FUNC 0x00235AC8 CarEquipments::getTCSCName
const char *CarEquipments::getTCSCName()
{
    return getPartName(getTCSCPart());
}

// FUNC 0x00235AF0 CarEquipments::getFronttireName
const char *CarEquipments::getFronttireName()
{
    return getPartName(getFronttirePart());
}

// FUNC 0x00235B18 CarEquipments::getReartireName
const char *CarEquipments::getReartireName()
{
    return getPartName(getReartirePart());
}

/* "なし" (none) for a car that is not a turbo in stock form and has no
 * turbine kit; else the short name ("ノーマル" for the stock turbo). */
// FUNC 0x00235B40 CarEquipments::getTurbineName
const char *CarEquipments::getTurbineName()
{
    int part;
    if (isTurboByDefault() == 0) {
        part = getTurbinePart();
        if (part == PART_TURBINE)
            return g_002F1048[79];              /* "なし" (0x002F1184) */
    } else {
        part = getTurbinePart();
    }
    return getPartName(part);
}

// FUNC 0x00235BA0 CarEquipments::getASCCName
const char *CarEquipments::getASCCName()
{
    return getPartName(getASCCPart());
}

// FUNC 0x00235BC8 CarEquipments::getDrivetrainName
const char *CarEquipments::getDrivetrainName()
{
    return getPartName(getDrivetrainPart());
}

/* ------------------------------------------------------------------------ */
/* Fitting parts                                                             */
/* ------------------------------------------------------------------------ */

/* 1 for the categories brake controller, steer, turbine, TCSC and ASCC: ?
 * the add-on parts (kind 0 of each with a part id is "none" / "-": 0x02,
 * 0x4E for a car that is not a turbo, 0x36, 0x53).  No caller. */
// FUNC 0x00235BF0 F_00235BF0
int CarEquipments::F_00235BF0(int category)
{
    switch (category) {
    case CARDB_BRAKE_CONTROLLER: case CARDB_STEER: case CARDB_TURBINE:
    case CARDB_TCSC: case CARDB_ASCC:
        return 1;
    }
    return 0;
}

/* 1 for the categories lightweight, port polish, engine balance,
 * displacement, computer, NA tune, flywheel, propeller shaft, intercooler,
 * muffler and wheel: ? the categories whose stock part a car may lack
 * (getPartCodeToFit asks it for a kind 0 part without a code). */
// FUNC 0x00235C28 F_00235C28
int CarEquipments::F_00235C28(int category)
{
    switch (category) {
    case CARDB_LIGHTWEIGHT: case CARDB_PORT_POLISH: case CARDB_ENGINE_BALANCE:
    case CARDB_DISPLACEMENT: case CARDB_COMPUTER: case CARDB_NA_TUNE:
    case CARDB_FLYWHEEL: case CARDB_PROPELLER_SHAFT: case CARDB_INTERCOOLER:
    case CARDB_MUFFLER: case CARDB_WHEEL:
        return 1;
    }
    return 0;
}

/* The code to fit for part id `part': this car's version of it, else the
 * car's stock part of the category. */
// FUNC 0x00235C60 CarEquipments::getPartCodeToFit
u64 CarEquipments::getPartCodeToFit(int part)
{
    u64 code = findPartCode(part);
    int kind = getPartKind(part);
    int category = getPartCategory(part);
    if (code == 0) {
        code = getDefaultPartCode(category);
        if (code == 0 && kind == 0)
            F_00235C28(category);   /* result unused: the code stays 0 (? a check
                                       or a message compiled out) */
    }
    return code;
}

/* Fits part id `part' (u010: the garage), then lets the settings follow the
 * new part (the reset* defaults). */
// FUNC 0x00235CF8 CarEquipments::fitPart
void CarEquipments::fitPart(int part)
{
    int category = getPartCategory(part);
    u64 code = getPartCodeToFit(part);
    switch (category) {
    case CARDB_BRAKE:           brake = code; break;
    case CARDB_BRAKE_CONTROLLER: brakeController = code; resetBrakeController(); break;
    case CARDB_CHASSIS:         chassis = code; break;
    case CARDB_LIGHTWEIGHT:     lightweight = code; break;
    case CARDB_RACING_MODIFY:   racingModify = code; resetRacingModify(); break;
    case CARDB_ENGINE:          engine = code; break;
    case CARDB_PORT_POLISH:     portPolish = code; break;
    case CARDB_ENGINE_BALANCE:  engineBalance = code; break;
    case CARDB_DISPLACEMENT:    displacement = code; break;
    case CARDB_COMPUTER:        computer = code; break;
    case CARDB_NA_TUNE:
        /* an NA tune puts the stock turbine kit back */
        turbine = getDefaultPartCode(CARDB_TURBINE);
        resetTurbineKit();
        naTune = code;
        break;
    case CARDB_TURBINE: {
        /* a turbine kit puts the stock NA tune back */
        u64 stockNaTune = getDefaultPartCode(CARDB_NA_TUNE);
        turbine = code;
        naTune = stockNaTune;
        resetTurbineKit();
        break;
    }
    case CARDB_DRIVETRAIN:      drivetrain = code; resetDrivetrain(); break;
    case CARDB_FLYWHEEL:        flywheel = code; break;
    case CARDB_CLUTCH:          clutch = code; break;
    case CARDB_PROPELLER_SHAFT: propellerShaft = code; break;
    case CARDB_GEAR:            gear = code; resetGear(); F_002369A8(); break;
    case CARDB_SUSPENSION:      suspension = code; resetSuspension(); break;
    case CARDB_INTERCOOLER:     intercooler = code; break;
    case CARDB_MUFFLER:         muffler = code; break;
    case CARDB_LSD:             lsd = code; resetLsd(); break;
    case CARDB_TCSC:            tcsc = code; resetTcsc(); break;
    case CARDB_ASCC:            ascc = code; resetAscc(); break;
    /* steer, the wheel, the tyre size / force / compound tables: nothing */
    case CARDB_FRONT_TIRE:      frontTire = code; break;
    case CARDB_REAR_TIRE:       rearTire = code; break;
    }
}

/* The engine record's autoGearRpm in rpm (no NULL check; no caller). */
// FUNC 0x00235EC8 CarEquipments::getAutoGearRpm
int CarEquipments::getAutoGearRpm()
{
    return getEngineRecord()->autoGearRpm * 100;
}

/* The engine record's revLimit in rpm (no NULL check; no caller). */
// FUNC 0x00235EF0 CarEquipments::getRevLimitRpm
int CarEquipments::getRevLimitRpm()
{
    return getEngineRecord()->revLimit * 100;
}

/* value limited to lo..hi (signed; `this' unused).  In its own variable:
 * `if (value < lo) value = lo; return hi < value ? hi : value;' folds to
 * MAX_EXPR / MIN_EXPR, another movz / movn order. */
// FUNC 0x00235F18 CarEquipments::clamp
int CarEquipments::clamp(int lo, int value, int hi)
{
    int r = value;
    if (r < lo)
        r = lo;
    if (r > hi)
        r = hi;
    return r;
}

/* ------------------------------------------------------------------------ */
/* Colours, maker, wheel                                                     */
/* ------------------------------------------------------------------------ */

/* colorCode = code; unless -1, also the matching colour index of the
 * model's colour list.  The test reads the u32 field (retail's `lui; ori'
 * 0xFFFFFFFF; u041's pending note). */
// FUNC 0x00235F30 CarEquipments::setColorCode
void CarEquipments::setColorCode(int code)
{
    colorCode = code;
    if (colorCode != (u32)-1)
        colorIndex = g_00352D98.getColorIndex(getModelCode(), code);
}

/* colorIndex = index, colorCode its colour code; 1 if the model has it. */
// FUNC 0x00235F88 CarEquipments::setColorIndex
int CarEquipments::setColorIndex(int index)
{
    colorIndex = index;
    colorCode = g_00352D98.getColor(getModelCode(), index);
    return colorCode != (u32)-1;
}

/* The index in a variable: retail calls getMakerIndex before it takes the
 * table's address (no $s0). */
// FUNC 0x00235FD8 CarEquipments::getMakerName
const char *CarEquipments::getMakerName()
{
    int index = getMakerIndex();
    return g_002EFF20[index];
}

// FUNC 0x00236008 CarEquipments::getBrandName
const char *CarEquipments::getBrandName()
{
    int index = getBrandIndex();
    return g_002EFFD8[index];
}

// FUNC 0x00236038 CarEquipments::isDefaultWheel
int CarEquipments::isDefaultWheel()
{
    if (wheel != 0)
        return 0;
    return 1;
}

/* The fitted wheel's name (`strings'), NULL for the default wheel. */
// FUNC 0x00236048 CarEquipments::getWheelName
const char *CarEquipments::getWheelName()
{
    WheelRecord *rec = getWheelRecord();
    if (rec == 0)
        return 0;
    return strings->getString(rec->nameText);
}

/* ------------------------------------------------------------------------ */
/* Stock or tuned                                                            */
/* ------------------------------------------------------------------------ */

/* 1 if every part slot holds the car's stock part (the tyres and the wheel
 * are not compared, the steer is); 0 also without a car record.  The first
 * test as if / else (retail `li 1; movz 0': the 1 stays in $v0 for the
 * result; `?:' gives a store-flag sltu) and the `> 0' test at the end. */
// FUNC 0x00236088 CarEquipments::hasStockParts
int CarEquipments::hasStockParts()
{
    CarRecord *car = (CarRecord *)getRecord(CARDB_CAR, carCode);
    if (car == 0)
        return 0;
    int changed;
    if (brake != car->parts.brake)
        changed = 1;
    else
        changed = 0;
    if (brakeController != car->parts.brakeController) changed++;
    if (chassis != car->parts.chassis) changed++;
    if (engine != car->parts.engine) changed++;
    if (drivetrain != car->parts.drivetrain) changed++;
    if (gear != car->parts.gear) changed++;
    if (suspension != car->parts.suspension) changed++;
    if (lsd != car->parts.lsd) changed++;
    if (steer != car->parts.steer) changed++;
    if (lightweight != car->parts.lightweight) changed++;
    if (racingModify != car->parts.racingModify) changed++;
    if (portPolish != car->parts.portPolish) changed++;
    if (engineBalance != car->parts.engineBalance) changed++;
    if (displacement != car->parts.displacement) changed++;
    if (computer != car->parts.computer) changed++;
    if (naTune != car->parts.naTune) changed++;
    if (turbine != car->parts.turbine) changed++;
    if (flywheel != car->parts.flywheel) changed++;
    if (clutch != car->parts.clutch) changed++;
    if (propellerShaft != car->parts.propellerShaft) changed++;
    if (muffler != car->parts.muffler) changed++;
    if (intercooler != car->parts.intercooler) changed++;
    if (ascc != car->parts.ascc) changed++;
    if (tcsc != car->parts.tcsc) changed++;
    if (changed > 0)
        return 0;
    return 1;
}

/* 1 if the car is tuned: a part differs from the stock one, or a fitted
 * part is not the category's base part (kind 0).  The first test as
 * hasStockParts' (retail keeps the 1 in $s2, also for LAYOUT_FF). */
// FUNC 0x002362B8 CarEquipments::isTuned
int CarEquipments::isTuned()
{
    if (hasStockParts() == 0)
        return 1;
    int tuned;
    if (getBrakePart() != PART_BRAKE)
        tuned = 1;
    else
        tuned = 0;
    if (getBrakecontrollerPart() != PART_BRAKE_CONTROLLER_NONE) tuned++;
    if (getDrivetrainPart() != PART_DRIVETRAIN) tuned++;
    if (getGearPart() != PART_GEAR) tuned++;
    if (getSuspensionPart() != PART_SUSPENSION) tuned++;
    if (getLSDPart() != PART_LSD) tuned++;
    if (getLightweightPart() != PART_LIGHTWEIGHT_NONE) tuned++;
    if (getRacingmodifyPart() != PART_RACING_MODIFY_NONE) tuned++;
    if (getPortpolishPart() != PART_PORT_POLISH_NONE) tuned++;
    if (getEnginebalancePart() != PART_ENGINE_BALANCE_NONE) tuned++;
    if (getDisplacementPart() != PART_DISPLACEMENT_NONE) tuned++;
    if (getComputerPart() != PART_COMPUTER) tuned++;
    if (getNATunePart() != PART_NA_TUNE_NONE) tuned++;
    if (getTurbinePart() != PART_TURBINE) tuned++;
    if (getFlywheelPart() != PART_FLYWHEEL) tuned++;
    if (getClutchPart() != PART_CLUTCH) tuned++;
    if (getPropellershaftPart() != PART_PROPELLER_SHAFT) {
        /* (redundant: getPropellershaftPart already gives PART_NONE for these) */
        switch (getDriveTrain()) {
        case LAYOUT_FF: case LAYOUT_MR: case LAYOUT_RR:
            break;
        default:
            tuned++;
            break;
        }
    }
    if (getMufflerPart() != PART_MUFFLER) tuned++;
    if (getIntercoolerPart() != PART_INTERCOOLER) {
        if (isTurboByDefault() != 0)
            tuned++;
    }
    if (getASCCPart() != PART_ASCC_NONE) tuned++;
    if (getTCSCPart() != PART_TCSC_NONE) tuned++;
    return tuned > 0;
}

/* 1 if both tyres are of kind TIRE_KIND_ARCADE (no NULL checks; caller
 * F_00236718, which fits them unless both already are).  This one and the
 * next two read the kind into an unsigned variable (retail's registers;
 * sltiu in hasFirstSetTires); hasDirtTires compares the field in place. */
// FUNC 0x002364E0 CarEquipments::hasArcadeTires
int CarEquipments::hasArcadeTires()
{
    int others = 0;
    u32 kind = getFronttireRecord()->kind;
    if (kind != TIRE_KIND_ARCADE)
        others++;
    kind = getReartireRecord()->kind;
    if (kind != TIRE_KIND_ARCADE)
        others++;
    if (others != 0)
        return 0;
    return 1;
}

/* 1 if both tyres are of kind TIRE_KIND_ARCADE_RACING (no NULL checks;
 * caller F_00236758). */
// FUNC 0x00236540 CarEquipments::hasArcadeRacingTires
int CarEquipments::hasArcadeRacingTires()
{
    int others = 0;
    u32 kind = getFronttireRecord()->kind;
    if (kind != TIRE_KIND_ARCADE_RACING)
        others++;
    kind = getReartireRecord()->kind;
    if (kind != TIRE_KIND_ARCADE_RACING)
        others++;
    if (others != 0)
        return 0;
    return 1;
}

/* 1 if neither tyre is of the second set (kind >= 13; no NULL checks;
 * caller F_00236868, which fits the first-set tyres unless both are). */
// FUNC 0x002365A0 CarEquipments::hasFirstSetTires
int CarEquipments::hasFirstSetTires()
{
    int secondSet = 0;
    u32 kind = getFronttireRecord()->kind;
    if (kind >= TIRE_KIND_SECOND_SET)
        secondSet++;
    kind = getReartireRecord()->kind;
    if (kind >= TIRE_KIND_SECOND_SET)
        secondSet++;
    if (secondSet != 0)
        return 0;
    return 1;
}

/* ------------------------------------------------------------------------ */
/* Tyres                                                                    */
/* ------------------------------------------------------------------------ */

/* 1 if both tyres are of the second set (kinds 13..24).  Callers:
 * RaceEntryCar::loadTires, RaceBase::virtual_39, F_00236798. */
// FUNC 0x00236600 CarEquipments::F_00236600
int CarEquipments::F_00236600()
{
    int others = 0;             /* tyres not of the second set */
    u32 kind = getFronttireRecord()->kind;
    if (kind < TIRE_KIND_SECOND_SET || kind > TIRE_KIND_SECOND_SET + 11)
        others++;
    kind = getReartireRecord()->kind;
    if (kind < TIRE_KIND_SECOND_SET || kind > TIRE_KIND_SECOND_SET + 11)
        others++;
    if (others != 0)
        return 0;
    return 1;
}

/* Fits the car's tyres of kind `kind' (front from table 0x1C, rear from
 * table 0x1D); nothing and 0 if the car has no such pair. */
// FUNC 0x00236668 CarEquipments::F_00236668
int CarEquipments::F_00236668(int kind)
{
    u64 front;
    u64 rear;

    front = findPart(CARDB_FRONT_TIRE, kind);
    if (front == 0)
        return 0;
    rear = findPart(CARDB_REAR_TIRE, kind);
    if (rear == 0)
        return 0;
    frontTire = front;
    rearTire  = rear;
    return 1;
}

/* The dirt tyres unless both tyres already are (1 then).  The record of the
 * front tyre is fetched and not used. */
// FUNC 0x002366D8 CarEquipments::F_002366D8
int CarEquipments::F_002366D8()
{
    if (hasDirtTires() != 0)
        return 1;
    getFronttireRecord();       /* result unused */
    return F_00236668(TIRE_KIND_DIRT);
}

/* The tyres of kind 0xA unless both already are (1 then). */
// FUNC 0x00236718 CarEquipments::F_00236718
int CarEquipments::F_00236718()
{
    if (hasArcadeTires() != 0)
        return 1;
    getFronttireRecord();       /* result unused */
    return F_00236668(TIRE_KIND_ARCADE);
}

/* The tyres of kind 0xB unless both already are (1 then). */
// FUNC 0x00236758 CarEquipments::F_00236758
int CarEquipments::F_00236758()
{
    if (hasArcadeRacingTires() != 0)
        return 1;
    getFronttireRecord();       /* result unused */
    return F_00236668(TIRE_KIND_ARCADE_RACING);
}

/* Unless both tyres are of the second set: the tyres of the second set
 * matching the front tyre (kind 0..11 -> 13..24); 0 for the other kinds. */
// FUNC 0x00236798 CarEquipments::F_00236798
int CarEquipments::F_00236798()
{
    int kind;

    if (F_00236600() != 0)
        return 1;
    kind = getFronttireRecord()->kind;
    switch (kind) {
    case 0x0: kind = TIRE_KIND_SECOND_SET + 0x0; break;
    case 0x1: kind = TIRE_KIND_SECOND_SET + 0x1; break;
    case 0x2: kind = TIRE_KIND_SECOND_SET + 0x2; break;
    case 0x3: kind = TIRE_KIND_SECOND_SET + 0x3; break;
    case 0x4: kind = TIRE_KIND_SECOND_SET + 0x4; break;
    case 0x5: kind = TIRE_KIND_SECOND_SET + 0x5; break;
    case 0x6: kind = TIRE_KIND_SECOND_SET + 0x6; break;
    case 0x7: kind = TIRE_KIND_SECOND_SET + 0x7; break;
    case 0x8: kind = TIRE_KIND_SECOND_SET + 0x8; break;
    case 0x9: kind = TIRE_KIND_SECOND_SET + 0x9; break;
    case 0xB: kind = TIRE_KIND_SECOND_SET + 0xB; break;     /* retail's order */
    case 0xA: kind = TIRE_KIND_SECOND_SET + 0xA; break;
    default:
        return 0;
    }
    return F_00236668(kind) != 0;
}

/* The reverse: unless neither tyre is of the second set (hasFirstSetTires), the
 * tyres of the first set matching the front tyre (kind 13..24 -> 0..11); 0
 * for the other kinds.  No caller. */
// FUNC 0x00236868 CarEquipments::F_00236868
int CarEquipments::F_00236868()
{
    int kind;

    if (hasFirstSetTires() != 0)
        return 1;
    kind = getFronttireRecord()->kind;
    switch (kind) {
    case TIRE_KIND_SECOND_SET + 0x0: kind = 0x0; break;
    case TIRE_KIND_SECOND_SET + 0x1: kind = 0x1; break;
    case TIRE_KIND_SECOND_SET + 0x2: kind = 0x2; break;
    case TIRE_KIND_SECOND_SET + 0x3: kind = 0x3; break;
    case TIRE_KIND_SECOND_SET + 0x4: kind = 0x4; break;
    case TIRE_KIND_SECOND_SET + 0x5: kind = 0x5; break;
    case TIRE_KIND_SECOND_SET + 0x6: kind = 0x6; break;
    case TIRE_KIND_SECOND_SET + 0x7: kind = 0x7; break;
    case TIRE_KIND_SECOND_SET + 0x8: kind = 0x8; break;
    case TIRE_KIND_SECOND_SET + 0x9: kind = 0x9; break;
    case TIRE_KIND_SECOND_SET + 0xA: kind = 0xA; break;
    case TIRE_KIND_SECOND_SET + 0xB: kind = 0xB; break;
    default:
        return 0;
    }
    return F_00236668(kind) != 0;
}

/* ------------------------------------------------------------------------ */
/* The save block                                                           */
/* ------------------------------------------------------------------------ */

/* Writes the 0x1D0-byte save block (CarSaveEquipments) into buf; returns
 * buf + 0x1D0.  Callers: ChampionshipSave::save, SettingSerialize::save. */
// FUNC 0x00236940 CarEquipments::serialize
u8 *CarEquipments::serialize(void *buf)
{
    memcpy(buf, static_cast<CarSaveEquipments *>(this), sizeof(CarSaveEquipments));
    return (u8 *)buf + sizeof(CarSaveEquipments);
}

/* Reads the save block from data; returns data + 0x1D0.  Callers:
 * ChampionshipSave::load, SettingSerialize::load. */
// FUNC 0x00236978 CarEquipments::deserialize
const u8 *CarEquipments::deserialize(const void *data)
{
    memcpy(static_cast<CarSaveEquipments *>(this), data, sizeof(CarSaveEquipments));
    return (const u8 *)data + sizeof(CarSaveEquipments);
}

/* If the transmission's ratios follow the auto level (gearFlag): the eight
 * gear ratios of level maxSpeed * 10 (gear_auto_ratios, T294) from the car's spec.
 * The transmission record is not checked for NULL.  Callers: setPreset,
 * setCar, fitPart. */
// FUNC 0x002369A8 CarEquipments::F_002369A8
void CarEquipments::F_002369A8()
{
    s16     ratios[8];          /* sp+0x00 */
    CarSpec spec;               /* sp+0x10 */
    int     i;

    if (getGearRecord()->gearFlag == 0)
        return;
    getSpec(&spec);
    gear_auto_ratios(&spec, maxSpeed * 10, ratios, 1);
    for (i = 0; i < 8; i++)
        gearRatio[i] = ratios[i];
}

/* region_lock_check of the car record's regionLock: 1 if the car is locked
 * out of this region.  Caller: run_license_test. */
// FUNC 0x00236A38 CarEquipments::F_00236A38
int CarEquipments::F_00236A38()
{
    return region_lock_check(getCarRecord()->regionLock);
}

/* loadPreset of the code of a name.  No caller. */
// FUNC 0x00236A60 CarEquipments::F_00236A60
int CarEquipments::F_00236A60(const char *name)
{
    return loadPreset(nameToCode(name));
}

/* findPart.  No caller. */
// FUNC 0x00236A98 CarEquipments::F_00236A98
u64 CarEquipments::F_00236A98(int table, int kind)
{
    return findPart(table, kind);
}

/* setCar of the code of a name.  No caller. */
// FUNC 0x00236AB8 CarEquipments::F_00236AB8
int CarEquipments::F_00236AB8(const char *name)
{
    return setCar(nameToCode(name));
}

/* ======================================================================== */
/* The class-inline members (0x00236AF0..0x002377A0): no caller anywhere.   */
/* egcs emits every inline member of CarEquipments here, in the unit of its */
/* key function ~CarEquipments, after the unit's own functions and before   */
/* __tf13CarEquipments, in the reverse of their definition order (the       */
/* bodies: game/data/T482_car_equipments.h, defined there from getSteerType back */
/* to getSettingRearLSDparam3Max).  The markers name the symbols.           */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/* Setting ranges: min / max of the fitted parts' records                   */
/* ------------------------------------------------------------------------ */

/* The records are not checked for NULL (their getters return NULL when no
 * part is fitted).  The LSD parameters param / param2 / param3 are the
 * record's initial / accel / decel. */
// FUNC 0x00236AF0 CarEquipments::getSettingRearLSDparam3Max (getSettingRearLSDparam3Max__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236B10 CarEquipments::getSettingRearLSDparam3Min (getSettingRearLSDparam3Min__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236B30 CarEquipments::getSettingRearLSDparam2Max (getSettingRearLSDparam2Max__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236B50 CarEquipments::getSettingRearLSDparam2Min (getSettingRearLSDparam2Min__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236B70 CarEquipments::getSettingRearLSDparamMax (getSettingRearLSDparamMax__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236B90 CarEquipments::getSettingRearLSDparamMin (getSettingRearLSDparamMin__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236BB0 CarEquipments::getSettingFrontLSDparam3Max (getSettingFrontLSDparam3Max__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236BD0 CarEquipments::getSettingFrontLSDparam3Min (getSettingFrontLSDparam3Min__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236BF0 CarEquipments::getSettingFrontLSDparam2Max (getSettingFrontLSDparam2Max__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236C10 CarEquipments::getSettingFrontLSDparam2Min (getSettingFrontLSDparam2Min__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236C30 CarEquipments::getSettingFrontLSDparamMax (getSettingFrontLSDparamMax__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236C50 CarEquipments::getSettingFrontLSDparamMin (getSettingFrontLSDparamMin__13CarEquipments)
/* implicit: getLSDRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236C70 CarEquipments::getSettingRearSpringrateMax (getSettingRearSpringrateMax__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236C90 CarEquipments::getSettingRearSpringrateMin (getSettingRearSpringrateMin__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236CB0 CarEquipments::getSettingFrontSpringrateMax (getSettingFrontSpringrateMax__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236CD0 CarEquipments::getSettingFrontSpringrateMin (getSettingFrontSpringrateMin__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236CF0 CarEquipments::getSettingRearRideheightMax (getSettingRearRideheightMax__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236D10 CarEquipments::getSettingRearRideheightMin (getSettingRearRideheightMin__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236D30 CarEquipments::getSettingFrontRideheightMax (getSettingFrontRideheightMax__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236D50 CarEquipments::getSettingFrontRideheightMin (getSettingFrontRideheightMin__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236D70 CarEquipments::getSettingRearToeMax (getSettingRearToeMax__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236D90 CarEquipments::getSettingRearToeMin (getSettingRearToeMin__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236DB0 CarEquipments::getSettingFrontToeMax (getSettingFrontToeMax__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236DD0 CarEquipments::getSettingFrontToeMin (getSettingFrontToeMin__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236DF0 CarEquipments::getSettingRearCamberMax (getSettingRearCamberMax__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236E10 CarEquipments::getSettingRearCamberMin (getSettingRearCamberMin__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236E30 CarEquipments::getSettingFrontCamberMax (getSettingFrontCamberMax__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236E50 CarEquipments::getSettingFrontCamberMin (getSettingFrontCamberMin__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236E70 CarEquipments::getSettingClRMax (getSettingClRMax__13CarEquipments)
/* implicit: getRacingmodifyRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236E90 CarEquipments::getSettingClRMin (getSettingClRMin__13CarEquipments)
/* implicit: getRacingmodifyRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236EB0 CarEquipments::getSettingClFMax (getSettingClFMax__13CarEquipments)
/* implicit: getRacingmodifyRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236ED0 CarEquipments::getSettingClFMin (getSettingClFMin__13CarEquipments)
/* implicit: getRacingmodifyRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236EF0 CarEquipments::getSettingParam4WDMax (getSettingParam4WDMax__13CarEquipments)
/* implicit: getDrivetrainRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236F10 CarEquipments::getSettingParam4WDMin (getSettingParam4WDMin__13CarEquipments)
/* implicit: getDrivetrainRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236F30 CarEquipments::getSettingMaxSpeedMax (getSettingMaxSpeedMax__13CarEquipments)
/* implicit: getGearRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236F50 CarEquipments::getSettingMaxSpeedMin (getSettingMaxSpeedMin__13CarEquipments)
/* implicit: getGearRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236F70 CarEquipments::getSettingFinalGearRatioMax (getSettingFinalGearRatioMax__13CarEquipments)
/* implicit: getGearRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00236F90 CarEquipments::getSettingFinalGearRatioMin (getSettingFinalGearRatioMin__13CarEquipments)
/* implicit: getGearRecord (inline in game/data/T482_car_equipments.h) */

/* ------------------------------------------------------------------------ */
/* Setting values                                                           */
/* ------------------------------------------------------------------------ */

// FUNC 0x00236FB0 CarEquipments::getSettingASCC (getSettingASCC__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FB8 CarEquipments::getSettingTCSC (getSettingTCSC__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FC0 CarEquipments::getSettingRearLSDparam3 (getSettingRearLSDparam3__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FC8 CarEquipments::getSettingRearLSDparam2 (getSettingRearLSDparam2__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FD0 CarEquipments::getSettingRearLSDparam (getSettingRearLSDparam__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FD8 CarEquipments::getSettingFrontLSDparam3 (getSettingFrontLSDparam3__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FE0 CarEquipments::getSettingFrontLSDparam2 (getSettingFrontLSDparam2__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FE8 CarEquipments::getSettingFrontLSDparam (getSettingFrontLSDparam__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FF0 CarEquipments::getSettingRearStabilizer (getSettingRearStabilizer__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00236FF8 CarEquipments::getSettingFrontStabilizer (getSettingFrontStabilizer__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237000 CarEquipments::getSettingRearSpringrate (getSettingRearSpringrate__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237008 CarEquipments::getSettingFrontSpringrate (getSettingFrontSpringrate__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237010 CarEquipments::getSettingRearRideheight (getSettingRearRideheight__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237018 CarEquipments::getSettingFrontRideheight (getSettingFrontRideheight__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237020 CarEquipments::getSettingRearToe (getSettingRearToe__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237028 CarEquipments::getSettingFrontToe (getSettingFrontToe__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237030 CarEquipments::getSettingRearCamber (getSettingRearCamber__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237038 CarEquipments::getSettingFrontCamber (getSettingFrontCamber__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237040 CarEquipments::getSettingRearDamperF2R (getSettingRearDamperF2R__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237048 CarEquipments::getSettingRearDamperF1R (getSettingRearDamperF1R__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237050 CarEquipments::getSettingRearDamperF2B (getSettingRearDamperF2B__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237058 CarEquipments::getSettingRearDamperF1B (getSettingRearDamperF1B__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237060 CarEquipments::getSettingFrontDamperF2R (getSettingFrontDamperF2R__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237068 CarEquipments::getSettingFrontDamperF1R (getSettingFrontDamperF1R__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237070 CarEquipments::getSettingFrontDamperF2B (getSettingFrontDamperF2B__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237078 CarEquipments::getSettingFrontDamperF1B (getSettingFrontDamperF1B__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237080 CarEquipments::getSettingResponse2 (getSettingResponse2__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237088 CarEquipments::getSettingPeekRPM2 (getSettingPeekRPM2__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237090 CarEquipments::getSettingBoost2 (getSettingBoost2__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237098 CarEquipments::getSettingResponse1 (getSettingResponse1__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002370A0 CarEquipments::getSettingPeekRPM1 (getSettingPeekRPM1__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002370A8 CarEquipments::getSettingBoost1 (getSettingBoost1__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002370B0 CarEquipments::getSettingClR (getSettingClR__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002370B8 CarEquipments::getSettingClF (getSettingClF__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002370C0 CarEquipments::getSettingRearABS (getSettingRearABS__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002370C8 CarEquipments::getSettingFrontABS (getSettingFrontABS__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002370D0 CarEquipments::getSettingParam4WD (getSettingParam4WD__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002370D8 CarEquipments::getSettingLastFinalGearRatio (getSettingLastFinalGearRatio__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

/* The transmission record's gearFlag (not checked for NULL). */
// FUNC 0x002370E0 CarEquipments::getGearFlag (getGearFlag__13CarEquipments)
/* implicit: getGearRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237100 CarEquipments::getSettingMaxSpeed (getSettingMaxSpeed__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237108 CarEquipments::getSettingFinalGearRatio (getSettingFinalGearRatio__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

/* Gear ratio i ([0] the reverse gear; no bounds check). */
// FUNC 0x00237110 CarEquipments::getSettingGearRatio (getSettingGearRatio__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

/* The forward gears of the transmission (not checked for NULL). */
// FUNC 0x00237120 CarEquipments::getGearNum (getGearNum__13CarEquipments)
/* implicit: getGearRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237140 CarEquipments::setSettingASCC (setSettingASCC__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237148 CarEquipments::setSettingTCSC (setSettingTCSC__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237150 CarEquipments::setSettingRearLSDparam3 (setSettingRearLSDparam3__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237158 CarEquipments::setSettingRearLSDparam2 (setSettingRearLSDparam2__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237160 CarEquipments::setSettingRearLSDparam (setSettingRearLSDparam__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237168 CarEquipments::setSettingFrontLSDparam3 (setSettingFrontLSDparam3__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237170 CarEquipments::setSettingFrontLSDparam2 (setSettingFrontLSDparam2__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237178 CarEquipments::setSettingFrontLSDparam (setSettingFrontLSDparam__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237180 CarEquipments::setSettingRearStabilizer (setSettingRearStabilizer__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237188 CarEquipments::setSettingFrontStabilizer (setSettingFrontStabilizer__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237190 CarEquipments::setSettingRearSpringrate (setSettingRearSpringrate__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237198 CarEquipments::setSettingFrontSpringrate (setSettingFrontSpringrate__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371A0 CarEquipments::setSettingRearRideheight (setSettingRearRideheight__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371A8 CarEquipments::setSettingFrontRideheight (setSettingFrontRideheight__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371B0 CarEquipments::setSettingRearToe (setSettingRearToe__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371B8 CarEquipments::setSettingFrontToe (setSettingFrontToe__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371C0 CarEquipments::setSettingRearCamber (setSettingRearCamber__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371C8 CarEquipments::setSettingFrontCamber (setSettingFrontCamber__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371D0 CarEquipments::setSettingRearDamperF2R (setSettingRearDamperF2R__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371D8 CarEquipments::setSettingRearDamperF1R (setSettingRearDamperF1R__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

/* Both rebound stages. */
// FUNC 0x002371E0 CarEquipments::setSettingRearDamperFR (setSettingRearDamperFR__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

/* index.txt: gt2k_menu::RawWindow::setInputOccurred(bool), a symbol match of
 * the same two instructions; the field (+0x180) and its neighbours make it
 * this setter. */
// FUNC 0x002371F0 CarEquipments::setSettingRearDamperF2B (setSettingRearDamperF2B__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002371F8 CarEquipments::setSettingRearDamperF1B (setSettingRearDamperF1B__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

/* Both bound stages. */
// FUNC 0x00237200 CarEquipments::setSettingRearDamperFB (setSettingRearDamperFB__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

/* index.txt: gt2k_menu::RawWindow::setMenuControlFocus(bool), a symbol match
 * of the same two instructions; the field (+0x178) and its neighbours make
 * it this setter. */
// FUNC 0x00237210 CarEquipments::setSettingFrontDamperF2R (setSettingFrontDamperF2R__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237218 CarEquipments::setSettingFrontDamperF1R (setSettingFrontDamperF1R__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237220 CarEquipments::setSettingFrontDamperFR (setSettingFrontDamperFR__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237230 CarEquipments::setSettingFrontDamperF2B (setSettingFrontDamperF2B__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237238 CarEquipments::setSettingFrontDamperF1B (setSettingFrontDamperF1B__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237240 CarEquipments::setSettingFrontDamperFB (setSettingFrontDamperFB__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237250 CarEquipments::setSettingResponse2 (setSettingResponse2__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237258 CarEquipments::setSettingPeekRPM2 (setSettingPeekRPM2__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237260 CarEquipments::setSettingBoost2 (setSettingBoost2__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237268 CarEquipments::setSettingResponse1 (setSettingResponse1__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237270 CarEquipments::setSettingPeekRPM1 (setSettingPeekRPM1__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237278 CarEquipments::setSettingBoost1 (setSettingBoost1__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237280 CarEquipments::setSettingClR (setSettingClR__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237288 CarEquipments::setSettingClF (setSettingClF__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237290 CarEquipments::setSettingRearABS (setSettingRearABS__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237298 CarEquipments::setSettingFrontABS (setSettingFrontABS__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002372A0 CarEquipments::setSettingParam4WD (setSettingParam4WD__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002372A8 CarEquipments::setSettingLastFinalGearRatio (setSettingLastFinalGearRatio__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002372B0 CarEquipments::setSettingMaxSpeed (setSettingMaxSpeed__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002372B8 CarEquipments::setSettingFinalGearRatio (setSettingFinalGearRatio__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

/* Gear ratio i (no bounds check). */
// FUNC 0x002372C0 CarEquipments::setSettingGearRatio (setSettingGearRatio__13CarEquipmentsii)
/* inline in game/data/T482_car_equipments.h */

/* ------------------------------------------------------------------------ */
/* Setting levels of the fitted parts' records                              */
/* ------------------------------------------------------------------------ */

/* The level counts of the settings (interpolate maps a level 1..count onto
 * the record's from..to); the records are not checked for NULL.  The ASCC
 * record's four settings are VSC param 1, VSC param B, VUC param 1, VUC
 * param B. */
// FUNC 0x002372D0 CarEquipments::getMaxLevelVUCparamB (getMaxLevelVUCparamB__13CarEquipments)
/* implicit: getASCCRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x002372F0 CarEquipments::getMaxLevelVUCparam1 (getMaxLevelVUCparam1__13CarEquipments)
/* implicit: getASCCRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237310 CarEquipments::getMaxLevelVSCparamB (getMaxLevelVSCparamB__13CarEquipments)
/* implicit: getASCCRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237330 CarEquipments::getMaxLevelVSCparam1 (getMaxLevelVSCparam1__13CarEquipments)
/* implicit: getASCCRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237350 CarEquipments::getMaxLevelTCSC (getMaxLevelTCSC__13CarEquipments)
/* implicit: getTCSCRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237370 CarEquipments::getMaxLevelRearStabilizer (getMaxLevelRearStabilizer__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237390 CarEquipments::getMaxLevelFrontStabilizer (getMaxLevelFrontStabilizer__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x002373B0 CarEquipments::getMaxLevelRearDamperFR (getMaxLevelRearDamperFR__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x002373D0 CarEquipments::getMaxLevelRearDamperFB (getMaxLevelRearDamperFB__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x002373F0 CarEquipments::getMaxLevelFrontDamperFR (getMaxLevelFrontDamperFR__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237410 CarEquipments::getMaxLevelFrontDamperFB (getMaxLevelFrontDamperFB__13CarEquipments)
/* implicit: getSuspensionRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237430 CarEquipments::getMaxLevelRearABS (getMaxLevelRearABS__13CarEquipments)
/* implicit: getBrakecontrollerRecord (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237450 CarEquipments::getMaxLevelFrontABS (getMaxLevelFrontABS__13CarEquipments)
/* implicit: getBrakecontrollerRecord (inline in game/data/T482_car_equipments.h) */

/* ? name: the order says Param4WD, but this reads the drive train record's
 * vcdMax (+0x12), the same field as getSettingParam4WDMax, not a level
 * count. */
// FUNC 0x00237470 CarEquipments::getMaxLevelParam4WD (getMaxLevelParam4WD__13CarEquipments)
/* implicit: getDrivetrainRecord (inline in game/data/T482_car_equipments.h) */

/* ------------------------------------------------------------------------ */
/* The fitted parts                                                         */
/* ------------------------------------------------------------------------ */

/* The setters of the parts with settings store the code, then reset the
 * settings to the part's defaults (resetGear .. resetTcsc, above). */
// FUNC 0x00237490 CarEquipments::setTCSC (setTCSC__13CarEquipmentsUx)
/* implicit: resetTcsc (inline in game/data/T482_car_equipments.h) */

// FUNC 0x002374B0 CarEquipments::setASCC (setASCC__13CarEquipmentsUx)
/* implicit: resetAscc (inline in game/data/T482_car_equipments.h) */

// FUNC 0x002374D0 CarEquipments::setDrivetrain (setDrivetrain__13CarEquipmentsUx)
/* implicit: resetDrivetrain (inline in game/data/T482_car_equipments.h) */

// FUNC 0x002374F0 CarEquipments::setGear (setGear__13CarEquipmentsUx)
/* implicit: resetGear (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237510 CarEquipments::setBrakecontroller (setBrakecontroller__13CarEquipmentsUx)
/* implicit: resetBrakeController (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237530 CarEquipments::setRacingmodify (setRacingmodify__13CarEquipmentsUx)
/* implicit: resetRacingModify (inline in game/data/T482_car_equipments.h) */

/* ? name (no Lupo 2 symbol: setTurbo or setTurbine).  Fits the turbine kit
 * and puts the car's stock NA tune back (the two exclude each other), then
 * the turbo settings follow the kit. */
// FUNC 0x00237550 CarEquipments::setTurbo (setTurbo__13CarEquipmentsUx)
/* implicit: getDefaultPartCode, resetTurbineKit (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237598 CarEquipments::setSuspension (setSuspension__13CarEquipmentsUx)
/* implicit: resetSuspension (inline in game/data/T482_car_equipments.h) */

// FUNC 0x002375B8 CarEquipments::setLSD (setLSD__13CarEquipmentsUx)
/* implicit: resetLsd (inline in game/data/T482_car_equipments.h) */

/* ? name (no Lupo 2 symbol).  The reverse of setTurbo: the car's stock
 * turbine kit back, resetTurbineKit, then the NA tune (stored after the
 * reset). */
// FUNC 0x002375D8 CarEquipments::setNATune (setNATune__13CarEquipmentsUx)
/* implicit: getDefaultPartCode, resetTurbineKit (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237620 CarEquipments::setDefaultWheel (setDefaultWheel__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237628 CarEquipments::setWheel (setWheel__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237630 CarEquipments::setIntercooler (setIntercooler__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237638 CarEquipments::setMuffler (setMuffler__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237640 CarEquipments::setPropellershaft (setPropellershaft__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237648 CarEquipments::setClutch (setClutch__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237650 CarEquipments::setFlywheel (setFlywheel__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237658 CarEquipments::setComputer (setComputer__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237660 CarEquipments::setDisplacement (setDisplacement__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237668 CarEquipments::setEnginebalance (setEnginebalance__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237670 CarEquipments::setPortpolish (setPortpolish__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237678 CarEquipments::setLightweight (setLightweight__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237680 CarEquipments::setReartire (setReartire__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237688 CarEquipments::setFronttire (setFronttire__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237690 CarEquipments::setEngine (setEngine__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237698 CarEquipments::setChassis (setChassis__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002376A0 CarEquipments::setBrake (setBrake__13CarEquipmentsUx)
/* inline in game/data/T482_car_equipments.h */

/* ------------------------------------------------------------------------ */
/* The car                                                                  */
/* ------------------------------------------------------------------------ */

// FUNC 0x002376A8 CarEquipments::getCarCode (getCarCode__C13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002376B0 CarEquipments::getColorIndex (getColorIndex__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x002376B8 CarEquipments::getColorCode (getColorCode__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

/* The shop flags of the car record (getShopFlags; the meanings are u009's
 * reading of its callers).  No callers of the three. */
/* Bit 2 (? the wheels cannot be changed). */
// FUNC 0x002376C0 CarEquipments::F_002376C0 (F_002376C0__13CarEquipments)
/* implicit: getShopFlags (inline in game/data/T482_car_equipments.h) */

/* Bit 1 (? not sold at the used car shop). */
// FUNC 0x002376E8 CarEquipments::F_002376E8 (F_002376E8__13CarEquipments)
/* implicit: getShopFlags (inline in game/data/T482_car_equipments.h) */

/* Bit 0 (? not sold by the dealer). */
// FUNC 0x00237710 CarEquipments::F_00237710 (F_00237710__13CarEquipments)
/* implicit: getShopFlags (inline in game/data/T482_car_equipments.h) */

/* getWheelName (the fitted wheel's name, NULL for the default wheel). */
// FUNC 0x00237730 CarEquipments::F_00237730 (F_00237730__13CarEquipments)
/* implicit: getWheelName (inline in game/data/T482_car_equipments.h) */

/* The assignment (operator=, the 0x1E8 bytes). */
// FUNC 0x00237750 CarEquipments::F_00237750 (F_00237750__13CarEquipmentsRC13CarEquipments)
/* implicit: operator= (inline in game/data/T482_car_equipments.h) */

// FUNC 0x00237770 CarEquipments::getCarType (getCarType__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237778 CarEquipments::setRestrictorLimit (setRestrictorLimit__13CarEquipmentsi)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237780 CarEquipments::getRestrictorLimit (getRestrictorLimit__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

/* The size of the save block (serialize / deserialize). */
// FUNC 0x00237788 CarEquipments::dataSize (dataSize__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

/* (a word store: egcs's bool is 4 bytes) */
// FUNC 0x00237790 CarEquipments::setPresetCar (setPresetCar__13CarEquipmentsb)
/* inline in game/data/T482_car_equipments.h */

// FUNC 0x00237798 CarEquipments::IsPresetCar (IsPresetCar__13CarEquipments)
/* inline in game/data/T482_car_equipments.h */

/* The car record's steering type (not checked for NULL). */
// FUNC 0x002377A0 CarEquipments::getSteerType (getSteerType__13CarEquipments)
/* implicit: getCarRecord (inline in game/data/T482_car_equipments.h) */

/* ------------------------------------------------------------------------ */
/* Typeinfo                                                                 */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated: builds the __class_type_info __ti13CarEquipments
 * (two bases: CarSaveEquipments at 0, CarDataBase at +0x1D0, list
 * 0x003422E0) on the first call.  Callers: __tf20RaceEntryInformation,
 * __tf14CarInformation; vtable 0x003422C0 entry 0. */
// FUNC 0x002377C0 __tf13CarEquipments
extern "C" void *__tf13CarEquipments(void)
{
    if (__ti13CarEquipments[0] == 0) {
        __tf11CarDataBase();
        __tf17CarSaveEquipments();
        __rtti_class(__ti13CarEquipments, "13CarEquipments", g_003422E0, 2);
    }
    return __ti13CarEquipments;
}

/* Compiler-generated: builds the __user_type_info __ti17CarSaveEquipments
 * (gp-0x4550) on the first call. */
// FUNC 0x00237820 __tf17CarSaveEquipments
extern "C" void *__tf17CarSaveEquipments(void)
{
    if (__ti17CarSaveEquipments[0] == 0)
        __rtti_user(__ti17CarSaveEquipments, "17CarSaveEquipments");
    return __ti17CarSaveEquipments;
}
#endif
