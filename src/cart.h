#ifndef CART_H
#define CART_H

#include <stdint.h>

#define GB_HEADER_SIZE   0x50

struct Cartridge 
{
    /* ROM and RAM that can be accessed */
    uint8_t
        * romData,
        * ramData;
    
    /* Information about the game and its hardware */
    uint8_t  
        header[GB_HEADER_SIZE],
        mbc,
        cartType;

    /* Other hardware present */
    uint8_t ram     : 1;
    uint8_t battery : 1;
    uint8_t rtc     : 1;

    /* MBC registers */
    uint8_t bankLo;  /* for most ROMS, 4 MiB and under               */
    uint8_t bankHi;  /* for RAM bank and/or additional ROM bank bits */

    /* Pointer to MBC read/write function */
    uint8_t (*rw)(struct Cartridge *, const uint16_t addr, const uint8_t val, const uint8_t write);

    uint32_t 
        romSizeKB,
        ramSizeKB,
        romOffset,
        ramOffset;
    uint8_t  
        totalBanks,
        bankMode,
        ramEnabled;
};

/* Concrete MBC read/write functions */

uint8_t none_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);
uint8_t mbc1_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);
uint8_t mbc2_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);
uint8_t mbc3_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);

#endif