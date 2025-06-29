#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "gb.h"
#include "ops.h"

#if defined(ASSERT_INSTR_TIMING) || !defined(USE_INC_MCYCLE)
#include "opcycles.h"
#endif

#define _FORCE_INLINE __attribute__((always_inline)) inline

/*
 **********  Memory/bus read and write  ************
 */
#define RISING_EDGE(before, after)  ((before & 1) < (after & 1))
#define FALLING_EDGE(before, after) ((before & 1) > (after & 1))

uint8_t gb_io_rw(struct GB *gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    const uint8_t reg = addr & 0xFF;

    if (!write)
    {
#ifdef ENABLE_AUDIO
        if (reg >= NR10 && reg <= Wave + 0xF)
            return gb_apu_rw(gb, reg, val, write);
#endif
        if (reg == IntrEnabled && gb->io[reg].r == 0)
            return gb->io[reg].r;

        const uint8_t bitmask = 
            (reg == Joypad)      ? 0xC0 :
            (reg == SerialCtrl)  ? 0x7E :
            (reg == IntrFlags)   ? 0xE0 :
            (reg == LCDStatus)   ? 0x80 :
            (reg == IntrEnabled) ? 0xE0 : 0;

        /* Return bitmasked value */
        return gb->io[reg].r | bitmask;
    }
    else
    {
        switch (reg)
        {
#ifdef USE_TIMER_SIMPLE
            case Divider:
                gb->io[Divider].r = 0;
                break;
            case TimA:
                gb->io[TimA].r = val;
                break;
            case TMA:
                gb->io[TMA].r = val;
                break;
#else
            case Divider:
                gb_update_timer(gb, 0);
                break; /* DIV reset                             */
            case TimA:
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
            case TimerCtrl: /* TODO: TimA should increase right here if last bit was 1 and current is 0  */
                gb->io[TimerCtrl].r = val | 0xF8;
                break;
            case IntrFlags: /* Mask unused bits for IE and IF         */
                gb->io[reg].r = val | 0xE0;
                break;
            /* APU registers */
            case NR10 ... Wave + 0xF:
#ifdef ENABLE_AUDIO
                gb_apu_rw(gb, addr & 0xFF, val, write);
#else
                gb->io[reg].r = val;
#endif
                break;
            /* PPU registers */
            case LCDControl:
            { /* Check whether LCD will be turned on or off */
                const uint8_t lcdEnabled =  gb->io[LCDControl].LCD_Enable;
                if (lcdEnabled && !(val & (1 << LCD_Enable)))
                {
                    LOG_("GB: [ ] LCD turn off (%d:%d)\n", gb->totalFrames, gb->io[LY].r);
                    IO_STAT_MODE = 0; /* Clear STAT mode when turning off LCD       */
                    gb->io[LY].r = 0;
                }
                else if (!lcdEnabled && (val & (1 << LCD_Enable)))
                {
                    LOG_("GB: [#] LCD turn on  (%d:%d)\n", gb->totalFrames, gb->io[LY].r);
                }
                gb->io[LCDControl].r = val;
                break;
            }
            case LY: /* Writing to LY resets line counter      */
                gb->io[LY].r = val;
                break;
            case DMA: /* OAM DMA transfer     */
                gb->io[DMA].r = val;
                int i = 0; /* TODO: Make it write across 160 cycles */
                const uint16_t src = val << 8;
                while (i < OAM_SIZE)
                {
                    gb->oam[i] = CPU_RB(src + i);
                    i++;
                }
                break;
            case 3: /* Other unmapped registers */
            case 0x8  ... 0xE:
            case 0x4C ... 0x7F:
                gb->io[addr & 0xFF].r = val | 0xFF;
                break;
            default:
                gb->io[addr & 0xFF].r = val;
        }
    }
    return 0;
}

