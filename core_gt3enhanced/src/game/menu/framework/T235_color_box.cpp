/* T235 - gt2k_menu::ColorBox (0x001C5228..0x001C5D40).
 *
 * A colour choice (the body colours of a car): `count` swatches, copies of
 * the Item tinted with colors[i], drawn h + 2.5 apart downwards from
 * count steps above the Item's place (swatch i at y - (count - i) * (h + 2.5)),
 * and a blinking marker left of the swatch under the cursor.  Up / down move
 * the cursor (while no popup is open); every frame the cursor is handed to
 * the setColor callback.  Built by the page hook create_color_box (p8 = the
 * colour getter, pC = the setter).
 *
 * Data (retail .rodata 0x00336430..0x003365AE): ColorBox's vtable, then
 * SmartIndexT<int>'s, the type names ColorBox, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * SmartIndexT<int>, Operator.  .lit4 0x00350074 (1 / 30).
 */
#include "game/menu/framework/T211_page_manager.h"         /* PageManager (control, popupLevel); first: it brings
                                               SmartIndexT<int> (PageHistory) before T216's lists */
#include "game/menu/framework/T235_color_box.h"
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase */
#include "engine/pgl/G001_flare.h"                /* gfx_*, pgl_*, PrimVertex */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T218_color.h"                /* Color4f */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive */
#ifndef BOOT_OLD_GCC
#include "game/common/T017_smart_index.h"          /* __tiQ29gt2k_menut11SmartIndexT1Zi */
#endif
#include "game/menu/framework/menu_links_internal.h"

namespace gt2k_menu {

/* idx->move(delta) through the vtable (? an inline of the original, as
 * u004's moveIndex): the cursor calls slot 3 virtually, the address computed
 * for each call (the inline's fresh pointer argument). */
static inline int moveIndex(SmartIndexT<int> *idx, int delta)
{
    return idx->move(delta);
}

/* The space between two swatches. */
static const f32 SWATCH_GAP = 2.5f;

// FUNC 0x001C5228 gt2k_menu::ColorBox::ColorBox
ColorBox::ColorBox(ControlBase *owner_, const u32 *colors_, int count_, PageManager *pages_,
                   ColorBoxSetFn *setColor_, MenuLayoutItem *res, MenuTexture *tex)
    : Item(res, tex), owner(owner_), colors(colors_), count(count_), index(),
      pages(pages_), setColor(setColor_)
{
    /* Item(res, tex); the inline SmartIndexT() of index; on a throw
     * Item::~Item. */
    blink = 1.0f;
    index.setRange(0, count);
    index.set(0);
    index.setClamp(true);                   /* through the index's address (retail sw 0xC(s0)) */
}

/* Nothing while a popup is open.  Transitive's update (not Item's), the
 * marker's blink (1/30 a frame, 1.0 again below 0), up / down of pad 1, then
 * setColor(owner, cursor). */
// FUNC 0x001C5340 gt2k_menu::ColorBox::virtual_1
void ColorBox::virtual_1()
{
    if (pages->popupLevel > 0)
        return;

    Transitive::virtual_1();

    blink -= 0.033333335f;                  /* 1 / 30 (gp-0x797C) */
    if (blink < 0.0f)
        blink = 1.0f;

    ControlBase *control = pages->control;
    int moved = 0;
    if (control->control1.up != 0)
        moved = moveIndex(&index, -1);
    else if (control->control1.down != 0)
        moved = moveIndex(&index, 1);

    if (moved != 0) {
        menu_play_se(MENU_SE_CURSOR);
        blink = 1.0f;
    }
    setColor(owner, index.value);
}

/* The swatches (Item's draw in colors[i], alpha blended), and over the
 * cursor's one the marker: an additive quad w / 2 wide, centred w left of
 * the swatch, of grey level `blink`.  y is moved for the draws and put back
 * afterwards. */
// FUNC 0x001C5418 gt2k_menu::ColorBox::virtual_2
void ColorBox::virtual_2()
{
    if (res == 0)
        return;

    f32 savedY = y.get();
    y.set(savedY - (h + SWATCH_GAP) * (f32)count);

    for (int i = 0; i < count; i++) {
        gfx_enable(PGL_CAP_BLEND);
        gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);

        u32 rgba = colors[i];
        color[0] = Color4f(rgba);
        color[1] = Color4f(rgba);
        /* An exact form, not the original: &x first appears here in retail's
         * insn stream (GCSE hoists the member addresses &color[0], &color[1],
         * &x, &color[2], &color[3] into the loop preheader in that order, and
         * retail reads x.value through $fp = &x); a pointer to x declared at
         * the marker, the loop's top or the function's top gives every other
         * order (99.3 .. 99.8%). */
        MotionValue *px = &x;
        color[2] = Color4f(rgba);
        color[3] = Color4f(rgba);
        Item::virtual_2();

        if (i == index.value) {
            gfx_enable(PGL_CAP_BLEND);
            gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
            PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);    /* 4 vertices */
            f32 cx = px->get() - w;
            f32 halfW = w * 0.5f * 0.5f;
            f32 left = cx - halfW;
            f32 right = cx + halfW;
            /* Each vertex's y (y and h reloaded after the previous vertex's
             * stores) before its x and z, as retail orders them. */
            v[0].y = y.get() - h * 0.5f; v[0].x = left;  v[0].z = 0;
            v[1].y = y.get() - h * 0.5f; v[1].x = right; v[1].z = 0;
            v[2].y = y.get() + h * 0.5f; v[2].x = left;  v[2].z = 0;
            v[3].y = y.get() + h * 0.5f; v[3].x = right; v[3].z = 0;
            pgl_color4f(blink, blink, blink, 1.0f);
            pgl_draw_strip(v, 0, 0, 0, 4);
        }
        incY(h + SWATCH_GAP);
    }

    y.set(savedY);
}

