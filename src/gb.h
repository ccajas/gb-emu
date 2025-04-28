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

#define SAMPLE_RATE         48000
#define GB_FRAME_RATE       (CPU_FREQ_DMG / FRAME_CYCLES)
#define CYCLES_PER_SAMPLE   (CPU_FREQ_DMG / SAMPLE_RATE)

/* Assign register pair as 16-bit union */

#define REG_16(XY, X, Y)\
    union {\
        struct { uint8_t X, Y; } r8;\
        uint16_t r16;\
    } XY;\

/* TODO: Reorganize structs so that macros for renaming regs aren't needed */

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

    /* Register bitfields */
    /* Assumes little-endian for the host platform. */

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

    /* Assigned bit values for certain registers */
    union regValues 
    {
#ifdef ENABLE_AUDIO
        struct /* Audio register NR10 */
        {
            uint8_t SweepStep : 3;
            uint8_t SweepDir  : 1;
            uint8_t SweepPace : 3;
            uint8_t Sweep_hb  : 1; /* Unused */
        };
        struct /* Audio register NRx1 */
        {
            uint8_t Length  : 6;
            uint8_t Duty    : 2;
        };
        struct /* Audio register NRx2 */
        {
            uint8_t EnvPace : 3;
            uint8_t EnvDir  : 1;
            uint8_t Volume  : 4;
        };
        struct /* Audio register NRx4 */
        {
            uint8_t PeriodH    : 3;
            uint8_t b3_5       : 3; /* Unused */
            uint8_t Len_Enable : 1;
            uint8_t Trigger    : 1;
        };
        struct /* Audio Control */
        {
            uint8_t Ch1_on    : 1;
            uint8_t Ch2_on    : 1;
            uint8_t Ch3_on    : 1;
            uint8_t Ch4_on    : 1;
            uint8_t b4_6      : 3; /* Unused */
            uint8_t Master_on : 1;
        };
#endif
        struct /* LCD Control */
        {
            uint8_t BG_Win_Enable : 1;
            uint8_t OBJ_Enable    : 1;
            uint8_t OBJ_Size      : 1;
            uint8_t BG_Area       : 1;
            uint8_t BG_Win_Data   : 1;
            uint8_t Window_Enable : 1;
            uint8_t Window_Area   : 1;
            uint8_t LCD_Enable    : 1;
        };
        struct /* LCD Status */
        {
            uint8_t stat_mode   : 2;
            uint8_t stat_LYC_LY : 1;
            uint8_t stat_HBlank : 1;
            uint8_t stat_VBlank : 1;
            uint8_t stat_OAM    : 1;
            uint8_t stat_LYC    : 1;
            uint8_t stat_hb     : 1; /* Unused */
        };
        uint8_t r;
    }
    io[IO_SIZE];
    
    /* Other CPU registers / general timekeeping */
    uint16_t pc, sp;
    uint16_t nn;
    uint64_t clock_t;
    uint16_t lineClock;
    uint16_t lineClockSt;
    uint8_t  drawFrame;
    uint32_t totalFrames;

    /* Timer data */
    uint_fast16_t divClock, lastDivClock;
    uint_fast16_t timAClock;
    uint_fast8_t  timAOverflow, nextTimA_IRQ, newTimALoaded;
    uint_fast16_t rm, rt; /* Tracks individual step cycles */
    uint_fast8_t  apuDiv;

    /* HALT and STOP status, PC increment toggle */
    uint8_t halted : 1, stopped : 1, pcInc : 1;

    /* PPU related tracking */
    uint8_t vramAccess : 1, oamAccess : 1;
    uint8_t windowLY;

    /* Memory and I/O registers */
    uint8_t ram [WRAM_SIZE];   /* Work RAM  */
    uint8_t vram[VRAM_SIZE];
    uint8_t oam [OAM_SIZE];
    uint8_t hram[HRAM_SIZE];   /* High RAM  */

    /* Interrupt master enable and PC increment */
    uint8_t ime : 1;
    uint8_t lastJoypad;
#ifdef ENABLE_AUDIO
    /* Audio channel data */
    struct {
        uint8_t  enabled   : 1;
        uint8_t  DAC       : 1;
        uint8_t  currentVol;
        uint8_t  patternStep;
        uint32_t periodTick;
        uint16_t lengthTick;
        uint8_t  envTick   : 4;
    }
    audioCh[4];
    uint8_t  sweepEnabled : 1;
    uint8_t  sweepTick : 4;
    uint16_t sweepBck;
    uint16_t audioLFSR;
#endif
    /* Catridge which holds ROM and RAM */
    struct Cartridge cart;
    uint8_t * bootRom;

    /* Directly accessible external data */
    struct gb_data_st
    {
        uint8_t joypad;
        uint8_t frameSkip;
        uint8_t pixelLine[DISPLAY_WIDTH];
        void *  ptr;
    }
    extData;

    /* Functions that rely on external data */
    void (*draw_line)    (void *, const uint8_t * pixels, const uint8_t line);
    void (*debug_cpu_log)(void *, const uint8_t);
};

uint8_t gb_apu_rw     (struct GB *, const uint8_t  reg,  const uint8_t val, const uint8_t write);
uint8_t gb_mem_access (struct GB *, const uint16_t addr, const uint8_t val, const uint8_t write);

uint8_t gb_mem_read   (struct GB *, const uint16_t addr);
uint8_t gb_mem_write  (struct GB *, const uint16_t addr, const uint8_t val);

void gb_init       (struct GB *, uint8_t *);
void gb_cpu_exec   (struct GB *, const uint8_t op);
void gb_exec_cb    (struct GB *, const uint8_t op);
void gb_reset      (struct GB *, uint8_t *);
void gb_boot_reset (struct GB *);

/* Other update-specific functions */