inline uint8_t gb_apu_rw(struct GB *gb, const uint8_t reg, const uint8_t val, const uint8_t write)
{
    if (!write)
    {
        //LOG_("APU value %02x read from %02x\n", gb->io[reg].r, reg);
        //LOG_("--> With OR %02x: %02x\n\n", 
        //    apu_bitmasks[reg - NR10], gb->io[reg].r | apu_bitmasks[reg - NR10]);
        return gb->io[reg].r | apu_bitmasks[reg - NR10];
    }
    else
    {
#ifdef ENABLE_AUDIO
        //LOG_("== Write %02x to APU: %02x\n", val, reg);
        if (reg == AudioCtrl)
        {
            gb->io[AudioCtrl].r = val & 0x80;

            if (!(gb->io[AudioCtrl].r >> 7))
            {
                int i = 0;
                while (i < 4) gb->audioCh[i++].enabled = 0;
                /* Clear registers */
                for (i = NR10; i < AudioCtrl; i++)
                    gb->io[i].r = 0;
            }
            return 0;
        }

        /* APU registers are read only when audio is turned off */
        if (gb->io[AudioCtrl].r == 0)
        {
            //LOG_("Audio is off, failed write (%02x)\n\n", reg);
            return 0;
        }

        gb->io[reg].r = val;

        switch (reg)
        {            
            case NR11:
            case NR21:
            case NR41: /* Channel 1,2,4 length */
            {
                const uint8_t channel = (reg - 0x10) / 5;
                gb->io[(channel * 5) + NR11].r = val;
                break;
            }
            case NR12:
            case NR22:
            case NR42: /* Channel 1,2,4 volume */
            {
                const uint8_t channel = (reg - 0x10) / 5;
                gb->io[(channel * 5) + NR12].r = val;
                gb->audioCh[channel].currentVol = val >> 4;
                gb->audioCh[channel].DAC = ((val & 0xF8) == 0) ? 0 : 1;
                break;
            }
            case NR32:
                gb->io[NR32].r = val;
                gb->audioCh[2].currentVol = (val >> 5) & 3;
                break;

            case NR14:
            case NR24:
            case NR34:
            case NR44: /* Channel control */
            {
                const uint8_t channel = (reg - 0x10) / 5;
                gb->io[(channel * 5) + NR14].r = val;
                if (val >> 7)
                    gb_ch_trigger(gb, channel);       
                break;
            }
        }
#endif
    }
    return 0;
}

static const uint16_t TAC_intervals[4] = {1024, 16, 64, 256};

#define UPDATE_TIMER_SIMPLE(gb, cycles)\
    if (gb->io[TimerCtrl].TAC_Enable)\
    {\
        const uint16_t clockRate = TAC_intervals[gb->io[TimerCtrl].TAC_clock];\
        gb->timAClock += cycles;\
        if (gb->timAClock >= clockRate)\
            while (gb->timAClock >= clockRate)\
            {\
                gb->timAClock -= clockRate;\
                if (++gb->io[TimA].r == 0)\
                {\
                    gb->io[IntrFlags].r |= IF_Timer;\
                    gb->io[TimA].r = gb->io[TMA].r;\
                }\
            }\
    }

uint8_t gb_mem_read(struct GB *gb, const uint16_t addr)
{
    const uint8_t val = 0;
    INC_MCYCLE;

#if !defined(FAST_TIMING) && defined(USE_TIMER_SIMPLE)
    ++gb->readWrite;
    gb->divClock += 4;
    UPDATE_TIMER_SIMPLE(gb, 4);
#endif

    /* For Blargg's CPU instruction tests */
#ifdef CPU_INSTRS_TESTING
    if (addr == 0xFF44)
        return 0x90;
    if (gb->io[SerialCtrl] == 0x81)
    {
        LOG_("%c", gb->io[SerialData]);
        gb->io[SerialCtrl] = 0x0;
    }
#endif

    if (addr < 0x0100 && gb->io[BootROM].r == 0) /* Run boot ROM if needed */
    {
        return gb->bootRom[addr];
    }
    if (addr < 0x8000)
    {
        return gb->cart.rw(&gb->cart, addr, val, 0); /* ROM from MBC     */
    }
    if (addr < 0xA000)
    {
        if (!gb->vramAccess)
            return 0xFF;
 
        return gb->vram[addr & 0x1FFF];
    } /* Video RAM        */
    if (addr < 0xC000)
    {
        return gb->cart.rw(&gb->cart, addr, val, 0); /* External RAM     */
    }
    if (addr < 0xE000)
    {
        return gb->ram[addr % WRAM_SIZE];
    } /* Work RAM         */
    if (addr < 0xFE00)
    {
        return gb->ram[addr % WRAM_SIZE];
    } /* Echo RAM         */
    if (addr < 0xFEA0) /* OAM              */
    {
        if (!gb->oamAccess)
            return 0xFF;

        return gb->oam[addr - 0xFE00];
    }
    if (addr < 0xFF00)
        return 0xFF; /* Not usable       */
    if (addr == 0xFF00)
        return gb_joypad(gb, val, 0);      /* Joypad           */
    if (addr < 0xFF80)
        return gb_io_rw(gb, addr, val, 0); /* I/O registers    */
    if (addr < 0xFFFF)
    {
        return gb->hram[addr % HRAM_SIZE];
    } /* High RAM         */
    if (addr == 0xFFFF)
        return gb->io[IntrEnabled].r;
        //return gb_io_rw(gb, addr, val, 0); /* Interrupt enable */
    return 0;
}

