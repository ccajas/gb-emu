#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include "gb.h"
#include "ops.h"

#define CPU_INSTRS

/*
 **********  Memory/bus read and write  ************
 ===================================================
*/

inline uint8_t gb_ppu_rw (struct GB * gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) {
        if (addr >= 0xFE00) return (!gb->oamBlocked)  ? gb->oam[addr & 0x9F]    : 0xFF;
        else                return (!gb->vramBlocked) ? gb->vram[addr & 0x1FFF] : 0xFF;
    }
    else {
        if (addr >= 0xFE00 && !gb->oamBlocked) gb->oam[addr & 0x9F]    = val;
        else if             (!gb->vramBlocked) gb->vram[addr & 0x1FFF] = val;
    }
    return 0;
}

#define RISING_EDGE(before, after)   ((before & 1) < (after & 1))
#define FALLING_EDGE(before, after)  ((before & 1) > (after & 1))

inline uint8_t gb_io_rw (struct GB * gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) 
        return gb->io[addr % 0x80];
    else
    {
        switch (addr % 0x80)
        {
            case TimA:
                if (!gb->newTimALoaded) gb->io[TimA] = val;    /* Update TIMA if new value wasn't loaded last cycle    */
                if (gb->nextTimA_IRQ)   gb->nextTimA_IRQ = 0;  /* Cancel any pending IRQ when accessing TIMA           */
                return 0;
            case TMA:                                          /* Update TIMA also if TMA is written in the same cycle */
                if (gb->newTimALoaded) gb->io[TimA] = val;
                break;
            case TimerCtrl:
                gb->io[TimerCtrl] = val | 0xF8; return 0;
            case IntrFlags:                               /* Mask unused bits for IE and IF        */
            case IntrEnabled:
                gb->io[addr % 0x80] = val | 0xE0; return 0;
            case BootROM:                 /* Boot ROM register should be unwritable at some point? */
                break;
            case Divider:
                gb_timer_update (gb, 0); return 0;        /* DIV reset                             */
            case DMA:                                     /* OAM DMA transfer                      */
                gb->io[DMA] = val; int i = 0;             /* Todo: Make it write across 160 cycles */
                const uint16_t src = val << 8;
                while (i < OAM_SIZE) { gb->oam[i] = CPU_RB (src + i); i++; } return 0;
        }
        gb->io[addr % 0x80] = val;
    }
    return 0;
}

uint8_t gb_mem_access (struct GB * gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    /* For debug logging purposes */
    //if (addr == 0xFF44 && !write) return 0x90;

    /* For Blargg's CPU instruction tests */
#ifdef CPU_INSTRS
    //if (gb->io[SerialCtrl] == 0x81) { LOG_("%c", gb->io[SerialData]); gb->io[SerialCtrl] = 0x0; }
#endif
    /* Byte to be accessed from memory */
    uint8_t * b;
    #define DIRECT_RW(b)  if (write) { *b = val; } return *b;
    struct Cartridge * cart = &gb->cart;

    if (addr < 0x0100 && gb->io[BootROM] == 0)                     /* Run boot ROM if needed */
        { if (!write) { return gb->bootRom[addr]; } else { return 0; }}
    if (addr < 0x8000)  return cart->rw (cart, addr, val, write);       /* ROM from MBC     */
    if (addr < 0xA000)  return gb_ppu_rw (gb, addr, val, write);        /* Video RAM        */
    if (addr < 0xC000)  return cart->rw (cart, addr, val, write);       /* External RAM     */
    if (addr < 0xE000)  { b = &gb->ram[addr % 0x2000]; DIRECT_RW(b); }  /* Work RAM         */
    if (addr < 0xFE00)  return 0xFF;                                    /* Echo RAM         */
    if (addr < 0xFEA0)  return gb_ppu_rw (gb, addr, val, write);        /* OAM              */
    if (addr < 0xFF00)  return 0xFF;                                    /* Not usable       */
    if (addr == 0xFF00) return gb_joypad (gb, val, write);              /* Joypad           */
    if (addr < 0xFF80)  return gb_io_rw (gb, addr, val, write);         /* I/O registers    */                      
    if (addr < 0xFFFF)  { b = &gb->hram[addr % 0x80]; DIRECT_RW(b); }   /* High RAM         */  
    if (addr == 0xFFFF) { b = &gb->io[addr % 0x80];   DIRECT_RW(b); }   /* Interrupt enable */

    #undef DIRECT_RW
    return 0;
}

