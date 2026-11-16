/* T318_control_record.h - AutomobileControlRecord, the record of a car's
 * control input for replays and ghosts (translation unit T318, unit u085),
 * 0x001F4360..0x001F5918.
 *
 * Recorder codes the frames of a car's input (CarInput, T342: u085 called it
 * ControlFrame) into a bit stream (BitStreamBuffer, T536) behind an 8-byte
 * header ([0] the frame count, [1] the bytes used).  Each frame the steering
 * is first predicted from the last two (predict); a frame equal to the
 * prediction costs nothing: the stream holds runs, each a gamma code of
 * (change mask + 1), the changed fields as signed codes of their deltas,
 * then the gamma code of the run length.  stop() closes the stream with the
 * last run and a mask of 0.
 *
 * Manager owns one Recorder.  Mode 0: the recorder covers a whole buffer of
 * the pool (allocBuffers).  Modes 1 / 2: the buffer is cut in 0x8000-byte
 * slots of 0x1B00 bytes of car snapshot (OneLapGhostInfo, T313) plus a
 * 0x64F8-byte record; mode 1 moves on through four slots (nextSlot).  A
 * RaceInput (T413) holds one Manager (its replay record) and points at two
 * more (the best lap and the lap being driven).
 *
 * AutomobileControlRecord is a namespace, as in the owner (u047 / u048 / u049
 * / u084 declared a class around the two classes; the mangled names
 * Q223AutomobileControlRecord7Manager do not tell).  Lupo 2 has the same two
 * classes; its names of the inline accessors (the same sizes, in reverse
 * order) are noted where the owner's names are kept.  Lupo 2 also shows
 * readDelta / writeDelta as instances of member templates read_sub<T> /
 * write_sub<T> (unsigned char, signed char, short), emitted at the end of
 * the unit. */
#ifndef GAME_T318_CONTROL_RECORD_H
#define GAME_T318_CONTROL_RECORD_H

#include "game/game.h"
#include "engine/pdistd/T536_bit_stream_buffer.h"    /* BitStreamBuffer (held by value) */
#include "game/car/physics/T342_car_input.h"            /* CarInput (held by value) */
#include <stddef.h>                         /* size_t */

class type_info;

/* The typeinfo functions, declared before the classes (see T305): each
 * builds its __user_type_info (0x00353418 / 0x00353420) on the first
 * call.  GCC 14 build only: egcs generates them, and an extern "C"
 * declaration is the one egcs uses, so it would not emit them (u085
 * matching: both are T318's own copies, after the inline accessors). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tfQ223AutomobileControlRecord7Manager(void);    /* 0x001F5888 */
const type_info &__tfQ223AutomobileControlRecord8Recorder(void);   /* 0x001F58D8 */
}
#endif

extern int g_003529B4;                      /* 0x003529B4 gp-0x503C the size of one buffer */
                                            /* (declared before the classes: the inline
                                               Manager::getBufferSize reads it) */

namespace AutomobileControlRecord {

/* The buffer pool: g_003529B0 (count buffers of size bytes, one block). */
void  allocBuffers(int size, int count);    /* 0x001F4D80 (F_00102B58) */
void  freeBuffers(void);                    /* 0x001F4DC0 */
u8   *getBuffer(int i);                     /* 0x001F4DF0 */
u8   *getBufferSlot(int i, int slot);       /* 0x001F4E08 buffer i + slot x 0x8000 */
void  copyBuffer(int dst, int src);         /* 0x001F4E38 */

/* vtable _vt$Q223AutomobileControlRecord8Recorder 0x0033DBA8 (1 slot: the
 * destructor), vptr +0x40 (after its data; bits is a member, not a base:
 * no RTTI base).  typeinfo 0x001F58D8 (__user_type_info 0x00353420).  Size
 * 0x44 (Manager +0x08..+0x4C; operator delete frees 0x44). */
class Recorder {
public:
    static void *operator new(size_t size);             /* 0x001F4360 the race object pool g_002B3BF0, 16-aligned */
    static void  operator delete(void *p, size_t size); /* 0x001F4388 */
    /* No buffer yet (header / capacity 0), then reset(). */
    Recorder();                                         /* 0x001F43B0 */
    virtual ~Recorder();                                /* [0] +0x0C 0x001F4420 */

