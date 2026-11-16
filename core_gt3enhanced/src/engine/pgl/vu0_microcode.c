/* vu0_microcode.c - the VU0 microcode g_gsVu0Microcode,
 * 0x00349600..0x00349998 (0x398 bytes, 115 VU instructions), the last
 * initialised data before .gcc_except_table (0x00349A00).  Read only by
 * u115's gfx_init (vu_upload_microcode(0, 0, 0x398, g_gsVu0Microcode)).
 * The same bytes as the bootstrap's data/gs_vu0_microcode.bin.
 *
 * A data-only object of its own (no code, no frame info: no DATA_TU line):
 *   - the bootstrap's ELF has its section headers: there these 0x398 bytes
 *     are the whole of an output section `.rdata' (0x01040C00, flags WA,
 *     alignment 16), which the linker script puts after `.rodata'
 *     (0x0103DE00..0x01040BC2) and before `.gcc_except_table' (0x01041000),
 *     every output section at ALIGN(128) (as ps2sdk-main's
 *     ee/startup/linkfile has them: `.rdata ALIGN(128): { *(.rdata) }'
 *     between the two).  egcs writes read-only data to
 *     `.rodata' (and GAS's MIPS `.rdata' directive means `.rodata' in ELF):
 *     a writable `.rdata' input section is an explicitly named one, most
 *     likely an assembled or converted blob linked in as its own object;
 *   - the core has the same layout with its section headers stripped:
 *     .rodata ends with libgcc2's _umoddi3.o __clz_tab at 0x003495A0, the
 *     microcode starts at the next 128 boundary (0x00349600, zeros between),
 *     .gcc_except_table at the next 128 boundary after its end (0x00349998 ->
 *     0x00349A00, zeros between);
 *   - the toolchain's libgcc.a members before it (_udivdi3.o, _umoddi3.o)
 *     have nothing in .rodata but their __clz_tab (0x100 bytes each).
 * Its link order cannot be told (its output section holds nothing else);
 * `// LINK_AFTER' puts it after libgcc2's file, the last .rodata before it,
 * so the data tools give it the range 0x00349600..0x00349A00.
 *
 * Here `const' in .rodata, which the tools and the full link place (the
 * original's .rdata is writable, but nothing writes it), 128-aligned as the
 * output section's ALIGN(128) put it (as the bootstrap's link/blobs.S does;
 * the DMA to VU0 micro memory needs 16).
 */
// LINK_AFTER src/runtime/libgcc/libgcc2_part3.c
// DATA_START 0x00349600
#include "types.h"

/* One 64-bit VU instruction, little-endian: the lower instruction's word
 * first, then the upper's.  Where the upper word has its I bit (bit 31) set,
 * the lower word is the float loaded into the I register (0x3F800000 1.0,
 * 0x3F490FDB pi/4, ...).  0x80000035 is the lower no-op this code pads with
 * (`ior vi00, vi00, vi00'), 0x000002FF the upper `nop', 0x400002FF `nop'
 * with the E bit (end of a program; one more instruction follows). */
#define VU_WORD(w)          (u8)(w), (u8)((w) >> 8), (u8)((w) >> 16), (u8)((w) >> 24)
#define VU_INSN(lower, upper)   VU_WORD(lower), VU_WORD(upper)

