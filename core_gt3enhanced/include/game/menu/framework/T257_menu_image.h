/* T257_menu_image.h - gt2k_menu::Image, gt2k_menu::ImageGZ (translation unit
 * T257, unit u072), 0x001CF178..0x001CFAA8.
 *
 * An Image draws texture `index' of a Tex1 image (texSet) as a Rectangle:
 * type 0 a plain quad, type 1 centred at (0, 0) and rotated by F0 degrees
 * about its position, type 2 tiled: the UV extents F0 / F1 with the wrap
 * modes (CLAMP WMS / WMT) set to repeat, so the power-of-two texture repeats
 * F0 x F1 times.  The texture size is its CLAMP register's MAXU / MAXV + 1; the
 * UVs cover size / the next power of two.  Nothing is drawn (not even the
 * frame) without a texture.  ImageGZ inflates a gzip'd Tex1 image into the
 * menu heap and frees it in its destructor.  Item (T213) derives from Image. */
#ifndef GAME_T257_MENU_IMAGE_H
#define GAME_T257_MENU_IMAGE_H

#include "game/game.h"
#include "game/menu/framework/T256_menu_rectangle.h"       /* base gt2k_menu::Rectangle */
#include "engine/pgl/G001_flare.h"               /* Tex1Image, GsTexRegs (tex_width / tex_height) */

/* The size of texture `index' of a TEX1 image from its CLAMP register
 * (REGION_CLAMP's MAXU bits 14..23, MAXV bits 34..43: the last texel).
 * (u072's static helpers of getU / getV, here for the inline getTexWidth /
 * getTexHeight.)  The + 1 in 64 bits, then truncated (daddiu before the
 * dsll32 / dsra32, as the race displays' copies of the expression). */
static inline int tex_width(const Tex1Image *img, int index)
{
    return (int)(((img->tex[index].clamp >> 14) & 0x3FF) + 1);
}
static inline int tex_height(const Tex1Image *img, int index)
{
    return (int)(((img->tex[index].clamp >> 34) & 0x3FF) + 1);
}

struct Tex1Image;                           /* G001 (pointers only) */
struct PrimUV;                              /* G001 (pointers only) */

namespace gt2k_menu {

/* vtable 0x003375A8 (8 slots: [1] / [4] / [6] / [7] Transitive's, [3]
 * Object's).  typeinfo __tfQ29gt2k_menu5Image 0x001CFA10 (__si_type_info
 * 0x004B26C0, base Rectangle).  The vtable symbols name slots 2 and 5
 * "ImageGZ::": they are Image's (ImageGZ and Item inherit them).  Size 0xD8
 * (ImageGZ: operator new(0xD8); Item's members start at +0xD8).
 *
 * The destructor is implicit: slot [0] 0x001CFA60 only calls ~Transitive and
 * stores no vptr, and ~Item (T213) calls ~Transitive directly. */
class Image : public Rectangle {
public:
    enum Type { NORMAL = 0, ROTATED = 1, TILED = 2 };   /* (? names) the type member; TILED (UV_F01 before
                                                           the sweep) tiles the texture (wrap REPEAT, UVs
                                                           up to F0 / F1) */

    /*  [0] +0x0C 0x001CFA60 the implicit destructor */
    virtual void virtual_2();                           /* [2] +0x1C 0x001CF378 draw: texSet made resident
                                                           (VramAllocator::loadTex1) and selected, then by
                                                           type (or drawBlur), then drawFrame */
    virtual void virtual_5();                           /* [5] +0x34 0x001CF358 fitToTexture() */

