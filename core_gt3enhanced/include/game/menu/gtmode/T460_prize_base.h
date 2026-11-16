/* T460_prize_base.h - PrizeBase, CallBackWhenPrizeRendering and the prize
 * camera (translation unit T460, unit u103; PrizeModelSetContext, whose
 * vtable and typeinfo function T460 emits, is in T459_prize_model.h).
 *
 * PrizeBase holds a loaded .prm model (PrizeModel, T459) and its pose (the
 * context): load / setModel, update (one animation step while updateFlag is
 * 1), draw (the root matrix, the lights, a flattened shadow pass per light,
 * each 0.1 lower: the ModelSet's part 1, then the model: part 0; an optional
 * callback object before each pass).  Base of MiniPrize (T145, then LicensePrize T146); also used by the
 * trophy items of the menus (T058).
 */
#ifndef GAME_T460_PRIZE_BASE_H
#define GAME_T460_PRIZE_BASE_H

#include "game/game.h"
#include "game/menu/gtmode/T459_prize_model.h"              /* PrizeModel, PrizeModelSetContext (held by value) */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions and their storage itself (and
 * rejects hand declarations). */
extern "C" {
void *__tf9PrizeBase(void);                     /* 0x0022A300 */
void *__tf20PrizeModelSetContext(void);         /* 0x0022A370 __user_type_info 0x00353330 */
/* the base node of MiniPrize's __si_type_info (T145) */
extern u32 __ti9PrizeBase[2];                   /* 0x00353460 gp-0x4590 __user_type_info */
extern u32 __ti20PrizeModelSetContext[2];       /* 0x00353330 gp-0x46C0 __user_type_info */
}
#endif

/* PrizeModelSetContext (vtable 0x00341430, typeinfo 0x0022A370) and
 * prize_matrix_set_diagonal (0x0022A3B0): T459_prize_model.h, above (u103's
 * data pass moved them there; the classes come in the same order). */

/* ? owner (only its RTTI name "26CallBackWhenPrizeRendering" is in T460's
 * .rodata; no vtable of it is known).  The callback object of
 * PrizeBase::draw; vptr +0x00.  All its virtual functions are inline (no
 * key function): egcs then writes its type name into the .rodata of every
 * unit that sees the class, as retail's T460 (between "9PrizeBase" and
 * "20PrizeModelSetContext"), T141, T107 and T061 have it (u103; the empty
 * bodies are a guess: no unit defines or constructs one). */
class CallBackWhenPrizeRendering {
public:
    virtual ~CallBackWhenPrizeRendering() {}    /* [0] +0x0C (? the destructor) */
    /* Before each pass of the model (each shadow, then the model). */
    virtual void beforePass() {}                /* [1] +0x14 (virtual_1) */
    /* Once, after the root matrix. */
    virtual void afterRoot() {}                 /* [2] +0x1C (virtual_2) */
};

/* vtable 0x00341408 (4 slots), typeinfo __tf9PrizeBase 0x0022A300.  0x8F
 * bytes of data, egcs vptr +0x90; 0x94 bytes. */
class PrizeBase {
public:
    PrizeBase();                                /* 0x00229C80 */
    /* One animation step (prize_model_step). */
    virtual void step();                        /* [0] +0x0C 0x00229F00 */
    /* Calls PrizeBase::unload (not through the vtable). */
    virtual ~PrizeBase();                       /* [1] +0x14 0x00229D00 */
    /* Unloads, reads the file into buf (size bytes; a new buffer when buf
     * is NULL), relocates it, resets the pose.  keep = 1: unload will not
     * free it.  keep is an int (stored in the u8 field): retail neither
     * masks it here nor in MiniPrize::loadPrize / setup, which pass theirs
     * on (u048). */
    virtual void load(const char *path, void *buf, int size, int keep); /* [2] +0x1C 0x00229DA8 */
    /* Frees the model unless kept. */
    virtual void unload();                      /* [3] +0x24 0x00229E78 */

    /* Takes a model already in memory (kept), relocates it, resets the pose. */
    void setModel(PrizeModel *m);               /* 0x00229D58 (? name) */
    void update();                              /* 0x00229EB8 step() if a model and updateFlag == 1 */
    void restart();                             /* 0x00229F28 (? name) the pose and the model back to the start */
    void show();                                /* 0x00229F68 both flags 1, restart */
    void hide();                                /* 0x00229F90 both flags 0 */
    /* The model must be there.  cb may be NULL.  root, cb->afterRoot(),
     * the lighting and the lights; per light its shadow matrix at y =
     * -0.1 * (i + 1), body, cb->beforePass() and the ModelSet's part 1;
     * then body, cb->beforePass() and part 0 (each pass inside a matrix
     * push / pop). */
    void draw(CallBackWhenPrizeRendering *cb);  /* 0x00229FA0 */
    /* Inline in the original: T460's copies 0x0022A258..0x0022A2E0 (in
     * this order) have no callers and sit with the code egcs emits at the
     * end of the unit (before the typeinfo functions); the trophy items
     * (T058, u019) expand setDispFlag, setUpdateFlag, getFocus and
     * getEyeDelta.  The getters give 0 without a model. */
    /* Declared in reverse address order: egcs emits a class's inline
     * copies in reverse declaration order (u048's finding for T141 / T145),
     * so T460's copies come out in retail's order. */
    void applyRoot() { context.applyRoot(); }  /* (? name) */
    void applyBody() { context.applyBody(); }  /* (? name) */
    u8   getDispFlag() const { return dispFlag; }
    u8   getUpdateFlag() const { return updateFlag; }
    void setDispFlag(u8 f) { dispFlag = f; }
    void setUpdateFlag(u8 f) { updateFlag = f; }
    int  getSize() const { return model != 0 ? model->size : 0; }
    f32  getFocus() const { return model != 0 ? model->focus : 0.0f; }
    f32  getEyeDelta() const { return model != 0 ? model->eyeDelta : 0.0f; }

    PrizeModel          *model;                 /* 0x00 */
    PrizeModelSetContext context;               /* 0x04 (its vptr +0x88) */
    u8                   dispFlag;              /* 0x8C */
    u8                   updateFlag;            /* 0x8D */
    u8                   keep;                  /* 0x8E 1: the model is the caller's buffer */
    /*                   vptr                      0x90 */
};
EGCS_SIZE(PrizeBase, 0x94);

/* The prize camera (? names). */
f32  F_0022A110(void);                          /* 0x0022A110 pi / 3 (? the field of view; no caller) */
/* The projection, multiplied into the current matrix: y divided by 1.098
 * (F_00257EF0, ? the pixel aspect), x scaled by 0.75 in the 16:9 mode, then
 * a frustum of half width tan(30 deg) and half height 0.7 tan(30 deg) at
 * near 0.1 (far 300; pgl_frustum's focal 0.1, the near distance), shifted by
 * (dx, dy). */
void prize_projection(f32 dx, f32 dy);          /* 0x0022A120 */
/* The off-axis frustum of an eye moved by (dx, dy) that keeps the plane at
 * the focus distance in place: shifted by -(dx, dy) * 0.1 / focus (the
 * parallax of T058's trophy ring). */
void prize_projection_eye(f32 dx, f32 dy, f32 focus);   /* 0x0022A200 */
/* The matching eye translation: pgl_translate(-dx, -dy, 0). */
void prize_eye_translate(f32 dx, f32 dy);       /* 0x0022A230 */

#endif /* GAME_T460_PRIZE_BASE_H */
