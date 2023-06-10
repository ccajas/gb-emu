#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include "gb.h"
#include "ops.h"

#define CPU_INSTRS

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
            case BootROM: 
                LOG_("Writing value to bootROM %d\n", val); break;
            case Divider:
                gb->divClock = 0; return 0;                                           /* DIV reset */
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

    if (addr < 0x0100 && !gb->io[BootROM])                        /* Run boot ROM if needed */
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

void gb_init (struct GB * gb, uint8_t * bootRom)
{
    /* Read checksum and see if it matches data */
    uint8_t checksum = 0;
    uint16_t addr;
    for (addr = 0x134; addr <= 0x14C; addr++) {
        checksum = checksum - gb->cart.romData[addr] - 1;
    }
    if (gb->cart.romData[0x14D] != checksum)
        LOG_(" %c%c Invalid checksum!\n", 192,196);
    else
        LOG_(" %c%c Valid checksum '%02X'\n", 192,196, checksum);

    /* Get cartridge type and MBC from header */
    memcpy (gb->cart.header, gb->cart.romData + 0x100, 80 * sizeof(uint8_t));
    const uint8_t * header = gb->cart.header;

    const uint8_t cartType = header[0x47];

    /* Select MBC for read/write */
    switch (cartType)
    {
        case 0:             gb->cart.mbc = 0; break;
        case 0x1  ... 0x3:  gb->cart.mbc = 1; break;
        case 0x5  ... 0x6:  gb->cart.mbc = 2; break;
        case 0xF  ... 0x13: gb->cart.mbc = 3; break;
        case 0x19 ... 0x1E: gb->cart.mbc = 5; break;
        default: 
            LOG_("GB: MBC not supported.\n"); return;
    }
    gb->cart.rw = cart_rw[gb->cart.mbc];

    printf ("GB: ROM file size (KiB): %d\n", 32 * (1 << header[0x48]));
    printf ("GB: Cart type: %02X Mapper type: %d\n", header[0x47], gb->cart.mbc);
    
    const uint8_t ramBanks[] = { 0, 0, 1, 4, 16, 8 };

    /* Add other metadata */
    gb->cart.romSizeKB = 32 * (1 << header[0x48]);
    gb->cart.ramSizeKB = 8 * ramBanks[header[0x49]];
    gb->cart.romMask = (1 << (header[0x48] + 1)) - 1;
    gb->cart.bank1st = 1;
    gb->cart.bank2nd = 0;
    gb->cart.romOffset = 0x4000;
    gb->cart.ramOffset = 0;

    printf ("GB: ROM mask: %d\n", gb->cart.romMask);

    if (bootRom != NULL)
    {
        gb_reset (gb, bootRom);
        return;
    }
    gb_boot_reset (gb);
}

