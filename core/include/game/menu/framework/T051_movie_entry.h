/* T051_movie_entry.h - play_movie, the full-screen movie player's entry
 * point (translation unit T051, unit u013; the classes are in
 * T051_movie_player.h).
 *
 * In its own header since u001's data pass (2026-10-17): of the originals,
 * only T051's .rodata has MovieControl's type name; its callers main (T000),
 * arcade_menu (T015) and F_0013F508 (T061) saw play_movie without the
 * controller classes (egcs writes the type name of every class it sees whose
 * virtual functions are all inline).  T051_movie_player.h includes this
 * header where it declared the function. */
#ifndef GAME_T051_MOVIE_ENTRY_H
#define GAME_T051_MOVIE_ENTRY_H

#include "game/game.h"

struct MovieParam;                          /* T000: file, size, flags (pointers only) */

/* Plays p (MenuBgMovie::start) and returns when it is over.  Callers: main
 * (T000), arcade_menu (T015), F_0013F508 (T061). */
void play_movie(struct MovieParam *p);      /* 0x001287A8 */

#endif /* GAME_T051_MOVIE_ENTRY_H */
