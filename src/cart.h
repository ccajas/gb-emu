#ifndef CART_H
#define CART_H

#include <stdint.h>
#include <string.h>

#ifdef GBE_DEBUG
    #define LOG_(f_, ...)  printf((f_),  ##__VA_ARGS__)
    #define LOGW_(f_, ...) wprintf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
    #define LOGW_(f_, ...)
#endif

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
    uint8_t romBank1;  /* for most ROMS, 4 MiB and under    */
    uint8_t romBank2;  /* for additional ROM bank bits      */
    uint8_t ramBank;

    /* Pointer to MBC read/write function */
    uint8_t (* rw)(struct Cartridge *, const uint16_t addr, const uint8_t val, const uint8_t write);

    uint16_t 
        romSizeKB,
        ramSizeKB,
        romMask;
    uint8_t
        mode,
        usingRAM;
};

void cart_identify (struct Cartridge *);

/* Concrete MBC read/write functions */

uint8_t none_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);
uint8_t mbc1_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);
uint8_t mbc2_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);
uint8_t mbc3_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);
uint8_t mbc5_rw (struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);

extern const uint8_t cartBattery[];
extern uint8_t (* cart_rw[])(struct Cartridge *, const uint16_t, const uint8_t, const uint8_t);

#endif