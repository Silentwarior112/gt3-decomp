/* T239_callback_text.h - gt2k_menu::CallbackText (translation unit T239,
 * unit u069), 0x001C6D58..0x001C6FC0.
 *
 * An Item whose text a page callback writes: fn(owner, buf, 0x80), once in
 * the constructor (once != 0) or before every draw.  Built by the page hook
 * create_callback_text (0x001C6E70; p8 = the callback, pC == 1: once). */
#ifndef GAME_T239_CALLBACK_TEXT_H
#define GAME_T239_CALLBACK_TEXT_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {
class ControlBase;                          /* T247 (pointers only) */
class Object;                               /* T220 */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 (T235: MenuResource) */
struct MenuTexture;                         /* T213 */

/* Writes the text (at most size bytes with the terminator) into buf. */
typedef void CallbackTextFn(ControlBase *owner, char *buf, int size);

/* vtable 0x003369D8 (8 slots: [1] Item's 0x001B9F98, [3] .. [7] as
 * ColorBox's).  typeinfo __tfQ29gt2k_menu12CallbackText 0x001C6F28 (si, base
 * Item; __si_type_info 0x004B2640).  Size 0x18C (operator new in
 * create_callback_text). */
class CallbackText : public Item {
public:
    CallbackText(CallbackTextFn *fn, ControlBase *owner, MenuLayoutItem *res,
                 MenuTexture *tex, int once); /* 0x001C6D58 */
    /*  [0] +0x0C 0x001C6F78 the implicit destructor: only ~Item, no vptr store */
    virtual void virtual_2();               /* [2] +0x1C 0x001C6E08 draw: fn (unless once), setText(buf),
                                               Item's */

    CallbackTextFn *fn;                     /* 0x100 */
    ControlBase    *owner;                  /* 0x104 fn's first argument */
    char            buf[0x80];              /* 0x108 */
    int             once;                   /* 0x188 the text is made once, by the constructor */
};
EGCS_SIZE(gt2k_menu::CallbackText, 0x18C);

/* The hook of the callback texts (a MenuHookFn, T229; no direct callers): ctx
 * is the owner (the screen's controller, fn's first argument), p8 the
 * CallbackTextFn, pC 1: the text is made once; z is left at Item's.  (u012,
 * u022, u027, u034, u040 and u069 declared it locally before the sweep.) */
Transitive *create_callback_text(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                 int p8, int pC, int p10, int p14);  /* 0x001C6E70 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu12CallbackText(void); /* 0x001C6F28 */
}
#endif

#endif /* GAME_T239_CALLBACK_TEXT_H */
