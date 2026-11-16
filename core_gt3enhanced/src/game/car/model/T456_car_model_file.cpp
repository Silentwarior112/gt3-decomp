/* T456_car_model_file.cpp - CarModelFile, a loaded car model file: the
 * description (T455), the body (a ModelSet, T451), the tyre and wheel
 * files (T447) (unit u102). */
#include "engine/gfx/model_set_internal.h"
#include "game/car/model/T456_car_model_file.h"

/* (no test of a zero delta here.)  Each part is relocated in a statement
 * block (`do { } while (0)', ? a macro, as T447's tire_file_relocate /
 * wheel_file_relocate): its loop notes keep the base store ahead of the
 * first load, as retail. */
// FUNC 0x002272B0 carmodelfile_relocate
void carmodelfile_relocate(CarModelFile *f)
{
    int delta = (u8 *)f - (u8 *)f->base;

    f->base = f;
    do {
        if (f->info != NULL) {
            U102_RELOC(f->info, delta);
            carmodelinfo_relocate(f->info);
        }
    } while (0);
    do {
        if (f->body != NULL) {
            U102_RELOC(f->body, delta);
            modelset_relocate(f->body);
        }
    } while (0);
    do {
        if (f->tire != NULL) {
            U102_RELOC(f->tire, delta);
            tire_file_relocate(f->tire);
        }
    } while (0);
    do {
        if (f->wheel != NULL) {
            U102_RELOC(f->wheel, delta);
            wheel_file_relocate(f->wheel);
        }
    } while (0);
    do {
        if (f->unk_14 != NULL)
            U102_RELOC(f->unk_14, delta);
    } while (0);
}

/* The end of the wheel file (the last part of the file), 64-byte aligned:
 * the free space after the file. */
// FUNC 0x00227358 carmodelfile_work_area
void *carmodelfile_work_area(CarModelFile *f)
{
    return (void *)(((u32)f->wheel + f->wheel->size + 0x3F) & 0xFFFFFFC0);
}
