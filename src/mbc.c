#include <stdio.h>
#include "mbc.h"

struct MBC mbc;

void mbc_init (uint8_t * rom)
{
    //memcpy (mbc.rom, rom, 32768 * sizeof(uint8_t));
    mbc.rom = rom;

    const uint8_t cartType = mbc.rom[0x147];
    const uint8_t cartSize = mbc.rom[0x148];

    uint8_t header[80];
    memcpy (header, rom + 0x100, 80 * sizeof(uint8_t));

    printf ("GB: ROM file size (KiB): %d\n", 32 * (1 << cartSize));
    printf ("GB: Cart type: %02X\n", cartType);
}

uint8_t mbc_read (const uint16_t addr)
{
    //assert (mbc.rom[addr] != NULL);
    /* Default behavior, for testing */
    return mbc.rom[addr];
}

uint8_t mbc_write (const uint16_t addr, const uint8_t val)
{
    //assert (mbc.rom[addr] != NULL);
    /* Default behavior, for testing */
    mbc.rom[addr] = val;
    return 0;
}

uint8_t mbc_rw (const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (write) return mbc_write (addr, val);
    else return mbc_read(addr);
}

/* PPU read/write, temporary */

uint8_t ppu_rw (const uint16_t addr, const uint8_t val, const uint8_t write)
{
    return 0;
}
