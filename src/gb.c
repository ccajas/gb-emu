#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "gb.h"
#include "ops.h"

#if defined(ASSERT_INSTR_TIMING) || !defined(USE_INC_MCYCLE)
#include "opcycles.h"
#endif

/*
 **********  Memory/bus read and write  ************
 */

#define RISING_EDGE(before, after)  ((before & 1) < (after & 1))
#define FALLING_EDGE(before, after) ((before & 1) > (after & 1))

inline uint8_t gb_io_rw(struct GB *gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write)
        return gb->io[addr % 0x80].r;
    else
    {
        switch (addr & 0xFF)
        {
#ifdef USE_TIMER_SIMPLE
            case Divider:
                gb->io[Divider].r = 0;
                break;
            case TimA:
                // LOG_("GB: TIMA update (%d, A: $%02x)\n", (int8_t)val, REG_A);
                break;
            case TMA:
                gb->io[TMA].r = val;
                break;
#else
            case Divider:
                gb_update_timer(gb, 0);
                break; /* DIV reset                             */
            case TimA:
                // LOG_("GB: TIMA update (%d, A: $%02x)\n", (int8_t)val, REG_A);
                if (!gb->newTimALoaded)
                    gb->io[TimA].r = val; /* Update TIMA if new value wasn't loaded last cycle    */
                if (gb->nextTimA_IRQ)
                    gb->nextTimA_IRQ = 0; /* Cancel any pending IRQ when accessing TIMA           */
                break;
            case TMA: /* Update TIMA also if TMA is written in the same cycle */
                if (gb->newTimALoaded)
                    gb->io[TimA].r = val;
                break;
#endif
            case TimerCtrl: /* Todo: TIMA should increase right here if last bit was 1 and current is 0  */
                gb->io[TimerCtrl].r = val | 0xF8;
                break;
            case IntrFlags: /* Mask unused bits for IE and IF         */
            case IntrEnabled:
                gb->io[addr % 0x80].r = val | 0xE0;
                break;
            /* APU registers */
            case Ch1_LD:
            case Ch2_LD:
            case Ch4_Length:
            {
                const uint8_t channel = ((addr & 0xff) - 0x10) / 5;
                gb->io[Ch1_Vol + (channel * 5)].r = val;
                break;
            }
            case Ch1_Vol:
            case Ch2_Vol:
            case Ch4_Vol:
            {
                const uint8_t channel = ((addr & 0xff) - 0x10) / 5;
                gb->audioCh[channel].DAC = (val & 0xF8) != 0;
                gb->io[Ch1_Vol + (channel * 5)].r = val;
                break;
            }
            case Ch3_Vol:
                gb->audioCh[2].currentVol = (val >> 5) & 3;
                gb->io[Ch3_Vol].r = val;
                break;
            case Ch1_Ctrl:
            case Ch2_Ctrl:
            case Ch3_Ctrl:
            case Ch4_Ctrl:
            {
                const uint8_t channel = ((addr & 0xff) - 0x10) / 5;
                gb->io[Ch1_Ctrl + (channel * 5)].r = val;
                if (val & 0x80)
                    gb_ch_trigger(gb, channel);
                
                break;
            }
            case LCDControl:
            { /* Check whether LCD will be turned on or off */
                const uint8_t lcdEnabled =  gb->io[LCDControl].LCD_Enable;
                if (lcdEnabled && !(val & (1 << LCD_Enable)))
                {
                    LOG_("GB: ░ LCD turn off (%d:%d)\n", gb->totalFrames, gb->io[LY].r);
                    IO_STAT_MODE = 0; /* Clear STAT mode when turning off LCD       */
                    gb->io[LY].r = 0;
                }
                else if (!lcdEnabled && (val & (1 << LCD_Enable)))
                {
                    LOG_("GB: ▓ LCD turn on  (%d:%d)\n", gb->totalFrames, gb->io[LY].r);
                }
                break;
            }
            case LY: /* Writing to LY resets line counter      */
                break;
            case DMA: /* OAM DMA transfer     */
                gb->io[DMA].r = val;
                int i = 0; /* Todo: Make it write across 160 cycles */
                const uint16_t src = val << 8;
                while (i < OAM_SIZE)
                {
                    gb->oam[i] = CPU_RB(src + i);
                    i++;
                }
                break;
        }
        gb->io[addr & 0xFF].r = val;
    }
    return 0;
}

