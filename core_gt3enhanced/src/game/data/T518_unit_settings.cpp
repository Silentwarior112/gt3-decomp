/* T518 - UnitSettings: the units the game shows (g_002F59D8): speed and
 * distance (metric / imperial), power, torque; their names and the
 * conversions from the internal units (m/s, PS, kgf.m, metres).  The US
 * build starts with preset 2 (mph, HP, ft.lb). */
#include "engine/gfx/vram_internal.h"
#include "game/data/T518_unit_settings.h"

/* The one instance, defined first as in T516 (egcs names the static
 * initialiser / destructor after it); .data, zero-filled (a public object
 * with a constructor). */
UnitSettings g_002F59D8;                        /* 0x002F59D8 */

/* The conversion factors, as the binary has them (one .sdata float per use,
 * 0x00350A10..0x00350A3C). */
#define KMH_PER_MPS   3.6f
#define MILES_PER_KM  0.62139f
#define KM_PER_MILE   1.609f
#define HP_PER_PS     0.98619f              /* HP, BHP and CV alike */
#define KW_PER_PS     0.7355f
#define FTLB_PER_KGFM 7.2329f
#define NM_PER_KGFM   9.8f
#define KM_PER_M      0.001f
#define MILES_PER_M   0.00062139f

// FUNC 0x002457C8 UnitSettings::UnitSettings
UnitSettings::UnitSettings()
{
    setPreset(2);
}

// FUNC 0x002457F8 UnitSettings::~UnitSettings
//   Nothing; egcs's deleting form calls operator delete when (flags & 1).
UnitSettings::~UnitSettings()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x00245820 UnitSettings::setPreset
//   The units of a preset (jump table 0x00343CB0, unsigned < 8):
//     1, 5     km/h, PS,  kgf.m
//     2        mph,  HP,  ft.lb
//     3, 6, 7  km/h, BHP, kgf.m
//     4        km/h, CV,  kgf.m
//     0, else  km/h, kW,  NM
void UnitSettings::setPreset(u32 preset)
{
    switch (preset) {
    case 1:
    case 5:
        system = UNIT_METRIC;
        power = UNIT_POWER_PS;
        torque = UNIT_TORQUE_KGFM;
        break;
    case 2:
        system = UNIT_IMPERIAL;
        power = UNIT_POWER_HP;
        torque = UNIT_TORQUE_FTLB;
        break;
    case 3:
    case 6:
    case 7:
        system = UNIT_METRIC;
        power = UNIT_POWER_BHP;
        torque = UNIT_TORQUE_KGFM;
        break;
    case 4:
        system = UNIT_METRIC;
        power = UNIT_POWER_CV;
        torque = UNIT_TORQUE_KGFM;
        break;
    case 0:
    default:
        system = UNIT_METRIC;
        power = UNIT_POWER_KW;
        torque = UNIT_TORQUE_NM;
        break;
    }
}

// FUNC 0x002458A0 UnitSettings::speedName
const char *UnitSettings::speedName()
{
    switch (system) {
    case UNIT_METRIC:
        return "km/h";
    case UNIT_IMPERIAL:
        return "mph";
    default:
        return "VVV";
    }
}

// FUNC 0x002458D8 UnitSettings::powerName
//   Jump table 0x00343CD0 (unsigned < 5).
const char *UnitSettings::powerName()
{
    switch ((u32)power) {
    case UNIT_POWER_PS:
        return "PS";
    case UNIT_POWER_HP:
        return "HP";
    case UNIT_POWER_BHP:
        return "BHP";
    case UNIT_POWER_CV:
        return "CV";
    case UNIT_POWER_KW:
        return "kW";
    default:
        return "PPP";
    }
}

// FUNC 0x00245948 UnitSettings::torqueName
const char *UnitSettings::torqueName()
{
    switch (torque) {
    case UNIT_TORQUE_KGFM:
        return "kgf.m";
    case UNIT_TORQUE_FTLB:
        return "ft.lb";
    case UNIT_TORQUE_NM:
        return "NM";
    default:
        return "TTT";
    }
}

