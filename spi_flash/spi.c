/*
Copyright (c) 2016, Joseph Tanen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* TODO Add error checking. */

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int size_t;
typedef signed int ssize_t;
#define NULL ((void *)0)
/* Note that this returns -1. */
#define ERROR(msg) do { uart_write_str(msg "\n\r"); return -1; } while (0)
#define INFO(msg) do { uart_write_str(msg "\n\r"); } while (0)

enum command {
    CMD_READ,
    CMD_AREAD,
    CMD_WRITE,
    CMD_ERASE_SECTOR,
    CMD_NONE
};
typedef void (*write_fn)(uint8_t c);

/* UART2 (ttyS1) is configured for no-FIFO polled operation. There are no interrupts, DMA,
 * FIFOS, etc., to worry about. We just wait for data to be ready and read it
 * or wait for the transmit buffer to be empty before writing. */
#define UART2_BASE (0xB0000C00)

#define UART_LSR_TEMT (1 << 6) /* TX empty */
#define UART_LSR_DR   (1 << 0) /* RX data ready */

#define SPI0_BASE (0xB0000B00)

#define SPI_STAT_BUSY   (1 << 0)
#define SPI_CTL_STARTWR (1 << 2)
#define SPI_CTL_STARTRD (1 << 1)
#define SPI_CTL_SPIENA  (1 << 0)

#define SPI_PAGE_SIZE   (256U)
#define SPI_SECTOR_SIZE (4096U)
#define SPI_START       (0x00000000u)
#define SPI_END         (0x01000000u)

/* Note that we always put stuff at 0xA1000000, so our effective size is much
 * less than the full 32MB. Also, uboot itself is at the end of RAM,
 * so this full RAM size isn't really safe. OH WELL DON'T BE STUPID. */
#define RAM_START       (0xA0000000U)
#define RAM_END         (0xA2000000U)

/* Page program. 1 256B page at a time. If the start address is after the
 * beginning of a page and you write past the end, it wraps to the beginning
 * of the page.
 */
#define SPI_CMD_PP   (0x02)
/* Read data. This is continuous. */
#define SPI_CMD_READ (0x03)
/* Disable write mode. */
#define SPI_CMD_WRDI (0x04)
/* Read status register. Continuous reads keep giving status. */
#define SPI_CMD_RDSR (0x05)
/* Enable write mode. */
#define SPI_CMD_WREN (0x06)
/* Erase 4K sector. */
#define SPI_CMD_SE   (0x20)
/* Read security register. */
#define SPI_CMD_RDSCUR (0x2B)
/* Clear SR fail flags in RDSCUR. */
#define SPI_CMD_CLSR (0x30)
/* Read block security regs. */
#define SPI_CMD_RDBLOCK (0x3C)
/* Read the device's manufacturer and device ID info. This info is 3 bytes. */
#define SPI_CMD_RDID (0x9F)
/* Continuous program. */
#define SPI_CMD_CP   (0xAD)

#define SPI_CMD_SR_WEL (1 << 1)
#define SPI_CMD_SR_WIP (1 << 0)

/* We need to make sure the compiler generates properly-aligned code by using
 * native data types in the struct at their native alignments.
 *
 * It may seem like __attribute__((__packed__)) is a good way to ensure that
 * the compiler does exactly what you want regarding alignment, but instead it
 * generates instruction streams that assume unaligned addresses, which don't
 * work properly with registers. Registers want to be read in 1 properly-formatted
 * op, e.g., a 32-bit read for a 32-bit register, unless specified otherwise.
 * Unaligned accesses are read byte-by-byte and reconstructed.
 */

struct uartregs {
    volatile uint32_t rbr; /* LSB only */
    volatile uint32_t tbr; /* LSB only */
    volatile uint32_t ier;
    volatile uint32_t iir;
    volatile uint32_t fcr;
    volatile uint32_t lcr;
    volatile uint32_t mcr;
    volatile uint32_t lsr;
    volatile uint32_t msr;
    volatile uint32_t scratch;
    volatile uint32_t dl;
    volatile uint32_t dllo;
    volatile uint32_t dlhi;
};

static struct uartregs *uart = (struct uartregs *)UART2_BASE;

