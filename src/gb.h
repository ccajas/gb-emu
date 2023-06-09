#ifndef GB_H
#define GB_H

#include <string.h>
#include "cart.h"
#include "io.h"
#include "ops.h"

#define FRAME_CYCLES      70224
#define DIV_CYCLES        16384
#define DISPLAY_WIDTH     160
#define DISPLAY_HEIGHT    144
#define SCAN_LINES        154

#define BOOT_ROM_SIZE 0x100
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
    enum { A = 0, F, B, C, D, E, H, L } registers;

    uint8_t    r[8];     /* A-F, H, L - 8-bit registers */
    uint16_t * r16;      /* Registers in 16-bit gorups  */

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
    
    /* Other CPU registers / general timekeeping */
    uint16_t pc, sp;
    uint64_t clock_t, clock_m;
    uint16_t lineClock;
    uint32_t frameClock;
    uint8_t  frame;

    /* Timer data */
    uint16_t divClock, lastDiv;
    uint8_t  timAOverflow;
    uint8_t  rt, rm; /* Tracks individual step clocks */

    uint8_t stop, halted;
    uint8_t vramBlocked, oamBlocked;
    uint8_t invalid;

    /* Memory and I/O registers */
    uint8_t ram [WRAM_SIZE];   /* Work RAM  */
    uint8_t vram[VRAM_SIZE];
    uint8_t oam [OAM_SIZE];
    uint8_t hram[HRAM_SIZE];   /* High RAM  */
    uint8_t io  [IO_SIZE];

    /* Interrupt master enable */
    uint8_t ime;

    /* Catridge which holds ROM and RAM */
    struct Cartridge cart;
    uint8_t * bootRom;

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

uint8_t gb_ppu_rw     (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t gb_io_rw      (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t gb_mem_access (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);

void    gb_init       (struct GB *, uint8_t *);
void    gb_cpu_exec   (struct GB *, const uint8_t op);
void    gb_exec_cb    (struct GB *, const uint8_t op);
void    gb_reset      (struct GB *, uint8_t *);
void    gb_boot_reset (struct GB *);

/* Other update-specific functions */

void gb_handle_interrupts (struct GB * gb);
void gb_handle_timings    (struct GB * gb);
void gb_ppu_step          (struct GB * gb);
void gb_render            (struct GB * gb);

#define cpu_read(X)     gb_mem_access (gb, X, 0, 0)

static inline uint8_t gb_rom_loaded (struct GB * gb)
{
    return gb->cart.romData != NULL;
};

static inline uint8_t gb_joypad (struct GB * gb, const uint8_t val, const uint8_t write)
{
    if (!write)
    {
        if (!(gb->io[Joypad] & 0x10))   /* Direction buttons */
            gb->io[Joypad] |= (gb->extData.joypad & 0xF);
        if (!(gb->io[Joypad] & 0x20))   /* Action buttons    */
            gb->io[Joypad] |= (gb->extData.joypad >> 4);
        return gb->io[Joypad];
    }
    else { /* Set only bits 5 and 6 for selecting action/direction */
        gb->io[Joypad] = (val & 0x30);
    }
    
    return 0;
}

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

    gb->rt = 0;
    gb->rm = 0;

    if (gb->halted)
        gb->rm++;
    else
    {    /* Load next op and execute */
        const uint8_t op  = CPU_RB (gb->pc++);
        if (op != 0xCB)
            gb_cpu_exec (gb, op);
        else
            gb_exec_cb (gb, CPU_RB (gb->pc++));
    }

    /* Update timers for every m-cycle */
    int m = 0;
    while (m++ < gb->rm)
        gb_handle_timings (gb);

    gb->rt = gb->rm * 4;
    gb->clock_m += gb->rm;
    gb->clock_t += gb->rm * 4;
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
    /* Indicates odd or even frame */
    gb->frame = 1 - gb->frame;
}

#endif