uint8_t gb_mem_access(struct GB *gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    /* For Blargg's CPU instruction tests */
#ifdef CPU_INSTRS_TESTING
    if (addr == 0xFF44 && !write)
        return 0x90;
    if (gb->io[SerialCtrl] == 0x81)
    {
        LOG_("%c", gb->io[SerialData]);
        gb->io[SerialCtrl] = 0x0;
    }
#endif
    /* Byte to be accessed from memory */
    uint8_t *b;
#define DIRECT_RW(b)\
    if (write)\
        *b = val;\
    return *b;\

    INC_MCYCLE;
    struct Cartridge *cart = &gb->cart;

    if (addr < 0x0100 && gb->io[BootROM].r == 0) /* Run boot ROM if needed */
    {
        if (!write)
            return gb->bootRom[addr];
        else
            return 0;
    }
    if (addr < 0x8000)
        return cart->rw(cart, addr, val, write); /* ROM from MBC     */
    if (addr < 0xA000)
    {
        if (gb->vramBlocked)
            return 0xFF;
        if (!write)
            return gb->vram[addr & 0x1FFF];
        else
            gb->vram[addr & 0x1FFF] = val;
        return 0;
    } /* Video RAM        */
    if (addr < 0xC000)
        return cart->rw(cart, addr, val, write); /* External RAM     */
    if (addr < 0xE000)
    {
        b = &gb->ram[addr % WRAM_SIZE];
        DIRECT_RW(b);
    } /* Work RAM         */
    if (addr < 0xFE00)
    {
        b = &gb->ram[addr % WRAM_SIZE];
        DIRECT_RW(b);
    } /* Echo RAM         */
    if (addr < 0xFEA0) /* OAM              */
    {
        if (gb->oamBlocked)
            return 0xFF;
        if (!write)
            return gb->oam[addr - 0xFE00];
        else
            gb->oam[addr - 0xFE00] = val;
        return 0;
    }
    if (addr < 0xFF00)
        return 0xFF; /* Not usable       */
    if (addr == 0xFF00)
        return gb_joypad(gb, val, write);      /* Joypad           */
    if (addr < 0xFF80)
        return gb_io_rw(gb, addr, val, write); /* I/O registers    */
    if (addr < 0xFFFF)
    {
        b = &gb->hram[addr % HRAM_SIZE];
        DIRECT_RW(b);
    } /* High RAM         */
    if (addr == 0xFFFF)
        return gb_io_rw(gb, addr, val, write); /* Interrupt enable */

#undef DIRECT_RW
    return 0;
}

/*
 **********  Console startup functions  ************
 */

void gb_init(struct GB *gb, uint8_t *bootRom)
{
    cart_identify(&gb->cart);
    LOG_("GB: ROM mask: %d\n", gb->cart.romMask);

    /* Initialize RAM and settings */
    memset(gb->ram, 0, WRAM_SIZE);
    memset(gb->vram, 0, VRAM_SIZE);
    memset(gb->hram, 0, HRAM_SIZE);
    memset(gb->oam, 0, OAM_SIZE);
    gb->vramBlocked = gb->oamBlocked = 0;

    memset(gb->io, 0, sizeof(gb->io));
    LOG_("GB: Memset done\n");

    if (bootRom != NULL)
        gb_reset(gb, bootRom);
    else
        gb_boot_reset(gb);

    LOG_CPU_STATE(gb, 0);
    gb->lineClock = gb->frameClock = 0;
    gb->clock_t = 0;
    gb->divClock = gb->timAClock = 0;
    gb->apuDiv = 0;
    gb->frame = gb->totalFrames = 0;
    gb->pcInc = 1;
    LOG_("GB: CPU state done\n");
}

void gb_reset(struct GB *gb, uint8_t *bootROM)
{
    LOG_("GB: Load Boot ROM\n");
    LOG_("GB: Set I/O\n");

    gb->bootRom = bootROM;
    gb->extData.joypad = 0xFF;
    gb->io[Joypad].r = 0xCF;
    gb_boot_register(gb, 0);

    gb->pc = 0;
    gb->ime = 0;
    gb->invalid = 0;
    gb->halted = 0;
    gb->stopped = 0;
}

void gb_boot_reset(struct GB *gb)
{
    gb->bootRom = NULL;
    gb->extData.joypad = 0xFF;

    /* Setup CPU registers as if bootrom was loaded */
    REG_A = 0x01;
    gb->flags = 0xB0;
    REG_B = 0x0;
    REG_C = 0x13;
    REG_D = 0x0;
    REG_E = 0xD8;
    REG_H = 0x01;
    REG_L = 0x4D;
    gb->sp = 0xFFFE;
    gb->pc = 0x0100;

    gb->ime = 1;
    gb->invalid = 0;
    gb->halted = 0;
    gb->stopped = 0;

    LOG_("GB: Launch without boot ROM\n");
    LOG_("GB: Set I/O\n");

    /* Initalize I/O registers (DMG) */
    memset(gb->io, 0xFF, sizeof(gb->io));
    gb->io[Joypad].r      = 0xCF;
    gb->io[SerialCtrl].r  = 0x7E;
    gb->io[Divider].r     = 0xAB;
    gb->io[TimA].r        = 0x0;
    gb->io[TMA].r         = 0x0;
    gb->io[TimerCtrl].r   = 0xF8;
    gb->io[IntrFlags].r   = 0xE1;
    gb->io[LCDControl].r  = 0x91;
    gb->io[LCDStatus].r   = 0x85;
    gb->io[ScrollY].r     = 0x0;
    gb->io[ScrollX].r     = 0x0;
    gb->io[LY].r          = 0x0;
    gb->io[LYC].r         = 0x0;
    gb->io[DMA].r         = 0xFF;
    gb->io[BGPalette].r   = 0xFC;
    gb->io[WindowY].r     = 0x0;
    gb->io[WindowX].r     = 0x0;
    gb->io[IntrEnabled].r = 0x0;

    gb_init_audio(gb);
    gb_boot_register(gb, 1);
}

