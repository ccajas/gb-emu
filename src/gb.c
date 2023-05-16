#include "utils/fileread.h"
#include "gb.h"

void gb_load_cart (GameBoy * const gb)
{
    MMU * mmu = &gb->mmu;
    /* Test file */
    uint64_t size = file_size("test/03-op sp,hl.gb");

    if (size == -1)
        LOG_("GB: Failed to load file! .\n");
    else
    {
        uint8_t * filebuf = (uint8_t*) read_file ("test/03-op sp,hl.gb");
        
        if (filebuf == NULL)
        {
            LOG_("GB: File load failed\n");
            return;
        }

        vc_init (&mmu->rom, size);
        vc_push_array (&mmu->rom, filebuf, size, 0);

        memcpy (&gb->cart.header, filebuf + 0x100, GB_HEADER_SIZE);

        free(filebuf);
        LOG_("GB: ROM loaded (%d bytes)\n", gb->cart.cart_type);//vc_size(&mmu->rom));
    }
}

void gb_unload_cart (GameBoy * const gb)
{
    vc_free (&gb->mmu.rom);

    LOG_("GB: ROM loaded (%08x)\n", gb->cart.entry);
    int i;
    for (i = 0; i < 48; i++)
    {
        LOG_("%02x ", gb->cart.logo[i]);
        if ((i & 0xf) == 0xf)
            LOG_("\n");
    }
    memset(&gb->cart.header, 0, GB_HEADER_SIZE);
}