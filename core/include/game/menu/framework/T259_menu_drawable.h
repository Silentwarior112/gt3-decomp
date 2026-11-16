/* T259_menu_drawable.h - gt2k_menu::Drawable (translation unit T259, unit
 * u072), 0x001CFE70..0x001D1A38.
 *
 * The visual state every drawn menu object shares: a centre (two
 * MotionValue springs), a size, four vertex colours, blur offsets, four
 * per-class parameters F0..F3, an anchor the position is relative to, an
 * alpha ratio, draw flags and visibility; their accessors; the vertex /
 * colour helpers of the draw methods; and virtual_2, which applies the
 * render state (a custom function, the default state, or one of four blend
 * modes).  The menu is laid out on a 640 x 448 virtual screen.
 *
 * Drawable owns every member from +0x20 to +0xAF: Transitive (T216) and the
 * classes below it start their own data at +0xB0.
 *
 * Vertices and UVs are the G001 primitive buffer formats (gfx_alloc_tail /
 * pgl_draw_strip): PrimVertex {x, y, z} (12 bytes, z stored 0) and PrimUV
 * {u, v}; u072's first pass called them MenuVertex / MenuUV. */
#ifndef GAME_T259_MENU_DRAWABLE_H
#define GAME_T259_MENU_DRAWABLE_H

#include "game/game.h"
#include "game/menu/framework/T220_object.h"               /* base gt2k_menu::Object */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue x, y */
#include "game/menu/framework/T218_color.h"                /* Color4f color[4] */
#include "engine/pgl/G001_flare.h"               /* PrimVertex (the inline setVertex) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_00352758 (the inline getScreen*) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer::displayHeight (the inline getScreen*) */


/* The menus' virtual screen MENU_SCREEN_W / _H / _CX / _CY (640 x 448): in
 * T261_menu_draw_state.h (included above), whose functions scale it. */

/* T259's own data (defined in T259; the inline Drawable::F_001D1988 reads
 * the pair, so it is declared before the class). */
extern const f32 g_00337830[16][2];         /* 0x00337830 drawBlur's tap offsets {-1.5, -1.75}, ... */
extern const f32 g_00352718[2];             /* 0x00352718 gp-0x52D8 {1000.0, 0.0} */

namespace gt2k_menu {

/* Drawable::drawFlags as virtual_2 reads them (u065's names; u072 had a
 * copy). */
enum {
    DRAW_OWN_SETTINGS = 0x01,               /* the flags below apply (else setDefaultDrawSetting) */
    DRAW_LINEAR       = 0x02,               /* TEX1 MMIN / MMAG linear (gfx_tex_param 2 / 3), else
                                               nearest */
    DRAW_BLEND_MASK   = 0xF0,               /* the blend (gfx_blend_func): */
    DRAW_BLEND_ALPHA  = 0x00,               /*   0x44 (Cs - Cd) * As + Cd */
    DRAW_BLEND_ADD    = 0x10,               /*   0x48 Cs * As + Cd */
    DRAW_BLEND_SUB    = 0x20,               /*   0x42 Cd - Cs * As */
    DRAW_BLEND_TESTED = 0x30                /*   0x84 (Cs - Cd) * As with the alpha test (GEQUAL 0.5) */
};

/* Drawable::anchorType: the coordinates taken from the anchor (u002, u004,
 * u027, u068 and u072 had copies). */
enum { ANCHOR_X = 0x1, ANCHOR_Y = 0x2, ANCHOR_XY = 0x3 };

/* vtable 0x003378B0 (5 slots: [1] / [3] / [4] Object's).  typeinfo
 * __tfQ29gt2k_menu8Drawable 0x001D19A0 (__si_type_info 0x004B26F0, base
 * Object).  Object's data +0x00..+0x1B, vptr +0x1C.  Size 0xB0 (Transitive's
 * members start at +0xB0).
 *
 * The destructor is implicit: slot [0] 0x001D19F0 only calls ~Object and
 * stores no vptr, and ~Transitive (T216) calls ~Object directly. */
class Drawable : public Object {
public:
    typedef void (*DrawSettingFunc)(void);              /* virtual_2's replacement */