uint8_t gb_mem_write(struct GB *gb, const uint16_t addr, const uint8_t val)
{
    INC_MCYCLE;
    
#if !defined(FAST_TIMING) && defined(USE_TIMER_SIMPLE)
    ++gb->readWrite;
    gb->divClock += 4;
    UPDATE_TIMER_SIMPLE(gb, 4);
#endif

    /* For Blargg's CPU instruction tests */
#ifdef CPU_INSTRS_TESTING
    if (gb->io[SerialCtrl] == 0x81)
    {
        LOG_("%c", gb->io[SerialData]);
        gb->io[SerialCtrl] = 0x0;
    }
#endif

    if (addr < 0x0100 && gb->io[BootROM].r == 0) /* Run boot ROM if needed */
    {
        return 0;
    }
    if (addr < 0x8000)
    {
        return gb->cart.rw(&gb->cart, addr, val, 1); /* ROM from MBC     */
    }
    if (addr < 0xA000)
    {
        if (!gb->vramAccess)
            return 0xFF;

        gb->vram[addr & 0x1FFF] = val;
        return 0;
    } /* Video RAM        */
    if (addr < 0xC000)
    {
        return gb->cart.rw(&gb->cart, addr, val, 1); /* External RAM     */
    }
    if (addr < 0xE000)
    {
        gb->ram[addr % WRAM_SIZE] = val;
        return 0;
    } /* Work RAM         */
    if (addr < 0xFE00)
    {
        gb->ram[addr % WRAM_SIZE] = val;
        return 0;
    } /* Echo RAM         */
    if (addr < 0xFEA0) /* OAM              */
    {
        if (!gb->oamAccess)
            return 0xFF;

        gb->oam[addr - 0xFE00] = val;
        return 0;
    }
    if (addr < 0xFF00)
        return 0xFF; /* Not usable       */
    if (addr == 0xFF00)
        return gb_joypad(gb, val, 1);      /* Joypad           */
    if (addr < 0xFF80)
        return gb_io_rw(gb, addr, val, 1); /* I/O registers    */
    if (addr < 0xFFFF)
    {
        gb->hram[addr % HRAM_SIZE] = val;
    } /* High RAM         */
    if (addr == 0xFFFF)
        gb->io[IntrEnabled].r = val;
        //return gb_io_rw(gb, addr, val, 1); /* Interrupt enable */
    return 0;
}

/*
 **********  Console startup functions  ************
 */

