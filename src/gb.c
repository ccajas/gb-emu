#include "utils/fileread.h"
#include "gb.h"

void gb_init (GameBoy * const gb, const char * defaultROM)
{
    /* Do a one time reset */
    gb_reset (gb);

    gb_load_cart (gb, defaultROM);
    cpu_state (&gb->cpu, &gb->mmu);

    gb->stepCount = 0;
    gb->running = 1;
}

void gb_load_cart (GameBoy * const gb, const char * defaultROM)
{
    MMU * mmu = &gb->mmu;
    const char * testRom = defaultROM;

    LOG_("GB: Loading file \"%s\"...\n", testRom);
    uint8_t * filebuf = (uint8_t*) read_file (testRom);
    
    if (filebuf == NULL)
    {
        LOG_("GB: Failed to load file! .\n");

        /* Fallback: load boot ROM */
        vc_init (&mmu->rom, CART_MIN_SIZE_KB);
        vc_push_array (&mmu->rom, mmu->bios, 256, 0);

        memset (gb->cart.logo, 0xFF, 48);
    }
    else
    {
        const uint32_t romSize = CART_MIN_SIZE_KB << gb->cart.romSize;
        vc_init (&mmu->rom, romSize);
        vc_push_array (&mmu->rom, filebuf, romSize, 0);

        memcpy (&gb->cart.header, filebuf + 0x100, GB_HEADER_SIZE);

        free(filebuf);

        LOG_("GB: ROM loaded (%s, %d KiB)\n", gb->cart.title, romSize);
        LOG_("GB: Cart type: %d\n", gb->cart.cartType);
    }

#ifdef GB_DEBUG
    gb_print_logo(gb, 177);
#endif
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

#define CPU_FREQ          4194304    /* Equal to (1 << 20) * 4 */
#define FRAME_CYCLES      70224
#define TEST_MAX_STEPS    45000000
#define TEST_MAX_CLOCKS   CPU_FREQ * 100

uint8_t gb_step (GameBoy * const gb)
{
    const int8_t tCycles = cpu_step (&gb->cpu, &gb->mmu);
    
    gb->clockCount += tCycles;
    gb->frameClock += tCycles;

#ifdef GB_DEBUG
    //cpu_state (&gb->cpu, &gb->mmu);
#endif
    const uint8_t frameDone = ppu_step (&gb->ppu, gb->mmu.hram, tCycles);

    if (gb->frameClock >= FRAME_CYCLES)
    {
        //printf("Frames passed: %d (%d)\n", gb->frames, gb->frameClock);
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
}

void gb_unload_cart (GameBoy * const gb)
{
    vc_free (&gb->mmu.rom);
    memset(&gb->cart.header, 0, GB_HEADER_SIZE);
}

void gb_shutdown (GameBoy * const gb)
{
    gb_unload_cart (gb);
}