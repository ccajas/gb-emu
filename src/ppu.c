#include "ppu.h"

#ifdef USING_DYNAMIC_ARRAY
    #define VRAM_DATA_(N)   ppu->vram.data[N & 0x1FFF]
#else
    #define VRAM_DATA_(N)   ppu->vram[N & 0x1FFF]
#endif

#define IO_STAT_CLEAR   (io_regs[IO_LCDStatus] & 0xFC)
#define IO_STAT_MODE    (io_regs[IO_LCDStatus] & 3)

inline uint8_t DMA_to_OAM_transfer (PPU * const ppu)
{
    return 0;
}

inline uint8_t ppu_OAM_fetch (PPU * const ppu, uint8_t * io_regs)
{
    return 0;
}

inline uint8_t ppu_pixel_fetch (PPU * const ppu, uint8_t * io_regs)
{
	/* Check if background is enabled */
	if ((io_regs[IO_LCDControl] & 1) == 1)
	{
        /* Get minimum starting address depends on LCDC bits 3 and 6 are set.
        Starts at 0x1800 as this is the VRAM index minus 0x8000 */
        const uint16_t BGTileMap  = (io_regs[IO_LCDControl] & 0x08) ? 0x9C00 : 0x9800;
        //const uint16_t winTileMap = (io_regs[IO_LCDControl] & 0x40) ? 0x9C00 : 0x9800;

        /* X position counter */
        uint8_t lineX = 0;
        const uint8_t lineY = io_regs[IO_LineY];

        assert (io_regs[IO_LineY] < DISPLAY_HEIGHT);
        assert (ppu->vram != NULL);// && ppu->vram->capacity >= 0x2000);

        /* Run at least 20 times (for the 160 pixel length) */
        for (lineX = 0; lineX < DISPLAY_WIDTH; lineX += 8)
        {
            /* BG tile fetcher gets tile ID. Bits 0-4 define X loction, bits 5-9 define Y location
            All related calculations following are found here:
            https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md */

            const uint8_t posX = lineX + io_regs[IO_ScrollX];
            const uint8_t posY = lineY + io_regs[IO_ScrollY];

            uint16_t tileAddr = BGTileMap + 
                ((posY / 8) << 5) +  /* Bits 5-9, Y location */
                (posX / 8);          /* Bits 0-4, X location */

            uint16_t tileID = VRAM_DATA_(tileAddr);

            /* Tilemap location depends on LCDC 4 set, which are different rules for BG and Window tiles */

            /* Fetcher gets low byte and high byte for tile */
            const uint16_t bit12 = !((io_regs[IO_LCDControl] & 0x10) || (tileID & 0x80)) << 12;
            const uint16_t tileRow = 0x8000 + bit12 + (tileID << 4) + ((posY & 7) << 1);

            /* Finally get the pixel bytes from these addresses */
            const uint8_t byteLo = VRAM_DATA_(tileRow);
            const uint8_t byteHi = VRAM_DATA_((tileRow + 1));

            /* Produce pixel data from the combined bytes*/
            int x;
            for (x = 0; x < 8; x++)
            {
                const uint8_t bitLo = (byteLo >> (7 - x)) & 1;
                const uint8_t bitHi = (byteHi >> (7 - x)) & 1;
                ppu->pixels[lineX + x] = (bitHi << 1) + bitLo;
            }
        }
    }

    return 0;
}