/*
 **********  Console startup functions  ************
 ===================================================
*/

void gb_init (struct GB * gb, uint8_t * bootRom)
{
    cart_identify (&gb->cart);
    printf ("GB: ROM mask: %d\n", gb->cart.romMask);

    /* Initialize RAM and settings */
    memset (gb->ram,  0, WRAM_SIZE);
    memset (gb->vram, 0, VRAM_SIZE);
    memset (gb->hram, 0, HRAM_SIZE);
    gb->vramBlocked = gb->oamBlocked = 0;

    memset(gb->io, 0, sizeof (gb->io));
    printf ("Memset done\n");

    if (bootRom != NULL)
        gb_reset (gb, bootRom);
    else
        gb_boot_reset (gb);

    gb_cpu_state (gb);
    gb->lineClock = gb->frameClock = 0;
    gb->clock_t = gb->clock_m = 0;
    gb->divClock = 0;
    gb->frame = 0;
    gb->pcInc = 1;
    printf ("CPU state done\n");
}

void gb_reset (struct GB * gb, uint8_t * bootROM)
{
    printf ("GB: Load Boot ROM\n");
    printf ("GB: Set I/O\n");

    gb->bootRom = bootROM;
    gb->extData.joypad = 0xFF;
    gb->io[Joypad]     = 0xCF;
    gb_boot_register (gb, 0);

    gb->pc = 0;
    gb->ime = 0;
    gb->invalid = 0;
    gb->halted = 0;
}

void gb_boot_reset (struct GB * gb)
{
    gb->bootRom = NULL;
    gb->extData.joypad = 0xFF;

    /* Setup CPU registers as if bootrom was loaded */
    gb->r[A]  = 0x01;
    gb->flags = 0xB0;
    gb->r[B]  = 0x0;
    gb->r[C]  = 0x13;
    gb->r[D]  = 0x0;
    gb->r[E]  = 0xD8;
    gb->r[H]  = 0x01;
    gb->r[L]  = 0x4D;
    gb->sp    = 0xFFFE;
    gb->pc    = 0x0100;

    gb->ime = 1;
    gb->invalid = 0;
    gb->halted = 0;

    printf ("GB: Launch without boot ROM\n");
    printf ("GB: Set I/O\n");

    /* Initalize I/O registers (DMG) */
    memset(gb->io, 0, sizeof (gb->io));  
    gb->io[Joypad]     = 0xCF;
    gb->io[SerialCtrl] = 0x7E;
    gb->io[Divider]    = 0x18;
    gb->io[TimerCtrl]  = 0xF8;
    gb->io[IntrFlags]  = 0xE1;
    gb->io[LCDControl] = 0x91;
    gb->io[LCDStatus]  = 0x81;
    gb->io[LY]         = 0x90;
    gb->io[BGPalette]  = 0xFC;
    gb->io[DMA]        = 0xFF;
    gb_boot_register (gb, 1);
}

