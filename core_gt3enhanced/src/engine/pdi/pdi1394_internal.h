/* pdi1394_internal.h - unit u128 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0026F538..0x00270628, second pass.
 *
 * Translation units:
 *   T541  pdi1394: Polyphony's i.LINK (IEEE 1394) client class
 *         (engine/pdi/T541_pdi1394.h: the class, the packet commands and node
 *         states), over the C driver of G003 (engine/pdi/G003_pdi1394_rpc.h, unit
 *         u127: SIF RPC to the IOP module, a semaphore-guarded message queue
 *         per IOP node).  IOP node / channel 0 is this console: every message
 *         sent is also pushed to channel 0, so a group's count includes this
 *         console and a barrier waits for its own packet too.
 *
 * Layout: offsets in comments are the ORIGINAL (egcs) ones.  pdi1394 has
 * its vptr after its data members (+0xC0, object size 0xC4); a GCC 14 build
 * puts the vptr at +0 and moves every member by 4 (the size stays 0xC4).
 *
 * Only what is local to T541 is here: its static helpers (its .bss table is
 * a file static in T541 itself).
 */
#ifndef U128_H
#define U128_H

#include "types.h"
#include "libc.h"
#include "cxxrt.h"
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "engine/pdi/G003_pdi1394_rpc.h"          /* the driver; Pdi1394Msg, Guid1394, Pdi1394Status */
#include "engine/pdi/T541_pdi1394.h"

/* The 8 slot entries as one copyable block (collectStates copies nodeId[]
 * and nodeState[] with 8-byte unaligned moves, i.e. a struct copy). */
struct SlotTable {
    int v[8];
};

/* Static helpers of T541. */
void sort_slots_by_guid(int *ids, Guid1394 *guids, int n);       /* 0x00270408 */
int  guid_greater(const u8 *a, const u8 *b, int n);              /* 0x00270560 */

/* T541's .bss (g_004AB4D0, the GUID table) is defined in T541 as a file
 * static (retail's .bss: a C++ file's uninitialised global would be zeros
 * in .data). */

#endif /* U128_H */
