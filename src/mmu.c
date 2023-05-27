#include <string.h>
#include "mmu.h"

#ifdef USING_DYNAMIC_ARRAY
    #define VRAM_DATA_(N)   mmu->vram.data[N & 0x1FFF]
    #define ERAM_DATA_(N)   mmu->eram.data[N & 0x1FFF]
    #define WRAM_DATA_(N)   mmu->wram.data[N & 0x1FFF]
#else
    #define VRAM_DATA_(N)   mmu->vram[N & 0x1FFF]
    #define ERAM_DATA_(N)   mmu->eram[N & 0x1FFF]
    #define WRAM_DATA_(N)   mmu->wram[N & 0x1FFF]
#endif

void mmu_reset (MMU * const mmu)
{
#ifdef FAST_ROM_READ
    vc_init (&mmu->rom, CART_MIN_SIZE_KB << 10);
#endif

#ifdef USING_DYNAMIC_ARRAY
    vc_init (&mmu->vram, VRAM_SIZE);
    vc_init (&mmu->eram, ERAM_SIZE);
    vc_init (&mmu->wram, WRAM_SIZE);
#else
    memset(mmu->vram, 0, VRAM_SIZE);
    memset(mmu->eram, 0, ERAM_SIZE);
    memset(mmu->wram, 0, WRAM_SIZE);
#endif

    mmu->inBios = 0;
    /* Set default IO register values */
    memset(mmu->io, 0, IO_HRAM_SIZE * sizeof(uint8_t));

    /* Interrupt request flags */
    mmu->io[0xF] = 0xE1;

    /* Display-related registers */
    mmu->io[IO_LCDControl] = 0x91;
    mmu->io[IO_LCDStatus]  = 0x85;
    mmu->io[IO_DMA]        = 0xFF;
    mmu->io[IO_BGPalette]  = 0xFC;

    /* Copy boot ROM */
    //memcpy(mmu->bios, testBootRom, sizeof(uint8_t) * 256);
}

void mmu_io_write (MMU * const mmu, uint8_t const addr, uint8_t const val)
{
    switch (addr) 
    {
        case IO_LineY: 
            /* Read Only*/
        break;
        case IO_DMA:
        {   /* DMA transfer to OAM memory */
            uint16_t srcAddr = (uint16_t)val << 8;
            int i;
            for (i = 0; i < 0x9F; i++)
                mmu->oam[i] = mmu_rb (mmu, srcAddr + i);
        }
        break;
        default:
            mmu->io[addr] = val;
    }
}

uint8_t mmu_rb (MMU * const mmu, uint16_t const addr) 
{
    /* Hardcode $FF44 for testing */
    //if (addr == 0xFF44) return 0x90;

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
#ifdef FAST_ROM_READ
                return mmu->rom.data[addr];
#else
                return mmu->rom_read(mmu->direct.ptr, addr);/* mmu->rom.data[addr]; */
#endif
        break;
        case 0x4 ... 0x7:        
        /* ROM bank 1 ... N */
#ifdef FAST_ROM_READ
            return mmu->rom.data[addr];
#else
            return mmu->rom_read(mmu->direct.ptr, addr);
#endif
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
            VRAM_DATA_(addr) = val;
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
                        mmu->oam[addr & 0x9F] = val;
                    }
                break;
                case 0xF:
                    if (addr >= 0xFF80) /* HRAM / IO */
                        mmu->hram[addr & 0x7F] = val;
                    else
                        mmu_io_write (mmu, addr & 0x7F, val);
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