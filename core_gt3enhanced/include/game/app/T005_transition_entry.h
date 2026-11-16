/* T005_transition_entry.h - start_transition, the screen transitions' entry
 * point (translation unit T005, unit u001; the classes are in
 * T005_transition.h).
 *
 * In its own header since u044's data pass (2026-10-17): of the originals,
 * only T005's .rodata has the transition objects' type names
 * (ComboutTransitionObject .. TransitionObject); its callers
 * RacePS2Base::virtual_41 (T130) and RenderBase (T246) saw start_transition
 * without the classes (egcs writes the type name of every class it sees
 * whose virtual functions are all inline).  T005_transition.h includes this
 * header where it declared the function. */
#ifndef GAME_T005_TRANSITION_ENTRY_H
#define GAME_T005_TRANSITION_ENTRY_H

#include "game/game.h"

/* Sets up one of T005's four transition objects and makes it the render
 * thread's object: type 0 fade-out, 1 delayed fade-out, 2 line-out, 3
 * comb-out (T246's enum TransitionType); other types do nothing. */
void start_transition(int type, int flag);  /* 0x00102228 type: TransitionType (T246) */

#endif /* GAME_T005_TRANSITION_ENTRY_H */
