/* T205 - the "blink" page hook.  0x001B3B58..0x001B3BE8.
 *
 * Data (retail .rodata 0x003329D8..0x00332B3E): the blink script, then the
 * type names of the operator list classes Item brings (OperatorList,
 * List<OperatorHolder>, OperatorHolder and its base list, Cell<
 * OperatorHolder>, Operator: T213 / T216's set). */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive::start */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (the blink script) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T205_blink.h"
#include "game/menu/framework/menu_buttons_internal.h"

using namespace gt2k_menu;

/* One instruction of the script: code (T217's Operator::op* index), arg,
 * the operands f10..f20, result (1 ends the frame); arg2 and the unknown
 * bytes are 0 (as u002's U002_OP). */
#define U063_OP(code, arg, f10, f14, f18, f1C, f20, result) \
    { (code), (arg), 0, { 0, 0, 0, 0 }, (f10), (f14), (f18), (f1C), (f20), \
      { 0, 0, 0, 0, 0, 0, 0, 0 }, (result) }

/* The blink script (external linkage: egcs writes it where it is defined,
 * at the start of T205's .rodata): opFadeColors with alpha +10 / s for
 * 0.1 s, opFadeColors with alpha -2.5 / s for 0.4 s, opJump -2: the item
 * flashes in and fades out again, every 0.5 s, for as long as it lives. */
extern const OperatorCode g_003329D8[3];
const OperatorCode g_003329D8[3] = {        /* 0x003329D8 */
    U063_OP(20 /* opFadeColors */,  0, 0.0f, 0.0f, 0.0f, 10.0f, 0.1f, 1),
    U063_OP(20 /* opFadeColors */,  0, 0.0f, 0.0f, 0.0f, -2.5f, 0.4f, 1),
    U063_OP( 1 /* opJump */,       -2, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0)
};

/* An Item of the layout item running the blink script g_003329D8 (it
 * flashes in and fades out every 0.5 s); ctx, parent and p8..p14 are not
 * used (the page attaches the result).  No direct callers: the "blink"
 * entry of the screens' hook tables (a MenuHookFn). */
// FUNC 0x001B3B58 create_blink
Transitive *create_blink(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                         int p8, int pC, int p10, int p14)
{
    Item *blink = new Item(item, layout);  /* Object::operator new: the current menu heap */  /* implicit: Object::operator delete (if the constructor throws) */
    blink->start(g_003329D8);
    return blink;
}
