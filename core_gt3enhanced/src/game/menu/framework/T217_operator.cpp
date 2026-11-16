/* T217 - gt2k_menu::Operator, the operator script interpreter.
 * 0x001BBC58..0x001BD250.
 *
 * A Transitive runs its scripts once per frame (T216: Transitive::start wraps
 * an Operator in an OperatorHolder, Transitive::virtual_1 steps them).
 * Operator::run executes instructions until one of them returns 1 (its
 * `result` field): most handlers advance pc and return `result`, so a script
 * chooses which steps end the frame.  Timed handlers count `timer` down
 * (OPERATOR_IDLE = not started: the first call loads it with (int)(seconds *
 * 60)) and advance pc when it runs out.  Rates in the scripts are per second
 * (x 1/60 per frame).  The operands are OperatorCode::f10..f20; each handler's
 * comment says what they hold. */
/* The original object's data: the handler table (.data) and, in .rodata
 * (0x003343E0..0x003344B6, no code reference), the type names of the
 * operator classes of T216's header only: OperatorList, List<OperatorHolder>,
 * OperatorHolder (with its base list), Cell<OperatorHolder>, Operator (the
 * same five as the end of T216's .rodata).  T218 has no .rodata, T219's
 * starts at 0x003344C0. */
// DATA_START 0x003343E0
#include "game/menu/framework/T217_operator.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (stopOperator, virtual_5 .. 7),
                                               Drawable (T259), MotionValue (T260) */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current (pdistd::Allocator) */
#include "engine/pgl/G001_flare.h"                /* lcg_rand_range */

namespace gt2k_menu {

/* seconds -> frames (inline: evaluated again at every use, as the code does) */
static inline int toFrames(f32 sec)
{
    return (int)(sec * 60.0f);
}

/* The handlers' shapes, as the matching pass found them: a timed handler
 * tests the member itself (if (timer == OPERATOR_IDLE) timer = ...;
 * if (--timer < 0) ... else ...; one return at the end), and the target is
 * changed through Drawable's inline accessors (incColorR .. A,
 * incAlphaRatio, incBlur2DX / Y, incF0 .. F3, setColorR .. A, getColorR .. A,
 * getW / getH) and MotionValue's (get / setValue, pull), whose expansions
 * keep &x / &y / &color[i] in registers as the code does. */

/* 0: next instruction. */
// FUNC 0x001BBC58 gt2k_menu::Operator::opNext
int Operator::opNext(const OperatorCode *op, Transitive *target)
{
    pc++;
    return op->result;
}

/* 1: relative jump by op->arg. */
// FUNC 0x001BBC70 gt2k_menu::Operator::opJump
int Operator::opJump(const OperatorCode *op, Transitive *target)
{
    pc += op->arg;
    return op->result;
}

/* 2: jumps by op->arg op->arg2 times, then goes on. */
// FUNC 0x001BBC88 gt2k_menu::Operator::opLoop
int Operator::opLoop(const OperatorCode *op, Transitive *target)
{
    if (loop == OPERATOR_IDLE)
        loop = op->arg2;
    if (--loop < 0) {
        loop = OPERATOR_IDLE;
        pc = pc + 1;
    } else {
        pc = pc + op->arg;
    }
    return op->result;
}

/* 3: removes this operator from the target (Transitive::stopOperator;
 * pc is not advanced). */
// FUNC 0x001BBCD8 gt2k_menu::Operator::opRemove
int Operator::opRemove(const OperatorCode *op, Transitive *target)
{
    target->stopOperator(id);
    return op->result;
}

/* 4: back to the first instruction. */
// FUNC 0x001BBD08 gt2k_menu::Operator::opRestart
int Operator::opRestart(const OperatorCode *op, Transitive *target)
{
    pc = 0;
    return op->result;
}

/* 5: waits op->f10 seconds. */
// FUNC 0x001BBD18 gt2k_menu::Operator::opWait
int Operator::opWait(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f10);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    }
    return op->result;
}

