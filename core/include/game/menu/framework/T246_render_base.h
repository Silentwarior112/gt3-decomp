/* T246_render_base.h - gt2k_menu::RenderBase (translation unit T246, unit
 * u071), 0x001CB3F0..0x001CB9E8.  (The drawing offset / scale it sets,
 * g_0035272C / g_00352730 / g_00352734, are T261's .sdata.)
 *
 * The GameObject a menu controller gives the render thread (attach()): it
 * owns the RootObject of the menu's object tree, updates it (unless paused)
 * and draws it between beginRender (the projection, the magnify spring) and
 * endRender (the optional full-screen mask colour).  detach() leaves the
 * render thread through a screen transition (T005 start_transition) when one
 * is set.  sleep() parks the calling thread on a Timer (T252) of the tree.
 * The derived renders: ArcadeRender (T011), GtmodeRender (T046),
 * MovieRender (T051), LoggerRender (T055), ShortmenuRender (T112),
 * ShortmenuLicenseRender (T103). */
#ifndef GAME_T246_RENDER_BASE_H
#define GAME_T246_RENDER_BASE_H

#include "game/game.h"
#include "game/app/T002_render_thread.h"        /* base GameObject */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::RootObject (root) */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (magnifySpring) */

class GSBuffer;                             /* T514 (references only) */

/* The screen transition of start_transition (T005): 0 fade-out, 1 delayed
 * fade-out, 2 line-out, 3 comb-out (other values: none).  (The enumerator
 * names are not known; the type name is, from the mangled name of
 * RenderBase::setTransition: a global enum.) */
enum TransitionType {
    TRANSITION_FADEOUT       = 0,           /* FadeoutTransitionObject */
    TRANSITION_DELAY_FADEOUT = 1,           /* DelayFadeoutTransitionObject */
    TRANSITION_LINEOUT       = 2,           /* LineoutTransitionObject */
    TRANSITION_COMBOUT       = 3            /* ComboutTransitionObject */
};

namespace gt2k_menu {

/* vtable 0x00336E18 (4 slots: [3] GameObject's 0x00101A38).  typeinfo
 * __tfQ29gt2k_menu10RenderBase 0x001CB950 (si, base GameObject;
 * __si_type_info 0x004B2660).  vptr +0x00 (GameObject has no data).  Size
 * 0x58.
 *
 * The destructor is implicit: slot [0] 0x001CB9A0 only destroys root and
 * calls ~GameObject, storing no vptr; the derived renders' destructors
 * (LoggerRender's, 0x00127498, 0x00128D48, 0x0012CB28) expand ~RootObject
 * and GameObject::~GameObject themselves. */
class RenderBase : public GameObject {
public:
    RenderBase();                           /* 0x001CB3F0 magnify 1, renderEnable 1, screenSaverFlag 1 */
    /*  [0] +0x0C 0x001CB9A0 the implicit destructor */
    virtual void update();                  /* [1] +0x14 0x001CB870 the tree's update unless paused */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x001CB898 beginRender, the tree in z order,
                                               endRender (unless g_00352748 or !renderEnable) */

    void attach();                          /* 0x001CB498 the render thread draws this */
    void detach();                          /* 0x001CB4B8 the transition (or no object), then the tree
                                               is released */
    void sleep(f32 seconds);                /* 0x001CB548 the calling thread sleeps on a Timer of the
                                               tree (60 frames a second) */
    void beginRender(GSBuffer &buf);        /* 0x001CB618 the magnify spring (1 .. 2) into g_00352734 /
                                               g_00352730 / g_0035272C */
    void endRender(GSBuffer &buf);          /* 0x001CB818 the mask */
    /* The accessors are inline in the original: no callers, and
     * 0x001CB8F8..0x001CB948 are egcs's out-of-line copies before the
     * typeinfo function; the controllers' begin functions expand
     * getRootObject / setTransition / setMask (u007, u019, u020, u026).
     * Declared in reverse address order: egcs writes a class's in-class
     * inline members out in reverse declaration order (u071's data pass:
     * T246's frame info, 0x002CD208, has getRootObject's FDE first). */
    void setPause(bool on) { pause = on; }          /* 0x001CB948 */
    int getPause() const { return pause; }          /* 0x001CB940 */
    void setMask(bool on) { mask = on; }            /* 0x001CB938 */
    void setMaskColor(unsigned int rgba) { maskColor = rgba; } /* 0x001CB930 */
    void setMagnify(f32 m) { magnify = m; }         /* 0x001CB928 */
    void setScreenSaver(bool on, int n)             /* 0x001CB918 */
    {
        screenSaver = on;
        screenSaverFlag = (n != 0);
    }
    void setTransition(bool on, TransitionType type) /* 0x001CB908 */
    {
        /* on first: the out-of-line copy 0x001CB908 (u071) is exact so
         * (66.7% the other way); no expanding caller changes (u018 notes). */
        transitionOn = on;
        transitionType = type;
    }
    void setRenderEnable(bool on) { renderEnable = on; } /* 0x001CB900 */
    RootObject *getRootObject() { return &root; }   /* 0x001CB8F8 */

    /*             vptr                        0x00 */
    RootObject     root;                    /* 0x04 the menu's object tree */
    int            pause;                   /* 0x28 nonzero: update skips the tree (debug pause) */
    int            mask;                    /* 0x2C nonzero: endRender fills the screen with maskColor */
    int            screenSaver;             /* 0x30 */
    int            screenSaverFlag;         /* 0x34 setScreenSaver's n != 0 */
    int            renderEnable;            /* 0x38 */
    /* detach() starts transitionType.  A bool (egcs's is 4 bytes; u018 /
     * u015): ShortmenuControl::begin and ShortmenuLicenseControl::begin
     * (T111 / T103) store it after the counts' loads, which only its own
     * alias set allows (with g_00352750 a bool too, T261).  GCC 14 keeps an
     * int (its bool is 1 byte; the values are 0 / 1). */
#ifdef BOOT_OLD_GCC
    bool           transitionOn;            /* 0x3C */
#else
    int            transitionOn;            /* 0x3C */
#endif
    TransitionType transitionType;          /* 0x40 */
    u32            maskColor;               /* 0x44 RGBA8 */
    MotionValue    magnifySpring;           /* 0x48 the magnification shown (springs to magnify) */
    f32            magnify;                 /* 0x54 its target */
};
EGCS_SIZE(gt2k_menu::RenderBase, 0x58);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu10RenderBase(void);   /* 0x001CB950 */
extern u32 __tiQ29gt2k_menu10RenderBase[3]; /* 0x004B2660 its __si_type_info */
}
#endif

#endif /* GAME_T246_RENDER_BASE_H */