const int8_t opTicks[256] = {
/*  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  A,  B,  C,  D,  E,  F  */
    1,  3,  2,  2,  1,  1,  2,  1,  5,  2,  2,  2,  1,  1,  2,  1, /* 0x */
	0,  3,  2,  2,  1,  1,  2,  1,  3,  2,  2,  2,  1,  1,  2,  1,
	2,  3,  2,  2,  1,  1,  2,  1,  2,  2,  2,  2,  1,  1,  2,  1,
	2,  3,  2,  2,  3,  3,  3,  1,  2,  2,  2,  2,  1,  1,  2,  1,

	1,  1,  1,  1,  1,  1,  2,  1,  1,  1,  1,  1,  1,  1,  2,  1, /* 4x */
	1,  1,  1,  1,  1,  1,  2,  1,  1,  1,  1,  1,  1,  1,  2,  1,
	1,  1,  1,  1,  1,  1,  2,  1,  1,  1,  1,  1,  1,  1,  2,  1,
	2,  2,  2,  2,  2,  2,  0,  2,  1,  1,  1,  1,  1,  1,  2,  1,

	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1, /* 8x */
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,

	2,3,3,4,3,4,2,4,2,4,3,0,3,6,2,4, /* Cx */
	2,3,3,0,3,4,2,4,2,4,3,0,3,0,2,4,
	3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
	3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4
};

/*
 *****************  CPU functions  *****************
 ===================================================
*/

