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

/* Assign register pair as 16-bit union */

#define REG_16(XY, X, Y)\
    union {\
        struct { uint8_t X, Y; } r8;\
        uint16_t r16;\
    } XY;\

#define REG_A    gb->af.r8.a
#define REG_B    gb->bc.r8.b
#define REG_C    gb->bc.r8.c
#define REG_D    gb->de.r8.d
#define REG_E    gb->de.r8.e
#define REG_H    gb->hl.r8.h
#define REG_L    gb->hl.r8.l

struct GB
{
    enum { A = 0, F } registers;

    /* A-F, H, L - 8-bit registers */
    REG_16(af, f, a);
    REG_16(bc, c, b);
    REG_16(de, e, d);
    REG_16(hl, l, h);

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
    uint8_t  frame, totalFrames;

    /* Timer data */
    uint16_t divClock, lastDiv;
    uint8_t  timAOverflow, nextTimA_IRQ, newTimALoaded;
    uint8_t  rt, rm; /* Tracks individual step clocks */

    /* HALT and STOP status, PC increment toggle */
    uint8_t stop : 1, halted : 1, pcInc : 1;

    /* PPU related tracking */
    uint8_t vramBlocked, oamBlocked;
    uint8_t windowLY;
    uint8_t invalid;

    /* Memory and I/O registers */
    uint8_t ram [WRAM_SIZE];   /* Work RAM  */
    uint8_t vram[VRAM_SIZE];
    uint8_t oam [OAM_SIZE];
    uint8_t hram[HRAM_SIZE];   /* High RAM  */
    uint8_t io  [IO_SIZE];

    /* Interrupt master enable and PC increment */
    uint8_t ime : 1;

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

void gb_handle_interrupts (struct GB *);
void gb_handle_timings    (struct GB *);
void gb_timer_update      (struct GB *, const uint8_t);
void gb_ppu_step          (struct GB *);
void gb_render            (struct GB *);

static inline uint8_t gb_rom_loaded (struct GB * gb)
{
    return gb->cart.romData != NULL;
};

static inline void gb_boot_register (struct GB * const gb, const uint8_t val)
{
    gb->io[BootROM] = val;
}

/*
 ********  General emulator input/update  **********
 ===================================================
*/

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
    #define cpu_read(X)   gb_mem_access (gb, X, 0, 0)

    //printf("%08X ", (uint32_t) gb->clock_t);
    printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X "
        "SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
        REG_A, gb->flags, REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, 
        gb->sp, pc, cpu_read (pc), cpu_read (pc+1), cpu_read (pc+2), cpu_read (pc+3)
    );
}

static inline void gb_step (struct GB * gb)
{
    gb->rt = 0;
    gb->rm = 0;

    if (gb->halted)
    {
        /* Check if interrupt is pending */
        if (gb->io[IntrEnabled] & gb->io[IntrFlags] & IF_Any)
            gb->halted = 0;

        if (gb->ime)
            gb_handle_interrupts (gb);

        gb->rm++;
    }
    else
    {   /* Load next op and execute */
        const uint8_t op = CPU_RB (gb->pc++);
        gb_cpu_exec (gb, op);
        //gb_cpu_state (gb);
    }

    if (gb->ime)
        gb_handle_interrupts (gb);

    /* Update timers for every remaining m-cycle */
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
    gb->totalFrames++;
}

#endif