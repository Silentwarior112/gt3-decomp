/* T093 - GT2KMSGSETTING::Message, the "SETTING" real-time text table, and its
 * global instance g_002B1958.  0x00174350..0x00174460.
 *
 * 5 key -> text entries: the units of the setting pages: UNIT_KGF_MM "kgf/mm", UNIT_MM "mm",
 * UNIT_LEVEL "Level", UNIT_DEGREE EUC-JP 0xA1EB "°", CUSTOM "CUSTOM". */
/* Its own header only: retail's message units have no type name but their
 * own (the keys' strings, the table's name, the vtable, then at once the
 * Message's name, which __tf loads). */
#include "game/data/messages/T093_msg_setting.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* The keys and texts (.data 0x002B1928 / 0x002B1940), defined first as in the
 * original: their strings open the unit's .rodata (and .sdata), in front
 * of the constructor's name, the vtable and the type name.  Equal
 * literals are one string (egcs shares them within a unit).  Only the
 * tables point at the keys, so the anchors say where the .rodata and the
 * .sdata start. */
// DATA_START 0x00325AF8
// DATA_START 0x00351CD0
const char *g_002B1928[5] = {
    "UNIT_KGF_MM",                               /* 0x00325AF8 */
    "UNIT_MM",                                   /* 0x00351CD0 */
    "UNIT_LEVEL",                                /* 0x00325B08 */
    "UNIT_DEGREE",                               /* 0x00325B18 */
    "CUSTOM",                                    /* 0x00351CD8 */
};
const char *g_002B1940[5] = {
    "kgf/mm",                                    /* 0x00351CE0 */
    "mm",                                        /* 0x00351CE8 */
    "Level",                                     /* 0x00351CF0 */
    "\241\353",                                  /* 0x00351CF8 ° */
    "CUSTOM",                                    /* 0x00351CD8 */
};

namespace GT2KMSGSETTING {

/* GT2KRealTimeText("SETTING", g_002B1928, g_002B1940, 5), then the vptr (+0x14).
 * Caller: the static initialiser of g_002B1958. */
// FUNC 0x00174350 GT2KMSGSETTING::Message::Message
Message::Message()
    : GT2KRealTimeText("SETTING", g_002B1928, g_002B1940, 5)
{
}

/* index.txt: GT2KMSGSETTING::Message::virtual_0 (slot 0). */
// FUNC 0x001743A0 GT2KMSGSETTING::Message::~Message
Message::~Message()
{
    /* egcs: the vptr (+0x14) = 0x00325B28, then
     * GT2KRealTimeText::~GT2KRealTimeText(this, flags). */
    /* implicit: ~GT2KRealTimeText (the base / member destructors) */
}

} /* namespace GT2KMSGSETTING */

/* The global instance (.data 0x002B1958: egcs puts a public object that has a
 * constructor in .data, zero-filled, not in .bss). */
GT2KMSGSETTING::Message g_002B1958;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002B1958 (.dtors entry 0x0028F1FC, .ctors entry 0x0028F0C0), emitted at the end
 * of the unit, the destructor first: ~Message(2) / Message() on it.
 * Named after the unit's first global, the keys g_002B1928. */
// FUNC 0x001743C8 T093_static_destroy (_GLOBAL_$D$g_002B1928)
/* implicit: ~Message (the static destructor of g_002B1958) */

// FUNC 0x001743F0 T093_static_init (_GLOBAL_$I$g_002B1928)
/* implicit: Message (the static initialiser of g_002B1958) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00174410 __tfQ214GT2KMSGSETTING7Message
extern "C" void *__tfQ214GT2KMSGSETTING7Message(void)
{
    if (__tiQ214GT2KMSGSETTING7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ214GT2KMSGSETTING7Message, "Q214GT2KMSGSETTING7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ214GT2KMSGSETTING7Message;
}
#endif
