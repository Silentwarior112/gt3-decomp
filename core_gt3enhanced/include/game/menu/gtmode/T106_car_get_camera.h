/* T106_car_get_camera.h - the prize car presentation helpers of the car
 * screen (T107 CarsItem, T061 CarGetControl): gt2k_menu::CarRoulette,
 * CarTurntable, the floor drawing and gt2k_menu::CarGetMenuCamera
 * (translation unit T106, unit u020), 0x0013B848..0x0013C8F0.
 *
 *   - CarRoulette (CarsItem +0x100): a cursor running over the 1..4 cars;
 *     start() spins it at 0.1 slot a frame, stop() computes the constant
 *     deceleration that ends it, after whole turns worth about 12 slots, in
 *     the middle of the `target' slot (the car that is won).
 *   - CarTurntable (CarGetControl +0xEA4): the cars on a circle sized from
 *     their dimensions; the table turns at -12.5 degrees a second and each
 *     car on itself at 60 degrees a second, both slowing down once
 *     `stopping' is set.
 *   - draw_table_floor: the textured square under the cars.
 *   - CarGetMenuCamera (CarsItem +0xED4): a spring camera that first looks
 *     at the whole table from above, then, two `next' requests later,
 *     follows the car that was won.
 * draw_ring_arc / draw_ring and two CarTurntable helpers have no callers.
 * CarRoulette and CarTurntable are u021's CarSelector and CarLayout (no
 * RTTI: the names are u020's guesses). */
#ifndef GAME_T106_CAR_GET_CAMERA_H
#define GAME_T106_CAR_GET_CAMERA_H

#include "game/game.h"
#include "game/camera/T411_menu_car_camera.h"      /* base MenuCarCamera */

struct Tex1Image;                           /* G001_flare.h (pointers only) */

namespace gt2k_menu {

/* (int)roulette.pos -> its slot: returns pos (CarRoulette::update, T107
 * CarsItem::virtual_1 through CarRoulette::slot). */
int car_roulette_slot(int pos);             /* 0x0013B848 */

/* (? name) The prize car roulette.  Not polymorphic; 0x1C bytes. */
struct CarRoulette {
    CarRoulette(s8 n, s8 t);                /* 0x0013B850 count n, target t; remain is not set */
    void start();                           /* 0x0013B880 state 1, speed 0.1 */
    /* state 2: remain = the distance to the middle of the target slot plus
     * count * (12 / count) slots; accel = -speed^2 / (2 (remain + 1)). */
    void stop();                            /* 0x0013B898 */
    /* Once a frame: while stopping the speed stays >= 1/60 and the cursor
     * stops once remain is covered in the target slot; pos wraps to
     * 0 .. count. */
    void update();                          /* 0x0013B960 */
    /* Inline (T107 CarsItem::virtual_1 reads pos and state through its
     * roulette's address, as an inline member does): the slot the cursor
     * is in; the roulette has stopped. */
    int slot() { return car_roulette_slot((int)pos); }
    int isIdle() { return state == 0; }
    /* Inline (T107 CarsItem's constructor: the car won, through the
     * roulette's address and an s8 like the constructor's t). */
    void setTarget(s8 t) { target = t; }

    f32 remain;                             /* 0x00 distance left while stopping */
    f32 pos;                                /* 0x04 the cursor, 0 .. count: slot (int)pos */
    f32 speed;                              /* 0x08 slots a frame */
    f32 accel;                              /* 0x0C added to speed while stopping (negative) */
    int count;                              /* 0x10 */
    int target;                             /* 0x14 the slot it stops on: the car won */
    s8  state;                              /* 0x18 0 idle, 1 spinning, 2 stopping */
};
EGCS_SIZE(gt2k_menu::CarRoulette, 0x1C);

/* (? name) The turntable carrying the prize cars; angles in degrees.  Not
 * polymorphic; 0x30 bytes. */
struct CarTurntable {
    /* halfSize / height per car, in metres (CarsItem::setupLayout: half of
     * T232 MenuCarSpec::length, and MenuCarSpec::height): spacing /
     * maxHeight the largest neighbour sum / height x 1.1, the radius that
     * puts neighbours `spacing' apart, the headings 80..100 degrees apart
     * from a random 0..20. */
    void setup(const f32 *halfSize, const f32 *height);    /* 0x0013BA80 */
    void update();                          /* 0x0013BCE0 once a frame */
    void getCarPos(int i, f32 *x, f32 *z);  /* 0x0013BE88 car i at i * 360 / count on the circle */
    void getWorldPos(int i, f32 *x, f32 *z);    /* 0x0013BF18 the same turned by angle */
    void rotate();                          /* 0x0013BFA0 (no callers) the matrix turned by angle */
    void transformCar(int i);               /* 0x0013BFC0 (no callers) to car i's place and heading */

