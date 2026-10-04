/* Build glue, not decompiled code.
 *
 * src/crtbegin.c's frame_table (retail @0x01026C00) points at the 36
 * per-object .eh_frame blocks GCC 2.9x emitted; a modern compiler emits no
 * such blocks (and all C++ here is built -fno-exceptions), so each name is
 * defined as a single zero word: the zero-length record at which
 * runtime/frame.c stops walking a block.  frame.c therefore registers no
 * FDEs, and an exception, which the bootstrap never throws in practice,
 * reaches terminate() instead of unwinding.
 *
 * Generated from src/crtbegin.c by tools/build.py's author; keep in sync.
 */
#include "types.h"

u32 eh_frame_01026CD8 = 0;
u32 eh_frame_01026E44 = 0;
u32 eh_frame_01026F1C = 0;
u32 eh_frame_01027034 = 0;
u32 eh_frame_01027170 = 0;
u32 eh_frame_01027308 = 0;
u32 eh_frame_01027978 = 0;
u32 eh_frame_01027BDC = 0;
u32 eh_frame_01027CE0 = 0;
u32 eh_frame_01027DF8 = 0;
u32 eh_frame_01027F20 = 0;
u32 eh_frame_01033C94 = 0;
u32 eh_frame_01033E50 = 0;
u32 eh_frame_0103445C = 0;
u32 eh_frame_01033D80 = 0;
u32 eh_frame_0103401C = 0;
u32 eh_frame_010340B0 = 0;
u32 eh_frame_01034144 = 0;
u32 eh_frame_0103426C = 0;
u32 eh_frame_01034390 = 0;
u32 eh_frame_010343E8 = 0;
u32 eh_frame_01039940 = 0;
u32 eh_frame_0103A184 = 0;
u32 eh_frame_01039540 = 0;
u32 eh_frame_01039B24 = 0;
u32 eh_frame_01039860 = 0;
u32 eh_frame_01039848 = 0;
u32 eh_frame_0103BE9C = 0;
u32 eh_frame_0103C490 = 0;
u32 eh_frame_0103C8C8 = 0;
u32 eh_frame_0103C910 = 0;
u32 eh_frame_0103C964 = 0;
u32 eh_frame_0103C9AC = 0;
u32 eh_frame_0103C9F4 = 0;
u32 eh_frame_0103CE00 = 0;
u32 eh_frame_0103C828 = 0;
