/* T095_msg_memorycard.h - GT2KMSGMEMORYCARDTEXT::Message, the
 * "MEMORYCARDTEXT" real-time text table (15 entries, the memory card
 * screen texts: NO_MEMORYCARD "NO MEMORY CARD(PS2)", Unformatted, Reading
 * "Reading...", Broken, CPYes, CPNo, KbFree "KB Free", Lap, Laps,
 * CorruptFile and the file kinds GT3GameData, GT3ReplayData,
 * GT3LicenseReplayData, GT3BestLapReplayData, GT3SettingData), translation
 * unit T095, unit u040, 0x00174678..0x00174788.  The global instance is
 * g_002B1CA8 (T095 .data, defined there); the name "MEMORYCARDTEXT" is at 0x00325DA8. */
#ifndef GAME_T095_MSG_MEMORYCARD_H
#define GAME_T095_MSG_MEMORYCARD_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"            /* base GT2KRealTimeText */

namespace GT2KMSGMEMORYCARDTEXT {

/* vtable 0x00325DB8 (1 slot).  typeinfo __tfQ221GT2KMSGMEMORYCARDTEXT7Message
 * 0x00174738 (si, base GT2KRealTimeText; storage 0x004B1DE0).  No data of
 * its own: size 0x18 (GT2KRealTimeText's vptr +0x14). */
class Message : public GT2KRealTimeText {
public:
    Message();                                  /* 0x00174678 GT2KRealTimeText("MEMORYCARDTEXT",
                                                   g_002B1C28, g_002B1C68, 15) */
    virtual ~Message();                         /* [0] +0x0C 0x001746C8 */
};

} /* namespace GT2KMSGMEMORYCARDTEXT */

EGCS_SIZE(GT2KMSGMEMORYCARDTEXT::Message, 0x18);

extern const char *g_002B1C28[15];              /* 0x002B1C28 the MEMORYCARDTEXT keys */
/* The texts, in the key order above: [0] "NO MEMORY CARD(PS2)" (u027's
 * memory card items print it, T069) ... [14] GT3SettingData. */
extern const char *g_002B1C68[15];              /* 0x002B1C68 the MEMORYCARDTEXT texts */
extern GT2KMSGMEMORYCARDTEXT::Message g_002B1CA8;   /* 0x002B1CA8 (T095 .data, defined there) */

/* 0x001746F0 (.dtors 0x0028F204) / 0x00174718 (.ctors 0x0028F0C8): egcs's static destructor
 * and initialiser of g_002B1CA8 (T095 defines it, and the tables):
 * _GLOBAL_$D$g_002B1C28 (~Message(2)) and _GLOBAL_$I$g_002B1C28 (Message()). */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tfQ221GT2KMSGMEMORYCARDTEXT7Message(void);      /* 0x00174738 */
extern u32 __tiQ221GT2KMSGMEMORYCARDTEXT7Message[3];    /* 0x004B1DE0 (__si_type_info) */
}
#endif

#endif /* GAME_T095_MSG_MEMORYCARD_H */