// FUNC 0x002459A8 UnitSettings::distanceName
const char *UnitSettings::distanceName()
{
    switch (system) {
    case UNIT_METRIC:
        return "km";
    case UNIT_IMPERIAL:
        return "miles";
    default:
        return "DDD";
    }
}

// FUNC 0x002459E0 UnitSettings::shortDistanceName
//   "m" metric, but "miles" (not feet) imperial (as in the binary).
const char *UnitSettings::shortDistanceName()
{
    switch (system) {
    case UNIT_METRIC:
        return "m";
    case UNIT_IMPERIAL:
        return "miles";
    default:
        return "DDD";
    }
}

// FUNC 0x00245A18 UnitSettings::convSpeed
//   m/s -> km/h or mph (two multiplies); unchanged otherwise.
f32 UnitSettings::convSpeed(f32 mps)
{
    switch (system) {
    case UNIT_METRIC:
        return mps * KMH_PER_MPS;
    case UNIT_IMPERIAL:
        return mps * KMH_PER_MPS * MILES_PER_KM;
    default:
        return mps;
    }
}

// FUNC 0x00245A58 UnitSettings::convPower
//   PS -> HP / BHP / CV, kW; PS and other values unchanged.
f32 UnitSettings::convPower(f32 ps)
{
    switch (power) {
    case UNIT_POWER_PS:
        return ps;
    case UNIT_POWER_HP:
    case UNIT_POWER_BHP:
    case UNIT_POWER_CV:
        return ps * HP_PER_PS;
    case UNIT_POWER_KW:
        return ps * KW_PER_PS;
    default:
        return ps;
    }
}

// FUNC 0x00245AB0 UnitSettings::convTorque
//   kgf.m -> ft.lb, NM; kgf.m and other values unchanged.
f32 UnitSettings::convTorque(f32 kgfm)
{
    switch (torque) {
    case UNIT_TORQUE_KGFM:
        return kgfm;
    case UNIT_TORQUE_FTLB:
        return kgfm * FTLB_PER_KGFM;
    case UNIT_TORQUE_NM:
        return kgfm * NM_PER_KGFM;
    default:
        return kgfm;
    }
}

// FUNC 0x00245B08 UnitSettings::convDistance
//   Metres -> km or miles; km for other values.  No callers.
f32 UnitSettings::convDistance(f32 metres)
{
    switch (system) {
    case UNIT_METRIC:
        return metres * KM_PER_M;
    case UNIT_IMPERIAL:
        return metres * MILES_PER_M;
    default:
        return metres * KM_PER_M;
    }
}

// FUNC 0x00245B48 UnitSettings::convertDistance
//   v given in fromSystem's unit -> this system's (km <-> miles), rounded
//   (+ 0.5, truncated).  Metric: from metric unchanged, else miles -> km;
//   imperial: from imperial unchanged, else km -> miles.
int UnitSettings::convertDistance(int v, int fromSystem)
{
    if (system == UNIT_METRIC) {
        if (fromSystem == UNIT_METRIC)
            return v;
        return (int)((f32)v * KM_PER_MILE + 0.5f);
    }
    if (fromSystem == UNIT_IMPERIAL)
        return v;
    return (int)((f32)v * MILES_PER_KM + 0.5f);
}

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002F59D8 (.dtors entry 0x0028F288, .ctors entry 0x0028F1B8), emitted at
 * the end of the unit, the destructor first: ~UnitSettings(2) /
 * UnitSettings() on it.  Named after the unit's first global, g_002F59D8. */
// FUNC 0x00245BC8 T518_static_dtor (_GLOBAL_$D$g_002F59D8)
/* implicit: ~UnitSettings (the static destructor of g_002F59D8) */

// FUNC 0x00245BF0 T518_static_init (_GLOBAL_$I$g_002F59D8)
/* implicit: UnitSettings (the static initialiser of g_002F59D8) */
