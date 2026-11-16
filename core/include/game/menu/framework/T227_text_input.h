/* T227_text_input.h - the text input page loop (translation unit T227, unit
 * u067).  Callers: T097 (name entry, u013), T068 (memory card, u026). */
#ifndef GAME_T227_TEXT_INPUT_H
#define GAME_T227_TEXT_INPUT_H

#include "game/game.h"

namespace gt2k_menu {
class PageManager;                          /* T211 (pointers only) */
}

/* Runs page `page' of pm as a text input: hands the TextBox statics (T231)
 * the limit maxLen, the output buffer out and a copy of initial (a 0x40-byte
 * local, strcpy), opens the page (setPage, startPage) and runs it frame by
 * frame (update, one vsync) until it is left (exited); in a frame where
 * START (CONTROL_START, T474) is pressed on pm->control's first pad the
 * current list's cursor goes to button 6 instead (? the keyboard page's end
 * key); abortFn(abortArg) != 0 sets pm->exitRequest (the expanded doExit).
 * Then closeAndExit and two vsyncs.  Returns pm->decided. */
int run_text_input(gt2k_menu::PageManager *pm, const char *page, const char *initial, char *out,
                   int maxLen, int (*abortFn)(void *), void *abortArg); /* 0x001C1B88 */

#endif /* GAME_T227_TEXT_INPUT_H */
