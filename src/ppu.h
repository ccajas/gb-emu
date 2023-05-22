#ifndef PPU_H
#define PPU_H

#include <stdint.h>

typedef struct PPU_struct
{
    enum {
        G_HBLANK = 0,
        G_VBLANK,
        G_VRAM_READ,
        G_OAM_READ
    }
    modes;

    uint8_t mode;
}
PPU;

void ppu_step (PPU * const);

#endif