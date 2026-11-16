/* T284 (0x001DB220..0x001DD0C8): CarDynamics_init, the physics parameter
 * block of one car (CarDynamics, T279) built from its packed spec record
 * (CarSpec, T482: bytes and halfwords in mm, %, km/h, degrees ...), and its
 * helpers.  The `log' argument passed down everywhere is not used by the
 * code (? a debug printer in the original; the damper labels 'Front/Bound'
 * and 'Front/Rebound' are passed with it). */
/* Retail T284's .rodata (0x0033C378): "bad_alloc" (nothing references it:
 * DATA_START), the two damper labels, then UnitArena<64>'s type name: the
 * original saw the race headers (every retail file with that name has a
 * "bad_alloc" of its own). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/T284_car_param.h"
#include "game/car/physics/car_param_internal.h"
#include "game/enhanced.h"                          /* ENH_PHYSICS_* */
// DATA_START 0x0033C378

// FUNC 0x001DB220 curve_setup_kmh_deg
void curve_setup_kmh_deg(Curve *c, f32 *x, f32 *y, u8 n, u8 *kmh, u8 *deg5)
{
    int i;

    for (i = 0; i < n; i++) {
        x[i] = (f32)kmh[i] * KMH_TO_MPS;
        y[i] = (f32)deg5[i] * 0.2f * DEG2RAD;
    }
    curveSet(c, n, x, y, 0, 0);
}

// FUNC 0x001DB2C8 CarDynamics_calcTopSpeed
/* Steps the speed from 0 to the current maxSpeed (in km/h, maxSpeed / 500 + 1
 * km/h a step), shifting up past the rev limit, and keeps the last speed
 * where the drive force at the wheels exceeds the resistance: rolling
 * (resistCurve(v) x rollingResistance[0] x (weight + lift)) and air drag.
 * The engine and ratio-table pointers are taken in the loop: GCSE hoists
 * them (retail's $s5 / $fp), and the lift terms before the curve lookup.
 * The enhanced engine (Lupo 2 L2 0x001DB940): an electric box is held at
 * 180 km/h (the AI drives by this); a CVT runs at its target rpm through the
 * ratio factor (gearbox_cvt_limit), with no gear steps. */
void CarDynamics_calcTopSpeed(CarDynamics *p, CarCondition *cond)
{
    GearCalc *g = &p->gear;
    int gear = 1;
    int kmh = 0;
    int topKmh;
    int step;

    if (gearbox_is_electric(g)) {
        p->maxSpeed = 49.999996f;           /* Lupo 2's 50 m/s (L2 gp-0x76BC) */
        return;
    }
    topKmh = (int)(p->maxSpeed * MPS_TO_KMH);
    step = topKmh / 500 + 1;
    for (; kmh < topKmh; kmh += step) {
        CarEngine *engine = &p->engine;
        f32 v = (f32)kmh * KMH_TO_MPS;
        f32 *ratios = g->ratioPerRadius;
        f32 *ratio = &ratios[gear];
        f32 engineSpeed = v * *ratio;       /* rad/s */
        f32 v2 = v * v;
        f32 factor = 1.0f;
        int rpm;
        f32 drive;

        gearbox_cvt_limit(g, &engineSpeed, &factor);     /* Lupo 2 (L2 0x001DBA34): types other than 1 untouched */
        rpm = (int)(engineSpeed * 60.0f / TWO_PI);
        drive = engine_power_limit(engine,
                                   engine_torque_at(engine, cond, engineSpeed),
                                   engineSpeed) * *ratio;
        if (gearbox_is_cvt(g))
            drive = drive * factor;
        f32 liftF = p->liftCoef[0] * v2;
        f32 liftR = p->liftCoef[1] * v2;
        f32 resist = curveLookupLinear(&g_002D2CE0.resistCurve, v)
                         * g_002D2CE0.rollingResistance[0]
                         * (p->weight + liftF + liftR)
                     + p->dragCoef * v2;

        if (resist < drive)
            p->maxSpeed = v;
        if (!gearbox_is_cvt(g) && gear < g->gearCount)
            gear += ((int)g->revLimit < rpm);
    }
}

// FUNC 0x001DB498 CarDynamics_initTopSpeed
/* maxSpeed = the speed at the redline in top gear, then refined by
 * CarDynamics_calcTopSpeed.  `log' is passed by the caller but not used.
 * The enhanced engine: a CVT's target rpm, the engine's peak power rpm, at
 * most the rev limit - 500, kept in GearCalc.shiftRpm[0] (Lupo 2 L2
 * 0x001DBC00..0x001DBC34). */
