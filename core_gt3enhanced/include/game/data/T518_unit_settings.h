/* T518_unit_settings.h - UnitSettings, the units the game shows (translation unit
 * T518, unit u112), 0x002457C8..0x00245C10.
 *
 * Speed and distance (metric / imperial), power and torque; their names and
 * the conversions from the internal units (m/s, PS, kgf.m, metres).  The one
 * instance is g_002F59D8; the US build starts with preset 2 (mph, HP, ft.lb). */
#ifndef GAME_T518_UNIT_SETTINGS_H
#define GAME_T518_UNIT_SETTINGS_H

#include "game/game.h"

/* UnitSettings::system / power / torque. */
enum { UNIT_METRIC = 0, UNIT_IMPERIAL = 1 };
enum { UNIT_POWER_PS = 0, UNIT_POWER_HP = 1, UNIT_POWER_BHP = 2, UNIT_POWER_CV = 3, UNIT_POWER_KW = 4 };
enum { UNIT_TORQUE_KGFM = 0, UNIT_TORQUE_FTLB = 1, UNIT_TORQUE_NM = 2 };

/* Not polymorphic. */
class UnitSettings {
public:
    UnitSettings();                             /* 0x002457C8 setPreset(2) */
    ~UnitSettings();                            /* 0x002457F8 */
    /* 1, 5: km/h PS kgf.m; 2: mph HP ft.lb; 3, 6, 7: km/h BHP kgf.m;
     * 4: km/h CV kgf.m; 0, else: km/h kW NM */
    void setPreset(u32 preset);                 /* 0x00245820 */
    const char *speedName();                    /* 0x002458A0 */
    const char *powerName();                    /* 0x002458D8 */
    const char *torqueName();                   /* 0x00245948 */
    const char *distanceName();                 /* 0x002459A8 */
    const char *shortDistanceName();            /* 0x002459E0 "m" metric, "miles" imperial */
    f32  convSpeed(f32 mps);                    /* 0x00245A18 m/s -> km/h or mph */
    f32  convPower(f32 ps);                     /* 0x00245A58 PS -> HP / BHP / CV, kW */
    f32  convTorque(f32 kgfm);                  /* 0x00245AB0 kgf.m -> ft.lb, NM */
    f32  convDistance(f32 metres);              /* 0x00245B08 metres -> km or miles */
    /* v in fromSystem's unit -> this system's (km <-> miles), rounded */
    int  convertDistance(int v, int fromSystem);    /* 0x00245B48 */

    int system;                                 /* 0x0 speed and distance: 0 metric (km/h, km, m), 1 imperial
                                                   (mph, miles) */
    int power;                                  /* 0x4 0 PS, 1 HP, 2 BHP, 3 CV, 4 kW */
    int torque;                                 /* 0x8 0 kgf.m, 1 ft.lb, 2 NM */
};
EGCS_SIZE(UnitSettings, 0xC);

extern UnitSettings g_002F59D8;                 /* 0x002F59D8 the unit settings */

/* T518 defines g_002F59D8; egcs generates its static destructor
 * _GLOBAL_$D$g_002F59D8 0x00245BC8 (.dtors 0x0028F288, ~UnitSettings) and
 * initialiser _GLOBAL_$I$g_002F59D8 0x00245BF0 (.ctors 0x0028F1B8, UnitSettings()). */

#endif /* GAME_T518_UNIT_SETTINGS_H */
