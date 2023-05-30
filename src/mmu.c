#include <stdio.h>
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

#ifdef FAST_ROM_READ
    #define ROM_READ_(N)  mmu->rom.data[N];
#else
    #define ROM_READ_(N)  mmu->rom_read(mmu->direct.ptr, N);
#endif

void mmu_init (MMU * const mmu)
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
}

void mmu_boot_reset (MMU * const mmu)
{
    /* Set default IO register values */
    memset(mmu->io, 0, IO_HRAM_SIZE * sizeof(uint8_t));

    /* Interrupt request flags */
    mmu->io[IO_Joypad]     = 0xCF;
    mmu->io[IO_IntrFlags]  = 0xE1;

    /* Display-related registers */
    mmu->io[IO_LCDControl] = 0x91;
    mmu->io[IO_LCDStatus]  = 0x85;
    mmu->io[IO_DMA]        = 0xFF;
    mmu->io[IO_BGPalette]  = 0xFC;

    mmu->bank8 = 1;
}

inline uint8_t mbc_read (MMU * const mmu, uint16_t const addr)
{
    const uint8_t mbc = mmu->mbc & 0xF0;

    switch (mbc) {
        case MBC1:
            if (addr <= 0x3FFF) return ROM_READ_(addr); /* ROM bank X0      */
            if (addr <= 0x7FFF) {                       /* ROM bank 01 - 7F */
                const uint8_t masked = mmu->bank8 & (mmu->romBanks - 1);
                return ROM_READ_(addr + (masked - 1) * ROM_BANK_SIZE);
            }
            if (addr >= 0xA000 && addr <= 0xBFFF) {
                if (mmu->romBanks > 0x20) {             /* Over 512 KiB     */
                    return ERAM_DATA_(addr);
                }
                if (mmu->romBanks <= 0x20 && mmu->bankMode) { /* Under 512 KiB */
                    return 0; /* Select a RAM baank */
                }
            }
        default:
            return 0;
    }
}

inline void mbc_write (MMU * const mmu, uint16_t const addr, uint8_t const val)
{
    const uint8_t mbc = mmu->mbc & 0xF0;

    switch (mbc) {
        case NO_MBC: return; /* Nothing to write in carts without an MBC */
        case MBC1:
            if (addr <= 0x1FFF) {                        /* Enable RAM read/write         */
                mmu->ramEnable = (val == 0xA) ? 1 : 0; return; 
            }
            if (addr <= 0x3FFF) {                        /* ROM bank number, lower 5 bits */ 
                mmu->bank8 = val & 0x1F; 
                if (mmu->bank8 == 0) { mmu->bank8++; } return; 
            }
            if (addr <= 0x5FFF) {                        /* ROM bank number, upper 2 bits */ 
                mmu->bank8 = ((val & 3) << 5) | (mmu->bank8 & 0x1F); return;
            }
            if (addr <= 0x7FFF) { mmu->bankMode = val; } /* Banking mode, use ROM or RAM  */
            return;
    }
}

void mmu_io_write (MMU * const mmu, uint16_t const addr, uint8_t const val)
{
    const uint8_t io_addr = addr & 0x7F;

    switch (io_addr) 
    {
        case IO_Joypad:
        break;
        case IO_IntrFlags:
            mmu->io[IO_IntrFlags] = val & 0x1F;
        break;
        case IO_LineY: /* Read Only*/
        break;
        case IO_DMA:
        {   /* DMA transfer to OAM memory */
            uint16_t srcAddr = (uint16_t)val << 8;
            int i;
            for (i = 0; i < 0x9F; i++)
                mmu->oam[i] = mmu_readByte (mmu, srcAddr + i);
        }
        break;
        default:
            mmu->io[io_addr] = val;
    }
}

uint8_t mmu_readByte (MMU * const mmu, uint16_t const addr) 
{
#ifdef DEBUG_CPU_LOG
    /* Hardcode $FF44 for testing */
    if (addr == 0xFF44) return 0x90;
#endif
    if ((mmu->mbc >> 4) != 0)
    {
        printf("MBC read 1\n");
        if (addr <= 0x7FFF)                   return mbc_read(mmu, addr);
        if (addr >= 0xA000 && addr <= 0xBFFF) return mbc_read(mmu, addr);
    }

    /* Read 8-bit byte from a given address */
    if (addr <= 0x7FFF) return ROM_READ_(addr);       /* ROM banks        */
    if (addr <= 0x9FFF) return VRAM_DATA_(addr);      /* Video RAM        */
    if (addr <= 0xBFFF) return ERAM_DATA_(addr);      /* External RAM     */
    if (addr <= 0xDFFF) return WRAM_DATA_(addr);      /* Work RAM         */
    if (addr <= 0xFDFF) return 0xFF;                  /* Echo RAM         */
    if (addr <  0xFEA0) return mmu->oam[addr & 0x9F]; /* OAM              */
    if (addr <= 0xFEFF) return 0xFF;                  /* Not usable       */
    if (addr <= 0xFF7F) return mmu->io[addr & 0x7F];  /* I/O registers    */
    if (addr <  0xFFFF) return mmu->hram[addr & 0x7F];/* High RAM         */
    if (addr == 0xFFFF) return mmu->hram[0x7F];       /* Interrupt enable */

    return 0; 
}

uint16_t mmu_readWord (MMU * const mmu, uint16_t const addr)
{ 
    /* Read 16-bit word from a given address */ 
    return mmu_readByte (mmu, addr) + (mmu_readByte (mmu, addr + 1) << 8);
}

void mmu_writeByte (MMU * const mmu, uint16_t const addr, uint8_t val) 
{ 
    /* Write 8-bit byte to a given address */ 
    if (addr <= 0x7FFF) { mbc_write (mmu, addr, val); return; }    /* MBC write         */
    if (addr <= 0x9FFF) { VRAM_DATA_(addr) = val; return; }        /* Video RAM         */
    if (addr <= 0xBFFF) { ERAM_DATA_(addr) = val; return; }        /* External RAM      */
    if (addr <= 0xFDFF) { WRAM_DATA_(addr) = val; return; }        /* Work RAM / echo   */
    if (addr <  0xFEA0) { mmu->oam[addr & 0x9F] = val; return; }   /* Write to OAM      */
    if (addr <= 0xFEFF) return;                                    /* Not usable        */
    if (addr <= 0xFF7F) { mmu_io_write (mmu, addr, val); return; } /* I/O registers     */
    if (addr <  0xFFFF) mmu->hram[addr & 0x7F] = val;              /* High RAM          */
    if (addr == 0xFFFF) mmu->hram[0x7F] = val & 0x1F;              /* Interrupt enable  */
}

void mmu_writeWord (MMU * const mmu, uint16_t const addr, uint16_t val)
{
    /* Write 16-bit word to a given address */
    mmu_writeByte (mmu, addr, val);
    mmu_writeByte (mmu, addr + 1, val >> 8);
}