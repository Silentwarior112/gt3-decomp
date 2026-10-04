/* Binary data the decompiled sources reference but do not spell out as C.
 * Build glue, not decompiled code: tools/build.py assembles it from the
 * decomp/bootstrap root so the .incbin paths resolve.
 *
 * g_startupaImgGz   the gzip'd "startupa.img" warning-screen texture,
 *                   .data 0x01027F38..0x01033C93 in the retail file
 *                   (data/startupa_img.gz, byte-identical to those 0xBD5B bytes;
 *                   regenerate with tools/extract_startup_img.py).
 * g_gsVu1Microcode  VU1 microcode, .data 0x01034730, 0x3DA8 bytes, stored
 *                   LCG-XOR scrambled and unscrambled IN PLACE at run time
 *                   (gs_driver.cpp), so it is writable and must be the
 *                   scrambled bytes (data/gs_vu1_microcode_scrambled.bin).
 * g_gsVu0Microcode  VU0 microcode, .rdata 0x01040C00, 0x398 bytes
 *                   (data/gs_vu0_microcode.bin).
 * Both microcode images are DMA'd to the VUs: quadword alignment is required;
 * 128 keeps them on their own cache lines as well.
 */
	.data
	.align	3
	.globl	g_startupaImgGz
	.type	g_startupaImgGz, @object
g_startupaImgGz:
	.incbin	"data/startupa_img.gz"
	.size	g_startupaImgGz, . - g_startupaImgGz

	.align	7
	.globl	g_gsVu1Microcode
	.type	g_gsVu1Microcode, @object
g_gsVu1Microcode:
	.incbin	"data/gs_vu1_microcode_scrambled.bin"
	.size	g_gsVu1Microcode, . - g_gsVu1Microcode
	.align	7

	.section .rodata
	.align	7
	.globl	g_gsVu0Microcode
	.type	g_gsVu0Microcode, @object
g_gsVu0Microcode:
	.incbin	"data/gs_vu0_microcode.bin"
	.size	g_gsVu0Microcode, . - g_gsVu0Microcode
	.align	7
