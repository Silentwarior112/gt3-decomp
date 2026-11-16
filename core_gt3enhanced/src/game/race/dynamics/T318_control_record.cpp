/* T318 - AutomobileControlRecord: the record of a car's input for replays
 * and ghosts (declared in T318_control_record.h).
 *
 * Recorder codes the input frames (CarInput, T342) into a bit stream
 * (BitStreamBuffer, T536) behind an 8-byte header ([0] the frame count, [1]
 * the bytes used).  Each frame the steering is first predicted from the
 * last two (predict); a frame equal to the prediction costs nothing: the
 * stream holds runs, each a gamma code of (change mask + 1), the changed
 * fields as signed codes of their deltas, then the gamma code of the run
 * length.  stop() closes the stream with the last run and a mask of 0.
 *
 * Manager owns one Recorder.  Mode 0: the recorder covers a whole buffer of
 * the pool (allocBuffers).  Modes 1 / 2: the buffer is cut in 0x8000-byte
 * slots, each the head of a lap (a OneLapGhostInfo, T313: the car's snapshot)
 * and a 0x64F8-byte record; mode 1 moves on through four slots (nextSlot),
 * mode 2 keeps one. */
/* Retail T318's .rodata (0x0033DB88): <new>'s "bad_alloc" (nothing
 * references it: DATA_START), the vtables of Manager and Recorder, then the
 * type names "t9UnitArena1Ui64", Manager's, Recorder's (reverse definition
 * order: the original defined the two classes before it saw the race
 * course).  .data: g_002D8EE0 before the frame info; .sdata: g_003529B0,
 * g_003529B4, g_003529B8. */
#include <new>                              /* (unused: "bad_alloc") */
#include "game/race/dynamics/T318_control_record.h"
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>, after the own classes) */
#include "game/race/dynamics/control_record_internal.h"
// DATA_START 0x0033DB88

using namespace AutomobileControlRecord;

/* A slot of a buffer in modes 1 / 2. */
enum {
    SLOT_SHIFT       = 15,
    SLOT_SIZE        = 1 << SLOT_SHIFT,     /* 0x8000 */
    SLOT_HEAD_SIZE   = 0x1B00,              /* sizeof(OneLapGhostInfo) */
    SLOT_RECORD_SIZE = 0x64F8               /* SLOT_SIZE - SLOT_HEAD_SIZE: the record's capacity */
};

/* The bits of the change mask: the CarInput fields that differ from the
 * prediction (play reads them, record writes them, in this order). */
enum {
    CHANGED_FLAGS     = 0x100,              /* flags (u8) */
    CHANGED_DEVICE    = 0x080,              /* device (u8) */
    CHANGED_SHIFT     = 0x010,              /* shift (s8) */
    CHANGED_STEER     = 0x002,              /* steer */
    CHANGED_STEER_MAG = 0x001,              /* steerMag */
    CHANGED_THROTTLE  = 0x004,              /* throttle */
    CHANGED_BRAKE     = 0x008,              /* brake */
    CHANGED_THROTTLE2 = 0x040,              /* throttle2 */
    CHANGED_HANDBRAKE = 0x020               /* handbrake */
};

/* The 0x8000 bytes of a slot, copied by value (a 32-byte block loop). */
struct RecordSlot {
    u8 bytes[SLOT_SIZE];
};

/* The bytes of a saved record, rounded up to 4.  Signed: retail masks with
 * addiu -4 (an unsigned mask is lui 0xFFFF / ori 0xFFFC). */
static inline int align4(int size)
{
    return (size + 3) & ~3;
}

// FUNC 0x001F4360 AutomobileControlRecord::Recorder::operator new
/* The race object pool, 16-byte aligned. */
void *Recorder::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x001F4388 AutomobileControlRecord::Recorder::operator delete
void Recorder::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x001F43B0 AutomobileControlRecord::Recorder::Recorder
/* bits first (BitStreamBuffer::BitStreamBuffer; the landing pad runs
 * ~BitStreamBuffer).  No buffer yet, so reset() leaves it finished. */
Recorder::Recorder()
{
    /* implicit: BitStreamBuffer, ~BitStreamBuffer (member bits; the landing pad) */
    header = 0;
    capacity = 0;
    reset();
}

