/* Runtime of the --trace build (tools/build.py): entry/exit lines for the
 * wrapped functions, written straight to the EE SIO transmit FIFO, which
 * PCSX2 logs as EE console output.  Self-contained on purpose: it must not
 * depend on the decompiled printf or kernel code it is used to debug. */
#include "types.h"

#define SIO_ISR    0x1000F130
#define SIO_TXFIFO 0x1000F180

static void tr_putc(char c)
{
    while (REG32(SIO_ISR) & 0x8000)
        ;
    *(volatile u8 *)SIO_TXFIFO = (u8)c;
}

static void tr_puts(const char *s)
{
    while (*s)
        tr_putc(*s++);
}

static void tr_hex(u32 v)
{
    int i;
    for (i = 28; i >= 0; i -= 4)
        tr_putc("0123456789abcdef"[(v >> i) & 15]);
}

void boot_trace_enter(const char *name, u32 a0, u32 a1, u32 a2)
{
    tr_puts("[trace] > ");
    tr_puts(name);
    tr_puts(" (");
    tr_hex(a0); tr_putc(' ');
    tr_hex(a1); tr_putc(' ');
    tr_hex(a2);
    tr_puts(")\n");
}

void boot_trace_exit(const char *name, u32 v0)
{
    tr_puts("[trace] < ");
    tr_puts(name);
    tr_puts(" = ");
    tr_hex(v0);
    tr_putc('\n');
}