/*
 *****************  CPU functions  *****************
 */

void gb_cpu_exec(struct GB *gb, const uint8_t op)
{
    /* Copied value for operands (can be overridden for other opcodes) */
    uint8_t tmp = REG_A;
    const uint8_t opHh = op >> 3; /* Octal divisions */

    /* For conditional jump / call instructions */
    const uint8_t cond = ((op >> 3) & 3);

    switch (opHh)
    {
        case 8 ... 0xF:
            /* Fetch value at address (HL) if it's needed */
            //uint8_t hl = ((op & 7) == 0x6) ? CPU_RB(REG_HL) : 0;
            switch (op)
            {
                /* Halt */
                case 0x76:
                OP(HALT)
                if (!gb->ime)
                {
                    if (gb->io[IntrEnabled].r && gb->io[IntrFlags].r & IF_Any)
                        gb->pcInc = 0;
                    else
                        gb->halted = 1;
                }
                else
                    gb->halted = 1;
                break;
                /* 8-bit load, LD or LDrHL */
                LD_OPS
                /* 8-bit load, LDHLr */
                OP_r8(0x70, LD_HL, 0)
            }
            break;
        default:
            switch (op)
            {
                case 0x0:
                    NOP break;
                /* 16-bit load, arithmetic instructions */
                OP_r16_g1(0x01, LDrr)
                OP_r16_g1(0x03, INCrr)
                OP_r16_g1(0x09, ADHLrr)
                OP_r16_g1(0x0B, DECrr) 
                case 0x08:
                    LDmSP break;
                /* 8-bit load instructions */
                OP_r16_g2(0x02, LDrrmA)
                OP_r16_g2(0x0A, LDArrm)
                /* Increment, decrement, LD r8,n  */
                OP_r8_g(0x04, INC, 0)
                OP_r8_g(0x05, DEC, 0)
                OP_r8_g(0x06, LDrm, 0)
                /* Opcode group 1 */
                case 0x07:
                    RLCA break;
                case 0x17:
                    RLA break;
                case 0x27:
                    DAA break;
                case 0x37:
                    SCF break;
                /* STOP and relative jump to nn */
                case 0x10:
                    STOP break;
                case 0x18:
                    JRm break;
                /* Opcode group 2 */
                case 0x0F:
                    RRCA break;
                case 0x1F:
                    RRA break;
                case 0x2F:
                    CPL break;
                case 0x3F:
                    CCF break;
                /* 8-bit arithmetic */
                OP_r8_hl(0x80, ADD, 0)
                OP_r8_hl(0x88, ADC, 0)
                OP_r8_hl(0x90, SUB, 0)
                OP_r8_hl(0x98, SBC, 0)
                OP_r8_hl(0xA0, AND, 0)
                OP_r8_hl(0xA8, XOR, 0)
                OP_r8_hl(0xB0, OR, 0)
                OP_r8_hl(0xB8, CP, 0)
                    /* ... */
                case 0xC3:
                    JPNN break;
                case 0xC6:
                    ADDm break;
                case 0xC9:
                    RET break;
                case 0xCD:
                    CALLm break;
                case 0xCE:
                    ADCm break;

                case 0xD6:
                    SUBm break;
                case 0xD9:
                    RETI break;
                case 0xDE:
                    SBCm break;

                case 0xE0:
                    LDHmA break;
                case 0xE2:
                    LDHCA break;
                case 0xE6:
                    ANDm break;
                case 0xE8:
                    ADDSPm break;
                case 0xE9:
                    JPHL break;
                case 0xEA:
                    LDmmA break;
                case 0xEE:
                    XORm break;

                case 0xF0:
                    LDHAm break;
                case 0xF2:
                    LDHAC break;
                case 0xF3:
                    DI break;
                case 0xF6:
                    ORm break;
                case 0xF8:
                    LDHLSP break;
                case 0xF9:
                    LDSPHL break;
                case 0xFA:
                    LDAmm break;
                case 0xFB:
                    EI break;
                case 0xFE:
                    CPm break;
                /* Conditional jump, return/call */
                case 0x20:
                case 0x28:
                case 0x30:
                case 0x38:
                    JR_(cond) break;
                case 0xC0:
                case 0xC8:
                case 0xD0:
                case 0xD8:
                    RET_(cond) break;
                case 0xC2:
                case 0xCA:
                case 0xD2:
                case 0xDA:
                    JP_(cond) break;
                case 0xC4:
                case 0xCC:
                case 0xD4:
                case 0xDC:
                    CALL_(cond) break;
                /* Call routines at addresses 0x00 to 0x38 */
                case 0xC7:
                case 0xCF:
                case 0xD7:
                case 0xDF:
                case 0xE7:
                case 0xEF:
                case 0xF7:
                case 0xFF:
                    RST break;
                /* 16-bit push and pop */
                OP_r16_g3(0xC1, POPrr)
                OP_r16_g3(0xC5, PUSHrr)
                /* CB prefix ops */
                case 0xCB:
                    gb_exec_cb(gb, CPU_RB(gb->pc++));
                    break;
                default:
                    INVALID
            }
    }

#ifndef USE_INC_MCYCLE
    gb->rm += opCycles[op];
#endif

    /* Handle effects of STOP instruction */
    /* Todo: Read joypad button selection/press */
    if (op == 0x10 && gb->stopped)
    {
        gb->stopped = 0;
        gb_mem_access(gb, 0xFF00 + gb->io[Divider].r, 0, 1);
    }

#ifdef ASSERT_INSTR_TIMING
    assert(gb->rm >= opCycles[op]);
#endif

    gb->rt = gb->rm * 4;
}

