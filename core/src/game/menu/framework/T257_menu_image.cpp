/* T257 - gt2k_menu::Image and gt2k_menu::ImageGZ (unit u072,
 * 0x001CF178..0x001CFAA8).
 *
 * An Image draws texture `index' of a Tex1 image (texSet) as a Rectangle:
 * type 0 a plain quad, type 1 centred at (0, 0) and rotated by F0 degrees
 * about its position, type 2 tiled (the UV extents F0 / F1 with the wrap
 * modes set to repeat).  The texture size comes from the CLAMP register of
 * the texture (MAXU / MAXV + 1); the UVs cover size / the next power of two
 * (the GS texture is that big).  ImageGZ inflates a gzip'd Tex1 image into
 * the menu heap and frees it in its destructor.
 */
#include "game/menu/framework/menu_drawables_internal.h"
#include "game/menu/framework/T257_menu_image.h"           /* (type names: ImageGZ, Image, Rectangle's operator classes) */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current (pdistd::Allocator: allocate, free) */
#include "engine/gfx/T512_render_context.h"       /* g_002F4250 (the VRAM allocator of g_002F4230) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 */
#include "engine/pdistd/T537_gzip_inflate.h"         /* GZIP::InflateBase */

using namespace gt2k_menu;

/* tex_width / tex_height: T257_menu_image.h (the inline getTexWidth /
 * getTexHeight use them too). */

// FUNC 0x001CF178 gt2k_menu::Image::getU
/* width / (the smallest power of two >= width, at most 0x800). */
f32 Image::getU()
{
    int size = tex_width(texSet, index);
    int pow2;
    for (pow2 = 1; pow2 <= 0x400 && pow2 < size; pow2 <<= 1)
        ;
    return (f32)size / (f32)pow2;
}

// FUNC 0x001CF208 gt2k_menu::Image::getV
/* height / (the smallest power of two >= height, at most 0x800). */
f32 Image::getV()
{
    int size = tex_height(texSet, index);
    int pow2;
    for (pow2 = 1; pow2 <= 0x400 && pow2 < size; pow2 <<= 1)
        ;
    return (f32)size / (f32)pow2;
}

// FUNC 0x001CF298 gt2k_menu::Image::setUV
/* The strip's UVs (0, 0) (u, 0) (0, v) (u, v); `this' is not used. */
void Image::setUV(PrimUV *uv, f32 u, f32 v)
{
    uv[0].u = 0;
    uv[0].v = 0;
    uv[1].v = 0;
    uv[2].u = 0;
    uv[1].u = u;
    uv[2].v = v;
    uv[3].u = u;
    uv[3].v = v;
}

// FUNC 0x001CF2C0 gt2k_menu::Image::setUVF01
/* The same with u = F0, v = F1 (type 2: the repeat counts). */
void Image::setUVF01(PrimUV *uv)
{
    uv[0].u = 0;
    uv[0].v = 0;
    uv[1].u = f[0];
    uv[1].v = 0;
    uv[2].u = 0;
    uv[2].v = f[1];
    uv[3].u = f[0];
    uv[3].v = f[1];
}

// FUNC 0x001CF2F8 gt2k_menu::Image::fitToTexture
/* (also called by kjm_test_image, T113) */
void Image::fitToTexture()
{
    w = (f32)getTexWidth();
    h = (f32)getTexHeight();
}

// FUNC 0x001CF358 gt2k_menu::Image::virtual_5
/* index.txt: ImageGZ::virtual_5; the slot is Image's (and every Item's). */
void Image::virtual_5()
{
    fitToTexture();
}

// FUNC 0x001CF378 gt2k_menu::Image::virtual_2
/* draw (index.txt: ImageGZ::virtual_2; Image's slot 2, inherited).  Nothing
 * at all, not even the frame, without a texture.  The texture is made
 * resident in VRAM (its uploads queued the first time) and its registers
 * selected before the draw settings. */
void Image::virtual_2()
{
    if (!visible)
        return;
    if (texSet == NULL)
        return;

    g_002F4250.loadTex1(texSet);
    gfx_set_texture_regs(&texSet->tex[index]);
    gfx_texflush();
    Drawable::virtual_2();
    u32 *colors = (u32 *)gfx_alloc_tail(1);
    getColors(colors);

    if (blur2DX > 0.0f || blur2DY > 0.0f) {
        PrimUV *uv = (PrimUV *)gfx_alloc_tail(2);
        f32 u = getU();
        setUV(uv, u, getV());
        drawBlur(colors, uv);
    } else {
        switch (type) {
        case NORMAL: {
            PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
            PrimUV *uv = (PrimUV *)gfx_alloc_tail(2);
            setRectVertices(v, x.value, y.value, w, h);
            f32 u = getU();
            setUV(uv, u, getV());
            pgl_draw_strip(v, NULL, colors, uv, 4);
            break;
        }
        case ROTATED: {
            /* the quad about (0, 0), moved and turned by the matrix */
            PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
            PrimUV *uv = (PrimUV *)gfx_alloc_tail(2);
            setRectVertices(v, 0.0f, 0.0f, w, h);
            f32 u = getU();
            setUV(uv, u, getV());
            pgl_matrix_mode(PGL_MTX_MODELVIEW);
            pgl_push_matrix();
            pgl_translate(x.value, y.value, 0.0f);
            vu0_rotate_z(f[0]);
            pgl_draw_strip(v, NULL, colors, uv, 4);
            pgl_pop_matrix();
            break;
        }
        case TILED: {
            /* tiled: the wrap modes stay REPEAT afterwards */
            gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REPEAT);
            gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REPEAT);
            PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
            PrimUV *uv = (PrimUV *)gfx_alloc_tail(2);
            setRectVertices(v, x.value, y.value, w, h);
            setUVF01(uv);
            pgl_draw_strip(v, NULL, colors, uv, 4);
            break;
        }
        }
    }
    drawFrame();
}

