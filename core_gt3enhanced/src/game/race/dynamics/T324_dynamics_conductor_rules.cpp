/* T324 - DynamicsConductor slots 34..40: the base answers of the rule
 * queries, the course-out rule (slot 39) and the collision rule (F_001DA130,
 * the License / TimeTrial slot 40).  A car's fault byte (DynCarState.fault)
 * gets FAULT_COLLISION or FAULT_COURSE_OUT.
 *
 * Retail T324's .rodata (0x0033C340..0x0033C368, referenced by no code):
 * <new>'s "bad_alloc", then UnitArena<64>'s type name (the race course,
 * seen through RaceCarState / DynamicsConductor), the pair every dynamics
 * file that includes the race headers has; T279 before it and T282 after it
 * have neither. */
// DATA_START 0x0033C340
#include <new>                                  /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO                /* UnitArena<64>'s type name */
#include "game/car/physics/car_spec_internal.h"
#include "game/race/dynamics/T324_dynamics_conductor_rules.h"

// FUNC 0x001D9FB8 DynamicsConductor::virtual_34
int DynamicsConductor::virtual_34()
{
    return 0;
}

// FUNC 0x001D9FC0 DynamicsConductor::virtual_35
int DynamicsConductor::virtual_35()
{
    return 0;
}

/* The course-out level of all four wheels on counted ground (COURSE_OUT_*;
 * chargePenaltyBySurface's countedWheels, inside the pit lane). */
// FUNC 0x001D9FC8 DynamicsConductor::virtual_36
int DynamicsConductor::virtual_36()
{
    return COURSE_OUT_NONE;
}

/* The course-out level of all four wheels off the course. */
// FUNC 0x001D9FD0 DynamicsConductor::virtual_37
int DynamicsConductor::virtual_37()
{
    return COURSE_OUT_NONE;
}

// FUNC 0x001D9FD8 DynamicsConductor::isPitEnabled
int DynamicsConductor::isPitEnabled()
{
    return 1;
}

/* The car's conductor's slot 34; 0 when the car is not driven by the pad
 * (DynCarState.driver != 0).  car_tyre_check (T296) compares it with 4. */
// FUNC 0x001D9FE0 F_001D9FE0
int F_001D9FE0(RaceCarState *car)
{
    if (car->st.driver != 0)
        return 0;
    return car->dc->virtual_34();
}

/* countedWheels / offCourseWheels: the wheels (0..4) on ground whose
 * RunwayContact.attr2 bit 0 is set and on an off-course surface
 * (car_ground_contact, T298).  All four on counted ground count only inside
 * the pit lane (pit_lane_contains, margin 0) at the level of slot 36, all
 * four off the course at the level of slot 37; the higher level applies,
 * and any count marks the car's course out. */
// FUNC 0x001DA020 DynamicsConductor::chargePenaltyBySurface
void DynamicsConductor::chargePenaltyBySurface(RaceCarState *car, int countedWheels, int offCourseWheels)
{
    DynCarState *st = &car->st;
    int level;

    if (st->fault != 0)
        return;
    if (st->driver != 0)
        return;
    level = COURSE_OUT_NONE;
    if (countedWheels == WHEELS_ALL) {
        if (pit_lane_contains(&lines, st->lapDistance, 0) != 0) {
            int countedLevel = virtual_36();
            if (level < countedLevel)
                level = countedLevel;
        }
    }
    if (offCourseWheels == WHEELS_ALL) {
        int offCourseLevel = virtual_37();
        if (level < offCourseLevel)
            level = offCourseLevel;
    }
    switch (level) {
    case COURSE_OUT_AT_ONCE:
        st->courseOut = 0xFF;
        break;
    case COURSE_OUT_COUNT:
        st->courseOut++;
        break;
    default:
        st->courseOut = 0;
        break;
    }
    if (st->courseOut != 0)
        st->fault = FAULT_COURSE_OUT;
}

// FUNC 0x001DA128 DynamicsConductor::issueCrashEvent
void DynamicsConductor::issueCrashEvent(RaceCarState *car, int kind, int arg, f32 force)
{
}

/* Only car 0, and only before it finished: a wall hit is a fault when force
 * x invMass (the speed change) is above 10, a course object always is,
 * another car never.  arg is not read. */
// FUNC 0x001DA130 DynamicsConductor::F_001DA130
void DynamicsConductor::F_001DA130(RaceCarState *car, int kind, int arg, f32 force)
{
    DynCarState *st = &car->st;
    int fault;

    if (st->carIndex != 0)
        return;
    if (st->finished != 0)
        return;
    fault = 0;
    switch (kind) {
    case CRASH_KIND_WALL:
        force *= car->params.invMass;           /* the speed change */
        if (force > 10.0f)
            fault = 1;
        break;
    case CRASH_KIND_CAR:
        break;
    case CRASH_KIND_GADGET:
        fault = 1;
        break;
    }
    if (fault != 0)
        st->fault |= FAULT_COLLISION;
}