void gb_init(struct GB *gb, uint8_t *bootRom)
{
    cart_identify(&gb->cart);
    LOG_("GB: ROM mask: %d\n", gb->cart.romMask);
    memcpy(gb->extData.title, gb->cart.title, sizeof(gb->cart.title));

    /* Initialize RAM and settings */
    memset(gb->ram, 0, WRAM_SIZE);
    memset(gb->vram, 0, VRAM_SIZE);
    memset(gb->hram, 0, HRAM_SIZE);
    memset(gb->oam, 0, OAM_SIZE);

    memset(gb->io, 0, sizeof(gb->io));
    LOG_("GB: Memset done\n");

    if (bootRom != NULL)
        gb_reset(gb, bootRom);
    else
        gb_boot_reset(gb);

    LOG_CPU_STATE(gb, 0);
    gb->lineClock = 0;
    gb->lineClockSt = 0;
    gb->clock_t = 0;
    gb->divClock = gb->timAClock = 0;
    gb->totalFrames = 0;
    gb->apuDiv = 0;
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
    gb->halted = 0;
    gb->stopped = 0;

    gb->vramAccess = gb->oamAccess = 1;
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
    gb->imePending = 0;
    gb->halted = 0;
    gb->stopped = 0;

    gb->vramAccess = gb->oamAccess = 1;

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

    switch (opHh)
    {
        case 8 ... 0xF:
            switch (op)
            {
                /* Halt */
                case 0x76:
                    OP(HALT)
                    gb->halted = 1;
                    if (!gb->ime)
                    {
                        if (gb->io[IntrEnabled].r & gb->io[IntrFlags].r & IF_Any)
                            gb->pcInc = gb->halted = 0;
                    }
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
                {
                    const uint8_t cond = ((op >> 3) & 3);
                    JR_(cond) break;
                }
                case 0xC0:
                case 0xC8:
                case 0xD0:
                case 0xD8:
                {
                    const uint8_t cond = ((op >> 3) & 3);
                    RET_(cond) break;
                }
                case 0xC2:
                case 0xCA:
                case 0xD2:
                case 0xDA:
                {
                    const uint8_t cond = ((op >> 3) & 3);
                    JP_(cond) break;
                }
                case 0xC4:
                case 0xCC:
                case 0xD4:
                case 0xDC:
                {
                    const uint8_t cond = ((op >> 3) & 3);
                    CALL_(cond) break;
                }
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
                    gb_exec_cb(gb, CPU_RB(gb->pc));
                    break;
                default:
                    INVALID
            }
    }

    /* Handle effects of STOP instruction */
    /* TODO: Read joypad button selection/press */
    if (op == 0x10 && gb->stopped)
    {
        gb->stopped = 0;
        gb_mem_write(gb, 0xFF00 + gb->io[Divider].r, 0);
        //gb_mem_access(gb, 0xFF00 + gb->io[Divider].r, 0, 1);
    }

#ifndef USE_INC_MCYCLE
    gb->rm += opCycles[op];
#endif
#ifdef ASSERT_INSTR_TIMING
    assert(gb->rm >= opCycles[op]);
#endif

    gb->rt = gb->rm * 4;
}

void gb_exec_cb(struct GB *gb, const uint8_t op_cb)
{
    #ifndef USE_INC_MCYCLE
    ++gb->rm;
    #endif

    ++gb->pc;
    const uint8_t opHh = op_cb >> 3; /* Octal divisions */
    const uint8_t r_bit = opHh & 7;

    /* Fetch value at address (HL) if it's needed */
    uint8_t hl = 0;

    switch (op_cb)
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
    if ((op_cb & 7) == 6 && (opHh < 8 || opHh > 0xF))
    {
        #ifndef USE_INC_MCYCLE
        ++gb->rm;
        #endif
        CPU_WB(REG_HL, hl);
    }
}

/* Update TIMA register */

