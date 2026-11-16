/* T114 - GT2KMSGSHORTMENU::Message, the "SHORTMENU" real-time text table, and its
 * global instance g_002B1B48.  0x00174568..0x00174678.
 *
 * 6 key -> text entries: the words of the short menu result list: SESSION_BRFORE / SESSION_AFTER
 * (EUC-JP "No." / "race" around a session number), LAPS_SINGLE "Lap",
 * LAPS_PLURAL "Laps", POINT_SINGLE "pt", POINT_PLURAL "pts". */
/* Its own header only: retail's message units have no type name but their
 * own (the keys' strings, the table's name, the vtable, then at once the
 * Message's name, which __tf loads). */
#include "game/data/messages/T114_msg_shortmenu.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* The keys and texts (.data 0x002B1B18 / 0x002B1B30), defined first as in the
 * original: their strings open the unit's .rodata (and .sdata), in front
 * of the constructor's name, the vtable and the type name.  Equal
 * literals are one string (egcs shares them within a unit).  Only the
 * tables point at the keys and texts, so the anchors say where the .rodata
 * and the .sdata start. */
// DATA_START 0x00325BA8
// DATA_START 0x00351D20
const char *g_002B1B18[6] = {
    "SESSION_BRFORE",                            /* 0x00325BA8 */
    "SESSION_AFTER",                             /* 0x00325BB8 */
    "LAPS_SINGLE",                               /* 0x00325BC8 */
    "LAPS_PLURAL",                               /* 0x00325BD8 */
    "POINT_SINGLE",                              /* 0x00325BE8 */
    "POINT_PLURAL",                              /* 0x00325BF8 */
};
const char *g_002B1B30[6] = {
    "\302\350",                                  /* 0x00351D20 第 */
    "\300\357",                                  /* 0x00351D28 戦 */
    "Lap",                                       /* 0x00351D30 */
    "Laps",                                      /* 0x00351D38 */
    "pt",                                        /* 0x00351D40 */
    "pts",                                       /* 0x00351D48 */
};

namespace GT2KMSGSHORTMENU {

/* GT2KRealTimeText("SHORTMENU", g_002B1B18, g_002B1B30, 6), then the vptr (+0x14).
 * Caller: the static initialiser of g_002B1B48. */
// FUNC 0x00174568 GT2KMSGSHORTMENU::Message::Message
Message::Message()
    : GT2KRealTimeText("SHORTMENU", g_002B1B18, g_002B1B30, 6)
{
}

/* index.txt: GT2KMSGSHORTMENU::Message::virtual_0 (slot 0). */
// FUNC 0x001745B8 GT2KMSGSHORTMENU::Message::~Message
Message::~Message()
{
    /* egcs: the vptr (+0x14) = 0x00325C18, then
     * GT2KRealTimeText::~GT2KRealTimeText(this, flags). */
    /* implicit: ~GT2KRealTimeText (the base / member destructors) */
}

} /* namespace GT2KMSGSHORTMENU */

/* The global instance (.data 0x002B1B48: egcs puts a public object that has a
 * constructor in .data, zero-filled, not in .bss). */
GT2KMSGSHORTMENU::Message g_002B1B48;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002B1B48 (.dtors entry 0x0028F208, .ctors entry 0x0028F0E0), emitted at the end
 * of the unit, the destructor first: ~Message(2) / Message() on it.
 * Named after the unit's first global, the keys g_002B1B18. */
// FUNC 0x001745E0 T114_static_destroy (_GLOBAL_$D$g_002B1B18)
/* implicit: ~Message (the static destructor of g_002B1B48) */

// FUNC 0x00174608 T114_static_init (_GLOBAL_$I$g_002B1B18)
/* implicit: Message (the static initialiser of g_002B1B48) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00174628 __tfQ216GT2KMSGSHORTMENU7Message
extern "C" void *__tfQ216GT2KMSGSHORTMENU7Message(void)
{
    if (__tiQ216GT2KMSGSHORTMENU7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ216GT2KMSGSHORTMENU7Message, "Q216GT2KMSGSHORTMENU7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ216GT2KMSGSHORTMENU7Message;
}
#endif
