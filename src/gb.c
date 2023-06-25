#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include "gb.h"
#include "ops.h"

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

#define GB_PPU_RW \
    if (!write) {\
        if (addr >= 0xFE00) return (!gb->oamBlocked)  ? gb->oam[addr & 0x9F]    : 0xFF;\
        else                return (!gb->vramBlocked) ? gb->vram[addr & 0x1FFF] : 0xFF;\
    }\
    else {\
        if (addr >= 0xFE00 && !gb->oamBlocked) gb->oam[addr & 0x9F]    = val;\
        else if             (!gb->vramBlocked) gb->vram[addr & 0x1FFF] = val;\
    }\
    return 0;\

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
            case TimerCtrl: /* Todo: TIMA should increase right here if last bit was 1 and current is 0 */
                gb->io[TimerCtrl] = val | 0xF8; return 0;
            case IntrFlags:                                    /* Mask unused bits for IE and IF        */
            case IntrEnabled:
                gb->io[addr % 0x80] = val | 0xE0; return 0;
            case BootROM:                 /* Boot ROM register should be unwritable at some point? */
                break;
            case Divider:
                gb_timer_update (gb, 0); return 0;             /* DIV reset                             */
            case DMA:                                          /* OAM DMA transfer                      */
                gb->io[DMA] = val; int i = 0; /* Todo: Make it write across 160 cycles */
                const uint16_t src = val << 8;
                while (i < OAM_SIZE) { gb->oam[i] = CPU_RB (src + i); i++; } return 0;
        }
        gb->io[addr % 0x80] = val;
    }
    return 0;
}

