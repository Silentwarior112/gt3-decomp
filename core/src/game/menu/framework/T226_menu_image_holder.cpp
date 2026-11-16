/* T226 - MenuImageHolder (? name).  0x001C1A18..0x001C1B88.
 *
 * Used on the stack by T036 make_car_render (the car view factory): the
 * texture of a layout item, unpacked from gzip into a menu-heap copy (and
 * its TEX1 offsets relocated) when the item's flag 0x40000 is set.
 * handOver() hangs a FreeAgent owning that copy under the object using it. */
/* Retail's T226 has no .rodata: it saw no polymorphic class but its
 * FreeAgent / Object (key functions elsewhere): the texture record from
 * T213_menu_texture.h, not the menu item. */
#include "engine/pgl/G001_flare.h"                /* tex1_relocate, Tex1Image */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current */
#include "game/menu/framework/T213_menu_texture.h"         /* gt2k_menu::MenuTexture */
#include "game/menu/framework/T229_page_layout.h"          /* MenuLayoutItem, ITEM_GZIPPED */
#include "game/menu/framework/T236_free_agent.h"           /* gt2k_menu::FreeAgent (T220 Object) */
#include "engine/pdistd/T537_gzip_inflate.h"         /* GZIP::InflateBase */
#include "game/menu/framework/T226_menu_image_holder.h"
#include "game/menu/framework/menu_widgets_internal.h"

using namespace gt2k_menu;

/* 94.9%: the body is retail's; left is the order of the prologue's
 * register saves (retail sd s1, s0, ra, then the `this' copy; egcs here
 * saves s0 first, which feeds the handedOver store).  That order comes out
 * when a scheduling barrier (a `do { } while (0)' or the EH region of a
 * throw() specification) follows the parameter copies, but then the
 * `lui 0x4' of the flag test stays behind the barrier, where retail has it
 * first (97.4%).  Tried: mem-initializer, negated test / early return, the
 * flags in a local, `this' / tex in locals; the reference compiler gives
 * the same. */
// FUNC 0x001C1A18 MenuImageHolder::MenuImageHolder
MenuImageHolder::MenuImageHolder(MenuLayoutItem *item, MenuTexture *tex)
{
    handedOver = 0;
    if (item->flags & ITEM_GZIPPED) {
        data = (Tex1Image *)menu_heap_current()->allocate(tex->size, 0x10);
        {
            GZIP::InflateBase inflate(data, tex->data);     /* sp+0x00, 0x3038 bytes */
        }                                                   /* ~InflateBase (flags 2) */
        tex1_relocate(data);
        unpacked = 1;
    } else {
        unpacked = 0;
        data = (Tex1Image *)tex->data;
    }
}

/* Empty; the deleting form calls the global operator delete (0x0027C0C0). */
// FUNC 0x001C1AB8 MenuImageHolder::~MenuImageHolder
MenuImageHolder::~MenuImageHolder()
{
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x001C1AE0 MenuImageHolder::handOver
void MenuImageHolder::handOver(Object *owner)
{
    if (unpacked != 0) {
        FreeAgent *agent = new FreeAgent(data);     /* 0x24 bytes, inline constructor */  /* implicit: Object::operator delete (if the constructor throws) */
        agent->attachTo(owner);
    }
    handedOver = 1;
}