void gb_cpu_exec (struct GB * gb, const uint8_t op)
{
    const uint8_t opL = op & 0xf;
    const uint8_t opHh = op >> 3; /* Octal divisions */

    uint8_t * r8_g[] = { 
        gb->r + 2, gb->r + 3, gb->r + 4, gb->r + 5, gb->r + 6,  gb->r + 7, &gb->flags, gb->r };

    /* Default values for operands (can be overridden for other opcodes) */
    uint8_t * reg1 = r8_g[opHh & 7];
    uint8_t * reg2 = r8_g[opL & 7];
    uint8_t tmp = gb->r[A];

    /* HALT bug skips PC increment, essentially rollback one byte */
    if (!gb->pcInc) {
        gb->pc--;
        gb->pcInc = 1;
    }

    switch (opHh)
    {
        case 0 ... 7:
        case 0x18 ... 0x1F:
        {
            /* For conditional jump / call instructions */
            const uint8_t cond = ((op >> 3) & 3);

            switch (op)
            {
                case 0x0:  NOP     break;
                /* 16-bit load, arithmetic instructions */
                case 0x01:   case 0x11:
                case 0x21:   case 0x31: LDrr    break;
                case 0x02:                case 0x12: LDrrmA  break; 
                case 0x22: LDHLIA  break; case 0x32: LDHLDA  break;
                case 0x03:   case 0x13:
                case 0x23:   case 0x33: INCrr   break;
                case 0x09:   case 0x19:
                case 0x29:   case 0x39: ADHLrr  break;
                case 0x0B:   case 0x1B:
                case 0x2B:   case 0x3B: DECrr   break;
                /* Increment, decrement, LD r8,n, RST */
                case 0x04:   case 0x0C:   case 0x14:
                case 0x1C:   case 0x24:   case 0x2C:
                case 0x34:   case 0x3C:              INC_    break;
                case 0x05:   case 0x0D:   case 0x15:
                case 0x1D:   case 0x25:   case 0x2D:
                case 0x35:   case 0x3D:              DEC_    break;
                case 0x06:   case 0x0E:   case 0x16:
                case 0x1E:   case 0x26:   case 0x2E:
                case 0x36:   case 0x3E:              LDrm_   break;
                case 0x07: RLCA    break; case 0x08: LDmSP   break;
                case 0x0A:                case 0x1A: LDArrm  break;
                case 0x2A: LDAHLI  break; case 0x3A: LDAHLD  break;
                case 0x0F: RRCA    break; 
                
                case 0x10: STOP    break; case 0x17: RLA     break; 
                case 0x18: JRm     break; case 0x1F: RRA     break; 
                case 0x27: DAA     break;
                case 0x2F: CPL     break; 

                case 0x37: SCF     break; case 0x3F: CCF     break;
                /* ... */
                case 0xC3: JPNN    break; case 0xC6: ADDm    break;
                case 0xC9: RET     break; 
                case 0xCD: CALLm   break; case 0xCE: ADCm    break;

                case 0xD6: SUBm    break; case 0xD9: RETI    break;
                case 0xDE: SBCm    break;

                case 0xE0: LDIOmA  break; case 0xE2: LDIOCA  break;
                case 0xE6: ANDm    break; case 0xE8: ADDSPm  break;
                case 0xE9: JPHL    break; case 0xEA: LDmmA   break;
                case 0xEE: XORm    break;

                case 0xF0: LDAIOm  break; case 0xF2: LDAIOC  break;
                case 0xF3: DI      break; case 0xF6: ORm     break;
                case 0xF8: LDHLSP  break; case 0xF9: LDSPHL  break;
                case 0xFA: LDAmm   break; case 0xFB: EI      break;
                case 0xFE: CPm     break;
                /* Conditional jump, return/call */
                case 0x20:   case 0x28:
                case 0x30:   case 0x38: JR_ (cond)  break;
                case 0xC0:   case 0xC8:
                case 0xD0:   case 0xD8: RET_(cond)  break;
                case 0xC2:   case 0xCA:
                case 0xD2:   case 0xDA: JP_(cond)   break;
                case 0xC4:   case 0xCC:
                case 0xD4:   case 0xDC: CALL_(cond) break;
                case 0xC7:   case 0xCF:   case 0xD7:
                case 0xDF:   case 0xE7:   case 0xEF:
                case 0xF7:   case 0xFF: RST         break;
                case 0xCB:   
                    gb_exec_cb (gb, CPU_RB (gb->pc++));
                break;
                default:   INVALID;
            }
            switch (op)
            {
                R16_OPS_3(POPrr_,  0xC1);
                R16_OPS_3(PUSHrr_, 0xC5);
            }
        }
        break;
        case 8 ... 0xD: case 0xF:
            /* 8-bit load, LD or LDrHL */
            //OPR_2_(LD_r8, LD_rHL)
            *reg1 = ((op & 7) == 6) ? 
                CPU_RB(ADDR_HL) : *reg2;
        break;
        case 0xE:
            /* 8-bit load, LDHLr or HALT */ 
            switch (op) {
                OP_R8_G (0x70)
                    OP(LDHLr); CPU_WB (ADDR_HL, *reg2); break;
                case 0x76: 
                    OP(HALT);
                    if (!gb->ime) {
                        if (gb->io[IntrEnabled] && gb->io[IntrFlags] & IF_Any) gb->pcInc = 0; 
                        else gb->halted = 1;
                    } else {
                        gb->halted = 1;
                    }
                break;
            }
            //OPR_2_(LD_HLr, HALT)
        break;
        case 0x10 ... 0x17: {
            /* 8-bit arithmetic */
            switch (op & 0xF8) 
            {
                case 0x80: OPR_2_(ADD_A_r8, ADD_A_HL) break;
                case 0x88: OPR_2_(ADC_A_r8, ADC_A_HL) break;
                case 0x90: OPR_2_(SUB_A_r8, SUB_A_HL) break;
                case 0x98: OPR_2_(SBC_A_r8, SBC_A_HL) break;
                case 0xA0: OPR_2_(AND_A_r8, AND_A_HL) break;
                case 0xA8: OPR_2_(XOR_A_r8, XOR_A_HL) break;
                case 0xB0: OPR_2_(OR_A_r8,  OR_A_HL ) break;
                case 0xB8: OPR_2_(CP_A_r8,  CP_A_HL ) break;
            }
        }
        break;
    }

    gb->rm += opTicks[op];

    /* Handle effects of STOP instruction */
    if (op == 0x10 && gb->stop)
    {
        gb->stop = 0;
        /* Todo: Read joypad button selection/press */
        gb->divClock = 0;
    }

    assert (gb->rm >= opTicks[op]);  
}

