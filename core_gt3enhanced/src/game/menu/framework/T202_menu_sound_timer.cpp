/* T202 - a menu sound played after a delay.  0x001B28D8..0x001B29A8. */
#include "game/menu/race_menu/race_menu_internal.h"
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T252_timer.h"                /* gt2k_menu::Timer */
#include "game/menu/framework/T202_menu_sound_timer.h"

using namespace gt2k_menu;

/* The Timer's callback (T202 only): the menu sound in its arg. */
// FUNC 0x001B28D8 menu_sound_timer_callback
static void menu_sound_timer_callback(Timer *timer, void *se)
{
    menu_play_se((int)(size_t)se);
}

/* Plays menu sound `se' after `frames' frames: a Timer (Object::operator
 * new(0x2C); its inline constructor: Object(), func, arg, vptr 0x00337130,
 * frames; Object::operator delete if it throws) appended to parent's
 * children.  Callers: T037 action_buy_car, action_change_wheel,
 * action_fit_wheel. */
// FUNC 0x001B28F8 menu_sound_after
void menu_sound_after(Object *parent, int se, int frames)
{
    Timer *timer = new Timer(menu_sound_timer_callback, (void *)(size_t)se, frames);  /* implicit: Object::operator delete (if the constructor throws) */
    timer->attachTo(parent);
}