void CarDynamics_initTopSpeed(CarDynamics *p, CarCondition *cond, void *log)
{
    p->maxSpeed = ((f32)p->engine.redline * RPM_TO_RADS)
                / p->gear.ratioPerRadius[p->gear.gearCount];
    p->unk_E34 = p->mass / (2.0f * (2.0f * (p->brakeTorque[0] * p->gear.tyreRadiusInv[0])));
    if (gearbox_is_cvt(&p->gear)) {
        EngineSpecSheet sheet;
        int limit;

        engine_spec_sheet(&p->engine, &sheet, cond);
        limit = (int)p->gear.revLimit - 500;
        p->gear.shiftRpm[0] = (u16)((limit < (u16)sheet.maxPowerRpm) ? limit : (u16)sheet.maxPowerRpm);
    }
    CarDynamics_calcTopSpeed(p, cond);
}

// FUNC 0x001DB510 inv_percent
f32 inv_percent(u8 pct)
{
    return 100.0f / (f32)pct;
}

// FUNC 0x001DB538 damper_unpack
/* One damper record: the slow and the fast point, each a speed (% x scale)
 * and a force (x 5 kgf). */
void damper_unpack(f32 *slowK, f32 *slowF, f32 *fastK, f32 *fastF,
                   u8 slowPct, u8 slowForce, u8 fastPct, u8 fastForce, f32 scale)
{
    *slowK = (f32)slowPct * 0.01f;
    *slowF = (f32)slowForce * 5.0f * GRAVITY;
    *fastK = (f32)fastPct * 0.01f;
    *fastF = (f32)fastForce * 5.0f * GRAVITY;
    *slowK *= scale;
    *fastK *= scale;
}

// FUNC 0x001DB5C0 damper_fit
/* Fits the damper curve F(v) = c (sqrt(v - a) - sqrt(-a)) (b = -c sqrt(-a):
 * F(0) = 0) through (x1, y1) and (x2, y2); all three stay 0 if the points
 * give no such curve.  The listing shows the two R5900 sqrt.s (encoded with
 * the source in ft): boot_sqrtf_v, the volatile form (retail loads *c only
 * after the second sqrt). */
void damper_fit(f32 *a, f32 *b, f32 *c, void *log, const char *label,
                f32 x1, f32 y1, f32 x2, f32 y2)
{
    f32 cross, crossSq, shift, gainSq;

    *a = *b = *c = 0.0f;
    if (y1 == 0.0f) return;
    if (y2 == 0.0f) return;
    if (x1 == 0.0f) return;
    if (x2 == 0.0f) return;
    if (x2 <= x1) return;
    if (y2 <= y1) return;
    cross = y1 * x2 - y2 * x1;
    if (cross <= 0.0f) return;
    crossSq = y1 * y1 * x2 - y2 * y2 * x1;
    if (0.0f < crossSq) return;
    shift = crossSq / (cross + cross);
    gainSq = y1 * y2 * (y2 - y1) / cross;
    *c = boot_sqrtf_v(gainSq);
    *a = -(shift * shift) / gainSq;
    {
        f32 r = boot_sqrtf_v(-*a);

        *b = -*c * r;
    }
}

// FUNC 0x001DB6B8 SuspParam_init
/* The spring (x 980 x the axle's %), the bump and rebound dampers (their
 * speeds scaled by 100 / %), the stabilizer, the bump stop and the unsprung
 * mass of one axle. */
void SuspParam_init(SuspParam *s, CarSpecSuspension *ss, u8 pct, void *log)
{
    f32 scale;
    f32 slowK, slowF, fastK, fastF;

    s->spring = (f32)ss->springRate * 980.0f * ((f32)pct * 0.01f);
    scale = inv_percent(pct);
    damper_unpack(&slowK, &slowF, &fastK, &fastF,
                  ss->bumpSlowSpeed, ss->damper, ss->bumpFastSpeed, ss->damper2, scale);
    damper_fit(&s->bumpOfs, &s->bumpBase, &s->bumpGain, log, "Front/Bound",
               slowK, slowF, fastK, fastF);
    damper_unpack(&slowK, &slowF, &fastK, &fastF,
                  ss->reboundSlowSpeed, ss->rebound, ss->reboundFastSpeed, ss->rebound2, scale);
    damper_fit(&s->reboundOfs, &s->reboundBase, &s->reboundGain, log, "Front/Rebound",
               slowK, slowF, fastK, fastF);
    s->antiRoll = (f32)ss->stabilizer * 10.0f * GRAVITY;
    s->bumpStop = (f32)ss->bumpStop * 10000.0f * GRAVITY;
    s->unsprungMass = (f32)ss->unsprungMass * 0.25f;
    s->unsprungWeight = (f32)ss->unsprungMass * 0.25f * GRAVITY;
    s->bumpStopDamp = (f32)ss->bumpStopDamp * 10.0f * GRAVITY;
}

