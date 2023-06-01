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

void mbc_init (uint8_t * rom);

uint8_t mbc_rw    (const uint16_t, const uint8_t val, const uint8_t);
uint8_t mbc_read  (const uint16_t);
uint8_t mbc_write (const uint16_t, const uint8_t val);

/* PPU read/write, temporary */

uint8_t ppu_rw (const uint16_t addr, const uint8_t val, const uint8_t write);

#endif