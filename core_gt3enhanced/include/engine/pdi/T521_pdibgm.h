/* T521_pdibgm.h - pdibgm.cxx: the streamed background music over the IOP
 * (translation unit T521, unit u119). */
#ifndef GAME_T521_PDIBGM_H
#define GAME_T521_PDIBGM_H

#include "game/game.h"

/* The file's RPC object (0x004A72E0, server 'PBGM' 0x5042474D) is of the class
 * RPC of its anonymous namespace (_GLOBAL_$N$.._.._pdibgm.cxxpLWmzt), derived
 * from PDIClientRpc (T527): vtable 0x003458D8, typeinfo 0x0025CDB0, 0xC0 bytes.
 * RPC() 0x0025C830, ~RPC() [0] +0x0C 0x0025C878, [1] PDIClientRpc::onCallEnd;
 * inline: updateStatus 0x0025CB88, setVolume 0x0025CC48, start 0x0025CC98,
 * stop 0x0025CD50, resume 0x0025CD70, pause 0x0025CD90.  Functions: 1 start,
 * 2 stop, 3 volume, 4 pause, 5 resume, 6 status.  The class stays defined in
 * src/engine/pdi/T521_pdibgm.cpp (see T519_pdicdvd.h). */

void pdibgm_init(void);                                     /* 0x0025C8A0 binds 'PBGM' */
/* Starts the stream (NOWAIT) of sectors sectors from lsn (as file_locate
 * resolves a file: hostName "" = the disc, else a host: file); the status time
 * is reset to -1. */
void pdibgm_start(int lsn, int sectors, int arg, int bufSize, const char *hostName); /* 0x0025C8C8 */
void pdibgm_pause(void);                                    /* 0x0025C980 */
void pdibgm_resume(void);                                   /* 0x0025C9A8 */
void pdibgm_stop(void);                                     /* 0x0025C9D0 */
/* volume is clamped to [-1, 1] and sent as volume * 32768. */
void pdibgm_set_volume(f32 volume);                         /* 0x0025C9F8 */
void pdibgm_update(void);                                   /* 0x0025CA78 per frame: fetches the status (fno 6) */
/* The stream position in seconds; *pos, *id (changes with the song) and
 * *ended (the song finished) are filled when not NULL. */
f32  pdibgm_get_status(int *pos, int *id, int *ended);      /* 0x0025CA98 */
/* T521_static_fini 0x0025CB40 (.dtors) and T521_static_init 0x0025CB68 (.ctors) are
 * egcs's own static destructor / initialiser of the object (no source
 * functions: the objects are defined in the .cpp file). */

#if defined(__cplusplus) && !defined(BOOT_OLD_GCC)  /* egcs generates it itself */
extern "C" void *tf_pdibgm_RPC(void);   /* 0x0025CDB0 __tfQ233_GLOBAL_$N$.._.._pdibgm.cxxpLWmzt3RPC */
#endif

#endif /* GAME_T521_PDIBGM_H */
