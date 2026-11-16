/* T448 - TStripBuilder: triangle strips into VIF / GIF packets.
 * 0x002212D8..0x00221E08.
 *
 * Vertices (any of position, normal, uv, colour; a format change flushes
 * the block) collect in 64-entry arrays; endStrip records a strip of the
 * block (>= 3 vertices).  flush writes the block at `cur':
 *   +0x00  next block (= the end of this one), qwc, the shader (first block
 *          after setShader only), -
 *   +0x10  UNPACK V4-32 1 qword to 0x0C0: the GIF tag (NLOOP = vertices,
 *          EOP; PRE, PRIM; NREG 3, REGS ST RGBAQ XYZF2)
 *   +0x24  UNPACK S-8 (strips + 1) bytes to 0x040: the strip count, then
 *          3 * (vertices - 2) per strip; padded to a word
 *          UNPACK V3-32 to 0x000 the positions, V3-32 to 0x080 the normals,
 *          V2-32 (masked) to 0x040 the uvs, V4-8 to 0x080 the colours
 *          (those present), padded with 0 words to a quadword.
 * A full block (64 vertices) is flushed and the strip goes on in the next
 * one with its last two vertices again.  finish() writes the packet header
 * (a G001 VifModel: the blocks' addresses / qwc / shader) and
 * starts a new packet behind it.  Without a buffer (beginMeasure) only the
 * size is counted (endMeasure: the bytes a packet will need). */
#include "game/car/model/car_model_internal.h"

/* A block holds at most this many vertices. */
#define STRIP_BLOCK_VERTICES 64

/* VIF UNPACK codes, with FLG and USN set (the address is relative to the
 * VU1 double buffer, the data unsigned). */
enum {
    VIF_S_8   = 0x62,
    VIF_V2_32 = 0x64,
    VIF_V3_32 = 0x68,
    VIF_V4_32 = 0x6C,
    VIF_V4_8  = 0x6E,
    VIF_MASK  = 0x10                        /* the write mask applies */
};

/* The GIF tag (as its four words). */
#define GIF_PRIM_SHIFT   15                 /* word 2 (bits 47..57) */
#define GIF_REGS_ST_RGBAQ_XYZF2  0x412      /* word 3: REGS 2, 1, 4 */

// FUNC 0x002212D8 TStripBuilder::TStripBuilder
TStripBuilder::TStripBuilder()
{
    beginMeasure();
}

// FUNC 0x00221310 TStripBuilder::TStripBuilder(void *)
TStripBuilder::TStripBuilder(void *buf)
{
    begin(buf);
}

// FUNC 0x00221348 TStripBuilder::~TStripBuilder
TStripBuilder::~TStripBuilder()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr; the deleting form (flags & 1) calls operator delete(this) */
}

// FUNC 0x00221378 TStripBuilder::begin
void TStripBuilder::begin(void *buf)
{
    /* (this order gives retail's store order: sched1 issues the stores that
     * are a value's last use first) */
    this->shader = -1;
    this->measured = 0;
    this->head = (TStripBlock *)buf;
    this->blockCount = 0;
    this->prim = 0;
    this->format = 0;
    this->vertexCount = 0;
    this->stripVertices = 0;
    this->stripCount = 0;
    this->totalVertices = 0;
    this->totalTriangles = 0;
    this->first = 1;
    this->write = 1;
    this->cur = (TStripBlock *)buf;
}

// FUNC 0x002213C0 TStripBuilder::beginMeasure
void TStripBuilder::beginMeasure()
{
    begin(NULL);
    this->write = 0;
}

// FUNC 0x002213F0 TStripBuilder::setShader
void TStripBuilder::setShader(int s)
{
    if (this->shader == s)
        return;
    flush();
    this->shader = s;
    this->first = 1;
}

// FUNC 0x00221440 TStripBuilder::setPrim
void TStripBuilder::setPrim(int prim)
{
    if (this->prim == prim)
        return;
    flush();
    this->prim = prim;
}

/* One strip of n vertices from arrays (NULL: not present). */
// FUNC 0x00221488 TStripBuilder::addStrip
void TStripBuilder::addStrip(const f32 (*pos)[3], const f32 (*nrm)[3], const f32 (*uv)[2],
                             const u32 *col, int n)
{
    endStrip();
    for (int i = 0; i < n; i++) {
        addVertex(pos != NULL ? pos[i] : NULL,
                  nrm != NULL ? nrm[i] : NULL,
                  uv != NULL ? uv[i] : NULL,
                  col != NULL ? &col[i] : NULL);
    }
    endStrip();
}