void gb_exec_cb (struct GB * gb, const uint8_t op)
{
    gb->rm = 3;

    const uint8_t opL  = op & 0xf;
    const uint8_t opHh = op >> 3; /* Octal divisions */

    uint8_t * r8_g[] = { 
        gb->r + 2, gb->r + 3, gb->r + 4, gb->r + 5,  gb->r + 6,  gb->r + 7, &gb->flags,  gb->r };

    const uint8_t r_bit  = opHh & 7;
    uint8_t * reg1 = r8_g[opL & 7];

    /* Fetch value at address (HL) if it's needed */
    uint8_t hl = (opL == 0x6 || opL == 0xE) ? CPU_RB (ADDR_HL) : 0;

    switch (opHh)
    {
        case 0 ... 7:
            switch (op & 0b00111000)
            {
                case 0:    OPR_2_(RLC,  RLCHL)  break;
                case 8:    OPR_2_(RRC,  RRCHL)  break;
                case 0x10: OPR_2_(RL,   RLHL)   break;
                case 0x18: OPR_2_(RR,   RRHL)   break;
                case 0x20: OPR_2_(SLA,  SLAHL)  break;
                case 0x28: OPR_2_(SRA,  SRAHL)  break;
                case 0x30: OPR_2_(SWAP, SWAPHL) break;
                case 0x38: OPR_2_(SRL,  SRLHL ) break;
            }
        break;
        case 8 ... 0xF:     OPR_2_(BIT, BITHL) break; /* Bit test  */
        case 0x10 ... 0x17: OPR_2_(RES, RESHL) break; /* Bit reset */
        case 0x18 ... 0x1F: OPR_2_(SET, SETHL) break; /* Bit set   */
    }
    /* Write back to (HL) for most (HL) operations except BIT */
    if ((op & 7) == 6 && (opHh < 8 || opHh > 0xF))
        CPU_WB (ADDR_HL, hl);
}

void gb_handle_interrupts (struct GB * gb)
{
    /* Get interrupt flags */
    const uint8_t io_IE = gb->io[IntrEnabled];
    const uint8_t io_IF = gb->io[IntrFlags];

    /* Run if CPU ran HALT instruction or IME enabled w/flags */
    if (io_IE & io_IF & IF_Any)
    {
        gb->ime = 0;
        /* Check all 5 IE and IF bits for flag confirmations 
           This loop also services interrupts by priority (0 = highest) */
        uint8_t i;
        uint16_t addr = 0x40;
        for (i = IF_VBlank; i <= IF_Joypad; i <<= 1)
        {
            const uint16_t requestAddress = addr;
            const uint8_t flag = i;
            addr += 8;

            if ((io_IE & flag) && (io_IF & flag))
            {
                gb->clock_t += 8;
                gb->sp -= 2;

                CPU_WW (gb->sp, gb->pc);
                gb->clock_t += 8;
                gb->pc = requestAddress;
                gb->clock_t += 4;

                /* Clear flag bit */
                gb->io[IntrFlags] = io_IF & ~flag;
                break;
            }
        }
    }
}

void gb_timer_update (struct GB * gb, const uint8_t change)
{
    /* Increment div every m-cycle and save bits 6-13 to DIV register */
    gb->divClock = (change) ? gb->divClock + 1 : 0 ;
    gb->io[Divider] = (gb->divClock >> 6) & 0xFF;

    /* Leave if timer is disabled */
    const uint8_t tac = gb->io[TimerCtrl];
    if (!(tac & 0x4)) return;

    /* Update timer, check bits for 1024, 16, 64, or 256 cycles respectively */
    const uint8_t checkBits[4] = { 7, 1, 3, 5 };
    const uint8_t cBit = checkBits[tac & 0x3];

    if (FALLING_EDGE (gb->lastDiv >> cBit, gb->divClock >> cBit))
    {
        uint8_t timA = gb->io[TimA];
        /* Request timer interrupt if pending */
        if (++timA == 0) gb->nextTimA_IRQ = 1;
        gb->io[TimA] = timA;
    }
    gb->lastDiv = gb->divClock;
}