/* 6: kills the target (Object::kill, the deferred delete; pc is not
 * advanced). */
// FUNC 0x001BBD68 gt2k_menu::Operator::opRelease
int Operator::opRelease(const OperatorCode *op, Transitive *target)
{
    target->kill();
    return op->result;
}

/* 7: position (f10, f14) and size (f18, f1C). */
// FUNC 0x001BBD98 gt2k_menu::Operator::opSetRect
int Operator::opSetRect(const OperatorCode *op, Transitive *target)
{
    target->x.set(op->f10);
    target->y.set(op->f14);
    pc++;
    target->w = op->f18;
    target->h = op->f1C;
    return op->result;
}

/* 8: random position, x in [f10, f14), y in [f18, f1C). */
// FUNC 0x001BBDE8 gt2k_menu::Operator::opSetRandomPos
int Operator::opSetRandomPos(const OperatorCode *op, Transitive *target)
{
    target->x.set(lcg_rand_range(op->f10, op->f14));
    target->y.set(lcg_rand_range(op->f18, op->f1C));
    pc++;
    return op->result;
}

/* 9: moves by (f10, f14) per second for f18 seconds. */
// FUNC 0x001BBE68 gt2k_menu::Operator::opMove
int Operator::opMove(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f18);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->x.setValue(target->x.get() + op->f10 * (1.0f / 60.0f));
        target->y.setValue(target->y.get() + op->f14 * (1.0f / 60.0f));
    }
    return op->result;
}

/* 10: moves to (f10, f14) in f18 seconds (at once if f18 is 0), linearly,
 * then places it exactly. */
// FUNC 0x001BBEF0 gt2k_menu::Operator::opMoveTo
int Operator::opMoveTo(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE) {
        f32 dx = op->f10 - target->x.get();
        f32 dy = op->f14 - target->y.get();
        if (dx == 0.0f && dy == 0.0f) {
            pc++;
            return op->result;
        }
        if (op->f18 == 0.0f) {
            target->x.set(op->f10);
            pc++;
            target->y.set(op->f14);
            return op->result;
        }
        work[0] = dx / (f32)toFrames(op->f18);
        work[1] = dy / (f32)toFrames(op->f18);
        timer = toFrames(op->f18);
    }
    if (--timer < 0) {
        target->x.set(op->f10);
        target->y.set(op->f14);
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->x.setValue(target->x.get() + work[0]);
        target->y.setValue(target->y.get() + work[1]);
    }
    return op->result;
}

/* 11: size *= (f10, f14). */
// FUNC 0x001BC080 gt2k_menu::Operator::opScale
int Operator::opScale(const OperatorCode *op, Transitive *target)
{
    pc++;
    target->w = target->w * op->f10;
    target->h = target->h * op->f14;
    return op->result;
}

/* 12: target->virtual_5() (Transitive [5]; Image: fitToTexture). */
// FUNC 0x001BC0B8 gt2k_menu::Operator::opVirtual5
int Operator::opVirtual5(const OperatorCode *op, Transitive *target)
{
    target->virtual_5();
    pc++;
    return op->result;
}

/* 13: size grows by (f10, f14) per second for f18 seconds. */
// FUNC 0x001BC108 gt2k_menu::Operator::opGrow
int Operator::opGrow(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f18);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->w = target->w + op->f10 * (1.0f / 60.0f);
        target->h = target->h + op->f14 * (1.0f / 60.0f);
    }
    return op->result;
}

/* 14: resizes to (f10, f14) over f18 seconds, then sets it exactly.  The
 * step is a 60th of the difference whatever f18 is (sic). */