// FUNC 0x001F4420 AutomobileControlRecord::Recorder::~Recorder
/* Slot 0: destroys bits (~BitStreamBuffer, flags 2); the deleting form
 * frees 0x44 bytes. */
Recorder::~Recorder()
{
    /* implicit: ~BitStreamBuffer (member bits) */
}

// FUNC 0x001F4478 AutomobileControlRecord::Recorder::attach
/* The header at buf, the stream right after it (BitStreamBuffer's inline
 * setBuffer, Lupo 2's name). */
void Recorder::attach(u32 *buf, u32 cap)
{
    header = buf;
    capacity = cap;
    bits.setBuffer(buf + 2);
}

// FUNC 0x001F4490 AutomobileControlRecord::Recorder::detach
void Recorder::detach()
{
    header = 0;
    capacity = 0;
}

// FUNC 0x001F44A0 AutomobileControlRecord::Recorder::copyFrom
/* o's buffer (o's capacity bytes, not the bytes used) into this one. */
void Recorder::copyFrom(const Recorder *o)
{
    memcpy(header, o->header, o->capacity);
}

// FUNC 0x001F44C8 AutomobileControlRecord::Recorder::reset
void Recorder::reset()
{
    bits.rewind();
    written = 0;
    finished = 0;
    frames = 0;
    run = 0;
    prevSteer = 0;
    cur.clear();
    if (header == 0)
        finished = 1;
}

// FUNC 0x001F4518 AutomobileControlRecord::Recorder::predict
/* Unless the steering is near the centre (|s| < 16): s = 2 s - previous,
 * clamped to +-0x1000.  The previous value is kept either way. */
void Recorder::predict()
{
    int steer = cur.steer;
    int prev = prevSteer;
    int mag = steer < 0 ? -steer : steer;

    prevSteer = steer;
    if (mag >= 0x10) {
        steer = steer * 2 - prev;
        if (steer >= 0x1001)
            steer = 0x1000;
        else if (!(-0x1001 < steer))
            steer = -0x1000;
        cur.steer = steer;
    }
}

// FUNC 0x001F4570 AutomobileControlRecord::Recorder::readGamma
/* n one-bits, a zero, n bits r: r + 2^n (>= 1). */
int Recorder::readGamma()
{
    int n = 0;

    while (bits.read(1) != 0)
        n++;
    return (int)bits.read(n) + (1 << n);
}

/* The offsets of the signed codes: the first value of each length k, the
 * sums of the steps writeSigned subtracts (.data: not const in retail). */
int g_002D8EE0[8] = { 0, 2, 10, 42, 170, 682, 2730, 10922 };   /* 0x002D8EE0 */

// FUNC 0x001F45D0 AutomobileControlRecord::Recorder::readSigned
/* k (3 bits), the sign (1 bit), 2k+1 bits plus g_002D8EE0[k]. */
int Recorder::readSigned()
{
    int k = (int)bits.read(3);
    int neg = (int)bits.read(1);
    int value = (int)bits.read(k * 2 | 1) + g_002D8EE0[k];

    return neg ? -value : value;
}

// FUNC 0x001F4660 AutomobileControlRecord::Recorder::play
/* The next frame of the record into out (the last one again once the
 * record is over; an all-zero frame after its end).  The header pointer is
 * read before the test (retail loads it in the branch's delay slot). */
void Recorder::play(CarInput *out)
{
    u32 *h = header;

    if (finished == 0) {
        frames++;
        if (!(frames < h[0])) {
            cur.clear();
            finished = 1;
        } else {
            predict();
            if (run == 0) {
                int mask = readGamma() - 1;
                if (mask & CHANGED_FLAGS)
                    readDelta(&cur.flags);
                if (mask & CHANGED_DEVICE)
                    readDelta(&cur.device);
                if (mask & CHANGED_SHIFT)
                    readDelta(&cur.shift);
                if (mask & CHANGED_STEER)
                    readDelta(&cur.steer);
                if (mask & CHANGED_STEER_MAG)
                    readDelta(&cur.steerMag);
                if (mask & CHANGED_THROTTLE)
                    readDelta(&cur.throttle);
                if (mask & CHANGED_BRAKE)
                    readDelta(&cur.brake);
                if (mask & CHANGED_THROTTLE2)
                    readDelta(&cur.throttle2);
                if (mask & CHANGED_HANDBRAKE)
                    readDelta(&cur.handbrake);
                run = readGamma();
            }
            run = run - 1;
        }
    }
    *out = cur;
}

