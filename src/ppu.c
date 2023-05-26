#include "ppu.h"

#define IO_STAT_CLEAR   (io_regs[IO_LCDStatus] & 0xFC)
#define IO_STAT_MODE    (io_regs[IO_LCDStatus] & 3)

uint8_t ppu_step (PPU * const ppu, uint8_t * io_regs, uint8_t const cycles)
{
    ppu->ticks += cycles;
    ppu->frameTicks += cycles;

    uint8_t frame = 0;

    if (io_regs[IO_LineY] < SCREEN_LINES)
    {
        /* Visible line, within screen bounds */
        if (ppu->ticks < TICKS_OAM_READ)
        {
            /* Mode 2 - OAM read */
            if (IO_STAT_MODE != Stat_OAM_Search)
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_OAM_Search;
        }
        else if (ppu->ticks < TICKS_OAM_READ + TICKS_LCDTRANSFER)
        {
            /* Mode 3 - Transfer to LCD */
            if (IO_STAT_MODE != Stat_Transfer)
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_Transfer;
        }
        else if (ppu->ticks < TICKS_OAM_READ + TICKS_LCDTRANSFER + TICKS_HBLANK)
        {
            /* Mode 0 - H-blank */
            if (IO_STAT_MODE != Stat_HBlank)
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_HBlank;     
        }
        else
        {
            /* Starting new line */
            io_regs[IO_LineY] = (io_regs[IO_LineY] + 1) % SCAN_LINES;
            ppu->ticks -= (TICKS_OAM_READ + TICKS_LCDTRANSFER + TICKS_HBLANK);

            //printf("Read %d lines. Took %d cycles. (%d leftover)\n", io_regs[IO_LineY], ppu->frameTicks, ppu->ticks);

            if (io_regs[IO_LineYC] == io_regs[IO_LineY])
            {
                /* Set bit 02 flag for comparing lYC and LY */
                io_regs[IO_LCDStatus] |= 1 << 2;
            }
            else
                /* Unset the flag */
                io_regs[IO_LCDStatus] &= ~(1 << 2);

            /* Check if we left screen bounds */
            if (io_regs[IO_LineY] == SCREEN_LINES)
            {
                /* Enter Vblank and indicate that a frame is completed */
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_VBlank;
                frame = 1;
            } 
        }
    }
    else
    {
        /* Mode 1 - H-blank */
        if(ppu->ticks >= TICKS_VBLANK)
        {
            ppu->ticks -= TICKS_VBLANK;
            //printf("Read %d lines. Took %d cycles. (%d leftover)\n", io_regs[IO_LineY], ppu->frameTicks, ppu->ticks);

            /* Advance though lines below the screen */
            io_regs[IO_LineY] = (io_regs[IO_LineY] + 1) % SCAN_LINES;

            if (io_regs[IO_LineY] == 0)
            {
                /* Return to top line and OAM read */
                ppu->frameTicks -= 70224;
            }
        }        
    }
#ifdef OLD_LOOP
	switch(IO_STAT_MODE)
	{
	    case Stat_OAM_Search: /* Mode 2 - OAM read */
            /* */
	        if (ppu->ticks >= TICKS_OAM_READ)
            {
                /* Enter pixel fetch/draw */
                ppu->ticks -= TICKS_OAM_READ;
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_Transfer;
            }
		break;
	    case Stat_Transfer: /* Mode 3 - Transfer to LCD */
            /* VRAM inaccessible by CPU */
            /* Fetch pixel data */
	        if(ppu->ticks >= TICKS_LCDTRANSFER)
            {
                /* Enter Hblank */
                ppu->ticks -= TICKS_LCDTRANSFER;
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_HBlank;
            }
		break;
	    case Stat_HBlank: /* Mode 0 - Hblank */ 
            /* Image data gets sent to the frontend */

            /* Move to next scanline */
	        if(ppu->ticks >= TICKS_HBLANK)
            {
                ppu->ticks -= TICKS_HBLANK;
                printf("Read %d lines. Took %d cycles. (%d leftover)\n", io_regs[IO_LineY], ppu->frameTicks, ppu->ticks);

                io_regs[IO_LineY] = (io_regs[IO_LineY] + 1) % SCAN_LINES;

                if (io_regs[IO_LineYC] == io_regs[IO_LineY])
                {
                    /* Set bit 02 flag for comparing lYC and LY */
                    io_regs[IO_LCDStatus] |= 1 << 2;
                }
                else
                    /* Unset the flag */
                    io_regs[IO_LCDStatus] &= ~(1 << 2);

                if (io_regs[IO_LineY] == SCREEN_LINES)
                {
                    /* Enter Vblank and indicate that a frame is completed */
                    io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_VBlank;
                    frame = 1;
                }
                else
                {
                    io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_OAM_Search;
                }
            }
		break;
	    case Stat_VBlank: /* Mode 1 - Vblank */

        	/* Advance though lines below the screen */
	        if(ppu->ticks >= TICKS_VBLANK)
            {
                ppu->ticks -= TICKS_VBLANK;
                printf("Read %d lines. Took %d cycles. (%d leftover)\n", io_regs[IO_LineY], ppu->frameTicks, ppu->ticks);

                io_regs[IO_LineY] = (io_regs[IO_LineY] + 1) % SCAN_LINES;

                /* Should only happen in the very first frame */
                if (io_regs[IO_LineY] == SCREEN_LINES)
                    frame = 1;

                if (io_regs[IO_LineY] == 0)
                {
                    /* Return to top line and OAM read */
                    io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_OAM_Search;
                    ppu->frameTicks -= 70224;
                }
		    }
		break;
	}
#endif
    return frame;
}