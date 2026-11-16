/* T493_msg_database_race_category.h - GT2KMSGDATABASERACECATEGORY::Message,
 * the "DATABASERACECATEGORY" real-time text table (14 entries:
 * TEXT_CATEGORY_ARCADE "Arcade Race" .. TEXT_CATEGORY_GT_MACHINE_TEST_MAX
 * "Test VMAX"), translation unit T493, unit u109, 0x0023B678..0x0023B780.
 * The global instance is g_002F12D8 (T493 .data; keys 0x002F1268, texts
 * 0x002F12A0). */
#ifndef GAME_T493_MSG_DATABASE_RACE_CATEGORY_H
#define GAME_T493_MSG_DATABASE_RACE_CATEGORY_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"            /* base GT2KRealTimeText */

namespace GT2KMSGDATABASERACECATEGORY {

/* vtable 0x003439F0 (1 slot).  typeinfo
 * __tfQ227GT2KMSGDATABASERACECATEGORY7Message 0x0023B738 (si, base
 * GT2KRealTimeText; storage 0x004B2A90).  No data of its own: size 0x18. */
class Message : public GT2KRealTimeText {
public:
    Message();                                  /* 0x0023B678 GT2KRealTimeText("DATABASERACECATEGORY",
                                                   keys, texts, 14) */
    virtual ~Message();                         /* [0] +0x0C 0x0023B6C8 */
};

} /* namespace GT2KMSGDATABASERACECATEGORY */

EGCS_SIZE(GT2KMSGDATABASERACECATEGORY::Message, 0x18);

/* The texts, in race_kind_text's order (T489): Arcade Race, 2P Battle, Time
 * Trial, Free Run, Championship, License B- .. License R-, the 3 machine
 * tests. */
extern const char *g_002F12A0[14];              /* 0x002F12A0 the DATABASERACECATEGORY texts */

/* g_002F12D8's static constructor / destructor 0x0023B718 / 0x0023B6F0
 * (.ctors 0x0028F198, .dtors 0x0028F26C) are egcs's (T493 defines it). */

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
extern "C" void *__tfQ227GT2KMSGDATABASERACECATEGORY7Message(void);  /* 0x0023B738 */
#endif

#endif /* GAME_T493_MSG_DATABASE_RACE_CATEGORY_H */
