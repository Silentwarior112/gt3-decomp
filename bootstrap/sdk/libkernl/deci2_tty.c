/* deci2_tty.c - Sony libkernl 2.2.4 (PsIIlibkernl2240), 0x01022F78 .. 0x01023678:
 *   - deci2.o  EE kernel DECI2 glue (sceDeci2Open .. sceDeci2ExUnLock, kPuts),
 *              all funnelled through the Deci2Call syscall (0x7C)
 *   - tty.o    DECI2 TTYP (protocol 0x210) console driver backing the newlib
 *              write()/read() glue in initsys.c: EE node 'E' <-> host 'H'.
 *
 * The only path the bootstrap actually runs is _deci2_tty_init, via
 * sceSifSyncIop -> tty_reinit_after_iop_reset; whether sceDeci2Open succeeds on
 * a retail kernel without a DECI2 host is not evidenced here (? runtime).
 * Nothing in the bootstrap calls write()/read() (Polyphony's printf goes
 * printf -> PrintTarget::flush -> tty_puts -> scePrintf 0x0101D2A8 ->
 * _kputs_putc 0x0101C9C8 -> kPuts, i.e. Deci2Call 0x10, not the TTYP driver).
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "kernl_rest_internal.h"

/* DECI2 handler events (Sony deci2.h). */
#define DECI2_READ       1
#define DECI2_READDONE   2
#define DECI2_WRITE      3
#define DECI2_WRITEDONE  4

#define DECI2_PROTO_TTYP 0x210
#define DECI2_NODE_EE    'E'
#define DECI2_NODE_HOST  'H'

/* DECI2 packet header. */
typedef struct {
    u16 length;         /* +0x00 total length incl. headers */
    u16 reserved;       /* +0x02 */
    u16 protocol;       /* +0x04 */
    u8  source;         /* +0x06 */
    u8  destination;    /* +0x07 */
} sceDeci2Hdr;          /* 0x08 */

/* TTYP packet: DECI2 header + 4-byte TTYP sub-header + text. */
typedef struct {
    sceDeci2Hdr h;      /* +0x00 */
    u32  ttyp_info;     /* +0x08 always 0 */
    u8   data[0x100];   /* +0x0C at most 0x100 payload bytes per packet */
} TtyPacket;            /* 0x10C */

/* Console input ring (filled by the handler, drained by read()). */
typedef struct {
    int           size;         /* +0x00 capacity (0x100) */
    volatile int  count;        /* +0x04 bytes available (read() spins on it) */
    u8           *rp;           /* +0x08 read pointer */
    u8           *wp;           /* +0x0C write pointer */
    u8            data[0x100];  /* +0x10 */
} TtyRing;                      /* 0x110 */

/* TTY state - passed to sceDeci2Open as the handler's `opt`. */
/* Volatility per field, as the binary shows it: sd and wlen are re-read
 * right after being stored / tested (_deci2_tty_init, _deci2_tty_write,
 * the WRITEDONE event), busy is the flag the writer polls, and no access to
 * these or rlen is ever moved into a branch delay slot (egcs never does
 * that with a volatile access); the handler's receive loop reuses a loaded
 * rbuf and a loaded wbuf sits in a delay slot, so the pointers are not.    */
typedef struct {
    volatile int sd;    /* +0x00 DECI2 socket from sceDeci2Open */
    volatile int wlen;  /* +0x04 bytes of the current packet still to send */
    volatile int rlen;  /* +0x08 bytes of the incoming packet received so far */
    volatile int busy;  /* +0x0C send in flight; cleared by the handler */
    u8      *wbuf;      /* +0x10 send cursor (uncached mirror of s_tty_sendbuf) */
    u8      *rbuf;      /* +0x14 receive buffer (uncached mirror of s_tty_recvbuf) */
    TtyRing *ring;      /* +0x18 */
} TtyCtx;               /* 0x1C */

_Static_assert(sizeof(TtyRing) == 0x110, "TtyRing");
_Static_assert(sizeof(TtyCtx) == 0x1C, "TtyCtx");
_Static_assert(offsetof(TtyPacket, data) == 0x0C, "TtyPacket");