// FUNC 0x001DB890 SuspParam_initStroke
/* The static stroke (the sprung weight on the spring) and the stroke limits
 * of one axle; spec strokeLimit[axle] / strokeExt[axle] and susp[axle].bumpStopStroke
 * are limits in mm.  `log' is passed by the caller but not used. */
void SuspParam_initStroke(CarDynamics *p, SuspParam *s, CarSpec *spec, int axle, void *log)
{
    f32 spring = s->spring;
    f32 stat, axleH, base, limit, bumpLimit, hangMax;

    if (spring != 0.0f)
        s->staticStroke = (-p->sprungWeight[axle] * 0.5f) / spring;
    stat = s->staticStroke;
    s->gapOffset = (p->gear.tyreRadius[axle] - p->axleHeight[axle]) + stat;
    axleH = p->axleHeight[axle];
    limit = (f32)spec->strokeLimit[axle] * 0.001f;
    base = stat - axleH;
    if (axleH < limit)
        limit = axleH;
    s->strokeMin = base + limit;
    bumpLimit = (f32)spec->susp[axle].bumpStopStroke * 0.001f;
    if (p->axleHeight[axle] < bumpLimit + limit)
        s->bumpStart = base + limit;
    else
        s->bumpStart = (base + p->axleHeight[axle]) - bumpLimit;
    {
        f32 max = stat + (f32)spec->strokeExt[axle] * 0.001f;

        hangMax = s->unsprungWeight / spring;
        s->strokeMax = max;
        if (hangMax < max)
            s->strokeMax = hangMax;
    }
}

// FUNC 0x001DB9B8 CarDynamics_getTyre
TyreData *CarDynamics_getTyre(CarDynamics *p, int wheel, int slot)
{
    return &p->tyre[p->tyreIndex[wheel >> 1][slot]];
}

// FUNC 0x001DB9E0 TyreData_initSlipCurves
/* slipCurve (slipY) and slipCurve2 (slipY2) over slipX: the negative side
 * (curve1: the slip x 1/255, negated, then the two values x 1/200, stored
 * backwards) then the positive side (curve2); the centre point is shared
 * (the records' first entries are forced: 0, and the negative side's
 * slipY2 centre = the positive side's).  Also the two peaks of slipCurve. */
void TyreData_initSlipCurves(TyreData *t, CarSpecTyre *ts)
{
    int nNeg, nPos, i, j, k;           /* k = nNeg + j: j the offset from the centre point */
    f32 peak;

    ts->curve1.v[0][0] = 0;
    nNeg = ts->curve1Count;
    nPos = ts->curve2Count;
    ts->curve1.v[2][0] = ts->curve2.v[2][0];
    ts->curve1.v[1][0] = 0;
    ts->curve2.v[0][0] = 0;
    ts->curve2.v[1][0] = 0;
    peak = 0.0f;
    t->peakSlipNeg = 0.0f;
    for (i = 0; i < nNeg; i++) {
        f32 force;

        j = -(i + 1);
        k = nNeg + j;
        t->slipX[k] = -(f32)ts->curve1.v[0][i] / 255.0f;
        t->slipY[k] = force = (f32)ts->curve1.v[1][i] / 200.0f;
        if (peak < force) {
            peak = force;
            t->peakSlipNeg = t->slipX[k];
        }
        t->slipY2[k] = (f32)ts->curve1.v[2][i] / 200.0f;
    }
    peak = 0.0f;
    t->peakSlipPos = 0.0f;
    t->peakPos = 1.0f;
    for (i = 0; i < nPos; i++) {
        f32 force;

        j = i - 1;
        k = nNeg + j;
        t->slipX[k] = (f32)ts->curve2.v[0][i] / 255.0f;
        t->slipY[k] = force = (f32)ts->curve2.v[1][i] / 200.0f;
        if (peak < force) {
            peak = force;
            t->peakSlipPos = t->slipX[k];
            t->peakPos = peak;
        }
        t->slipY2[k] = (f32)ts->curve2.v[2][i] / 200.0f;
    }
    curveSet(&t->slipCurve, nNeg + nPos - 1, t->slipX, t->slipY, 0, 0);
    curveSet(&t->slipCurve2, nNeg + nPos - 1, t->slipX, t->slipY2, 0, 0);
}

