/* readr.c - newlib readr.o: _read_r (0x00285198), the reentrant
 * wrapper of the libkernl read() glue (DECI2 TTY, fd 0 only).
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part3_internal.h"

// FUNC 0x00285198 _read_r
//   Caller: __sread (0x00282A80).  Same shape as _write_r: clear the global
//   errno, call read(), copy errno to ptr->_errno on -1.  Returns newlib's
//   `long' (64-bit; __sread truncates it to int).
s64 _read_r(struct _reent *ptr, int fd, void *buf, size_t cnt)
{
    s64 ret;

    sys_errno = 0;
    if ((ret = (s64)read(fd, buf, cnt)) == -1 && sys_errno != 0)
        ptr->_errno = sys_errno;
    return ret;
}
