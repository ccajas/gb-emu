#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "utils/v_array.h"

#define VRAM_SIZE    0x2000 /* Video RAM*/
#define ERAM_SIZE    0x2000 /* External RAM */
#define WRAM_SIZE    0x2000 /* Work RAM */
#define HRAM_SIZE    0x80   /* High RAM */

typedef struct MMU_struct
{
    uint8_t bios[256];
    uint8_t inBios;
    uint8_t cartType;

    struct VArray rom;

    uint8_t vram[VRAM_SIZE];
    uint8_t eram[ERAM_SIZE];
    uint8_t wram[WRAM_SIZE];
    uint8_t hram[HRAM_SIZE];
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