// FUNC 0x001DBC50 TyreData_initAngleCurves
/* latCurve: the lateral force over the slip angle (curve0: x 1/255 of
 * maxAngle, at most 90 degrees); dragCurve: the force along the wheel
 * (curve4: x 1/255 of 90 degrees).  Both curves' first point is forced to 0. */
void TyreData_initAngleCurves(TyreData *t, CarSpecTyre *ts)
{
    int n, i;
    int deg = ts->maxAngle;
    f32 maxAngle;

    ts->curve0.x[0] = 0;
    if (deg == 0)
        deg = 90;
    maxAngle = (f32)deg * DEG2RAD;
    ts->curve0.y[0] = 0;
    if (HALF_PI < maxAngle)
        maxAngle = HALF_PI;
    n = ts->curve0Count;
    for (i = 0; i < n; i++) {
        t->latX[i] = ((f32)ts->curve0.x[i] * maxAngle) / 255.0f;
        t->latY[i] = (f32)ts->curve0.y[i] / 200.0f;
    }
    curveSet(&t->latCurve, n, t->latX, t->latY, 0, 0);

    n = ts->curve4Count;
    ts->curve4.x.v[0] = 0;
    ts->curve4.y.v[0] = 0;
    maxAngle = HALF_PI;
    for (i = 0; i < n; i++) {
        t->dragX[i] = ((f32)ts->curve4.x.v[i] * maxAngle) / 255.0f;
        t->dragY[i] = (f32)ts->curve4.y.v[i] / 200.0f;
    }
    curveSet(&t->dragCurve, n, t->dragX, t->dragY, 0, 0);
}

// FUNC 0x001DBE40 TyreData_initLoadCurve
/* The grip per surface type (the record's % x its grip, 0: the grip), the
 * wear floor surfaceGrip[11] = scale / grip (at most 1; it overwrites the
 * surface value the loop stored there), the load curve (curve3: the load x
 * 20 kgf, the first forced to 0) and the grip at rest.  `log' (a0) is not
 * used. */
void TyreData_initLoadCurve(void *log, TyreData *t, CarSpecTyre *ts, f32 scale)
{
    int i, n;
    f32 grip, wearFloor;

    n = ts->grip;                   /* one variable for the % and the count */
    if (n == 0)
        n = 100;
    grip = (f32)n * 0.01f;
    wearFloor = scale / grip;

    for (i = 0; i < 16; i++) {
        u8 pct = ts->unk_4B[i];

        if (pct == 0)
            t->surfaceGrip[i] = grip;
        else
            t->surfaceGrip[i] = (f32)pct * 0.01f * grip;
    }
    t->wearRate = (f32)ts->unk_6B * 0.01f;
    t->surfaceGrip[11] = wearFloor;
    if (1.0f < wearFloor)
        t->surfaceGrip[11] = 1.0f;
    n = ts->curve3Count;
    ts->curve3.x[0] = 0;
    for (i = 0; i < n; i++) {
        t->loadX[i] = (f32)ts->curve3.x[i] * 20.0f * GRAVITY;
        t->loadY[i] = (f32)ts->curve3.y[i] / 200.0f;
    }
    curveSet(&t->loadCurve, n, t->loadX, t->loadY, 0, 0);
    t->grip = curveLookup(&t->slipCurve2, 0.0f) * GRAVITY;
}

// FUNC 0x001DBFD8 CarDynamics_init
/* Builds the parameter block from the spec record (the spec's layout >= 5 is
 * cleared, as are the first entries of its tyre curves).  driver 2 (a
 * computer car) scales axleGrip[] by the rules' enemy grip volumes of its
 * drive class; rules->powerLimit x 10 goes to engine_setup.  cond (may be
 * NULL) goes to the top speed search.  Returns 0.  Matching notes: each
 * axle loop has its own `axle' (retail keeps them in temporaries; one
 * shared variable lives across calls in an s-register), and frontalArea is
 * read where it is used (GCSE loads it early as a halfword: retail's lhu,
 * then the sign extension at the test). */
