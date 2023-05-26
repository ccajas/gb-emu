#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "utils/v_array.h"

#define CART_MIN_SIZE_KB  32

#define USING_DYNAMIC_ARRAY
#define FAST_ROM_READ__

typedef struct MMU_struct
{
    uint8_t bios[256];
    uint8_t inBios;
    uint8_t cartType;

    enum {
        VRAM_SIZE = 0x2000, /* Video RAM*/
        ERAM_SIZE = 0x2000, /* External RAM */
        WRAM_SIZE = 0x2000, /* Work RAM */
        OAM_SIZE  = 0x100,
        IO_HRAM_SIZE = 0x80 /* I/O registers/HRAM */
    }
    ramSizes;

#ifdef FAST_ROM_READ
    struct VArray rom;
#endif

#ifdef USING_DYNAMIC_ARRAY
    struct VArray vram, eram, wram;
#else
    uint8_t vram[VRAM_SIZE];
    uint8_t eram[ERAM_SIZE];
    uint8_t wram[WRAM_SIZE];
#endif
    uint8_t oam[OAM_SIZE];
    uint8_t hram[0x80];

    /* I/O registers */
    uint8_t io[0x80];

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

void mmu_reset (MMU * const);
void mmu_load (MMU * const);

/* Read functions */

uint8_t  mmu_rb (MMU * const, uint16_t const addr);
uint16_t mmu_rw (MMU * const, uint16_t const addr);

/* Write functions */

void mmu_wb (MMU * const, uint16_t const addr, uint8_t val);
void mmu_ww (MMU * const, uint16_t const addr, uint16_t val);

#endif