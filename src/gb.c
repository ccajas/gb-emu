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

        free(filebuf);
        LOG_("GB: ROM loaded (%d bytes)\n", vc_size(&mmu->rom));
    }
}

void gb_unload_cart (GameBoy * const gb)
{
    vc_free (&gb->mmu.rom);

    LOG_("GB: Header size: %d bytes\n", sizeof(gb->cart.header));
    memset(&gb->cart.header, 0, GB_HEADER_SIZE);
}