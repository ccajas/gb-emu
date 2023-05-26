
#include "gb.h"

void gb_init (GameBoy * const gb, void * dataPtr,
              struct gb_func  * gb_func,
              struct gb_debug * gb_debug)
{
    /* Do a reset for the first time */
    gb_reset (gb);

    /* Pass down data pointer from frontend to components */
    gb->direct.ptr     = dataPtr;
    gb->mmu.direct.ptr = dataPtr;

    /* Pass down emulator context functions to components */
    gb->gb_func  = gb_func;
    gb->gb_debug = gb_debug;
    
    gb->mmu.rom_read = gb_func->gb_rom_read;

    /* Debug functions */
    //gb->gb_debug.update_tiles = update_tiles;

    /* Copy ROM header */
    int i;
    uint8_t header[GB_HEADER_SIZE];

    for (i = 0; i < GB_HEADER_SIZE; i++)
        header[i] = gb->mmu.rom_read(gb->direct.ptr, 0x100 + i);

    memcpy (&gb->cart.header, header, GB_HEADER_SIZE);

    /* Assign MBC and available hardware */
    gb->cart.hardware_MBC = cartTypes[gb->cart.cartType];
    
    LOG_("Test read byte 0x148 (1): %02X\n", gb->cart.romSize);
    LOG_("Test read byte 0x148 (2): %02X\n", gb->mmu.rom_read(gb->direct.ptr, 0x148));

    const uint_fast32_t romSize = (CART_MIN_SIZE_KB << gb->cart.romSize) << 10;

#ifdef FAST_ROM_READ
    vc_init (&gb->mmu.rom, romSize);
    for (i = 0; i < romSize; i++)
        vc_push (&gb->mmu.rom, gb->mmu.rom_read(gb->direct.ptr, i));
#endif
    //free (romData);

    /* Fallback: load boot ROM */
    /*vc_init (&mmu->rom, CART_MIN_SIZE_KB);
    vc_push_array (&mmu->rom, mmu->bios, 256, 0);

    memset (gb->cart.logo, 0xFF, 48);*/

    LOG_("GB: ROM loaded (%s, %d KiB)\n", gb->cart.title, romSize >> 10);
    LOG_("GB: Cart type: %02X\n", gb->cart.cartType);

#ifdef GB_DEBUG
    gb_print_logo(gb, 177);
#endif
    /* Set initial values */
    cpu_state (&gb->cpu, &gb->mmu);

    gb->stepCount = 0;
    gb->frames = 0;
}

#ifdef GB_DEBUG
void gb_print_logo (GameBoy * const gb, const uint8_t charCode)
{
    int i = 0;
    const uint8_t rows = 8, cols = 24;
    char logoImg[(cols * 4 + 1) * 8]; /* Extra chars for line breaks */

    memset(logoImg, 0, (cols * 4 + 1) * 8);

    int x;
    for (x = 0; x < rows; x++)
    {
        uint8_t mask   = (x % 2) ? 0x0f : 0xf0;
        uint8_t offset = (x % 4 < 2) ? 0 : 1; 

        int j = 0;
        for (j = 0; j < cols; j += 2)
        {
            uint8_t bits = (uint8_t)gb->cart.logo[j + offset + (cols * (x > 3))] & mask;

            int b;
            for (b = 0; b < 4; b++)
            {
                logoImg[i] = (bits & ((mask == 0xf0) ? 0x80 : 0x08)) ? charCode : ' ';
                logoImg[i + 1] = logoImg[i];
                i += 2;
                bits <<= 1;
            }
        }
        if (x < 7) logoImg[i++] = '\n';
    }

    LOG_("\n%s\n\n",logoImg);
}
#endif

uint8_t gb_step (GameBoy * const gb)
{
    const int8_t tCycles = cpu_step (&gb->cpu, &gb->mmu);
    
    gb->clockCount += tCycles;
    gb->frameClock += tCycles;

#ifdef GB_DEBUG
    //cpu_state (&gb->cpu, &gb->mmu);
#endif

    const uint8_t frameDone = ppu_step (&gb->ppu, gb->mmu.io, tCycles);

    if (gb->frameClock >= FRAME_CYCLES)
    {
        /*LOG_("Frame cycles: %d\n", gb->frameClock);*/
        gb->frames++;
        gb->frameClock -= FRAME_CYCLES;
    }
    gb->stepCount++;

    return frameDone;
}

void gb_frame (GameBoy * const gb)
{
    /* Returns 1 when frame is completed */
    while (!gb_step(gb))
    {
        /* Do extra stuff in between steps */
    }
    /* Call update to debug visualizations after frame is done */
    gb_debug_update (gb);
}

void gb_debug_update (GameBoy * const gb)
{
    if (gb->gb_debug != NULL)
    {
        /* Fetch VRAM data */
        if (gb->gb_debug->peek_vram)
        {
            gb->gb_debug->peek_vram (gb->direct.ptr, gb->mmu.vram.data);
        }

        /* Convert tileset data into pixels */
        if (gb->gb_debug->update_tiles)
        {
            gb->gb_debug->update_tiles (gb->direct.ptr, gb->mmu.vram.data);
        }
    }
}

void gb_unload_cart (GameBoy * const gb)
{
    memset(&gb->cart.header, 0, GB_HEADER_SIZE);
}

void gb_shutdown (GameBoy * const gb)
{
    gb_unload_cart (gb);
}

/* Debug functions, used here for now */
#if DEBUG_TILES
void debug_update_tiles (GameBoy * const gb)
{
    /* VRAM offset address */
    uint16_t addr;
    
    for (addr = 0; addr < 0x1000; addr++)
    {
        /* Identify tile and pixel row */
        const uint8_t tile = (addr >> 4) & 255;
        const uint8_t y = (addr >> 1) & 7;

        uint8_t idx;
        uint8_t x;
#if LOOP
        for (x = 0; x < 8; x++)
        {
            /* Bit index for byte value */
            idx = 1 << (7 - x);

            gb->tileSet[tile][y][x] =
                ((gb->mmu.vram.data[addr] & idx)   ? 1 : 0) +
                ((gb->mmu.vram.data[addr+1] & idx) ? 2 : 0);
        }
#endif
    }
}
#endif