void gb_exec_cb(struct GB *gb, const uint8_t op)
{
    #ifndef USE_INC_MCYCLE
    ++gb->rm;
    #endif

    const uint8_t opL = op & 0xf;
    const uint8_t opHh = op >> 3; /* Octal divisions */
    const uint8_t r_bit = opHh & 7;

    /* Fetch value at address (HL) if it's needed */
    uint8_t hl = (opL == 0x6 || opL == 0xE) ? CPU_RB(REG_HL) : 0;

    switch (op)
    {
        OPR_2_(0,    RLC)
        OPR_2_(0x8,  RRC)
        OPR_2_(0x10, RL)
        OPR_2_(0x18, RR) 
        OPR_2_(0x20, SLA)
        OPR_2_(0x28, SRA)
        OPR_2_(0x30, SWAP)
        OPR_2_(0x38, SRL)

        OPR_2R_(0x40, BIT) /* Bit test  */
        OPR_2R_(0x80, RES) /* Bit reset */
        OPR_2R_(0xC0, SET) /* Bit set   */
    }

    /* Write back to (HL) for most (HL) operations except BIT */
    if ((op & 7) == 6 && (opHh < 8 || opHh > 0xF))
    {
        #ifndef USE_INC_MCYCLE
        ++gb->rm;
        #endif
        CPU_WB(REG_HL, hl);
    }
}

void gb_handle_interrupts(struct GB *gb)
{
    /* Get interrupt flags */
    const uint8_t io_IE = gb->io[IntrEnabled].r;
    const uint8_t io_IF = gb->io[IntrFlags].r;

    if (!gb->pcInc)
    {
        --gb->pc;
        gb->pcInc = 1;
    }
    /* Run if CPU ran HALT instruction or IME enabled w/flags */
    if (io_IE & io_IF & IF_Any)
    {
        gb->ime = 0;
        /* Check all 5 IE and IF bits for flag confirmations
           This loop also services interrupts by priority (0 = highest) */
        uint8_t i;
        uint8_t addr = 0x40;
        for (i = IF_VBlank; i <= IF_Joypad; i <<= 1)
        {
            const uint8_t requestAddress = addr;
            const uint8_t flag = i;
            addr += 8;

            if ((io_IE & flag) && (io_IF & flag))
            {
                gb->sp -= 2;
                CPU_WW(gb->sp, gb->pc);
                gb->pc = requestAddress;

                /* Clear flag bit */
                gb->io[IntrFlags].r = io_IF & ~flag;
                gb->rt += 20;
                break;
            }
        }
    }
}

void gb_update_timer(struct GB *gb, const uint8_t change)
{
    /* Increment div every m-cycle and save bits 6-13 to DIV register */
    gb->divClock = (change) ? gb->divClock + 1 : 0;
    gb->io[Divider].r = (gb->divClock >> 8);

    gb->lastDivClock = gb->divClock;

    /* Leave if timer is disabled */
    const uint8_t tac = gb->io[TimerCtrl].r;
    if ((tac & 4) == 0)
        return;

    /* Update timer, check bits for 1024, 16, 64, or 256 cycles respectively */
    const uint8_t checkBits[4] = {9, 3, 5, 7};
    const uint8_t cBit = checkBits[tac & 0x3];

    if (FALLING_EDGE(gb->lastDivClock >> cBit, gb->divClock >> cBit))
    {
        /* Request timer interrupt if pending */
        if (++gb->io[TimA].r == 0)
            gb->nextTimA_IRQ = 1;
    }
}

/* Update DIV register */

inline void gb_update_div(struct GB *gb)
{
    gb->divClock += gb->rt;

    while (gb->divClock >= 256)
    {
        const uint8_t lastDiv = gb->io[Divider].r;
        ++gb->io[Divider].r;
        gb->divClock -= 256;

        if (lastDiv & 0x10 && !(gb->io[Divider].r & 0x10))
            gb_update_div_apu(gb);
    }
}