inline void gb_update_timer_simple(struct GB *gb, const uint16_t cycles)
{
    if (!gb->io[TimerCtrl].TAC_Enable) /* TIMA counter disabled */
        return;

    const uint16_t clockRate = TAC_intervals[gb->io[TimerCtrl].TAC_clock];
    gb->timAClock += cycles;

    /* Exit when clock didn't pass interval */
    if (gb->timAClock >= clockRate)
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

/* Custom pixel palette values for the frontend */

enum
{   
    PIXEL_BG   = 4,
    PIXEL_OBJ1 = 8,
    PIXEL_OBJ2 = 12
};

struct sprite_data
{
    uint8_t oamEntry;
    uint8_t x;
};

#define HIGH_SORT_ACCURACY

#ifdef HIGH_SORT_ACCURACY
int compare_sprites(const void * in1, const void * in2)
{
    const struct sprite_data * sd1, * sd2;
    int x_res;

    sd1 = (struct sprite_data*)in1;
    sd2 = (struct sprite_data*)in2;
    x_res = (int)sd1->x - (int)sd2->x;

    if (x_res != 0)
        return x_res;

    return (int)sd1->oamEntry - (int)sd2->oamEntry;
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
//uint8_t bgpValues[172 >> 3];
//uint8_t bgAreaValues[172 >> 3];

#define BGP_VAL       gb->io[BGPalette].r
#define BGAREA_VAL    gb->io[LCDControl].BG_Area
//bgpValues[lineX >> 3]

static const int_fast16_t bgWinMapAddr[2] = { 0x9800, 0x9C00 };

uint8_t * gb_pixels_fetch(struct GB *gb)
{
    uint8_t *pixels = gb->extData.pixelLine;
    //assert(gb->io[LY] < DISPLAY_HEIGHT);

    /* If background is enabled, draw it. */
    if (gb->io[LCDControl].BG_Win_Enable)
    {
        //if (!(gb->io[LCDControl].Window_Enable && gb->io[LY].r >= gb->io[WindowY].r))
        //{    
            /* BG tile fetcher gets tile ID. Bits 0-4 define X loction, bits 5-9 define Y location
             * All related calculations following are found here:
             * https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md */
    
            uint8_t lineX = DISPLAY_WIDTH - 1;
            if (gb->io[LCDControl].Window_Enable &&
                gb->io[LY].r >= gb->io[WindowY].r && gb->io[WindowX].r <= 166)
            {
                lineX = (gb->io[WindowX].r < 7 ? 0 : gb->io[WindowX].r - 7) - 1;
            }
    
            /* Calculate current background line to draw */
            const uint8_t posY = gb->io[LY].r + gb->io[ScrollY].r;
            /* Get selected background map address for first tile
             * corresponding to current line  */
            uint16_t tileMap = bgWinMapAddr[BGAREA_VAL] | ((posY >> 3) << 5);

            uint8_t posX, px, tileID, rowLSB, rowMSB;
            PPU_GET_TILE(7 - (posX & 7), lineX + gb->io[ScrollX].r)
            const uint8_t end = 0xFF;
    
            for (; lineX != end; --lineX)
            {
                if ((px & 7) == 0)
                {
                    tileMap = bgWinMapAddr[BGAREA_VAL] | ((posY >> 3) << 5);
                    PPU_GET_TILE(0, lineX + gb->io[ScrollX].r)
                }
                /* Get background color */
                uint8_t palIndex = (rowLSB & 0x1) | ((rowMSB & 0x1) << 1);
                pixels[lineX] = ((BGP_VAL >> (palIndex << 1)) & 3) | PIXEL_BG;
    
                rowLSB >>= 1;
                rowMSB >>= 1;
                ++px;
            }
        //}
    }

    /* draw window */
    if (gb->io[LCDControl].Window_Enable && 
        gb->io[LY].r >= gb->io[WindowY].r && gb->io[WindowX].r <= 166)
    {
        /* Calculate Window Map Address. */
        const uint16_t tileMap = bgWinMapAddr[gb->io[LCDControl].Window_Area] |
                ((gb->windowLY >> 3) << 5);

        uint8_t lineX = DISPLAY_WIDTH - 1;
        const uint8_t posY = gb->windowLY & 7;

        uint8_t posX, px, tileID, rowLSB, rowMSB;
        PPU_GET_TILE(7 - (posX & 7), lineX - gb->io[WindowX].r + 7)
        const uint8_t end = (gb->io[WindowX].r < 7 ? 0 : gb->io[WindowX].r - 7) - 1;

        for (; lineX != end; --lineX)
        {
            if ((px & 7) == 0)
            {
                PPU_GET_TILE(0, lineX - gb->io[WindowX].r + 7)
            }
            uint8_t palIndex = (rowLSB & 0x1) | ((rowMSB & 0x1) << 1);
            pixels[lineX] = (((BGP_VAL) >> (palIndex << 1)) & 3) | PIXEL_BG;

            rowLSB >>= 1;
            rowMSB >>= 1;
            ++px;
        }
        ++gb->windowLY;
    }

#define MAX_SPRITES_LINE 10
#define NUM_SPRITES 40

    if (gb->io[LCDControl].OBJ_Enable)
    {
        uint8_t totalSprites = 0;

        /* Record number of sprites on the line being rendered, limited
         * to the maximum number sprites that the Game Boy is able to
         * render on each line (10 sprites). */
        struct sprite_data sprites[MAX_SPRITES_LINE];

        /* Find available sprites that could be visible on this line */
        uint8_t s;
        for (s = 0; s < OAM_SIZE; s += 4)
        {
            if (gb->oam[s + 1] == 0 && gb->oam[s + 1] >= DISPLAY_WIDTH)
                continue;
            if (gb->io[LY].r >= gb->oam[s] - (gb->io[LCDControl].OBJ_Size ? 0 : 8) ||
                gb->io[LY].r < gb->oam[s] - 16)
                continue;

            sprites[totalSprites].oamEntry = s;
            sprites[totalSprites++].x = gb->oam[s + 1];

            if (totalSprites == MAX_SPRITES_LINE)
                break;
        }
#ifdef HIGH_SORT_ACCURACY
        /* Sort sprites by coordinate and object location in OAM.
         * TODO: Maybe forgo qsort for a sparse array if performance differs */
        qsort(sprites, totalSprites, sizeof(sprites[0]), compare_sprites);
#endif
        /* Sprites are rendered from low priority to high priority */
        for (s = totalSprites - 1; s != 0xFF; s--)
        {
            const uint8_t entry = sprites[s].oamEntry;
            const uint8_t objX = gb->oam[entry + 1];

            /* Skip sprite if not visible */
            if (objX == 0 || objX >= DISPLAY_WIDTH + 8)
                continue;

            const uint8_t objY = gb->oam[entry + 0];
            /* Handle Y flip */
            const uint8_t objFlags = gb->oam[entry + 3];
            uint8_t posY = gb->io[LY].r - objY + 16;
            if (objFlags & 0x40)
                posY = (gb->io[LCDControl].OBJ_Size ? 15 : 7) - posY;

            const uint8_t objTile = (gb->oam[entry + 2] & 
                (gb->io[LCDControl].OBJ_Size ? 0xFE : 0xFF));

            const uint8_t rowLSB = gb->vram[(objTile << 4) | (posY << 1)];
            const uint8_t rowMSB = gb->vram[(objTile << 4) | ((posY << 1) + 1)];

            const uint8_t sLeft = (objX < 8) ? 8 : objX;
            const uint8_t sRight = (objX >= DISPLAY_WIDTH) ? DISPLAY_WIDTH : objX;

            /* Loop through tile row pixels */
            uint8_t lineX;
            for (lineX = sLeft - 8; lineX != sRight; lineX++)
            {
                /* handle X flip */
                const uint8_t relX = (gb->oam[entry + 3] & 0x20) ? lineX - objX : 7 - (lineX - objX);
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
                    pixels[lineX] |= (objFlags & 0x10) ? PIXEL_OBJ2 : PIXEL_OBJ1;
                }
            }
        }
    }
    return pixels;
}

#undef MAX_SPRITES_LINE
#undef NUM_SPRITES

inline void gb_oam_read(struct GB *gb)
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

inline void gb_transfer(struct GB *gb)
{
    //const uint8_t bgpIndex = gb->lineClock - TICKS_OAM_READ;
    //bgpValues[bgpIndex >> 3]    = gb->io[BGPalette].r;
    //bgAreaValues[bgpIndex >> 3] = gb->io[LCDControl].BG_Area;
    
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


#define PPU_PACE  TICKS_HBLANK / 6

void gb_render(struct GB *const gb)
{
    gb->lineClock += gb->rt;

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
#if ENABLE_LCD
                /* Fetch line of pixels for the screen and draw them */
                gb_pixels_fetch(gb);
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
                gb->drawFrame = 1;
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

#ifdef ENABLE_AUDIO

#define PERIOD_MAX       2048
#define LENGTH_MAX       64
#define LENGTH_MAX_WAVE  256

#endif

void gb_init_audio (struct GB * const gb)
{
#ifdef ENABLE_AUDIO
    LOG_("GB: Init audio\n");\
    
    int i;
    for (i = 0; i < 4; i++)
        gb->audioCh[i].periodTick = 0;
#endif
    gb->io[NR10].r = 0x80;
    gb->io[NR11].r = 0xBF;
    gb->io[NR12].r = 0xF3;
    
    gb->io[NR13].r = gb->io[NR23].r = 
    gb->io[NR33].r = 0xFF;
    gb->io[NR14].r = gb->io[NR24].r = 
    gb->io[NR34].r = gb->io[NR44].r = 0xBF;
    gb->io[NR31].r = gb->io[NR41].r = 0xFF;

    gb->io[NR21].r = 0x3F;
    gb->io[NR22].r = 0x0;
    gb->io[NR30].r = 0x7F;
    gb->io[NR32].r = 0x9F;
    gb->io[NR42].r = gb->io[NR43].r = 0x0;
}

void gb_ch_trigger (struct GB * const gb, const uint8_t n)
{
#ifdef ENABLE_AUDIO
    const uint8_t ch_pos = n * 5;
    const uint8_t periodH = gb->io[ch_pos + NR14].PeriodH;
    const uint16_t period = gb->io[ch_pos + NR13].r | (periodH << 8);

/*
    Action                          1 2 3 4
    ---------------------------------------
    Enable channel                  X X X X
    Reset length timer if expired   X X X X
    Set period to NRx3 and NRx4     X X X -
    Reset envelope timer            X X - X
    Set volume to NRx2              X X X X
    Sweep-related updates           X - - -
    Reset Wave RAM index            - - X -
    Reset LSFR bits                 - - - X
*/
    switch (n + 1)
    {
        case 1:
        {
            /* Sweep reset */
            const uint8_t pace = gb->io[NR10].SweepPace;
            gb->sweepBck = period;
            gb->sweepTick = (pace == 0) ? 8 : pace;
    
            if (gb->io[NR10].SweepStep > 0 || gb->io[NR10].SweepPace > 0)
                gb->sweepEnabled = 1;
            else
                gb->sweepEnabled = 0;
        }
        case 2:
            /* Pulse trigger reset */
            gb->audioCh[n].patternStep = 0;
        case 3:
        {
            /* Reset period divider */
            gb->audioCh[n].periodTick = period;
            /* Wave pattern reset */
            if (n + 1 == 3)
            {
                gb->audioCh[2].currentVol = (gb->io[NR32].r >> 5) & 3;
                gb->audioCh[2].patternStep = 1; /* Start at 2nd wave sample */
            }
        }
        case 4:
            /* Enable channel */
            gb->audioCh[n].enabled = 1;
            /* Reset length timer if expired  */
            const uint16_t max = (n == 2 ? LENGTH_MAX_WAVE : LENGTH_MAX);
            if (gb->audioCh[n].lengthTick >= max)
                gb->audioCh[n].lengthTick = gb->io[ch_pos + NR11].Length;
            /* Reset envelope timer */
            if (!(n + 1 == 3))
                gb->audioCh[n].envTick = gb->io[ch_pos + NR12].EnvPace;
            /* Reset volume */
            gb->audioCh[n].currentVol = gb->io[ch_pos + NR12].Volume;
            /* LFSR reset */
            if (n + 1 == 4)
                gb->audioLFSR = 0;
        break;
    }
#endif
}

void gb_update_div_apu (struct GB * const gb)
{
    gb->apuDiv++;
#ifdef ENABLE_AUDIO
    if ((gb->apuDiv & 7) == 7) /* Envelope sweep, 64 Hz */
    {
        int n;
        for (n = 0; n < 4; n++)
        {
            /** TODO: Envelope sweep needs to be fixed */
            const uint8_t ch_pos = n * 5;
            const uint8_t pace = gb->io[ch_pos + NR12].EnvPace;
       
            if (pace == 0) 
            {
                gb->audioCh[n].currentVol = gb->io[ch_pos + NR12].Volume;
                continue;
            }

            gb->audioCh[n].envTick--;

            if (gb->audioCh[n].envTick == 0)
            {
                gb->audioCh[n].envTick = pace;
                const int8_t dir = (gb->io[ch_pos + NR12].EnvDir) ? 1 : -1;
                int8_t vol = gb->audioCh[n].currentVol + dir;

                if (vol > 0xF) vol = 0xF;
                if (vol < 0)   vol = 0;
                gb->audioCh[n].currentVol = vol;
            }
        }
    }

    if ((gb->apuDiv & 1) == 0) /* Length ++, 256 Hz */
    {
        int n;
        for (n = 0; n < 4; n++)
        {
            const uint8_t ch_pos = n * 5;

            if (gb->audioCh[n].enabled && gb->io[NR14 + ch_pos].Len_Enable)
            {
                gb->audioCh[n].lengthTick++;

                const uint16_t max = (n == 2) ? LENGTH_MAX_WAVE : LENGTH_MAX;
                if (gb->audioCh[n].lengthTick == max)
                {
                    gb->audioCh[n].enabled = 0;
                    gb->audioCh[n].lengthTick = 0;
                }
            }
        }
    }

    if ((gb->apuDiv & 3) == 2) /* Ch 1 sweep ++, 128 Hz */
    {
        /** TODO: Finish overflow check */
        const uint8_t pace = gb->io[NR10].SweepPace;
        if (gb->sweepTick > 0)
            gb->sweepTick--;

        if (gb->sweepTick == 0)
        {
            gb->sweepTick = (pace == 0) ? 8 : pace;

            if (gb->sweepEnabled && gb->sweepTick > 0)
            {
                uint16_t newPeriod = gb->sweepBck >> gb->io[NR10].SweepStep;
                /* Check if decrementing */
                newPeriod = gb->sweepBck + ((gb->io[NR10].SweepDir == 1) ? 
                    -newPeriod : newPeriod);

                if (newPeriod < PERIOD_MAX)
                {
                    gb->io[NR14].PeriodH = newPeriod >> 8;
                    gb->io[NR13].r = newPeriod & 255;
                    gb->sweepBck = newPeriod;
                }
            }
        }
    }
#endif
}

#ifdef ENABLE_AUDIO

static const uint8_t dutyCycles[4] = { 0x01, 0x03, 0x0F, 0xFC };
static const uint8_t clkDivider[8] = 
        { 0x8, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70 };

int16_t gb_update_audio (struct GB * const gb, const uint16_t cycles)
{
    /* Use for better performance (lower accuracy) */
    int32_t pulse[2] = {0};
    int32_t sample = 0;

    if (!gb->io[AudioCtrl].Master_on)
        return (int16_t)sample;

    /* Update pulse channels */
    uint8_t n;
    for (n = 0; n < 2; n++)
    {
        if (!gb->audioCh[n].DAC || !gb->audioCh[n].enabled)
            continue;

        const uint8_t ch_pos = n * 5;

        gb->audioCh[n].periodTick += (cycles >> 2);

        while (gb->audioCh[n].periodTick >= PERIOD_MAX)
        {
            const uint16_t period =  
                (gb->io[NR14 + ch_pos].PeriodH << 8) |
                gb->io[NR13 + ch_pos].r;

            gb->audioCh[n].periodTick -= PERIOD_MAX;
            gb->audioCh[n].periodTick += period;
            gb->audioCh[n].patternStep++;
        }

        const uint8_t duty = gb->io[NR11 + ch_pos].Duty;
        
        pulse[n] = dutyCycles[duty] >> (gb->audioCh[n].patternStep & 7);
        pulse[n] = (pulse[n] & 1) ? INT16_MIN : INT16_MAX;
        sample += pulse[n] / 15 * gb->audioCh[n].currentVol;
    }

    /* Update wave channel */
    if (gb->audioCh[2].enabled && (gb->io[NR30].r & 0x80))
    {
        gb->audioCh[2].periodTick += (cycles >> 1);

        while (gb->audioCh[2].periodTick >= PERIOD_MAX)
        {
            const uint16_t period =  
                (gb->io[NR34].PeriodH << 8) | gb->io[NR33].r;

            gb->audioCh[2].periodTick -= PERIOD_MAX;
            gb->audioCh[2].periodTick += period;
            gb->audioCh[2].patternStep++;
        }

        uint8_t patternStep = gb->audioCh[2].patternStep & 31;
        int32_t wav = gb->io[Wave + (patternStep >> 1)].r;
        const uint8_t vol = (gb->io[NR32].r >> 5) & 3;

        wav = (patternStep & 1) ? wav & 0xF : wav >> 4;
        wav = (vol == 1) ? wav :
            (vol == 2) ? wav >> 1 :
            (vol == 3) ? wav >> 2 : 0;

        sample += wav * INT16_MAX / 7;
    }

    /* Update noise channel */
    if (gb->audioCh[3].DAC && gb->audioCh[3].enabled)
    {
        /* "Period" determines how often LFSR is clocked */
        gb->audioCh[3].periodTick += cycles;

        const uint8_t shift = (gb->io[NR43].r >> 4) & 15;
        const uint8_t clock = gb->io[NR43].r & 7;
        const uint32_t freq = clkDivider[clock] << shift;

        assert (freq != 0);
        while (gb->audioCh[3].periodTick >= freq)
        {
            gb->audioCh[3].periodTick -= freq;
            /* Shift LFSR */
            uint16_t nextBit = ~((gb->audioLFSR & 1) ^ ((gb->audioLFSR & 2) >> 1));
            nextBit &= 1;
            gb->audioLFSR >>= 1;
            gb->audioLFSR |= (nextBit << 14);

            /* Adjust LSFR according to width */
            if (gb->io[NR43].r & 8) {
                gb->audioLFSR &= ~(1 << 6);
                gb->audioLFSR |= (nextBit << 6);
            }
        }

        int32_t noise = (gb->audioLFSR & 1) ? INT16_MIN : INT16_MAX;
        sample += (noise / 24) * gb->audioCh[3].currentVol;
    }

    /* Mix channel outputs */
    sample /= (4 << 1);
    return (uint16_t)sample;
}

#undef PERIOD_MAX
#undef LENGTH_MAX
#undef LENGTH_MAX_WAVE

#endif