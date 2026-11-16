/* T256_menu_rectangle.h - gt2k_menu::Rectangle (translation unit T256, unit
 * u072), 0x001CE5D8..0x001CF178.
 *
 * A Transitive drawn as one flat quad in its vertex colours (or
 * Drawable::drawBlur when a blur offset is set), then an optional frame of
 * frameSize pixels: raised / sunken bevels (four edge strips in grey
 * 0x80808080 with two blend modes) or a drop shadow (an 8-vertex strip below
 * and right of the rectangle).  The base of Image (T257), hence of Item. */
#ifndef GAME_T256_MENU_RECTANGLE_H
#define GAME_T256_MENU_RECTANGLE_H

#include "game/game.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* base gt2k_menu::Transitive */

namespace gt2k_menu {

/* Rectangle::frameType (u065's names; u072 had a copy).  (? RAISED / SUNKEN
 * may be swapped: with y down, type 1 darkens the left / top edges and so
 * looks sunken, u072.) */
enum { FRAME_NONE = 0, FRAME_RAISED = 1, FRAME_SUNKEN = 2, FRAME_SHADOW = 3 };

/* vtable 0x00337428 (8 slots: [1] / [4]..[7] Transitive's, [3] Object's).
 * typeinfo __tfQ29gt2k_menu9Rectangle 0x001CF0E0 (__si_type_info 0x004B26B0,
 * base Transitive).  Size 0xCC (Image's members start at +0xCC).
 *
 * The destructor is implicit: slot [0] 0x001CF130 only calls ~Transitive and
 * stores no vptr, and ~Item (T213) calls ~Transitive directly. */
class Rectangle : public Transitive {
public:
    /* Inline in the original: `new Rectangle' (PageManager::startFade
     * 0x001B7A7C, T010 F_00114090), Image(Tex1Image *), the ImageGZ and the
     * Item constructors expand it after Transitive(); 0x001CF0A0 is an
     * out-of-line copy with no callers. */
    Rectangle() : frameType(0), frameSize(0) {}         /* 0x001CF0A0 */
    /*  [0] +0x0C 0x001CF130 the implicit destructor */
    virtual void virtual_2();                           /* [2] +0x1C 0x001CEF40 draw: Drawable::virtual_2,
                                                           the quad (or drawBlur), then drawFrame */

    void shiftByAlign(int align);                       /* 0x001CE5D8 (? name; no callers) the centre moved
                                                           by half the size: align & 3 1 right, 2 left;
                                                           align & 0xC 4 down, 8 up (G001 font_set_align's
                                                           bits: the position was the left / right / top /
                                                           bottom edge) */
    /* The four bevel strips, frameSize wide, inside the edge, in one
     * colour (pgl_color), at getPosition(). */
    void drawLeftEdge(u32 rgba);                        /* 0x001CE6A0 */
    void drawRightEdge(u32 rgba);                       /* 0x001CE7E0 */
    void drawTopEdge(u32 rgba);                         /* 0x001CE920 at y - h / 2 */
    void drawBottomEdge(u32 rgba);                      /* 0x001CEA60 at y + h / 2 */
    /* Frame types 1 / 2: grey strips subtracted from (blend Cd - Cs * As) and
     * added to (Cs * As + Cd) the frame buffer.  Type 1 darkens the left / top
     * edges and lightens the right / bottom ones, type 2 the other way round;
     * with y growing downwards type 1 looks sunken and type 2 raised (? the
     * names may be swapped). */
    void drawRaised();                                  /* 0x001CEBA0 frame type 1 */
    void drawSunken();                                  /* 0x001CEC40 frame type 2 */
    void drawShadow();                                  /* 0x001CECE0 frame type 3: an 8-vertex strip below and
                                                           right of the quad, grey of alpha alphaRatio fading to
                                                           0, subtracted */
    void drawFrame();                                   /* 0x001CF020 by frameType */
    void setFrame(int type, f32 size);                  /* 0x001CF090 */

    int frameType;                          /* 0xC4 FRAME_*: 0 none, 1 raised, 2 sunken, 3 shadow */
    f32 frameSize;                          /* 0xC8 width of the frame */
};
EGCS_SIZE(gt2k_menu::Rectangle, 0xCC);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu9Rectangle(void);     /* 0x001CF0E0 */
extern u32 __tiQ29gt2k_menu9Rectangle[3];   /* 0x004B26B0 its __si_type_info, filled by it (read by
                                               Image's, T257) */
}
#endif

#endif /* GAME_T256_MENU_RECTANGLE_H */