    void  attach(u32 *buf, u32 cap);        /* 0x001F4478 header = buf, the stream from buf + 8 */
    void  detach();                         /* 0x001F4490 */
    void  copyFrom(const Recorder *o);      /* 0x001F44A0 */
    void  reset();                          /* 0x001F44C8 */
    /* Unless the steering is near the centre (|s| < 16): s = 2 s - previous,
     * clamped to +-0x1000.  The previous value is kept either way. */
    void  predict();                        /* 0x001F4518 */
    int   readGamma();                      /* 0x001F4570 n one-bits, a zero, n bits r: r + 2^n */
    int   readSigned();                     /* 0x001F45D0 k (3 bits), the sign, 2k+1 bits + g_002D8EE0[k] */
    /* The next frame into out (the last one again once the record is over;
     * an all-zero frame after its end). */
    void  play(CarInput *out);              /* 0x001F4660 */
    void  skip(int n, int keep);            /* 0x001F47D8 n frames into a local frame, from the start unless keep */
    void  writeGamma(u32 n);                /* 0x001F4830 */
    void  writeSigned(int v);               /* 0x001F48D8 */
    /* Adds frame in: after the steering prediction, a frame with changes
     * (and the first frame) ends the current run and writes the change mask
     * and the deltas; less than 0x40 bytes of room stops the record. */
    void  record(const CarInput *in);       /* 0x001F4998 */
    void  stop();                           /* 0x001F4C00 the last run, mask 0, the header */
    u32   frameCount();                     /* 0x001F4C78 */
    /* Copies a saved record (src[1] bytes) into the buffer; returns its end,
     * rounded up to 4 bytes (src itself without a buffer). */
    const u32 *load(const u32 *src);        /* 0x001F4C98 */
    /* Appends r's record at buf + buf[0] (buf[0] advanced by the size
     * rounded up to 4).  r may be NULL. */
    static void save(Recorder *r, int *buf); /* 0x001F4CE8 */
    static int  saveSize(Recorder *r);      /* 0x001F4D50 r may be NULL */
    /* v += readSigned() (Lupo 2: read_sub<T>). */
    void  readDelta(u8 *v);                 /* 0x001F5648 */
    void  readDelta(s8 *v);                 /* 0x001F5678 */
    void  readDelta(s16 *v);                /* 0x001F56A8 */
    /* writeSigned(*n - *v), *v = *n (Lupo 2: write_sub<T>). */
    void  writeDelta(u8 *v, const u8 *n);   /* 0x001F56D8 */
    void  writeDelta(s8 *v, const s8 *n);   /* 0x001F5720 */
    void  writeDelta(s16 *v, const s16 *n); /* 0x001F5768 */
    /* Inline (u085 matching; u046's RaceSinglePlayer::virtual_28 reads
     * input.rec.isFinished()): egcs emits the copies 0x001F58C8 / 0x001F58D0
     * after Manager's, in reverse order of definition. */
    int   isFinished() { return finished; } /* 0x001F58D0 finished (Lupo 2 isDone) */
    u32   getFrames() { return frames; }    /* 0x001F58C8 frames (Lupo 2 getFrameNow) */

    BitStreamBuffer bits;                   /* 0x00 */
    int             prevSteer;              /* 0x14 */
    CarInput        cur;                    /* 0x18 the frame of the current run (u085 ControlFrame) */
    int             finished;               /* 0x28 played to its end / full (u047 f28) */
    int             written;                /* 0x2C */
    int             frames;                 /* 0x30 the frames recorded / played (36000 = 10 min); signed */
                                            /* (slt: checkRunResult, T308; FreeRun slot 17, T313) */
    int             run;                    /* 0x34 frames left of the current run */
    u32            *header;                 /* 0x38 [0] frames, [1] bytes */
    u32             capacity;               /* 0x3C */
    /*  vptr                                   0x40 */
};

/* vtable _vt$Q223AutomobileControlRecord7Manager 0x0033DB98 (1 slot: the
 * destructor), vptr +0x5C (after its data).  typeinfo 0x001F5888
 * (__user_type_info 0x00353418).  Size 0x60 (RaceInput +0xC4..+0x124, T413;
 * operator delete frees 0x60). */
class Manager {
public:
    /* Lupo 2: Manager::MODE (setMode(MODE)); here setMode takes an int (its
     * mangled name) and only the member has the enum type.  u085 matching:
     * save() loads mode ahead of the int store *buf, which an int member
     * could not pass (an enum has its own alias set). */
    enum MODE {
        MODE_BUFFER = 0,                    /* the recorder covers the whole buffer */
        MODE_SLOTS  = 1,                    /* four lap slots (nextSlot) */
        MODE_SLOT   = 2                     /* one lap slot */
    };

    static void *operator new(size_t size);             /* 0x001F4E80 the race object pool, 16-aligned */
    static void  operator delete(void *p, size_t size); /* 0x001F4EA8 */
    Manager();                              /* 0x001F4ED0 (base is not set) */
    virtual ~Manager();                     /* [0] +0x0C 0x001F4F20 */