/* SPI reads are weird on this platform. Many other platforms use the data register for
 * read/write, and even read-only operations are driven with do-not-care writes.
 * On this platform, however, you can only read or write. You need to either:
 *
 * 1) prime data for write, then set a write bit, or
 * 2) set a read bit, then read the data when it's ready.
 *
 * So just be aware of that.
 */

struct spiregs {
    volatile uint32_t stat;
    volatile uint32_t res[3];
    volatile uint32_t cfg;
    volatile uint32_t ctl;
    volatile uint32_t res2[2];
    volatile uint32_t data;
};

static struct spiregs *spi = (struct spiregs *)SPI0_BASE;

/* Command buffer.
 * TODO maybe stop using this global? */
static uint8_t cbuf[16];
/* Data buffer. Continue using this global so that you don't wreck your (possibly) small
 * stack. */
static uint8_t dbuf[4096];

static uint8_t
uart_read (void)
{
    while ((uart->lsr & UART_LSR_DR) == 0) { /* Nada */ }

    return uart->rbr & 0xFF;
}

static void
uart_wait (void)
{
    while ((uart->lsr & UART_LSR_TEMT) == 0) { /* Nada */ }
}

static void
uart_write (uint8_t data)
{
    uart_wait();

    uart->tbr = data;
}

static void
spi_wait (void)
{
    uint32_t busy;

    do {
        busy = spi->stat & SPI_STAT_BUSY;
    } while (busy != 0);
}

static void
spi_addr_cmd (uint8_t cmd, uint32_t addr, int write, uint8_t *buffer, int const len)
{
    int index;

    /* Set the chip select. It's active low. */
    spi->ctl &= ~SPI_CTL_SPIENA;

    /* Write the opcode. */
    spi->data = cmd;
    spi->ctl |= SPI_CTL_STARTWR;
    /* Wait until that's done. */
    spi_wait();

    /* Write the 3-byte address. */
    for (index = 0; index < 3; ++index) {
        /* Write the byte. MSB-first. */
        spi->data = (addr >> (8 * (2 - index))) & 0xFF;
        spi->ctl |= SPI_CTL_STARTWR;
        /* Wait until that's done. */
        spi_wait();
    }

    if (write) {
        for (index = 0; index < len; ++index) {
            /* Write the data. */
            spi->data = *(buffer++);
            spi->ctl |= SPI_CTL_STARTWR;
            /* Wait until that's done. */
            spi_wait();
        }
    } else {
        for (index = 0; index < len; ++index) {
            /* Start read data. */
            spi->ctl |= SPI_CTL_STARTRD;
            /* Wait for the byte to be done. */
            spi_wait();
            /* Read the data. */
            *(buffer++) = spi->data;
        }
    }

    /* Clear the chip select. */
    spi->ctl |= SPI_CTL_SPIENA;
}

static void
spi_cmd (uint8_t cmd, int write, uint8_t *buffer, int const len)
{
    int index;

    /* Set the chip select. It's active low. */
    spi->ctl &= ~SPI_CTL_SPIENA;

    /* Write the opcode. */
    spi->data = cmd;
    spi->ctl |= SPI_CTL_STARTWR;
    /* Wait until that's done. */
    spi_wait();

    /* If len > 0, read/write some stuff. If not, we just wanted to write the
     * command byte. */
    if (write) {
        for (index = 0; index < len; ++index) {
            /* Write the data. */
            spi->data = *(buffer++);
            spi->ctl |= SPI_CTL_STARTWR;
            /* Wait until that's done. */
            spi_wait();
        }
    } else {
        for (index = 0; index < len; ++index) {
            /* Start read data. */
            spi->ctl |= SPI_CTL_STARTRD;
            /* Wait for the byte to be done. */
            spi_wait();
            /* Read the data. */
            *(buffer++) = spi->data;
        }
    }

    /* Clear the chip select. */
    spi->ctl |= SPI_CTL_SPIENA;
}

static uint8_t
hextostr (uint8_t c)
{
    switch (c) {
        case 0x0: return '0';
        case 0x1: return '1';
        case 0x2: return '2';
        case 0x3: return '3';
        case 0x4: return '4';
        case 0x5: return '5';
        case 0x6: return '6';
        case 0x7: return '7';
        case 0x8: return '8';
        case 0x9: return '9';
        case 0xA: return 'A';
        case 0xB: return 'B';
        case 0xC: return 'C';
        case 0xD: return 'D';
        case 0xE: return 'E';
        case 0xF: return 'F';
        default: return 'x';
    }
}

