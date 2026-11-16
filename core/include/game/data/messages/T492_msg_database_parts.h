/* T492_msg_database_parts.h - GT2KMSGDATABASEPARTS::Message, the
 * "DATABASEPARTS" real-time text table (0x50 entries: the part names),
 * translation unit T492, unit u109, 0x0023B568..0x0023B670.  The global
 * instance is g_002F1188 (T492 .data; keys 0x002F0F08, texts 0x002F1048). */
#ifndef GAME_T492_MSG_DATABASE_PARTS_H
#define GAME_T492_MSG_DATABASE_PARTS_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"            /* base GT2KRealTimeText */

namespace GT2KMSGDATABASEPARTS {

/* vtable 0x003436F0 (1 slot).  typeinfo __tfQ220GT2KMSGDATABASEPARTS7Message
 * 0x0023B628 (si, base GT2KRealTimeText; storage 0x004B2A80).  No data of its
 * own: size 0x18 (GT2KRealTimeText: next, name, keys, texts, count, vptr
 * +0x14). */
class Message : public GT2KRealTimeText {
public:
    Message();                                  /* 0x0023B568 GT2KRealTimeText("DATABASEPARTS",
                                                   keys, texts, 0x50) */
    virtual ~Message();                         /* [0] +0x0C 0x0023B5B8 */
};

} /* namespace GT2KMSGDATABASEPARTS */

EGCS_SIZE(GT2KMSGDATABASEPARTS::Message, 0x18);

extern const char *g_002F1048[0x50];            /* 0x002F1048 the DATABASEPARTS texts (the part names) */

/* g_002F1188's static constructor / destructor 0x0023B608 / 0x0023B5E0
 * (.ctors 0x0028F194, .dtors 0x0028F268) are egcs's (T492 defines it). */

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
extern "C" void *__tfQ220GT2KMSGDATABASEPARTS7Message(void);   /* 0x0023B628 */
#endif

#endif /* GAME_T492_MSG_DATABASE_PARTS_H */
