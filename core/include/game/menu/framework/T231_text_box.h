/* T231_text_box.h - gt2k_menu::TextBox: the text field of the on-screen
 * keyboard (translation unit T231, unit u068).
 *
 * The page's code sets the maximum length, the initial text and the output
 * buffer (text_box_set_*, all three called by run_text_input, T227) before
 * the page is built; the TextBox (hook create_text_box) takes the initial
 * text, is the one current TextBox while it lives (static 0x00352628), and
 * writes its text into the output buffer when it is destroyed.  The keys of
 * the keyboard are buttons whose execute function (text_box_key) inserts the
 * first character of their page item's text.  The text is kept one
 * character per int: an SJIS lead byte (bit 7 set) and the next byte as
 * (lead << 8) | next.  Cancel deletes before the cursor; two auto-repeated
 * buttons move it.  Users of the hooks: u013 (T097), u027 / u028
 * (memory card). */
#ifndef GAME_T231_TEXT_BOX_H
#define GAME_T231_TEXT_BOX_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item, held by value */
#include "game/common/T251_smart_index.h"          /* SmartIndexT<int>, held by value */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class MenuControl;                          /* T214 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */
struct ButtonData;                          /* T244 */

/* vtable 0x00335C00 (8 slots: [3] Object's, [4] Item's, [5] ImageGZ's, [6] /
 * [7] Transitive's).  typeinfo __tfQ29gt2k_menu7TextBox 0x001C4180 (si, base
 * Item; __si_type_info 0x004B25C0).  T231 also has the out-of-line
 * __tfQ29gt2k_menut11SmartIndexT1Zi 0x001C41D0.  Size 0x424 (operator new in
 * create_text_box 0x001C408C). */
class TextBox : public Item {
public:
    TextBox(MenuControl *control, PageLayout *layout, MenuLayoutItem *item,
            MenuTexture *tex);              /* 0x001C3990 the cursor: the hidden item "cursor"; the
                                               index [0, max + 1), clamped, at the end of the text */
    virtual ~TextBox();                     /* [0] +0x0C 0x001C3B88 the text into the output buffer (at
                                               most the maximum length: an empty string if it was
                                               never set); clears the statics */
    virtual void virtual_1();               /* [1] +0x14 0x001C3CB0 Item's; cancel (0x80) deletes before
                                               the cursor, R1 / L1 (0x10000 / 0x2000, repeat 30 / 3)
                                               move it */
    virtual void virtual_2();               /* [2] +0x1C 0x001C3FD8 the picture, the text, the cursor */
    void insertChar(int c);                 /* 0x001C3C18 at the cursor if there is room (max, else 0x80) */
    void drawCursor();                      /* 0x001C3EF0 after the text before the cursor */

    MenuControl     *control;               /* 0x100 the page's pad (ctx of the hook) */
    Item             cursor;                /* 0x104 hidden item "cursor" */
    int              hasCursor;             /* 0x204 0: an 8 x 8 quad instead */
    int              chars[0x81];           /* 0x208 0-terminated, one character per int */
    SmartIndexT<int> index;                 /* 0x40C the cursor position */
};
EGCS_SIZE(gt2k_menu::TextBox, 0x424);

/* The text helpers (one character per int, n the buffer length). */
void text_insert(int *buf, int n, int pos, int c);     /* 0x001C3828 c at pos (the last one is lost);
                                               menu_play_se(3) */
void text_delete(int *buf, int n, int pos);            /* 0x001C38C0 the character at pos (pos < 0:
                                               nothing); menu_play_se(3) unless empty */
int  text_length(const int *buf, int n);               /* 0x001C3960 */
void text_encode(const int *chars, char *out, int n);  /* 0x001C3DD0 the first n characters -> string */
void text_decode(const char *s, int *chars, int n);    /* 0x001C3E60 at most n bytes of s -> chars */

/* The TextBox hook (a MenuHookFn, T229): ctx is the page's MenuControl;
 * parent and p8..p14 are not used. */
Transitive *create_text_box(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                            int p8, int pC, int p10, int p14);  /* 0x001C4050 */
/* The execute function of a key button: the first character of the text of
 * the page item named by the record (a space if empty) into the current
 * TextBox. */
void text_box_key(void *ctx, ButtonData *rec, int index, PageLayout *layout); /* 0x001C40F0 */
void text_box_set_max_length(int n);        /* 0x001C4168 <= 0: 0x80 */
void text_box_set_output(char *buf);        /* 0x001C4170 gets the text when the TextBox goes */
void text_box_set_initial(const char *s);   /* 0x001C4178 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu7TextBox(void);    /* 0x001C4180 */
#endif

#endif /* GAME_T231_TEXT_BOX_H */
