#ifndef MBC_H
#define MBC_H

#include <string.h>
#include <stdint.h>

struct MBC
{
    /* ROM and RAM that can be accessed */
    uint8_t * romData;
    uint8_t * ramData;
    
    /* Information about the hardware */
    uint8_t type;
    uint8_t hardware;
    uint8_t header[80];

    /* MBC registers */
    uint8_t  bank8;     /* for most ROMS, 4 MiB and under */
    uint16_t bank16;    /* for large ROMS over 4 MiB      */
    uint8_t  bankMode;
    uint8_t  ramEnable;
};

/* For reference in app.c */
extern struct MBC mbc;

uint8_t * mbc_load_rom (const char *);

uint8_t mbc_rw    (const uint16_t, const uint8_t val, const uint8_t);
uint8_t mbc_read  (const uint16_t);
uint8_t mbc_write (const uint16_t, const uint8_t val);

/* PPU read/write, temporary */

inline uint8_t mbc_rom_loaded()
{
    if (mbc.romData != NULL) return 1;
    return 0;
}

#endif