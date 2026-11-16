/* T258 - gt2k_menu::String (unit u072, 0x001CFAA8..0x001CFE70).
 *
 * A Transitive that prints its text (a copy of up to 0x40 bytes) with the
 * system font at its screen position: the character size is w x h pixels,
 * colour 0 (alpha scaled), only the first F0 characters (F0 starts at the
 * length, so an operator script can type the text out).  With a split
 * character the text from that character on is printed in colour 1.
 */
#include "game/menu/framework/menu_drawables_internal.h"
#include "game/menu/framework/T258_menu_string.h"          /* (type names: String, Transitive's operator classes) */
#include "engine/media/G002_mpeg_frameq.h"          /* strlen (the MMI one, 0x00269A68) */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/text/T499_font_metrics.h"         /* text_char_height */

using namespace gt2k_menu;

// FUNC 0x001CFAA8 gt2k_menu::String::String
/* Transitive() (0x001BB4F8), then (callers ArcadeControl::createLabel,
 * specPanelAddText); an exception runs ~Transitive. */
String::String(const char *text, const char *font)
{
    /* implicit: Transitive (base constructor), ~Transitive (exception cleanup) */
    this->font = font;
    align = 0;
    splitChar = 0;
    strcpy(this->text, text);
    length = strlen(text);
    f[0] = (f32)length;
}

// FUNC 0x001CFB50 gt2k_menu::String::setText
/* (callers ArcadeControl::createPowerText / createTorqueText) neither
 * length nor F0 is updated. */
void String::setText(const char *text)
{
    strcpy(this->text, text);
}

// FUNC 0x001CFB70 gt2k_menu::String::virtual_2
/* draw.  The font is scaled so that a character is w x h pixels.  Quirk:
 * the part from the split character on is passed to Font::print as its
 * format string. */
void String::virtual_2()
{
    char shown[0x40];

    if (!visible)
        return;
    Drawable::virtual_2();
    if (font != NULL && font[0] != 0)
        g_002F1968.setStyle(font);
    else
        g_002F1968.setStyle(STRING_DEFAULT_FONT);
    f32 scale = 1.0f / (f32)text_char_height(&g_002F1968);
    g_002F1968.setSize(w * scale, h * scale);
    g_002F1968.setColor(color[0].toRGBA8(alphaRatio));
    f32 screenX = getScreenX();
    g_002F1968.setPos(screenX, getScreenY());

    /* only the first F0 characters */
    strcpy(shown, text);
    if ((int)f[0] < (int)strlen(shown))
        shown[(int)f[0]] = 0;
    g_002F1968.setAlign(align);

    if (splitChar == 0) {
        g_002F1968.print("%s", shown);
    } else {
        int len = strlen(shown);
        int i;
        for (i = 0; i < len && shown[i] != splitChar; i++)
            ;
        if (i != len) {
            char *rest = &shown[i];
            *rest = 0;
            g_002F1968.print("%s", shown);
            g_002F1968.setColor(color[1].toRGBA8(alphaRatio));
            *rest = (char)splitChar;
            g_002F1968.print(rest);
        } else {
            g_002F1968.print("%s", shown);
        }
    }
    g_002F1968.setAlign(0);
    g_002F1968.setSize(1.0f, 1.0f);
}

// FUNC 0x001CFDC0 gt2k_menu::String::setSplitChar
void String::setSplitChar(int c)
{
    splitChar = c;
    align = 0;
}

// FUNC 0x001CFDD0 gt2k_menu::String::setAlign
void String::setAlign(int a)
{
    align = a;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Transitive). */
// FUNC 0x001CFDD8 __tfQ29gt2k_menu6String
extern "C" void *__tfQ29gt2k_menu6String(void)
{
    if (__tiQ29gt2k_menu6String[0] == 0) {
        __tfQ29gt2k_menu10Transitive();
        __rtti_si(__tiQ29gt2k_menu6String, "Q29gt2k_menu6String",
                  (const type_info *)__tiQ29gt2k_menu10Transitive);
    }
    return __tiQ29gt2k_menu6String;
}
#endif

// FUNC 0x001CFE28 gt2k_menu::String::~String
/* the implicit destructor (game/menu/framework/T258_menu_string.h; index.txt:
 * String::virtual_0): ~Transitive(0) without a vptr store,
 * Object::operator delete if flags & 1 */
/* implicit: ~Transitive (base destructor), operator delete (deleting destructor) */
