#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "utils/v_array.h"

#define RAM_CAPACITY 65536
#define VRAM_SIZE    8192
#define ERAM_SIZE    8192

typedef struct MMU_struct
{
    uint8_t bios[256];
    uint8_t inBios;
    uint8_t cartType;

    struct VArray rom;

    uint8_t vram[VRAM_SIZE];
    uint8_t eram[ERAM_SIZE];
}
MMU;

void mmu_reset (MMU * const);
void mmu_load (MMU * const);

uint8_t  mmu_rb (MMU * const, uint16_t const addr);
uint16_t mmu_rw (MMU * const, uint16_t const addr);

void mmu_wb (MMU * const, uint16_t const addr, uint8_t val);
void mmu_ww (MMU * const, uint16_t const addr, uint16_t val);

#endif