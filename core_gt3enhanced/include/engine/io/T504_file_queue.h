/* T504_file_queue.h - the queued (callback) loads (translation unit T504, unit u111),
 * 0x00241BF0..0x00241E08.
 *
 * file_req_alloc queues a load in a static array of 0x20-byte entries
 * (0x004A3E38[count++]), linked in a list sorted by first sector so the disc
 * is read in order; file_req_run (from RaceBase) loads them all and calls each
 * one's callback with the data. */
#ifndef GAME_T504_FILE_QUEUE_H
#define GAME_T504_FILE_QUEUE_H

#include "game/game.h"

/* A load's callback: done(arg, the data) (data NULL for an unknown kind). */
typedef void (*FileDoneFn)(void *arg, void *data);

/* Queues path (resolved now, retrying, for its sector) into buf / size.
 * kind 0: file_read_blocking (unpacked) from vol_device_object; 1: a sound
 * bank (file_read_sound_bank; arg = the GTSOUNDINSTRUMENT, whose nextSpuAddr
 * is the SPU address). */
void file_req_alloc(const char *path, void *buf, int size, FileDoneFn done,
                    void *arg, int kind);       /* 0x00241BF0 */
void file_req_run(void);                        /* 0x00241D30 loads every queued entry in order */

#endif /* GAME_T504_FILE_QUEUE_H */
