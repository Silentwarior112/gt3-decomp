/* T215 - the showroom light sets (0x001BB1D8..0x001BB430).
 *
 * showroom_load loads the 11 "menu/showroom_*.fbi" files of gt3.vol into one
 * memalign(64) block (0x140 bytes a record; the block is sized 11 x the size
 * of showroom_single.fbi); the accessors read a flag bit or a float of
 * record `index'.  showroom_index maps a menu camera's mode to its record.
 */
/* SimpleCameraBase's inline print puts its format string at the start of
 * the unit's .rodata (0x00334020), as in retail: the original included this
 * header (u064's T212 does too).  Nothing references it, so the file's
 * .rodata starts there unreferenced; retail's has no type name. */
// DATA_START 0x00334020
#include "game/camera/T412_simple_camera_base.h"
#include "game/menu/framework/menu_item_internal.h"
#include "game/camera/T411_menu_car_camera.h"      /* MenuCarCamera::type (showroom_index) */
#include "engine/io/T502_file_resolve.h"         /* file_size_retry */
#include "engine/io/T503_vol_thread.h"           /* vol_device_object, file_read_blocking */
#include "game/menu/gtmode/T215_showroom_data.h"

/* The table (T215 .sdata, zero: an uninitialised global, written at the
 * end of the file's output). */
ShowroomData *g_0035257C;                   /* 0x0035257C gp-0x5474 */

// FUNC 0x001BB1D8 showroom_record_flag
int showroom_record_flag(ShowroomData *rec, int bit)
{
    return (rec->flags >> bit) & 1;
}

// FUNC 0x001BB1E8 showroom_record_value
f32 showroom_record_value(ShowroomData *rec, int i)
{
    return rec->values[i];
}

// FUNC 0x001BB1F8 showroom_record_value_ptr
f32 *showroom_record_value_ptr(ShowroomData *rec, int i)
{
    return &rec->values[i];
}

// FUNC 0x001BB208 showroom_load
void showroom_load(void)
{
    u32 size = file_size_retry("menu/showroom_single.fbi", 1, &vol_device_object);
    g_0035257C = (ShowroomData *)memalign(0x40, size * SHOWROOM_RECORDS);
    showroom_read_files(g_0035257C);
}

// FUNC 0x001BB258 showroom_index
/* The record of a menu camera's mode (jump table 0x00334060): 0 single,
 * 1 and 2 (mirrored) split, 3 compulsory, 4 dealer, 5 garage, 6 wheel,
 * 7 wheelshop, 8 carwash, 9 prize, 10 oilchange, 11 buycar (the file order
 * of showroom_read_files); anything else 0. */
int showroom_index(MenuCarCamera *cam)
{
    switch (cam->type) {
    case 0:  return 0;
    case 1:
    case 2:  return 1;
    case 3:  return 2;
    case 4:  return 3;
    case 5:  return 4;
    case 6:  return 5;
    case 7:  return 6;
    case 8:  return 7;
    case 9:  return 8;
    case 10: return 9;
    case 11: return 10;
    }
    return 0;
}

// FUNC 0x001BB2E0 showroom_read_files
/* The files in record order, each read whole (unpacked) into its record. */
void showroom_read_files(ShowroomData *table)
{
    /* copied to the stack from .rodata 0x003341B8 */
    const char *names[SHOWROOM_RECORDS] = {
        "menu/showroom_single.fbi",
        "menu/showroom_split.fbi",
        "menu/showroom_compulsory.fbi",
        "menu/showroom_dealer.fbi",
        "menu/showroom_garage.fbi",
        "menu/showroom_wheel.fbi",
        "menu/showroom_wheelshop.fbi",
        "menu/showroom_carwash.fbi",
        "menu/showroom_prize.fbi",
        "menu/showroom_oilchange.fbi",
        "menu/showroom_buycar.fbi",
    };
    const char **name = names;

    for (int i = 0; i < SHOWROOM_RECORDS; i++) {   /* counted down from 10 */
        file_read_blocking(*name, table, 0, 1, &vol_device_object);
        name++;
        table++;                        /* + 0x140 */
    }
}

// FUNC 0x001BB3B0 showroom_flag
int showroom_flag(ShowroomData *table, int bit, int index)
{
    return showroom_record_flag(&table[index], bit);
}

// FUNC 0x001BB3D8 showroom_value
f32 showroom_value(ShowroomData *table, int i, int index)
{
    return showroom_record_value(&table[index], i);
}

// FUNC 0x001BB400 showroom_value_ptr
f32 *showroom_value_ptr(ShowroomData *table, int i, int index)
{
    return showroom_record_value_ptr(&table[index], i);
}

// FUNC 0x001BB428 showroom_table
ShowroomData *showroom_table(void)
{
    return g_0035257C;
}
