/* T187_msg_race_msg.h - GT2KMSGRACEMSG::Message, the "RACEMSG" real-time
 * text table (one entry: SessionBest -> "SESSION BEST"), translation unit
 * T187, unit u061, 0x001AE990..0x001AEA98.  The global instance is
 * g_002C45A8 (T187 .data); the key and text arrays (one entry each) are
 * g_00352380 / g_00352388 (gp-0x5670 / gp-0x5668, T187's .sdata; replay_effects_internal.h
 * declares them, T121_race_entry.h the text too: RaceEntryCar's default
 * driver name). */
#ifndef GAME_T187_MSG_RACE_MSG_H
#define GAME_T187_MSG_RACE_MSG_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"            /* base GT2KRealTimeText */

namespace GT2KMSGRACEMSG {

/* vtable 0x00331258 (1 slot).  typeinfo __tfQ214GT2KMSGRACEMSG7Message
 * 0x001AEA48 (si, base GT2KRealTimeText; storage 0x004B23C0).  No data of
 * its own: size 0x18 (GT2KRealTimeText's vptr +0x14). */
class Message : public GT2KRealTimeText {
public:
    Message();                                  /* 0x001AE990 GT2KRealTimeText("RACEMSG",
                                                   keys, texts, 1) */
    virtual ~Message();                         /* [0] +0x0C 0x001AE9D8 */
};

} /* namespace GT2KMSGRACEMSG */

EGCS_SIZE(GT2KMSGRACEMSG::Message, 0x18);

/* The table (T187 .data).  Its static destructor / initialiser (egcs's
 * _GLOBAL_$D$ / _GLOBAL_$I$, 0x001AEA00 / 0x001AEA28, .dtors 0x0028F228 /
 * .ctors 0x0028F148) are compiler-generated. */
extern GT2KMSGRACEMSG::Message g_002C45A8;  /* 0x002C45A8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ214GT2KMSGRACEMSG7Message(void);         /* 0x001AEA48 */
#endif

#endif /* GAME_T187_MSG_RACE_MSG_H */
