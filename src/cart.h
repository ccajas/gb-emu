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
        cartType,
        hardware;

    /* MBC registers */
    uint8_t  bank8;     /* for most ROMS, 4 MiB and under */
    uint16_t bank16;    /* for large ROMS over 4 MiB      */

    uint8_t  
        totalBanks,
        bankMode,
        ramEnable;
};

#endif