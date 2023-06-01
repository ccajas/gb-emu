#ifndef PPU_H
#define PPU_H

#include <string.h>
#include <stdint.h>
#include "io.h"

#define FRAME_CYCLES      70224

#define DISPLAY_WIDTH     160
#define DISPLAY_HEIGHT    144
#define SCAN_LINES        154

struct PPU
{
    enum {
        TICKS_OAM_READ = 80,
        TICKS_TRANSFER = 252,
        TICKS_HBLANK   = 456,
        TICKS_VBLANK   = 456
    }
    modeTicks;

    uint8_t vram[0x2000];
    uint8_t oam [0xA0];

    uint16_t ticks;
    uint32_t frameTicks;
    uint8_t  
        vramBlocked,
        oamBlocked;
};

uint8_t ppu_rw (const uint16_t, const uint8_t val, const uint8_t);
void    ppu_reset ();
void    ppu_step (uint8_t * io);

void    ppu_dump_tiles (uint8_t * pixelData);

#endif