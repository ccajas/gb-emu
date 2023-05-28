#include "ppu.h"

#define IO_STAT_CLEAR   (io_regs[IO_LCDStatus] & 0xFC)
#define IO_STAT_MODE    (io_regs[IO_LCDStatus] & 3)

uint8_t ppu_OAM_fetch (PPU * const ppu, uint8_t * io_regs)
{
    return 0;
}

uint8_t ppu_pixel_fetch (PPU * const ppu, uint8_t * io_regs)
{
    /* Get minimum starting address depends on LCDC bits 3 and 6 are set.
       Starts at 0x1800 as this is the VRAM index minus 0x8000 */
    const uint16_t BGTileMap  = 0x1800 + (io_regs[IO_LCDControl] & 0x08) ? 0x400 : 0;
    const uint16_t winTileMap = 0x1800 + (io_regs[IO_LCDControl] & 0x40) ? 0x400 : 0;   

    /* X position counter */
    uint8_t lineX = 0;
    const uint8_t lineY = io_regs[IO_LineY];

    assert (ppu->vram != NULL && ppu->vram->capacity >= 0x2000);

    /* Run at least 20 times (for the 160 pixel length) */
    for (lineX = 0; lineX < SCREEN_COLUMNS; lineX += 8)
    {
        /* BG tile fetcher gets tile ID. Bits 0-4 define X loction, bits 5-9 define Y location
           All related calculations following are found here:
           https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md */

        uint16_t tileID = BGTileMap + 
            (((io_regs[IO_ScrollY] + lineY) >> 3) << 5) +  /* Bits 5-9, Y location */
             ((io_regs[IO_ScrollX] + lineX) >> 3);                    /* Bits 0-4, X location */

        /* Tilemap location depends on LCDC 4 set, which are different rules for BG and Window tiles */

        const uint16_t BGTileData = 0x800 - (io_regs[IO_LCDControl] & 0x08) ? 0x800 : 0;

        /* Fetcher gets low byte and high byte for tile */
        const uint16_t tileRowLo = BGTileData + tileID + (((lineY + io_regs[IO_ScrollY]) & 7) << 2);
        const uint16_t tileRowHi = tileRowLo + 1;

        /* Finally get the pixel bytes from these addresses */
        const uint8_t byteLo = ppu->vram->data[tileRowLo];
        const uint8_t byteHi = ppu->vram->data[tileRowHi];

        /* Produce pixel data from the combined bytes*/
        int x;
        for (x = 0; x < 8; x++)
        {
            const uint8_t bitLo = (byteLo >> (7 - x)) & 1;
            const uint8_t bitHi = (byteHi >> (7 - x)) & 1;
            ppu->pixels[lineX + x] = (bitHi << 1) + bitLo;
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

    if (io_regs[IO_LineY] < SCREEN_LINES)
    {
        /* Visible line, within screen bounds */
        if (ppu->ticks < TICKS_OAM_READ)
        {
            /* Mode 2 - OAM read */
            if (IO_STAT_MODE != Stat_OAM_Search)
            {
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_OAM_Search;

                /* Fetch OAM data for sprites to be drawn on this line */
                ppu_OAM_fetch (ppu, io_regs);
            }
        }
        else if (ppu->ticks < TICKS_OAM_READ + TICKS_LCDTRANSFER)
        {
            /* Mode 3 - Transfer to LCD */
            if (IO_STAT_MODE != Stat_Transfer)
            {
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_Transfer;

                /* Fetch line of pixels for the screen */
                ppu_pixel_fetch (ppu, io_regs);
            }
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

                /* If STAT interrupt is enabled, an interrupt is requested */
                if (io_regs[IO_LCDStatus] & 0x40)
                    io_regs[IO_IntrFlag] |= 2;
            }
            else
                /* Unset the flag */
                io_regs[IO_LCDStatus] &= ~(1 << 2);

            /* Check if all visible lines are done */
            if (io_regs[IO_LineY] == SCREEN_LINES)
            {
                /* Enter Vblank and indicate that a frame is completed */
                io_regs[IO_LCDStatus] = IO_STAT_CLEAR | Stat_VBlank;
                io_regs[IO_IntrFlag] |= 1;
                /* Todo: handle STAT related interrupts */
                frame = 1;
            }
        }
    }
    else /* Outside of screen Y bounds */
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

    /* ...and DMA transfer to OMA, if needed */

    return frame;
}