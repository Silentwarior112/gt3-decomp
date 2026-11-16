/* T239 - gt2k_menu::CallbackText (0x001C6D58..0x001C6FC0).
 *
 * An Item whose text a page callback writes: fn(owner, buf, 0x80), once in
 * the constructor (once != 0) or before every draw.  Built by the page hook
 * create_callback_text (p8 = the callback, pC == 1: once).
 *
 * Data (retail .rodata 0x003369D8..0x00336B16): CallbackText's vtable, then
 * the type names CallbackText, OperatorList, List<OperatorHolder>,
 * OperatorHolder, Cell<OperatorHolder>, Operator (Item's header only).
 */
#include "game/menu/framework/T239_callback_text.h"
#include "game/menu/framework/menu_links_internal.h"                           /* (T229: PageLayout::getTexture) */

namespace gt2k_menu {

// FUNC 0x001C6D58 gt2k_menu::CallbackText::CallbackText
CallbackText::CallbackText(CallbackTextFn *fn_, ControlBase *owner_, MenuLayoutItem *res,
                           MenuTexture *tex, int once_)
    : Item(res, tex)
{
    /* Item(res, tex); on a throw Item::~Item */
    fn = fn_;
    owner = owner_;
    once = once_;
    if (once_ != 0)
        fn_(owner_, buf, sizeof(buf));
}

// FUNC 0x001C6E08 gt2k_menu::CallbackText::virtual_2
void CallbackText::virtual_2()
{
    if (once == 0)
        fn(owner, buf, sizeof(buf));
    setText(buf);
    Item::virtual_2();
}

/* The hook of the callback texts (z is left at Item's). */
// FUNC 0x001C6E70 gt2k_menu::create_callback_text
Transitive *create_callback_text(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *res,
                                 int p8, int pC, int p10, int p14)
{
    CallbackTextFn *fn = (CallbackTextFn *)p8;
    MenuTexture *tex = layout->getTexture(res);
    int once = (pC == 1);                   /* before operator new (retail) */
    /* Object::operator new(0x18C); Object::operator delete if the constructor
     * throws */
    return new CallbackText(fn, (ControlBase *)ctx, res, tex, once);  /* implicit: Object::operator delete (if the constructor throws) */
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001C6F28 __tfQ29gt2k_menu12CallbackText
extern "C" void *__tfQ29gt2k_menu12CallbackText(void)
{
    if (__tiQ29gt2k_menu12CallbackText[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu12CallbackText, "Q29gt2k_menu12CallbackText",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu12CallbackText;
}
#endif

// FUNC 0x001C6F78 gt2k_menu::CallbackText::~CallbackText
/* the implicit destructor (game/menu/framework/T239_callback_text.h): Item::~Item(0)
 * without a vptr store, Object::operator delete if flags & 1 */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