void gb_handle_timings (struct GB * gb)
{
    gb->newTimALoaded = 0;

    if (gb->nextTimA_IRQ) 
    {
        /* Check overflow in the next cycle */
        gb->nextTimA_IRQ--;
        if (gb->nextTimA_IRQ == 0)
        {
            gb->io[IntrFlags] |= IF_Timer;
            gb->io[TimA] = gb->io[TMA];
            gb->newTimALoaded = 1;
        }
    }

    gb_timer_update (gb, 1);
}

/*
 *****************  PPU functions  *****************
 ===================================================
*/

/* Used for comparing and setting PPU mode timings */
enum {
    Stat_HBlank = 0,
    Stat_VBlank,
    Stat_OAM_Search,
    Stat_Transfer
}
modes;

enum {
    TICKS_OAM_READ    = 80,
    TICKS_TRANSFER    = 252,
    TICKS_HBLANK      = 456,
    TICKS_VBLANK      = 456
}
modeTicks;

/* Custom pixel palette values for the frontend */

enum {
    PIXEL_BG   = 4,
    PIXEL_OBJ1 = 8,
    PIXEL_OBJ2 = 12
};

static inline uint8_t * gb_pixel_fetch (const struct GB * gb)
{
    uint8_t * pixels = calloc(DISPLAY_WIDTH, sizeof(uint8_t));
    assert (gb->io[LY] < DISPLAY_HEIGHT);

    const uint8_t lineY = gb->io[LY];
    uint8_t sprites[10];
    uint8_t visibleSprites = 0;
    memset(sprites, 0, 10);

    /* Find available sprites that could be visible on this line */
    uint8_t s;
    for (s = 0; s < OAM_SIZE; s += 4)
    {
        if (gb->oam[s] == 0 && gb->oam[s] >= 160) continue;
        if (lineY >= gb->oam[s] - (LCDC_(2) ? 0 : 8) || 
            lineY < gb->oam[s] - 16) continue;
        
        sprites[visibleSprites++] = s;
        if (visibleSprites == 10) break;
    }

	/* Check if background is enabled */
	if (LCDC_(LCD_Enable) && LCDC_(BG_Win_Enable))
	{
        /* Get minimum starting address depends on LCDC bits 3 and 6 are set.
        Starts at 0x1800 as this is the VRAM index minus 0x8000 */
        uint16_t BGTileMap  = (LCDC_(BG_Area))     ? 0x9C00 : 0x9800;
        uint16_t winTileMap = (LCDC_(Window_Area)) ? 0x9C00 : 0x9800;

        /* X position counter */
        uint8_t lineX = 0;

        /* For storing pixel bytes */
        uint8_t byteLo = 0;
        uint8_t byteHi = 0;

        /* Run for the entire 160 pixel length */
        for (lineX = 0; lineX < DISPLAY_WIDTH; lineX++)
        {
            /* BG tile fetcher gets tile ID. Bits 0-4 define X loction, bits 5-9 define Y location
            All related calculations following are found here:
            https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md */

            /* Stop BG rendering if window is found here */
            const uint8_t isWindow = (LCDC_(Window_Enable) && lineX >= gb->io[WindowX] - 7 && lineY >= gb->io[WindowY]);

            const uint8_t posX = (isWindow) ? lineX : lineX + gb->io[ScrollX];
            const uint8_t posY = (isWindow) ? gb->windowLY - gb->io[WindowY] : lineY + gb->io[ScrollY];
            uint8_t relX = (isWindow) ? gb->io[WindowX] - 7 + (lineX % 8) : posX % 8;

            /* Get next tile to be drawn */
            if (lineX == 0 || relX == 0)
            {
                const uint16_t tileAddr = ((isWindow) ? winTileMap : BGTileMap) + 
                    ((posY >> 3) << 5) +  /* Bits 5-9, Y location */
                    (posX >> 3);          /* Bits 0-4, X location */
                const uint16_t tileID = gb->vram[tileAddr & 0x1FFF];

                /* Tilemap location depends on LCDC 4 set, which are different rules for BG and Window tiles */
                /* Fetcher gets low byte and high byte for tile */
                const uint16_t bit12 = !(LCDC_(4) || (tileID & 0x80)) << 12;
                const uint16_t tileRow = bit12 + (tileID << 4) + ((posY & 7) << 1);

                /* Finally get the pixel bytes from these addresses */
                byteLo = gb->vram[tileRow];
                byteHi = gb->vram[tileRow + 1];
            }

            /* Produce pixel data from the combined bytes*/
            const uint8_t bitLo = (byteLo >> (7 - relX)) & 1;
            const uint8_t bitHi = (byteHi >> (7 - relX)) & 1;
            const uint8_t bgIndex = (bitHi << 1) + bitLo;
            /* Add 4 (set bit 2) to denote background pixel */
            pixels[lineX] = ((gb->io[BGPalette] >> (bgIndex << 1)) & 3) | ((isWindow) ? 0 : PIXEL_BG);
        }

        /* Draw sprites */
        if (LCDC_(OBJ_Enable) && visibleSprites > 0)
        {
            int8_t obj;
            for (obj = visibleSprites - 1; obj >= 0; obj--) 
            {
                const uint8_t s = sprites[obj];
                const uint8_t spriteX = gb->oam[s + 1], spriteY = gb->oam[s];

                if (lineY - (LCDC_(OBJ_Size) ? 0 : 8) >= spriteY || lineY < spriteY - 16) continue;
                if (spriteX == 0 || spriteX >= DISPLAY_WIDTH + 8) continue;

                const uint8_t sLeft = (spriteX - 8 < 0) ? 0 : spriteX - 8;
                const uint8_t sRight = (spriteX  >= DISPLAY_WIDTH) ? DISPLAY_WIDTH : spriteX;
                const uint8_t posY = (lineY - gb->oam[s]) & 7;

                /* Get tile ID depending on sprite size */
                uint16_t tileID = gb->oam[s + 2] & (LCDC_(2) ? 0xFE : 0xFF);
                if (LCDC_(OBJ_Size) && spriteY - lineY <= 8) tileID = gb->oam[s + 2] | 1;

                /* Flip Y if necessary */
                const uint8_t relY = (gb->oam[s + 3] & 0x40) ? (LCDC_(OBJ_Size) ? 15 : 7) : 0;
                const uint16_t tileRow = (tileID << 4) + ((posY ^ relY) << 1);

                /* Get the pixel bytes from these addresses */
                byteLo = gb->vram[tileRow];
                byteHi = gb->vram[tileRow + 1];

                for (lineX = sLeft; lineX < sRight; lineX++)
                {
                    /* Flip X if necessary */
                    const uint8_t relX = (gb->oam[s + 3] & 0x20) ? lineX - spriteX : 7 - (lineX - spriteX);

                    /* Produce pixel data from the combined bytes*/
                    const uint8_t bitLo = (byteLo >> (relX & 7)) & 1;
                    const uint8_t bitHi = (byteHi >> (relX & 7)) & 1;
                    const uint8_t objIndex = (bitHi << 1) + bitLo;

                    if (objIndex == 0) continue;
                    if (gb->oam[s + 3] & 0x80 && pixels[lineX] & 0x3) continue;

                    const uint8_t palette = OBJPalette0 + !!(gb->oam[s + 3] & 0x10);
                    pixels[lineX] = ((gb->io[palette] >> (objIndex << 1)) & 3) | PIXEL_OBJ1;
                }
            }
        }

        /* Leave prematurely if window is disabled */
        //if (!LCDC_(5)) return pixels;
    }

    return pixels;
}