/* tty.o calls sceDeci2Poll as a void function (Sony's deci2.h declares it
 * void; eekernel.h here says int): the call then does not set $v0, which
 * changes the register the polling loop uses.  Under the matching compiler
 * the call goes through a void-returning view of the same function; GCC 14
 * just discards the int result.                                            */
#ifdef BOOT_OLD_GCC
#define TTY_POLL(s)  (((void (*)(int))sceDeci2Poll)(s))
#else
#define TTY_POLL(s)  ((void)sceDeci2Poll(s))
#endif

/* Kernel-side status word(s) handed to every sceDeci2Open (uncached).
 * ? size: nothing else touches 0x0104D8E8..0x0104D93F.                     */
static u32 s_deci2_kstatus[4];                                   /* 0x0104D8E8 .bss */
static TtyRing s_tty_ring __attribute__((aligned(64)));          /* 0x0104D940 .bss (? alignment) */
static TtyCtx s_tty;                                             /* 0x0104DA50 .bss */
static u8 s_tty_sendbuf[0x140] __attribute__((aligned(64)));     /* 0x0104DA80 .bss */
static u8 s_tty_recvbuf[0x140] __attribute__((aligned(64)));     /* 0x0104DBC0 .bss */

/* ------------------------------------------------------------------------ */
/* deci2.o - Deci2Call glue                                                  */
/* ------------------------------------------------------------------------ */

// FUNC 0x01022F78 sceDeci2Open
//   Deci2Call(1, {protocol, opt, handler, uncached &kernel status}).
//   Returns the socket (>= 0) or a negative error.
int sceDeci2Open(unsigned short protocol, void *opt,
                 void (*handler)(int event, int param, void *opt))
{
    u32 param[4];

    param[0] = protocol;
    param[1] = (u32)opt;
    param[2] = (u32)handler;
    param[3] = (u32)UNCACHED(s_deci2_kstatus);
    return Deci2Call(1, param);
}

// FUNC 0x01022FC0 sceDeci2Close
//   Deci2Call(2, {s}).  No callers.
int sceDeci2Close(int s)
{
    u32 param[1];

    param[0] = s;
    return Deci2Call(2, param);
}

// FUNC 0x01022FE8 sceDeci2ReqSend
//   Deci2Call(3, {s, dest}) - request a send slot to node `dest` (sign-
//   extended byte); the handler then gets DECI2_WRITE events.
int sceDeci2ReqSend(int s, char dest)
{
    u32 param[2];

    param[0] = s;
    param[1] = (int)(s8)dest;
    return Deci2Call(3, param);
}

// FUNC 0x01023018 sceDeci2Poll
//   Deci2Call(4, {s}) - pump the DECI2 manager (delivers handler events).
int sceDeci2Poll(int s)
{
    u32 param[1];

    param[0] = s;
    return Deci2Call(4, param);
}

// FUNC 0x01023040 sceDeci2ExRecv
//   Deci2Call(-5, {s, buf, len}) - receive from inside the handler.
int sceDeci2ExRecv(int s, void *buf, unsigned short len)
{
    u32 param[3];

    param[0] = s;
    param[1] = (u32)buf;
    param[2] = len;
    return Deci2Call(-5, param);
}

// FUNC 0x01023078 sceDeci2ExSend
//   Deci2Call(-6, {s, buf, len}) - send from inside the handler.
int sceDeci2ExSend(int s, void *buf, unsigned short len)
{
    u32 param[3];

    param[0] = s;
    param[1] = (u32)buf;
    param[2] = len;
    return Deci2Call(-6, param);
}

// FUNC 0x010230B0 sceDeci2ExReqSend
//   Deci2Call(-7, {s, dest}) - interrupt-context ReqSend.  No callers.
int sceDeci2ExReqSend(int s, char dest)
{
    u32 param[2];

    param[0] = s;
    param[1] = (int)(s8)dest;
    return Deci2Call(-7, param);
}

