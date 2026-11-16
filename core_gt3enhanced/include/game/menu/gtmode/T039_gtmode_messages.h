/* T039_gtmode_messages.h - GT2KMSGGTMODE::Message, the "GTMODE" real-time
 * text table (1 entry: ALREADY_YOU_BOUGHT "購入済", the "purchased" of the
 * tuning shop's prices, T036), translation unit T039, unit u013,
 * 0x00127790..0x00127898.  The global instance is g_0029AD40 (T039 .data);
 * its keys are g_003512B8, its texts g_003512C8 (both T039 .sdata). */
#ifndef GAME_T039_GTMODE_MESSAGES_H
#define GAME_T039_GTMODE_MESSAGES_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"        /* base GT2KRealTimeText */

namespace GT2KMSGGTMODE {

/* vtable 0x003129C0 (1 slot).  typeinfo __tfQ213GT2KMSGGTMODE7Message
 * 0x00127848 (si, base GT2KRealTimeText; storage 0x004B1600).  No data of
 * its own: size 0x18 (GT2KRealTimeText: next, name, keys, texts, count, vptr
 * +0x14). */
class Message : public GT2KRealTimeText {
public:
    Message();                              /* 0x00127790 GT2KRealTimeText("GTMODE", g_003512B8,
                                               g_003512C8, 1) */
    virtual ~Message();                     /* [0] +0x0C 0x001277D8 */
};

} /* namespace GT2KMSGGTMODE */

EGCS_SIZE(GT2KMSGGTMODE::Message, 0x18);

extern GT2KMSGGTMODE::Message g_0029AD40;   /* 0x0029AD40 the GTMODE table */
extern const char *g_003512B8[1];           /* 0x003512B8 gp-0x6738 its keys {"ALREADY_YOU_BOUGHT"}
                                               (0x003129A8) */
/* Its texts: [0] "購入済" (EUC-JP, 0x003512C0) until a language is
 * applied.  T036's tuning-shop prices print it for a part already
 * bought. */
extern const char *g_003512C8[1];           /* 0x003512C8 gp-0x6728 */

/* 0x00127800 (.dtors 0x0028F1E8) / 0x00127828 (.ctors 0x0028F050): egcs's
 * static destructor and initialiser of g_0029AD40 (T039 defines it):
 * _GLOBAL_$D$__Q213GT2KMSGGTMODE7Message (~Message(2)) and
 * _GLOBAL_$I$__Q213GT2KMSGGTMODE7Message (Message()). */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ213GT2KMSGGTMODE7Message(void);   /* 0x00127848 */
#endif

#endif /* GAME_T039_GTMODE_MESSAGES_H */
