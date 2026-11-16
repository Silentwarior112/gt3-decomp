/* T217_operator.h - gt2k_menu::Operator, the operator script interpreter
 * (translation unit T217, unit u066), 0x001BBC58..0x001BD250.
 *
 * An operator script is a static array of OperatorCode instructions that
 * animates a gt2k_menu::Transitive (T216): position, size, colours, alpha,
 * waits, loops, calls.  Transitive::start wraps an Operator in an
 * OperatorHolder (T216) and steps it once per frame (Operator::run).  run
 * executes instructions until a handler returns 1 (most handlers advance pc
 * and return the instruction's `result`, so the script chooses which steps
 * end the frame).  Timed handlers count `timer` down (-0x80 = not started:
 * the first call loads (int)(seconds * 60)).  Times are in seconds, rates per
 * second.
 */
#ifndef GAME_T217_OPERATOR_H
#define GAME_T217_OPERATOR_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */

namespace gt2k_menu {

class Transitive;                           /* T216 (pointers only) */

/* One instruction of an operator script (static data), 0x30 bytes.  The
 * scripts of other units (Item's fade in / out g_00333D80 / g_00333E08, the
 * default close script g_003341E8, the page transitions of T229 / T211) are
 * arrays of these.  (u066, the interpreter's unit, called it OperatorOp.) */
struct OperatorCode {
    int code;                               /* 0x00 handler index 0..33 (g_002C8850) */
    union {                                 /* 0x04 */
        int arg;                            /* jump offset, colour index, ... */
        const OperatorCode *script;         /* code 31 (opGoto): the script to go on with */
        void (*func)(Transitive *target);   /* code 30 (opCall): the function to call */
    };
    int arg2;                               /* 0x08 loop count, second colour index */
    u8  unk_0C[4];                          /* 0x0C */
    f32 f10;                                /* 0x10 the operands (seconds, rates, targets) */
    f32 f14;                                /* 0x14 */
    f32 f18;                                /* 0x18 */
    f32 f1C;                                /* 0x1C */
    f32 f20;                                /* 0x20 */
    u8  unk_24[8];                          /* 0x24 */
    int result;                             /* 0x2C what the handler returns: 1 ends the frame */
};

/* The interpreter state of a running script: a user type (no vtable, RTTI
 * __tfQ29gt2k_menu8Operator 0x001BBC18, __user_type_info 0x00353300
 * gp-0x46F0, built in T216).  0x28 bytes: the first base of
 * gt2k_menu::OperatorHolder (T216), whose Cell<OperatorHolder> part follows
 * at +0x28.  Not polymorphic, so the pointer-to-member calls of run() have
 * no virtual test.  Transitive::start expands an inline constructor (the
 * stores 0x001BB6D8..0x001BB714: script / pc 0, loop / timer -0x80, the
 * rest 0) before init(). */
/* Operator::loop / timer while no loop or timed instruction runs (u066's
 * name). */
enum { OPERATOR_IDLE = -0x80 };

class Operator {
public:
    /* unk24 after work[] (u065 matching: Transitive::start stores them in
     * this order). */
    Operator() : script(0), pc(0), loop(OPERATOR_IDLE), timer(OPERATOR_IDLE)
    {
        work[0] = 0; work[1] = 0; work[2] = 0; work[3] = 0;
        unk24 = 0;
    }
    static void *operator new(size_t size);         /* 0x001BD1A8 menu heap, 16-aligned */
    static void *operator new[](size_t size);       /* 0x001BD1E0 */
    static void  operator delete(void *p);          /* 0x001BD200 */
    static void  operator delete[](void *p);        /* 0x001BD230 */

    /* script = s, id = i, pc = 0, loop / timer = -0x80, work = 0. */
    void init(const OperatorCode *s, int i);        /* 0x001BD178 */
    /* Runs instructions until a handler returns 1 (script tested once). */
    void run(Transitive *target);                   /* 0x001BD0F0 */