static const uint16_t TAC_INTERVALS[4] = {1024, 16, 64, 256};

/* Update TIMA register */

inline void gb_update_timer_simple(struct GB *gb)
{
    if ((gb->io[TimerCtrl].r & 4) == 0) /* TIMA counter disabled */
        return;

    const uint16_t clockRate = TAC_INTERVALS[gb->io[TimerCtrl].r & 3];
    gb->timAClock += gb->rt;

    /* Exit when clock didn't pass interval */
    while (gb->timAClock >= clockRate)
    {
        /* Reset clock */
        gb->timAClock -= clockRate;
        /* Increment TIMA and request interrupt on TIMA overflow */
        if (++gb->io[TimA].r == 0)
        {
            gb->io[IntrFlags].r |= IF_Timer;
            gb->io[TimA].r = gb->io[TMA].r;
        }
    }
}

void gb_handle_timers(struct GB *gb)
{
    gb->newTimALoaded = 0;

    if (gb->nextTimA_IRQ)
    {
        /* Check overflow in the next cycle */
        if (--gb->nextTimA_IRQ == 0)
        {
            gb->io[IntrFlags].r |= IF_Timer;
            gb->io[TimA].r = gb->io[TMA].r;
            gb->newTimALoaded = 1;
        }
    }

    gb_update_timer(gb, 1);
}

/*
 *****************  PPU functions  *****************
 */

/* Used for comparing and setting PPU mode timings */
enum
{
    Stat_HBlank = 0,
    Stat_VBlank,
    Stat_OAM_Search,
    Stat_Transfer
}
modes;

enum
{
    TICKS_OAM_READ = 80,
    TICKS_TRANSFER = 252,
    TICKS_HBLANK   = 456,
    TICKS_VBLANK   = 456
}
modeTicks;

/* Custom pixel palette values for the frontend */

enum
{   
    PIXEL_BG   = 4,
    PIXEL_OBJ1 = 8,
    PIXEL_OBJ2 = 12
};

struct sprite_data
{
    uint8_t sprite_number;
    uint8_t x;
};

//#define HIGH_SORT_ACCURACY
#ifdef HIGH_SORT_ACCURACY
int compare_sprites(const void *in1, const void *in2)
{
    const struct sprite_data *sd1, *sd2;
    int x_res;

    sd1 = (struct sprite_data *)in1;
    sd2 = (struct sprite_data *)in2;
    x_res = (int)sd1->x - (int)sd2->x;
    if (x_res != 0)
        return x_res;

    return (int)sd1->sprite_number - (int)sd2->sprite_number;
}
#endif

#define PPU_GET_TILE(pixelX, X)\
    /* fetch next tile */\
    posX = X;\
    tileID = gb->vram[(tileMap & 0x1FFF) + (posX >> 3)];\
    px = pixelX;\
    /* Select addressing mode */\
    const uint16_t bit12 = !(gb->io[LCDControl].BG_Win_Data || (tileID & 0x80)) << 12;\
    const uint16_t tile = bit12 + (tileID << 4) + ((posY & 7) << 1);\
    \
    rowLSB = gb->vram[tile] >> px;\
    rowMSB = gb->vram[tile + 1] >> px;\
    /* End fetch tile macro */

/* Stored BG Palette values */
uint8_t bgpValues[172 >> 3];

#define BGP_VAL    gb->io[BGPalette].r

