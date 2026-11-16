/* T114_msg_shortmenu.h - GT2KMSGSHORTMENU::Message, the "SHORTMENU"
 * real-time text table (6 entries, the words of the short menu's result
 * list: SESSION_BRFORE / SESSION_AFTER, EUC-JP "No." / "race" around a
 * session number, LAPS_SINGLE "Lap", LAPS_PLURAL "Laps", POINT_SINGLE "pt",
 * POINT_PLURAL "pts"), translation unit T114, unit u040,
 * 0x00174568..0x00174678.  The global instance is g_002B1B48 (T114 .data, defined there);
 * the name "SHORTMENU" is at 0x00325C08. */
#ifndef GAME_T114_MSG_SHORTMENU_H
#define GAME_T114_MSG_SHORTMENU_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"            /* base GT2KRealTimeText */

namespace GT2KMSGSHORTMENU {

/* vtable 0x00325C18 (1 slot).  typeinfo __tfQ216GT2KMSGSHORTMENU7Message
 * 0x00174628 (si, base GT2KRealTimeText; storage 0x004B1DD0).  No data of
 * its own: size 0x18 (GT2KRealTimeText's vptr +0x14). */
class Message : public GT2KRealTimeText {
public:
    Message();                                  /* 0x00174568 GT2KRealTimeText("SHORTMENU",
                                                   g_002B1B18, g_002B1B30, 6) */
    virtual ~Message();                         /* [0] +0x0C 0x001745B8 */
};

} /* namespace GT2KMSGSHORTMENU */

EGCS_SIZE(GT2KMSGSHORTMENU::Message, 0x18);

extern const char *g_002B1B18[6];               /* 0x002B1B18 the SHORTMENU keys */
/* The texts: [0] / [1] before / after the session number, [2] "Lap",
 * [3] "Laps", [4] "pt", [5] "pts" (u015's short menu items, T113). */
extern const char *g_002B1B30[6];               /* 0x002B1B30 the SHORTMENU texts */
extern GT2KMSGSHORTMENU::Message g_002B1B48;    /* 0x002B1B48 (T114 .data, defined there) */

/* 0x001745E0 (.dtors 0x0028F208) / 0x00174608 (.ctors 0x0028F0E0): egcs's static destructor
 * and initialiser of g_002B1B48 (T114 defines it, and the tables):
 * _GLOBAL_$D$g_002B1B18 (~Message(2)) and _GLOBAL_$I$g_002B1B18 (Message()). */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tfQ216GT2KMSGSHORTMENU7Message(void);   /* 0x00174628 */
extern u32 __tiQ216GT2KMSGSHORTMENU7Message[3]; /* 0x004B1DD0 (__si_type_info) */
}
#endif

#endif /* GAME_T114_MSG_SHORTMENU_H */