int CarDynamics_init(CarDynamics *p, CarSpec *spec, int driver, void *log,
                     RaceRuleOptions *rules, CarCondition *cond)
{
    s8  setUsers[6];            /* the slots using each tyre set */
    int gripPct[2];
    u32 i;
    int k;
    f32 front, wheelbase, rearDist, offset;
    f64 track;

    p->unk_033 = spec->unk_02D;
    p->specialKind = spec->specialKind;     /* Lupo 2: the special kind (physics.md 2; L2 0x001DC7A4) */
    if (!(spec->layout < 5))
        spec->layout = 0;

    /* dimensions, weight distribution */
    front = (f32)spec->frontWeight * 0.01f;
    p->lenFront = (f32)spec->bodyFront * 0.001f;
    p->lenRear = (f32)spec->bodyRear * 0.001f;
    wheelbase = (f32)spec->wheelBase * 0.001f;
    p->frontRatio = front;
    p->wheelbase = wheelbase;
    rearDist = wheelbase * front;
    p->rearDist = rearDist;
    p->frontDist = wheelbase - rearDist;
    /* implicit: soft-float; The two double expressions compile to the soft-float helpers fptodp,
     * dpsub, dpmul, dpadd and dptofp (G005 / G006: F_0027AFA0, F_00279418,
     * F_00279480, F_002793C0, F_00279C50). */
    p->rearRatio = (f32)(1.0 - (f64)front);
    offset = -(p->frontDist - p->rearDist) * 0.5f;
    p->originOffset = offset;
    p->lenFront = p->lenFront - offset;
    p->lenRear = p->lenRear + offset;
    p->width = (f32)spec->width * 0.001f;
    p->halfTrack[0] = (f32)(spec->treadFront - spec->trackOffset[0] * 2) * 0.001f * 0.5f;
    p->halfTrack[1] = (f32)(spec->treadRear - spec->trackOffset[1] * 2) * 0.001f * 0.5f;
    track = (f64)(p->halfTrack[0] * front) + (f64)p->halfTrack[1] * (1.0 - (f64)front);
    p->track = (f32)(track + track);
    p->height = (f32)spec->height * 0.001f;

    /* engine, drivetrain (both get the unused log pointer as their int
     * argument) */
    engine_setup(&p->engine, spec, rules->powerLimit * 10, (int)log);
    gear_calc(spec, &p->gear, &p->engine, (int)log);
    p->driveClass = drive_class(spec->layout, spec->awdType);   /* 0x001F5EF0 */

    /* surface slots -> tyre sets (0..2 front, 3..5 rear), count the users */
    memset(setUsers, 0, 6);
    for (i = 0; i < 16; i++) {
        u8 set = spec->tyreSet[i];

        if (set >= 3)
            set = 0;
        p->tyreIndex[0][i] = set;
        p->tyreIndex[1][i] = set + 3;
        setUsers[set]++;
        setUsers[set + 3]++;
    }
    for (i = 0; i < 6; i++) {
        u32 rec;
        TyreData *t;
        CarSpecTyre *ts;
        f32 peakAngle;
        u32 tyreAxle;

        if (setUsers[i] == 0)
            continue;
        rec = i;
        if (spec->separateRearTyres == 0)
            rec = i % 3;                    /* the rear sets reuse the front records */
        t = &p->tyre[i];
        ts = &spec->tyre[rec];
        /* Lupo 2: the compound flags (physics.md 8; L2 0x001DCACC..
         * 0x001DCAE8).  0 for every GT3 compound. */
        t->slipVelocityModel = ts->slipVelocityModel;
        t->latVelocityModel = ts->latVelocityModel;
        t->noFrictionCircle = ts->noFrictionCircle;
        TyreData_initSlipCurves(t, ts);
        TyreData_initAngleCurves(t, ts);
        peakAngle = tyre_peak_slip_angle(t);
        t->peakSlipAngle = peakAngle;
        if (QUARTER_PI < peakAngle)
            t->peakSlipAngle = QUARTER_PI;
        TyreData_initLoadCurve(log, t, ts, (f32)g_002D28B0.compoundLoad[rec % 3] * 0.01f);
        tyreAxle = i / 3;
#if !ENH_PHYSICS_TYRE_REF_GRIP
        /* GT3: the grip at the static load, here, before SuspParam_init and
         * the mass set the weights it reads (a bug: 0 on a fresh block, else
         * the previous car's).  Lupo 2 does it after the mass (below). */
        {
            f32 load = tyre_load_per_width(p, tyreAxle * 2,
                                           p->sprungWeight[tyreAxle] * 0.5f
                                           + p->susp[tyreAxle].unsprungWeight);

            t->grip = t->grip * curveLookupLinear(&t->loadCurve, load);
        }
#endif
    }

    /* the camber grip curves: x = (0x80 - byte) / 8 degrees, stored
     * backwards. */
    for (int axle = 0; axle < 2; axle++) {
        int n = spec->camberPoints[axle];

        for (k = 0; k < n; k++) {
            f32 x = (f32)(0x80 - spec->camberX[axle][k]) * 0.06981317f * 0.03125f;
            f32 y = (f32)spec->camberY[axle][k] / 200.0f;

            p->camberX[axle][-(k + 1) + n] = x;
            p->camberY[axle][n - k - 1] = y;
        }
        curveSet(&p->camberCurve[axle], n, p->camberX[axle], p->camberY[axle], 0, 0);
    }

    /* tyre wear: the load range wearLo..wearHi (x 98) */
    for (int axle = 0; axle < 2; axle++) {
        f32 lo = (f32)g_002D28B0.wearLo * 98.0f;
        f32 span = (f32)g_002D28B0.wearHi * 98.0f - lo;

        p->wearBase[axle] = lo - span;
        p->wearLoad[axle] = span / (p->gear.tyreWidth[axle] * 9800.0f);
    }

    /* axle grip (+ 900 per mille), the rules' enemy volumes for a computer car */
    gripPct[0] = spec->axleGrip[0];
    gripPct[1] = spec->axleGrip[1];
    for (int axle = 0; axle < 2; axle++)
        p->axleGrip[axle] = (f32)(gripPct[axle] + 900) * 0.001f;
    if (driver == 2) {
        gripPct[0] = rules->enemyMuVolumeF[p->driveClass];
        gripPct[1] = rules->enemyMuVolumeR[p->driveClass];
        for (int axle = 0; axle < 2; axle++) {
            if (gripPct[axle] == 0)
                gripPct[axle] = 100;
            p->axleGrip[axle] *= (f32)gripPct[axle] * 0.01f;
        }
    }

    /* aerodynamics: area x g_00352920 / 2 (T329's .sdata, 0.8 in the file,
     * 1.2 once slipstream_init has run: ? the air density), the area from the
     * spec (x 1/10000 m^2) or width x height */
    {
        f32 areaCoef, dragShare, liftF, liftR;

        if (spec->frontalArea == 0)
            areaCoef = p->width * p->height * g_00352920 * 0.5f;
        else {
            areaCoef = (f32)spec->frontalArea / 10000.0f;
            areaCoef *= g_00352920 * 0.5f;
        }
        dragShare = (f32)spec->dragShare * 0.01f;
        p->unk_E5C = dragShare;
        p->dragCoef = areaCoef * dragShare;
        liftF = areaCoef * ((f32)spec->aeroF * 0.01f);
        p->liftCoef[0] = liftF;
        liftR = areaCoef * ((f32)spec->aeroR * 0.01f);
        p->liftCoef[1] = liftR;
        p->dragCoef = areaCoef * dragShare + (liftF + liftR) * 0.1f;
    }

    /* brakes */
    for (int axle = 0; axle < 2; axle++) {
        u8 raw = spec->brakeTorque[axle];
        f32 torque = (f32)(raw * 10) * GRAVITY;

        p->brakeTorque[axle] = torque;
        if (raw == 0)
            p->absGain[axle] = 0.0f;
        else
            p->absGain[axle] = p->gear.tyreRadius[axle] / torque;
    }
    p->handbrakeTorque = (f32)(spec->brakeTorque[2] * 10) * GRAVITY;
    for (int axle = 0; axle < 2; axle++) {
        p->absK[axle] = (f32)spec->brakeBalance[axle] * 0.1f;
        p->brakeK[axle] = (f32)spec->brakeK[axle] * 0.01f;
    }

    /* driving aids: ascFlags bits 0..7 are the eight enable bytes; the code
     * reads each group of four as one word (low half brake, high half ABS) */
    {
        TcsParams *tcs = &p->tcs;
        u8 flags = spec->ascFlags;
        u32 word;

        tcs->brakeGain = (f32)spec->asc0 * 0.01f;
        tcs->brakeSlip = (f32)spec->brakeSlip * 0.01f;
        tcs->absGain = (f32)spec->asc1 * 0.01f;
        if (flags & 1)
            tcs->brakeFront = 1;
        else
            tcs->brakeFront = 0;
        tcs->brakeRear = (flags >> 1) & 1;
        tcs->absFront = (flags >> 2) & 1;
        tcs->absRear = (flags >> 3) & 1;
        word = *(u32 *)&tcs->brakeFront;
        tcs->brakeOn = ((word & 0xFFFF) && tcs->brakeGain != 0.0f) ? 1 : 0;
        tcs->absOn = ((word & 0xFFFF0000) && tcs->absGain != 0.0f) ? 1 : 0;
        tcs->brakeGain2 = (f32)spec->asc2 * 0.01f;
        tcs->brakeSlip2 = (f32)spec->brakeSlip2 * 0.01f;
        tcs->absGain2 = (f32)spec->asc3 * 0.01f;
        tcs->brakeFront2 = (flags >> 4) & 1;
        tcs->brakeRear2 = (flags >> 5) & 1;
        tcs->absFront2 = (flags >> 6) & 1;
        tcs->absRear2 = flags >> 7;
        word = *(u32 *)&tcs->brakeFront2;
        tcs->brakeOn2 = ((word & 0xFFFF) && tcs->brakeGain2 != 0.0f) ? 1 : 0;
        tcs->absOn2 = ((word & 0xFFFF0000) && tcs->absGain2 != 0.0f) ? 1 : 0;
        tcs->fadeGain = (f32)spec->fadeGain * 0.1f;
        tcs->fadeStart = (f32)spec->fadeStart * 0.01f;
        tcs->throttleGain = (f32)spec->tcsThrottle * 0.1f;
        if (spec->tcs == 100 || spec->tcs == 0)
            tcs->slipTarget = 1.0f;
        else
            tcs->slipTarget = (f32)spec->tcs * 0.01f;
    }

    /* steering: the lock and its three curves over the speed */
    p->steerLock = (f32)spec->steerLock * DEG2RAD;
    curve_setup_kmh_deg(&p->steer0, p->steer0X, p->steer0Y,
                        spec->steer[0].count, spec->steer[0].c.x.v, spec->steer[0].c.y.v);
    curve_setup_kmh_deg(&p->steer1, p->steer1X, p->steer1Y,
                        spec->steer[2].count, spec->steer[2].c.x.v, spec->steer[2].c.y.v);
    curve_setup_kmh_deg(&p->steer2, p->steer2X, p->steer2Y,
                        spec->steer[1].count, spec->steer[1].c.x.v, spec->steer[1].c.y.v);
    {
        f32 wb = p->wheelbase;
        f32 halfTrack = p->halfTrack[0];

        p->steerGain[0] = atan2f(wb, wb - halfTrack) / QUARTER_PI;
        p->steerGain[1] = atan2f(wb, wb + halfTrack) / QUARTER_PI;
    }
    if (spec->caster == 0)
        p->casterCamber = 0.0f;
    else
        p->casterCamber = sinf((f32)spec->caster * DEG2RAD);

    /* wheel alignment */
    for (int axle = 0; axle < 2; axle++) {
        p->toe[axle] = atan2f((f32)(spec->toe[axle] - 0x80) * 0.5f * 0.001f,
                              p->gear.tyreRadius[axle]);
        p->camber[axle] = -((f32)spec->camber[axle] * 0.1f * DEG2RAD);
        p->camberGain[axle] = (f32)spec->camberGain[axle] * DEG2RAD;
        p->camberGrip[axle] = curveLookup(&p->camberCurve[axle], p->camber[axle]);
        p->swingArm[axle] = (f32)spec->swingArm[axle] * 0.01f;
    }

    {
        f32 yawI, pitchI, rollI;

        /* inertias (x 100; 0: the defaults) */
        if (spec->yawInertia == 0)
            yawI = 6000.0f;
        else
            yawI = (f32)spec->yawInertia * 100.0f;
        p->yawInertia = yawI;
        p->yawInertiaInv = 1.0f / yawI;
        if (spec->pitchInertia == 0)
            pitchI = 4000.0f;
        else
            pitchI = (f32)spec->pitchInertia * 100.0f;
        p->pitchFront = (p->frontDist * TWO_PI) / pitchI;
        p->pitchRear = (p->rearDist * TWO_PI) / pitchI;
        if (spec->rollInertia == 0)
            rollI = 2000.0f;
        else
            rollI = (f32)spec->rollInertia * 100.0f;
        p->rollFront = (p->halfTrack[0] * TWO_PI) / rollI;
        p->rollRear = (p->halfTrack[1] * TWO_PI) / rollI;

        /* suspension */
        SuspParam_init(&p->susp[0], &spec->susp[0], spec->suspRate[0], log);
        SuspParam_init(&p->susp[1], &spec->susp[1], spec->suspRate[1], log);

        /* centre of gravity, mass */
        {
            f32 fr = p->frontRatio;
            f32 rr = p->rearRatio;
            f32 cg, lever, pitchK, rollK;

            p->axleHeight[0] = (f32)spec->rideHeight[0] * 0.001f;
#if ENH_PHYSICS_REAR_RIDE_HEIGHT
            p->axleHeight[1] = (f32)spec->rideHeight[1] * 0.001f;  /* Lupo 2: the rear's own (physics.md 6; L2 0x001DD4C4) */
#else
            p->axleHeight[1] = (f32)spec->rideHeight[0] * 0.001f;  /* rideHeight[0] for both */
#endif
            cg = p->axleHeight[0] * fr + p->axleHeight[1] * rr;
            p->cgHeight = cg;
            lever = ((f32)spec->cgOffset * 0.001f + cg) * 0.5f;
            rollK = lever / p->track;
            pitchK = lever / p->wheelbase;
            p->longTransfer = pitchK;
            p->latTransfer = rollK;
            if (spec->weight != 0) {
                f32 mass = (f32)spec->weight;
                f32 sprung;

                p->mass = mass;
                if (drivetrain_is_hybrid(spec))
                    /* Lupo 2: the Dualnote leaves out the fuel term (L2
                     * 0x001DD534..0x001DD558: GearCalc.driveType 7 there) */
                    mass = mass + (f32)g_002D28B0.driverMass;
                else
                    mass = mass + (f32)(g_002D28B0.driverMass + g_002D28B0.fuelMass);
                sprung = mass - 2.0f * (p->susp[0].unsprungMass + p->susp[1].unsprungMass);
                p->mass = mass;
                p->weight = mass * GRAVITY;
                p->invMass = 1.0f / mass;
                if (!(sprung <= 0.0f)) {
                    f32 sprungWeight = sprung * GRAVITY;

                    p->latTransfer = sprung * rollK;
                    p->invSprungMass = 1.0f / sprung;
                    p->sprungWeight[0] = sprungWeight * fr;
                    p->sprungWeight[1] = sprungWeight * rr;
                    p->longTransfer = (sprung / mass) * pitchK;
                }
            }
        }
        {
            f32 mass = p->mass;
            f32 frontDist = p->frontDist;
            f32 rearDist2 = p->rearDist;
            int gain = g_002D28B0.unk_07;
            f32 k;

            /* Lupo 2: kind 7 (the SLs) without the active suspension has no
             * drive / brake pitch moment (L2 0x001DD68C..0x001DD6A8; the
             * suspension is on as shipped, so this does not run). */
            if (spec->specialKind == CAR_KIND_SL && !sl_active_suspension_on())
                gain = 0;
            k = (f32)gain * 0.01f * TWO_PI;
            p->unk_130[0] = k / (pitchI + mass * frontDist * frontDist);
            p->unk_130[1] = k / (pitchI + mass * rearDist2 * rearDist2);
        }
    }
#if ENH_PHYSICS_TYRE_REF_GRIP
    /* Lupo 2: the tyres' grip at rest x the load curve at the real static
     * load, now that the suspension and the mass are set (physics.md 7; L2
     * 0x001DD718..0x001DD7AC).  It normalises the lateral grip use, the aids'
     * slip estimate and the AI's corner speed. */
    for (i = 0; i < 6; i++) {
        TyreData *t = &p->tyre[i];
        u32 tyreAxle = i / 3;
        f32 load;

        if (setUsers[i] == 0)
            continue;
        load = tyre_load_per_width(p, tyreAxle * 2,
                                   p->sprungWeight[tyreAxle] * 0.5f + p->susp[tyreAxle].unsprungWeight);
        t->grip = t->grip * curveLookupLinear(&t->loadCurve, load);
    }
#endif
    for (int axle = 0; axle < 2; axle++)
        SuspParam_initStroke(p, &p->susp[axle], spec, axle, log);
    CarDynamics_initTopSpeed(p, cond, log);
    return 0;
}
