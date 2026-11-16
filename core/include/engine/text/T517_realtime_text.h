/* T517_realtime_text.h - GT2KRealTimeText, the texts that change with the
 * language at run time (translation unit T517, unit u112), 0x002454E0..0x002457C8.
 *
 * Every table (a static GT2KRealTimeText subclass per message group, e.g.
 * GT2KMSGGTMODE::Message) links itself into the list 0x00353024 (gp-0x49CC).
 * text/realtime.dat (0x00353018, gp-0x49D8, once loaded) holds a sorted index
 * of "<table>_<key>" names and one block of texts per language;
 * realtime_set_language points each table's texts[] into the block of a
 * language (an empty text keeps the built-in default). */
#ifndef GAME_T517_REALTIME_TEXT_H
#define GAME_T517_REALTIME_TEXT_H

#include "game/game.h"

/* A named table of count key -> text pairs, pushed on the front of the list
 * by its constructor (never unlinked).  vtable 0x00343C88 (1 slot), typeinfo
 * __tf16GT2KRealTimeText 0x00245788 (__user_type_info 0x003534D0, gp-0x4520).
 * Data, then the vptr at +0x14 (egcs); size 0x18. */
class GT2KRealTimeText {
public:
    /* texts[] holds the defaults until a language is applied. */
    GT2KRealTimeText(const char *name, const char **keys, const char **texts,
                     int count);                /* 0x00245728 */
    virtual ~GT2KRealTimeText();                /* [0] +0x0C 0x00245758 */

    GT2KRealTimeText *next;                     /* 0x00 the list head before this one */
    const char  *name;                          /* 0x04 the table's name */
    const char **keys;                          /* 0x08 [count] */
    const char **texts;                         /* 0x0C [count] the current language's texts */
    int          count;                         /* 0x10 */
    /*           vptr                              0x14 */
};
EGCS_SIZE(GT2KRealTimeText, 0x18);

/* Once: reads text/realtime.dat (unpacked, from vol_device_object), turns its
 * offsets into pointers, applies the current language (0x00352FD8). */
void realtime_load(void);                       /* 0x00245588 */
/* Every key of every table: the text of "<name>_<key>" in language lang's
 * block, if it is in the index and not empty.  The file must be loaded. */
void realtime_set_language(int lang);           /* 0x00245628 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tf16GT2KRealTimeText(void);             /* 0x00245788 */
extern u32 __ti16GT2KRealTimeText[2];           /* 0x003534D0 gp-0x4520 __user_type_info */
}
#endif

#endif /* GAME_T517_REALTIME_TEXT_H */