// FUNC 0x001CF670 gt2k_menu::ImageGZ::load
/* Hidden while it inflates: size bytes (16-aligned) of the menu heap, a
 * GZIP::InflateBase on the stack inflates the whole file into them (and is
 * destroyed with flags 2), then the image's offsets are made pointers. */
void ImageGZ::load(const void *gz, u32 size)
{
    visible = 0;
    texSet = (Tex1Image *)menu_heap_current()->allocate(size, 0x10);
    {
        GZIP::InflateBase inflater(texSet, gz);
    }
    tex1_relocate(texSet);
    visible = 1;
}

// FUNC 0x001CF6F0 gt2k_menu::ImageGZ::ImageGZ
/* Transitive() (0x001BB4F8), the inline Rectangle / Image(0) parts
 * (+0xC4..+0xD4 zeroed), the vptr; an exception runs ~Transitive.  Image(0)
 * rather than T257's protected Image(): the same code, and egcs then emits
 * no out-of-line Image() copy (the listing has none). */
ImageGZ::ImageGZ(const void *gz, u32 size) : Image(0)
{
    /* implicit: Transitive (base constructor, inline Rectangle), ~Transitive (exception cleanup) */
    if (gz)
        load(gz, size);
}

// FUNC 0x001CF798 gt2k_menu::ImageGZ::~ImageGZ
/* index.txt: ImageGZ::virtual_0.  Frees the inflated image from the
 * current menu heap, then ~Transitive(0); Object::operator delete
 * (0x001BF458) if flags & 1. */
ImageGZ::~ImageGZ()
{
    if (texSet)
        menu_heap_current()->free(texSet);
    texSet = NULL;
    /* implicit: ~Transitive (base destructor), operator delete (deleting destructor) */
}

// FUNC 0x001CF810 gt2k_menu::ImageGZ::ImageGZ (__Q29gt2k_menu7ImageGZ)
/* Transitive() (0x001BB4F8), then the inline Rectangle / Image(0) parts
 * and the vptr: no texture.  (The marker names the symbol: egcs's stabs put
 * this function in the header that the destructor's inline code left
 * current, so the line does not map it.) */
ImageGZ::ImageGZ() : Image(0)
{
    /* implicit: Transitive (base constructor, inline Rectangle), ~Transitive (exception cleanup) */
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Image). */
// FUNC 0x001CF858 __tfQ29gt2k_menu7ImageGZ
extern "C" void *__tfQ29gt2k_menu7ImageGZ(void)
{
    if (__tiQ29gt2k_menu7ImageGZ[0] == 0) {
        __tfQ29gt2k_menu5Image();
        __rtti_si(__tiQ29gt2k_menu7ImageGZ, "Q29gt2k_menu7ImageGZ",
                  (const type_info *)__tiQ29gt2k_menu5Image);
    }
    return __tiQ29gt2k_menu7ImageGZ;
}
#endif

// FUNC 0x001CF8A8 gt2k_menu::Image::setIndex
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF8B0 gt2k_menu::Image::setType
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF8B8 gt2k_menu::Image::setTexSet
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF8C0 gt2k_menu::Image::getTexSet
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF8C8 gt2k_menu::Image::F_001CF8C8
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF8D0 gt2k_menu::Image::setReverseY
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF8E0 gt2k_menu::Image::setReverseX
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF8F0 gt2k_menu::Image::Image
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy (no callers):
 * Transitive(), texSet, the vptr, Rectangle's part (frameType = frameSize
 * = 0), type and index */
/* implicit: Transitive (base constructor) */

// FUNC 0x001CF948 gt2k_menu::Image::getTexHeight
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF980 gt2k_menu::Image::getTexWidth
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy */

// FUNC 0x001CF9B8 gt2k_menu::Image::setFullUV
/* inline in game/menu/framework/T257_menu_image.h; egcs emits this copy (getU, getV, setUV) */
/* implicit: getU, getV, setUV (the inline body) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Rectangle). */
// FUNC 0x001CFA10 __tfQ29gt2k_menu5Image
extern "C" void *__tfQ29gt2k_menu5Image(void)
{
    if (__tiQ29gt2k_menu5Image[0] == 0) {
        __tfQ29gt2k_menu9Rectangle();
        __rtti_si(__tiQ29gt2k_menu5Image, "Q29gt2k_menu5Image",
                  (const type_info *)__tiQ29gt2k_menu9Rectangle);
    }
    return __tiQ29gt2k_menu5Image;
}
#endif

// FUNC 0x001CFA60 gt2k_menu::Image::~Image
/* the implicit destructor (game/menu/framework/T257_menu_image.h): ~Transitive(0)
 * without a vptr store, Object::operator delete if flags & 1 */
/* implicit: ~Transitive (base destructor), operator delete (deleting destructor) */
