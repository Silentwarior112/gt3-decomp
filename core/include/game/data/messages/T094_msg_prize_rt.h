/* T094_msg_prize_rt.h - GT2KMSGPRIZE_RT_MSG::Message, the "PRIZE_RT_MSG"
 * real-time text table (one entry: BONUS_PREFIX -> EUC-JP "prize money",
 * the word before a prize amount), translation unit T094, unit u040,
 * 0x00174460..0x00174568.  The global instance is g_002B1A38 (T094 .data, defined there);
 * the name "PRIZE_RT_MSG" is at 0x00325B68.  The key and text arrays are in
 * .sdata: g_00351D08 (gp-0x5CE8) and g_00351D18 (gp-0x5CD8; the prize
 * counter of T099 prints its text). */
#ifndef GAME_T094_MSG_PRIZE_RT_H
#define GAME_T094_MSG_PRIZE_RT_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"            /* base GT2KRealTimeText */

namespace GT2KMSGPRIZE_RT_MSG {

/* vtable 0x00325B78 (1 slot).  typeinfo __tfQ219GT2KMSGPRIZE_RT_MSG7Message
 * 0x00174518 (si, base GT2KRealTimeText; storage 0x004B1DC0).  No data of
 * its own: size 0x18 (GT2KRealTimeText's vptr +0x14). */
class Message : public GT2KRealTimeText {
public:
    Message();                                  /* 0x00174460 GT2KRealTimeText("PRIZE_RT_MSG",
                                                   g_00351D08, g_00351D18, 1) */
    virtual ~Message();                         /* [0] +0x0C 0x001744A8 */
};

} /* namespace GT2KMSGPRIZE_RT_MSG */

EGCS_SIZE(GT2KMSGPRIZE_RT_MSG::Message, 0x18);

extern const char *g_00351D08[1];               /* 0x00351D08 gp-0x5CE8 the PRIZE_RT_MSG keys */
/* The texts: EUC-JP "賞金" (prize money, key BONUS_PREFIX), at 0x00351D10.
 * PrizeCounterRemainItem (T099) prints it. */
extern const char *g_00351D18[1];               /* 0x00351D18 gp-0x5CD8 */
extern GT2KMSGPRIZE_RT_MSG::Message g_002B1A38; /* 0x002B1A38 (T094 .data, defined there) */

/* 0x001744D0 (.dtors 0x0028F200) / 0x001744F8 (.ctors 0x0028F0C4): egcs's static destructor
 * and initialiser of g_002B1A38 (T094 defines it, and the tables):
 * _GLOBAL_$D$g_00351D08 (~Message(2)) and _GLOBAL_$I$g_00351D08 (Message()). */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tfQ219GT2KMSGPRIZE_RT_MSG7Message(void);    /* 0x00174518 */
extern u32 __tiQ219GT2KMSGPRIZE_RT_MSG7Message[3];  /* 0x004B1DC0 (__si_type_info) */
}
#endif

#endif /* GAME_T094_MSG_PRIZE_RT_H */
