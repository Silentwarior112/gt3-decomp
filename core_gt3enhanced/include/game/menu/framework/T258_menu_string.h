/* T258_menu_string.h - gt2k_menu::String (translation unit T258, unit u072),
 * 0x001CFAA8..0x001CFE70.
 *
 * A Transitive that prints its text (a copy of up to 0x40 bytes) with the
 * system font (T496) at its screen position: the character size is w x h
 * pixels, colour 0 (alpha scaled), only the first F0 characters (F0 starts at
 * the length, so an operator script can type the text out).  With a split
 * character, the text from that character on is printed in colour 1 (passed
 * to the print function as its format: a quirk of the original). */
#ifndef GAME_T258_MENU_STRING_H
#define GAME_T258_MENU_STRING_H

#include "game/game.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* base gt2k_menu::Transitive */

namespace gt2k_menu {

/* vtable 0x003376F8 (8 slots: [1] / [4]..[7] Transitive's, [3] Object's).
 * typeinfo __tfQ29gt2k_menu6String 0x001CFDD8 (__si_type_info 0x004B26E0,
 * base Transitive).  Size 0x114. */
class String : public Transitive {
public:
    String(const char *text, const char *font);         /* 0x001CFAA8 align = splitChar = 0, the text
                                                           copied, F0 = length = strlen(text) */
    /*  [0] +0x0C 0x001CFE28 the implicit destructor: only ~Transitive, no vptr store */
    virtual void virtual_2();                           /* [2] +0x1C 0x001CFB70 draw: prints the text */

    void setText(const char *text);                     /* 0x001CFB50 strcpy (length / F0 unchanged) */
    void setSplitChar(int c);                           /* 0x001CFDC0 (? name) also align = 0 */
    void setAlign(int a);                               /* 0x001CFDD0 (? name) */

    char        text[0x40];                 /* 0xC4 */
    const char *font;                       /* 0x104 font style ("sw_16" if NULL or empty) */
    int         align;                      /* 0x108 font alignment while printing (12 for the spec texts) */
    int         splitChar;                  /* 0x10C text from it on in colour 1 (0 none) */
    int         length;                     /* 0x110 strlen at construction */
};
EGCS_SIZE(gt2k_menu::String, 0x114);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu6String(void);        /* 0x001CFDD8 */
}
#endif

#endif /* GAME_T258_MENU_STRING_H */
