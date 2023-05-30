#include "gb.h"

#ifdef USING_DYNAMIC_ARRAY
    #define VRAM_DATA  mmu.vram.data
#else
    #define VRAM_DATA  mmu.vram
#endif

inline void gb_reset(GameBoy * const gb)
{
    cpu_boot_reset (&gb->cpu);
    mmu_reset (&gb->mmu);

    gb->frameClock = 0;
    gb->direct.paused = 0;

    /* Init PPU with default values */
    gb->ppu.ticks = 0;
    gb->ppu.vram = gb->VRAM_DATA;
}

void gb_init (GameBoy * const gb, void * dataPtr,
              struct gb_func  * gb_func,
              struct gb_debug * gb_debug)
{
    /* Do a reset for the first time */
    gb_reset (gb);

    /* Pass down data pointer from frontend to components */
    gb->direct.ptr     = dataPtr;
    gb->mmu.direct.ptr = dataPtr;
    gb->ppu.direct.ptr = dataPtr;

    /* Pass down emulator context functions to components */
    gb->gb_func  = gb_func;
    gb->gb_debug = gb_debug;
    
    gb->mmu.rom_read  = gb_func->gb_rom_read;
    gb->ppu.draw_line = gb_func->gb_draw_line;

    /* Copy ROM header */
    int i;
    for (i = 0; i < GB_HEADER_SIZE; i++)
        gb->cart.header[i] = gb->mmu.rom_read(gb->direct.ptr, 0x100 + i);

    /* Assign MBC and available hardware */
    gb->cart.hardwareType = cartTypes[gb->cart.cartType];

    const uint_fast32_t romSize = (CART_MIN_SIZE_KB << gb->cart.romSize) << 10;

#ifdef FAST_ROM_READ
    vc_init (&gb->mmu.rom, romSize);
    for (i = 0; i < romSize; i++)
        vc_push (&gb->mmu.rom, gb->mmu.rom_read(gb->direct.ptr, i));
#endif
    LOG_("GB: ROM loaded (%s, %d KiB)\n", gb->cart.title, romSize >> 10);
    LOG_("GB: Cart type: %02X\n", gb->cart.cartType);

#ifdef GB_DEBUG
    //gb_print_logo(gb, 177);
#endif
    /* Set initial values */
    cpu_state (&gb->cpu, &gb->mmu);

    gb->stepCount = 0;
    gb->frames = 0;
}


uint8_t gb_step (GameBoy * const gb)
{
    int16_t tCycles = cpu_step (&gb->cpu, &gb->mmu);
    
    gb->clockCount += tCycles;
    gb->frameClock += tCycles;

#ifdef GB_DEBUG
    //cpu_state (&gb->cpu, &gb->mmu);
#endif

    uint8_t frameDone = ppu_step (&gb->ppu, gb->mmu.io, tCycles);

    if (gb->frameClock >= FRAME_CYCLES)
    {
        gb->frames++;
        gb->frameClock -= FRAME_CYCLES;
        frameDone = 1;
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
            gb->gb_debug->peek_vram (gb->direct.ptr, gb->VRAM_DATA);
        /* Convert tileset data into pixels */
        if (gb->gb_debug->update_tiles)
            gb->gb_debug->update_tiles (gb->direct.ptr, gb->VRAM_DATA);
    }
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

void gb_unload_cart (GameBoy * const gb)
{
    memset(&gb->cart.header, 0, GB_HEADER_SIZE);
}

void gb_shutdown (GameBoy * const gb)
{
    gb_unload_cart (gb);
}