// FUNC 0x001F47D8 AutomobileControlRecord::Recorder::skip
/* Plays n frames into a local frame, from the start unless keep. */
void Recorder::skip(int n, int keep)
{
    CarInput frame;

    if (keep == 0)
        reset();
    if (n > 0) {
        do {
            play(&frame);
            n--;
        } while (n != 0);
    }
}

// FUNC 0x001F4830 AutomobileControlRecord::Recorder::writeGamma
/* The code readGamma reads back as n (n >= 1; 0 is written like 1).
 * The 1 that is shifted is a variable: retail loads it at the join, before
 * the loop's entry test, and compares rest with the shifted value; with a
 * constant egcs folds rest >= 1 << nbits into (rest >> nbits) != 0 (89%),
 * and a do-while with its own step gave 93.3 / 96.3%. */
void Recorder::writeGamma(u32 n)
{
    u64 rest = n;
    u64 one;
    int nbits;

    if (n != 0)
        rest = (u32)(n - 1);
    one = 1;
    nbits = 0;
    while (rest >= (one << nbits)) {
        rest -= one << nbits;
        nbits++;
    }
    bits.write((u64)-1, nbits);
    bits.write(0, 1);
    bits.write(rest, nbits);
}

// FUNC 0x001F48D8 AutomobileControlRecord::Recorder::writeSigned
/* The code readSigned reads back as v: g_002D8EE0 = {0, 2, 10, 42, 170,
 * 682, 2730, 10922} are the sums of the steps subtracted here.  The loop
 * as writeGamma's (a do-while with its own step: 87.8 / 91.0%). */
void Recorder::writeSigned(int v)
{
    int neg = 0;
    int one;
    int nbits;

    if (v < 0) {
        v = -v;
        neg = 1;
    }
    one = 1;
    nbits = 1;
    while (v >= (one << nbits)) {
        v -= one << nbits;
        nbits += 2;
    }
    bits.write(nbits >> 1, 3);
    bits.write(neg, 1);
    bits.write(v, nbits);
}

// FUNC 0x001F4998 AutomobileControlRecord::Recorder::record
/* Adds frame `in': after the steering prediction, a frame with changes
 * (and the first frame) ends the current run and writes the change mask
 * and the deltas; less than 0x40 bytes of room left stops the record. */
void Recorder::record(const CarInput *in)
{
    if (finished == 0) {
        int mask = 0;

        predict();
        written = 1;
        if (cur.flags != in->flags)
            mask |= CHANGED_FLAGS;
        if (cur.device != in->device)
            mask |= CHANGED_DEVICE;
        if (cur.shift != in->shift)
            mask |= CHANGED_SHIFT;
        if (cur.steer != in->steer)
            mask |= CHANGED_STEER;
        if (cur.steerMag != in->steerMag)
            mask |= CHANGED_STEER_MAG;
        if (cur.throttle != in->throttle)
            mask |= CHANGED_THROTTLE;
        if (cur.brake != in->brake)
            mask |= CHANGED_BRAKE;
        if (cur.throttle2 != in->throttle2)
            mask |= CHANGED_THROTTLE2;
        if (cur.handbrake != in->handbrake)
            mask |= CHANGED_HANDBRAKE;
        frames++;
        if (frames == 1 || mask != 0) {
            if (run != 0)
                writeGamma(run);
            writeGamma(mask + 1);
            if (mask & CHANGED_FLAGS)
                writeDelta(&cur.flags, &in->flags);
            if (mask & CHANGED_DEVICE)
                writeDelta(&cur.device, &in->device);
            if (mask & CHANGED_SHIFT)
                writeDelta(&cur.shift, &in->shift);
            if (mask & CHANGED_STEER)
                writeDelta(&cur.steer, &in->steer);
            if (mask & CHANGED_STEER_MAG)
                writeDelta(&cur.steerMag, &in->steerMag);
            if (mask & CHANGED_THROTTLE)
                writeDelta(&cur.throttle, &in->throttle);
            if (mask & CHANGED_BRAKE)
                writeDelta(&cur.brake, &in->brake);
            if (mask & CHANGED_THROTTLE2)
                writeDelta(&cur.throttle2, &in->throttle2);
            if (mask & CHANGED_HANDBRAKE)
                writeDelta(&cur.handbrake, &in->handbrake);
            if (!((u32)(bits.size() + 0x40) < capacity))
                stop();
            run = 0;
        }
        run = run + 1;
    }
    cur = *in;
}

