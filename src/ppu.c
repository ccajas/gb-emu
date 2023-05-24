#include "ppu.h"

uint8_t ppu_step (PPU * const ppu, uint8_t * io_regs, uint8_t const cycles)
{
    ppu->ticks += cycles;
    uint8_t frame = 0;

	switch(ppu->mode)
	{
	    case STAT_OAM_SEARCH: /* Mode 2 - OAM read */
            /* */
	        if (ppu->ticks >= TICKS_OAM_READ)
            {
                /* Enter pixel fetch/draw */
                ppu->ticks -= TICKS_OAM_READ;
                ppu->mode = STAT_TRANSFER;
            }
		break;
	    case STAT_TRANSFER: /* Mode 3 - VRAM read */
            /* VRAM inaccessible by CPU */
            /* Fetch pixel data */
	        if(ppu->ticks >= TICKS_LCDTRANSFER)
            {
                /* Enter Hblank */
                ppu->ticks -= TICKS_LCDTRANSFER;
                ppu->mode = STAT_HBLANK;
            }
		break;
	    case STAT_HBLANK: /* Mode 0 - Hblank */ 
            /* Image data gets sent to the frontend */

            /* Move to next scanline */
	        if(ppu->ticks >= TICKS_HBLANK)
            {
                ppu->ticks -= TICKS_HBLANK;
                ppu->line = (ppu->line + 1) % SCAN_LINES;

                if(ppu->line == SCREEN_LINES)
                {
                    /* Enter Vblank */
                    ppu->mode = STAT_VBLANK;
                }
                else
                {
                    ppu->mode = STAT_OAM_SEARCH;
                }
            }
		break;
	    case STAT_VBLANK: /* Mode 1 - Vblank */
        	/* Advance though 10 lines below the screen */
	        if(ppu->ticks >= TICKS_VBLANK)
            {
                ppu->ticks -= TICKS_VBLANK;
                ppu->line = (ppu->line + 1) % SCAN_LINES;

                if (ppu->line == 0)
                {
                    /* Return to top line and OAM read */
                    ppu->mode = STAT_OAM_SEARCH;
                    frame = 1;
                }
		    }
		break;
	}

    return frame;
}