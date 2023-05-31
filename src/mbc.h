#ifndef MBC_H
#define MBC_H

#include <string.h>
#include <stdint.h>

struct MBC
{
    uint8_t * rom;
    
    /* Information about the hardware */
    uint8_t type;
    uint8_t hardware;
};

extern struct MBC mbc;

void mbc_init (uint8_t * rom);

uint8_t mbc_rw    (const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t mbc_read  (const uint16_t addr);
uint8_t mbc_write (const uint16_t addr, const uint8_t val);

/* PPU read/write, temporary */

uint8_t ppu_rw (const uint16_t addr, const uint8_t val, const uint8_t write);

#endif