    /*  [0] +0x0C 0x001D19F0 the implicit destructor */
    virtual void virtual_2();                           /* [2] +0x1C 0x001D0478 the draw settings:
                                                           drawSettingFunc if set, else without drawFlags
                                                           bit 0 setDefaultDrawSetting(), else the texture
                                                           filter of bit 1 and the blend of bits 4..7 */

    /* blur / vertex / position helpers */
    void drawBlur(u32 *colors, PrimUV *uv);             /* 0x001CFE70 (? name) 16 copies of the quad,
                                                           offset by the blur taps times blur2DX / Y */
    void setRectVertices(PrimVertex *v, f32 x, f32 y, f32 w, f32 h); /* 0x001D0058 (? name) the quad
                                                           (x, y) +- (w, h) / 2 plus the anchor's offset;
                                                           right / bottom edges +0.5 */
    void F_001D01A0(PrimVertex *v, f32 x, f32 y, f32 w, f32 h);      /* 0x001D01A0 = setRectVertices */
    void getColors(u32 *out);                           /* 0x001D01C0 the 4 colours packed, alpha scaled */
    void getPosition(f32 *px, f32 *py);                 /* 0x001D0228 x / y plus the anchor's (anchorType
                                                           bit 0 x, bit 1 y) */
    f32  getScreenX() const;                            /* 0x001D02E0 x.value plus the anchors' */
    f32  getScreenY() const;                            /* 0x001D0320 y.value plus the anchors' */
    void getScreenXY(f32 *px, f32 *py) const;           /* 0x001D0360 */
    static void setDefaultDrawSetting();                /* 0x001D03D8 the render state without own
                                                           settings: alpha blend, no alpha / depth test,
                                                           linear filter (? static: uses no member) */

    /* The accessors and helpers below are inline in the original: none has
     * a caller, and 0x001D05E0..0x001D1988 are the out-of-line copies egcs
     * emits between virtual_2 and the typeinfo function, Drawable() and
     * setColor(u32) among them; the users expand setDrawSettingFunc (u064),
     * setX / setY (u007), setRect (u010).  The sweep moved u072's definitions
     * here.  egcs writes a class's in-class inline members in reverse
     * declaration order (DATA.md), so they are declared in the reverse of
     * their addresses (T259's frame info, u072's data pass): F_001D1988
     * first, the constructor between setVertex and setX, getAlphaRatio last
     * (the sets come out as X, Y, W, H; R, G, B, A; F0..F3). */

