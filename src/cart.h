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
                    uint8_t   cgbFlag;        /* 0x143 */
                };
                char   title[0x10];
            };
            uint16_t   licensee_new; /* 0x144 */
            /* Cart technical info */
            uint8_t    sgbFlag;     /* 0x146 */
            uint8_t    cartType;    /* 0x147 */
            uint8_t    romSize;     /* 0x148 */
            uint8_t    ramSize;     /* 0x149 */
            /* Other metadata */
            uint8_t    destCode;    /* 0x14A */
            uint8_t    licensee_old; /* 0x14B */
            uint8_t    mask_rom;     /* 0x14C */
            
            uint8_t    checksumHeader;   /* 0x14D */
            uint16_t   checksumRom;      /* 0x14E */
        };

        uint8_t    header[GB_HEADER_SIZE];
    };
    /* Memory bank controller */
    enum
    {
        NO_MBC = 0,
        MBC1,
        MBC2,
        MBC3,
        MBC5,
        MBC6,
        MBC7,
        MMM01,
        TAMA5,
        HUC1,
        HUC3
    }
    mapper;
    /* Available hardware in cart */
    enum
    {
        RAM = 1,
        BATTERY = 2,
        TIMER = 4,
        RUMBLE = 8
    }
    hardware;

    uint16_t  numRomBanks;
}
Cartridge;

#endif