static inline uint8_t *gb_pixel_fetch(struct GB *gb)
{
    uint8_t *pixels = gb->extData.pixelLine;
    //assert(gb->io[LY] < DISPLAY_HEIGHT);

    /* If background is enabled, draw it. */
    if (gb->io[LCDControl].BG_Win_Enable)
    {
        uint8_t lineX, posX, tileID, px, rowLSB, rowMSB;

        /* Calculate current background line to draw */
        const uint8_t posY = gb->io[LY].r + gb->io[ScrollY].r;

        /* BG tile fetcher gets tile ID. Bits 0-4 define X loction, bits 5-9 define Y location
         * All related calculations following are found here:
         * https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md */

        /* Get selected background map address for first tile
         * corresponding to current line  */
        const uint16_t tileMap =
            (gb->io[LCDControl].BG_Area ? 0x9C00 : 0x9800) + ((posY >> 3) << 5);

        lineX = DISPLAY_WIDTH - 1;
        if (gb->io[LCDControl].Window_Enable &&
            gb->io[LY].r >= gb->io[WindowY].r && gb->io[WindowX].r <= 166)
            lineX = (gb->io[WindowX].r < 7 ? 0 : gb->io[WindowX].r - 7) - 1;

        PPU_GET_TILE(7 - (posX & 7), lineX + gb->io[ScrollX].r)
        const uint8_t end = 0xFF;

        for (; lineX != end; lineX--)
        {
            if (px % 8 == 0)
            {
                PPU_GET_TILE(0, lineX + gb->io[ScrollX].r)
            }
            /* Get background color */
            uint8_t palIndex = (rowLSB & 0x1) | ((rowMSB & 0x1) << 1);
            pixels[lineX] = ((BGP_VAL >> (palIndex << 1)) & 3) | PIXEL_BG;

            rowLSB >>= 1;
            rowMSB >>= 1;
            px++;
        }
    }

    /* draw window */
    if (gb->io[LCDControl].Window_Enable && gb->io[LY].r >= gb->io[WindowY].r && gb->io[WindowX].r <= 166)
    {
        uint8_t lineX, posX, px, tileID, rowLSB, rowMSB;

        /* Calculate Window Map Address. */
        const uint16_t tileMap = 
            (gb->io[LCDControl].Window_Area ? 0x9C00 : 0x9800) + ((gb->windowLY >> 3) << 5);

        lineX = DISPLAY_WIDTH - 1;
        const uint8_t posY = gb->windowLY & 7;

        PPU_GET_TILE(7 - (posX & 7), lineX - gb->io[WindowX].r + 7)
        const uint8_t end = (gb->io[WindowX].r < 7 ? 0 : gb->io[WindowX].r - 7) - 1;

        for (; lineX != end; lineX--)
        {
            if (px % 8 == 0)
            {
                PPU_GET_TILE(0, lineX - gb->io[WindowX].r + 7)
            }
            uint8_t palIndex = (rowLSB & 0x1) | ((rowMSB & 0x1) << 1);
            pixels[lineX] = (((BGP_VAL) >> (palIndex << 1)) & 3) | PIXEL_BG;

            rowLSB >>= 1;
            rowMSB >>= 1;
            px++;
        }
        gb->windowLY++;
    }

#define MAX_SPRITES_LINE 10
#define NUM_SPRITES 40
#define SPRITE_SORTED

    if (gb->io[LCDControl].OBJ_Enable)
    {
        uint8_t sprite;
        uint8_t totalSprites = 0;

        struct sprite_data sprites_to_render[NUM_SPRITES];

        /* Record number of sprites on the line being rendered, limited
         * to the maximum number sprites that the Game Boy is able to
         * render on each line (10 sprites). */
#ifdef SPRITE_SORTED
        for (sprite = 0; sprite < (sizeof(sprites_to_render) / sizeof(sprites_to_render[0])); sprite++)
        {
            uint8_t objY = gb->oam[4 * sprite];
            uint8_t objX = gb->oam[4 * sprite + 1];

            /* If sprite isn't on this line, continue */
            if (gb->io[LY].r + (gb->io[LCDControl].OBJ_Size ? 0 : 8) >= objY || gb->io[LY].r + 16 < objY)
                continue;

            sprites_to_render[totalSprites].sprite_number = sprite;
            sprites_to_render[totalSprites].x = objX;
            totalSprites++;
        }
#endif
        uint8_t sprites[10];
        memset(sprites, 0, 10);
        totalSprites = 0;

        /* Find available sprites that could be visible on this line */
        uint8_t s;
        for (s = 0; s < OAM_SIZE; s += 4)
        {
            if (gb->oam[s] == 0 && gb->oam[s] >= 160)
                continue;
            if (gb->io[LY].r >= gb->oam[s] - (gb->io[LCDControl].OBJ_Size ? 0 : 8) ||
                gb->io[LY].r < gb->oam[s] - 16)
                continue;

            sprites[totalSprites++] = s;
            if (totalSprites == 10)
                break;
        }
#ifdef HIGH_SORT_ACCURACY
        /* If maximum number of sprites reached, prioritise X
         * coordinate and object location in OAM. */
        qsort(&sprites_to_render[0], totalSprites,
              sizeof(sprites_to_render[0]), compare_sprites);
#endif
        if (totalSprites > MAX_SPRITES_LINE)
            totalSprites = MAX_SPRITES_LINE;

        /* Sprites are rendered from low priority to high priority */
        for (sprite = totalSprites - 1; sprite != 0xFF; sprite--)
        {
            const uint8_t s = sprites[sprite];

            const uint8_t objY = gb->oam[s + 0], objX = gb->oam[s + 1];
            const uint8_t objTile = gb->oam[s + 2] & 
                (gb->io[LCDControl].OBJ_Size ? 0xFE : 0xFF);
            const uint8_t objFlags = gb->oam[s + 3];

            /* Skip sprite if not visible */
            if (objX == 0 || objX >= DISPLAY_WIDTH + 8)
                continue;

            /* Handle Y flip */
            uint8_t posY = gb->io[LY].r - objY + 16;
            if (objFlags & 0x40)
                posY = (gb->io[LCDControl].OBJ_Size ? 15 : 7) - posY;

            const uint8_t rowLSB = gb->vram[(objTile << 4) + (posY << 1)];
            const uint8_t rowMSB = gb->vram[(objTile << 4) + (posY << 1) + 1];

            const uint8_t sLeft = (objX - 8 < 0) ? 0 : objX - 8;
            const uint8_t sRight = (objX >= DISPLAY_WIDTH) ? DISPLAY_WIDTH : objX;

            /* Loop through tile row pixels */
            uint8_t lineX;
            for (lineX = sLeft; lineX != sRight; lineX++)
            {
                /* handle X flip */
                const uint8_t relX = (gb->oam[s + 3] & 0x20) ? lineX - objX : 7 - (lineX - objX);
                const uint8_t palIndex =
                    ((rowLSB >> (relX & 7)) & 1) |
                    (((rowMSB >> (relX & 7)) & 1) << 1);

                /* Handle sprite priority */
                if (palIndex && !(objFlags & 0x80 && !((pixels[lineX] & 0x3) == (BGP_VAL & 3))))
                {
                    /* Set pixel based on palette */
                    pixels[lineX] = (objFlags & 0x10) ?
                        ((gb->io[OBJPalette1].r >> (palIndex << 1)) & 3):
                        ((gb->io[OBJPalette0].r >> (palIndex << 1)) & 3);

                    pixels[lineX] &= 3;
                    pixels[lineX] |= (objFlags & 0x10) ? PIXEL_OBJ1 : PIXEL_OBJ2;
                }
            }
        }
    }
    return pixels;
}

