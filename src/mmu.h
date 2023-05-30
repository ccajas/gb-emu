#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "utils/v_array.h"
#include "cart.h"
#include "io.h"

#define CART_MIN_SIZE_KB  32

#define USING_DYNAMIC_ARRAY__
#define FAST_ROM_READ__

typedef struct MMU_struct
{
    uint8_t bios[256];
    uint8_t inBios;
    uint8_t cartType;

    enum {
        VRAM_SIZE     = 0x2000, /* Video RAM*/
        ERAM_SIZE     = 0x2000, /* External RAM */
        WRAM_SIZE     = 0x2000, /* Work RAM */
        OAM_SIZE      = 0xA0,
        IO_HRAM_SIZE  = 0x80, /* I/O registers/HRAM */
        ROM_BANK_SIZE = 0x4000 
    }
    ramSizes;

#ifdef USING_DYNAMIC_ARRAY
    struct VArray vram, eram, wram;
#else
    uint8_t vram[VRAM_SIZE];
    uint8_t eram[ERAM_SIZE];
    uint8_t wram[WRAM_SIZE];
#endif
    uint8_t oam[OAM_SIZE];
    uint8_t hram[0x80];

    /* MBC type and information */
    uint8_t mbc;
    uint16_t romBanks;

    /* I/O registers */
    uint8_t io[0x80];

    /* MBC related registers */
    uint16_t bank16;
    uint8_t  bank8;
    uint8_t  bankMode;
    uint8_t  ramEnable;

    /* Direct access to frontend data */
    struct
	{
		void * ptr;
	}
    direct;

    /* Memory read/write functions passed on from GameBoy object */
    uint8_t (*rom_read)(void *, const uint_fast32_t addr);
}
MMU;

void mmu_init (MMU * const);
void mmu_boot_reset (MMU * const);

/* Read functions */

uint8_t  mmu_readByte (MMU * const, uint16_t const addr);
uint16_t mmu_readWord (MMU * const, uint16_t const addr);

/* Write functions */

void mmu_writeByte       (MMU * const, uint16_t const addr, uint8_t val);
void mmu_writeWord       (MMU * const, uint16_t const addr, uint16_t val);
void mmu_io_write (MMU * const, uint16_t const addr, uint8_t const val);

#endif