// FUNC 0x00221568 TStripBuilder::addVertex
void TStripBuilder::addVertex(const f32 *pos, const f32 *nrm, const f32 *uv, const u32 *col)
{
    int fmt = 0;
    if (pos != NULL)
        fmt |= 1;
    if (nrm != NULL)
        fmt |= 2;
    if (uv != NULL)
        fmt |= 4;
    if (col != NULL)
        fmt |= 8;
    if (this->format != fmt) {
        flush();
        this->format = fmt;
    }
    if (pos != NULL) {
        this->pos[this->vertexCount][0] = pos[0];
        this->pos[this->vertexCount][1] = pos[1];
        this->pos[this->vertexCount][2] = pos[2];
    }
    if (nrm != NULL) {
        this->nrm[this->vertexCount][0] = nrm[0];
        this->nrm[this->vertexCount][1] = nrm[1];
        this->nrm[this->vertexCount][2] = nrm[2];
    }
    if (uv != NULL) {
        this->uv[this->vertexCount][0] = uv[0];
        this->uv[this->vertexCount][1] = uv[1];
    }
    if (col != NULL)
        this->col[this->vertexCount] = *col;
    this->vertexCount++;
    this->stripVertices++;
    if (this->vertexCount >= STRIP_BLOCK_VERTICES) {
        /* the block is full: the strip goes on in the next one, from its
         * last two vertices (flush leaves the arrays as they are) */
        flush();
        int i = STRIP_BLOCK_VERTICES - 2;
        addVertex(pos != NULL ? this->pos[i] : NULL,
                  nrm != NULL ? this->nrm[i] : NULL,
                  uv != NULL ? this->uv[i] : NULL,
                  col != NULL ? &this->col[i] : NULL);
        addVertex(pos != NULL ? this->pos[i + 1] : NULL,
                  nrm != NULL ? this->nrm[i + 1] : NULL,
                  uv != NULL ? this->uv[i + 1] : NULL,
                  col != NULL ? &this->col[i + 1] : NULL);
    }
}

/* Closes the open strip.  NOTE a strip of 1 or 2 vertices clears the whole
 * block's vertex count (not only its own vertices), strips already recorded
 * stay in the table: reproduced as is. */
// FUNC 0x00221780 TStripBuilder::endStrip
void TStripBuilder::endStrip()
{
    int n = this->stripVertices;
    if (n == 0)
        return;
    if (n < 3) {
        this->stripVertices = 0;
        this->vertexCount = 0;
        return;
    }
    this->strips[this->stripCount] = n;
    this->stripCount++;
    this->totalTriangles += this->stripVertices - 2;
    this->stripVertices = 0;
    if (STRIP_BLOCK_VERTICES - this->vertexCount < 3)
        flush();
}

