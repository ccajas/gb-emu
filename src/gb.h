#ifndef GB_H
#define GB_H

#include <string.h>
#include "cart.h"
#include "io.h"
#include "ops.h"

#define CPU_FREQ_DMG      4194304.0
#define FRAME_CYCLES      70224.0
#define DIV_CYCLES        16384
#define DISPLAY_WIDTH     160
#define DISPLAY_HEIGHT    144
#define SCAN_LINES        154

#define BOOT_ROM_SIZE 0x100
#define VRAM_SIZE     0x2000
#define WRAM_SIZE     0x2000
#define OAM_SIZE      0xA0
#define HRAM_SIZE     0x80
#define IO_SIZE       0x100

#define GB_FRAME_RATE    CPU_FREQ_DMG / FRAME_CYCLES

/* Assign register pair as 16-bit union */

#define REG_16(XY, X, Y)\
    union {\
        struct { uint8_t X, Y; } r8;\
        uint16_t r16;\
    } XY;\

/* Todo: Reorganize structs so that macros for renaming regs aren't needed */

#define REG_A    gb->af.r8.a
#define R_FLAGS  gb->flags    
#define REG_B    gb->bc.r8.b
#define REG_C    gb->bc.r8.c
#define REG_D    gb->de.r8.d
#define REG_E    gb->de.r8.e
#define REG_H    gb->hl.r8.h
#define REG_L    gb->hl.r8.l

#define REG_AF   gb->af.r16
#define REG_BC   gb->bc.r16
#define REG_DE   gb->de.r16
#define REG_HL   gb->hl.r16

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
        struct {
            uint8_t f, a, c, b, e, d, l, h;
        } r8;
        struct {
            uint16_t af, bc, de, hl; 
        } r16;
    }; 

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
    uint64_t clock_t;
    uint16_t lineClock;
    uint32_t frameClock;
    uint8_t  frame;
    uint32_t totalFrames;

    /* Timer data */
    uint16_t divClock, lastDiv;
    uint16_t timAClock;
    uint8_t  timAOverflow, nextTimA_IRQ, newTimALoaded;
    int16_t  rm, rt; /* Tracks individual step cycles */

    /* HALT and STOP status, PC increment toggle */
    uint8_t halted : 1, stopped : 1, pcInc : 1;

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
    uint8_t lastJoypad;

    /* Catridge which holds ROM and RAM */
    struct Cartridge cart;
    uint8_t * bootRom;

    /* Directly accessible external data */
    struct gb_data_sj
    {
        /* Joypad button inputs */
        uint8_t joypad;
        uint8_t frameSkip;
        uint8_t pixelLine[DISPLAY_WIDTH];
        void * ptr;
    }
    extData;

    /* Functions that rely on external data */
    void (*draw_line)    (void *, const uint8_t * pixels, const uint8_t line);
    void (*debug_cpu_log)(void *, const uint8_t);
};

uint8_t gb_ppu_rw     (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t gb_io_rw      (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);
uint8_t gb_mem_access (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);

void gb_init       (struct GB *, uint8_t *);
void gb_cpu_exec   (struct GB *, const uint8_t op);
void gb_exec_cb    (struct GB *, const uint8_t op);
void gb_reset      (struct GB *, uint8_t *);
void gb_boot_reset (struct GB *);

/* Other update-specific functions */

void gb_handle_interrupts   (struct GB *);
void gb_handle_timers       (struct GB *);
void gb_update_div          (struct GB *);
void gb_update_timer        (struct GB *, const uint8_t);
void gb_update_timer_simple (struct GB *);
void gb_render              (struct GB *);

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
*/

static inline uint8_t gb_joypad (struct GB * gb, const uint8_t val, const uint8_t write)
{
    if (!write)
    {
        gb->io[Joypad] |= (!(gb->io[Joypad] & 0x10)) ?
            (gb->extData.joypad & 0xF) :  /* Direction buttons */
            (gb->extData.joypad >> 4);    /* Action buttons    */

        int i;
        for (i = 0; i < 4; i++)
        {
            const uint8_t btnLast = (gb->lastJoypad >> i) & 1;
            const uint8_t btnCurr = (gb->io[Joypad] >> i) & 1;

            if (btnLast == 1 && btnCurr == 0)
            {
                gb->io[IntrFlags] |= IF_Joypad;
                break;
            }
        }
        gb->lastJoypad = gb->io[Joypad] & 0xF;

        if (!(gb->io[Joypad] & 0xF))
            LOG_("GB: Joypad reset detected!\n");
        return gb->io[Joypad];
    }
    else { /* Set only bits 5 and 6 for selecting action/direction */
        gb->io[Joypad] = (val & 0x30);
    }
    
    return 0;
}

#ifndef CPU_LOG_INSTRS
    #define LOG_CPU_STATE(gb, op)
    #else
    #define cpu_read(X)   gb_mem_access (gb, X, 0, 0)
    #define LOG_CPU_STATE(gb, op) {\
        LOG_("fclock %08d op: %02X "\
            "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X "\
            "SP:%04X PC:%04X LY:%3d mode: %d\n",\
            gb->frameClock, op, REG_A, gb->flags, REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, \
            gb->sp, gb->pc, gb->io[LY], IO_STAT_MODE\
        );\
    }
#endif

#define USE_TIMER_SIMPLE

static inline void gb_step (struct GB * gb)
{
    gb->rt = 0;

    if (gb->stopped)
    {
        gb->rt += 4;

        gb->clock_t += gb->rt;
        gb->frameClock += gb->rt;
        return;
    }

    if (gb->halted)
    {
        /* Check if interrupt is pending */
        if (gb->io[IntrEnabled] & gb->io[IntrFlags] & IF_Any)
            gb->halted = 0;

        if (gb->ime)
            gb_handle_interrupts (gb);

        gb->rt += 4;
    }
    else
    {   /* Load next op and execute */
        gb->rm = 0;
        const uint8_t op = CPU_RB (gb->pc++);
        gb_cpu_exec (gb, op);
        LOG_CPU_STATE (gb, op);
    }

    if (gb->ime)
        gb_handle_interrupts (gb);

    /* Update timers for every remaining m-cycle */
#ifdef USE_TIMER_SIMPLE
    gb_update_div (gb);
    gb_update_timer_simple (gb);
#else
    int t = 0;
    while (t++ < gb->rt)
        gb_handle_timers (gb);
#endif
    /* Update PPU if LCD is turned on */
    if (LCDC_(LCD_Enable))
        gb_render (gb);

    gb->clock_t += gb->rt;
    gb->frameClock += gb->rt;
}

static inline void gb_frame (struct GB * gb)
{
    /* Returns when frame is completed (indicated by frame cycles) */
    while (gb->frameClock < FRAME_CYCLES)
    {
        gb_step (gb);
    }
    gb->frameClock -= FRAME_CYCLES;

    /* Indicates odd or even frame */
    gb->frame = 1 - gb->frame;
    gb->totalFrames++;
}

#endif