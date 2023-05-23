#include "ppu.h"

void ppu_step (PPU * const ppu, uint8_t const cycles)
{
    ppu->ticks += cycles;

	switch(ppu->mode)
	{
	    case G_OAM_READ: /* Mode 2 - OAM read */
            /* */
	        if (ppu->ticks >= TICKS_OAM_READ)
            {
                /* Enter pixel fetch/draw */
                ppu->ticks = 0;
                ppu->mode = G_PIXEL_DRAW;
            }
		break;
	    case G_PIXEL_DRAW: /* Mode 3 - VRAM read */
            /* VRAM inaccessible by CPU */
            /* Fetch pixel data */
	        if(ppu->ticks >= TICKS_PIXEL_DRAW)
            {
                /* Enter Hblank */
                ppu->ticks = 0;
                ppu->mode = G_HBLANK;
            }
		break;
	    case G_HBLANK: /* Hblank */ 
            /* Image data gets sent to the frontend */
	        if(ppu->ticks >= TICKS_HBLANK)
            {
                ppu->ticks = 0;
                ppu->line++;

                if(ppu->line == SCREEN_LINES)
                {
                    /* Enter Vblank */
                    ppu->mode = G_VBLANK;
                }
                else
                {
                    ppu->mode = G_OAM_READ;
                }
            }
		break;
	    case G_VBLANK: /* Vblank */
        	/* Advance though 10 lines below the screen */
	        if(ppu->ticks >= TICKS_VBLANK)
            {
                ppu->ticks = 0;
                ppu->line++;

                if(ppu->line > SCAN_LINES)
                {
                    /* Return to top line and OAM read */
                    ppu->mode = G_OAM_READ;
                    ppu->line = 0;
                }
		    }
		break;
	}
}