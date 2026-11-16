/* T231 - gt2k_menu::TextBox: the text field of the on-screen keyboard.
 * 0x001C3828..0x001C4210.
 *
 * The page's code sets the maximum length, the initial text and the output
 * buffer (text_box_set_max_length / _initial / _output, all three called by
 * run_text_input, T227) before the page is built; the TextBox (hook
 * create_text_box) takes the initial text, is the one current TextBox while
 * it lives, and writes its text into the output buffer when it is
 * destroyed.  The keys of the keyboard are buttons whose execute function
 * (text_box_key) inserts the first character of their page item's text.
 * The text is kept one character per int: an SJIS lead byte (bit 7 set)
 * and the next byte as (lead << 8) | next.  Cancel deletes before the
 * cursor; R1 / L1 (auto-repeated) move the cursor.
 *
 * Data (retail .rodata 0x00335C00..0x00335D80): TextBox's vtable, then
 * SmartIndexT<int>'s (the index member's inline constructor needs it), the
 * type names TextBox, SmartIndexT<int>, the operator list classes (Item's),
 * Operator: no Button.  .sdata 0x00352628..0x00352644: the four statics
 * below (zero), "cursor", "%s". */
#include "game/menu/framework/page_layout_internal.h"
#include "game/menu/framework/T231_text_box.h"
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (setResource, drawImage, beginText) */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (getRepeat), MENU_BUTTON_L1 / R1 */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the hook's result) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuText, ITEM_HAS_TEXT */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se, MENU_SE_CURSOR */
#include "game/menu/framework/T244_button_data.h"          /* gt2k_menu::ButtonData (text_box_key) */
#include "engine/input/T474_control.h"              /* Control (getButtonsOn), CONTROL_CANCEL */
#include "engine/text/T496_font.h"                 /* Font::print, the system font g_002F1968 */
#include "engine/text/T499_font_metrics.h"         /* text_char_height, text_string_width */
#include "engine/pgl/G001_flare.h"                /* pgl_color */
#include "game/common/T494_text_utils.h"           /* draw_rect */
#ifndef BOOT_OLD_GCC
#include "game/common/T017_smart_index.h"          /* __tiQ29gt2k_menut11SmartIndexT1Zi (GCC 14's __tf below) */
#endif

/* Defined with an initialiser: retail has them in .sdata (zero) in front of
 * the file's literals "cursor" / "%s", where egcs writes initialised data
 * defined before the functions (uninitialised globals would come after the
 * literals). */
gt2k_menu::TextBox *text_box_current = 0;       /* 0x00352628 */
int                 text_box_max_length = 0;    /* 0x0035262C */
const char         *text_box_initial_text = 0;  /* 0x00352630 */
char               *text_box_output = 0;        /* 0x00352634 */

