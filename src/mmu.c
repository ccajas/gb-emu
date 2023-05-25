#include <string.h>
#include "mmu.h"

#ifdef USING_DYNAMIC_ARRAY_
    #define VRAM_DATA_(N)   mmu->vram.data[N & 0x1FFF]
    #define ERAM_DATA_(N)   mmu->eram.data[N & 0x1FFF]
    #define WRAM_DATA_(N)   mmu->wram.data[N & 0x1FFF]
#else
    #define VRAM_DATA_(N)   mmu->vram[N & 0x1FFF]
    #define ERAM_DATA_(N)   mmu->eram[N & 0x1FFF]
    #define WRAM_DATA_(N)   mmu->wram[N & 0x1FFF]
#endif

const uint8_t testBootRom[] =
{
    0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x4C,
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

void mmu_reset (MMU * const mmu)
{
#ifdef USING_DYNAMIC_ARRAY_
    vc_init (&mmu->vram, VRAM_SIZE);
    vc_init (&mmu->eram, ERAM_SIZE);
    vc_init (&mmu->wram, WRAM_SIZE);
#else
    memset(mmu->vram, 0, VRAM_SIZE);
    memset(mmu->eram, 0, ERAM_SIZE);
    memset(mmu->wram, 0, WRAM_SIZE);
#endif

    mmu->inBios = 0;
    mmu->hram[0x50] = 0; /* Boot ROM not read by default */

    /* Copy boot ROM */
    memcpy(mmu->bios, testBootRom, sizeof(uint8_t) * 256);
}

uint8_t mmu_rb (MMU * const mmu, uint16_t const addr) 
{
    /* Hardcode $FF44 for testing */
    if (addr == 0xFF44) return 0x90;

    /* Read 8-bit byte from a given address */
    switch (addr >> 12)
    {
        /* ROM bank 0 */
        case 0 ... 0x3:
            if (addr < 0x100 && !mmu->hram[0x50])
            {   /* Read boot ROM */
                return mmu->bios[addr];
            }
            else
                return mmu->rom_read(mmu->direct.ptr, addr);/* mmu->rom.data[addr]; */
        break;
        case 0x4 ... 0x7:        
        /* ROM bank 1 ... N */
            return mmu->rom_read(mmu->direct.ptr, addr);
        break;
        case 0x8: case 0x9:
        /* Video RAM */
            return VRAM_DATA_(addr);
        break;
        case 0xA: case 0xB:
        /* External RAM */
            return ERAM_DATA_(addr);
        break;
        case 0xC: case 0xD:
        /* Work RAM */
            return WRAM_DATA_(addr);
        break;
        case 0xE:
        /* Work RAM echo */
            return WRAM_DATA_(addr);
        break;
        case 0xF:
            switch ((addr & 0x0F00) >> 8)
		    {
                case 0 ... 0xD: /* Work RAM echo */
                    return WRAM_DATA_(addr);
                case 0xE:
                    if (addr < 0xFEA0)
                        /* return OAM */
                        return mmu->oam[addr & 0x9F];
                    else
                        return 0;
                case 0xF:
                    if (addr >= 0xFF80) /* HRAM */
                        return mmu->hram[addr & 0x7F];
                    else
                        /* IO registers */
                        return mmu->io[addr & 0x7F];
                default:
                    /* Prohibited area, if OAM is not blocked, actual value 
                        depends on hardware. Return the default 0xFF for now */
                    return 0xFF;
            }
        break;
        default:
            return 0;
    }
    return 0; 
}

uint16_t mmu_rw (MMU * const mmu, uint16_t const addr)
{ 
    /* Read 16-bit word from a given address */ 
    return mmu_rb (mmu, addr) + (mmu_rb (mmu, addr + 1) << 8);
}

void mmu_wb (MMU * const mmu, uint16_t const addr, uint8_t val) 
{ 
    /* Write 8-bit byte to a given address */ 
    switch (addr & 0xF000)
    {
        case 0x8000: case 0x9000:
        /* Video RAM */
        break;
        case 0xA000: case 0xB000:
        /* External RAM */
            ERAM_DATA_(addr) = val;
	    break;
        case 0xC000: case 0xD000:
        /* Work RAM and echo */
            WRAM_DATA_(addr) = val;
        break;
        case 0xE000:
            /* Echo RAM */
            WRAM_DATA_(addr) = val;
	    break;
        case 0xF000:
            switch ((addr & 0x0F00) >> 8)
		    {
                case 0 ... 0xD: /* Work RAM echo */
                    WRAM_DATA_(addr) = val;
                break;
                case 0xE:
                    if (addr < 0xFEA0) {
                        /* Write to OAM */
                    }
                break;
                case 0xF:
                    if (addr >= 0xFF80) /* HRAM / IO */
                        mmu->hram[addr & 0x7F] = val;
                    else
                        {}
                break;
            }
    }
}
void mmu_ww (MMU * const mmu, uint16_t const addr, uint16_t val)
{
    /* Write 16-bit word to a given address */
    mmu_wb (mmu, addr, val);
    mmu_wb (mmu, addr + 1, val >> 8);
}