void gb_reset (struct GB * gb, uint8_t * bootROM)
{
    gb->bootRom = bootROM;
    gb->extData.joypad = 0xFF;
    gb->io[Joypad]     = 0xCF;
    gb->io[BootROM]    = 0;

    printf ("GB: Load Boot ROM\n");

    gb->pc = 0;
    gb->ime = 0;
    gb->invalid = 0;
    gb->halted = 0;

    printf ("GB: Set I/O\n");

    /* Initalize I/O registers (DMG) */
    memset(gb->io, 0, sizeof (gb->io));

    gb->lineClock = gb->frameClock = 0;
    gb->clock_t = gb->clock_m = 0;
    gb->divClock = 0;
    gb->frame = 0;
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
    gb->io[BootROM]    = 0x01;

    /* Initialize RAM and settings */
    memset (gb->ram,  0, WRAM_SIZE);
    memset (gb->vram, 0, VRAM_SIZE);
    memset (gb->hram, 0, HRAM_SIZE);
    gb->vramBlocked = gb->oamBlocked = 0;

    printf ("Memset done\n");

    gb_cpu_state (gb);
    gb->lineClock = gb->frameClock = 0;
    gb->clock_t = gb->clock_m = 0;
    gb->divClock = 0;
    gb->frame = 0;
    printf ("CPU state done\n");
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

static const uint8_t r8_group[] = { B, C, D, E, H, L, 0, A };

void gb_exec_cb (struct GB * gb, const uint8_t op)
{
    gb->rm = 3;

    const uint8_t opL  = op & 0xf;
    const uint8_t opHh = op >> 3; /* Octal divisions */

    const uint8_t r = r8_group[(opL & 7)];
    const uint8_t r_bit  = opHh & 7;

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

void gb_cpu_exec (struct GB * gb, const uint8_t op)
{
    const uint8_t opL = op & 0xf;
    const uint8_t opHh = op >> 3; /* Octal divisions */
    //uint16_t hl = 0;

    /* Default values for operands (can be overridden for other opcodes) */
    const uint8_t r1 = r8_group[(opHh & 7)];
    const uint8_t r2 = r8_group[(opL  & 7)];

    uint8_t tmp = gb->r[A];

    switch (opHh)
    {
        case 0 ... 7:
        case 0x18 ... 0x1F:

            switch (op)
            {
                case 0x0:  NOP     break; 
                case 0x02:                case 0x12: LDrrmA  break; 
                case 0x22: LDHLIA  break; case 0x32: LDHLDA  break;
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
                case 0xC9: RET     break; case 0xCB: PREFIX  break;
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
                default:   INVALID;
            }
            /* Jump / call instructions */
            const uint8_t cond = ((op >> 3) & 3);
            switch (op & 0b11100111)
            {
                case 0b00100000: JR_  (cond) break;
                case 0b11000000: RET_ (cond) break;
                case 0b11000010: JP_  (cond) break;
                case 0b11000100: CALL_(cond) break;
            }
            /* Increment, decrement, LD r8,n, RST */
            switch (op & 0b11000111)
            {
                case 0b00000100: INC_   break;
                case 0b00000101: DEC_   break;
                case 0b00000110: LDrm_  break;
                case 0b11000111: RST    break;
            }
            /* 16-bit load, arithmetic instructions */
            switch (op & 0b11001111)
            {
                case 1:    LDrr    break; case 3:    INCrr   break;
                case 0x9:  ADHLrr  break; case 0xB:  DECrr   break;
            }
            switch (op)
            {
                R16_G3_(POPrr_OP,  0xC1);
                R16_G3_(PUSHrr_OP, 0xC5);
            }
        break;
        case 8 ... 0xD: case 0xF:
            /* 8-bit load, LD or LDrHL */
            OPR_2_(LD_r8, LD_rHL)
        break;
        case 0xE:
            /* 8-bit load, LDHLr or HALT */
            OPR_2_(LD_HLr, HALT)
        break;
        case 0x10 ... 0x17: {
            /* 8-bit arithmetic */
            uint16_t hl = CPU_RB (ADDR_HL);
            /* Mask bits for ALU operations */
            switch (op & 0b11111000) 
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
}

void gb_handle_interrupts (struct GB * gb)
{
    if (!gb->halted && !gb->ime) return;

    /* Get interrupt flags */
    const uint8_t io_IE = gb->io[IntrEnabled % 0x80];
    const uint8_t io_IF = gb->io[IntrFlags];

    /* Run if CPU ran HALT instruction or IME enabled w/flags */
    if (io_IE & io_IF & IF_Any)
    {
        gb->halted = 0;
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

void gb_handle_timings (struct GB * gb)
{
    /* Increment div every m-cycle and save bits 6-13 to DIV register */
    gb->divClock++;
    gb->io[Divider] = (gb->divClock >> 6) & 0xFF;

    if (gb->timAOverflow)
    {
        /* Check overflow in the next cycle */
        gb->io[IntrFlags] |= IF_Timer;
        gb->io[TimA] = gb->io[TMA];

        gb_handle_interrupts (gb);
        gb->timAOverflow = 0;
    }

    /* Leave if timer is disabled */
    const uint8_t tac = gb->io[TimerCtrl];
    if (!(tac & 0x4)) return;

    /* Update timer, check bits for 1024, 16, 64, or 256 cycles respectively  */
    const uint8_t checkBits[4] = { 7, 1, 3, 5 };
    const uint8_t cBit = checkBits[tac & 0x3];

    if (FALLING_EDGE (gb->lastDiv >> cBit, gb->divClock >> cBit))
    {
        uint8_t timA = gb->io[TimA];
        /* Request timer interrupt if pending */
        if (++timA == 0) gb->timAOverflow = 1;
        gb->io[TimA] = timA;
    }
    gb->lastDiv = gb->divClock;
}

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

#define IO_STAT_CLEAR   (gb->io[LCDStatus] & 0xFC)
#define IO_STAT_MODE    (gb->io[LCDStatus] & 3)

#define LCDC_(bit)  (gb->io[LCDControl] & (1 << bit))
#define STAT_(bit)  (gb->io[LCDStatus]  & (1 << bit))

static inline uint8_t * gb_pixel_fetch (const struct GB * gb)
{
    uint8_t * pixels = calloc(DISPLAY_WIDTH, sizeof(uint8_t));
    assert (gb->io[LY] < DISPLAY_HEIGHT);

	/* Check if background is enabled */
	if (LCDC_(0) && LCDC_(7))
	{
        /* Get minimum starting address depends on LCDC bits 3 and 6 are set.
        Starts at 0x1800 as this is the VRAM index minus 0x8000 */
        uint16_t BGTileMap  = (LCDC_(3)) ? 0x9C00 : 0x9800;
        uint16_t winTileMap = (LCDC_(6)) ? 0x9C00 : 0x9800;

        /* X position counter */
        uint8_t lineX = 0;
        const uint8_t lineY = gb->io[LY];
        uint8_t posY = lineY + gb->io[ScrollY];

        uint16_t tileAddr, tileID, bit12, tileRow;
        /* For storing pixel bytes */
        uint8_t byteLo = 0;
        uint8_t byteHi = 0;

        /* Run at most 20 times (for the 160 pixel length) */
        for (lineX = 0; lineX < DISPLAY_WIDTH; lineX++)
        {
            /* BG tile fetcher gets tile ID. Bits 0-4 define X loction, bits 5-9 define Y location
            All related calculations following are found here:
            https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md */

            const uint8_t posX = lineX + gb->io[ScrollX];
            const uint8_t relX = posX % 8;

            /* Stop BG rendering if window is found here */
            if (gb->io[WindowX] - 7 >= lineX && lineY >= gb->io[WindowY])
                break;

            /* Get next tile being scrolled in */
            if (lineX == 0 || relX == 0)
            {
                BGTileMap  = (LCDC_(3)) ? 0x9C00 : 0x9800;
                tileAddr = BGTileMap + 
                    ((posY >> 3) << 5) +  /* Bits 5-9, Y location */
                    (posX >> 3);          /* Bits 0-4, X location */
                tileID = gb->vram[tileAddr & 0x1FFF];

                /* Tilemap location depends on LCDC 4 set, which are different rules for BG and Window tiles */
                /* Fetcher gets low byte and high byte for tile */
                bit12 = !(LCDC_(4) || (tileID & 0x80)) << 12;
                tileRow = 0x8000 + bit12 + (tileID << 4) + ((posY & 7) << 1);

                /* Finally get the pixel bytes from these addresses */
                byteLo = gb->vram[tileRow & 0x1FFF];
                byteHi = gb->vram[(tileRow + 1) & 0x1FFF];
            }

            /* Produce pixel data from the combined bytes*/
            const uint8_t bitLo = (byteLo >> (7 - relX)) & 1;
            const uint8_t bitHi = (byteHi >> (7 - relX)) & 1;
            const uint8_t index = (bitHi << 1) + bitLo;
            
            pixels[lineX] = (gb->io[BGPalette] >> (index * 2)) & 3;
        }

        /* Leave prematurely if window is disabled */
        //if (!LCDC_(5)) return pixels;

        /* Get line of window to draw */
        posY = lineY - gb->io[WindowY];

        /* Draw window if visible */
        uint8_t windowX;
        for (windowX = 0; windowX < DISPLAY_WIDTH - lineX; windowX += 8)
        {
            /* Get next tile for window */
            winTileMap  = (LCDC_(6)) ? 0x9C00 : 0x9800;
            tileAddr = winTileMap + 
                ((posY >> 3) << 5) +  /* Bits 5-9, Y location */
                ((windowX) >> 3);     /* Bits 0-4, X location */
            tileID = gb->vram[tileAddr & 0x1FFF];

            /* Tilemap location depends on LCDC 4 set, which are different rules for BG and Window tiles */
            /* Fetcher gets low byte and high byte for tile */
            bit12 = !(LCDC_(4) || (tileID & 0x80)) << 12;
            tileRow = 0x8000 + bit12 + (tileID << 4) + ((posY & 7) << 1);

            /* Finally get the pixel bytes from these addresses */
            byteLo = gb->vram[tileRow & 0x1FFF];
            byteHi = gb->vram[(tileRow + 1) & 0x1FFF];

            int x;
            for (x = 0; x < 8; x++)
            {
                /* Produce pixel data from the combined bytes*/
                const uint8_t bitLo = (byteLo >> (7 - x)) & 1;
                const uint8_t bitHi = (byteHi >> (7 - x)) & 1;
                const uint8_t index = (bitHi << 1) + bitLo;
                
                pixels[lineX + windowX + x] = (gb->io[BGPalette] >> (index * 2)) & 3;
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
        if STAT_(5) gb->io[IntrFlags] |= IF_LCD_STAT;      /* Mode 2 interrupt */

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

        /* Fetch line of pixels for the screen and draw them */
        if (gb->frame == 0) {
            uint8_t * pixels = gb_pixel_fetch (gb);
            gb->draw_line (gb->extData.ptr, pixels, gb->io[LY]);
        }
    }
}

static inline void gb_hblank (struct GB * gb)
{ 
    /* Mode 0 - H-blank */
    if (IO_STAT_MODE != Stat_HBlank)
    {
        gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_HBlank;
        /* Mode 0 interrupt */
        if STAT_(3) gb->io[IntrFlags] |= IF_LCD_STAT;
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
    /* Starting new line */
    gb->io[LY] = (gb->io[LY] + 1) % SCAN_LINES;
    gb->lineClock -= (TICKS_OAM_READ + TICKS_TRANSFER + TICKS_HBLANK);
    gb_eval_LYC (gb);

    /* Check if all visible lines are done */
    if (gb->io[LY] == DISPLAY_HEIGHT)
    {
        /* Enter Vblank and indicate that a frame is completed */
        gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_VBlank;
        gb->io[IntrFlags] |= IF_VBlank;
        /* Mode 1 interrupt */
        if STAT_(4) gb->io[IntrFlags] |= IF_LCD_STAT;
    }
}

void gb_ppu_step (struct GB * const gb)
{
    switch (gb->io[LY])
    {
        case 0 ... DISPLAY_HEIGHT - 1:
            if (gb->lineClock < TICKS_OAM_READ) { gb_oam_read (gb);    break; }
            if (gb->lineClock < TICKS_TRANSFER) { gb_pixel_fetch (gb); break; }
            if (gb->lineClock < TICKS_HBLANK)   { gb_hblank (gb);      break; }

            /* Fallthrough: lineClock >= TICKS_HBLANK. Starting a new line */ 
            gb->io[LY] = (gb->io[LY] + 1) % SCAN_LINES;
            gb->lineClock -= TICKS_HBLANK;
            gb_eval_LYC (gb);
        break;
        default:
            gb_vblank (gb);
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
        if (gb->lineClock < TICKS_OAM_READ)
        {
            gb_oam_read (gb);
        }
        else if (gb->lineClock < TICKS_TRANSFER)
        {
            gb_transfer (gb);
        }
        else if (gb->lineClock < TICKS_HBLANK)
        {
            gb_hblank (gb);
        }
        else
        {
            /* Starting new line */
            gb->lineClock -= TICKS_HBLANK;
            gb->io[LY] = (gb->io[LY] + 1) % SCAN_LINES;
            gb_eval_LYC (gb);

            /* Check if all visible lines are done */
            if (gb->io[LY] == DISPLAY_HEIGHT)
            {
                /* Enter Vblank and indicate that a frame is completed */
                gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_VBlank;
                gb->io[IntrFlags] |= IF_VBlank;
                /* Mode 1 interrupt */
                if LCDC_(4) gb->io[IntrFlags] |= IF_LCD_STAT;
            }
        }
    }
    else /* Outside of screen Y bounds */
    {
        /* Mode 1 - V-blank */
        if(gb->lineClock >= TICKS_VBLANK)
        {
            /* Advance though lines below the screen */
            gb->io[LY] = (gb->io[LY] + 1) % SCAN_LINES;
            gb->lineClock -= TICKS_VBLANK;
            gb_eval_LYC (gb);
        }
    }
    /* ...and DMA transfer to OMA, if needed */
}