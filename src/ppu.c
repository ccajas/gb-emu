#include "ppu.h"

#define IO_STAT_CLEAR   (io_regs[IO_STAT] & 0xFC)
#define IO_STAT_MODE    (io_regs[IO_STAT] & 3)

uint8_t ppu_step (PPU * const ppu, uint8_t * io_regs, uint8_t const cycles)
{
    ppu->ticks += cycles;
    ppu->frameTicks += cycles;

    uint8_t frame = 0;

	switch(IO_STAT_MODE)
	{
	    case STAT_OAM_SEARCH: /* Mode 2 - OAM read */
            /* */
	        if (ppu->ticks >= TICKS_OAM_READ)
            {
                /* Enter pixel fetch/draw */
                ppu->ticks -= TICKS_OAM_READ;
                io_regs[IO_STAT] = IO_STAT_CLEAR | STAT_TRANSFER;
            }
		break;
	    case STAT_TRANSFER: /* Mode 3 - Transfer to LCD */
            /* VRAM inaccessible by CPU */
            /* Fetch pixel data */
	        if(ppu->ticks >= TICKS_LCDTRANSFER)
            {
                /* Enter Hblank */
                ppu->ticks -= TICKS_LCDTRANSFER;
                io_regs[IO_STAT] = IO_STAT_CLEAR | STAT_HBLANK;
            }
		break;
	    case STAT_HBLANK: /* Mode 0 - Hblank */ 
            /* Image data gets sent to the frontend */

            /* Move to next scanline */
	        if(ppu->ticks >= TICKS_HBLANK)
            {
                //printf("Read %d lines. Took %d cycles.\n", io_regs[IO_LY], ppu->frameTicks);

                ppu->ticks -= TICKS_HBLANK;
                io_regs[IO_LY] = (io_regs[IO_LY] + 1) % SCAN_LINES;

                if (io_regs[IO_LYC] == io_regs[IO_LY])
                {
                    /* Set bit 02 flag for comparing lYC and LY */
                    io_regs[IO_STAT] |= 1 << 2;
                }
                else
                    /* Unset the flag */
                    io_regs[IO_STAT] &= ~(1 << 2);

                if(io_regs[IO_LY] == SCREEN_LINES)
                {
                    /* Enter Vblank */
                    io_regs[IO_STAT] = IO_STAT_CLEAR | STAT_VBLANK;
                }
                else
                {
                    io_regs[IO_STAT] = IO_STAT_CLEAR | STAT_OAM_SEARCH;
                }
            }
		break;
	    case STAT_VBLANK: /* Mode 1 - Vblank */
        	/* Advance though 10 lines below the screen */
	        if(ppu->ticks >= TICKS_VBLANK)
            {
                if (io_regs[IO_LY] == 0)
                {
                    /* Return to top line and OAM read */
                    io_regs[IO_STAT] = IO_STAT_CLEAR | STAT_OAM_SEARCH;
                    
                    /* Indicates a frame is completed */
                    frame = 1;
                    ppu->frameTicks -= 70224;
                }
		    }
		break;
	}

    return frame;
}