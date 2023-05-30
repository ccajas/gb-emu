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

    /* Combination of MBC and extra hardware stored as a bitfield */
    /*union
    {
        struct 
        {
            uint8_t hardware : 4;
            uint8_t cartMBC : 4;
        };
        uint8_t hardware_MBC;
    };*/

    /* Memory bank controller */
    enum mbc_t
    {
        NO_MBC = 0,
        MBC1  = 0x10,
        MBC2  = 0x20,
        MMM01 = 0x30,
        MBC3  = 0x40,
        MBC5  = 0x50,
        MBC6  = 0x60,
        MBC7  = 0x70,
        CAM   = 0x80,
        TAMA5 = 0x90,
        HUC1  = 0xA0,
        HUC3  = 0xB0
    }
    mbc;

    /* Available hardware in cart */
    enum
    {
        HW_RAM = 1,
        HW_BATTERY = 2,
        HW_TIMER = 4,
        HW_RUMBLE = 8
    }
    hardware;

    /* Combination of MBC and extra hardware stored as a bitfield */
    uint8_t hardwareType;

    /* Determined by ROM size value in header*/
    uint16_t numRomBanks;
}
Cartridge;

/* 
   All common cart type combinations, taken from:
   http://gbdev.gg8.se/wiki/articles/The_Cartridge_Header#0147_-_Cartridge_Type 
*/
static const uint8_t cartTypes[256] = 
{
    NO_MBC, 
    MBC1, 
    MBC1   + HW_RAM,
    MBC1   + HW_RAM + HW_BATTERY,
    [0x5] = 
    MBC2,
    MBC2   + HW_BATTERY,
    [0x8] =
    NO_MBC + HW_RAM,
    NO_MBC + HW_RAM + HW_BATTERY,
    [0xB] =
    MMM01,
    MMM01  + HW_RAM,
    MMM01  + HW_RAM + HW_BATTERY,
    [0xF] =
    MBC3   + HW_TIMER + HW_BATTERY,
    MBC3   + HW_TIMER + HW_RAM + HW_BATTERY,
    MBC3,
    MBC3   + HW_RAM,
    MBC3   + HW_RAM + HW_BATTERY,
    [0x19] =
    MBC5,
    MBC5   + HW_RAM,
    MBC5   + HW_RAM + HW_BATTERY,
    MBC5   + HW_RUMBLE,
    MBC5   + HW_RUMBLE + HW_RAM,
    MBC5   + HW_RUMBLE + HW_RAM + HW_BATTERY,
    [0x20] =
    MBC6,
    [0x22] =
    MBC7   + HW_RUMBLE + HW_RAM + HW_BATTERY,
    [0xFC] =
    CAM,
    TAMA5,
    HUC3,
    HUC1   + HW_RAM + HW_BATTERY
};

#endif