    /* (? name) g_00352718[i]: no `this' (the index in $a0), so a static
     * member; no callers, the meaning of the pair is unknown. */
    static f32 F_001D1988(int i) { return g_00352718[i]; } /* 0x001D1988 */
    /* The vertices of this rectangle through F_001D01A0 (F_001D1940 is the
     * same through setRectVertices: two names for one helper, ? the original
     * pair). */
    void F_001D1968(PrimVertex *v, f32 px, f32 py)  /* 0x001D1968 setRectVertices of w x h at (px, py) */
    {
        setRectVertices(v, px, py, w, h);
    }
    void F_001D1940(PrimVertex *v)                  /* 0x001D1940 setRectVertices of this rectangle */
    {
        setRectVertices(v, x.value, y.value, w, h);
    }
    void F_001D1920(PrimVertex *v, f32 px, f32 py)  /* 0x001D1920 F_001D01A0 of w x h at (px, py) */
    {
        F_001D01A0(v, px, py, w, h);
    }
    void F_001D18F8(PrimVertex *v)                  /* 0x001D18F8 F_001D01A0 of this rectangle */
    {
        F_001D01A0(v, x.value, y.value, w, h);
    }
    /* color[i] packed, alpha times alphaRatio. */
    u32 getPackedColor(int i)                       /* 0x001D18D0 color[i] packed, alpha scaled */
    {
        return color[i].toRGBA8(alphaRatio);
    }
    f32 getScreenPosX()                             /* 0x001D1898 x.value in buffer pixels */
    {
        return (x.value - 0.0f) * ((f32)g_00352758->width / MENU_SCREEN_W);
    }
    /* (y - 0) scaled: the code subtracts a literal 0.0 (? an origin constant of
     * 0, folded). */
    f32 getScreenPosY()                             /* 0x001D1848 y.value in buffer pixels */
    {
        f32 posY = y.value;
        return (posY - 0.0f) * ((f32)g_00352758->displayHeight() / MENU_SCREEN_H);
    }
    /* w scaled from the 640-pixel virtual screen (the buffer's width). */
    f32 getScreenW()                                /* 0x001D1818 w in pixels of the buffer drawn
                                                           (640 wide) */
    {
        return w * ((f32)g_00352758->width / MENU_SCREEN_W);
    }
    /* h scaled from the 448-line virtual screen to the buffer being drawn (its
     * display height: the rectangle's, doubled for a field buffer). */
    f32 getScreenH()                                /* 0x001D17D0 h in pixels of the buffer drawn
                                                           (448 lines) */
    {
        f32 height = h;
        return height * ((f32)g_00352758->displayHeight() / MENU_SCREEN_H);
    }
    /* v[i] = (px, py, 0); `this' is not used. */
    void setVertex(PrimVertex *v, int i, f32 px, f32 py) /* 0x001D17B0 v[i] = (px, py, 0); this unused */
    {
        PrimVertex *p = &v[i];
        p->x = px;
        p->y = py;
        p->z = 0;
    }
    /* Inline in the original: Transitive() (T216) and the ButtonList
     * constructors (T244) expand it; 0x001D16E0 is an out-of-line copy with
     * no callers (egcs -O2 gives its code exactly).  x / y (MotionValue's
     * constructor), w / h and color[4] (Color4f(): 0.502) are built as
     * members; anchorType is left unset. */
    Drawable() : w(100.0f), h(100.0f)                   /* 0x001D16E0 */
    {
        blur2DX = 0.0f;
        blur2DY = 0.0f;
        drawSettingFunc = 0;
        f[0] = 0.0f;
        f[1] = 0.0f;
        f[2] = 0.0f;
        f[3] = 0.0f;
        anchor = 0;
        alphaRatio = 1.0f;
        drawFlags = 0;
        visible = 1;
    }
    void setX(f32 v) { x.set(v); }                  /* 0x001D16C8 x.set(v) */
    void setY(f32 v) { y.set(v); }                  /* 0x001D16B0 y.set(v) */
    void setW(f32 v) { w = v; }                     /* 0x001D16A8 */
    void setH(f32 v) { h = v; }                     /* 0x001D16A0 */
    /* w before h: the copy stores h first and w in the jr's delay slot (the
     * last store, where `this' dies, is scheduled first); u010's
     * make_car_render, which expands it, stores w, then h. */
    void setRect(f32 px, f32 py, f32 pw, f32 ph)    /* 0x001D1670 */
    {
        x.set(px);
        y.set(py);
        w = pw;
        h = ph;
    }
    void setColorR(int i, f32 v) { color[i].setR(v); } /* 0x001D1660 */
    void setColorG(int i, f32 v) { color[i].setG(v); } /* 0x001D1650 */
    void setColorB(int i, f32 v) { color[i].setB(v); } /* 0x001D1640 */
    void setColorA(int i, f32 v) { color[i].setA(v); } /* 0x001D1630 */
    void setColor(int i, f32 r, f32 g, f32 b, f32 a) /* 0x001D1608 */
    {
        color[i].setR(r);
        color[i].setG(g);
        color[i].setB(b);
        color[i].setA(a);
    }
    /* Corner i, channel by channel through Color4f's setters (u072: the
     * copies keep &color[i] in a register, the first store folded onto it);
     * setColor(u32, int) converts into a local first (`this' then leaves
     * $a0 for the colour). */
    void setColor(u32 rgba, int i)                  /* 0x001D1490 */
    {
        Color4f c(rgba);
        color[i].setR(c.r);
        color[i].setG(c.g);
        color[i].setB(c.b);
        color[i].setA(c.a);
    }
    /* The four corners from packed colours (top left, top right, bottom left,
     * bottom right: the vertex order of setRectVertices). */
    void setColors(u32 c0, u32 c1, u32 c2, u32 c3)  /* 0x001D0F20 */
    {
        color[0] = Color4f(c0);
        color[1] = Color4f(c1);
        color[2] = Color4f(c2);
        color[3] = Color4f(c3);
    }
    /* Corner i = *c ($a1 the colour, $a2 the corner), through Color4f's
     * operator= (&color[i] built as index + this, u072). */
    void setColor(const Color4f *c, int i)          /* 0x001D0EF0 */
    {
        color[i] = *c;
    }
    /* Inline in the original: GarageButton::virtual_2 (T219) expands it (four
     * Color4f(rgba) temporaries); 0x001D0998 is an out-of-line copy with no
     * callers.  Items hide it with Item::setColor(u32) (T213, the text
     * colour): call it as Drawable::setColor there. */
    void setColor(u32 rgba)                             /* 0x001D0998 all 4 corners, each converted anew */
    {
        color[0] = Color4f(rgba);
        color[1] = Color4f(rgba);
        color[2] = Color4f(rgba);
        color[3] = Color4f(rgba);
    }
    void setBlur2DX(f32 v) { blur2DX = v; }         /* 0x001D0990 */
    void setBlur2DY(f32 v) { blur2DY = v; }         /* 0x001D0988 */
    void setDrawSettingFunc(DrawSettingFunc func) { drawSettingFunc = func; } /* 0x001D0980 */
    void setF0(f32 v) { f[0] = v; }                 /* 0x001D0978 */
    void setF1(f32 v) { f[1] = v; }                 /* 0x001D0970 */
    void setF2(f32 v) { f[2] = v; }                 /* 0x001D0968 */
    void setF3(f32 v) { f[3] = v; }                 /* 0x001D0960 */
    void setF0123(f32 f0, f32 f1, f32 f2, f32 f3)   /* 0x001D0948 */
    {
        f[0] = f0;
        f[1] = f1;
        f[2] = f2;
        f[3] = f3;
    }
    /* The stores in source order here: the copies store the last one first
     * (its value dies there) and the rest after it (u072). */
    void setAnchor(const Drawable *a, int type)     /* 0x001D0938 */
    {
        anchor = a;
        anchorType = type;
    }
    void setAlphaRatio(f32 a) { alphaRatio = a; }   /* 0x001D0930 */
    void setVisible(bool on) { visible = on; }      /* 0x001D0928 */
    /* (? name) turns the own draw settings on and ORs in mode (DRAW_LINEAR,
     * a DRAW_BLEND_* value). */
    void setDrawMode(u32 mode)                      /* 0x001D0910 (? name) drawFlags |= 1 | mode */
    {
        /* two statements: one `|' folds to (mode | 1) | drawFlags (u072) */
        drawFlags |= DRAW_OWN_SETTINGS;
        drawFlags |= mode;
    }
    /* Through MotionValue's accessors (the copies keep &y / &x in a register;
     * T217's opMove expands them). */
    void incX(f32 delta) { x.setValue(x.get() + delta); } /* 0x001D08F8 x.value */
    void incY(f32 delta) { y.setValue(y.get() + delta); } /* 0x001D08E0 y.value */
    void incW(f32 delta) { w = w + delta; }         /* 0x001D08D0 */
    void incH(f32 delta) { h = h + delta; }         /* 0x001D08C0 */
    /* Clamped to [0, 1] (Color4f::incA .. incR). */
    void incColorR(int i, f32 delta) { color[i].incR(delta); } /* 0x001D0878 */
    void incColorG(int i, f32 delta) { color[i].incG(delta); } /* 0x001D0830 */
    void incColorB(int i, f32 delta) { color[i].incB(delta); } /* 0x001D07E8 */
    void incColorA(int i, f32 delta) { color[i].incA(delta); } /* 0x001D07A0 */
    /* Clamped to [0, 1] (as the colour channels above; the alpha ratio too)
     * by menu_clampf (T218), which loads both bounds before its tests. */
    void incBlur2DX(f32 delta)                      /* 0x001D0760 */
    {
        blur2DX = menu_clampf(blur2DX + delta, 0.0f, 1.0f);
    }
    /* Bug (as in the code): blur2DY + d, clamped to [0, 1], is stored into
     * blur2DX. */
    void incBlur2DY(f32 delta)                      /* 0x001D0720 (bug: stores into blur2DX) */
    {
        blur2DX = menu_clampf(blur2DY + delta, 0.0f, 1.0f);
    }
    void incF0(f32 delta) { f[0] = f[0] + delta; }  /* 0x001D0710 */
    void incF1(f32 delta) { f[1] = f[1] + delta; }  /* 0x001D0700 */
    void incF2(f32 delta) { f[2] = f[2] + delta; }  /* 0x001D06F0 */
    void incF3(f32 delta) { f[3] = f[3] + delta; }  /* 0x001D06E0 */
    void incAlphaRatio(f32 delta)                   /* 0x001D06A0 clamped to [0, 1] */
    {
        alphaRatio = menu_clampf(alphaRatio + delta, 0.0f, 1.0f);
    }
    MotionValue *getDX() { return &x; }             /* 0x001D0698 &x */
    MotionValue *getDY() { return &y; }             /* 0x001D0690 &y */
    f32 getX() const { return x.value; }            /* 0x001D0688 */
    f32 getY() const { return y.value; }            /* 0x001D0680 */
    f32 getW() const { return w; }                  /* 0x001D0678 */
    f32 getH() const { return h; }                  /* 0x001D0670 */
    /* The channel accessors go through Color4f's (T218): the copies build
     * &color[i] as index + this (u066's matching pass). */
    f32 getColorR(int i) { return color[i].getR(); } /* 0x001D0660 */
    f32 getColorG(int i) { return color[i].getG(); } /* 0x001D0650 */
    f32 getColorB(int i) { return color[i].getB(); } /* 0x001D0640 */
    f32 getColorA(int i) { return color[i].getA(); } /* 0x001D0630 */
    Color4f *getColor(int i) { return &color[i]; }  /* 0x001D0620 */
    f32 getBlur2DX() const { return blur2DX; }      /* 0x001D0618 */
    f32 getBlur2DY() const { return blur2DY; }      /* 0x001D0610 */
    f32 getF0() const { return f[0]; }              /* 0x001D0608 */
    f32 getF1() const { return f[1]; }              /* 0x001D0600 */
    f32 getF2() const { return f[2]; }              /* 0x001D05F8 */
    f32 getF3() const { return f[3]; }              /* 0x001D05F0 */
    const Drawable *getAnchor() const { return anchor; } /* 0x001D05E8 */
    f32 getAlphaRatio() const { return alphaRatio; } /* 0x001D05E0 */

