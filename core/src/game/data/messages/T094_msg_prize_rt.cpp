/* T094 - GT2KMSGPRIZE_RT_MSG::Message, the "PRIZE_RT_MSG" real-time text table, and its
 * global instance g_002B1A38.  0x00174460..0x00174568.
 *
 * 1 key -> text entries: BONUS_PREFIX -> EUC-JP "prize money" (the tables are in .sdata,
 * gp-0x5CE8 / gp-0x5CD8). */
/* Its own header only: retail's message units have no type name but their
 * own (the keys' strings, the table's name, the vtable, then at once the
 * Message's name, which __tf loads). */
#include "game/data/messages/T094_msg_prize_rt.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* The keys and texts (.sdata 0x00351D08 / 0x00351D18), defined first as in the
 * original: their strings open the unit's .rodata (and .sdata), in front
 * of the constructor's name, the vtable and the type name.  Equal
 * literals are one string (egcs shares them within a unit).  Only the
 * table points at the key, so the anchor says where the .rodata starts. */
// DATA_START 0x00325B58
const char *g_00351D08[1] = {
    "BONUS_PREFIX",                              /* 0x00325B58 */
};
const char *g_00351D18[1] = {
    "\276\336\266\342",                          /* 0x00351D10 賞金 */
};

namespace GT2KMSGPRIZE_RT_MSG {

/* GT2KRealTimeText("PRIZE_RT_MSG", g_00351D08, g_00351D18, 1), then the vptr (+0x14).
 * Caller: the static initialiser of g_002B1A38. */
// FUNC 0x00174460 GT2KMSGPRIZE_RT_MSG::Message::Message
Message::Message()
    : GT2KRealTimeText("PRIZE_RT_MSG", g_00351D08, g_00351D18, 1)
{
}

/* index.txt: GT2KMSGPRIZE_RT_MSG::Message::virtual_0 (slot 0). */
// FUNC 0x001744A8 GT2KMSGPRIZE_RT_MSG::Message::~Message
Message::~Message()
{
    /* egcs: the vptr (+0x14) = 0x00325B78, then
     * GT2KRealTimeText::~GT2KRealTimeText(this, flags). */
    /* implicit: ~GT2KRealTimeText (the base / member destructors) */
}

} /* namespace GT2KMSGPRIZE_RT_MSG */

/* The global instance (.data 0x002B1A38: egcs puts a public object that has a
 * constructor in .data, zero-filled, not in .bss). */
GT2KMSGPRIZE_RT_MSG::Message g_002B1A38;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002B1A38 (.dtors entry 0x0028F200, .ctors entry 0x0028F0C4), emitted at the end
 * of the unit, the destructor first: ~Message(2) / Message() on it.
 * Named after the unit's first global, the keys g_00351D08. */
// FUNC 0x001744D0 T094_static_destroy (_GLOBAL_$D$g_00351D08)
/* implicit: ~Message (the static destructor of g_002B1A38) */

// FUNC 0x001744F8 T094_static_init (_GLOBAL_$I$g_00351D08)
/* implicit: Message (the static initialiser of g_002B1A38) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00174518 __tfQ219GT2KMSGPRIZE_RT_MSG7Message
extern "C" void *__tfQ219GT2KMSGPRIZE_RT_MSG7Message(void)
{
    if (__tiQ219GT2KMSGPRIZE_RT_MSG7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ219GT2KMSGPRIZE_RT_MSG7Message, "Q219GT2KMSGPRIZE_RT_MSG7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ219GT2KMSGPRIZE_RT_MSG7Message;
}
#endif
