/* tty.c - Sony libkernl 2240 tty.o, core
 * 0x00286D48..0x00287267: the DECI2 TTYP (protocol 0x210) console driver
 * behind the newlib write()/read() glue (write 0x00286180 calls
 * _deci2_tty_write, read 0x00286200 _deci2_tty_read; both open it with
 * _deci2_tty_init): EE node 'E' <-> host 'H'.  Same machine code as the
 * tty.o half of decomp/bootstrap/sdk/libkernl/deci2_tty.c; in the core it is
 * linked right after thread.o (the deci2.o glue it calls is at 0x0028D778..).
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part1_internal.h"

/* tty.o calls sceDeci2Poll as a void function (Sony's deci2.h declares it
 * void; G007_stdio.h / eekernel.h say int): the call then does not set $v0,
 * so _deci2_tty_write's polling loop reloads busy into $v0 (0x0028709C).
 * Under egcs the call goes through a void-returning view of the same
 * function, as in the bootstrap's deci2_tty.c; GCC 14 just discards the int
 * result.                                                                  */
#ifdef BOOT_OLD_GCC
#define TTY_POLL(s)  (((void (*)(int))sceDeci2Poll)(s))
#else
#define TTY_POLL(s)  ((void)sceDeci2Poll(s))
#endif

/* tty.o's data: file statics, the object's .bss in this order (between
 * thread.o's and the printf object's statics in the original).  The packet
 * buffers are DECI2 DMA buffers, used through their uncached mirrors: 64-byte
 * aligned (the section's alignment, which also puts s_tty_ring at
 * 0x004ADA80 after thread.o's .bss ends at 0x004ADA70). */
static TtyRing s_tty_ring;                                          /* 0x004ADA80 */
static TtyCtx  s_tty;                                               /* 0x004ADB90 */
static u8      s_tty_sendbuf[TTY_BUF_SIZE] __attribute__((aligned(64)));  /* 0x004ADBC0 */
static u8      s_tty_recvbuf[TTY_BUF_SIZE] __attribute__((aligned(64)));  /* 0x004ADD00 */

// FUNC 0x00286D48 tty_ring_init
//   Reset the (single, static) input ring: size, empty, rp = wp = data.
static TtyRing *tty_ring_init(int size)
{
    TtyRing *ring = &s_tty_ring;

    ring->size  = size;
    ring->count = 0;
    ring->wp    = ring->data;
    ring->rp    = ring->data;
    return ring;
}

// FUNC 0x00286D70 tty_ring_put
//   Commit one byte the caller already stored at *wp: count++, advance wp
//   with wrap-around.  No overflow check.
static void tty_ring_put(TtyRing *ring)
{
    ring->count++;
    ring->wp++;
    if (ring->wp == ring->data + ring->size)
        ring->wp = ring->data;
}

// FUNC 0x00286DB0 tty_ring_get
//   Consume the byte the caller read from *rp: count--, advance rp with wrap.
static void tty_ring_get(TtyRing *ring)
{
    ring->count--;
    ring->rp++;
    if (ring->rp == ring->data + ring->size)
        ring->rp = ring->data;
}

// FUNC 0x00286DF0 deci2_tty_handler
//   DECI2 event callback (opt = &s_tty), runs inside the DECI2 manager:
//   WRITE: push the pending packet with sceDeci2ExSend and advance the cursor
//          (on error print and drop the send: busy = 0);
//   WRITEDONE: complain if bytes are left, clear busy (releases the writer);
//   READ/READDONE with len != 0: append len bytes to the receive buffer
//          (bound TTY_BUF_SIZE only warned about - the receive still happens);
//   READ/READDONE with len == 0: packet complete - push its payload (bytes
//          TTY_HDR_SIZE .. u16 length) into the input ring, reset the
//          receive count.
//   Other events (<= 0, > 4) are ignored.
static void deci2_tty_handler(int event, int param, void *opt)
{
    TtyCtx *ctx = opt;
    u8 *packet;
    int sent;
    int n;      /* bytes received, then the packet byte index */

    switch (event) {
    case DECI2_READ:
    case DECI2_READDONE:
        if (param != 0) {
            if (!((unsigned int)(ctx->rlen + param) < TTY_BUF_SIZE + 1))
                sce_printf_sio("TTY: packet size larger than expect\n");
            n = sceDeci2ExRecv(ctx->sd, ctx->rbuf + ctx->rlen, (unsigned short)param);
            if (n < 0)
                sce_printf_sio("TTY: receive error");     /* no newline in the binary */
            ctx->rlen += n;                                /* sic: also on error */
        } else {
            packet = ctx->rbuf;
            for (n = TTY_HDR_SIZE; n < ((sceDeci2Hdr *)packet)->length; n++) {
                *ctx->ring->wp = ctx->rbuf[n];
                tty_ring_put(ctx->ring);
            }
            ctx->rlen = 0;
        }
        break;

    case DECI2_WRITE:
        sent = sceDeci2ExSend(ctx->sd, ctx->wbuf, (unsigned short)ctx->wlen);
        if (sent < 0) {
            sce_printf_sio("TTY: send err %d\n", sent);
            ctx->busy = 0;
            break;
        }
        ctx->wbuf += sent;
        ctx->wlen -= sent;
        break;

    case DECI2_WRITEDONE:
        if (ctx->wlen != 0)
            sce_printf_sio("TTY: err ti->wlen=%08x\n", ctx->wlen);
        ctx->busy = 0;
        break;
    }
}

