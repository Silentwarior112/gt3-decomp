/* T187 - GT2KMSGRACEMSG::Message, the "RACEMSG" real-time text table (one
 * key / text: SessionBest -> "SESSION BEST", gp-0x5670 / gp-0x5668), and
 * its global instance g_002C45A8.  0x001AE990..0x001AEA98.  The text is
 * also RaceEntryCar's default driver name (T121 declares it).
 *
 * Data: .rodata 0x00331238..0x00331286: the key and the text (only the
 * tables point at them: DATA_START), the vtable, the one type name
 * (Message: retail's message units have no type name but their own).
 * .sdata 0x00352380..0x00352398: the keys, the texts (arrays of one
 * pointer: 8-aligned, 4 zero bytes each), the constructor's "RACEMSG".
 * .data: g_002C45A8, then the frame info. */
// DATA_START 0x00331238
#include "game/data/messages/T187_msg_race_msg.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/effects/replay_effects_internal.h"                           /* g_00352380, g_00352388 */

/* The keys and texts, defined first as in the original: their strings open
 * the unit's .rodata, in front of the vtable and the type name; the tables
 * open its .sdata, in front of the constructor's "RACEMSG". */
const char *g_00352380[1] = { "SessionBest" };      /* 0x00352380 (the string 0x00331238) */
const char *g_00352388[1] = { "SESSION BEST" };     /* 0x00352388 (the string 0x00331248) */

namespace GT2KMSGRACEMSG {

/* Caller: the static initialiser of g_002C45A8. */
// FUNC 0x001AE990 GT2KMSGRACEMSG::Message::Message
Message::Message()
    : GT2KRealTimeText("RACEMSG", g_00352380, g_00352388, 1)
{
}

// FUNC 0x001AE9D8 GT2KMSGRACEMSG::Message::~Message
/* Slot 0 (index.txt: GT2KMSGRACEMSG::Message::virtual_0).  The vptr
 * (+0x14) = 0x00331258, then GT2KRealTimeText::~GT2KRealTimeText(this,
 * flags). */
Message::~Message()
{
    /* implicit: ~GT2KRealTimeText (base destructor) */
}

} /* namespace GT2KMSGRACEMSG */

/* The global instance (.data 0x002C45A8: egcs puts a public object that has
 * a constructor in .data, zero-filled, not in .bss). */
GT2KMSGRACEMSG::Message g_002C45A8;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002C45A8 (.dtors entry 0x0028F228, .ctors entry 0x0028F148), emitted
 * at the end of the unit, the destructor first: ~Message(2) / Message() on
 * it.  Named after the unit's first global, the keys g_00352380. */
// FUNC 0x001AEA00 T187_static_destroy (_GLOBAL_$D$g_00352380)
/* implicit: ~Message (the static destructor of g_002C45A8) */

// FUNC 0x001AEA28 T187_static_init (_GLOBAL_$I$g_00352380)
/* implicit: Message (the static initialiser of g_002C45A8) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001AEA48 __tfQ214GT2KMSGRACEMSG7Message
/* __si_type_info (0x004B23C0), base __ti16GT2KRealTimeText. */
extern "C" void *__tfQ214GT2KMSGRACEMSG7Message(void)
{
    if (__tiQ214GT2KMSGRACEMSG7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ214GT2KMSGRACEMSG7Message, "Q214GT2KMSGRACEMSG7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ214GT2KMSGRACEMSG7Message;
}
#endif
