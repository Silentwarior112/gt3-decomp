/* T515 - TexSet (? name): a texture set placed in VRAM from g_002F4250, a
 * Tex1 image (its register blocks used in place, or copied by ownTexRegs) or
 * one raw image (the register block inside the object, `single`).  The
 * uploads are queued on the GIF list when a packet is open (the VIF1 list
 * then waits for them), else done at once. */
#include "engine/gfx/vram_internal.h"
#include "engine/gfx/T515_tex_set.h"

// FUNC 0x00244FA8 TexSet::TexSet
TexSet::TexSet()
{
    size = 0;
    addr = -1;
    tex = NULL;
    ownsTex = 0;
}

// FUNC 0x00244FC8 TexSet::~TexSet
//   egcs's deleting form calls operator delete when (flags & 1).
TexSet::~TexSet()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    clear();
}

// FUNC 0x00245010 TexSet::allocVram
//   Drops what it had, then words words of VRAM (addr -1 if none).
//   Not exact (93.8%): retail copies alloc's result to $v1 before the two
//   stores (`daddu v1,v0`), egcs's combine here stores $v0 directly.  Tried:
//   both store orders, the result in a local / an inline wrapper / an inline
//   setter, a dead test of it, s64; the reference compiler 991111-01 gives
//   the same.
void TexSet::allocVram(int words)
{
    int vramAddr;

    clear();
    vramAddr = g_002F4250.alloc(words, NULL);
    size = words;
    addr = vramAddr;
}

// FUNC 0x00245060 TexSet::clear
//   Frees the VRAM (dealloc ignores -1) and a copied register array.
void TexSet::clear()
{
    g_002F4250.dealloc(addr);
    if (ownsTex != 0)
        free(tex);
    ownsTex = 0;
    size = 0;
    tex = NULL;
    addr = -1;
}

// FUNC 0x002450B8 TexSet::load
//   VRAM for a Tex1 set, linked for it and uploaded (queued, else
//   tex1_upload_to_vram).  Uses the image's register blocks.
void TexSet::load(Tex1Image *img)
{
    clear();
    allocVram(img->vramSize << VRAM_BLOCK_SHIFT);
    tex1_set_vram_base(img, addr);
    if (gfx_packet_is_open() != 0) {
        int prevList = gfx_select_list(PGL_LIST_GIF);

        tex1_upload_packet(img);
        gfx_put_irq_marker(PGL_MARK_SIGNAL);
        gfx_select_list(PGL_LIST_VIF1);
        gfx_put_irq_marker(PGL_MARK_WAIT);
        gfx_select_list(prevList);
        gfx_texflush();
    } else {
        tex1_upload_to_vram(img);
    }
    tex = img->tex;
    nTex = img->nTex;
    ownsTex = 0;
}

// FUNC 0x00245180 TexSet::loadImage
//   VRAM for a raw w x h image of psm (the width rounded up to the psm's
//   page), uploaded (queued, else gs_upload_image_sync), and the one
//   register block (single) set up for it: clamped, the whole image.  No
//   callers.
void TexSet::loadImage(int psm, int w, int h, const void *pixels)
{
    s32 alignedW;
    int words;

    words = tex_vram_size(psm, w, h, &alignedW);
    clear();
    allocVram(words);
    if (gfx_packet_is_open() != 0) {
        int prevList = gfx_select_list(PGL_LIST_GIF);

        gs_upload_image(addr, psm, alignedW, 0, 0, w, h, pixels);
        gfx_put_irq_marker(PGL_MARK_SIGNAL);
        gfx_select_list(PGL_LIST_VIF1);
        gfx_put_irq_marker(PGL_MARK_WAIT);
        gfx_select_list(prevList);
        gfx_texflush();
    } else {
        gs_upload_image_sync(addr, psm, alignedW, 0, 0, w, h, pixels);
    }
    nTex = 1;
    tex = &single;
    texregs_init(&single);
    texregs_set_buffer(tex, 0, addr, psm, alignedW);
    texregs_set_size(&tex->tex0, w, h);
    texregs_set_clamp_region(tex, PGL_REGION_U, 0, w - 1);
    texregs_set_clamp_region(tex, PGL_REGION_V, 0, h - 1);
    texregs_set_mode(tex, PGL_TEXPARAM_WMS, PGL_WRAP_CLAMP);
    texregs_set_mode(tex, PGL_TEXPARAM_WMT, PGL_WRAP_CLAMP);
}

// FUNC 0x00245320 TexSet::ownTexRegs
//   Replaces the borrowed register blocks with a malloc'd copy (not
//   NULL-checked).  No callers.
void TexSet::ownTexRegs()
{
    if (ownsTex == 0) {
        int bytes = nTex * sizeof(GsTexRegs);
        GsTexRegs *regsCopy = (GsTexRegs *)malloc(bytes);

        memcpy(regsCopy, tex, bytes);
        tex = regsCopy;
        ownsTex = 1;
    }
}