// FUNC 0x001F4C00 AutomobileControlRecord::Recorder::stop
/* Closes the stream (the last run, mask 0) and fills the header. */
void Recorder::stop()
{
    if (finished == 0 && written != 0) {
        u32 *h;

        writeGamma(run);
        writeGamma(1);
        h = header;
        h[0] = frames;
        h[1] = bits.size() + 8;
    }
    finished = 1;
}

// FUNC 0x001F4C78 AutomobileControlRecord::Recorder::frameCount
/* The frames of the record in the buffer (0 without one). */
u32 Recorder::frameCount()
{
    if (header != 0)
        return header[0];
    return 0;
}

// FUNC 0x001F4C98 AutomobileControlRecord::Recorder::load
/* Copies a saved record (src[1] bytes) into the buffer; returns the end of
 * it, rounded up to 4 bytes (src itself when there is no buffer). */
const u32 *Recorder::load(const u32 *src)
{
    if (header == 0)
        return src;
    memcpy(header, src, src[1]);
    return (const u32 *)((const u8 *)src + align4(src[1]));
}

// FUNC 0x001F4CE8 AutomobileControlRecord::Recorder::save
/* Appends r's record at buf + buf[0] (buf[0]: the byte offset, advanced
 * by the size rounded up to 4). */
void Recorder::save(Recorder *r, int *buf)
{
    if (r != 0) {
        u32 *h = r->header;
        if (h != 0) {
            memcpy((u8 *)buf + *buf, h, h[1]);
            *buf += align4(h[1]);
        }
    }
}

// FUNC 0x001F4D50 AutomobileControlRecord::Recorder::saveSize
int Recorder::saveSize(Recorder *r)
{
    if (r == 0 || r->header == 0)
        return 0;
    return align4(r->header[1]);
}

/* The buffer pool (.sdata, zero until allocBuffers; explicit initialisers:
 * written here in this order, as retail's). */
u8 *g_003529B0 = 0;                         /* 0x003529B0 the record buffers */
int g_003529B4 = 0;                         /* 0x003529B4 the size of one buffer */
int g_003529B8 = 0;                         /* 0x003529B8 the number of buffers */

// FUNC 0x001F4D80 AutomobileControlRecord::allocBuffers
/* race_begin (F_00102B58): `count' buffers of `size' bytes in one block. */
void AutomobileControlRecord::allocBuffers(int size, int count)
{
    u8 *block = (u8 *)malloc(size * count);

    g_003529B4 = size;
    g_003529B8 = count;
    g_003529B0 = block;
}

// FUNC 0x001F4DC0 AutomobileControlRecord::freeBuffers
void AutomobileControlRecord::freeBuffers(void)
{
    free(g_003529B0);
    g_003529B0 = 0;
    g_003529B4 = 0;
    g_003529B8 = 0;
}

// FUNC 0x001F4DF0 AutomobileControlRecord::getBuffer
u8 *AutomobileControlRecord::getBuffer(int i)
{
    return g_003529B0 + g_003529B4 * i;
}

// FUNC 0x001F4E08 AutomobileControlRecord::getBufferSlot
u8 *AutomobileControlRecord::getBufferSlot(int i, int slot)
{
    return getBuffer(i) + (slot << SLOT_SHIFT);
}

// FUNC 0x001F4E38 AutomobileControlRecord::copyBuffer
void AutomobileControlRecord::copyBuffer(int dst, int src)
{
    u8 *to = getBuffer(dst);

    memcpy(to, getBuffer(src), g_003529B4);
}

