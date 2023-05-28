#ifndef PPU_H
#define PPU_H

#include <stdio.h>
#include <stdint.h>
#include "utils/v_array.h"

//#define CPU_FREQ          4194304    /* Equal to (1 << 20) * 4 */
#define FRAME_CYCLES      70224

#define DISPLAY_WIDTH     160
#define DISPLAY_HEIGHT    144
#define SCAN_LINES        154

/* Related IO registers */
static const enum
{
    IO_LCDControl	= 0x40,
    IO_LCDStatus	= 0x41,
    IO_ScrollY	    = 0x42,
    IO_ScrollX	    = 0x43,
    IO_LineY	    = 0x44,
    IO_LineYC	    = 0x45,
    IO_DMA	        = 0x46,
    IO_BGPalette	= 0x47,
    IO_OBJPalette0	= 0x48,
    IO_OBJPalette1	= 0x49,
    IO_WindowY	    = 0x4A,
    IO_WindowX	    = 0x4B,
    IO_IntrFlag     = 0x0F,
    IO_IntrEnabled  = 0xFF
}
registers;

typedef struct PPU_struct
{
    /* Used for comparing and setting PPU mode timings */
    enum {
        Stat_HBlank = 0,
        Stat_VBlank,
        Stat_OAM_Search,
        Stat_Transfer
    }
    modes;

    enum {
        TICKS_OAM_READ    = 80,
        TICKS_LCDTRANSFER = 172,
        TICKS_HBLANK      = 204,
        TICKS_VBLANK      = 456
    }
    modeTicks;

    struct LCD_control
    {
        union
        {
            /* LCDC bits */
            struct
            {
                uint8_t BG_win_enable  : 1;
                uint8_t OBJ_enable     : 1;
                uint8_t OBJ_size       : 1;
                uint8_t BG_tilemap     : 1;
                uint8_t BG_win_data    : 1;
                uint8_t Window_enable  : 1;
                uint8_t Win_tilemap    : 1;
                uint8_t LCD_PPU_enable : 1;
            };
            uint8_t r;
        };
    }
    LCDC;

    uint16_t ticks;
    uint32_t frameTicks;
    
    /* Video memory access for fetching/drawing pixels */
    struct VArray * vram;

    /* Row of pixels stored for a line */
    uint8_t pixels[DISPLAY_WIDTH];

    /* Direct access to frontend data */
    struct
	{
		void * ptr;
	}
    direct;

    /* Line drawing function passed on from GameBoy object */
    void (*draw_line)(void *, const uint8_t * pixels, const uint8_t line);
}
PPU;

uint8_t ppu_step (PPU * const, uint8_t * io_regs, const uint16_t);

/* Processes for modes 2 and 3 of the scanlines */

uint8_t ppu_OAM_fetch   (PPU * const, uint8_t * io_regs);
uint8_t ppu_pixel_fetch (PPU * const, uint8_t * io_regs);

/* OAM data transfer when writing to 0xFF46 */

uint8_t DMA_to_OAM_transfer (PPU * const);

#endif