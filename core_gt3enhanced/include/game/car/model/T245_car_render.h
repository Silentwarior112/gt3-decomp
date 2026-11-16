/* T245_car_render.h - CarRender (? name, as in u064 / u070) (translation unit
 * T245, unit u070), 0x001CA550..0x001CB3F0.
 *
 * Draws the 3D car of the menu screens (gt2k_menu::Car, T212): the body
 * CarModel and its four MenuTireModels lit by the light set of the camera
 * (T215).  draw() renders `passes' jittered copies (the sub-pixel offsets of
 * the table 0x002CCFC0[passes]) into buf0 and accumulates them with weights
 * 1 / (i + 1) (anti-aliasing by a running average); the passes from `split'
 * on use the previous body colour, so the colour cross-fades with `blend'.
 * The result goes over the screen with alpha; the mirror pass (the
 * reflection on the floor) is drawn upside down at 0.65 of the alpha. */
#ifndef GAME_T245_CAR_RENDER_H
#define GAME_T245_CAR_RENDER_H

#include "game/game.h"
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer (buf0, buf1) */

struct MenuCarSpec;                         /* T232 (pointers only) */
class MenuCarCamera;                        /* T411 */
class CarModel;                             /* T446 */
class MenuTireModel;                        /* T450 */
struct Tex1Image;                           /* G001 */

/* Not polymorphic.  Size 0xA4 (a member of gt2k_menu::Car, T212). */
class CarRender {
public:
    CarRender(MenuCarSpec *spec, MenuCarCamera *camera);   /* 0x001CA550 viewport (320, 112, 640, 224) */
    void setColor(CarModel *model, MenuTireModel *tires, int color); /* 0x001CA658 the colour variation of
                                               the body and the four tyres */
    void setLights(int mirror);             /* 0x001CA690 the ambient colour and the three lights of
                                               the camera's light set */
    int  hiddenWheel();                     /* 0x001CA9A8 (F_001CA9A8) the wheel whose direction from the car is
                                               nearest to the camera's orbit yaw + 180 (? the one on
                                               the far side, hidden behind the body), -1 if none (no
                                               callers; drawModel's `hidden' wheel is -1) */
    void drawModel(CarModel *model, MenuTireModel *tires, int mirror, int withTire); /* 0x001CABE0 one
                                               pass: lights, camera, body, the tyres in place */
    void beginFrame();                      /* 0x001CAE58 buf0 / buf1: copies of the screen buffer with
                                               VRAM of their own */
    void endFrame();                        /* 0x001CAEC8 frees their VRAM */
    /* Inline (no out-of-line copies; T212's Car::updateViewport / setView /
     * the Car constructor show them: `this' is a pointer to the member).
     * The viewport from a rectangle, y and the height halved (a
     * field-height buffer). */
    void setViewport(f32 x, f32 y, f32 w, f32 h)
    {
        vpX = x;
        vpY = y * 0.5f;
        vpW = w;
        vpH = h * 0.5f;
    }
    /* camera's projection (-320..320 x -224..224, 0.1 .. 1000, focal) and
     * tilt; defined in game/menu/framework/T212_menu_car.h, after MenuCarCamera (T411). */
    inline void setView(f32 focal, f32 tiltX, f32 tiltY);
    void setEnvImage(Tex1Image *image) { envImage = image; }
    /* passes 1..5; blend 0: every pass in color, towards 1: more of them in
     * prevColor (all but the first; Car's blend decays from 1 after a colour
     * change); mirror: the reflection (upside down, added at 0.65 alpha);
     * lamps: CarModel::setLamps; withTire: MenuTireModel::draw's. */
    void draw(CarModel *model, MenuTireModel *tires, int color, int prevColor, int mirror,
              int lamps, int withTire, f32 alpha, f32 blend, int passes); /* 0x001CAEF8 */

    MenuCarSpec   *spec;                    /* 0x00 */
    MenuCarCamera *camera;                  /* 0x04 */
    Tex1Image     *envImage;                /* 0x08 the texture the car reflects, loaded into a TexSet
                                               every draw (the Car constructor's first argument) */
    int            unk0C;                   /* 0x0C 1 */
    int            unk10;                   /* 0x10 1 */
    GSBuffer       buf0;                    /* 0x14 the passes are drawn into it */
    GSBuffer       buf1;                    /* 0x4C the accumulation */
    f32            unk84;                   /* 0x84 0.5 */
    f32            unk88;                   /* 0x88 0.5 */
    f32            unk8C;                   /* 0x8C 0.5 */
    f32            unk90;                   /* 0x90 0.5 */
    f32            vpX;                     /* 0x94 viewport (setViewport): 320 */
    f32            vpY;                     /* 0x98 112 */
    f32            vpW;                     /* 0x9C 640 */
    f32            vpH;                     /* 0xA0 224 */
};
EGCS_SIZE(CarRender, 0xA4);

#endif /* GAME_T245_CAR_RENDER_H */
