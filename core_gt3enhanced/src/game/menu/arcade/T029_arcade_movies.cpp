/* T029 - the background movies of the arcade menu.  0x0010FE58..0x0010FF90.
 *
 * Only the static initialiser of g_00292930[8] (MovieParam, T000): [0] is
 * the arcade top's movie (T031), [3] the menu background of the arcade
 * setup (T010, T014, T023), [4] the course select's (T025); [2] and [4] are
 * the same file.  None plays its sound or once only (withSound / oneShot 0:
 * the menu backgrounds, u120); field is 1 for the full-screen (640 x 448)
 * ones.
 *
 * Includes: only the table's header (retail's .rodata, 0x0030BF30, is the
 * seven file names, no type name). */
#include "game/menu/arcade/T029_arcade_movies.h"
#include "game/menu/arcade/arcade_course_select_internal.h"

/* The table (T029's .data, before its frame info), filled by
 * T029_static_init: zeros in the image (egcs writes a C++ global without an
 * initialiser as zeros in .data). */
MovieParam g_00292930[8];                   /* 0x00292930 */

/* Static initialiser (.ctors entry 0x0028F040).  In the original a
 * compiler-generated _GLOBAL_$I$ function: the table defined here with
 * MovieParam objects built by a constructor, which egcs fills at run time
 * through a walking cursor (cp/init.c build_vec_init; see u006's
 * T030_static_init).  The table stays at its original address, so the walk
 * is written out: an exact form, not the original (the same 8 entries). */
// FUNC 0x0010FE58 T029_static_init
void T029_static_init(void)
{
    MovieParam *p = g_00292930;

    p->file = "stream/arcade_menu_full.pss";
    p->width = 640;
    p->height = 448;
    p->field = 1;
    p->withSound = 0;
    p->oneShot = 0;
    p++;
    p->file = "stream/level_menu_full.pss";
    p->width = 640;
    p->height = 448;
    p->field = 1;
    p->withSound = 0;
    p->oneShot = 0;
    p++;
    p->file = "stream/laguna_full.pss";
    p->width = 640;
    p->height = 448;
    p->field = 1;
    p->withSound = 0;
    p->oneShot = 0;
    p++;
    p->file = "stream/menu_bg2.pss";
    p->width = 320;
    p->height = 112;
    p->field = 0;
    p->withSound = 0;
    p->oneShot = 0;
    p++;
    p->file = "stream/laguna_full.pss";
    p->width = 640;
    p->height = 448;
    p->field = 1;
    p->withSound = 0;
    p->oneShot = 0;
    p++;
    p->file = "stream/menu_bg.pss";
    p->width = 640;
    p->height = 224;
    p->field = 0;
    p->withSound = 0;
    p->oneShot = 0;
    p++;
    p->file = "stream/arcade_top.pss";
    p->width = 320;
    p->height = 448;
    p->field = 0;
    p->withSound = 0;
    p->oneShot = 0;
    p++;
    p->file = "stream/arcade_level.pss";
    p->width = 320;
    p->height = 448;
    p->field = 0;
    p->withSound = 0;
    p->oneShot = 0;
}
