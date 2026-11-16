/* T503_vol_thread.h - the volume reader thread (translation unit T503, unit u111),
 * 0x00240FA0..0x00241BF0.
 *
 * One request at a time, served by a thread (priority 4, 0x8000-byte stack)
 * that reads the file through the IOP pipe (T523) in 0x40000-byte pieces, or
 * inflates a packed one (GZIP::InflateLoader at priority 9), then hands the
 * result to every caller waiting for it.  T503 also owns vol_device_object
 * (its static constructor / destructor).  C++ only. */
#ifndef GAME_T503_VOL_THREAD_H
#define GAME_T503_VOL_THREAD_H

#include "game/game.h"

#ifdef __cplusplus

#include "engine/io/T510_vol_device.h"               /* VolDevice, held by value */

/* The gt3.vol device (mounted by vol_init). */
extern VolDevice vol_device_object;             /* 0x002F33B0 */

void  vol_thread_main(void *arg);               /* 0x002411E8 the reader thread */
/* Reads path into buf (NULL: memalign(64, size) here), waiting first for a
 * request in progress, then for this one.  unpack: inflate a packed (.gz)
 * file.  The buffer, NULL if cancelled and allocated here. */
void *file_read_blocking(const char *path, void *buf, int size, int unpack,
                         VolDevice *dev);       /* 0x002417C0 */
/* A sound bank (.ins "INST" / .es "ENGN") on the caller's thread: the header
 * into buf (NULL: memalign(64, size, or the header size if size is 0)), the
 * samples straight to SPU address spuAddr.  buf. */
void *file_read_sound_bank(const char *path, int spuAddr, void *buf, int size); /* 0x002418D0 */
/* Once: the disc sector cache (0x3180 bytes at 0x004A0CB8), mounts volName on
 * the disc (type 0), mc_manager_init, the reader thread. */
void  vol_init(const char *volName);            /* 0x00241A68 */
void  vol_shutdown(void);                       /* 0x00241B20 undoes vol_init */

/* 0x00241BA8 (.dtors 0x0028F278) / 0x00241BD0 (.ctors 0x0028F1A4): egcs's
 * static destructor and initialiser of vol_device_object (~VolDevice(2) /
 * VolDevice()), compiler-generated (T503 defines the object). */

#endif /* __cplusplus */

#endif /* GAME_T503_VOL_THREAD_H */
