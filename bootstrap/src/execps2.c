/* execps2.c - the bootstrap's launch into core.gt3 (BOOTSTRAP_SCUS-97102),
 * 0x01016F70..0x01016FD8.
 *
 * Placement decision (libmc unit): this is APPLICATION code (Polyphony's
 * bootstrap sources), not Sony SDK code, although it sits in .text between
 * the last libmc function (sceMcGetEntSpace, ends 0x01016F6C) and libgcc2
 * (__do_global_dtors, 0x01016FD8).  Evidence:
 *   - Nothing in it belongs to libmc: no mcserv RPC, no libmc state, and no
 *     Sony libmc 2.x API has this shape.  libmc's .data ends at 0x0103BE90
 *     exactly where libgcc2's .data (dtor cursor, __terminate_func, FDE
 *     records) begins, so this object contributes no .data - a plain C
 *     translation unit linked after -lmc (e.g. last on the link line), not a
 *     member of any Sony archive; libkernl's own ExecPS2 here is just the
 *     syscall-7 stub it calls.
 *   - The same code (byte pattern with call targets masked) is in the other
 *     Polyphony bootstraps checked:
 *       BOOTSTRAP_SCES-50294 (GT3 PAL, SDK 2.2.4): same address 0x01016F70,
 *         same targets 0x0101B870 / 0x0101B830;
 *       BOOTSTRAP_SCES-50858 (linked against "PsIIlibmc   2500",
 *         "PsIIlibkernl2520"): at 0x010160E8, again directly after libmc's
 *         last epilogue; its jal goes to that build's syscall-0x0B stub
 *         (0x0101B770) and its tail jump to SDK 2.5's C ExecPS2 wrapper
 *         (0x01023428: stages argv in a static buffer, then syscall-7 stub
 *         0x0101B730) - i.e. it references the ExecPS2 symbol and was
 *         carried unchanged across SDK versions as part of the bootstrap
 *         project.
 *     No match in the BASE_* game ELFs (SCUS-97102, SCES-50294, SCES-50858,
 *     PAPX-90509).
 *   - Its only caller is main (0x01000538), with the Polyphony-specific
 *     argv {source path, "hot", NULL}.
 * Nothing in this binary calls AddSbusIntcHandler (syscall 0x0A stub
 * 0x0101B860 has no callers); the removal loop is unconditional cleanup of
 * whatever SBUS handlers the SIF/IOP side left installed before handing the
 * machine to core.gt3.
 */
#include "types.h"
#include "eekernel.h"

// FUNC 0x01016F70 exec_ps2_with_sbus_cleanup
// Removes the SBUS interrupt handlers of causes 0..15 (RemoveSbusIntcHandler,
// syscall 0x0B, loop counter compared signed), then tail-calls
// ExecPS2(entry, gp = NULL, argc, argv) (syscall 7 stub 0x0101B830), which
// does not return on success.  Only caller: main (0x01000538, call at
// 0x01000650) as (entry, 2, {path, "hot", NULL}), where entry is the value
// returned by ImageLoader::load (0x010049C8); main only gets here when that
// value is non-zero (entry == 0 -> main spins forever at 0x01000610) and
// ignores the result.
// void: egcs emits the tail jump (`j ExecPS2` after the epilogue, as in
// retail) only for a final call whose value is unused; ExecPS2's $v0 still
// reaches the caller unchanged.
void exec_ps2_with_sbus_cleanup(void *entry, int argc, char **argv)
{
    int i;

    for (i = 0; i < 16; i++)
        RemoveSbusIntcHandler(i);
    ExecPS2(entry, NULL, argc, argv);
}
