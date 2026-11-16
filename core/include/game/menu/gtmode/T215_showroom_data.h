/* T215_showroom_data.h - the showroom light sets (translation unit T215,
 * unit u065), 0x001BB1D8..0x001BB430.
 *
 * showroom_load loads the 11 "menu/showroom_*.fbi" files of gt3.vol (single,
 * split, compulsory, dealer, garage, wheel, wheelshop, carwash, prize,
 * oilchange, buycar) into one memalign(64) block, 0x140 bytes each; the
 * menu car renderers (u021, u070, u094) read the record of their camera's
 * mode: flag bits and floats (u070: value 0 the ambient, then 4 values for
 * each of three lights).
 */
#ifndef GAME_T215_SHOWROOM_DATA_H
#define GAME_T215_SHOWROOM_DATA_H

#include "game/game.h"

class MenuCarCamera;                        /* T411 (pointers only): mode at +0x04 */

/* One "menu/showroom_*.fbi" record, 0x140 bytes (the table stride). */
struct ShowroomData {
    u8  unk_00[4];                          /* 0x00 */
    s32 flags;                              /* 0x04 bit flags (showroom_flag) */
    u8  unk_08[8];                          /* 0x08 */
    f32 values[0x4C];                       /* 0x10 (showroom_value / showroom_value_ptr) */
};
EGCS_SIZE(ShowroomData, 0x140);

/* The 11 records (memalign(64, 11 x the size of showroom_single.fbi)).  The
 * per-record accessors showroom_record_flag 0x001BB1D8, _value 0x001BB1E8,
 * _value_ptr 0x001BB1F8 and the loader showroom_read_files 0x001BB2E0 are
 * only used here (menu_item_internal.h).  (The sweep named the F_ functions, u065's
 * proposal.) */
extern ShowroomData *g_0035257C;            /* 0x0035257C gp-0x5474 */

void          showroom_load(void);          /* 0x001BB208 allocates and loads the table (F_001BB208) */
/* The record of a menu camera's mode: 0 -> 0, 1 and 2 -> 1, 3 -> 2, ...
 * 11 -> 10, anything else 0. */
int           showroom_index(MenuCarCamera *cam);                         /* 0x001BB258 */
int           showroom_flag(ShowroomData *table, int bit, int index);     /* 0x001BB3B0 flag `bit' of record index */
f32           showroom_value(ShowroomData *table, int i, int index);      /* 0x001BB3D8 value i of record index */
f32          *showroom_value_ptr(ShowroomData *table, int i, int index);  /* 0x001BB400 &value i */
ShowroomData *showroom_table(void);         /* 0x001BB428 g_0035257C */

#endif /* GAME_T215_SHOWROOM_DATA_H */
