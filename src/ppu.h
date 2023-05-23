#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#define CLOCKS_HBLANK      208
#define CLOCKS_VBLANK      456 * 10
#define CLOCKS_PIXEL_DRAW  172
#define CLOCKS_OAM_READ    80

typedef struct PPU_struct
{
    enum {
        G_HBLANK = 0,
        G_VBLANK,
        G_PIXEL_DRAW,
        G_OAM_READ
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
}
PPU;

void ppu_step (PPU * const);

#endif