/* T455_car_model_info.h - CarModelInfo, the car's description in its model
 * file (translation unit T455, unit u102).
 *
 * CarModelFile +0x04 (T456).  The tables are stored as offsets relative to
 * `base'; carmodelinfo_relocate turns each into a pointer when its count is
 * not zero (the tyre table when its offset is not zero).
 */
#ifndef GAME_T455_CAR_MODEL_INFO_H
#define GAME_T455_CAR_MODEL_INFO_H

#include "game/game.h"

struct CarLamp;                             /* T159_flare.h */
struct CameraMount;                             /* u089: an onboard camera position */

/* ? name (u060).  The tyre record of an axle, 0x14 bytes: RaceCarModel::
 * setupTires (u060) passes (b, a, c, d, e) to the axle's two RaceTireModels. */
typedef struct CarAxleSpec {
    int a;                                      /* 0x00 */
    int b;                                      /* 0x04 */
    f32 c;                                      /* 0x08 */
    f32 d;                                      /* 0x0C */
    f32 e;                                      /* 0x10 degrees */
} CarAxleSpec;
EGCS_SIZE(CarAxleSpec, 0x14);

typedef struct CarModelInfo {
    u8                   unk_00[4];             /* 0x00 */
    struct CarModelInfo *base;                  /* 0x04 the address the offsets are for */
    u8                   unk_08[0xC];           /* 0x08 */
    CarAxleSpec          axles[2];              /* 0x14 front, rear */
    int                  defaultTire;           /* 0x3C carmodelinfo_tire's "none" */
    /* The lamps (T180 RaceCarModel::addLamps / drawLampReflections).  List C
     * is switched: a lamp with flags bit 0 is lit at the body's lamp state
     * fraction, the others at 1.0 when the lamps are on. */
    int                  lampCountA;            /* 0x40 */
    struct CarLamp  *lampsA;                /* 0x44 always lit, directional */
    int                  lampCountB;            /* 0x48 */
    struct CarLamp  *lampsB;                /* 0x4C always lit, not directional (reflections at 0.3) */
    int                  lampCountC;            /* 0x50 */
    struct CarLamp  *lampsC;                /* 0x54 switched (see above) */
    int                  sparkCount;            /* 0x58 */
    f32                (*sparks)[4];            /* 0x5C the points under the car that throw sparks */
    int                  countD;                /* 0x60 */
    void                *tableD;                /* 0x64 */
    int                  mountCount;            /* 0x68 */
    struct CameraMount  *mounts;                /* 0x6C */
    int                 *tires;                 /* 0x70 [16] tyre file numbers (the second 8: flag != 0) */
} CarModelInfo;

void carmodelinfo_relocate(CarModelInfo *info);                 /* 0x00227168 */
/* The tyre file number of part 0x38 .. 0x42 (the 8 tyre parts: not 0x3A,
 * 0x3C, 0x3E), from the second half of the table when flag != 0;
 * defaultTire without a table or for another part. */
int  carmodelinfo_tire(CarModelInfo *info, int part, int flag); /* 0x00227220 */

#endif /* GAME_T455_CAR_MODEL_INFO_H */
