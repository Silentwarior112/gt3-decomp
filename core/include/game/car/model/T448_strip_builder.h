/* T448_strip_builder.h - TStripBuilder: triangle strips into VIF / GIF
 * packets (translation unit T448, unit u100).
 *
 * Vertices (any of position, normal, uv, colour) collect in 64-entry arrays;
 * endStrip records a strip of the block.  flush writes the block at `cur':
 * a TStripBlock header, then UNPACK codes for the GIF tag, the strip table
 * and the vertex attributes present.  A full block (64 vertices) is flushed
 * and the strip goes on in the next one.  finish() writes the packet header
 * (a G001 VifModel) and starts a new packet behind it.  Without a buffer
 * (beginMeasure) only the size is counted.  Users: TirePacket (T462),
 * RaceTirePacket (T449), u055. */
#ifndef GAME_T448_STRIP_BUILDER_H
#define GAME_T448_STRIP_BUILDER_H

#include "game/game.h"
#include "engine/pgl/G001_flare.h"                /* VifModel / VifPart: the packet header finish() writes */

/* One block in the buffer (a DMA chain element). */
struct TStripBlock {
    TStripBlock *next;                      /* 0x00 the end of this block = the next one */
    u32          qwc;                       /* 0x04 the quadwords after the header */
    u32          shader;                    /* 0x08 (first block after setShader only) */
    u32          unk_0C;                    /* 0x0C */
    u32          data[1];                   /* 0x10 VIF codes, GIF tag, the strip table, the vertices */
};

/* The packet header finish() writes (the blocks are in the buffer before
 * it) is G001's VifModel, which model_draw draws: self = 0 (nothing to
 * relocate), ptr04 not written, mode = finish's a (the lighting mode; 1:
 * the tread side, lit; 0 is model_draw's unlit fast path), scaleFlags = b
 * (0), nParts = the blocks, nVerts / nPrims = the vertices / triangles,
 * part[i] = block i's data, qwc and shader (VifPart.tex). */

/* vtable _vt$13TStripBuilder 0x00340EB0 (1 slot), typeinfo
 * __tf13TStripBuilder 0x00221DC8 (__user_type_info 0x00353450).  0x98C bytes
 * of data, vptr +0x98C.  Size 0x990 (on the stack of TirePacket::virtual_1). */
class TStripBuilder {
public:
    TStripBuilder();                        /* 0x002212D8 measuring only (beginMeasure) */
    TStripBuilder(void *buf);               /* 0x00221310 begin(buf) */
    virtual ~TStripBuilder();               /* [0] +0x0C 0x00221348 */

    void  begin(void *buf);                 /* 0x00221378 */
    void  beginMeasure();                   /* 0x002213C0 */
    void  setShader(int s);                 /* 0x002213F0 the number the part-texture callback (TirePacket::virtual_2)
                                               gets when the block is drawn (VifPart.tex; 0: none) */
    void  setPrim(int prim);                /* 0x00221440 the GS PRIM */
    void  addStrip(const f32 (*pos)[3], const f32 (*nrm)[3], const f32 (*uv)[2],
                   const u32 *col, int n);  /* 0x00221488 */
    void  addVertex(const f32 *pos, const f32 *nrm, const f32 *uv, const u32 *col); /* 0x00221568 NULL: absent */
    void  endStrip();                       /* 0x00221780 */
    void  flush();                          /* 0x00221808 writes the block */
    VifModel *finish(int a, int b);         /* 0x00221CA0 the packet header; a new packet behind it */
    int   endMeasure();                     /* 0x00221D70 the bytes the packet needs */
    void *getCurrentAddress();              /* 0x00221DC0 cur */

    TStripBlock *head;                      /* 0x000 the first block */
    TStripBlock *cur;                       /* 0x004 the block being written */
    int  blockCount;                        /* 0x008 */
    f32  pos[64][3];                        /* 0x00C */
    f32  nrm[64][3];                        /* 0x30C */
    f32  uv[64][2];                         /* 0x60C */
    u32  col[64];                           /* 0x80C */
    int  strips[21];                        /* 0x90C the vertex counts of the block's strips */
    int  shader;                            /* 0x960 */
    int  prim;                              /* 0x964 */
    int  format;                            /* 0x968 1 pos, 2 normal, 4 uv, 8 colour */
    int  vertexCount;                       /* 0x96C in the block */
    int  stripVertices;                     /* 0x970 of the open strip */
    int  stripCount;                        /* 0x974 in the block */
#ifdef BOOT_OLD_GCC
    bool first;                             /* 0x978 the next block starts a shader (bool: retail's
                                               flush loads it ahead of the u32 packet stores) */
#else
    int  first;
#endif
    int  totalVertices;                     /* 0x97C */
    int  totalTriangles;                    /* 0x980 */
    int  write;                             /* 0x984 0: measure only */
    int  measured;                          /* 0x988 the words needed so far */
    /*   vptr                                  0x98C */
};
EGCS_SIZE(TStripBuilder, 0x990);

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and its storage itself (and rejects
 * hand declarations). */
extern "C" {
void *__tf13TStripBuilder(void);            /* 0x00221DC8 */
extern u32 __ti13TStripBuilder[2];          /* 0x00353450 gp-0x45A0 __user_type_info */
}
#endif

#endif /* GAME_T448_STRIP_BUILDER_H */
