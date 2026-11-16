/* T505_host_files.h - host: file helpers (translation unit T505, unit u111),
 * 0x00241E08..0x00242058.
 *
 * The same machine code as the bootstrap's (src/imageloader_source.cpp
 * 0x01005970..0x01005BC0).  Every path gets "host:" in front.  C++ only
 * (C++ linkage; host_write_file returns bool). */
#ifndef GAME_T505_HOST_FILES_H
#define GAME_T505_HOST_FILES_H

#include "game/game.h"

#ifdef __cplusplus

/* len bytes (0 or more than there is: up to the end) from offset into buf
 * (NULL: malloc'ed here).  buf if all of them were read, else NULL (NULL
 * too if the file cannot be opened). */
void *host_read_file(const char *path, void *buf, u32 offset, u32 len); /* 0x00241E08 */
/* Creates / truncates the file and writes len bytes (the result of the
 * write is ignored).  false if it cannot be opened. */
bool  host_write_file(const char *path, const void *buf, u32 len);      /* 0x00241F10 */
u32   host_file_size(const char *path);         /* 0x00241FA0 0 if it cannot be opened */

#endif /* __cplusplus */

#endif /* GAME_T505_HOST_FILES_H */