uint8_t gb_mem_access (struct GB * gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    /* For Blargg's CPU instruction tests */
#ifdef CPU_INSTRS_TESTING
    if (addr == 0xFF44 && !write) return 0x90;
    if (gb->io[SerialCtrl] == 0x81) { LOG_("%c", gb->io[SerialData]); gb->io[SerialCtrl] = 0x0; }
#endif
    /* Byte to be accessed from memory */
    uint8_t * b;
    #define DIRECT_RW(b)  if (write) { *b = val; } return *b;
    struct Cartridge * cart = &gb->cart;

    if (addr < 0x0100 && gb->io[BootROM] == 0)                    /* Run boot ROM if needed */
        { if (!write) { return gb->bootRom[addr]; } else { return 0; }}
    if (addr < 0x8000)  return cart->rw (cart, addr, val, write);       /* ROM from MBC     */
    if (addr < 0xA000)  { GB_PPU_RW }                                   /* Video RAM        */
    if (addr < 0xC000)  return cart->rw (cart, addr, val, write);       /* External RAM     */
    if (addr < 0xE000)  { b = &gb->ram[addr % 0x2000]; DIRECT_RW(b); }  /* Work RAM         */
    if (addr < 0xFE00)  { b = &gb->ram[addr % 0x2000]; DIRECT_RW(b); }  /* Echo RAM         */
    if (addr < 0xFEA0)  { GB_PPU_RW }                                   /* OAM              */
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

    LOG_CPU_STATE (gb);
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
    /* Copied value for operands (can be overridden for other opcodes) */
    uint8_t tmp = REG_A;

    /* HALT bug skips PC increment, essentially rollback one byte */
    if (!gb->pcInc) {
        gb->pc--;
        gb->pcInc = 1;
    }
    /* For conditional jump / call instructions */
    const uint8_t cond = ((op >> 3) & 3);

    switch (op)
    {
        case 0x0:  NOP     break;
        /* 16-bit load, arithmetic instructions */
        OP_r16_g1 (0x01, LDrr)
        OP_r16_g1 (0x03, INCrr)
        OP_r16_g1 (0x09, ADHLrr)
        OP_r16_g1 (0x0B, DECrr)
        case 0x08: LDmSP   break;
        /* 8-bit load instructions */
        OP_r16_g2 (0x02, LDrrmA)
        OP_r16_g2 (0x0A, LDArrm)
        /* Increment, decrement, LD r8,n  */
        OP_r8_g (0x04, INC, 0)
        OP_r8_g (0x05, DEC, 0)
        OP_r8_g (0x06, LDrm, 0)
        /* Opcode group 1 */
        case 0x07: RLCA    break; 
        case 0x17: RLA     break;
        case 0x27: DAA     break;
        case 0x37: SCF     break; 
        /* STOP and relative jump to nn */
        case 0x10: STOP    break;
        case 0x18: JRm     break; 
        /* Opcode group 2 */
        case 0x0F: RRCA    break; 
        case 0x1F: RRA     break; 
        case 0x2F: CPL     break; 
        case 0x3F: CCF     break;
        /* 8-bit load, LD or LDrHL */
        LD_OPS
        /* 8-bit load, LDHLr */ 
        OP_r8(0x70, LD_HL, 0)
        /* HALT */
        case 0x76: 
            OP(HALT);
            if (!gb->ime) {
                if (gb->io[IntrEnabled] && gb->io[IntrFlags] & IF_Any) 
                    gb->pcInc = 0; 
                else 
                    gb->halted = 1;
            } else {
                gb->halted = 1;
            }
        break;
        /* 8-bit arithmetic */
        OP_r8_hl (0x80, ADD, 0)
        OP_r8_hl (0x88, ADC, 0)
        OP_r8_hl (0x90, SUB, 0)
        OP_r8_hl (0x98, SBC, 0)
        OP_r8_hl (0xA0, AND, 0)
        OP_r8_hl (0xA8, XOR, 0)
        OP_r8_hl (0xB0, OR, 0)
        OP_r8_hl (0xB8, CP, 0)
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
        /* Call routines at addresses 0x00 to 0x38 */
        case 0xC7:   case 0xCF:   case 0xD7:
        case 0xDF:   case 0xE7:   case 0xEF:
        case 0xF7:   case 0xFF:   RST  break;
        /* 16-bit push and pop */
        OP_r16_g3 (0xC1, POPrr)
        OP_r16_g3 (0xC5, PUSHrr)
        /* CB prefix ops */
        case 0xCB:   
            gb_exec_cb (gb, CPU_RB (gb->pc++));
        break;
        default: INVALID;
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
        &REG_B, &REG_C, &REG_D, &REG_E, &REG_H, &REG_L, &R_FLAGS, &REG_A };

    const uint8_t r_bit  = opHh & 7;
    uint8_t * reg1 = r8_g[opL & 7];

    /* Fetch value at address (HL) if it's needed */
    uint8_t hl = (opL == 0x6 || opL == 0xE) ? CPU_RB (REG_HL) : 0;

    switch (opHh)
    {
        case 0 ... 7:
            switch (op & 0x38)
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
        CPU_WB (REG_HL, hl);
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
    const uint8_t tacEnabled = (tac >> 2) & 1;
    if (!tacEnabled) return;

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
 *==================================================
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

static inline uint8_t * gb_old_pixel_fetch (const struct GB * gb)
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
            uint8_t relX = (isWindow) ? (lineX - gb->io[WindowX] + 7) % 8 : posX % 8;

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
            pixels[lineX] = ((gb->io[BGPalette] >> (bgIndex << 1)) & 3);// | ((isWindow) ? 0 : PIXEL_BG);
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
                    const uint8_t palIndex = (bitHi << 1) + bitLo;

                    if (palIndex == 0) continue;
                    if ((gb->oam[s + 3] & 0x80) && (pixels[lineX] & 0x3) > 0) continue;

                    const uint8_t palette = OBJPalette0 + ((gb->oam[s + 3] & 0x10) ? 1 : 0);
                    pixels[lineX] = ((gb->io[palette] >> (palIndex * 2)) & 3) | PIXEL_OBJ1;
                }
            }
        }
    }

    gb->draw_line (gb->extData.ptr, pixels, gb->io[LY]);

    return pixels;
}

struct sprite_data 
{
    uint8_t sprite_number;
    uint8_t x;
};

int compare_sprites (const void *in1, const void *in2)
{
	const struct sprite_data *sd1, *sd2;
	int x_res;

	sd1 = (struct sprite_data *)in1;
	sd2 = (struct sprite_data *)in2;
	x_res = (int)sd1->x - (int)sd2->x;
	if(x_res != 0)
		return x_res;

	return (int)sd1->sprite_number - (int)sd2->sprite_number;
}

