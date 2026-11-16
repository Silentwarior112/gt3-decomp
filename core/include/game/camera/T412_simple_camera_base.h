/* T412_simple_camera_base.h - SimpleCameraBase (translation unit T412, unit
 * u094), 0x00210008..0x002104E8.
 *
 * A camera with a translation, a rotation (degrees) and a frustum window.
 * virtual_8 can split the window into n x n tiles (a picture drawn in tiles);
 * virtual_9 keeps the angles in [-180, 180].  Nothing in this binary
 * constructs one or calls its non-virtual methods (the names are Lupo 2's).
 */
#ifndef GAME_T412_SIMPLE_CAMERA_BASE_H
#define GAME_T412_SIMPLE_CAMERA_BASE_H

#include "game/game.h"
#include "libc.h"                           /* printf (print, inline) */
#include "game/camera/T410_camera_base.h"          /* base CameraBase */

/* vtable 0x0033F6E0 (10 slots: [2]..[7] CameraBase's, [8] / [9] new).
 * typeinfo __tf16SimpleCameraBase 0x00210478 (__si_type_info 0x004B2980,
 * base CameraBase).  vptr +0x00 (CameraBase's).  Size 0x40 (the data fill
 * it; no instance in the binary).
 *
 * The destructor is implicit (compiler-generated), as MenuCarCamera's (T411):
 * 0x002104C8, the one the vtable uses, stores no vptr and only calls
 * CameraBase::~CameraBase with the same flags; a declared destructor, even
 * an empty inline one, stores the class's vptr first (u094 pass 2; the egcs
 * test of i4, build/scratch/i4/dtor).
 *
 * The constructor and the accessors are inline (u094 pass 2): T412's copies
 * 0x002102E0..0x00210428 have no callers and follow T412's own functions, in
 * the reverse order of Lupo 2's linkonce (inline) copies of the same methods
 * (the constructor, setTranslation .. getFar, print), as T411's: declared in
 * Lupo 2's order, egcs writes them reversed. */
class SimpleCameraBase : public CameraBase {
public:
    SimpleCameraBase()                      /* 0x00210428 every member 0 (no callers) */
    {
        speed[0] = 0;
        speed[1] = 0;
        translation[0] = 0;
        translation[1] = 0;
        translation[2] = 0;
        rotation[0] = 0;
        rotation[1] = 0;
        rotation[2] = 0;
        left = 0;
        right = 0;
        bottom = 0;
        top = 0;
        nearZ = 0;
        farZ = 0;
        dist = 0;
    }
    /* [0] +0x0C 0x002104C8: the implicit destructor (see above). */
    virtual void applyProjection();         /* [1] +0x14 0x00210028 the projection: pgl_frustum(left .. dist) */
    /* tile 0: the whole window (applyProjection); tile k (1..n*n): column (k-1) % n,
     * row (k-1) / n counted from the top. */
    virtual void virtual_8(int tile, int n);    /* [8] +0x4C 0x00210060 */
    virtual void virtual_9();               /* [9] +0x54 0x00210140 the rotation into [-180, 180] */

    /* (? name) The seven window values; d is the window's distance (focal). */
    void setFrustum(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f, f32 d);  /* 0x00210008 */
    /* The inline members in Lupo 2's order (the constructor above first):
     * egcs writes them in reverse declaration order, retail T412's
     * 0x002102E0 print .. 0x00210418 setTranslation, then the constructor
     * (u094's data pass: T412's frame info). */
    void setTranslation(f32 x, f32 y, f32 z)    /* 0x00210418 */
    {
        translation[0] = x;
        translation[1] = y;
        translation[2] = z;
    }
    void setRotation(f32 x, f32 y, f32 z)   /* 0x00210408 */
    {
        rotation[0] = x;
        rotation[1] = y;
        rotation[2] = z;
    }
    void setSpeed(f32 a, f32 b) { speed[0] = a; speed[1] = b; }        /* 0x002103F8 */
    /* The window at near (d = n; Lupo 2's 28-byte copy stores it itself). */
    void setFrustum(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f)   /* 0x002103D8 */
    {
        setFrustum(l, r, b, t, n, f, n);
    }
    void setNear(f32 n) { nearZ = n; }                      /* 0x002103D0 */
    void setFar(f32 f) { farZ = f; }                        /* 0x002103C8 */
    f32  getTranslationX() const { return translation[0]; } /* 0x002103C0 */
    f32  getTranslationY() const { return translation[1]; } /* 0x002103B8 */
    f32  getTranslationZ() const { return translation[2]; } /* 0x002103B0 */
    f32  getNear() const { return nearZ; }                  /* 0x002103A8 */
    f32  getFar() const { return farZ; }                    /* 0x002103A0 */
    /* The translation and the rotation (Lupo 2's print, the same 188 bytes). */
    void print()                            /* 0x002102E0 */
    {
        printf("(%f, %f, %f) %f %f %f\n",
               (f64)translation[0], (f64)translation[1], (f64)translation[2],
               (f64)rotation[0], (f64)rotation[1], (f64)rotation[2]);
    }

    /*  vptr                                   0x00 */
    f32 speed[2];                           /* 0x04 (? of the translation / the rotation; only setSpeed) */
    f32 translation[3];                     /* 0x0C */
    f32 rotation[3];                        /* 0x18 degrees */
    f32 left;                               /* 0x24 */
    f32 right;                              /* 0x28 */
    f32 bottom;                             /* 0x2C */
    f32 top;                                /* 0x30 */
    f32 nearZ;                              /* 0x34 */
    f32 farZ;                               /* 0x38 */
    f32 dist;                               /* 0x3C the window's distance (pgl_frustum's focal) */
};
EGCS_SIZE(SimpleCameraBase, 0x40);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects hand declarations) */
extern "C" {
void *__tf16SimpleCameraBase(void);         /* 0x00210478 */
extern u32 __ti16SimpleCameraBase[3];       /* 0x004B2980 its __si_type_info (only T412 uses it) */
}
#endif

#endif /* GAME_T412_SIMPLE_CAMERA_BASE_H */