    f32  getU();                                        /* 0x001CF178 width / the next power of 2 */
    f32  getV();                                        /* 0x001CF208 height / the next power of 2 */
    void setUV(PrimUV *uv, f32 u, f32 v);               /* 0x001CF298 (0, 0) (u, 0) (0, v) (u, v); this unused */
    void setUVF01(PrimUV *uv);                          /* 0x001CF2C0 (? name) the same with u = F0, v = F1 */
    void fitToTexture();                                /* 0x001CF2F8 w, h = the texture size */
    /* setFullUV .. setIndex and Image(Tex1Image *) are inline in the
     * original: no callers, and 0x001CF8A8..0x001CF9B8 are egcs's out-of-line
     * copies between ImageGZ's and Image's typeinfo functions.  egcs writes a
     * class's in-class inline members in reverse declaration order (DATA.md),
     * so they are declared here in the reverse of their addresses, the
     * constructor between getTexHeight and setReverseX (T257's frame info,
     * u072's data pass). */
    void setFullUV(PrimUV *uv)                      /* 0x001CF9B8 setUV(uv, getU(), getV()) */
    {
        f32 u = getU();
        setUV(uv, u, getV());
    }
    int getTexWidth() { return tex_width(texSet, index); } /* 0x001CF980 */
    int getTexHeight() { return tex_height(texSet, index); } /* 0x001CF948 */
    /* Inline in the original: Item's (T213) and ImageGZ's constructors
     * expand an Image constructor (no texture) after Transitive(), with no
     * Rectangle / Image vptr store left (egcs drops them before the class's
     * own); 0x001CF8F0 is an out-of-line copy with no callers. */
    Image(Tex1Image *tex) : texSet(tex), type(0), index(0) {}  /* 0x001CF8F0 */
    void setReverseX() { w = -w; }                  /* 0x001CF8E0 w = -w */
    /* A negative height mirrors the quad vertically. */
    void setReverseY() { h = -h; }                  /* 0x001CF8D0 h = -h */
    /* The same code as setTexSet (? an overload taking another pointer type). */
    void F_001CF8C8(Tex1Image *tex) { texSet = tex; } /* 0x001CF8C8 the same code as setTexSet */
    Tex1Image *getTexSet() { return texSet; }       /* 0x001CF8C0 */
    void setTexSet(Tex1Image *tex) { texSet = tex; } /* 0x001CF8B8 (? name) */
    void setType(Type t) { type = t; }              /* 0x001CF8B0 */
    void setIndex(int i) { index = i; }             /* 0x001CF8A8 */

    Tex1Image *texSet;                      /* 0xCC the Tex1 image set */
    int        type;                        /* 0xD0 Type */
    int        index;                       /* 0xD4 the texture of texSet drawn */

    /* No Image(): egcs would emit an out-of-line copy of it in T257 (where
     * the vtable is) and the listing has none; Item's (T213) and ImageGZ's
     * constructors build their base with Image(0), which gives the same
     * stores (the sweep deleted the protected Image(), u072's proposal). */
};
EGCS_SIZE(gt2k_menu::Image, 0xD8);

/* vtable 0x00337560 (8 slots: [2] / [5] Image's).  typeinfo
 * __tfQ29gt2k_menu7ImageGZ 0x001CF858 (__si_type_info 0x004B26D0, base
 * Image).  No data of its own: size 0xD8. */
class ImageGZ : public Image {
public:
    ImageGZ(const void *gz, u32 size);                  /* 0x001CF6F0 load(gz, size) if gz */
    ImageGZ();                                          /* 0x001CF810 no texture */
    virtual ~ImageGZ();                                 /* [0] +0x0C 0x001CF798 frees texSet (the menu heap) */

    void load(const void *gz, u32 size);                /* 0x001CF670 hidden while it inflates: size bytes
                                                           of the menu heap, GZIP::InflateBase, tex1_relocate */
};
EGCS_SIZE(gt2k_menu::ImageGZ, 0xD8);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu7ImageGZ(void);       /* 0x001CF858 */
void *__tfQ29gt2k_menu5Image(void);         /* 0x001CFA10 */
extern u32 __tiQ29gt2k_menu5Image[3];       /* 0x004B26C0 its __si_type_info, filled by it */
}
#endif

#endif /* GAME_T257_MENU_IMAGE_H */
