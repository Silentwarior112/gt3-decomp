/* T411_menu_car_camera.h - MenuCarCamera (translation unit T411, unit u094),
 * 0x0020EFE0..0x00210008.
 *
 * The menu camera that orbits a car (the showroom, the garage, the car wash,
 * the prize cars; gt2k_menu::Car T212 holds one, CarGetMenuCamera T106
 * derives from it).  Every coordinate is a MotionValue spring (T260): the eye
 * `trans' (moved only by updateTrans_, which has no callers), the point looked
 * at `interest', the orbit angles `rot' (pushed by a constant force each
 * frame and wrapped to [0, 360)), the orbit distance `dist', and a
 * translation `shift' and rotation `tilt' of the view (each one MotionValue
 * per axis: transX / Y / Z, ..., tiltX / Y).  The view (F_0020F9D8)
 * puts the eye at (0, 0, dist) turned by rot, looks from it at interest and
 * stores it back into trans.  The presets come from the showroom record of
 * the camera's type (T215, floats 0x34..0x4B); type 2 mirrors them.
 *
 * The setters come in pairs: (values) and (const f32 *v), the same values
 * read from a table; a MotionValue set to a value is at rest there.
 */
#ifndef GAME_T411_MENU_CAR_CAMERA_H
#define GAME_T411_MENU_CAR_CAMERA_H

#include "game/game.h"
#include "game/camera/T410_camera_base.h"          /* base CameraBase */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (held by value) */

class Control;                              /* T474 (pointers only) */

/* vtable 0x0033F668 (9 slots: [3]..[7] CameraBase's, [8] new).  typeinfo
 * __tf13MenuCarCamera 0x0020FF98 (__si_type_info 0x004B2970, base CameraBase;
 * the node is declared below: the plan had it in T060_trophy_get.h).  vptr
 * +0x00 (CameraBase's).  Size 0x14C (gt2k_menu::Car +0xC4).
 *
 * The constructor is inline: the owners expand it, and T411's 0x0020FDA8 is
 * an out-of-line copy (no callers).  The destructor is implicit (compiler-
 * generated): 0x0020FFE8, the one the vtable uses, stores no vptr and only
 * calls CameraBase::~CameraBase (as Car's destructor does directly), which a
 * declared destructor, even an empty inline one, would not give.  The
 * constructor sets type; the 16 MotionValues are zeroed by their own inline
 * constructor (T260), the other members are not set.  They are separate
 * members, not arrays: egcs builds a member array of a class with a
 * constructor by a loop, and both copies of this constructor (0x0020FDA8,
 * and CarGetMenuCamera's 0x0013C760, T106) are straight-line stores; no T411
 * function loops over them either.  An egcs -O2 compile of this declaration
 * gives the 70 words of 0x0020FDA8 (the vtable relocation aside). */
class MenuCarCamera : public CameraBase {
public:
    /* [0] +0x0C 0x0020FFE8: the implicit destructor (see above). */
    virtual void applyProjection();         /* [1] +0x14 0x0020F9A0 the projection: pgl_frustum(proj[0..6]) */
    virtual void applyView();               /* [2] +0x1C 0x0020FB30 the view: F_0020F9D8(1.0f) */
    /* One frame of the springs (not trans): each pulled towards its goal by
     * (goal - value) * spring and damped, the angles pushed by rotSpeed and
     * wrapped to [0, 360).  ctl is not used; every caller (reset, T106, T212)
     * passes the spare Control g_002EB170 (T475). */
    virtual void virtual_8(Control *ctl);   /* [8] +0x4C 0x0020FB58 */

