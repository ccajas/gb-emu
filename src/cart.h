#ifndef CART_H
#define CART_H

#include <stdint.h>

#define GB_HEADER_SIZE   0x50

typedef struct Cartridge_struct 
{
    /* First 80 bytes after BIOS address */
    union
    {
        struct 
        {
            uint32_t   entry;       /* 0x100 */
            uint8_t    logo [0x30]; /* 0x104 */
            union
            {
                struct {
                    char      oldTitle[0x0f]; /* 0x134 */
                    uint8_t   cgb_flag;       /* 0x143 */
                };
                char   title[0x10];
            };
            uint16_t   licensee_new; /* 0x144 */
            /* Cart technical info */
            uint8_t    sgb_flag;     /* 0x146 */
            uint8_t    cart_type;    /* 0x147 */
            uint8_t    rom_size;     /* 0x148 */
            uint8_t    ram_size;     /* 0x149 */
            /* Other metadata */
            uint8_t    dest_code;    /* 0x14A */
            uint8_t    licensee_old; /* 0x14B */
            uint8_t    mask_rom;     /* 0x14C */
            
            uint8_t    checksum_header;   /* 0x14D */
            uint16_t   checksum_rom;      /* 0x14E */
        };
        uint8_t    header[GB_HEADER_SIZE];
    };
}
Cartridge;

#endif