// FUNC 0x001F4E80 AutomobileControlRecord::Manager::operator new
/* The race object pool, 16-byte aligned. */
void *Manager::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x001F4EA8 AutomobileControlRecord::Manager::operator delete
void Manager::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x001F4ED0 AutomobileControlRecord::Manager::Manager
/* rec first (Recorder::Recorder); base is not set here. */
Manager::Manager()
{
    /* implicit: Recorder (member rec) */
    playback = 0;
    mode = MODE_BUFFER;
    slot = 0;
    index = 0;
    count = 0;
}

// FUNC 0x001F4F20 AutomobileControlRecord::Manager::~Manager
/* Slot 0: destroys rec (~Recorder, flags 2); the deleting form frees 0x60
 * bytes. */
Manager::~Manager()
{
    /* implicit: ~Recorder (member rec) */
}

// FUNC 0x001F4F78 AutomobileControlRecord::Manager::attachBuffer
void Manager::attachBuffer(int i)
{
    setBuffer(getBuffer(i));
}

// FUNC 0x001F4FB0 AutomobileControlRecord::Manager::setBuffer
/* Mode 0: the whole buffer (less the header) is the record; else slot 0. */
void Manager::setBuffer(u8 *buf)
{
    base = buf;
    if (mode == MODE_BUFFER) {
        rec.attach((u32 *)buf, g_003529B4 - 8);
        return;
    }
    selectSlot(0);
}

// FUNC 0x001F4FF0 AutomobileControlRecord::Manager::selectSlot
/* Slot n: its head, then the record after it. */
void Manager::selectSlot(int n)
{
    slot = base + (n << SLOT_SHIFT);
    rec.attach((u32 *)(slot + SLOT_HEAD_SIZE), SLOT_RECORD_SIZE);
}

// FUNC 0x001F5028 AutomobileControlRecord::Manager::nextSlot
/* No callers.  Mode 1: closes the current record and moves to the next
 * slot (at most 3).  When recording and the count reaches 4, three slots
 * are moved down by one; the moves start at the current slot (+0x50), not
 * at base, as in the code. */
void Manager::nextSlot()
{
    if (mode == MODE_SLOTS) {
        int next;

        rec.stop();
        if (playback == 0) {
            int c = count;
            if (++c == 4) {
                int i;
                for (i = 0; i < 3; i++)
                    ((RecordSlot *)slot)[i] = ((RecordSlot *)slot)[i + 1];
                c = 4;
            }
            count = c;
        }
        next = index + 1;
        if (next == 4)
            next = 3;
        index = next;
        selectSlot(next);
    }
}

// FUNC 0x001F5188 AutomobileControlRecord::Manager::getSlot
/* The current slot's head (a OneLapGhostInfo), NULL in mode 0. */
u8 *Manager::getSlot()
{
    if (mode == MODE_BUFFER)
        return 0;
    return slot;
}

// FUNC 0x001F51A8 AutomobileControlRecord::Manager::copyFrom
void Manager::copyFrom(Manager *o)
{
    if (mode == MODE_BUFFER && o->mode == MODE_BUFFER)
        rec.copyFrom(&o->rec);
}

// FUNC 0x001F51E0 AutomobileControlRecord::Manager::copySlotFrom
void Manager::copySlotFrom(Manager *o)
{
    if (mode != MODE_BUFFER && o->mode != MODE_BUFFER)
        *(RecordSlot *)slot = *(RecordSlot *)o->slot;
}

// FUNC 0x001F52B0 AutomobileControlRecord::Manager::reset
void Manager::reset()
{
    if (mode == MODE_SLOTS) {
        index = 0;
        count = 0;
        selectSlot(0);
    }
    rec.reset();
}

// FUNC 0x001F52F8 AutomobileControlRecord::Manager::load
/* A saved record: [0] the number of slots (0: a mode 0 record follows),
 * then each slot (its head and a record).  multi != 0 loads them all
 * (mode 1), else only the first (mode 2).  count is not changed. */