#undef MAX_SPRITES_LINE
#undef NUM_SPRITES

#define _FORCE_INLINE __attribute__((always_inline)) inline

_FORCE_INLINE void gb_oam_read(struct GB *gb)
{
    /* Mode 2 - OAM read */
    if (IO_STAT_MODE != Stat_OAM_Search)
    {
        IO_STAT_MODE = Stat_OAM_Search;
        if (gb->io[LCDStatus].stat_OAM)
            gb->io[IntrFlags].r |= IF_LCD_STAT; /* Mode 2 interrupt */
        /* Fetch OAM data for sprites to be drawn on this line */
        // ppu_OAM_fetch (ppu, io_regs);
    }
}

_FORCE_INLINE void gb_transfer(struct GB *gb)
{
    const uint8_t bgpIndex = gb->lineClock - TICKS_OAM_READ;
    bgpValues[bgpIndex >> 3] = gb->io[BGPalette].r;
    
    /* Mode 3 - Transfer to LCD */
    if (IO_STAT_MODE != Stat_Transfer)
    {
        IO_STAT_MODE = Stat_Transfer;
    }
}

/* Evaluate LY==LYC */

//_FORCE_INLINE void gb_eval_LYC(struct GB *const gb)
#define GB_EVAL_LYC(gb)\
    /* Set bit 02 flag for comparing lYC and LY \
       If STAT interrupt is enabled, an interrupt is requested */\
    if (gb->io[LYC].r == gb->io[LY].r)\
    {\
        gb->io[LCDStatus].stat_LYC_LY = 1;\
        if (gb->io[LCDStatus].stat_LYC)\
            gb->io[IntrFlags].r |= IF_LCD_STAT;\
    }\
    else /* Unset the flag */\
        gb->io[LCDStatus].stat_LYC_LY = 0;\

//#define DISABLE_LCD

void gb_render(struct GB *const gb)
{
    gb->lineClock += gb->rt;

    /* Todo: continuously fetch pixels clock by clock for LCD data transfer.
       Similarly do clock-based processing for the other actions. */
    if (gb->io[LY].r < DISPLAY_HEIGHT)
    {
        /* Visible line, within screen bounds */
        if (gb->lineClock < TICKS_OAM_READ)
            gb_oam_read(gb);
        else if (gb->lineClock < TICKS_TRANSFER)
            gb_transfer(gb);
        else if (gb->lineClock < TICKS_HBLANK)
        { /* Mode 0 - H-blank */
            if (IO_STAT_MODE != Stat_HBlank)
            {
                IO_STAT_MODE = Stat_HBlank;
                /* Mode 0 interrupt */
                if (gb->io[LCDStatus].stat_HBlank)
                    gb->io[IntrFlags].r |= IF_LCD_STAT;

                if ((!gb->io[LCDControl].LCD_Enable) || (gb->extData.frameSkip &&
                    (gb->totalFrames % (gb->extData.frameSkip + 1) > 0)))
                    return;
                    
#ifndef DISABLE_LCD
                /* Fetch line of pixels for the screen and draw them */
                gb_pixel_fetch(gb);
                if (gb->totalFrames % (gb->extData.frameSkip + 1) == 0)
                    gb->draw_line (gb->extData.ptr, gb->extData.pixelLine, gb->io[LY].r);
#endif
            }
        }
        else
        { /* Starting new line */
            gb->lineClock -= TICKS_HBLANK;
            gb->io[LY].r = (gb->io[LY].r + 1) % SCAN_LINES;
            GB_EVAL_LYC(gb);
        }
    }
    else /* Outside of screen Y bounds */
    {
        if (gb->lineClock < TICKS_VBLANK)
        {
            if (IO_STAT_MODE != Stat_VBlank)
            {
                /* Enter Vblank and indicate that a frame is completed */
                IO_STAT_MODE = Stat_VBlank;
                gb->io[IntrFlags].r |= IF_VBlank;
                /* Mode 1 interrupt */
                if (gb->io[LCDStatus].stat_VBlank)
                    gb->io[IntrFlags].r |= IF_LCD_STAT;
            }
        }
        else
        { /* Starting new line */
            gb->io[LY].r = (gb->io[LY].r + 1) % SCAN_LINES;
            gb->lineClock -= TICKS_VBLANK;
            if (gb->io[LY].r > DISPLAY_HEIGHT)
                gb->windowLY = 0; /* Reset window Y counter if line is 0 */
            GB_EVAL_LYC(gb);
        }
    }
    /* ...and DMA transfer to OMA, if needed */
}