namespace gt2k_menu {

/* Inserts c at pos, the characters after it move up (the last of the n is
 * lost); with the cursor sound.  Nothing if pos is not inside the buffer. */
// FUNC 0x001C3828 gt2k_menu::text_insert
void text_insert(int *buf, int n, int pos, int c)
{
    if (pos < n) {
        menu_play_se(MENU_SE_CURSOR);
        int i;
        for (i = n - 1; pos < i; i--)
            buf[i] = buf[i - 1];
        buf[i] = c;
    }
}

/* Deletes the character at pos (pos < 0: nothing); the sound if the text
 * is not empty.  The last of the n becomes 0. */
// FUNC 0x001C38C0 gt2k_menu::text_delete
void text_delete(int *buf, int n, int pos)
{
    if (pos >= 0) {
        if (buf[0] != 0)
            menu_play_se(MENU_SE_CURSOR);
        int i;
        for (i = pos; i < n - 1; i++)
            buf[i] = buf[i + 1];
        buf[i] = 0;
    }
}

// FUNC 0x001C3960 gt2k_menu::text_length
int text_length(const int *buf, int n)
{
    int len = 0;
    while (len < n && buf[len] != 0)
        len++;
    return len;
}

/* The inline SmartIndexT<int>() of `index' stores {0, 0, 1, 0, 0} and its
 * vtable (0x0030D998); then the index is set to [0, 0x81), clamped, and to
 * the length of the initial text.  The maximum length, if set, narrows the
 * range to [0, max + 1).  The clamp is set through the inline setClamp
 * (stored through &index); the clearing loop counts up (loop.c reverses
 * it into retail's count-down, &chars[0] computed first). */
// FUNC 0x001C3990 gt2k_menu::TextBox::TextBox
TextBox::TextBox(MenuControl *control_, PageLayout *layout, MenuLayoutItem *item, MenuTexture *tex)
    : Item(item, tex), control(control_)
{
    index.setRange(0, TEXT_BOX_MAX_CHARS + 1);
    index.set(0);
    index.setClamp(true);

    MenuLayoutItem *cursorItem = layout->findHiddenItem("cursor");
    if (cursorItem != 0) {
        cursor.setResource(cursorItem, layout->getTexture(cursorItem));
        hasCursor = 1;
    } else {
        hasCursor = 0;
    }

    for (int i = 0; i <= TEXT_BOX_MAX_CHARS; i++)
        chars[i] = 0;

    text_box_current = this;
    if (text_box_max_length > 0)
        index.setRange(0, text_box_max_length + 1);
    if (text_box_initial_text != 0) {
        text_decode(text_box_initial_text, chars, strlen(text_box_initial_text));
        int len = 0;
        while (chars[len] != 0)
            len++;
        index.set(len);
    }
}

/* Writes the text into the output buffer (at most the maximum length of
 * characters: 0 if it was never set, an empty string then) and clears the
 * four statics; then cursor's ~Item and ~Item, operator delete (0x001BF458)
 * if flags & 1. */
// FUNC 0x001C3B88 gt2k_menu::TextBox::~TextBox
TextBox::~TextBox()
{
    if (text_box_output != 0)
        text_encode(chars, text_box_output, text_box_max_length);
    text_box_initial_text = 0;
    text_box_output = 0;
    text_box_max_length = 0;
    text_box_current = 0;
    /* implicit: ~Item (base destructor), operator delete (deleting destructor) */
}

/* At the cursor, if there is room; the cursor moves after it.  The index
 * moves through a pointer to it, here and in virtual_1: retail calls
 * SmartIndexT::move through the vtable (slot 3). */
// FUNC 0x001C3C18 gt2k_menu::TextBox::insertChar
void TextBox::insertChar(int c)
{
    int maxLength = (text_box_max_length > 0) ? text_box_max_length : TEXT_BOX_MAX_CHARS;
    if (text_length(chars, TEXT_BOX_MAX_CHARS) < maxLength) {
        SmartIndexT<int> *idx = &index;
        text_insert(chars, TEXT_BOX_MAX_CHARS, idx->value, c);
        idx->move(1);
    }
}

/* Cancel (CONTROL_CANCEL, newly pressed) deletes the character before the
 * cursor; R1 / L1 (auto-repeat after 30 frames, every 3) move the cursor
 * right (not past the end of the text) / left, with the cursor sound. */
// FUNC 0x001C3CB0 gt2k_menu::TextBox::virtual_1
void TextBox::virtual_1()
{
    Item::virtual_1();
    if (control->getButtonsOn() & CONTROL_CANCEL) {
        SmartIndexT<int> *idx = &index;
        idx->move(-1);
        text_delete(chars, TEXT_BOX_MAX_CHARS, idx->value);
    } else if (control->getRepeat(MENU_BUTTON_R1, TEXT_BOX_REPEAT_WAIT, TEXT_BOX_REPEAT_STEP) != 0) {
        SmartIndexT<int> *idx = &index;
        if (chars[idx->value] != 0 && idx->move(1) != 0)
            menu_play_se(MENU_SE_CURSOR);
    } else if (control->getRepeat(MENU_BUTTON_L1, TEXT_BOX_REPEAT_WAIT, TEXT_BOX_REPEAT_STEP) != 0) {
        SmartIndexT<int> *idx = &index;
        if (idx->move(-1) != 0)
            menu_play_se(MENU_SE_CURSOR);
    }
}

/* The first n characters (or up to the terminator) as a string: a code
 * from 0x100 up is two bytes, lead byte first. */
// FUNC 0x001C3DD0 gt2k_menu::text_encode
void text_encode(const int *chars, char *out, int n)
{
    int len = 0;
    for (int i = 0; i < n && chars[i] != 0; i++) {
        if (chars[i] >= 0x100) {
            out[len++] = chars[i] >> 8;
            out[len++] = chars[i];
        } else {
            out[len++] = chars[i];
        }
    }
    out[len] = '\0';
}

/* At most n bytes of s (a lead byte takes its next byte with it, even past
 * the n-th). */
// FUNC 0x001C3E60 gt2k_menu::text_decode
void text_decode(const char *s, int *chars, int n)
{
    int len = 0;
    for (int i = 0; i < n && s[i] != '\0'; i++) {
        int c = (s8)s[i];
        if (c & 0x80) {
            chars[len++] = ((c & 0xFF) << 8) | (u8)s[i + 1];
            i++;
        } else {
            chars[len++] = c;
        }
    }
    chars[len] = 0;
}

/* The cursor after the text before it, half a character height down: the
 * hidden item "cursor", else an 8 x 8 white quad. */
// FUNC 0x001C3EF0 gt2k_menu::TextBox::drawCursor
void TextBox::drawCursor()
{
    char before[TEXT_BOX_STRING_SIZE];      /* sp+0 (0x101 bytes; the frame has 0x110) */
    text_encode(chars, before, index.value);
    f32 cursorX = textX + text_string_width(&g_002F1968, before);
    f32 cursorY = textY + (f32)text_char_height(&g_002F1968) * 0.5f;
    if (hasCursor != 0) {
        cursor.x.set(cursorX);
        cursor.y.set(cursorY);
        cursor.virtual_2();
    } else {
        pgl_color(TEXT_BOX_CURSOR_COLOR);
        draw_rect(cursorX, cursorY, 8.0f, 8.0f);
    }
}

/* The picture, the text (if the item has text) in the item's font and
 * place, then the cursor. */
// FUNC 0x001C3FD8 gt2k_menu::TextBox::virtual_2
void TextBox::virtual_2()
{
    char text[TEXT_BOX_STRING_SIZE];        /* sp+0, as in drawCursor */
    drawImage();
    if (res->flags & ITEM_HAS_TEXT) {
        beginText();
        text_encode(chars, text, TEXT_BOX_MAX_CHARS);
        g_002F1968.print("%s", text);
        endText();
    }
    drawCursor();
}

/* Hook: the TextBox (ctx = the page's MenuControl). */
// FUNC 0x001C4050 gt2k_menu::create_text_box
Transitive *create_text_box(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                            int p8, int pC, int p10, int p14)
{
    MenuTexture *tex = layout->getTexture(item);
    return new TextBox((MenuControl *)ctx, layout, item, tex);  /* implicit: Object::operator delete (if the constructor throws) */
}

/* Execute function of a key button (ExecuteFuncData, T211): inserts the
 * first character of the text of the button's page item (a space for an
 * empty text) into the current TextBox (which must exist). */
// FUNC 0x001C40F0 gt2k_menu::text_box_key
void text_box_key(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    MenuLayoutItem *key = layout->findPageItem(rec->name);
    if (key == 0)
        return;
    MenuText *text = layout->getText(key);
    if (text == 0)
        return;
    const char *s = text->text;
    int c = (s8)s[0];
    if (c == 0)
        c = ' ';
    else if (c & 0x80)
        c = ((c & 0xFF) << 8) | (u8)s[1];
    text_box_current->insertChar(c);
}

// FUNC 0x001C4168 gt2k_menu::text_box_set_max_length
void text_box_set_max_length(int n)
{
    text_box_max_length = n;
}

// FUNC 0x001C4170 gt2k_menu::text_box_set_output
void text_box_set_output(char *buf)
{
    text_box_output = buf;
}

// FUNC 0x001C4178 gt2k_menu::text_box_set_initial
void text_box_set_initial(const char *s)
{
    text_box_initial_text = s;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001C4180 __tfQ29gt2k_menu7TextBox
extern "C" void *__tfQ29gt2k_menu7TextBox(void)
{
    if (__tiQ29gt2k_menu7TextBox[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu7TextBox, "Q29gt2k_menu7TextBox",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu7TextBox;
}

/* Compiler-generated typeinfo function of the template instance
 * SmartIndexT<int> (__user_type_info, no bases): T231's copy, no callers
 * (T017's 0x00103690 is the one called).  Weak so that the copies link. */
// FUNC 0x001C41D0 __tfQ29gt2k_menut11SmartIndexT1Zi
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut11SmartIndexT1Zi(void)
{
    if (__tiQ29gt2k_menut11SmartIndexT1Zi[0] == 0)
        __rtti_user(__tiQ29gt2k_menut11SmartIndexT1Zi, "Q29gt2k_menut11SmartIndexT1Zi");
    return __tiQ29gt2k_menut11SmartIndexT1Zi;
}
#endif
