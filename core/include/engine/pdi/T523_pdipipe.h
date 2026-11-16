/* T523_pdipipe.h - pdipipe.cxx: streamed file reads over the IOP
 * (translation unit T523, unit u119). */
#ifndef GAME_T523_PDIPIPE_H
#define GAME_T523_PDIPIPE_H

#include "game/game.h"

/* The file's RPC object (0x004A7520, server 'PDIP' 0x50444950) is of the class
 * RPC of its anonymous namespace (_GLOBAL_$N$.._.._pdipipe.cxxIVS7RR), derived
 * from PDIClientRpc (T527): vtable 0x00345968, typeinfo 0x0025D9B0, 0xC0 bytes.
 * RPC() 0x0025D348, ~RPC() [0] +0x0C 0x0025D390, [1] PDIClientRpc::onCallEnd,
 * open 0x0025D3B8, transfer 0x0025D480; inline: F_0025D7A8, close 0x0025D870,
 * request 0x0025D8C0, F_0025D940.  Functions (all blocking): 1 close, 2 / 5
 * open, 3 / 6 read into an EE buffer, 4 read into SPU2 memory (the sound
 * bank samples of file_read_sound_bank), 7 / 8 a name with a two-word answer.  The class stays defined in
 * src/engine/pdi/T523_pdipipe.cpp (see T519_pdicdvd.h). */

void pdipipe_init(void);                                    /* 0x0025D520 binds 'PDIP' */
/* Opens a file stream (fno 2) of size bytes: hostPath NULL = from sector lsn
 * of the disc, else the host: file hostPath (lsn = the byte offset in it);
 * the callers pass bufSize 0x40000.  Returns the handle. */
int  pdipipe_open(int lsn, int size, int bufSize, const char *hostPath); /* 0x0025D548 */
int  F_0025D580(int lsn, int size, int bufSize, const char *hostPath);   /* 0x0025D580 the same with fno 5 */
/* Reads the next size bytes of stream h into buf (fno 3). */
void pdipipe_read(int h, void *buf, int size);              /* 0x0025D5B8 */
/* toSpu == 0: pdipipe_read; else fno 4 {h, buf, size} without the D-cache
 * write-backs (buf: a SPU address). */
void pdipipe_read2(int h, void *buf, int size, int toSpu);  /* 0x0025D5F0 */
void F_0025D690(int h, void *buf, int size);                /* 0x0025D690 pdipipe_read with fno 6 */
void pdipipe_close(int h);                                  /* 0x0025D6C8 */
/* fno 8 (flag) or 7 with name; *out0 / *out1 = the answer words; -1 when
 * *out0 is -1, else 0. */
int  F_0025D720(const char *name, int *out0, int *out1, int flag); /* 0x0025D720 */
/* T523_static_fini 0x0025D760 (.dtors) and T523_static_init 0x0025D788 (.ctors) are
 * egcs's own static destructor / initialiser of the object (no source
 * functions: the objects are defined in the .cpp file). */

#if defined(__cplusplus) && !defined(BOOT_OLD_GCC)  /* egcs generates it itself */
extern "C" void *tf_pdipipe_RPC(void);  /* 0x0025D9B0 __tfQ234_GLOBAL_$N$.._.._pdipipe.cxxIVS7RR3RPC */
#endif

#endif /* GAME_T523_PDIPIPE_H */
