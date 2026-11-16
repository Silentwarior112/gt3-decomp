/* T361 - the course drive parameters: the 0x20-byte CourseDriveParam record
 * of a drive class, from the course data (its +0x94 table, indexed by the
 * drive class drive_class gives a car spec) or, when the course has none,
 * the default record of the course kind (g_002D9A08).  The computer
 * driver's parameters are made from it (ai_load_params, T362); record 0 also
 * gives DynamicsConductor's start type 3 (courseStart).  Declared in
 * T361_drive_params.h. */
/* Retail T361's .rodata (0x0033DC48): <new>'s "bad_alloc" and the type name
 * "t9UnitArena1Ui64" (nothing references them: DATA_START).  Its .data: the
 * two default tables (0x002D9988, 0x002D9A08) before the frame info. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/control_record_internal.h"
#include "game/race/dynamics/T361_drive_params.h"
// DATA_START 0x0033DC48

/* The records' fields in CourseDriveParam's order (T361_drive_params.h):
 * brake, cornerScale, cornerOffset, steerGain, slideThrottle, slideBrake,
 * lookSpeed, lookNear, lookFar, brakeGain, brakeSlip, fadeGain, fadeStart,
 * throttleGain, gripBalance, cornerBonus, steerRate, bonusDelay, pitSpeed,
 * assistFlags; the rest 0. */

/* Four default records (one per drive class?), all equal to
 * g_002D9A08[0].  Nothing in the program reads them (no code or data
 * reference); retail's T361 has them in front of g_002D9A08. */
CourseDriveParam g_002D9988[4] = {          /* 0x002D9988 */
    { 100, 100, 100, 24, 100, 0, 90, 15, 40, 15, 5, 10, 0, 60, 100, 20, 9, 0, 80, 0 },
    { 100, 100, 100, 24, 100, 0, 90, 15, 40, 15, 5, 10, 0, 60, 100, 20, 9, 0, 80, 0 },
    { 100, 100, 100, 24, 100, 0, 90, 15, 40, 15, 5, 10, 0, 60, 100, 20, 9, 0, 80, 0 },
    { 100, 100, 100, 24, 100, 0, 90, 15, 40, 15, 5, 10, 0, 60, 100, 20, 9, 0, 80, 0 },
};   /* 0x002D9988 */

/* The default record of a course kind (0, 1, 2: dirt) for a course whose
 * data has none (getDriveParam); dirt looks further ahead (30 / 60 m) with
 * no brake gain / slip. */
CourseDriveParam g_002D9A08[3] = {          /* 0x002D9A08 */
    { 100, 100, 100, 24, 100, 0, 90, 15, 40, 15, 5, 10, 0, 60, 100, 20, 9, 0, 80, 0 },
    { 100, 100, 100, 24, 100, 0, 90, 15, 40, 15, 5, 10, 0, 60, 100, 20, 9, 0, 80, 0 },
    { 100, 100, 100, 24, 100, 0, 90, 30, 60,  0, 0, 10, 0, 60, 100, 20, 9, 0, 80, 0 },
};   /* 0x002D9A08 */

// FUNC 0x001F5EF0 drive_class
/* CarDynamics_init: CarDynamics.driveClass = drive_class(spec layout,
 * awdType).  1 FF and the 4WD of awdType 0; 3 the 4WD of awdType 3; 2 the
 * other 4WD; 0 the rest (FR, MR, RR). */
int drive_class(int layout, int awdType)
{
    int cls = 0;

    switch (layout) {
    case LAYOUT_FF:
        cls = 1;
        break;
    case LAYOUT_4WD:
        switch (awdType) {
        case 0:
            cls = 1;
            break;
        case 3:
            cls = 3;
            break;
        default:
            cls = 2;
            break;
        }
        break;
    }
    return cls;
}

// FUNC 0x001F5F38 getDriveParam
/* The course's record [idx] (idx >= 4 is taken as 0); without one the
 * default of the course kind: record 1 / 2 for kind 1 / 2, record 0 for
 * any other kind.  The switch names the four kinds 0..3 (retail's case
 * tree: == 1, < 2, == 2; with only cases 1 and 2 egcs tests == 1, == 2). */
CourseDriveParam *getDriveParam(DynamicsConductor *dc, int kind, u32 idx)
{
    CourseDriveParam *p;

    if (idx >= 4)
        idx = 0;
    p = dc->course->data->driveParam[idx];
    if (p == 0) {
        int def = 0;
        switch (kind) {
        case 0:
            break;
        case COURSE_KIND_1:
            def = 1;
            break;
        case COURSE_KIND_DIRT:
            def = 2;
            break;
        case 3:                             /* ? (no name) */
            break;
        }
        p = &g_002D9A08[def];
    }
    return p;
}

// FUNC 0x001F5FB8 ai_drive_param
/* For ai_load_params (T362): the record of the driver's conductor. */
CourseDriveParam *ai_drive_param(AIDriver *ai, int kind, u32 idx)
{
    return getDriveParam(ai->conductor, kind, idx);
}
