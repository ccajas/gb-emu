#include "utils/fileread.h"
#include "gb.h"

void gb_load_cart (GameBoy * const gb)
{
    MMU * mmu = &gb->mmu;
    const char * testRom = "test/09-op r,r.gb";

    /* Test file size */
    uint64_t size = file_size(testRom);

    if (size == -1)
        LOG_("GB: Failed to load file! .\n");
    else
    {
        LOG_("GB: Loaded file\n");
        uint8_t * filebuf = (uint8_t*) read_file (testRom);
        
        if (filebuf == NULL)
        {
            LOG_("GB: File load failed\n");
            return;
        }

        vc_init (&mmu->rom, size);
        vc_push_array (&mmu->rom, filebuf, size, 0);

        memcpy (&gb->cart.header, filebuf + 0x100, GB_HEADER_SIZE);

        free(filebuf);
        gb_print_logo(gb, 177);

        LOG_("GB: ROM loaded (%s, %d KiB)\n", gb->cart.title, CART_MIN_SIZE_KB << gb->cart.rom_size);
        LOG_("GB: Cart type: %d\n", gb->cart.cart_type);
    }
}

void gb_print_logo (GameBoy * const gb, const uint8_t charCode)
{
#ifdef GB_DEBUG
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
#endif
}

void gb_unload_cart (GameBoy * const gb)
{
    vc_free (&gb->mmu.rom);
    memset(&gb->cart.header, 0, GB_HEADER_SIZE);
}