static void
print_char(uint8_t c)
{
    uint8_t hi = (c >> 4) & 0xF;
    uint8_t lo = c & 0xF;

    hi = hextostr(hi);
    lo = hextostr(lo);

    uart_write(hi);
    uart_write(lo);
}

static void
uart_write_str (uint8_t *str)
{
    if (str == 0) return;
    while (*str != 0) {
        uart_write(*(str++));
    }
}

static void
print_int (int c)
{
    uint8_t buffer[9];
    int i;

    for (i = 0; i < 8; ++i) {
        buffer[7 - i] = hextostr(c & 0xF);
        c >>= 4;
    }
    buffer[8] = 0;

    uart_write_str(buffer);
}

static void
print_ptr (void *c)
{
    print_int((int)c);
}

void
delay (int const ticks)
{
    volatile int i = 0;

    while (++i < ticks) {}
}

void
enable_write (void)
{
    uint8_t buffer[1];

    spi_cmd(SPI_CMD_WREN, 0, 0, 0);
    do {
        spi_cmd(SPI_CMD_RDSR, 0, buffer, 1);
    } while (!(buffer[0] & SPI_CMD_SR_WEL));
}

/* Any address inside of a sector causes that sector to be erased. */
void
erase_sector (uint32_t addr)
{
    enable_write();
    spi_addr_cmd(SPI_CMD_SE, addr, 0, 0, 0);
    /* Datasheet says to wait for WIP then WEL. */
    do {
        spi_cmd(SPI_CMD_RDSR, 0, cbuf, 1);
    } while (cbuf[0] & SPI_CMD_SR_WIP);
    do {
        spi_cmd(SPI_CMD_RDSR, 0, cbuf, 1);
    } while (cbuf[0] & SPI_CMD_SR_WEL);

    // TODO Error check erase: spi_cmd(SPI_CMD_RDSCUR, 0, buffer, 1);
}

/* We're writing inside of a page. The write will wrap to the beginning of the
 * page if we write too much. We will error out if (addr + len) goes past a page
 * boundary.
 */
int
write_page (uint32_t addr, uint8_t *buffer, uint32_t len)
{
    uint32_t offset;
    uint32_t max;

    offset = addr % SPI_PAGE_SIZE;
    max = SPI_PAGE_SIZE - addr;
    if (len > max) ERROR("Too much to write to page.");

#if 0
#if 0
    print_ptr(buffer);
    uart_write_str("->");
    print_int(addr);
    uart_write('@');
    print_int(len);
    uart_write_str("\n\r");
#endif

    do {
        uint32_t i;
        for (i = 0; i < len; ++i) {
            print_char(buffer[i]);
        }
    } while (0);

    return 0;
#else
    enable_write();
    spi_addr_cmd(SPI_CMD_PP, addr, 1, buffer, len);
    /* Datasheet says to wait for WIP then WEL. */
    do {
        spi_cmd(SPI_CMD_RDSR, 0, cbuf, 1);
    } while (cbuf[0] & SPI_CMD_SR_WIP);
    do {
        spi_cmd(SPI_CMD_RDSR, 0, cbuf, 1);
    } while (cbuf[0] & SPI_CMD_SR_WEL);

    // TODO Error check write: spi_cmd(SPI_CMD_RDSCUR, 0, cbuf, 1);

    return 0;
#endif
}

static uint8_t
strtohex (char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    } else {
        return 0xFF;
    }
}

int
atoui (char const *in, uint32_t *out)
{
    int i;

    if (in == NULL) return -1;
    if (out == NULL) return -1;

    *out = 0;
    for (i = 0; (i < sizeof(*out) * 2) && (*in != 0); ++i, ++in) {
        uint8_t val;

        val = strtohex(*in);
        if (val == 0xFF) return -1;
        *out <<= 4;
        *out += val;
    }
    return 0;
}

int
strncmp (char const *a, char const *b, size_t count)
{
    if (a == NULL || b == NULL) return 0;

    while (count > 0) {
        if (*a == 0 && *b == 0) break;
        else if (*a < *b) return -1;
        else if (*a > *b) return 1;
        a++;
        b++;
        count--;
    }

    return 0;
}

