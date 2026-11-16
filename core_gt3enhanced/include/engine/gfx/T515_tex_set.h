/* T515_tex_set.h - TexSet (? name), a texture set placed in VRAM (translation
 * unit T515, unit u112), 0x00244FA8..0x00245390.
 *
 * VRAM from the allocator g_002F4250 for a Tex1 image (linked for it and
 * uploaded: queued on the packet path, else at once; its register blocks used
 * in place, or copied by ownTexRegs) or for one raw image (the register block
 * inside the object). */
#ifndef GAME_T515_TEX_SET_H
#define GAME_T515_TEX_SET_H

#include "game/game.h"
#include "engine/pgl/G001_flare.h"                    /* GsTexRegs (held by value), Tex1Image */

/* Not polymorphic.  Size 0x40 (a stack TexSet at sp+0x00 and the next local
 * at sp+0x40 in 0x0013E0B8; single 0x18 + 0x28). */
class TexSet {
public:
    TexSet();                                   /* 0x00244FA8 addr -1, nothing */
    ~TexSet();                                  /* 0x00244FC8 clear() */
    void allocVram(int size);                   /* 0x00245010 clear(), then size words (addr -1 if none) */
    void clear();                               /* 0x00245060 frees the VRAM and a copied register array */
    void load(Tex1Image *img);                  /* 0x002450B8 VRAM for img, linked and uploaded; its register blocks */
    void loadImage(int psm, int w, int h, const void *pixels); /* 0x00245180 a raw image, the block in single */
    void ownTexRegs();                          /* 0x00245320 the borrowed register blocks -> a malloc'd copy */

    int        addr;                            /* 0x00 VRAM word address, -1 none */
    int        size;                            /* 0x04 words */
    GsTexRegs *tex;                             /* 0x08 the register blocks (the image's, a copy, or &single) */
    int        nTex;                            /* 0x0C */
    int        ownsTex;                         /* 0x10 tex is a malloc'd copy */
    u8         unk_14[4];                       /* 0x14 */
    GsTexRegs  single;                          /* 0x18 the block of a loadImage */
};
EGCS_SIZE(TexSet, 0x40);

#endif /* GAME_T515_TEX_SET_H */