const u32 *Manager::load(const u32 *src, int multi)
{
    int slots = (int)*src++;
    int i;

    if (slots == 0) {
        mode = MODE_BUFFER;
        return rec.load(src);
    }
    if (multi != 0) {
        mode = MODE_SLOTS;
        for (i = 0; i < slots; i++) {
            selectSlot(i);
            src = loadSlot(src);
        }
    } else {
        mode = MODE_SLOT;
        selectSlot(0);
        src = loadSlot(src);
    }
    return src;
}

// FUNC 0x001F53D0 AutomobileControlRecord::Manager::loadSlot
const u32 *Manager::loadSlot(const u32 *src)
{
    u32 size = SLOT_HEAD_SIZE;

    memcpy(slot, src, size);
    return rec.load((const u32 *)((const u8 *)src + size));
}

// FUNC 0x001F5418 AutomobileControlRecord::Manager::save
/* Appends [the number of slots] and the slots (or the mode 0 record) at
 * buf + buf[0]; the format load reads.  multi (1 from every caller) is not
 * read. */
void Manager::save(int *buf, int multi)
{
    int offset = *buf;
    int *slots = (int *)((u8 *)buf + offset);
    int i;

    *buf = offset + 4;
    switch (mode) {
    case MODE_BUFFER:
        *slots = 0;
        Recorder::save(&rec, buf);
        break;
    case MODE_SLOTS:
        *slots = count;
        for (i = 0; i < count; i++) {
            selectSlot(i);
            saveSlot(buf);
        }
        break;
    case MODE_SLOT:
        *slots = 1;
        saveSlot(buf);
        break;
    }
}

// FUNC 0x001F5508 AutomobileControlRecord::Manager::saveSlot
void Manager::saveSlot(int *buf)
{
    u32 size = SLOT_HEAD_SIZE;

    memcpy((u8 *)buf + *buf, slot, size);
    *buf += size;
    Recorder::save(&rec, buf);
}

// FUNC 0x001F5560 AutomobileControlRecord::Manager::saveSize
/* The bytes save() appends; multi is not read. */
int Manager::saveSize(int multi)
{
    int size = 4;
    int i;

    switch (mode) {
    case MODE_BUFFER:
        size = Recorder::saveSize(&rec) + 4;
        break;
    case MODE_SLOTS:
        for (i = 0; i < count; i++) {
            selectSlot(i);
            size += SLOT_HEAD_SIZE;
            size += Recorder::saveSize(&rec);
        }
        break;
    case MODE_SLOT:
        size = Recorder::saveSize(&rec) + SLOT_HEAD_SIZE + 4;
        break;
    }
    return size;
}

// FUNC 0x001F5638 AutomobileControlRecord::Manager::hasSlots
/* A saved record (what load reads) holds lap slots: its slot count is not
 * 0.  play_saved_replay (F_00100838) asks. */
int Manager::hasSlots(const u32 *data)
{
    return data[0] != 0;
}

// FUNC 0x001F5648 AutomobileControlRecord::Recorder::readDelta
/* Lupo 2: read_sub<unsigned char>. */
void Recorder::readDelta(u8 *v)
{
    *v += readSigned();
}

// FUNC 0x001F5678 AutomobileControlRecord::Recorder::readDelta
/* Lupo 2: read_sub<signed char>. */
void Recorder::readDelta(s8 *v)
{
    *v += readSigned();
}

// FUNC 0x001F56A8 AutomobileControlRecord::Recorder::readDelta
/* Lupo 2: read_sub<short>. */
void Recorder::readDelta(s16 *v)
{
    *v += readSigned();
}

// FUNC 0x001F56D8 AutomobileControlRecord::Recorder::writeDelta
/* Lupo 2: write_sub<unsigned char>. */
void Recorder::writeDelta(u8 *v, const u8 *n)
{
    writeSigned(*n - *v);
    *v = *n;
}

// FUNC 0x001F5720 AutomobileControlRecord::Recorder::writeDelta
/* Lupo 2: write_sub<signed char>. */
void Recorder::writeDelta(s8 *v, const s8 *n)
{
    writeSigned(*n - *v);
    *v = *n;
}

