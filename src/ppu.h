#ifndef PPU_H
#define PPU_H

#include <stdio.h>
#include <stdint.h>

#define SCREEN_LINES      144
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
    //uint8_t  line;
}
PPU;

uint8_t ppu_step (PPU * const, uint8_t * io_regs, uint8_t const cycles);

#endif