/* T470 - sound init / term.  0x0022C900..0x0022C960. */
#include "game/sound/gtsound_internal.h"
#include "game/sound/T470_sound_init.h"

/* Called by the game init (0x00102C00). */
// FUNC 0x0022C900 sound_init
void sound_init(void)
{
    F_0025C280(1, 1, 1);                    /* T520: 'SPUP' fno 5 */
    bgm_init();
    se_init();
}

// FUNC 0x0022C938 sound_term
void sound_term(void)
{
    se_term();
    bgm_term();
}