// FUNC 0x001BC188 gt2k_menu::Operator::opResizeTo
int Operator::opResizeTo(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE) {
        f32 dw = op->f10 - target->getW();
        f32 dh = op->f14 - target->getH();
        if (dw == 0.0f && dh == 0.0f) {
            pc++;
            return op->result;
        }
        work[0] = dw * (1.0f / 60.0f);
        work[1] = dh * (1.0f / 60.0f);
        timer = toFrames(op->f18);
    }
    if (--timer < 0) {
        target->w = op->f10;
        timer = OPERATOR_IDLE;
        pc++;
        target->h = op->f14;
    } else {
        target->w = target->w + work[0];
        target->h = target->h + work[1];
    }
    return op->result;
}

/* 15: scales the size by (f10, f14) over f18 seconds (nothing if f18 is 0;
 * no exact set at the end). */
// FUNC 0x001BC270 gt2k_menu::Operator::opScaleTo
int Operator::opScaleTo(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE) {
        f32 w = target->w;
        f32 h = target->h;
        f32 dw = w * op->f10 - w;
        f32 dh = h * op->f14 - h;
        if (dw == 0.0f && dh == 0.0f) {
            pc++;
            return op->result;
        }
        if (op->f18 == 0.0f) {
            pc++;
            return op->result;
        }
        work[0] = dw / (f32)toFrames(op->f18);
        work[1] = dh / (f32)toFrames(op->f18);
        timer = toFrames(op->f18);
    }
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->w = target->w + work[0];
        target->h = target->h + work[1];
    }
    return op->result;
}

/* 16: springs towards (f10, f14), stiffness f18, damping f1C, for f20
 * seconds. */
// FUNC 0x001BC3A8 gt2k_menu::Operator::opSpringTo
int Operator::opSpringTo(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE) {
        work[0] = op->f10;
        work[1] = op->f14;
        work[2] = op->f18;
        work[3] = op->f1C;
        timer = toFrames(op->f20);
    }
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->x.pull(work[0], work[2]);
        target->x.update(work[3]);
        target->y.pull(work[1], work[2]);
        target->y.update(work[3]);
    }
    return op->result;
}

/* 17: eases towards (f10, f14) at rate f18 per frame, for f1C seconds. */
// FUNC 0x001BC4A8 gt2k_menu::Operator::opEaseTo
int Operator::opEaseTo(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE) {
        work[0] = op->f10;
        work[1] = op->f14;
        work[2] = op->f18;
        timer = toFrames(op->f1C);
    }
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->x.approach(work[0], work[2]);
        target->y.approach(work[1], work[2]);
    }
    return op->result;
}

/* 18: the four vertex colours = (f10, f14, f18, f1C). */
// FUNC 0x001BC568 gt2k_menu::Operator::opSetColors
int Operator::opSetColors(const OperatorCode *op, Transitive *target)
{
    for (int i = 0; i < 4; i++) {
        Color4f *c = &target->color[i];
        c->r = op->f10;
        c->g = op->f14;
        c->b = op->f18;
        c->a = op->f1C;
    }
    pc++;
    return op->result;
}

/* 19: vertex colour op->arg = (f10, f14, f18, f1C). */
// FUNC 0x001BC5B8 gt2k_menu::Operator::opSetColor
int Operator::opSetColor(const OperatorCode *op, Transitive *target)
{
    int i = op->arg;
    target->setColorR(i, op->f10);
    pc++;
    target->setColorG(i, op->f14);
    target->setColorB(i, op->f18);
    target->setColorA(i, op->f1C);
    return op->result;
}

/* 20: the four vertex colours change by (f10, f14, f18, f1C) per second,
 * clamped to [0, 1], for f20 seconds. */
// FUNC 0x001BC608 gt2k_menu::Operator::opFadeColors
int Operator::opFadeColors(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f20);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        for (int i = 0; i < 4; i++) {
            target->incColorR(i, op->f10 * (1.0f / 60.0f));
            target->incColorG(i, op->f14 * (1.0f / 60.0f));
            target->incColorB(i, op->f18 * (1.0f / 60.0f));
            target->incColorA(i, op->f1C * (1.0f / 60.0f));
        }
    }
    return op->result;
}