static inline void gb_oam_read (struct GB * gb) 
{
    /* Mode 2 - OAM read */
    if (IO_STAT_MODE != Stat_OAM_Search)
    {
        gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_OAM_Search;
        if STAT_(IR_OAM) gb->io[IntrFlags] |= IF_LCD_STAT;      /* Mode 2 interrupt */
        /* Increment Window Y counter when window is enabled   */
        if (LCDC_(Window_Enable)) gb->windowLY++;
        /* Fetch OAM data for sprites to be drawn on this line */
        //ppu_OAM_fetch (ppu, io_regs);
    }
}

static inline void gb_transfer (struct GB * gb)
{
    /* Mode 3 - Transfer to LCD */
    if (IO_STAT_MODE != Stat_Transfer)
    {
        gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_Transfer;
        if (gb->frame || !LCDC_(LCD_Enable)) return;
        /* Fetch line of pixels for the screen and draw them */
        uint8_t * pixels = gb_pixel_fetch (gb);
        gb->draw_line (gb->extData.ptr, pixels, gb->io[LY]);
    }
}

static inline void gb_hblank (struct GB * gb)
{ 
    /* Mode 0 - H-blank */
    if (IO_STAT_MODE != Stat_HBlank)
    {
        gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_HBlank;
        /* Mode 0 interrupt */
        if STAT_(IR_HBlank) gb->io[IntrFlags] |= IF_LCD_STAT;
    }
}

