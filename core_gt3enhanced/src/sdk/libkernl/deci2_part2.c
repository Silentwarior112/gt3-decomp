/* deci2_part2.c - Sony libkernl DECI2 client stubs (deci2.o),
 * 0x0028D7C0..0x0028D958.
 *
 * Each stub packs its arguments into a small parameter block on the stack
 * and hands it to the Deci2Call kernel syscall (0x7C, 0x00286090).  Negative
 * function numbers are the "Ex" (interrupt / handler context) forms.  The
 * same machine code as the bootstrap's sdk/libkernl/deci2_tty.c.
 * Users: the libkernl DECI2 TTY code (deci2_tty_handler 0x00286DF0,
 * _deci2_tty_write 0x00286F88, deci2_sink 0x002872A0).  sceDeci2Open is
 * the first function of deci2.o, in u141.
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/deci2_libgraph_internal.h"

// FUNC 0x0028D7C0 sceDeci2Close
//   Deci2Call(2, {s}).  No callers.
int sceDeci2Close(int s)
{
    u32 param[1];

    param[0] = s;
    return Deci2Call(DECI2_CALL_CLOSE, param);
}

// FUNC 0x0028D7E8 sceDeci2ReqSend
//   Deci2Call(3, {s, dest}) - request a send slot to node `dest` (sign-
//   extended byte); the handler then gets DECI2_WRITE events.
int sceDeci2ReqSend(int s, char dest)
{
    u32 param[2];

    param[0] = s;
    param[1] = (int)(s8)dest;
    return Deci2Call(DECI2_CALL_REQSEND, param);
}

// FUNC 0x0028D818 sceDeci2Poll
//   Deci2Call(4, {s}) - pump the DECI2 manager (delivers handler events).
int sceDeci2Poll(int s)
{
    u32 param[1];

    param[0] = s;
    return Deci2Call(DECI2_CALL_POLL, param);
}

// FUNC 0x0028D840 sceDeci2ExRecv
//   Deci2Call(-5, {s, buf, len}) - receive from inside the handler.
int sceDeci2ExRecv(int s, void *buf, unsigned short len)
{
    u32 param[3];

    param[0] = s;
    param[1] = (u32)buf;
    param[2] = len;
    return Deci2Call(-DECI2_CALL_EXRECV, param);
}

// FUNC 0x0028D878 sceDeci2ExSend
//   Deci2Call(-6, {s, buf, len}) - send from inside the handler.
int sceDeci2ExSend(int s, void *buf, unsigned short len)
{
    u32 param[3];

    param[0] = s;
    param[1] = (u32)buf;
    param[2] = len;
    return Deci2Call(-DECI2_CALL_EXSEND, param);
}

// FUNC 0x0028D8B0 sceDeci2ExReqSend
//   Deci2Call(-7, {s, dest}) - interrupt-context ReqSend.  No callers.
int sceDeci2ExReqSend(int s, char dest)
{
    u32 param[2];

    param[0] = s;
    param[1] = (int)(s8)dest;
    return Deci2Call(-DECI2_CALL_EXREQSEND, param);
}

// FUNC 0x0028D8E0 sceDeci2ExLock
//   Deci2Call(-8, {s}).  No callers.
int sceDeci2ExLock(int s)
{
    u32 param[1];

    param[0] = s;
    return Deci2Call(-DECI2_CALL_EXLOCK, param);
}

// FUNC 0x0028D908 sceDeci2ExUnLock
//   Deci2Call(-9, {s}).  No callers.
int sceDeci2ExUnLock(int s)
{
    u32 param[1];

    param[0] = s;
    return Deci2Call(-DECI2_CALL_EXUNLOCK, param);
}

// FUNC 0x0028D930 deci2_write
//   libkernl kPuts (the bootstrap's name): Deci2Call(0x10, {s}), a kernel
//   puts to the DECI2 console.  Only caller: deci2_sink (0x002872A0).  (The
//   binary passes Deci2Call's return value through in $v0; nothing reads it.)
void deci2_write(const char *s)
{
    u32 param[1];

    param[0] = (u32)s;
    Deci2Call(DECI2_CALL_KPUTS, param);
}
