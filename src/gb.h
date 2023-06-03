#ifndef GB_H
#define GB_H

#include <string.h>
#include "cart.h"
#include "io.h"
#include "ops.h"

#define FRAME_CYCLES      70224
#define DISPLAY_WIDTH     160
#define DISPLAY_HEIGHT    144
#define SCAN_LINES        154

#define VRAM_SIZE     0x2000
#define WRAM_SIZE     0x2000
#define OAM_SIZE      0xA0
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
    
    /* Other CPU registers/timing data */
    uint16_t pc, sp;
    uint64_t clock_t;
    uint16_t lineClock;
    uint32_t frameClock;
    uint8_t  rt; /* Tracks individual step clocks */

    uint8_t stop, halted;
    uint8_t vramBlocked, oamBlocked;
    uint8_t invalid;

    /* PPU timing */

    /* Memory and I/O registers */
    uint8_t vram[VRAM_SIZE];
    uint8_t oam [OAM_SIZE];
    uint8_t ram [WRAM_SIZE];   /* Work RAM  */
    uint8_t hram[HRAM_SIZE];   /* High RAM  */
    uint8_t io  [IO_SIZE];

    /* Interrupt master enable */
    uint8_t ime;

    /* Catridge which holds ROM and RAM */
    struct Cartridge cart;
    uint8_t bootrom;

    /* Directly accessible external data */
    struct gb_data_s
    {
        /* Joypad button inputs */
        uint8_t joypad;
        void * ptr;
    }
    extData;

    /* Functions that rely on external data */
    void (*draw_line)(void *, const uint8_t * pixels, const uint8_t line);
};

uint8_t mbc_rw        (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t ppu_rw        (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t gb_mem_access (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);

void    gb_init     (struct GB *);
uint8_t gb_cpu_exec (struct GB *);
void    gb_exec_cb  (struct GB * gb, const uint8_t op);

void gb_handle_interrupts (struct GB * gb);
void gb_handle_timings    (struct GB * gb);
void gb_ppu_step          (struct GB * gb);
void gb_render            (struct GB * gb);

#define cpu_read(X)     gb_mem_access (gb, X, 0, 0)

inline uint8_t gb_rom_loaded (struct GB * gb)
{
    return gb->cart.romData != NULL;
};

static inline void gb_cpu_state (struct GB * gb)
{
    const uint16_t pc = gb->pc;

    //printf("%08X ", (uint32_t) gb->clock_t);
    printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X "
        "SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
        gb->r[A], gb->flags, gb->r[B], gb->r[C], gb->r[D], gb->r[E], gb->r[H], gb->r[L], 
        gb->sp, pc, cpu_read (pc), cpu_read (pc+1), cpu_read (pc+2), cpu_read (pc+3)
    );
}

static inline void gb_step (struct GB * gb)
{
    gb_handle_interrupts (gb);

    if (gb->halted)
    {
        gb->rt = 4;
        gb->clock_t    += gb->rt;
        gb->lineClock  += gb->rt;
        gb->frameClock += gb->rt;
    }
    else
    {    /* Load next op and execute */
        gb->frameClock += gb_cpu_exec (gb);
        gb->clock_t    += gb->rt;
        gb->lineClock  += gb->rt;
    }

    gb_handle_timings (gb);
    //gb_ppu_step (gb);
    gb_render (gb);
}

static inline void gb_frame (struct GB * gb)
{
    uint8_t frameDone = 0;
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