uint8_t ppu_step (PPU * const ppu, uint8_t * io_regs, const uint16_t tCycles)
{
    ppu->ticks += tCycles;
    ppu->frameTicks += tCycles;

    uint8_t frame = 0;

    /* Todo: continuously fetch pixels clock by clock for LCD data transfer.
       Similarly do clock-based processing for the other actions. */
    if (io_regs[IO_LineY] < DISPLAY_HEIGHT)
    {
        /* Visible line, within screen bounds */
        if (ppu->ticks < TICKS_OAM_READ)
        {
            /* Mode 2 - OAM read */
            if (IO_STAT_MODE != Stat_OAM_Search)
            {
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_OAM_Search;

                if (io_regs[IO_LCDStatus] & 0x20) /* Mode 2 interrupt */
					io_regs[IO_IntrFlags] |= IF_LCD_STAT;

                /* Fetch OAM data for sprites to be drawn on this line */
                //ppu_OAM_fetch (ppu, io_regs);
            }
        }
        else if (ppu->ticks < TICKS_OAM_READ + TICKS_LCDTRANSFER)
        {
            /* Mode 3 - Transfer to LCD */
            if (IO_STAT_MODE != Stat_Transfer)
            {
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_Transfer;
            }
        }
        else if (ppu->ticks < TICKS_OAM_READ + TICKS_LCDTRANSFER + TICKS_HBLANK)
        {
            /* Mode 0 - H-blank */
            if (IO_STAT_MODE != Stat_HBlank)
            {   
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_HBlank;

                if (io_regs[IO_LCDStatus] & 0x08) /* Mode 0 interrupt */
					io_regs[IO_IntrFlags] |= IF_LCD_STAT;
                /* Fetch line of pixels for the screen and draw them */
                ppu_pixel_fetch (ppu, io_regs);
                ppu->draw_line (ppu->direct.ptr, ppu->pixels, io_regs[IO_LineY]);
            }
        }
        else
        {
            /* Starting new line */
            io_regs[IO_LineY] = (io_regs[IO_LineY] + 1) % SCAN_LINES;
            ppu->ticks -= (TICKS_OAM_READ + TICKS_LCDTRANSFER + TICKS_HBLANK);

            if (io_regs[IO_LineYC] == io_regs[IO_LineY])
            {
                /* Set bit 02 flag for comparing lYC and LY */
                io_regs[IO_LCDStatus] |= (1 << 2);

                /* If STAT interrupt is enabled, an interrupt is requested */
                if (io_regs[IO_LCDStatus] & 0x40) /* LYC = LY stat interrupt */
					io_regs[IO_IntrFlags] |= IF_LCD_STAT;
            }
            else
                /* Unset the flag */
                io_regs[IO_LCDStatus] &= ~(1 << 2);

            /* Check if all visible lines are done */
            if (io_regs[IO_LineY] == DISPLAY_HEIGHT)
            {
                /* Enter Vblank and indicate that a frame is completed */
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_VBlank;

                io_regs[IO_IntrFlags] |= IF_VBlank;

                if (io_regs[IO_LCDStatus] & 0x10) /* Mode 1 interrupt */
					io_regs[IO_IntrFlags] |= IF_LCD_STAT;

                frame = 1;
            }
        }
    }
    else /* Outside of screen Y bounds */
    {
        /* Mode 1 - V-blank */
        if(ppu->ticks >= TICKS_VBLANK)
        {
            //printf("Read %d lines. Took %d cycles. (%d leftover)\n", io_regs[IO_LineY], ppu->frameTicks, ppu->ticks);

            /* Advance though lines below the screen */
            io_regs[IO_LineY] = (io_regs[IO_LineY] + 1) % SCAN_LINES;
            ppu->ticks -= TICKS_VBLANK;

            if (io_regs[IO_LineYC] == io_regs[IO_LineY])
            {
                /* Set bit 02 flag for comparing lYC and LY */
                io_regs[IO_LCDStatus] |= (1 << 2);

                /* If STAT interrupt is enabled, an interrupt is requested */
                if (io_regs[IO_LCDStatus] & 0x40) /* LYC = LY stat interrupt */
					io_regs[IO_IntrFlags] |= IF_LCD_STAT;
            }
            else
                /* Unset the flag */
                io_regs[IO_LCDStatus] &= ~(1 << 2);

            if (io_regs[IO_LineY] == 0)
            {
                /* Return to top line and OAM read */
                ppu->frameTicks -= 70224;
            }
        }
    }

    /* ...and DMA transfer to OMA, if needed */
    //DMA_to_OAM_transfer (ppu);

    return frame;
}