/* 21: the same for vertex colour op->arg only. */
// FUNC 0x001BC780 gt2k_menu::Operator::opFadeColor
int Operator::opFadeColor(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f20);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        int i = op->arg;
        target->incColorR(i, op->f10 * (1.0f / 60.0f));
        target->incColorG(i, op->f14 * (1.0f / 60.0f));
        target->incColorB(i, op->f18 * (1.0f / 60.0f));
        target->incColorA(i, op->f1C * (1.0f / 60.0f));
    }
    return op->result;
}

/* 22: the same for vertex colours op->arg and op->arg2. */
// FUNC 0x001BC8C8 gt2k_menu::Operator::opFadeColor2
int Operator::opFadeColor2(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f20);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        int i = op->arg;
        target->incColorR(i, op->f10 * (1.0f / 60.0f));
        target->incColorG(i, op->f14 * (1.0f / 60.0f));
        target->incColorB(i, op->f18 * (1.0f / 60.0f));
        target->incColorA(i, op->f1C * (1.0f / 60.0f));
        int j = op->arg2;
        target->incColorR(j, op->f10 * (1.0f / 60.0f));
        target->incColorG(j, op->f14 * (1.0f / 60.0f));
        target->incColorB(j, op->f18 * (1.0f / 60.0f));
        target->incColorA(j, op->f1C * (1.0f / 60.0f));
    }
    return op->result;
}

/* 23: fades vertex colour 0 to (f10, f14, f18, f1C) over f20 seconds and
 * copies it to the three others; at the end all four are set exactly.  The
 * step is (target - colour) / f20 * 60 per frame (sic: 3600 times the
 * linear step, so the channels jump to 0 or 1 on the first frame; only the
 * end sets the target). */
// FUNC 0x001BCB08 gt2k_menu::Operator::opFadeColorsTo
int Operator::opFadeColorsTo(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE) {
        timer = toFrames(op->f20);
        if (op->f20 != 0.0f) {
            work[0] = (op->f10 - target->getColorR(0)) / op->f20 * 60.0f;
            work[1] = (op->f14 - target->getColorG(0)) / op->f20 * 60.0f;
            work[2] = (op->f18 - target->getColorB(0)) / op->f20 * 60.0f;
            work[3] = (op->f1C - target->getColorA(0)) / op->f20 * 60.0f;
        }
    }
    if (--timer < 0) {
        for (int i = 0; i < 4; i++) {
            Color4f *c = &target->color[i];
            c->r = op->f10;
            c->g = op->f14;
            c->b = op->f18;
            c->a = op->f1C;
        }
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->incColorR(0, work[0]);
        target->incColorG(0, work[1]);
        target->incColorB(0, work[2]);
        target->incColorA(0, work[3]);
        for (int i = 1; i < 4; i++)
            target->color[i] = target->color[0];
    }
    return op->result;
}

/* 24: alphaRatio = f10. */
// FUNC 0x001BCD48 gt2k_menu::Operator::opSetAlpha
int Operator::opSetAlpha(const OperatorCode *op, Transitive *target)
{
    pc++;
    target->alphaRatio = op->f10;
    return op->result;
}

/* 25: alphaRatio changes by f10 per second, clamped, for f14 seconds. */
// FUNC 0x001BCD68 gt2k_menu::Operator::opFadeAlpha
int Operator::opFadeAlpha(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f14);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->incAlphaRatio(op->f10 * (1.0f / 60.0f));
    }
    return op->result;
}

/* 26: the blur offsets (blur2DX, blur2DY) = (f10, f14). */
// FUNC 0x001BCE00 gt2k_menu::Operator::opSet80
int Operator::opSet80(const OperatorCode *op, Transitive *target)
{
    target->blur2DX = op->f10;
    pc++;
    target->blur2DY = op->f14;
    return op->result;
}

