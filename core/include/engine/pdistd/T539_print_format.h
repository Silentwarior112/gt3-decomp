/* T539_print_format.h - pdistd::PrintFormatTargetBase and pdistd::PrintFormat, the
 * printf engine of Polyphony's pdistd (printformat.cxx; translation unit T539, unit u125).
 *
 * Same machine code as the bootstrap's src/pdistd/printformat.cpp; the class
 * and PrintFormat are declared once, in the shared include/pdistd/printf.h
 * (its address comments are the bootstrap's).  Core addresses
 * (src/engine/pdistd/T539_print_format.cpp):
 *   PrintFormatTargetBase()                    0x0026B1C8
 *   [0] +0x0C  ~PrintFormatTargetBase()        0x0026B1E0
 *   [1] +0x14  int putc(int c)  (return 0)     0x0026B210
 *   int PrintFormat(PrintFormatTargetBase *target, const char *format,
 *                   va_list args)              0x0026B218 (callers: vprintf, vsprintf)
 * vtable _vt$Q26pdistd21PrintFormatTargetBase 0x003476A0, vptr +0x00 (no
 * data members), size 0x4.  typeinfo 0x0026C080, RTTI name 0x003476B8,
 * __user_type_info 0x003534F8.  Tables: s_pow10_double 0x003460C0,
 * s_pow10_u64 0x003474C0, s_digits 0x00347560.
 * Subclasses: the anonymous-namespace PrintTargets of T533 and T535.
 */
#ifndef GAME_T539_PRINT_FORMAT_H
#define GAME_T539_PRINT_FORMAT_H

#include "game/game.h"
#include "pdistd/printf.h"                      /* class pdistd::PrintFormatTargetBase, PrintFormat */

#ifdef __cplusplus
EGCS_SIZE(pdistd::PrintFormatTargetBase, 0x4);

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ26pdistd21PrintFormatTargetBase(void);    /* 0x0026C080 */
#endif
#endif /* __cplusplus */

#endif /* GAME_T539_PRINT_FORMAT_H */