// FUNC 0x010230E0 sceDeci2ExLock
//   Deci2Call(-8, {s}).  No callers.
int sceDeci2ExLock(int s)
{
    u32 param[1];

    param[0] = s;
    return Deci2Call(-8, param);
}

// FUNC 0x01023108 sceDeci2ExUnLock
//   Deci2Call(-9, {s}).  No callers.
int sceDeci2ExUnLock(int s)
{
    u32 param[1];

    param[0] = s;
    return Deci2Call(-9, param);
}

// FUNC 0x01023130 kPuts
//   Deci2Call(0x10, {s}): kernel puts to the DECI2 console.  Used by the
//   libkernl printf line-buffer flusher F_0101C9C8.  (The binary passes
//   Deci2Call's return value through in $v0; eekernel.h declares it void and
//   the only caller ignores it.)
void kPuts(const char *s)
{
    u32 param[1];

    param[0] = (u32)s;
    Deci2Call(0x10, param);
}

/* ------------------------------------------------------------------------ */
/* tty.o - DECI2 TTYP console                                               */
/* ------------------------------------------------------------------------ */

// FUNC 0x01023158 tty_ring_init
//   Reset the (single, static) input ring: size, empty, rp = wp = data.
static TtyRing *tty_ring_init(int size)
{
    TtyRing *q = &s_tty_ring;

    q->size  = size;
    q->count = 0;
    q->wp    = q->data;
    q->rp    = q->data;
    return q;
}

// FUNC 0x01023180 tty_ring_put
//   Commit one byte the caller already stored at *wp: count++, advance wp
//   with wrap-around.  No overflow check.
static void tty_ring_put(TtyRing *q)
{
    q->count++;
    q->wp++;
    if (q->wp == q->data + q->size)
        q->wp = q->data;
}

// FUNC 0x010231C0 tty_ring_get
//   Consume the byte the caller read from *rp: count--, advance rp with wrap.
static void tty_ring_get(TtyRing *q)
{
    q->count--;
    q->rp++;
    if (q->rp == q->data + q->size)
        q->rp = q->data;
}

// FUNC 0x01023200 deci2_tty_handler
//   DECI2 event callback (opt = &s_tty), runs inside the DECI2 manager:
//   WRITE: push the pending packet with sceDeci2ExSend and advance the cursor
//          (on error print and drop the send: busy = 0);
//   WRITEDONE: complain if bytes are left, clear busy (releases the writer);
//   READ/READDONE with len != 0: append len bytes to the receive buffer
//          (bound 0x140 only warned about - the receive still happens);
//   READ/READDONE with len == 0: packet complete - push its payload (bytes
//          12 .. u16 length) into the input ring, reset the receive count.
static void deci2_tty_handler(int event, int param, void *opt)
{
    TtyCtx *ctx = opt;
    u8 *pkt;
    int r;
    int i;      /* receive count, then the packet byte index */

    switch (event) {
    case DECI2_READ:
    case DECI2_READDONE:
        if (param != 0) {
            if (!((unsigned int)(ctx->rlen + param) < 0x141))
                _printf("TTY: packet size larger than expect\n");
            i = sceDeci2ExRecv(ctx->sd, ctx->rbuf + ctx->rlen, (unsigned short)param);
            if (i < 0)
                _printf("TTY: receive error");      /* no newline in the binary */
            ctx->rlen += i;                         /* sic: also on error */
        } else {
            pkt = ctx->rbuf;
            for (i = 12; i < *(u16 *)pkt; i++) {
                *ctx->ring->wp = ctx->rbuf[i];
                tty_ring_put(ctx->ring);
            }
            ctx->rlen = 0;
        }
        break;

    case DECI2_WRITE:
        r = sceDeci2ExSend(ctx->sd, ctx->wbuf, (unsigned short)ctx->wlen);
        if (r < 0) {
            _printf("TTY: send err %d\n", r);
            ctx->busy = 0;
            break;
        }
        ctx->wbuf += r;
        ctx->wlen -= r;
        break;

    case DECI2_WRITEDONE:
        if (ctx->wlen != 0)
            _printf("TTY: err ti->wlen=%08x\n", ctx->wlen);
        ctx->busy = 0;
        break;
    }
}