// FUNC 0x00286F88 _deci2_tty_write
//   (Unnamed in index.txt; the bootstrap's _deci2_tty_write 0x01023398, same
//   size and the same code read against listing.txt.)  Send up to
//   TTY_MAX_PAYLOAD bytes (after LF -> CR LF expansion) as one TTYP packet to
//   the host and wait for it to go out: interrupts off (DIntr, result
//   ignored), busy = 1, fill the uncached packet, set its length, ReqSend to
//   the header's destination ('H'), pump sceDeci2Poll until the handler
//   clears busy, then EIntr (unconditionally - the previous interrupt state
//   is not restored).  Returns the number of SOURCE bytes consumed; -1 if a
//   send is already in flight or ReqSend fails.  An LF whose CR filled the
//   last slot is not consumed (the CR is still sent).  Caller: newlib write
//   (0x00286180).
int _deci2_tty_write(const char *buf, int len)
{
    TtyPacket *packet;
    u8 *dst;
    int payload = 0;    /* payload bytes (incl. inserted CRs) */
    int consumed = 0;   /* source bytes taken */

    if (s_tty.busy != 0)
        return -1;
    DIntr();
    packet = UNCACHED(s_tty_sendbuf);
    s_tty.busy = 1;
    s_tty.wbuf = (u8 *)packet;
    dst = packet->data;
    while (--len != -1) {
        if (*buf == '\n') {
            *dst++ = '\r';
            if (++payload >= TTY_MAX_PAYLOAD)
                break;
        }
        *dst++ = *buf++;
        payload++;
        consumed++;
        if (payload >= TTY_MAX_PAYLOAD)
            break;
    }
    s_tty.wlen = payload + TTY_HDR_SIZE;
    packet->h.length = s_tty.wlen;
    if (sceDeci2ReqSend(s_tty.sd, (char)packet->h.destination) < 0) {
        *(int *)&s_tty.busy = 0;    /* a plain store in the binary (EIntr's delay slot) */
        EIntr();
        return -1;
    }
    while (s_tty.busy != 0)
        TTY_POLL(s_tty.sd);  /* implicit: sceDeci2Poll (macro) */
    EIntr();
    return consumed;
}

// FUNC 0x002870D8 _deci2_tty_read
//   Blocking line read from the host console: for each byte spin until the
//   input ring is non-empty, take one byte; stop after storing LF or CR
//   (returning the count including it) or when len bytes were read.
//   len <= 0 returns 0.  Caller: newlib read (0x00286200).
int _deci2_tty_read(char *buf, int len)
{
    char *dst;
    int n;

    for (n = 0; n < len; n++) {
        dst = buf + n;
        while (s_tty.ring->count == 0)
            ;
        *dst = *s_tty.ring->rp;
        tty_ring_get(s_tty.ring);
        if (*dst == '\n' || *dst == '\r')
            return n + 1;
    }
    return n;
}

// FUNC 0x002871A8 _deci2_tty_init
//   Open the DECI2 TTYP console: FlushCache(0) (the buffers are then used
//   through the uncached mirror), sceDeci2Open(0x210, &s_tty, handler);
//   on failure return 0.  Else reset the state, point wbuf/rbuf at the
//   uncached send/receive buffers, pre-build the packet header
//   {len, 0, 0x210, 'E', 'H'} + TTYP word 0, and set up the
//   TTY_RING_SIZE-byte input ring.  Returns 1.  Callers: newlib write / read
//   (on their first call).
int _deci2_tty_init(void)
{
    TtyPacket *packet;

    FlushCache(WRITEBACK_DCACHE);
    s_tty.sd = sceDeci2Open(DECI2_PROTO_TTYP, (void *)&s_tty, deci2_tty_handler);
    if (s_tty.sd < 0)
        return 0;
    s_tty.busy = 0;
    s_tty.wlen = 0;
    s_tty.rlen = 0;
    s_tty.rbuf = UNCACHED(s_tty_recvbuf);
    packet = UNCACHED(s_tty_sendbuf);
    s_tty.wbuf = (u8 *)packet;
    packet->h.protocol    = DECI2_PROTO_TTYP;
    packet->h.source      = DECI2_NODE_EE;
    packet->h.destination = DECI2_NODE_HOST;
    packet->h.reserved    = 0;
    packet->ttyp_info     = 0;
    s_tty.ring = tty_ring_init(TTY_RING_SIZE);
    return 1;
}