    void clear();                           /* 0x0020EFE0 every spring and its parameters 0 (type, proj stay) */
    void setTrans(f32 x, f32 y, f32 z);     /* 0x0020F150 at rest */
    void setTrans(const f32 *v);            /* 0x0020F188 */
    void setTransGoal(f32 x, f32 y, f32 z, f32 spring, f32 damping); /* 0x0020F1C8 */
    void setTransGoal(const f32 *v);        /* 0x0020F1E0 */
    void setInterest(f32 x, f32 y, f32 z);  /* 0x0020F210 at rest */
    void setInterest(const f32 *v);         /* 0x0020F248 */
    void setInterestGoal(f32 x, f32 y, f32 z, f32 spring, f32 damping); /* 0x0020F288 */
    void setInterestGoal(const f32 *v);     /* 0x0020F2A0 */
    void F_0020F2D0(f32 x, f32 y, f32 z);   /* 0x0020F2D0 unk_078 at rest */
    void F_0020F308(const f32 *v);          /* 0x0020F308 */
    void F_0020F348(f32 a, f32 b);          /* 0x0020F348 unk_09C / unk_0A0 */
    void F_0020F358(const f32 *v);          /* 0x0020F358 */
    void setOrbit(f32 pitch, f32 yaw, f32 d);   /* 0x0020F370 tumbleX, tumbleY, dist at rest */
    void setOrbit(const f32 *v);            /* 0x0020F3A8 */
    void setOrbitMotion(f32 pitchSpeed, f32 yawSpeed, f32 damping,
                        f32 dGoal, f32 dSpring, f32 dDamping);  /* 0x0020F3E8 */
    void setOrbitMotion(const f32 *v);      /* 0x0020F408 */
    void setShift(f32 x, f32 y);            /* 0x0020F440 at rest */
    void setShift(const f32 *v);            /* 0x0020F470 the values only */
    void setShiftGoal(f32 x, f32 y, f32 spring, f32 damping);   /* 0x0020F4A0 */
    void setShiftGoal(const f32 *v);        /* 0x0020F4B8 */
    void setTilt(f32 x, f32 y);             /* 0x0020F4E0 at rest */
    void setTilt(const f32 *v);             /* 0x0020F510 the values and the goals */
    void setTiltGoal(f32 x, f32 y, f32 spring, f32 damping);    /* 0x0020F550 */
    void setTiltGoal(const f32 *v);         /* 0x0020F568 */
    /* The preset of the showroom record of type (T215 showroom_index): floats
     * 0x34 interest, 0x38 its goal, 0x3D orbit, 0x41 orbit motion, 0x47 shift
     * then tilt (0x49), 0x4B the vertical field of view of a 640 x 448
     * window.  setTilt(const f32 *) takes four values, so the tilt goals are
     * 0x4B and the word after the record; harmless: tiltSpring stays 0. */
    void loadPreset();                      /* 0x0020F590 */
    void loadPresetMirrored();              /* 0x0020F718 the same mirrored in x (type 2) */
    void reset();                           /* 0x0020F918 clear, the preset, one virtual_8 */
    void setProjection(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f, f32 d);  /* 0x0020F980 proj[0..6] */
    /* The eye (0, 0, dist) scaled in x by scaleX and turned by rot; the shift,
     * the tilt and the look-at from the eye to interest; the eye into trans. */
    void F_0020F9D8(f32 scaleX);            /* 0x0020F9D8 */
    /* Inline in the original, as the constructor (u094 pass 2): T411's copies
     * 0x0020FD88..0x0020FF90 have no callers and follow T411's own functions
     * (with the constructor among them, 0x0020FDA8), in the reverse order of
     * Lupo 2's linkonce (inline) copies of the same accessors.  They are
     * declared in Lupo 2's order, getTransX_ .. getInterestZ_, updateTrans_,
     * the constructor, setType, getType, getTumbleY, getTumbleDynamY: egcs
     * writes a class's inline members in reverse declaration order (u094's
     * data pass: T411's frame info, the FDE order, is retail's only so).  The
     * users expand setType (u064).  The names are Lupo 2's (u094; first pass
     * F_0020FD88 / F_0020FD90, getMode / setMode, updateTrans; Lupo 2's
     * getters are const). */
    MotionValue *getTransX_() { return &transX; }           /* 0x0020FF90 */
    MotionValue *getTransY_() { return &transY; }           /* 0x0020FF88 */
    MotionValue *getTransZ_() { return &transZ; }           /* 0x0020FF80 */
    MotionValue *getInterestX_() { return &interestX; }     /* 0x0020FF78 */
    MotionValue *getInterestY_() { return &interestY; }     /* 0x0020FF70 */
    MotionValue *getInterestZ_() { return &interestZ; }     /* 0x0020FF68 */
    /* One frame of the eye springs, as virtual_8's.  MotionValue::pull: the
     * value is read through the MotionValue (addiu a0, s, 8; lwc1 8(a0)),
     * in this copy and in CarGetMenuCamera::update 0x0013C3A0 (u020: exact
     * only so, 97.1% with the fields written out). */
    void updateTrans_()                                     /* 0x0020FEC0 */
    {
        transX.pull(transGoal[0], transSpring);
        transX.update(transDamping);
        transY.pull(transGoal[1], transSpring);
        transY.update(transDamping);
        transZ.pull(transGoal[2], transSpring);
        transZ.update(transDamping);
    }
    MenuCarCamera() : type(0) {}            /* 0x0020FDA8 (out-of-line copy) */
    void setType(int t) { type = t; }                       /* 0x0020FDA0 (Lupo 2: a MenuCarCamera::Type) */
    int  getType() { return type; }                         /* 0x0020FD98 */
    f32  getTumbleY() { return tumbleY.value; }             /* 0x0020FD90 */
    MotionValue *getTumbleDynamY() { return &tumbleY; }     /* 0x0020FD88 */