// FUNC 0x01023398 _deci2_tty_write
//   Send up to 0x100 bytes (after LF -> CR LF expansion) as one TTYP packet
//   to the host and wait for it to go out: interrupts off (DIntr, result
//   ignored), busy = 1, fill the uncached packet, set its length, ReqSend to
//   the header's destination ('H'), pump sceDeci2Poll until the handler
//   clears busy, then EIntr (unconditionally - the previous interrupt state is
//   not restored).  Returns the number of SOURCE bytes consumed; -1 if a send
//   is already in flight or ReqSend fails.  An LF whose CR filled the last
//   slot is not consumed (the CR is still sent).
int _deci2_tty_write(const char *buf, int len)
{
    TtyPacket *pkt;
    u8 *dst;
    int n = 0;          /* payload bytes (incl. inserted CRs) */
    int consumed = 0;   /* source bytes taken */

    if (s_tty.busy != 0)
        return -1;
    DIntr();
    pkt = UNCACHED(s_tty_sendbuf);
    s_tty.busy = 1;
    s_tty.wbuf = (u8 *)pkt;
    dst = pkt->data;
    while (--len != -1) {
        if (*buf == '\n') {
            *dst++ = '\r';
            if (++n >= 0x100)
                break;
        }
        *dst++ = *buf++;
        n++;
        consumed++;
        if (n >= 0x100)
            break;
    }
    s_tty.wlen = n + 12;
    pkt->h.length = s_tty.wlen;
    if (sceDeci2ReqSend(s_tty.sd, (char)pkt->h.destination) < 0) {
        *(int *)&s_tty.busy = 0;    /* a plain store in the binary (it sits in
                                       EIntr's delay slot) */
        EIntr();
        return -1;
    }
    while (s_tty.busy != 0)
        TTY_POLL(s_tty.sd);
    EIntr();
    return consumed;
}

// FUNC 0x010234E8 _deci2_tty_read
//   Blocking line read from the host console: for each byte spin until the
//   input ring is non-empty, take one byte; stop after storing LF or CR
//   (returning the count including it) or when len bytes were read.
//   len <= 0 returns 0.
int _deci2_tty_read(char *buf, int len)
{
    char *p;
    int n;

    for (n = 0; n < len; n++) {
        p = buf + n;
        while (s_tty.ring->count == 0)
            ;
        *p = *s_tty.ring->rp;
        tty_ring_get(s_tty.ring);
        if (*p == '\n' || *p == '\r')
            return n + 1;
    }
    return n;
}

// FUNC 0x010235B8 _deci2_tty_init
//   Open the DECI2 TTYP console: FlushCache(0) (the buffers are then used
//   through the uncached mirror), sceDeci2Open(0x210, &s_tty, handler);
//   on failure return 0.  Else reset the state, point wbuf/rbuf at the
//   uncached send/receive buffers, pre-build the packet header
//   {len, 0, 0x210, 'E', 'H'} + TTYP word 0, and set up the 0x100-byte
//   input ring.  Returns 1.
int _deci2_tty_init(void)
{
    TtyPacket *pkt;

    FlushCache(WRITEBACK_DCACHE);
    s_tty.sd = sceDeci2Open(DECI2_PROTO_TTYP, (void *)&s_tty, deci2_tty_handler);
    if (s_tty.sd < 0)
        return 0;
    s_tty.busy = 0;
    s_tty.wlen = 0;
    s_tty.rlen = 0;
    s_tty.rbuf = UNCACHED(s_tty_recvbuf);
    pkt = UNCACHED(s_tty_sendbuf);
    s_tty.wbuf = (u8 *)pkt;
    pkt->h.protocol    = DECI2_PROTO_TTYP;
    pkt->h.source      = DECI2_NODE_EE;
    pkt->h.destination = DECI2_NODE_HOST;
    pkt->h.reserved    = 0;
    pkt->ttyp_info     = 0;
    s_tty.ring = tty_ring_init(0x100);
    return 1;
}
