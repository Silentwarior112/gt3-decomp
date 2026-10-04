/* kernl_rest_internal.h - libkernl internals shared between the files of the
 * kernl_rest unit (filedev.c, loadfile.c, iopreboot.c, initsys.c, deci2_tty.c).
 * Not part of the Sony API.
 */
#ifndef KERNL_REST_INTERNAL_H
#define KERNL_REST_INTERNAL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DECI2 TTYP console driver (deci2_tty.c).                                 */
int  _deci2_tty_init(void);                          /* 0x010235B8: 1 ok, 0 sceDeci2Open failed */
int  _deci2_tty_write(const char *buf, int len);     /* 0x01023398: bytes consumed, -1 busy/error */
int  _deci2_tty_read(char *buf, int len);           /* 0x010234E8: bytes read (line based)     */

/* initsys.c: clears the "TTY opened" flag (0x0103DADC) used by write()/read()
 * and re-runs _deci2_tty_init() (tail call; result unused).  Called by
 * sceSifSyncIop after an IOP reboot.                                        */
void tty_reinit_after_iop_reset(void);               /* 0x01022C40 */

#ifdef __cplusplus
}
#endif

#endif /* KERNL_REST_INTERNAL_H */