    /*  vptr                                   0x000 */
    int         type;                       /* 0x004 the menu (the showroom record, showroom_index); 2: mirrored */
                                            /* (Lupo 2: a MenuCarCamera::Type, setType / getType; u094 mode) */
    MotionValue transX;                     /* 0x008 the eye (getTransX_ .. Z_) */
    MotionValue transY;                     /* 0x014 */
    MotionValue transZ;                     /* 0x020 */
    f32         transGoal[3];               /* 0x02C */
    f32         transSpring;                /* 0x038 */
    f32         transDamping;               /* 0x03C */
    MotionValue interestX;                  /* 0x040 the point looked at (getInterestX_ .. Z_) */
    MotionValue interestY;                  /* 0x04C */
    MotionValue interestZ;                  /* 0x058 */
    f32         interestGoal[3];            /* 0x064 */
    f32         interestSpring;             /* 0x070 */
    f32         interestDamping;            /* 0x074 */
    MotionValue unk_078;                    /* 0x078 x, y, z of F_0020F2D0 (only set and cleared here) */
    MotionValue unk_084;                    /* 0x084 */
    MotionValue unk_090;                    /* 0x090 */
    f32         unk_09C;                    /* 0x09C (F_0020F348) */
    f32         unk_0A0;                    /* 0x0A0 */
    MotionValue tumbleX;                    /* 0x0A4 the eye around the interest about x (pitch; u094 rotX) */
                                            /* (both degrees [0, 360)) */
    MotionValue tumbleY;                    /* 0x0B0 about y (yaw; u070 reads tumbleY.value, +0xB8; u094 rotY) */
                                            /* (T037's TumbleCar spins it, Car +0x174; Lupo 2's getTumbleY) */
    f32         rotSpeed[2];                /* 0x0BC the force added each frame */
    f32         rotDamping;                 /* 0x0C4 */
    MotionValue dist;                       /* 0x0C8 the distance of the eye */
    f32         distGoal;                   /* 0x0D4 */
    f32         distSpring;                 /* 0x0D8 */
    f32         distDamping;                /* 0x0DC */
    MotionValue shiftX;                     /* 0x0E0 the view translated by x, y */
    MotionValue shiftY;                     /* 0x0EC */
    f32         shiftGoal[2];               /* 0x0F8 */
    f32         shiftSpring;                /* 0x100 */
    f32         shiftDamping;               /* 0x104 */
    MotionValue tiltX;                      /* 0x108 the view turned about x, y */
    MotionValue tiltY;                      /* 0x114 */
    f32         tiltGoal[2];                /* 0x120 */
    f32         tiltSpring;                 /* 0x128 */
    f32         tiltDamping;                /* 0x12C */
    f32         proj[7];                    /* 0x130 pgl_frustum's l, r, b, t, near, far, focal */
};
EGCS_SIZE(MenuCarCamera, 0x14C);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects a hand declaration) */
extern "C" void *__tf13MenuCarCamera(void); /* 0x0020FF98 */
/* Its __si_type_info (CarGetMenuCamera's base, T106, uses it too). */
extern "C" u32 __ti13MenuCarCamera[3];      /* 0x004B2970 */
#endif

#endif /* GAME_T411_MENU_CAR_CAMERA_H */
