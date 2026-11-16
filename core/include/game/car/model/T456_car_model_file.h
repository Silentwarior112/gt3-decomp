/* T456_car_model_file.h - the loaded car model file (translation unit T456,
 * unit u102).
 *
 * The plan put CarModelFile in T061_car_get_control.h (u021 defined it most
 * fully); this unit relocates it.
 */
#ifndef GAME_T456_CAR_MODEL_FILE_H
#define GAME_T456_CAR_MODEL_FILE_H

#include "game/game.h"

struct CarModelInfo;                            /* T455 (pointers only) */
struct ModelSet;                                /* T451 (a class there) */
struct TireFile2;                               /* T447 */
struct WheelFile2;                              /* T447 */

/* A loaded car model file (menu/cars/<name>, cars/day/<name>, ...).  The
 * pointers are stored as offsets from base; carmodelfile_relocate turns
 * them into pointers and sets base = the file.  The wheel file is the last
 * part; the free space after it is carmodelfile_work_area. */
typedef struct CarModelFile {
    struct CarModelFile *base;                  /* 0x00 the address the offsets are relative to */
    struct CarModelInfo *info;                  /* 0x04 (T455) */
    struct ModelSet     *body;                  /* 0x08 the body model (T451); +0x2C its texture set */
    struct TireFile2    *tire;                  /* 0x0C the tyre file (T447) */
    struct WheelFile2   *wheel;                 /* 0x10 the wheel file (T447) */
    void                *unk_14;                /* 0x14 an offset, relocated with the others */
} CarModelFile;

/* Relocates the file and its parts (always: no test of a zero delta). */
void  carmodelfile_relocate(struct CarModelFile *f);            /* 0x002272B0 */
/* The free space after the file: the end of the wheel file (its size at
 * +0x0C), 64-byte aligned. */
void *carmodelfile_work_area(struct CarModelFile *f);           /* 0x00227358 */

#endif /* GAME_T456_CAR_MODEL_FILE_H */