int
meat (int argc, char *argv[])
{
    int i;
    int rv;
    enum command cmd = CMD_NONE;
    uint32_t addr;
    uint32_t end;

    uart_write_str("argc: ");
    print_int(argc);
    uart_write_str(" argv: ");
    print_ptr(argv);
    uart_write_str(":");
    for (i = 0; i < argc; ++i) {
        uint32_t val;

        uart_write(' ');
        uart_write_str(argv[i]);
        uart_write(':');
        rv = atoui(argv[i], &val);
        if (rv == 0) {
            print_int(val);
        } else {
            uart_write_str("NOTNUM");
        }
    }
    uart_write_str("\n\r");

    if (argc < 2) {
        /* Nada. */
    } else if (0 == strncmp(argv[1], "read", sizeof("read"))) {
        cmd = CMD_READ;
    } else if (0 == strncmp(argv[1], "aread", sizeof("aread"))) {
        cmd = CMD_AREAD;
    } else if (0 == strncmp(argv[1], "write", sizeof("write"))) {
        cmd = CMD_WRITE;
    } else if (0 == strncmp(argv[1], "erase", sizeof("erase"))) {
        cmd = CMD_ERASE_SECTOR;
    }

    if (cmd == CMD_READ || cmd == CMD_AREAD) {
        int index;
        write_fn wr;

        if (cmd == CMD_READ) {
            wr = uart_write;
        } else {
            wr = print_char;
        }
        if (argc < 4) ERROR("Not enough read args.");
        rv = atoui(argv[2], &addr);
        if (rv != 0 || addr >= SPI_END) ERROR("Bad read addr.");
        rv = atoui(argv[3], &end);
        end += addr;
        if (rv != 0 || end > SPI_END) ERROR("Bad read size.");
        INFO("Reading...");

        uart_write_str("\n\rJOEJOEJOE\n\r");
        for ( ; addr < end; addr += sizeof(dbuf)) {
            int size = end - addr;
            size = (size > sizeof(dbuf)) ? sizeof(dbuf) : size;

            spi_addr_cmd(SPI_CMD_READ, addr, 0, dbuf, size);
            for (index = 0; index < size; ++index) {
                (*wr)(dbuf[index]);
            }
        }
        uart_write_str("\n\rJOEJOEJOE\n\r");
        uart_wait();
    } else if (cmd == CMD_WRITE) {
        uint32_t ram;
        uint32_t ram_end;
        uint32_t size;
        uint32_t towrite;

        if (argc < 5) ERROR("Not enough write args.");
        rv = atoui(argv[2], &ram);
        if (rv != 0 || ram < RAM_START || ram >= RAM_END) ERROR("Bad writefrom addr.");
        rv = atoui(argv[3], &addr);
        if (rv != 0 || addr >= SPI_END) ERROR("Bad writeto addr.");
        rv = atoui(argv[4], &size);
        ram_end = size;
        ram_end += ram;
        end = addr + size;
        if (rv != 0 || ram_end > RAM_END || end > SPI_END) ERROR("Bad write size.");
        INFO("Writing...");

        /* Write pages. If the start address is in the middle of a page, don't
         * write bigger than the page. */
        towrite = SPI_PAGE_SIZE - addr % SPI_PAGE_SIZE;
        towrite = (towrite > size) ? size : towrite;

        /* After the first one, we're page-aligned. */
        while (size > 0) {
            print_int(ram);
            uart_write(' ');
            print_int(addr);
            uart_write(' ');
            print_int(towrite);
            uart_write_str("\n\r");

            rv = write_page(addr, (uint8_t *)ram, towrite);
            if (0 != rv) ERROR("Failed to write page.");

            size -= towrite;
            ram += towrite;
            addr += towrite;
            towrite = (size < SPI_PAGE_SIZE) ? size : SPI_PAGE_SIZE;
        }
    } else if (cmd == CMD_ERASE_SECTOR) {
        uint32_t sector_addr;

        if (argc < 3) ERROR("Not enough erase args.");
        rv = atoui(argv[2], &addr);
        if (rv != 0 || addr >= SPI_END) ERROR("Bad erase addr.");
        sector_addr = addr;
        sector_addr -= addr % SPI_SECTOR_SIZE;
        INFO("Erasing sector...");
        print_int(addr);
        uart_write(' ');
        print_int(sector_addr);
        uart_write_str("\n\r");

        erase_sector(sector_addr);
    }

    return 0;
}

