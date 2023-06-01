#include "ppu.h"

struct PPU ppu;

#define LCDC_(bit)  (io[LCDControl] & (1 << bit))

void ppu_reset()
{
    memset(ppu.vram, 0, 0x2000);

    ppu.ticks = 0;
    ppu.frameTicks = 0;
    ppu.vramBlocked = 0;
}

inline uint8_t ppu_read (const uint16_t addr)
{
    //assert (mbc.rom[addr] != NULL);
    if (ppu.vramBlocked) return 0xFF;
    return ppu.vram[addr & 0x1FFF];
}

inline uint8_t ppu_write (const uint16_t addr, const uint8_t val)
{
    //assert (mbc.rom[addr] != NULL);
    ppu.vram[addr & 0x1FFF] = val;
    return 0;
}

/* PPU read/write */

uint8_t ppu_rw (const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (write) return ppu_write (addr, val);
    else return ppu_read(addr);
}

/* Called in mode 0 */

inline void ppu_hblank() { }

/* Called in mode 1 */

inline void ppu_vblank() { }

/* Called in mode 2 */

inline void ppu_oam() { }

/* Called in mode 3 */

inline void ppu_pixelFIFO() { }

/* Run PPU for a few cycles */

void ppu_step (uint8_t * io)
{
    switch (io[LY])
    {
        case 0 ... DISPLAY_HEIGHT - 1:
            if (ppu.ticks < TICKS_OAM_READ) { ppu_oam();       break; }
            if (ppu.ticks < TICKS_TRANSFER) { ppu_pixelFIFO(); break; }
            if (ppu.ticks < TICKS_HBLANK)   { ppu_hblank();    break; }
            // Starting a new line 
            //io[LY] = io[LY]++ % SCAN_LINES;
            ppu.ticks -= TICKS_HBLANK;
        break;
        default:
            ppu_vblank();
    }
    // Update dot and scanline counters:    
    //if (++io[LY] > SCAN_LINES)
    {
        //ppu.frameTicks -= 70224;
    }
}

/* Debug function for viewing tiles */

void ppu_dump_tiles (uint8_t * pixelData)
{
    const uint8_t * data = ppu.vram;
    const uint8_t TILE_SIZE_BYTES = 16;

    const uint16_t NUM_TILES = 256;
    const uint16_t TILE_SIZE = 8;

    int t;
    for (t = 0; t < NUM_TILES; t++)
    {
        const uint16_t tileXoffset = (t % 16) * TILE_SIZE;
        const uint16_t tileYoffset = (t >> 4) * 1024;

        int y;
        for (y = 0; y < 8; y++)
        {
            const uint8_t row1 = *(data + (y * 2));
            const uint8_t row2 = *(data + (y * 2) + 1);
            const uint16_t yOffset = y * 128;

            int x;
            for (x = 0; x < 8; x++)
            {
                uint8_t col1 = row1 >> (7 - x);
                uint8_t col2 = row2 >> (7 - x);
                
                const uint8_t  colorID = 3 - ((col1 & 1) + ((col2 & 1) << 1));
                const uint16_t idx = (tileYoffset + yOffset + tileXoffset + x) * 3;

                pixelData[idx] = colorID * 0x55;
                pixelData[idx + 1] = colorID * 0x55;
                pixelData[idx + 2] = colorID * 0x55;
            }
        }
        data += TILE_SIZE_BYTES;
    }
}