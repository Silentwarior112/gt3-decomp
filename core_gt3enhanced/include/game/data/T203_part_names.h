/* T203_part_names.h - the car part name tables (translation unit T203, unit
 * u063), 0x001B2C20..0x001B3918.
 *
 * Two arrays of {name, part id} sorted by name (strcmp order, for a binary
 * search), filled by the static initialiser: 0x35 part names ("ayc_on",
 * "tir_sports", "tbk_2", ...) at 0x0049C638 and 0x49 "set_*" setting names
 * ("set_tcs_off", "set_tur_stage3", ...) at 0x0049C3F0.  Both map to the
 * same part id numbering; 0x58 = an unknown name.  The tables and their
 * search (find_part_name 0x001B2C90) are only used here.
 */
#ifndef GAME_T203_PART_NAMES_H
#define GAME_T203_PART_NAMES_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */

int  part_id_of_name(const char *name);     /* 0x001B2C20 0x58 if unknown */
int  setting_id_of_name(const char *name);  /* 0x001B2C58 0x58 if unknown */
/* egcs's _GLOBAL_$I$ function of T203 (no name in C): the .ctors entry
 * 0x0028F150 builds the two tables (0x001B2D28). */

#endif /* __cplusplus */

#endif /* GAME_T203_PART_NAMES_H */