/* Evaluate LY==LYC */

static inline void gb_eval_LYC (struct GB * const gb)
{
    /* Set bit 02 flag for comparing lYC and LY
       If STAT interrupt is enabled, an interrupt is requested */
    if (gb->io[LYC] == gb->io[LY])
    {
        gb->io[LCDStatus] |= (1 << LYC_LY);
        if (STAT_(IR_LYC)) gb->io[IntrFlags] |= IF_LCD_STAT;
    }
    else /* Unset the flag */
        gb->io[LCDStatus] &= ~(1 << LYC_LY);
}

static inline void gb_vblank (struct GB * const gb) 
{
    if (gb->lineClock < TICKS_VBLANK)
    {
        /* Mode 1 - V-blank */
        if (IO_STAT_MODE != Stat_VBlank)
        {
            /* Enter Vblank and indicate that a frame is completed */
            if (LCDC_(LCD_Enable)) 
            {
                gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_VBlank;
                gb->io[IntrFlags] |= IF_VBlank;
                /* Mode 1 interrupt */
                if STAT_(IR_VBLank) gb->io[IntrFlags] |= IF_LCD_STAT;
            }
        }
    }
    else
    {
        /* Starting new line */
        gb->io[LY] = (gb->io[LY] + 1) % SCAN_LINES;
        gb->lineClock -= TICKS_VBLANK;
        if (!gb->io[LY]) gb->windowLY = -1; /* Reset window Y counter if line is 0 */
        gb_eval_LYC (gb);
    }
}

void gb_render (struct GB * const gb)
{
    gb->lineClock  += gb->rt;
    gb->frameClock += gb->rt;

    /* Todo: continuously fetch pixels clock by clock for LCD data transfer.
       Similarly do clock-based processing for the other actions. */
    if (gb->io[LY] < DISPLAY_HEIGHT)
    {
        /* Visible line, within screen bounds */
        if      (gb->lineClock < TICKS_OAM_READ) gb_oam_read (gb);
        else if (gb->lineClock < TICKS_TRANSFER) gb_transfer (gb);
        else if (gb->lineClock < TICKS_HBLANK)   gb_hblank (gb);
        else
        {   /* Starting new line */
            gb->lineClock -= TICKS_HBLANK;
            gb->io[LY] = (gb->io[LY] + 1) % SCAN_LINES;
            gb_eval_LYC (gb);
        }
    }
    else /* Outside of screen Y bounds */
        gb_vblank (gb);
    /* ...and DMA transfer to OMA, if needed */
}