void gb_handle_timers       (struct GB *);
void gb_update_timer        (struct GB *, const uint8_t);
void gb_update_timer_simple (struct GB *);

void gb_render              (struct GB *);
uint8_t * gb_pixels_fetch   (struct GB *);

void gb_init_audio          (struct GB *);
void gb_ch_trigger          (struct GB *, const uint8_t);
void gb_update_div_apu      (struct GB *);

#ifdef ENABLE_AUDIO
int16_t gb_update_audio     (struct GB *);
#endif

static inline uint8_t gb_rom_loaded (struct GB * gb)
{
    return gb->cart.romData != NULL;
};

static inline void gb_boot_register (struct GB * const gb, const uint8_t val)
{
    gb->io[BootROM].r = val;
}

/*
 ********  General emulator input/update  **********
*/

static inline uint8_t gb_joypad (struct GB * gb, const uint8_t val, const uint8_t write)
{
    if (!write)
    {
        gb->io[Joypad].r |= (!(gb->io[Joypad].r & 0x10)) ?
            (gb->extData.joypad & 0xF) :  /* Direction buttons */
            (gb->extData.joypad >> 4);    /* Action buttons    */

        int i;
        for (i = 0; i < 4; i++)
        {
            const uint8_t btnLast = (gb->lastJoypad >> i) & 1;
            const uint8_t btnCurr = (gb->io[Joypad].r >> i) & 1;

            if (btnLast == 1 && btnCurr == 0)
            {
                gb->io[IntrFlags].r |= IF_Joypad;
                break;
            }
        }
        gb->lastJoypad = gb->io[Joypad].r & 0xF;

        if (!(gb->io[Joypad].r & 0xF))
            LOG_("GB: Joypad reset detected!\n");
        return gb->io[Joypad].r | 0xC0;
    }
    else { /* Set only bits 5 and 6 for selecting action/direction */
        gb->io[Joypad].r = (val & 0x30);
    }
    
    return 0;
}

static inline void gb_handle_interrupts(struct GB *gb)
{
    /* Get interrupt flags */
    const uint8_t io_IE = gb->io[IntrEnabled].r;
    const uint8_t io_IF = gb->io[IntrFlags].r;

    if (!gb->ime) return;

    /* Run if CPU ran HALT instruction or IME enabled w/flags */
    if (io_IE & io_IF & IF_Any)
    {
        gb->ime = 0;
        /* Check all 5 IE and IF bits for flag confirmations
           This loop also services interrupts by priority (0 = highest) */
        uint8_t f;
        uint8_t addr = 0x40;
        for (f = IF_VBlank; f <= IF_Joypad; f <<= 1)
        {
            if ((io_IE & f) && (io_IF & f))
            {
                gb->sp -= 2;
                CPU_WW(gb->sp, gb->pc);
                gb->pc = addr;

                /* Clear flag bit */
                gb->io[IntrFlags].r = io_IF & ~f;
                gb->rt += 20;
                break;
            }
            addr += 8;
        }
    }
}

#define USE_TIMER_SIMPLE

#ifndef CPU_LOG_INSTRS
    #define LOG_CPU_STATE(gb, op)
    #else
    #define cpu_read(X)   gb_mem_access (gb, X, 0, 0)
    #define LOG_CPU_STATE(gb, op) {\
        LOG_("op: %02X "\
            "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X "\
            "SP:%04X PC:%04X LY:%3d mode: %d\n",\
            op, REG_A, gb->flags, REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, \
            gb->sp, gb->pc, gb->io[LY], IO_STAT_MODE\
        );\
    }
#endif

#ifdef ENABLE_AUDIO
    #define UPDATE_DIV_APU   { gb_update_div_apu(gb); }
#else
    #define UPDATE_DIV_APU   { }
#endif

#ifdef USE_TIMER_SIMPLE /* Update DIV register */

#define UPDATE_DIV(gb) \
    gb->divClock += gb->rt;\
    while (gb->divClock >= 256)\
    {\
        const uint_fast8_t lastDiv = gb->io[Divider].r;\
        ++gb->io[Divider].r;\
        gb->divClock -= 256;\
        if (!(gb->io[Divider].r & 0x10) && lastDiv & 0x10)\
            UPDATE_DIV_APU \
    }\

#endif

static inline void gb_step (struct GB * gb)
{
    gb->rt = 0;

    if (gb->stopped)
    {
        gb->rt += 4;
        gb->clock_t += gb->rt;
        return;
    }

    if (gb->halted)
    {
        /* Check if interrupt is pending */
        if (gb->io[IntrEnabled].r & gb->io[IntrFlags].r & IF_Any)
            gb->halted = 0;
        
        //gb_handle_interrupts (gb);

        gb->rt += 4;
    }
    else
    {   /* Load next op and execute */
        gb->rm = 0;
        const uint8_t op = CPU_RB (gb->pc++);
        gb_cpu_exec (gb, op);
        LOG_CPU_STATE (gb, op);
    }

    gb_handle_interrupts (gb);

        /* Update timers for every remaining m-cycle */
    #ifdef USE_TIMER_SIMPLE
        UPDATE_DIV (gb);
        gb_update_timer_simple (gb);
    #else
        int t = 0;
        while (t++ < gb->rt)
            gb_handle_timers (gb);
    #endif

    /* Update PPU if LCD is turned on */
    if (gb->io[LCDControl].LCD_Enable)
        gb_render (gb);

    /* Update APU if turned on */

    gb->clock_t += gb->rt;
}

static inline void gb_frame (struct GB * gb)
{
    gb->drawFrame = 0;  
    /* Returns when frame is completed (indicated by frame cycles) */
    while (!gb->drawFrame)
    {
        gb_step (gb);
    }

    gb->totalFrames++;
}

#endif