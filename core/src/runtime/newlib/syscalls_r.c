/* syscalls_r.c - newlib reentrant syscall wrappers writer.o,
 * closer.o, fstatr.o, lseekr.o (0x00284000..0x00284177).  _read_r (readr.o)
 * sits after mprec.o, in readr.c.
 *
 * Each one clears the plain global errno (sys_errno, 0x00353520), calls the
 * libkernl glue function and, if that returned -1 with errno set, copies
 * errno into ptr->_errno.  (The glue functions themselves set errno through
 * __errno(), i.e. _impure_ptr->_errno, not this global.)
 *
 * _write_r returns newlib's `long' and _lseek_r takes and returns `_off_t'
 * (= long): 64 bits on the EE.  The bodies cannot show it (every value is
 * moved with daddu and compared against a 64-bit -1), the callers can: see
 * runtime/newlib/G007_stdio.h.
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part3_internal.h"

// FUNC 0x00284000 _write_r
//   Caller: __swrite (0x00282AE8).
s64 _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt)
{
    s64 ret;

    sys_errno = 0;
    if ((ret = (s64)write(fd, buf, cnt)) == -1 && sys_errno != 0)
        ptr->_errno = sys_errno;
    return ret;
}

// FUNC 0x00284060 _close_r
//   Caller: __sclose (0x00282BD0).  close() is the `return -1` stub at
//   0x002862A0.
int _close_r(struct _reent *ptr, int fd)
{
    int ret;

    sys_errno = 0;
    if ((ret = close(fd)) == -1 && sys_errno != 0)
        ptr->_errno = sys_errno;
    return ret;
}

// FUNC 0x002840B8 _fstat_r
//   Caller: __smakebuf (0x00281C38: _fstat_r, _malloc_r, then isatty).
int _fstat_r(struct _reent *ptr, int fd, struct stat *st)
{
    int ret;

    sys_errno = 0;
    if ((ret = fstat(fd, st)) == -1 && sys_errno != 0)
        ptr->_errno = sys_errno;
    return ret;
}

// FUNC 0x00284118 _lseek_r
//   Callers: __swrite (0x00282AE8, append mode) and __sseek (0x00282B68).
//   It calls the `return -1` stub at 0x002862B0, which is therefore lseek.
//   pos and the result are newlib's 64-bit off_t (libc.h's lseek; __sseek
//   truncates the result with dsll32 / dsra32 before storing _offset).
s64 _lseek_r(struct _reent *ptr, int fd, s64 pos, int whence)
{
    s64 ret;

    sys_errno = 0;
    if ((ret = lseek(fd, pos, whence)) == -1 && sys_errno != 0)
        ptr->_errno = sys_errno;
    return ret;
}
