/* T282 - SetupCarSpec: the CarSpec record of a CarEquipments, with the
 * defaults of the fields the car data leaves at 0.  Its callers pass their
 * own name as `caller' ("ShowRoomCar::SetupShowRoomCar()",
 * "DynamicsConductor::initialize()", ...), which is not used. */
#include "game/car/physics/car_spec_internal.h"
#include "game/car/physics/T282_car_spec.h"

/* fitTyres (0 from every caller in this build): first fit the car's tyres
 * of the course to equip: the dirt tyres on a dirt course, none on course
 * kind 1, else TIRE_KIND_ARCADE_RACING if tyreFlag. */
// FUNC 0x001DA308 SetupCarSpec
void SetupCarSpec(CarSpec *spec, CarEquipments *equip, int tyreFlag, int courseKind,
                  int fitTyres, const char *caller)
{
    CarSpecTyre *first;
    u32          i, j;                          /* sltiu */

    memset(spec, 0, sizeof(CarSpec));
    if (fitTyres != 0) {
        int kind = -1;

        if (courseKind == COURSE_KIND_DIRT)
            kind = TIRE_KIND_DIRT;
        else if (courseKind != COURSE_KIND_1)
            kind = (tyreFlag != 0) ? TIRE_KIND_ARCADE_RACING : -1;
        if (kind != -1) {
            u64 code = equip->findPart(CARDB_FRONT_TIRE, kind);
            if (code != 0)
                equip->frontTire = code;
            code = equip->findPart(CARDB_REAR_TIRE, kind);
            if (code != 0)
                equip->rearTire = code;
        }
    }
    equip->getSpec(spec);
    if (spec->bodyFront == 0)
        spec->bodyFront = spec->length / 2;
    if (spec->bodyRear == 0)
        spec->bodyRear = spec->length / 2;
    spec->unk_02D = equip->isRacingModified();
    spec->unk_010 = 2;
    {
        /* The default steering curves (local arrays from .sdata 0x00352900 /
         * 0x00352908, initialised here; the tyre block curve is g_00352910 /
         * g_00352918). */
        Curve6Row steerX = { { 0x00, 0x20, 0x4C, 0x70, 0xA6, 0xFF } };
        Curve6Row steerY = { { 0xFF, 0xCA, 0x8A, 0x5E, 0x3E, 0x2F } };

        if (spec->steer[2].count == 0) {
            spec->steer[2].count = 6;
            spec->steer[2].c.x = steerX;        /* 6-byte record copies */
            spec->steer[2].c.y = steerY;
        }
        if (spec->steer[1].c.x.v[5] == 0) {      /* (sic) its last x, not its count */
            spec->steer[1].count = 6;
            spec->steer[1].c.x = steerX;
            spec->steer[1].c.y = steerY;
        }
    }
    if (courseKind == COURSE_KIND_DIRT) {      /* no driving aids on dirt */
        spec->fadeGain = 0;
        spec->fadeStart = 0;
        spec->tcsThrottle = 0;
        spec->ascFlags = 0;
        spec->asc0 = 0;
        spec->brakeSlip = 0;
        spec->asc1 = 0;
        spec->asc2 = 0;
        spec->brakeSlip2 = 0;
        spec->asc3 = 0;
    }
    if (spec->brakeK[0] == 0)
        spec->brakeK[0] = 100;
    if (spec->brakeK[1] == 0)
        spec->brakeK[1] = 100;
    if (spec->tcs == 100)
        spec->tcsThrottle = 0;
    spec->separateRearTyres = 1;
    {
        /* The tyre compound of each surface slot (CarDynamics.tyreIndex;
         * local array from .rodata 0x0033C368), copied byte by byte
         * (retail's pointer loop counting down). */
        u8  slotCompound[16] = { 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 2, 2, 2, 2 };
        u8 *d = spec->tyreSet;
        u8 *s = slotCompound;
        int n = 16;

        while (n--)
            *d++ = *s++;
    }

    first = &spec->tyre[0];
    if (first->unk_4B[12] == 0)
        first->unk_4B[12] = first->unk_4B[0];
    if (first->unk_4B[13] == 0)
        first->unk_4B[13] = first->unk_4B[1];
    if (first->unk_4B[14] == 0)
        first->unk_4B[14] = first->unk_4B[9];
    for (i = 0; i < 6; i++) {
        CarSpecTyre *t = &spec->tyre[i];

        if (t->maxAngle == 0)
            t->maxAngle = 45;
        if (t->curve4Count == 0) {
            t->curve4Count = 6;
            for (j = 0; j < 6; j++) {
                t->curve4.x.v[j] = g_00352910[j];
                t->curve4.y.v[j] = g_00352918[j];
            }
        }
        for (j = 0; j < 16; j++) {
            if (t->unk_4B[j] == 0)
                t->unk_4B[j] = spec->tyre[0].unk_4B[j];
        }
    }
}

/* The default curve of the tyre blocks (CarSpecTyre.curve4: x, then y).
 * Defined after SetupCarSpec: retail's .sdata has the function's two local
 * steering curves (0x00352900 / 0x00352908) first. */
u8 g_00352910[6] = { 0x00, 0x1F, 0x3E, 0x69, 0xA5, 0xFF };   /* 0x00352910 */
u8 g_00352918[6] = { 0x00, 0x1C, 0x3D, 0x72, 0xB4, 0xC4 };   /* 0x00352918 */