/* 27: the blur offsets change by (f10, f14) per second, clamped to [0, 1],
 * for f18 seconds.  Bug: the second result is stored to blur2DX too, blur2DY
 * never changes (the same slip as Drawable::incBlur2DY, T259). */
// FUNC 0x001BCE28 gt2k_menu::Operator::opFade80
int Operator::opFade80(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f18);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        target->incBlur2DX(op->f10 * (1.0f / 60.0f));
        target->incBlur2DY(op->f14 * (1.0f / 60.0f));
    }
    return op->result;
}

/* 28: the class parameters F0..F3 (Drawable::f) = (f10, f14, f18, f1C). */
// FUNC 0x001BCEF8 gt2k_menu::Operator::opSet8C
int Operator::opSet8C(const OperatorCode *op, Transitive *target)
{
    target->f[0] = op->f10;
    pc++;
    target->f[1] = op->f14;
    target->f[2] = op->f18;
    target->f[3] = op->f1C;
    return op->result;
}

/* 29: F0..F3 change by (f10, f14, f18, f1C) per second for f20
 * seconds.  The add runs four times per frame on the same four values
 * (sic: probably copied from the colour loop). */
// FUNC 0x001BCF30 gt2k_menu::Operator::opAdd8C
int Operator::opAdd8C(const OperatorCode *op, Transitive *target)
{
    if (timer == OPERATOR_IDLE)
        timer = toFrames(op->f20);
    if (--timer < 0) {
        timer = OPERATOR_IDLE;
        pc++;
    } else {
        for (int i = 0; i < 4; i++) {
            target->incF0(op->f10 * (1.0f / 60.0f));
            target->incF1(op->f14 * (1.0f / 60.0f));
            target->incF2(op->f18 * (1.0f / 60.0f));
            target->incF3(op->f1C * (1.0f / 60.0f));
        }
    }
    return op->result;
}

/* 30: calls op->func(target). */
// FUNC 0x001BCFF0 gt2k_menu::Operator::opCall
int Operator::opCall(const OperatorCode *op, Transitive *target)
{
    op->func(target);
    pc++;
    return op->result;
}

/* 31: goes on with the script op->script from its start. */
// FUNC 0x001BD038 gt2k_menu::Operator::opGoto
int Operator::opGoto(const OperatorCode *op, Transitive *target)
{
    script = op->script;
    pc = 0;
    return op->result;
}

/* 32: target->virtual_6() (Transitive [6], empty in Transitive). */
// FUNC 0x001BD050 gt2k_menu::Operator::opVirtual6
int Operator::opVirtual6(const OperatorCode *op, Transitive *target)
{
    target->virtual_6();
    pc++;
    return op->result;
}

/* 33: target->virtual_7() (Transitive [7], empty in Transitive). */
// FUNC 0x001BD0A0 gt2k_menu::Operator::opVirtual7
int Operator::opVirtual7(const OperatorCode *op, Transitive *target)
{
    target->virtual_7();
    pc++;
    return op->result;
}

} /* namespace gt2k_menu */

/* The handler table, by OperatorCode::code: egcs pointers to member
 * functions {s16 delta 0, s16 index -1 (non-virtual), pfn}, static
 * initialisers.  Initialised data (.data, 0x002C8850..0x002C8960, before
 * the frame info); only run uses it. */
