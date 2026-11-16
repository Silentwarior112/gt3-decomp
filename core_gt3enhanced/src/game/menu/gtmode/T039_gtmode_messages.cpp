/* T039 - GT2KMSGGTMODE::Message, the "GTMODE" real-time text table, and its
 * global instance g_0029AD40.  0x00127790..0x00127898.
 *
 * Data: .rodata 0x003129A8..0x003129EA: the key "ALREADY_YOU_BOUGHT" (only
 * the key table points at it: DATA_START), the vtable, the one type name
 * (Message: the original saw no other class with a type name).  .sdata
 * 0x003512B8..0x003512D7: the keys, the text "購入済", the texts, the
 * constructor's "GTMODE".  .data: g_0029AD40, then the frame info. */
// DATA_START 0x003129A8
#include "game/menu/gtmode/T039_gtmode_messages.h"
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si (GCC 14's __tf) */
#include "game/menu/gtmode/gtmode_main_internal.h"

/* The table's keys (.sdata 0x003512B8; their string in .rodata) and texts
 * (.sdata 0x003512C8; the text, EUC-JP "購入済", "purchased", in .sdata
 * 0x003512C0 under -G8), until a language is applied (GT2KRealTimeText,
 * T517).  Defined before the constructor: retail has them in front of its
 * "GTMODE" (0x003512D0). */
const char *g_003512B8[1] = { "ALREADY_YOU_BOUGHT" };      /* 0x003512B8 */
const char *g_003512C8[1] = { "\xB9\xD8\xC6\xFE\xBA\xD1" };  /* 0x003512C8 */

namespace GT2KMSGGTMODE {

/* One entry: "ALREADY_YOU_BOUGHT" -> "購入済" (gp-0x6738 / gp-0x6728).
 * Caller: T039_static_init. */
// FUNC 0x00127790 GT2KMSGGTMODE::Message::Message
Message::Message()
    : GT2KRealTimeText("GTMODE", g_003512B8, g_003512C8, 1)
{
}

/* index.txt: GT2KMSGGTMODE::Message::virtual_0 (slot 0).  The vptr (+0x14)
 * = 0x003129C0, then GT2KRealTimeText::~GT2KRealTimeText(this, flags). */
// FUNC 0x001277D8 GT2KMSGGTMODE::Message::~Message
Message::~Message()
{
    /* implicit: ~GT2KRealTimeText (base destructor) */
}

} /* namespace GT2KMSGGTMODE */

/* The GTMODE table (.data 0x0029AD40: egcs puts a public object that has a
 * constructor in .data, zero-filled, not in .bss). */
GT2KMSGGTMODE::Message g_0029AD40;         /* 0x0029AD40 */

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_0029AD40 (.dtors entry 0x0028F1E8, .ctors entry 0x0028F050), emitted at
 * the end of the unit, the destructor first: ~Message(2) / Message() on
 * it.  egcs names them after the file's first public object assembled, now
 * the key table g_003512B8 (it named them after Message's constructor while
 * the tables were not defined here). */
// FUNC 0x00127800 T039_static_destroy (_GLOBAL_$D$g_003512B8)
/* implicit: ~Message (the static destructor of g_0029AD40) */

// FUNC 0x00127828 T039_static_init (_GLOBAL_$I$g_003512B8)
/* implicit: Message (the static initialiser of g_0029AD40) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00127848 __tfQ213GT2KMSGGTMODE7Message
extern "C" void *__tfQ213GT2KMSGGTMODE7Message(void)
{
    if (__tiQ213GT2KMSGGTMODE7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ213GT2KMSGGTMODE7Message, "Q213GT2KMSGGTMODE7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ213GT2KMSGGTMODE7Message;
}
#endif
