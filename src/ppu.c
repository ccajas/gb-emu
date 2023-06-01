#include "ppu.h"

struct PPU ppu;

#define LCDC_(bit)  (io[LCDControl] & (1 << bit))

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