/* T093_msg_setting.h - GT2KMSGSETTING::Message, the "SETTING" real-time
 * text table (5 entries: UNIT_KGF_MM "kgf/mm", UNIT_MM "mm", UNIT_LEVEL
 * "Level", UNIT_DEGREE EUC-JP 0xA1EB (the degree sign), CUSTOM "CUSTOM":
 * the units of the setting pages), translation unit T093, unit u040,
 * 0x00174350..0x00174460.  The global instance is g_002B1958 (T093 .data, defined there);
 * the name "SETTING" is at 0x00351D00. */
#ifndef GAME_T093_MSG_SETTING_H
#define GAME_T093_MSG_SETTING_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"            /* base GT2KRealTimeText */

namespace GT2KMSGSETTING {

/* vtable 0x00325B28 (1 slot).  typeinfo __tfQ214GT2KMSGSETTING7Message
 * 0x00174410 (si, base GT2KRealTimeText; storage 0x004B1DB0).  No data of
 * its own: size 0x18 (GT2KRealTimeText's vptr +0x14). */
class Message : public GT2KRealTimeText {
public:
    Message();                                  /* 0x00174350 GT2KRealTimeText("SETTING",
                                                   g_002B1928, g_002B1940, 5) */
    virtual ~Message();                         /* [0] +0x0C 0x001743A0 */
};

} /* namespace GT2KMSGSETTING */

EGCS_SIZE(GT2KMSGSETTING::Message, 0x18);

extern const char *g_002B1928[5];               /* 0x002B1928 the SETTING keys */
/* The texts (the current language's once realtime_set_language ran):
 * [0] "kgf/mm", [1] "mm", [2] "Level", [3] the degree sign, [4] "CUSTOM"
 * (u033: the units of the setting sliders, T081). */
extern const char *g_002B1940[5];               /* 0x002B1940 the SETTING texts */
/* g_002B1940's indexes, named after their keys (g_002B1928). */
enum {
    SETTING_UNIT_KGF_MM = 0,                    /* "kgf/mm" */
    SETTING_UNIT_MM     = 1,                    /* "mm" */
    SETTING_UNIT_LEVEL  = 2,                    /* "Level" */
    SETTING_UNIT_DEGREE = 3,                    /* the degree sign (EUC-JP 0xA1EB) */
    SETTING_CUSTOM      = 4                     /* "CUSTOM" */
};
extern GT2KMSGSETTING::Message g_002B1958;      /* 0x002B1958 (T093 .data, defined there) */

/* 0x001743C8 (.dtors 0x0028F1FC) / 0x001743F0 (.ctors 0x0028F0C0): egcs's static destructor
 * and initialiser of g_002B1958 (T093 defines it, and the tables):
 * _GLOBAL_$D$g_002B1928 (~Message(2)) and _GLOBAL_$I$g_002B1928 (Message()). */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tfQ214GT2KMSGSETTING7Message(void);     /* 0x00174410 */
extern u32 __tiQ214GT2KMSGSETTING7Message[3];   /* 0x004B1DB0 (__si_type_info) */
}
#endif

#endif /* GAME_T093_MSG_SETTING_H */