/* The hook of "car_color_box": the colours from getColors, the cursor goes
 * to setColor; z from the item. */
// FUNC 0x001C5B80 gt2k_menu::create_color_box
Transitive *create_color_box(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *res,
                             int p8, int pC, int p10, int p14)
{
    ControlBase *owner = (ControlBase *)ctx;
    ColorBoxGetFn *getColors = (ColorBoxGetFn *)p8;
    ColorBoxSetFn *setColor = (ColorBoxSetFn *)pC;
    MenuTexture *tex = layout->getTexture(res);
    int count;
    const u32 *colors = getColors(&count);
    PageManager *pages = owner->pageManager;    /* read before operator new (retail) */
    /* Object::operator new(0x130); Object::operator delete if the constructor
     * throws */
    ColorBox *box = new ColorBox(owner, colors, count, pages, setColor, res, tex);  /* implicit: Object::operator delete (if the constructor throws) */
    box->z = res->z;
    return box;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001C5C68 __tfQ29gt2k_menu8ColorBox
extern "C" void *__tfQ29gt2k_menu8ColorBox(void)
{
    if (__tiQ29gt2k_menu8ColorBox[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu8ColorBox, "Q29gt2k_menu8ColorBox",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu8ColorBox;
}
#endif

// FUNC 0x001C5CB8 gt2k_menu::ColorBox::~ColorBox
/* the implicit destructor (game/menu/framework/T235_color_box.h): Item::~Item(0) without a
 * vptr store, Object::operator delete if flags & 1 */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function of the template instance
 * SmartIndexT<int> (__user_type_info, no bases): T235's copy, never
 * referenced (the linker kept T017's 0x00103690).  Weak so that the copies
 * can be linked. */
// FUNC 0x001C5D00 __tfQ29gt2k_menut11SmartIndexT1Zi
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut11SmartIndexT1Zi(void)
{
    if (__tiQ29gt2k_menut11SmartIndexT1Zi[0] == 0)
        __rtti_user(__tiQ29gt2k_menut11SmartIndexT1Zi, "Q29gt2k_menut11SmartIndexT1Zi");
    return __tiQ29gt2k_menut11SmartIndexT1Zi;
}
#endif