    f32 angle;                              /* 0x00 of the table, 0 .. 360 */
    f32 speed;                              /* 0x04 added to angle a frame */
    f32 carAngle[4];                        /* 0x08 the heading of each car */
    f32 carSpin;                            /* 0x18 added to every carAngle a frame */
    f32 radius;                             /* 0x1C */
    f32 spacing;                            /* 0x20 the largest (halfSize[i] + halfSize[i + 1]) x 1.1 */
    f32 maxHeight;                          /* 0x24 the largest height x 1.1 */
    s8  count;                              /* 0x28 the cars, 1..4 */
    int stopping;                           /* 0x2C set once the roulette stops: slows the table and the cars */
};
EGCS_SIZE(gt2k_menu::CarTurntable, 0x30);

/* (? names) The floor drawing.  draw_ring_arc: a flat ring between radius
 * 1 and `radius' from angle `from' to `to' (spans over 120 degrees halved
 * recursively; only called by itself); draw_ring (no callers): the whole
 * ring at y = -0.01, innerColor at radius 1, outerColor at `radius';
 * draw_table_floor: the square -1..1 on y = 0 with the texture tex
 * (nothing without one; T107 scales it to the table). */
void draw_ring_arc(f32 from, f32 to, f32 radius);           /* 0x0013C018 */
void draw_ring(u32 outerColor, u32 innerColor, f32 radius);  /* 0x0013C170 */
void draw_table_floor(Tex1Image *tex);                      /* 0x0013C2D8 */

/* vtable 0x003192E8 (9 slots: [1], [8] MenuCarCamera's, [3] .. [7]
 * CameraBase's).  typeinfo __tfQ29gt2k_menu16CarGetMenuCamera 0x0013C880
 * (si, base MenuCarCamera; storage 0x004B1820; the base's node
 * __ti13MenuCarCamera is declared in T411).  vptr +0x00; size 0x154
 * (CarsItem +0xED4).  state: 0 sets up the overview of the table, 1 / 3
 * wait for `next', 2 / 4 step on, 5 follows the car that was won. */
class CarGetMenuCamera : public MenuCarCamera {
public:
    /* Inline: CarsItem's constructor expands it (MenuCarCamera's inline
     * constructor, the vptr, state and next); 0x0013C760 is an out-of-line
     * copy without callers. */
    CarGetMenuCamera() : state(0), next(0) {}   /* 0x0013C760 */
    /* CarsItem's destructor expands it (CameraBase::~CameraBase);
     * 0x0013C8D0 is the copy for the vtable. */
    /* [0] +0x0C 0x0013C8D0 ~CarGetMenuCamera: implicit (no vptr store) */
    /* The view: a look-at from trans to interest, the up vector
     * perpendicular to the sight line in the y-z plane. */
    virtual void applyView();               /* [2] +0x1C 0x0013C668 */
    /* Once a frame (CarsItem::virtual_1): the state step, the inherited
     * springs (MenuCarCamera::virtual_8), then the eye springs.  selected:
     * the slot the roulette shows (-1 none); count: the cars. */
    void update(CarTurntable *table, int selected, int count);  /* 0x0013C3A0 */
    /* Inline (CarsItem::virtual_1 stores the byte itself); 0x0013C750 is an
     * out-of-line copy without callers.  Through an int (u021: virtual_1
     * keeps the 1 in a word register it also compares count with).  Not
     * through an inline member setNext(int) (u021's first form): T106 has
     * the key function, so egcs wrote every inline member out of line and
     * retail's frame info has no such copy between requestNext and the
     * constructor (u020's data pass; T107, T061 compile the same). */
    void requestNext() { int n = 1; next = n; }     /* 0x0013C750 */

    /*           MenuCarCamera                 0x000 .. 0x14C (vptr +0x00) */
    int          state;                     /* 0x14C */
    /* 0x150 a step requested (CarsItem: when the roulette starts and once it
     * has stopped).  An 8-bit field of an unsigned int (bit-field), not a
     * u8: its stores are not char accesses, so they do not alias the other
     * members' stores: the constructor's vptr store between the
     * MotionValues' (0x0013C760, u020) and CarsItem::virtual_1's schedule
     * (u021) are retail's only so; the loads stay lbu. */
    unsigned int next : 8;                  /* (no offsetof for a bit-field) */
};
EGCS_SIZE(gt2k_menu::CarGetMenuCamera, 0x154);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu16CarGetMenuCamera(void);  /* 0x0013C880 */
#endif

#endif /* GAME_T106_CAR_GET_CAMERA_H */