const u8 g_gsVu0Microcode[0x398] __attribute__((aligned(128))) = {   /* 0x00349600 */
    VU_INSN(0x80000035, 0x01E1E1BC),     /* 0x000 */
    VU_INSN(0x80000035, 0x01E1E8BD),     /* 0x008 */
    VU_INSN(0x80000035, 0x01E1F0BE),     /* 0x010 */
    VU_INSN(0x80000035, 0x01E1F84B),     /* 0x018 */
    VU_INSN(0x80000035, 0x01E2E1BC),     /* 0x020 */
    VU_INSN(0x80000035, 0x01E2E8BD),     /* 0x028 */
    VU_INSN(0x80000035, 0x01E2F0BE),     /* 0x030 */
    VU_INSN(0x80000035, 0x01E2F88B),     /* 0x038 */
    VU_INSN(0x80000035, 0x01E3E1BC),     /* 0x040 */
    VU_INSN(0x80000035, 0x01E3E8BD),     /* 0x048 */
    VU_INSN(0x80000035, 0x01E3F0BE),     /* 0x050 */
    VU_INSN(0x80000035, 0x01E3F8CB),     /* 0x058 */
    VU_INSN(0x80000035, 0x01E4E1BC),     /* 0x060 */
    VU_INSN(0x81FC0B3C, 0x01E4E8BD),     /* 0x068 */
    VU_INSN(0x81FD133C, 0x01E4F0BE),     /* 0x070 */
    VU_INSN(0x81FE1B3C, 0x01E4FFCB),     /* 0x078 */
    VU_INSN(0x80000035, 0x400002FF),     /* 0x080 */
    VU_INSN(0x80000035, 0x000002FF),     /* 0x088 */
    VU_INSN(0x80000035, 0x01FC09BC),     /* 0x090 */
    VU_INSN(0x80000035, 0x01FC10BD),     /* 0x098 */
    VU_INSN(0x80000035, 0x01FC18BE),     /* 0x0A0 */
    VU_INSN(0x80000035, 0x01FC270B),     /* 0x0A8 */
    VU_INSN(0x80000035, 0x01FD09BC),     /* 0x0B0 */
    VU_INSN(0x80000035, 0x01FD10BD),     /* 0x0B8 */
    VU_INSN(0x80000035, 0x01FD18BE),     /* 0x0C0 */
    VU_INSN(0x80000035, 0x01FD274B),     /* 0x0C8 */
    VU_INSN(0x80000035, 0x01FE09BC),     /* 0x0D0 */
    VU_INSN(0x80000035, 0x01FE10BD),     /* 0x0D8 */
    VU_INSN(0x80000035, 0x01FE18BE),     /* 0x0E0 */
    VU_INSN(0x80000035, 0x01FE278B),     /* 0x0E8 */
    VU_INSN(0x80000035, 0x01FF09BC),     /* 0x0F0 */
    VU_INSN(0x80000035, 0x01FF10BD),     /* 0x0F8 */
    VU_INSN(0x80000035, 0x01FF18BE),     /* 0x100 */
    VU_INSN(0x80000035, 0x01FF27CB),     /* 0x108 */
    VU_INSN(0x80000035, 0x400002FF),     /* 0x110 */
    VU_INSN(0x80000035, 0x000002FF),     /* 0x118 */
    VU_INSN(0x01F80008, 0x000002FF),     /* 0x120 */
    VU_INSN(0x01F90009, 0x000002FF),     /* 0x128 */
    VU_INSN(0x01FA000A, 0x000002FF),     /* 0x130 */
    VU_INSN(0x01FB000B, 0x000002FF),     /* 0x138 */
    VU_INSN(0x80000035, 0x01FCC1BC),     /* 0x140 */
    VU_INSN(0x80000035, 0x01FCC8BD),     /* 0x148 */
    VU_INSN(0x80000035, 0x01FCD0BE),     /* 0x150 */
    VU_INSN(0x80000035, 0x01FCD84B),     /* 0x158 */
    VU_INSN(0x80000035, 0x01FDC1BC),     /* 0x160 */
    VU_INSN(0x80000035, 0x01FDC8BD),     /* 0x168 */
    VU_INSN(0x80000035, 0x01FDD0BE),     /* 0x170 */
    VU_INSN(0x80000035, 0x01FDD88B),     /* 0x178 */
    VU_INSN(0x80000035, 0x01FEC1BC),     /* 0x180 */
    VU_INSN(0x80000035, 0x01FEC8BD),     /* 0x188 */
    VU_INSN(0x80000035, 0x01FED0BE),     /* 0x190 */
    VU_INSN(0x80000035, 0x01FED8CB),     /* 0x198 */
    VU_INSN(0x80000035, 0x01FFC1BC),     /* 0x1A0 */
    VU_INSN(0x81F80B3C, 0x01FFC8BD),     /* 0x1A8 */
    VU_INSN(0x81F9133C, 0x01FFD0BE),     /* 0x1B0 */
    VU_INSN(0x81FA1B3C, 0x01FFDECB),     /* 0x1B8 */
    VU_INSN(0x80000035, 0x400002FF),     /* 0x1C0 */
    VU_INSN(0x80000035, 0x000002FF),     /* 0x1C8 */
    VU_INSN(0x80000035, 0x01E108AA),     /* 0x1D0 */
    VU_INSN(0x80000035, 0x000002FF),     /* 0x1D8 */
    VU_INSN(0x80000035, 0x01E2112A),     /* 0x1E0 */
    VU_INSN(0x80000035, 0x01E110EA),     /* 0x1E8 */
    VU_INSN(0x80000035, 0x01E1216A),     /* 0x1F0 */
    VU_INSN(0xBE2AAAA4, 0x81E321AA),     /* 0x1F8 */
    VU_INSN(0x80000035, 0x01E019FE),     /* 0x200 */
    VU_INSN(0x3C08873E, 0x81E5212A),     /* 0x208 */
    VU_INSN(0xB94FB21F, 0x81E02A3F),     /* 0x210 */
    VU_INSN(0x80000035, 0x01E0323F),     /* 0x218 */
    VU_INSN(0x362E9C14, 0x81E008BF),     /* 0x220 */
    VU_INSN(0x80000035, 0x01E02063),     /* 0x228 */
    VU_INSN(0x80000035, 0x400002FF),     /* 0x230 */
    VU_INSN(0x80000035, 0x000002FF),     /* 0x238 */
    VU_INSN(0x806603BE, 0x01F7086C),     /* 0x240 */
    VU_INSN(0x81E72B3C, 0x01E3A1BC),     /* 0x248 */
    VU_INSN(0x80000035, 0x01E3A8BD),     /* 0x250 */
    VU_INSN(0x80000035, 0x01E3B14A),     /* 0x258 */
    VU_INSN(0x80000035, 0x01E20ABE),     /* 0x260 */
    VU_INSN(0x80000035, 0x01E20A29),     /* 0x268 */
    VU_INSN(0x80000035, 0x01E42181),     /* 0x270 */
    VU_INSN(0x80000035, 0x01E009BF),     /* 0x278 */
    VU_INSN(0x80000035, 0x01E810FC),     /* 0x280 */
    VU_INSN(0x80000035, 0x01E810FD),     /* 0x288 */
    VU_INSN(0x80000035, 0x01E810CE),     /* 0x290 */
    VU_INSN(0x800003BF, 0x01E43182),     /* 0x298 */
    VU_INSN(0x80000035, 0x01E039DC),     /* 0x2A0 */
    VU_INSN(0x80000035, 0x400002FF),     /* 0x2A8 */
    VU_INSN(0x80000035, 0x000002FF),     /* 0x2B0 */
    VU_INSN(0x3F800000, 0x800002FF),     /* 0x2B8 */
    VU_INSN(0x40000003, 0x01E00962),     /* 0x2C0 */
    VU_INSN(0x80000035, 0x01E009A6),     /* 0x2C8 */
    VU_INSN(0x80000035, 0x01E11168),     /* 0x2D0 */
    VU_INSN(0x80000035, 0x01E111AC),     /* 0x2D8 */
    VU_INSN(0x800533BC, 0x000002FF),     /* 0x2E0 */
    VU_INSN(0x3F800000, 0x800002FF),     /* 0x2E8 */
    VU_INSN(0x800003BF, 0x01E00222),     /* 0x2F0 */
    VU_INSN(0x80000035, 0x01E00060),     /* 0x2F8 */
    VU_INSN(0x3F7FFFF5, 0x81E109EA),     /* 0x300 */
    VU_INSN(0x80000035, 0x01E009FE),     /* 0x308 */
    VU_INSN(0x80000035, 0x01E1396A),     /* 0x310 */
    VU_INSN(0xBEAAA61C, 0x81E729AA),     /* 0x318 */
    VU_INSN(0x80000035, 0x01E02A3F),     /* 0x320 */
    VU_INSN(0x3E4C40A6, 0x81E7316A),     /* 0x328 */
    VU_INSN(0x80000035, 0x01E0323F),     /* 0x330 */
    VU_INSN(0xBE0E6C63, 0x81E729AA),     /* 0x338 */
    VU_INSN(0x80000035, 0x01E02A3F),     /* 0x340 */
    VU_INSN(0x3DC577DF, 0x81E7316A),     /* 0x348 */
    VU_INSN(0x80000035, 0x01E0323F),     /* 0x350 */
    VU_INSN(0xBD6501C4, 0x81E729AA),     /* 0x358 */
    VU_INSN(0x80000035, 0x01E02A3F),     /* 0x360 */
    VU_INSN(0x3CB31652, 0x81E7316A),     /* 0x368 */
    VU_INSN(0x3F490FDB, 0x81E0323F),     /* 0x370 */
    VU_INSN(0xBB84D7E7, 0x81E0423F),     /* 0x378 */
    VU_INSN(0x80000035, 0x01E02923),     /* 0x380 */
    VU_INSN(0x80000035, 0x400002FF),     /* 0x388 */
    VU_INSN(0x80000035, 0x000002FF),     /* 0x390 */
};
