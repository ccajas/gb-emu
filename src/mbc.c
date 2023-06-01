#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "mbc.h"

/* Define MBC used by the program */

struct MBC mbc;

uint8_t * mbc_load_rom (const char * fileName) 
{
    /* Load file from command line */
    FILE * f = fopen (fileName, "rb");

    fseek (f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fseek (f, 0, SEEK_SET);

    if (!f)
    {
        fclose (f);
        LOG_("Failed to load file \"%s\"\n", fileName);
        return NULL;
    }
    else
    {
        uint8_t * rom = calloc(size, sizeof (uint8_t));
        fread (rom, size, 1, f);
        fclose (f);

        /* Get cartridge type and MBC from header */
        const uint8_t cartType = rom[0x147];
        uint8_t mbcType = 0;

        switch (cartType) 
        {
            case 0:            mbcType = 0; break;
            case 0x1 ... 0x3:  mbcType = 1; break;
            case 0x5 ... 0x6:  mbcType = 2; break;
            case 0xF ... 0x13: mbcType = 3; break;
            default: 
                LOG_("GB: MBC not supported.\n"); return rom;
        }

        memcpy (mbc.header, rom + 0x100, 80 * sizeof(uint8_t));
        printf ("GB: ROM file size (KiB): %d\n", 32 * (1 << rom[0x148]));
        printf ("GB: Cart type: %02X\n", rom[0x147]);

        mbc.type = mbcType;
        mbc.romData = rom;

        /* Restart components */
        cpu_boot_reset();
        ppu_reset();
        cpu_state();

        return rom;
    }
}

uint8_t mbc_read (const uint16_t addr)
{
    //assert (mbc.rom[addr] != NULL);
    /* Default behavior, for testing */
    return mbc.romData[addr];
}

uint8_t mbc_write (const uint16_t addr, const uint8_t val)
{
    //assert (mbc.rom[addr] != NULL);
    /* Default behavior, for testing */
    mbc.romData[addr] = val;
    return 0;
}

uint8_t mbc_rw (const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (write) return mbc_write (addr, val);
    else return mbc_read(addr);
}