    MotionValue x;                          /* 0x20 centre x */
    MotionValue y;                          /* 0x2C centre y (down is +) */
    f32      w;                             /* 0x38 width (100.0) */
    f32      h;                             /* 0x3C height (100.0) */
    Color4f  color[4];                      /* 0x40 vertex colours (0.502) */
    f32      blur2DX;                       /* 0x80 blur offsets (non-zero: drawBlur) */
    f32      blur2DY;                       /* 0x84 */
    DrawSettingFunc drawSettingFunc;        /* 0x88 replaces virtual_2's settings */
    /* F0..F3, per class: String the characters shown, Image type 1 the
     * rotation, type 2 the UV extents; drawBlur's blend parameter is F0. */
    f32      f[4];                          /* 0x8C */
    const Drawable *anchor;                 /* 0x9C the position is relative to it */
    int      anchorType;                    /* 0xA0 ANCHOR_*: bit 0 x, bit 1 y (not set by the constructor) */
    f32      alphaRatio;                    /* 0xA4 (1.0) */
    int      drawFlags;                     /* 0xA8 DRAW_*: own settings, linear filter, the blend
                                               (int: virtual_2's switch compares signed, slti) */
    int      visible;                       /* 0xAC (1) */
};
EGCS_SIZE(gt2k_menu::Drawable, 0xB0);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu8Drawable(void);      /* 0x001D19A0 */
extern u32 __tiQ29gt2k_menu8Drawable[3];    /* 0x004B26F0 its __si_type_info, filled by it */
}
#endif

#endif /* GAME_T259_MENU_DRAWABLE_H */