/*
 *****************  APU functions  *****************
 */

#define CH_POS       (n * 5)
#define PERIOD_MAX   2048

void gb_init_audio (struct GB * const gb)
{
    gb->io[Ch1_Sweep].r = 0x80;
    gb->io[Ch1_LD].r   = 0xBF;
    gb->io[Ch1_Vol].r  = 0xF3;
    
    gb->io[Ch1_Period].r = gb->io[Ch2_Period].r = 
    gb->io[Ch3_Period].r = 0xFF;
    gb->io[Ch1_Ctrl].r = gb->io[Ch2_Ctrl].r = 
    gb->io[Ch3_Ctrl].r = gb->io[Ch4_Ctrl].r = 0xBF;
    gb->io[Ch3_Length].r = gb->io[Ch4_Length].r = 0xFF;

    gb->io[Ch2_LD].r   = 0x3F;
    gb->io[Ch2_Vol].r  = 0x0;
    gb->io[Ch3_DAC].r  = 0x7F;
    gb->io[Ch3_Vol].r  = 0x9F;
    gb->io[Ch4_Vol].r  = gb->io[Ch4_Freq].r = 0x0;
}

void gb_ch_trigger (struct GB * const gb, const uint8_t n)
{
    //LOG_("Trigger channel %d\n", n);

    gb->audioCh[n].enabled = 1;
    gb->audioCh[n].currentVol = gb->io[CH_POS + Ch1_Vol].Volume;

    /* Length timer reset */
    if (gb->audioCh[n].lengthTick > (n == 2 ? 255 : 63))
        gb->audioCh[n].lengthTick = gb->io[CH_POS + Ch1_LD].Length;

    /* Pulse trigger reset */
    if (n < 2)
    {
        const uint8_t periodH = gb->io[CH_POS + Ch1_Ctrl].PeriodH;

        gb->audioCh[n].envTick = 0;
        gb->audioCh[n].periodTick = (gb->io[CH_POS + Ch1_Period].r | (periodH << 8));
    }

    /* Sweep reset */
    if (n == 0)
    {

    }
}

void gb_update_div_apu (struct GB * const gb)
{
    gb->apuDiv++;

    if (!(gb->apuDiv & 7)) {
        /* Envelope sweep, 64 Hz */
        int n;
        for (n = 0; n < 4; n++)
        {
            if (n == 2) continue;

            gb->audioCh[n].envTick++;
            if (gb->audioCh[n].envTick == gb->io[CH_POS + Ch1_Vol].EnvPace)
            {
                const int8_t volStep = 
                    (gb->io[CH_POS + Ch1_Vol].EnvDir * 2) - 1;
                gb->audioCh[n].currentVol += volStep;
                gb->audioCh[n].envTick = 0;
            }
        }
    }

    if (!(gb->apuDiv & 1)) {
        /* Length ++, 256 Hz */
        int n;
        for (n = 0; n < 4; n++)
        {
            if (gb->audioCh[n].enabled && gb->io[Ch1_Ctrl + CH_POS].Len_Enable)
                gb->audioCh[n].lengthTick++;

            if (gb->audioCh[n].lengthTick == ((n == 2) ? 256 : 64))
                gb->audioCh[n].enabled = 0;
        }
    }

    if (!(gb->apuDiv & 3)) {
        /* Ch 1 sweep ++, 128 Hz */
    }
}

void gb_update_audio (struct GB * const gb)
{
    gb->apuClock += gb->rt;
    const uint8_t step = gb->rt >> 2;
    //const uint8_t stepWav = gb->rt >> 1;

    /* Update pulse channels */
    uint8_t n;
    for (n = 0; n < 2; n++)
    {
        if (!gb->audioCh[n].DAC || !gb->audioCh[n].enabled)
            continue;

        gb->audioCh[n].periodTick += step;

        while (gb->audioCh[n].periodTick > PERIOD_MAX)
        {
            const uint16_t period =  
                (gb->io[Ch1_Ctrl + CH_POS].PeriodH << 8) |
                gb->io[Ch1_Period + CH_POS].r;

            gb->audioCh[n].periodTick -= PERIOD_MAX - period;
        }
    }
}

#undef CH_POS
#undef PERIOD_MAX