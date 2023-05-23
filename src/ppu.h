#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#define TICKS_HBLANK      208
#define TICKS_VBLANK      456
#define TICKS_LCDTRANSFER 172
#define TICKS_OAM_READ    80

#define SCREEN_LINES      144
#define SCAN_LINES        154

typedef struct PPU_struct
{
    enum {
        STAT_HBLANK = 0,
        STAT_VBLANK,
        STAT_OAM_SEARCH,
        STAT_TRANSFER,
    }
    modes;
    uint8_t mode;

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
    uint8_t  line;
}
PPU;

uint8_t ppu_step (PPU * const, uint8_t const cycles);

#endif