// FUNC 0x001F5768 AutomobileControlRecord::Recorder::writeDelta
/* Lupo 2: write_sub<short>. */
void Recorder::writeDelta(s16 *v, const s16 *n)
{
    writeSigned(*n - *v);
    *v = *n;
}

/* Manager's inline accessors, emitted out of line (no callers): Lupo 2's
 * getBufferSize .. setReplay in reverse order, with the same sizes. */

// FUNC 0x001F57B0 AutomobileControlRecord::Manager::getBufferSize
/* The size of one buffer of the pool (Lupo 2 Manager::getBufferSize). */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: return g_003529B4; */

// FUNC 0x001F57B8 AutomobileControlRecord::Manager::frameCount
/* Lupo 2 getFrameMax. */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: return rec.frameCount(); */
/* implicit: frameCount (the copy's call) */

// FUNC 0x001F57D8 AutomobileControlRecord::Manager::getFrames
/* Lupo 2 getFrameNow: rec.getFrames() inlined (+0x38 = rec +0x30). */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: return rec.frames; */

// FUNC 0x001F57E0 AutomobileControlRecord::Manager::isFinished
/* Lupo 2 isDone: rec.isFinished() inlined (+0x30 = rec +0x28). */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: return rec.finished; */

// FUNC 0x001F57E8 AutomobileControlRecord::Manager::stop
/* Lupo 2 finish. */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: rec.stop(); */
/* implicit: stop (the copy's call) */

// FUNC 0x001F5808 AutomobileControlRecord::Manager::record
/* Lupo 2 write. */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: rec.record(in); */
/* implicit: record (the copy's call) */

// FUNC 0x001F5828 AutomobileControlRecord::Manager::skip
/* Lupo 2 seek(int, bool). */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: rec.skip(n, keep); */
/* implicit: skip (the copy's call) */

// FUNC 0x001F5848 AutomobileControlRecord::Manager::play
/* Lupo 2 read. */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: rec.play(out); */
/* implicit: play (the copy's call) */

// FUNC 0x001F5868 AutomobileControlRecord::Manager::getMode
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: return mode; */

// FUNC 0x001F5870 AutomobileControlRecord::Manager::setMode
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: mode = m; */

// FUNC 0x001F5878 AutomobileControlRecord::Manager::getPlayback
/* Lupo 2 getReplay. */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: return playback; */

// FUNC 0x001F5880 AutomobileControlRecord::Manager::setPlayback
/* Lupo 2 setReplay(bool). */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: playback = v; */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001F5888 __tfQ223AutomobileControlRecord7Manager
/* Builds the __user_type_info __tiQ223AutomobileControlRecord7Manager
 * (0x00353418) on the first call (__rtti_user: no bases). */
extern "C" const type_info &__tfQ223AutomobileControlRecord7Manager(void)
{
    void *node = __tiQ223AutomobileControlRecord7Manager;

    if (__tiQ223AutomobileControlRecord7Manager[0] == 0)
        __rtti_user(node, "Q223AutomobileControlRecord7Manager");
    return *(const type_info *)node;
}
#endif

/* Recorder's inline accessors, emitted out of line (no callers). */

// FUNC 0x001F58C8 AutomobileControlRecord::Recorder::getFrames
/* Lupo 2 getFrameNow. */
/* inline in game/race/dynamics/T318_control_record.h (no callers); egcs emits this copy
 * with the vtable: return frames; */

// FUNC 0x001F58D0 AutomobileControlRecord::Recorder::isFinished
/* Lupo 2 isDone. */
/* inline in game/race/dynamics/T318_control_record.h (u046 inlines it); egcs emits this
 * copy with the vtable: return finished; */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001F58D8 __tfQ223AutomobileControlRecord8Recorder
/* Builds the __user_type_info __tiQ223AutomobileControlRecord8Recorder
 * (0x00353420) on the first call (__rtti_user: no bases). */
extern "C" const type_info &__tfQ223AutomobileControlRecord8Recorder(void)
{
    void *node = __tiQ223AutomobileControlRecord8Recorder;

    if (__tiQ223AutomobileControlRecord8Recorder[0] == 0)
        __rtti_user(node, "Q223AutomobileControlRecord8Recorder");
    return *(const type_info *)node;
}
#endif
