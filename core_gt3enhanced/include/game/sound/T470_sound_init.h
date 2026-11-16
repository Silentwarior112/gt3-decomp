/* T470_sound_init.h - sound init / term (translation unit T470, unit u104). */
#ifndef GAME_T470_SOUND_INIT_H
#define GAME_T470_SOUND_INIT_H

#include "game/game.h"

/* F_0025C280(1, 1, 1) (T520, 'SPUP' fno 5), bgm_init, se_init.  Called by
 * the game init (0x00102C00). */
void sound_init(void);                          /* 0x0022C900 (F_0022C900) */
/* se_term, bgm_term. */
void sound_term(void);                          /* 0x0022C938 (F_0022C938) */

#endif /* GAME_T470_SOUND_INIT_H */