    /* The handlers, in table order (code 0..33). */
    int opNext(const OperatorCode *op, Transitive *t);         /*  0 0x001BBC58 */
    int opJump(const OperatorCode *op, Transitive *t);         /*  1 0x001BBC70 */
    int opLoop(const OperatorCode *op, Transitive *t);         /*  2 0x001BBC88 */
    int opRemove(const OperatorCode *op, Transitive *t);       /*  3 0x001BBCD8 */
    int opRestart(const OperatorCode *op, Transitive *t);      /*  4 0x001BBD08 */
    int opWait(const OperatorCode *op, Transitive *t);         /*  5 0x001BBD18 */
    int opRelease(const OperatorCode *op, Transitive *t);      /*  6 0x001BBD68 */
    int opSetRect(const OperatorCode *op, Transitive *t);      /*  7 0x001BBD98 */
    int opSetRandomPos(const OperatorCode *op, Transitive *t); /*  8 0x001BBDE8 */
    int opMove(const OperatorCode *op, Transitive *t);         /*  9 0x001BBE68 */
    int opMoveTo(const OperatorCode *op, Transitive *t);       /* 10 0x001BBEF0 */
    int opScale(const OperatorCode *op, Transitive *t);        /* 11 0x001BC080 */
    int opVirtual5(const OperatorCode *op, Transitive *t);     /* 12 0x001BC0B8 t->virtual_5() */
    int opGrow(const OperatorCode *op, Transitive *t);         /* 13 0x001BC108 */
    int opResizeTo(const OperatorCode *op, Transitive *t);     /* 14 0x001BC188 */
    int opScaleTo(const OperatorCode *op, Transitive *t);      /* 15 0x001BC270 */
    int opSpringTo(const OperatorCode *op, Transitive *t);     /* 16 0x001BC3A8 */
    int opEaseTo(const OperatorCode *op, Transitive *t);       /* 17 0x001BC4A8 */
    int opSetColors(const OperatorCode *op, Transitive *t);    /* 18 0x001BC568 */
    int opSetColor(const OperatorCode *op, Transitive *t);     /* 19 0x001BC5B8 */
    int opFadeColors(const OperatorCode *op, Transitive *t);   /* 20 0x001BC608 */
    int opFadeColor(const OperatorCode *op, Transitive *t);    /* 21 0x001BC780 */
    int opFadeColor2(const OperatorCode *op, Transitive *t);   /* 22 0x001BC8C8 */
    int opFadeColorsTo(const OperatorCode *op, Transitive *t); /* 23 0x001BCB08 */
    int opSetAlpha(const OperatorCode *op, Transitive *t);     /* 24 0x001BCD48 */
    int opFadeAlpha(const OperatorCode *op, Transitive *t);    /* 25 0x001BCD68 */
    int opSet80(const OperatorCode *op, Transitive *t);        /* 26 0x001BCE00 Drawable blur2DX / Y */
    int opFade80(const OperatorCode *op, Transitive *t);       /* 27 0x001BCE28 */
    int opSet8C(const OperatorCode *op, Transitive *t);        /* 28 0x001BCEF8 Drawable f[0..3] */
    int opAdd8C(const OperatorCode *op, Transitive *t);        /* 29 0x001BCF30 */
    int opCall(const OperatorCode *op, Transitive *t);         /* 30 0x001BCFF0 op->func(t) */
    int opGoto(const OperatorCode *op, Transitive *t);         /* 31 0x001BD038 */
    int opVirtual6(const OperatorCode *op, Transitive *t);     /* 32 0x001BD050 t->virtual_6() */
    int opVirtual7(const OperatorCode *op, Transitive *t);     /* 33 0x001BD0A0 t->virtual_7() */

    const OperatorCode *script;             /* 0x00 NULL: nothing to run */
    int id;                                 /* 0x04 the holder's id (Transitive::stopOperator) */
    int pc;                                 /* 0x08 index of the current instruction */
    int loop;                               /* 0x0C loop counter, OPERATOR_IDLE = idle */
    int timer;                              /* 0x10 frames left, OPERATOR_IDLE = idle */
    f32 work[4];                            /* 0x14 per-instruction state (rates, targets) */
    int unk24;                              /* 0x24 */
};

/* An entry of the handler table g_002C8850 (T217 .data, only used by run):
 * egcs pointers to member functions {s16 delta 0, s16 index -1
 * (non-virtual), pfn}, 34 entries in code order. */
typedef int (Operator::*OperatorHandler)(const OperatorCode *op, Transitive *t);

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::OperatorCode, 0x30);
EGCS_SIZE(gt2k_menu::Operator, 0x28);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu8Operator(void);   /* 0x001BBC18 (emitted in T216) */
#endif

#endif /* GAME_T217_OPERATOR_H */
