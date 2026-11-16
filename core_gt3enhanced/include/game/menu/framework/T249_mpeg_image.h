/* T249_mpeg_image.h - gt2k_menu::MpegImage (translation unit T249, unit
 * u071), 0x001CBE80..0x001CC5F8.
 *
 * An Item that draws, in order of preference: the current frame of the menu
 * background movie (g_002CDC10, T253), a decoded IPU picture (setIpic, T250
 * Ipic) or its plain Image texture.  u0 / v0 / uw / vh select the part of
 * the source that is shown; mirror draws it flipped. */
#ifndef GAME_T249_MPEG_IMAGE_H
#define GAME_T249_MPEG_IMAGE_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

struct PrimUV;                              /* G001: {f32 u, v;} (pointers only) */

namespace gt2k_menu {
class Ipic;                                 /* T250 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 (T235: MenuResource) */

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T249_mpeg_image.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtable 0x00336F80 (8 slots: [3] .. [7] as ColorBox's).  typeinfo
 * __tfQ29gt2k_menu9MpegImage 0x001CC5A8 (si, base Item; __si_type_info
 * 0x004B2670).  Size 0x120 (operator new in F_0010FF90, T030). */
class MpegImage : public Item {
public:
    MpegImage(Tex1Image *tex);              /* 0x001CBE80 Item(NULL, NULL) with the texture set tex
                                               (Image::texSet; T051 passes NULL) */
    MpegImage(MenuLayoutItem *res, PageLayout *layout); /* 0x001CBEF0 Item(res, layout) */
    virtual ~MpegImage();                   /* [0] +0x0C 0x001CBF48 frees the picture */
    virtual void virtual_1();               /* [1] +0x14 0x001CC268 update: Item's */
    virtual void virtual_2();               /* [2] +0x1C 0x001CC448 draw: the movie frame, else the
                                               picture, else the texture */

    void setIpic(Ipic *pic);                /* 0x001CBFC8 decodes pic into a new menu-heap buffer
                                               (NULL: no picture) */
    void movieUV(PrimUV *uv);               /* 0x001CC068 the texture coordinates of the movie frame */
    void ipicUV(PrimUV *uv, int w, int h);  /* 0x001CC140 those of a w x h picture */
    void drawIpic();                        /* 0x001CC288 uploads the picture to a temporary GSBuffer
                                               and draws it */
    void setMirror(int on);                 /* 0x001CC588 */
    void setUVRect(f32 u, f32 v, f32 w, f32 h); /* 0x001CC590 */

    f32   u0;                               /* 0x100 the source rectangle (0, 0, 1, 1) */
    f32   v0;                               /* 0x104 */
    f32   uw;                               /* 0x108 */
    f32   vh;                               /* 0x10C */
    int   mirror;                           /* 0x110 nonzero: drawn flipped (Drawable F_001D01A0) */
    void *pixels;                           /* 0x114 the decoded picture (menu heap), NULL: none */
    int   picWidth;                         /* 0x118 */
    int   picHeight;                        /* 0x11C */
};

/* The inline accessors 0x001CC588..0x001CC590: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
/* 0x001CC590 */
inline void MpegImage::setUVRect(f32 u, f32 v, f32 width, f32 height)
{
    /* declaration order; sched2 puts vh's store first (retail's order) */
    u0 = u;
    v0 = v;
    uw = width;
    vh = height;
}
inline void MpegImage::setMirror(int on) { mirror = on; }  /* 0x001CC588 */
EGCS_SIZE(gt2k_menu::MpegImage, 0x120);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu9MpegImage(void);     /* 0x001CC5A8 */
}
#endif

#endif /* GAME_T249_MPEG_IMAGE_H */
