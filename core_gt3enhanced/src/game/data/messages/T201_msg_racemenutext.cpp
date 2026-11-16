/* T201 - GT2KMSGRACEMENUTEXT::Message, the "RACEMENUTEXT" real-time text
 * table, and its global instance g_002C5D20.  0x001B27C8..0x001B28D8.
 *
 * 5 key -> text entries (keys g_002C5CF0, texts g_002C5D08): Lap -> "lap",
 * Laps -> "laps", Pt -> "pt", Pts -> "pts", Running -> "Running...": the
 * words of the result board's rows (ResultRankBar::setLaps / setPoints /
 * setRunning read the texts, which realtime_set_language may replace). */
/* The tables' strings open T201's .rodata ("Running...", 0x003321E0) and
 * .sdata ("Lap", 0x00352408); only the tables point at them. */
// DATA_START 0x003321E0
// DATA_START 0x00352408
#include "game/menu/race_menu/race_menu_internal.h"
#include "game/data/messages/T201_msg_racemenutext.h"

/* The keys and the (replaceable) texts, .data 0x002C5CF0 / 0x002C5D08; the
 * short strings in .sdata 0x00352408.., "Running..." first in T201's
 * .rodata (0x003321E0). */
/* The enhanced engine: Lupo 2's sixth entry, Disqualified -> "Disqualified"
 * (L2 0x002DDD60 / 0x002DDD78; menus_b.md 14).  A language file without the
 * key leaves the English text. */
const char *g_002C5CF0[RACEMENUTEXT_COUNT] = {  /* 0x002C5CF0 */
    "Lap", "Laps", "Pt", "Pts", "Running", "Disqualified"
};
const char *g_002C5D08[RACEMENUTEXT_COUNT] = {  /* 0x002C5D08 */
    "lap", "laps", "pt", "pts", "Running...", "Disqualified"
};

namespace GT2KMSGRACEMENUTEXT {

/* GT2KRealTimeText("RACEMENUTEXT", keys, texts, 5 (6)), then the vptr (+0x14)
 * 0x00332200.  Caller: T201_static_init. */
// FUNC 0x001B27C8 GT2KMSGRACEMENUTEXT::Message::Message
Message::Message()
    : GT2KRealTimeText("RACEMENUTEXT", g_002C5CF0, g_002C5D08, RACEMENUTEXT_COUNT)
{
}

/* Slot 0 (index: GT2KMSGRACEMENUTEXT::Message::virtual_0).  The vptr
 * (+0x14) = 0x00332200, then GT2KRealTimeText::~GT2KRealTimeText(this,
 * flags), which does the egcs deleting flag. */
// FUNC 0x001B2818 GT2KMSGRACEMENUTEXT::Message::~Message
Message::~Message()
{
    /* implicit: ~GT2KRealTimeText (base destructor) */
    /* egcs: the vptr, then GT2KRealTimeText::~GT2KRealTimeText(this, flags) */
}

} /* namespace GT2KMSGRACEMENUTEXT */

/* The RACEMENUTEXT table (.data 0x002C5D20: egcs puts a public object that
 * has a constructor in .data, zero-filled). */
GT2KMSGRACEMENUTEXT::Message g_002C5D20;   /* 0x002C5D20 */

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002C5D20 (.dtors entry 0x0028F22C, .ctors entry 0x0028F14C), emitted at
 * the end of the unit, the destructor first: ~Message(2) / Message() on
 * it. */
// FUNC 0x001B2840 T201_static_destroy (_GLOBAL_$D$g_002C5CF0)
/* implicit: ~Message (the static destructor of g_002C5D20) */

// FUNC 0x001B2868 T201_static_init (_GLOBAL_$I$g_002C5CF0)
/* implicit: Message (the static initialiser of g_002C5D20) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated: the __si_type_info __tiQ219GT2KMSGRACEMENUTEXT7Message,
 * base __ti16GT2KRealTimeText (gp-0x4520). */
// FUNC 0x001B2888 __tfQ219GT2KMSGRACEMENUTEXT7Message
extern "C" void *__tfQ219GT2KMSGRACEMENUTEXT7Message(void)
{
    if (__tiQ219GT2KMSGRACEMENUTEXT7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ219GT2KMSGRACEMENUTEXT7Message, "Q219GT2KMSGRACEMENUTEXT7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ219GT2KMSGRACEMENUTEXT7Message;
}
#endif
