/* T252_timer.h - gt2k_menu::Timer (translation unit T252, unit u071),
 * 0x001CCA88..0x001CCBD0.
 *
 * An Object of the menu tree that counts frames down, then calls func(this,
 * arg) and kills itself: RenderBase::sleep (T246), the delayed menu sounds
 * (T202), the page fades of the PageManager (T211). */
#ifndef GAME_T252_TIMER_H
#define GAME_T252_TIMER_H

#include "game/game.h"
#include "game/menu/framework/T220_object.h"               /* base gt2k_menu::Object */

namespace gt2k_menu {
class Timer;

/* Called once the frames have run out. */
typedef void (*TimerFunc)(Timer *timer, void *arg);

/* vtable 0x00337130 (5 slots: [2] Object's 0x001BFEF8, [3] / [4]
 * 0x001BFF00 / 0x001BFF08).  typeinfo __tfQ29gt2k_menu5Timer 0x001CCB38 (si,
 * base Object; __si_type_info 0x004B2680).  Size 0x2C (Object::operator new
 * in RenderBase::sleep). */
class Timer : public Object {
public:
    /* Inline in the original: RenderBase::sleep (T246), T202 and
     * PageManager::startPage (T211) expand it; T252 also has an out-of-line
     * copy with no callers: 0x001CCAD0. */
    Timer(TimerFunc f, void *a, int n) { func = f; arg = a; frames = n; }
    /*  [0] +0x0C 0x001CCB88 the implicit destructor: only ~Object, no vptr store */
    virtual void virtual_1();               /* [1] +0x14 0x001CCA88 update: frames - 1; below 0:
                                               func(this, arg), kill() */

    TimerFunc func;                         /* 0x20 */
    void     *arg;                          /* 0x24 */
    int       frames;                       /* 0x28 frames left */
};
EGCS_SIZE(gt2k_menu::Timer, 0x2C);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu5Timer(void);         /* 0x001CCB38 */
}
#endif

#endif /* GAME_T252_TIMER_H */