// FUNC 0x00221808 TStripBuilder::flush
void TStripBuilder::flush()
{
    endStrip();
    int nv = this->vertexCount;
    if (nv == 0)
        return;
    if (this->write) {
        int i;
        f32 *dst;
        u8  *block = (u8 *)this->cur;
        u32 *data = (u32 *)(block + 0x10);
        u32 *out = data;
        data[0] = VIF_UNPACK(VIF_V4_32, 1, 0x0C0);       /* the GIF tag */
        data[1] = this->vertexCount | GIF_EOP;           /* NLOOP */
        data[2] = (this->prim << GIF_PRIM_SHIFT) | GIF_HI_NREG(3) | GIF_HI_PRE;
        data[4] = 0;
        data[3] = GIF_REGS_ST_RGBAQ_XYZF2;
        *(u32 *)(block + 0x24) = VIF_UNPACK(VIF_S_8, this->stripCount + 1, 0x040);
        block[0x28] = (u8)this->stripCount;
        u8 *table = block + 0x29;
        for (i = 0; i < this->stripCount; i++)
            *table++ = (this->strips[i] - 2) * 3;       /* 3 * the strip's triangles */
        while ((size_t)table & 3)
            *table++ = 0;
        out = (u32 *)table;
        if (this->format & 1) {
            *out = VIF_UNPACK(VIF_V3_32, this->vertexCount, 0x000);
            dst = (f32 *)(out + 1);
            int n = this->vertexCount;
            for (i = 0; i < n; i++) {
                dst[0] = this->pos[i][0];
                dst[1] = this->pos[i][1];
                dst[2] = this->pos[i][2];
                dst += 3;
            }
            out = (u32 *)dst;
        }
        if (this->format & 2) {
            *out = VIF_UNPACK(VIF_V3_32, this->vertexCount, 0x080);
            dst = (f32 *)(out + 1);
            int n = this->vertexCount;
            for (i = 0; i < n; i++) {
                dst[0] = this->nrm[i][0];
                dst[1] = this->nrm[i][1];
                dst[2] = this->nrm[i][2];
                dst += 3;
            }
            out = (u32 *)dst;
        }
        if (this->format & 4) {
            *out = VIF_UNPACK(VIF_V2_32 | VIF_MASK, this->vertexCount, 0x040);
            dst = (f32 *)(out + 1);
            int n = this->vertexCount;
            for (i = 0; i < n; i++) {
                dst[0] = this->uv[i][0];
                dst[1] = this->uv[i][1];
                dst += 2;
            }
            out = (u32 *)dst;
        }
        if (this->format & 8) {
            *out++ = VIF_UNPACK(VIF_V4_8, this->vertexCount, 0x080);
            for (i = 0; i < this->vertexCount; i++)
                *out++ = this->col[i];
        }
        while ((size_t)out & 0xF)
            *out++ = 0;
        int words = out - data;
        TStripBlock *written = this->cur;
        written->qwc = words / 4;
        if (this->first) {
            this->first = 0;
            written->shader = this->shader;
        } else {
            written->shader = 0;
        }
        written->next = (TStripBlock *)out;
        this->cur = (TStripBlock *)out;
    } else {
        /* the words the block will take: header 4, tag 5, strip table
         * 1 + (strips + 1 rounded up), the data with one VIF code each,
         * padded to a quadword */
        this->measured += 10 + (this->stripCount + 4) / 4;
        int fmt = this->format;
        if (fmt & 1)
            this->measured += nv * 3 + 1;
        if (fmt & 2)
            this->measured += nv * 3 + 1;
        if (fmt & 4)
            this->measured += nv * 2 + 1;
        if (fmt & 8)
            this->measured += nv + 1;
        while (this->measured & 3)
            this->measured++;
        this->first = 0;
    }
    this->blockCount++;
    this->totalVertices += this->vertexCount;
    this->vertexCount = 0;
    this->stripVertices = 0;
    this->stripCount = 0;
}

/* Ends the packet: the header (at cur) lists the blocks written since
 * begin(); the next packet starts behind it, 16-aligned. */
// FUNC 0x00221CA0 TStripBuilder::finish
VifModel *TStripBuilder::finish(int a, int b)
{
    flush();
    VifModel *packet = (VifModel *)this->cur;
    packet->mode = a;
    packet->scaleFlags = b;
    packet->self = 0;
    TStripBlock *block = this->head;
    packet->nParts = this->blockCount;
    packet->nVerts = this->totalVertices;
    packet->nPrims = this->totalTriangles;
    int n = this->blockCount;
    if (n > 0) {
        VifPart *entry = packet->part;
        int left = n;
        do {
            left--;
            entry->data = block->data;
            entry->qwc = (u16)block->qwc;
            entry->tex = (u16)block->shader;
            block = block->next;
            entry++;
        } while (left != 0);
    }
    /* the next packet: behind this one's part table, 16-aligned (95.2%:
     * retail loads head before blockCount; sched1 issues the count first, as
     * it feeds the branch - tried the reads in several orders, n as a
     * variable or the field, for / while / do loops) */
    begin((void *)((this->blockCount * sizeof(VifPart) + (u32)packet + 0x1F) & ~0xF));
    return packet;
}

/* The bytes the packet measured so far will need (blocks + header). */
// FUNC 0x00221D70 TStripBuilder::endMeasure
int TStripBuilder::endMeasure()
{
    flush();
    int words = this->measured;
    int size = this->blockCount * 8 + words * 4;    /* the parts' entries + the blocks */
    size = (size + 0x1F) & ~0xF;                    /* + the header, 16-aligned */
    beginMeasure();
    return size;
}

// FUNC 0x00221DC0 TStripBuilder::getCurrentAddress(void)
void *TStripBuilder::getCurrentAddress()
{
    return this->cur;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __user_type_info
 * __ti13TStripBuilder (0x00353450, name 0x00340EC0) on the first call. */
// FUNC 0x00221DC8 __tf13TStripBuilder
extern "C" void *__tf13TStripBuilder(void)
{
    if (__ti13TStripBuilder[0] == 0)
        __rtti_user(__ti13TStripBuilder, "13TStripBuilder");
    return __ti13TStripBuilder;
}
#endif