gt2k_menu::OperatorHandler g_002C8850[34] = {   /* 0x002C8850 */
    &gt2k_menu::Operator::opNext,           /*  0 */
    &gt2k_menu::Operator::opJump,           /*  1 */
    &gt2k_menu::Operator::opLoop,           /*  2 */
    &gt2k_menu::Operator::opRemove,         /*  3 */
    &gt2k_menu::Operator::opRestart,        /*  4 */
    &gt2k_menu::Operator::opWait,           /*  5 */
    &gt2k_menu::Operator::opRelease,        /*  6 */
    &gt2k_menu::Operator::opSetRect,        /*  7 */
    &gt2k_menu::Operator::opSetRandomPos,   /*  8 */
    &gt2k_menu::Operator::opMove,           /*  9 */
    &gt2k_menu::Operator::opMoveTo,         /* 10 */
    &gt2k_menu::Operator::opScale,          /* 11 */
    &gt2k_menu::Operator::opVirtual5,       /* 12 */
    &gt2k_menu::Operator::opGrow,           /* 13 */
    &gt2k_menu::Operator::opResizeTo,       /* 14 */
    &gt2k_menu::Operator::opScaleTo,        /* 15 */
    &gt2k_menu::Operator::opSpringTo,       /* 16 */
    &gt2k_menu::Operator::opEaseTo,         /* 17 */
    &gt2k_menu::Operator::opSetColors,      /* 18 */
    &gt2k_menu::Operator::opSetColor,       /* 19 */
    &gt2k_menu::Operator::opFadeColors,     /* 20 */
    &gt2k_menu::Operator::opFadeColor,      /* 21 */
    &gt2k_menu::Operator::opFadeColor2,     /* 22 */
    &gt2k_menu::Operator::opFadeColorsTo,   /* 23 */
    &gt2k_menu::Operator::opSetAlpha,       /* 24 */
    &gt2k_menu::Operator::opFadeAlpha,      /* 25 */
    &gt2k_menu::Operator::opSet80,          /* 26 */
    &gt2k_menu::Operator::opFade80,         /* 27 */
    &gt2k_menu::Operator::opSet8C,          /* 28 */
    &gt2k_menu::Operator::opAdd8C,          /* 29 */
    &gt2k_menu::Operator::opCall,           /* 30 */
    &gt2k_menu::Operator::opGoto,           /* 31 */
    &gt2k_menu::Operator::opVirtual6,       /* 32 */
    &gt2k_menu::Operator::opVirtual7,       /* 33 */
};

namespace gt2k_menu {

/* Runs instructions until a handler returns 1 (g_002C8850[op->code], a
 * non-virtual pointer to member).  script is tested once, on entry: the loop
 * reloads it (opGoto changes it) but does not test it again. */
// FUNC 0x001BD0F0 gt2k_menu::Operator::run
void Operator::run(Transitive *target)
{
    if (script != NULL) {
        const OperatorCode *op;
        do {
            op = &script[pc];
        } while ((this->*g_002C8850[op->code])(op, target) != 1);
    }
}

/* Transitive::start calls it on a new holder (after the inline Operator()). */
// FUNC 0x001BD178 gt2k_menu::Operator::init
void Operator::init(const OperatorCode *start, int holderId)
{
    script = start;
    id = holderId;
    pc = 0;
    loop = OPERATOR_IDLE;
    timer = OPERATOR_IDLE;
    work[0] = 0;
    work[1] = 0;
    work[2] = 0;
    work[3] = 0;
}

/* The current menu heap (T204), 16-byte aligned.  (T216's
 * OperatorHolder::operator new 0x001BB430 is the same plus a heap dump on
 * failure; T216's F_001BB488 / F_001BB4D8, without callers, forward to this
 * operator new / delete.)  The [] forms have no callers. */
// FUNC 0x001BD1A8 gt2k_menu::Operator::operator new
void *Operator::operator new(size_t size)
{
    return menu_heap_current()->allocate(size, 0x10);
}

// FUNC 0x001BD1E0 gt2k_menu::Operator::operator new[]
void *Operator::operator new[](size_t size)
{
    return operator new(size);
}

// FUNC 0x001BD200 gt2k_menu::Operator::operator delete
void Operator::operator delete(void *block)
{
    menu_heap_current()->free(block);
}

// FUNC 0x001BD230 gt2k_menu::Operator::operator delete[]
void Operator::operator delete[](void *block)
{
    operator delete(block);
}

} /* namespace gt2k_menu */
