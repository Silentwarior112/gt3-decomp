/* T493 - GT2KMSGDATABASERACECATEGORY::Message, the "DATABASERACECATEGORY"
 * real-time text table (14 entries: TEXT_CATEGORY_ARCADE "Arcade Race" ..
 * TEXT_CATEGORY_GT_MACHINE_TEST_MAX "Test VMAX"; T489's race_kind_text
 * reads its texts), and its global instance g_002F12D8.
 * 0x0023B678..0x0023B780.
 *
 * Data: .rodata 0x00343728..0x00343A28: the keys' and the texts' strings
 * (all longer than 8 bytes; only the tables point at them: DATA_START), the
 * constructor's "DATABASERACECATEGORY", the vtable, the one type name
 * (Message).  .data 0x002F1268: the keys, the texts, g_002F12D8, then the
 * frame info. */
// DATA_START 0x00343728
#include "game/data/messages/T493_msg_database_race_category.h"
#include "game/data/car_database_internal.h"                               /* g_002F1268 */

/* The keys and texts (.data 0x002F1268 / 0x002F12A0), defined first as in
 * the original: their strings open the unit's .rodata, in front of the
 * constructor's name, the vtable and the type name. */
const char *g_002F1268[14] = {   /* 0x002F1268 */
    "TEXT_CATEGORY_ARCADE",             /* 0x00343728 */
    "TEXT_CATEGORY_2P_BATTLE",          /* 0x00343740 */
    "TEXT_CATEGORY_LIMITED_TIME_TRIAL", /* 0x00343758 */
    "TEXT_CATEGORY_FREE_TIME_TRIAL",    /* 0x00343780 */
    "TEXT_CATEGORY_GT_CHAMPIONSHIP",    /* 0x003437A0 */
    "TEXT_CATEGORY_GT_LICENSE_B",       /* 0x003437C0 */
    "TEXT_CATEGORY_GT_LICENSE_A",       /* 0x003437E0 */
    "TEXT_CATEGORY_GT_LICENSE_IB",      /* 0x00343800 */
    "TEXT_CATEGORY_GT_LICENSE_IA",      /* 0x00343820 */
    "TEXT_CATEGORY_GT_LICENSE_SUPER",   /* 0x00343840 */
    "TEXT_CATEGORY_GT_LICENSE_RALLY",   /* 0x00343860 */
    "TEXT_CATEGORY_GT_MACHINE_TEST_400", /* 0x00343880 */
    "TEXT_CATEGORY_GT_MACHINE_TEST_1000", /* 0x003438A8 */
    "TEXT_CATEGORY_GT_MACHINE_TEST_MAX", /* 0x003438D0 */
};
const char *g_002F12A0[14] = {   /* 0x002F12A0 */
    "Arcade Race",                      /* 0x003438F8 */
    "2P Battle",                        /* 0x00343908 */
    "Time Trial ",                      /* 0x00343918 */
    "Free Run",                         /* 0x00343928 */
    "Championship",                     /* 0x00343938 */
    "License B-",                       /* 0x00343948 */
    "License A-",                       /* 0x00343958 */
    "License IB-",                      /* 0x00343968 */
    "License IA-",                      /* 0x00343978 */
    "License S-",                       /* 0x00343988 */
    "License R-",                       /* 0x00343998 */
    "Test 0-400m",                      /* 0x003439A8 */
    "Test 0-1000m",                     /* 0x003439B8 */
    "Test VMAX",                        /* 0x003439C8 */
};

namespace GT2KMSGDATABASERACECATEGORY {

/* GT2KRealTimeText("DATABASERACECATEGORY", keys, texts, 14), then the vptr
 * (+0x14) = 0x003439F0.  Caller: T493_static_init. */
// FUNC 0x0023B678 GT2KMSGDATABASERACECATEGORY::Message::Message
Message::Message()
    : GT2KRealTimeText("DATABASERACECATEGORY", g_002F1268, g_002F12A0, 14)
{
}

/* index.txt: GT2KMSGDATABASERACECATEGORY::Message::virtual_0 (slot 0). */
// FUNC 0x0023B6C8 GT2KMSGDATABASERACECATEGORY::Message::~Message
Message::~Message()
{
    /* Implicit: the vptr (+0x14) = 0x003439F0, then
     * GT2KRealTimeText::~GT2KRealTimeText(this, flags). */
}

} /* namespace GT2KMSGDATABASERACECATEGORY */

/* The global instance (.data 0x002F12D8, after the keys and texts: egcs
 * puts a public object that has a constructor in .data, zero-filled, not in
 * .bss). */
GT2KMSGDATABASERACECATEGORY::Message g_002F12D8;   /* 0x002F12D8 */

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002F12D8 (.dtors entry 0x0028F26C, .ctors entry 0x0028F198), emitted at
 * the end of the unit, the destructor first: ~Message(2) / Message() on it.
 * Named after the unit's first public object, the keys g_002F1268. */
// FUNC 0x0023B6F0 T493_static_destroy (_GLOBAL_$D$g_002F1268)
/* implicit: ~Message (the static destructor of g_002F12D8) */

// FUNC 0x0023B718 T493_static_init (_GLOBAL_$I$g_002F1268)
/* implicit: Message (the static initialiser of g_002F12D8) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base GT2KRealTimeText). */
// FUNC 0x0023B738 __tfQ227GT2KMSGDATABASERACECATEGORY7Message
extern "C" void *__tfQ227GT2KMSGDATABASERACECATEGORY7Message(void)
{
    if (__tiQ227GT2KMSGDATABASERACECATEGORY7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ227GT2KMSGDATABASERACECATEGORY7Message,
                  "Q227GT2KMSGDATABASERACECATEGORY7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ227GT2KMSGDATABASERACECATEGORY7Message;
}
#endif
