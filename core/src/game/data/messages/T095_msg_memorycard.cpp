/* T095 - GT2KMSGMEMORYCARDTEXT::Message, the "MEMORYCARDTEXT" real-time text table, and its
 * global instance g_002B1CA8.  0x00174678..0x00174788.
 *
 * 15 key -> text entries: the memory card screen texts (NO_MEMORYCARD "NO MEMORY CARD(PS2)",
 * Unformatted, Reading "Reading...", Broken, CPYes / CPNo, KbFree "KB Free",
 * Lap / Laps, CorruptFile and the five GT3*Data file kinds, in EUC-JP). */
/* Its own header only: retail's message units have no type name but their
 * own (the keys' strings, the table's name, the vtable, then at once the
 * Message's name, which __tf loads). */
#include "game/data/messages/T095_msg_memorycard.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* The keys and texts (.data 0x002B1C28 / 0x002B1C68), defined first as in the
 * original: their strings open the unit's .rodata (and .sdata), in front
 * of the constructor's name, the vtable and the type name.  Equal
 * literals are one string (egcs shares them within a unit).  Only the
 * tables point at the keys and texts, so the anchors say where the .rodata
 * and the .sdata start. */
// DATA_START 0x00325C48
// DATA_START 0x00351D50
const char *g_002B1C28[15] = {
    "NO_MEMORYCARD",                             /* 0x00325C48 */
    "Unformatted",                               /* 0x00325C58 */
    "Reading",                                   /* 0x00351D50 */
    "Broken",                                    /* 0x00351D58 */
    "CPYes",                                     /* 0x00351D60 */
    "CPNo",                                      /* 0x00351D68 */
    "KbFree",                                    /* 0x00351D70 */
    "Lap",                                       /* 0x00351D78 */
    "Laps",                                      /* 0x00351D80 */
    "CorruptFile",                               /* 0x00325C68 */
    "GT3GameData",                               /* 0x00325C78 */
    "GT3ReplayData",                             /* 0x00325C88 */
    "GT3LicenseReplayData",                      /* 0x00325C98 */
    "GT3BestLapReplayData",                      /* 0x00325CB0 */
    "GT3SettingData",                            /* 0x00325CC8 */
};
const char *g_002B1C68[15] = {
    "NO MEMORY CARD(PS2)",                       /* 0x00325CD8 */
    "Unformatted",                               /* 0x00325C58 */
    "Reading...",                                /* 0x00325CF0 */
    "Broken",                                    /* 0x00351D58 */
    "yes",                                       /* 0x00351D88 */
    "no",                                        /* 0x00351D90 */
    "KB Free",                                   /* 0x00351D98 */
    "lap",                                       /* 0x00351DA0 */
    "laps",                                      /* 0x00351DA8 */
    "\307\313\302\273\245\307\241\274\245\277",  /* 0x00325D00 破損データ */
    "\243\307\243\324\243\263\245\262\241\274\245\340\245\307\241\274\245\277", /* 0x00325D10 ＧＴ３ゲームデータ */
    "\243\307\243\324\243\263\245\352\245\327\245\354\245\244\245\307\241\274\245\277\241\241", /* 0x00325D28 ＧＴ３リプレイデータ */
    "\243\307\243\324\243\263\245\351\245\244\245\273\245\363\245\271\245\352\245\327\245\354\245\244\245\307\241\274\245\277", /* 0x00325D40 ＧＴ３ライセンスリプレイデータ */
    "\243\307\243\324\243\263\245\331\245\271\245\310\245\351\245\303\245\327\245\352\245\327\245\354\245\244\245\307\241\274\245\277", /* 0x00325D60 ＧＴ３ベストラップリプレイデータ */
    "\243\307\243\324\243\263\245\273\245\303\245\306\245\243\245\363\245\260\245\307\241\274\245\277", /* 0x00325D88 ＧＴ３セッティングデータ */
};

namespace GT2KMSGMEMORYCARDTEXT {

/* GT2KRealTimeText("MEMORYCARDTEXT", g_002B1C28, g_002B1C68, 15), then the vptr (+0x14).
 * Caller: the static initialiser of g_002B1CA8. */
// FUNC 0x00174678 GT2KMSGMEMORYCARDTEXT::Message::Message
Message::Message()
    : GT2KRealTimeText("MEMORYCARDTEXT", g_002B1C28, g_002B1C68, 15)
{
}

/* index.txt: GT2KMSGMEMORYCARDTEXT::Message::virtual_0 (slot 0). */
// FUNC 0x001746C8 GT2KMSGMEMORYCARDTEXT::Message::~Message
Message::~Message()
{
    /* egcs: the vptr (+0x14) = 0x00325DB8, then
     * GT2KRealTimeText::~GT2KRealTimeText(this, flags). */
    /* implicit: ~GT2KRealTimeText (the base / member destructors) */
}

} /* namespace GT2KMSGMEMORYCARDTEXT */

/* The global instance (.data 0x002B1CA8: egcs puts a public object that has a
 * constructor in .data, zero-filled, not in .bss). */
GT2KMSGMEMORYCARDTEXT::Message g_002B1CA8;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002B1CA8 (.dtors entry 0x0028F204, .ctors entry 0x0028F0C8), emitted at the end
 * of the unit, the destructor first: ~Message(2) / Message() on it.
 * Named after the unit's first global, the keys g_002B1C28. */
// FUNC 0x001746F0 T095_static_destroy (_GLOBAL_$D$g_002B1C28)
/* implicit: ~Message (the static destructor of g_002B1CA8) */

// FUNC 0x00174718 T095_static_init (_GLOBAL_$I$g_002B1C28)
/* implicit: Message (the static initialiser of g_002B1CA8) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00174738 __tfQ221GT2KMSGMEMORYCARDTEXT7Message
extern "C" void *__tfQ221GT2KMSGMEMORYCARDTEXT7Message(void)
{
    if (__tiQ221GT2KMSGMEMORYCARDTEXT7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ221GT2KMSGMEMORYCARDTEXT7Message, "Q221GT2KMSGMEMORYCARDTEXT7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ221GT2KMSGMEMORYCARDTEXT7Message;
}
#endif
