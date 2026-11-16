/* T455_car_model_info.cpp - CarModelInfo, the car's description in its
 * model file (unit u102). */
#include "engine/gfx/model_set_internal.h"
#include "game/car/model/T455_car_model_info.h"

/* base is set before the zero-delta test.  Each table is relocated only
 * when its count is non-zero (the tyre table at +0x70 when its offset
 * is). */
// FUNC 0x00227168 carmodelinfo_relocate
void carmodelinfo_relocate(CarModelInfo *info)
{
    int delta = (u8 *)info - (u8 *)info->base;

    info->base = info;
    if (delta == 0)
        return;
    if (info->lampCountA != 0)
        U102_RELOC(info->lampsA, delta);
    if (info->lampCountB != 0)
        U102_RELOC(info->lampsB, delta);
    if (info->lampCountC != 0)
        U102_RELOC(info->lampsC, delta);
    if (info->sparkCount != 0)
        U102_RELOC(info->sparks, delta);
    if (info->countD != 0)
        U102_RELOC(info->tableD, delta);
    if (info->mountCount != 0)
        U102_RELOC(info->mounts, delta);
    if (info->tires != NULL)
        U102_RELOC(info->tires, delta);
}

/* The tyre file number of part 0x38 .. 0x42 (8 of them: 0x3A, 0x3C and
 * 0x3E are not tyre parts; a jump table of 11 cases @0x00341350), from the
 * second half of the table when flag != 0 (movn); defaultTire without a
 * table or for another part. */
// FUNC 0x00227220 carmodelinfo_tire
int carmodelinfo_tire(CarModelInfo *info, int part, int flag)
{
    int *table = info->tires;
    int *entry;
    int slot;

    if (table == NULL)
        return info->defaultTire;
    switch (part) {
    case 0x38: slot = 0; break;
    case 0x39: slot = 1; break;
    case 0x3B: slot = 2; break;
    case 0x3D: slot = 3; break;
    case 0x3F: slot = 4; break;
    case 0x40: slot = 5; break;
    case 0x41: slot = 6; break;
    case 0x42: slot = 7; break;
    default:
        return info->defaultTire;
    }
    if (flag != 0)
        slot += 8;
    /* the entry's address in a local: addu table, index as retail (a
     * direct table[slot] adds index + table) */
    entry = &table[slot];
    return *entry;
}
