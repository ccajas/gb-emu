#ifndef GB_H
#define GB_H

#include <string.h>
#include "cart.h"
#include "io.h"

#define FRAME_CYCLES      70224
#define DISPLAY_WIDTH     160
#define DISPLAY_HEIGHT    144
#define SCAN_LINES        154

#define VRAM_SIZE     0x2000
#define WRAM_SIZE     0x2000
#define HRAM_SIZE     0x80
#define IO_SIZE       0x80

#define GB_DEBUG

#ifdef GB_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

struct GB
{
    enum { A = 0, B, C, D, E, H, L, F = 10 } registers;

    uint8_t r[7];     /* A-E, H, L - 8-bit registers */
    
    union
    {
        struct 
        {
            uint8_t f_lb : 4; /* Unused */
            uint8_t f_c  : 1;
            uint8_t f_h  : 1;
            uint8_t f_n  : 1;
            uint8_t f_z  : 1;
        };
        uint8_t flags;
    };
    
    uint16_t pc, sp;
    uint64_t clock_t;
    uint32_t frameClock;
    uint8_t  rt; /* Tracks individual step clocks */

    uint8_t  stop, halted;
    uint8_t  invalid;

    /* Memory and I/O registers */
    uint8_t vram[VRAM_SIZE];
    uint8_t ram [WRAM_SIZE];   /* Work RAM  */
    uint8_t hram[HRAM_SIZE];   /* High RAM  */
    uint8_t io  [IO_SIZE];

    /* Interrupt master enable */
    uint8_t ime;

    /* Directly accessible data by frontend */
    struct gb_data_s
    {
        /* Joypad button inputs */
        uint8_t joypad;
    }
    gbData_;

    /* Catridge which holds ROM and RAM */
    struct Cartridge cart;
};

uint8_t mbc_rw        (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t ppu_rw        (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t gb_mem_access (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);

void    gb_init     (struct GB *);
uint8_t gb_cpu_exec (struct GB * gb, const uint8_t op);
void    gb_exec_cb  (struct GB * gb, const uint8_t op);

void gb_handle_interrupts (struct GB * gb);
void gb_handle_timings    (struct GB * gb);
void gb_render            (struct GB * gb);

inline void gb_step (struct GB * gb)
{
    gb_handle_interrupts (gb);

    /* Load next op and execute */
    const uint8_t op = gb_mem_access (gb, gb->pc++, 0, 0);
    gb->frameClock += gb_cpu_exec (gb, op);

    gb_handle_timings (gb);
    gb_render (gb);
}

inline void gb_frame (struct GB * gb)
{
    uint8_t frameDone = 0;
    printf("Running frame\n");
    /* Returns when frame is completed */
    while (!frameDone) 
    {
        gb_step (gb);
        /* Check if a frame is done */
        const uint32_t lastClock = gb->frameClock;
        gb->frameClock %= FRAME_CYCLES;

        if (lastClock > gb->frameClock) frameDone = 1;
    }
}

#endif