static inline uint8_t * gb_pixel_fetch (struct GB * gb)
{
    uint8_t * pixels = calloc(DISPLAY_WIDTH, sizeof(uint8_t));
    assert (gb->io[LY] < DISPLAY_HEIGHT);

	/* If background is enabled, draw it. */
	if (LCDC_(BG_Win_Enable))
	{
		uint8_t lineX, posX, tileID, px, rowLSB, rowMSB;
		uint16_t BGTileMap, tileRow;

		/* Calculate current background line to draw */
		const uint8_t posY = gb->io[LY] + gb->io[ScrollY];

        /* BG tile fetcher gets tile ID. Bits 0-4 define X loction, bits 5-9 define Y location
         * All related calculations following are found here:
         * https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md */

		/* Get selected background map address for first tile
		 * corresponding to current line  */
		BGTileMap = ((LCDC_(BG_Area)) ? 0x9C00 : 0x9800);
		BGTileMap += ((posY >> 3) << 5);

		lineX = DISPLAY_WIDTH - 1;
        if (LCDC_(Window_Enable) && gb->io[LY] >= gb->io[WindowY] && gb->io[WindowX] <= 166)
            lineX = (gb->io[WindowX] < 7 ? 0 : gb->io[WindowX] - 7) - 1;

		/* Fetch first tile */
		posX = lineX + gb->io[ScrollX];
		tileID = gb->vram[(BGTileMap & 0x1FFF) + (posX >> 3)];
		px = 7 - (posX & 7);

		/* Select addressing mode */
        const uint16_t bit12 = !(LCDC_(BG_Win_Data) || (tileID & 0x80)) << 12;
        tileRow = bit12 + (tileID << 4) + ((posY & 7) << 1);

		rowLSB = gb->vram[tileRow] >> px;
		rowMSB = gb->vram[tileRow + 1] >> px;

		for(; lineX != 0xFF; lineX--)
		{
			uint8_t palIndex;

			if (px == 8)
			{
				/* fetch next tile */
				posX = lineX + gb->io[ScrollX];
				tileID = gb->vram[(BGTileMap & 0x1FFF) + (posX >> 3)];
				px = 0;

		        /* Select addressing mode */
                const uint16_t bit12 = !(LCDC_(BG_Win_Data) || (tileID & 0x80)) << 12;
                tileRow = bit12 + (tileID << 4) + ((posY & 7) << 1);

				rowLSB = gb->vram[tileRow] >> px;
				rowMSB = gb->vram[tileRow + 1] >> px;
			}

			/* Get background color */
			palIndex = (rowLSB & 0x1) | ((rowMSB & 0x1) << 1);
			pixels[lineX] = ((gb->io[BGPalette] >> (palIndex << 1)) & 3);

			rowLSB >>= 1;
			rowMSB >>= 1;
			px++;
		}
	}

	/* draw window */
	if (LCDC_(Window_Enable) && gb->io[LY] >= gb->io[WindowY] && gb->io[WindowX] <= 166)
	{
		uint8_t lineX, win_x, py, px, tileID, rowLSB, rowMSB, end;
		uint16_t WinTileMap, tile;

		/* Calculate Window Map Address. */
		WinTileMap = (LCDC_(Window_Area)) ? 0x9C00 : 0x9800;
		WinTileMap += (gb->windowLY >> 3) << 5;

		lineX = DISPLAY_WIDTH - 1;
		py = gb->windowLY & 7;

		win_x = lineX - gb->io[WindowX] + 7;
		px = 7 - (win_x & 7);
		tileID = gb->vram[(WinTileMap & 0x1FFF) + (win_x >> 3)];

        /* Select addressing mode */
        const uint16_t bit12 = !(LCDC_(BG_Win_Data) || (tileID & 0x80)) << 12;
        tile = bit12 + (tileID << 4) + (py << 1);

		/* fetch tile */
		rowLSB = gb->vram[tile] >> px;
		rowMSB = gb->vram[tile + 1] >> px;

		end = (gb->io[WindowX] < 7 ? 0 : gb->io[WindowX] - 7) - 1;

		for(; lineX != end; lineX--)
		{
			uint8_t palIndex;

			if (px == 8)
			{
				win_x = lineX - gb->io[WindowX] + 7;
				px = 0;
				tileID = gb->vram[(WinTileMap & 0x1FFF) + (win_x >> 3)];

		        /* Select addressing mode */
                const uint16_t bit12 = !(LCDC_(BG_Win_Data) || (tileID & 0x80)) << 12;
                tile = bit12 + (tileID << 4) + (py << 1);

		        /* fetch tile */
				rowLSB = gb->vram[tile] >> px;
				rowMSB = gb->vram[tile + 1] >> px;
			}

			palIndex = (rowLSB & 0x1) | ((rowMSB & 0x1) << 1);
			pixels[lineX] = ((gb->io[BGPalette] >> (palIndex << 1)) & 3);

			rowLSB >>= 1;
			rowMSB >>= 1;
			px++;
		}
        gb->windowLY++;
	}

    #define MAX_SPRITES_LINE  10
    #define NUM_SPRITES       40

	if (LCDC_(OBJ_Enable))
	{
		uint8_t sprite;
		uint8_t totalSprites = 0;

		struct sprite_data sprites_to_render[NUM_SPRITES];

		/* Record number of sprites on the line being rendered, limited
		 * to the maximum number sprites that the Game Boy is able to
		 * render on each line (10 sprites). */
		for (sprite = 0; sprite < (sizeof (sprites_to_render) / sizeof (sprites_to_render[0])); sprite++)
		{
			uint8_t objY = gb->oam[4 * sprite];
			uint8_t objX = gb->oam[4 * sprite + 1];

			/* If sprite isn't on this line, continue */
			if (gb->io[LY] + (LCDC_(OBJ_Size) ? 0 : 8) >= objY || gb->io[LY] + 16 < objY)
				continue;

			sprites_to_render[totalSprites].sprite_number = sprite;
			sprites_to_render[totalSprites].x = objX;
			totalSprites++;
		}

        uint8_t sprites[10];
        memset(sprites, 0, 10);
        totalSprites = 0;

        /* Find available sprites that could be visible on this line */
        uint8_t s;
        for (s = 0; s < OAM_SIZE; s += 4)
        {
            if (gb->oam[s] == 0 && gb->oam[s] >= 160) continue;
            if (gb->io[LY] >= gb->oam[s] - (LCDC_(2) ? 0 : 8) || 
                gb->io[LY] < gb->oam[s] - 16) continue;
            
            sprites[totalSprites++] = s;
            if (totalSprites == 10) break;
        }

		/* If maximum number of sprites reached, prioritise X
		 * coordinate and object location in OAM. */
		//qsort (&sprites_to_render[0], totalSprites,
		//		sizeof(sprites_to_render[0]), compare_sprites);
		//if (totalSprites > MAX_SPRITES_LINE)
		//	totalSprites = MAX_SPRITES_LINE;
        
		/* Sprites are rendered from low priority to high priority */
		for (sprite = totalSprites - 1; sprite != 0xFF; sprite--)
		{
			const uint8_t s = sprites[sprite];//sprites_to_render[sprite].sprite_number << 2;

			const uint8_t objY = gb->oam[s + 0], objX = gb->oam[s + 1];
			const uint8_t objTile = gb->oam[s + 2] & (LCDC_(OBJ_Size) ? 0xFE : 0xFF);
			const uint8_t objFlags = gb->oam[s + 3];

            /* Skip sprite if not visible */
			if (objX == 0 || objX >= DISPLAY_WIDTH + 8) continue;

			/* Handle Y flip */
			uint8_t posY = gb->io[LY] - objY + 16;
			if (objFlags & 0x40) posY = (LCDC_(OBJ_Size) ? 15 : 7) - posY;

			const uint8_t rowLSB = gb->vram[(objTile << 4) + (posY << 1)];
			const uint8_t rowMSB = gb->vram[(objTile << 4) + (posY << 1) + 1];

            const uint8_t sLeft  = (objX - 8 < 0) ? 0 : objX - 8;
            const uint8_t sRight = (objX  >= DISPLAY_WIDTH) ? DISPLAY_WIDTH : objX;

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
				if (palIndex && !(objFlags & 0x80 && !((pixels[lineX] & 0x3) == (gb->io[BGPalette] & 3))))
				{
					/* Set pixel based on palette */
					pixels[lineX] = (objFlags & 0x10)
						? ((gb->io[OBJPalette1] >> (palIndex << 1)) & 3)
						: ((gb->io[OBJPalette0] >> (palIndex << 1)) & 3);
				}
			}
		}
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
        if (!LCDC_(LCD_Enable) || (gb->extData.frameSkip && 
            (gb->totalFrames % gb->extData.frameSkip > 0))) return;
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
        gb->io[LCDStatus] &= 0xFB;// &= ~(1 << LYC_LY);
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
        if (gb->io[LY] > DISPLAY_HEIGHT) gb->windowLY = 0; /* Reset window Y counter if line is 0 */
        gb_eval_LYC (gb);
    }
}

void gb_render (struct GB * const gb)
{
    gb->rt = gb->rm * 4;

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