    void  attachBuffer(int i);              /* 0x001F4F78 base = getBuffer(i) */
    void  setBuffer(u8 *buf);               /* 0x001F4FB0 */
    void  selectSlot(int n);                /* 0x001F4FF0 slot = base + n x 0x8000 */
    /* No callers.  Mode 1: closes the record and moves to the next slot (at
     * most 3); recording at count 4, three slots move down by one (from the
     * current slot, not from base, as in the code). */
    void  nextSlot();                       /* 0x001F5028 */
    u8   *getSlot();                        /* 0x001F5188 slot when mode != 0, else NULL (the OneLapGhostInfo head) */
    void  copyFrom(Manager *o);             /* 0x001F51A8 (both in mode 0) o's record */
    void  copySlotFrom(Manager *o);         /* 0x001F51E0 */
    void  reset();                          /* 0x001F52B0 rewinds */
    /* A saved record: [0] the number of slots (0: a mode 0 record follows),
     * then each slot (0x1B00 bytes and a record); multi != 0 loads them all
     * (mode 1), else the first (mode 2).  count is not changed. */
    const u32 *load(const u32 *src, int multi); /* 0x001F52F8 */
    const u32 *loadSlot(const u32 *src);    /* 0x001F53D0 */
    /* Appends the number of slots and the slots (or the mode 0 record) at
     * buf + buf[0], the format load reads.  multi: every caller passes 1
     * (as for load); save / saveSize do not read it (u046 / u047). */
    void  save(int *buf, int multi);        /* 0x001F5418 */
    void  saveSlot(int *buf);               /* 0x001F5508 */
    int   saveSize(int multi);              /* 0x001F5560 */
    static int hasSlots(const u32 *data);   /* 0x001F5638 a saved record's slot count != 0 */
    /* getBufferSize .. setPlayback (0x001F57B0..0x001F5880) are inline (u047
     * matching): no copy has a caller; RaceSolitaire's constructor (T139)
     * sets its lap records' mode through setMode.  u085's T318 keeps
     * comment-only FUNC blocks.  Defined in reverse address order: egcs
     * emits the copies in reverse order of definition (u085 matching). */
    void  setPlayback(int v) { playback = v; }  /* 0x001F5880 (Lupo 2 setReplay(bool)) */
    int   getPlayback() { return playback; }    /* 0x001F5878 (Lupo 2 getReplay) */
    void  setMode(int m) { mode = (MODE)m; }    /* 0x001F5870 (Lupo 2 setMode(MODE)) */
    int   getMode() { return mode; }        /* 0x001F5868 */
    void  play(CarInput *out) { rec.play(out); }  /* 0x001F5848 rec.play(out) (Lupo 2 read) */
    void  skip(int n, int keep) { rec.skip(n, keep); }    /* 0x001F5828 rec.skip(n, keep) (Lupo 2 seek(int, bool)) */
    void  record(const CarInput *in) { rec.record(in); }  /* 0x001F5808 rec.record(in) (Lupo 2 write) */
    void  stop() { rec.stop(); }            /* 0x001F57E8 rec.stop() (Lupo 2 finish) */
    int   isFinished() { return rec.isFinished(); }   /* 0x001F57E0 rec.finished (Lupo 2 isDone; u046 "full") */
    u32   getFrames() { return rec.getFrames(); }     /* 0x001F57D8 rec.frames (Lupo 2 getFrameNow) */
    u32   frameCount() { return rec.frameCount(); }   /* 0x001F57B8 rec.frameCount() (Lupo 2 getFrameMax) */
    /* No callers; g_003529B4 (u085: the namespace function
     * AutomobileControlRecord::getBufferSize; Lupo 2 Manager::getBufferSize,
     * among Manager's accessors here too). */
    int   getBufferSize() { return g_003529B4; }    /* 0x001F57B0 */

    int      playback;                      /* 0x00 non-zero: the record is played (RaceInput::setReplayMode) */
    MODE     mode;                          /* 0x04 MODE_* (4 bytes in both builds) */
    Recorder rec;                           /* 0x08 */
    u8      *base;                          /* 0x4C the buffer (u047 / u084 buffer) */
    u8      *slot;                          /* 0x50 base + index x 0x8000 (u047 / u084 cur) */
    int      index;                         /* 0x54 */
    int      count;                         /* 0x58 */
    /*  vptr                                   0x5C */
};

} /* namespace AutomobileControlRecord */

EGCS_SIZE(AutomobileControlRecord::Recorder, 0x44);
EGCS_SIZE(AutomobileControlRecord::Manager, 0x60);

/* u018 / u078's CarDynamicsCC (the plan put it in T055): the replay recorder
 * a RaceInput holds (rec.rec, RaceInput +0xCC); its +0x30 is frames, u018's
 * F_001F4C78 is frameCount. */
typedef AutomobileControlRecord::Recorder CarDynamicsCC;

extern u8 *g_003529B0;                      /* 0x003529B0 gp-0x5040 the record buffers (allocBuffers) */
extern int g_003529B8;                      /* 0x003529B8 gp-0x5038 the number of buffers */
extern int g_002D8EE0[8];                   /* 0x002D8EE0 the offsets of the signed codes (readSigned; */
                                            /* .data in retail: not const) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
extern u32 __tiQ223AutomobileControlRecord7Manager[2];  /* 0x00353418 gp-0x45D8 (u085 g_00353418) */
extern u32 __tiQ223AutomobileControlRecord8Recorder[2]; /* 0x00353420 gp-0x45D0 (u085 g_00353420) */
}
#endif

#endif /* GAME_T318_CONTROL_RECORD_H */
