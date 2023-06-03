#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "gb.h"
#include "ops.h"

#define CPU_INSTRS

uint8_t gb_mem_access (struct GB * gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    /* For debug logging purposes */
    //if (addr == 0xFF44 && !write) return 0x90;

    /* For Blargg's CPU instruction tests */
#ifdef CPU_INSTRS
    if (gb->io[SerialCtrl] == 0x81) { LOG_("%c", gb->io[SerialData]); gb->io[SerialCtrl] = 0x0; }
#endif

    /* Byte to be accessed from memory */
    uint8_t * b;
    #define DIRECT_RW(b)  if (write) { *b = val; } return *b;
    switch (addr)
    {
        case 0x0000 ... 0x7FFF: return mbc_rw (gb, addr, val, write);   /* ROM from MBC     */
        case 0x8000 ... 0x9FFF: return ppu_rw (gb, addr, val, write);   /* Video RAM        */
        case 0xA000 ... 0xBFFF: return mbc_rw (gb, addr, val, write);   /* External RAM     */
        case 0xC000 ... 0xDFFF: b = &gb->ram[addr % 0x2000];            /* Work RAM         */
                                DIRECT_RW(b);
        case 0xE000 ... 0xFDFF: return 0xFF;                            /* Echo RAM         */
        case 0xFE00 ... 0xFE9F: return ppu_rw (gb, addr, val, write);   /* OAM              */
        case 0xFEA0 ... 0xFEFF: return 0xFF;                            /* Not usable       */
        case 0xFF00 ... 0xFF7F: b = &gb->io[addr % 0x80]; DIRECT_RW(b); /* I/O registers    */                        
        case 0xFF80 ... 0xFFFE: b = &gb->hram[addr % 0x80];             /* High RAM         */  
                                DIRECT_RW(b);
        case 0xFFFF:            b = &gb->io[addr & 0x7F]; DIRECT_RW(b); /* Interrupt enable */
    }
}

uint8_t mbc_rw (struct GB * gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    struct Cartridge * cart = &gb->cart;

    if (!write) /* Read from cartridge */
        switch (addr)
        {
            case 0x0000 ... 0x3FFF:
                return cart->romData[addr];
            case 0x4000 ... 0x7FFF:
                if (cart->mbc == 1) {
                    if (!cart->bankMode && cart->romSizeKB >= 1024)
                        cart->romOffset = ((cart->bankHi << 5) + cart->bankLo) * 0x4000;
                    else
                        cart->romOffset = cart->bankLo * 0x4000;
                    return cart->romData[(addr % 0x4000) + cart->romOffset]; 
                } /* Fall-through for no MBC */
                return cart->romData[addr];
            case 0xA000 ... 0xBFFF:
                if (cart->mbc == 1 && cart->ram) {
                    /* Select RAM bank and fetch data (if enabled) */ 
                    if (!cart->ramEnabled) return 0xFF;
                    const uint16_t ramAddr = ((cart->bankMode) ? cart->ramOffset : 0);
                    return cart->ramData[(addr % 0x2000) + ramAddr];
                }
            default: return 0xFF;
        }
    else /* Write to cartridge/registers */
        switch (addr)
        {
            case 0x0000 ... 0x1FFF:
                if (cart->mbc == 1) {
                    cart->ramEnabled = ((val & 0xF) == 0xA) ? 1 : 0; }
            break;
            case 0x2000 ... 0x3FFF:
                if (cart->mbc == 1) {
                    cart->bankLo = (val & 0x1F) | (cart->bankLo & 0x60);
                    if (cart->bankLo == 0) cart->bankLo++;
                }
                cart->romOffset = cart->bankLo * 0x4000;
            break;
            case 0x4000 ... 0x5FFF: 
                /* MBC1: Write upper 2 bank bits*/
                if (cart->mbc == 1) cart->bankHi = val;   
            break;
            case 0x6000 ... 0x7FFF:
                if (cart->mbc == 1) cart->bankMode = (val & 1); 
            break;
        }

    return 0;
}

uint8_t ppu_rw (struct GB * gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) {
        if (addr >= 0xFE00)
            return (!gb->oamBlocked)  ? gb->oam[addr & 0x9F] : 0xFF;
        else
            return (!gb->vramBlocked) ? gb->vram[addr & 0x1FFF] : 0xFF;
    }
    else {
        if (addr >= 0xFE00 && !gb->oamBlocked)
            gb->oam[addr & 0x9F] = val;
        else if (!gb->vramBlocked)
            gb->vram[addr & 0x1FFF] = val;
    }
    return 0;
}

void gb_init (struct GB * gb)
{
    /* Read checksum and see if it matches data */
    uint8_t checksum = 0;
    uint16_t addr;
    for (addr = 0x134; addr <= 0x14C; addr++) {
        checksum = checksum - gb->cart.romData[addr] - 1;
    }
    if (gb->cart.romData[0x14D] != checksum)
        LOG_("Invalid checksum!\n");
    else
        LOG_("Valid checksum '%02X'\n", checksum);

    /* Get cartridge type and MBC from header */
    memcpy (gb->cart.header, gb->cart.romData + 0x100, 80 * sizeof(uint8_t));
    uint8_t * header = gb->cart.header;

    const uint8_t cartType = header[0x47];
    uint8_t mbc = 0;

    switch (cartType)
    {
        case 0:            mbc = 0; break;
        case 0x1 ... 0x3:  mbc = 1; break;
        case 0x5 ... 0x6:  mbc = 2; break;
        case 0xF ... 0x13: mbc = 3; break;
        default: 
            LOG_("GB: MBC not supported.\n"); return;
    }

    printf ("GB: ROM file size (KiB): %d\n", 32 * (1 << header[0x48]));
    printf ("GB: Cart type: %02X\n", header[0x47]);
    const uint8_t ramBanks[] = { 0, 0, 1, 4, 16, 8 };

    /* Add other metadata */
    gb->cart.romSizeKB = 32 * (1 << header[0x48]);
    gb->cart.ramSizeKB = 8 * ramBanks[header[0x49]];
    gb->cart.bankLo = 1;
    gb->cart.bankHi = 0;
    gb->cart.romOffset = 0x4000;
    gb->cart.ramOffset = 0;
    gb->cart.mbc = mbc;
    gb->bootrom = 0;

    /* Setup CPU registers as if bootrom was loaded */
    if (!gb->bootrom)
    {
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
    }

    /* Initalize I/O registers */
    memset(gb->io, 0, sizeof (gb->io));  
    gb->io[Joypad]     = 0xCF;
    gb->io[SerialCtrl] = 0x7E;
    gb->io[TimerCtrl]  = 0xF8;
    gb->io[IntrFlags]  = 0xE1;
    gb->io[LCDControl] = 0x91;
    gb->io[LCDStatus]  = 0x81;
    gb->io[LY]         = 0x90;
    gb->io[DMA]        = 0xFF;

    gb->vramBlocked = gb->oamBlocked = 0;

    /* Clear memory */
    memset (gb->ram,  0, WRAM_SIZE);
    memset (gb->vram, 0, VRAM_SIZE);
    memset (gb->hram, 0, HRAM_SIZE);

    gb_cpu_state (gb);
}

const int8_t opTicks[256] = {
/*   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  A, B,  C,  D, E,  F*/
     4, 12,  8,  8,  4,  4,  8,  4, 20,  8,  8,  8,  4,  4,  8,  4,  /* 0x */
     4, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4,
     8, 12,  8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,
     8, 12,  8,  8, 12, 12, 12,  4,  8,  8,  8,  8,  4,  4,  8,  4,

     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 4x */
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
     8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,

     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 8x */
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,

     8, 12, 12, 16, 12, 16,  8, 16,  8, 16, 12,  0, 12, 24,  8, 16,  /* Cx */
     8, 12, 12,  4, 12, 16,  8, 16,  8, 16, 12,  4, 12,  4,  8, 16,
    12, 12,  8,  4,  4, 16,  8, 16, 16,  4, 16,  4,  4,  4,  8, 16,
    12, 12,  8,  4,  4, 16,  8, 16, 12,  8, 16,  4,  0,  4,  8, 16
};

void gb_exec_cb (struct GB * gb, const uint8_t op)
{
    const uint8_t opL  = op & 0xf;
    const uint8_t opHh = op >> 3; /* Octal divisions */

    const uint8_t r = ((opL & 7) == 7) ? A : ((opL & 7) < 6) ? B + (opL & 7) : 255;
    const uint8_t r_bit  = opHh & 7;

    /* Fetch value at address (HL) if it's needed */
    uint8_t hl = (opL == 0x6 || opL == 0xE) ? CPU_RB (ADDR_HL) : 0;

    gb->rt += 8;

    switch (opHh)
    {
        case 0 ... 7:
            switch (op)
            {
                case 0x00      ... 0x05:  case 0x07: RLC     break;
                case 0x08      ... 0x0D:  case 0x0F: RRC     break;
                case 0x06: RLCHL   break; case 0x0E: RRCHL   break;
                case 0x10      ... 0x15:  case 0x17: RL      break;
                case 0x18      ... 0x1D:  case 0x1F: RR      break;
                case 0x16: RLHL   break;  case 0x1E: RRHL    break;
                case 0x20      ... 0x25:  case 0x27: SLA     break;
                case 0x28      ... 0x2D:  case 0x2F: SRA     break;
                case 0x26: SLAHL   break; case 0x2E: SRAHL   break;
                case 0x30      ... 0x35:  case 0x37: SWAP    break;
                case 0x38      ... 0x3D:  case 0x3F: SRL     break;
                case 0x36: SWAPHL  break; case 0x3E: SRLHL   break;
            }
            if (op & 7) gb->rt += 8;
        break;
        case 8 ... 0xF:
            /* Bit test */
            if (opL == 0x6 || opL == 0xE ) { BITHL; gb->rt += 4; } else { BIT }
        break;
        case 0x10 ... 0x17:
            /* Bit reset */
            if (opL == 0x6 || opL == 0xE ) { RESHL; gb->rt += 8; } else { RES }
        break;
        case 0x18 ... 0x1F:
            /* Bit set */
            if (opL == 0x6 || opL == 0xE ) { SETHL; gb->rt += 8; } else { SET }
        break;
    }
}

uint8_t gb_cpu_exec (struct GB * gb)
{
    gb->rt = 0;

    const uint8_t op  = CPU_RB (gb->pc++);
    const uint8_t opL = op & 0xf;
    const uint8_t opHh = op >> 3; /* Octal divisions */
    uint16_t hl = ADDR_HL;

    /* Default values for operands (can be overridden for other opcodes) */
    const uint8_t  r1 = ((opHh & 7) == 7) ? A : B + (opHh & 7);
    const uint8_t  r2 = ((opL & 7)  == 7) ? A : ((opL & 7) < 6) ? B + (opL & 7) : 255;

    uint8_t tmp = gb->r[A];

    switch (opHh)
    {
        case 0 ... 7:
        case 0x18 ... 0x1F:

            switch (op)
            {
                case 0x0:  NOP     break; 
                case 0x01:   case 0x11:   case 0x21: LDrr    break;
                case 0x02:                case 0x12: LDrrmA  break; 
                case 0x03:   case 0x13:   case 0x23: INCrr   break; 
                case 0x04:   case 0x14:   case 0x24:
                case 0x0C:   case 0x1C:   case 0x2C:     
                case 0x3C:                           INC     break;   
                case 0x05:   case 0x15:   case 0x25:
                case 0x0D:   case 0x1D:   case 0x2D:
                case 0x3D:                           DEC     break;
                case 0x06:   case 0x16:   case 0x26:
                case 0x0E:   case 0x1E:   case 0x2E:
                case 0x3E:                           LDrm    break;
                case 0x07: RLCA    break; case 0x08: LDmSP   break;
                case 0x09:                case 0x19:
                case 0x29: ADHLrr  break;
                case 0x0A:                case 0x1A: LDArrm  break;
                case 0x0B:   case 0x1B:   case 0x2B: DECrr   break;
                case 0x0F: RRCA    break; 
                
                case 0x10: STOP    break; case 0x17: RLA     break; 
                case 0x18: JRm     break; case 0x1F: RRA     break; 
                case 0x20: JRNZ    break; case 0x22: LDHLIA  break; 
                case 0x27: DAA     break; case 0x28: JRZ     break; 
                case 0x2A: LDAHLI  break; case 0x2F: CPL     break; 

                case 0x30: JRNC    break; case 0x31: LDSP    break;
                case 0x32: LDHLDA  break; case 0x33: INCSP   break;
                case 0x34: INCHL   break; case 0x35: DECHL   break;
                case 0x36: LDHLm   break; case 0x37: SCF     break;
                case 0x38: JRC     break;
                case 0x39: ADHLSP  break; case 0x3A: LDAHLD  break;
                case 0x3B: DECSP   break; case 0x3F: CCF     break;
                /* ... */
                case 0xC0: RETNZ   break;
                case 0xC1:   case 0xD1:   case 0xE1: POP     break;
                case 0xC2: JPNZ    break; case 0xC3: JPNN    break;
                case 0xC4: CALLNZ  break;
                case 0xC5:   case 0xD5:   case 0xE5: PUSH    break;
                case 0xC6: ADDm    break;
                case 0xC7:   case 0xCF:   case 0xD7:
                case 0xDF:   case 0xE7:   case 0xEF:
                case 0xF7:   case 0xFF:              RST     break;
                case 0xC8: RETZ    break; case 0xC9: RET     break;
                case 0xCA: JPZ     break; case 0xCB: PREFIX  break;
                case 0xCC: CALLZ   break; case 0xCD: CALLm   break;
                case 0xCE: ADCm    break;

                case 0xD0: RETNC   break; case 0xD2: JPNC    break;
                case 0xD4: CALLNC  break; case 0xD6: SUBm    break;
                case 0xD8: RETC    break; case 0xD9: RETI    break;
                case 0xDA: JPC     break; case 0xDC: CALLC   break;
                case 0xDE: SBCm    break;

                case 0xE0: LDIOmA  break; case 0xE2: LDIOCA  break;
                case 0xE6: ANDm    break; case 0xE8: ADDSPm  break;
                case 0xE9: JPHL    break; case 0xEA: LDmmA   break;
                case 0xEE: XORm    break;

                case 0xF0: LDAIOm  break; case 0xF1: POPF    break;
                case 0xF2: LDAIOC  break; case 0xF3: DI      break;
                case 0xF5: PUSHF   break; case 0xF6: ORm     break;
                case 0xF8: LDHLSP  break; case 0xF9: LDSPHL  break;
                case 0xFA: LDAmm   break; case 0xFB: EI      break;
                case 0xFE: CPm     break;
                default:   INVALID;
            }
        break;
        case 8 ... 0xD: case 0xF:
            /* 8-bit load, LD or LDrHL */
            if (opL == 0x6 || opL == 0xE ) { LDrHL } else { LD }
        break;
        case 0xE:
            /* 8-bit load, LDHLr or HALT */
            if (opL != 0x6) { LDHLr } else { HALT }
        break;
        case 0x10 ... 0x17:
            /* 8-bit arithmetic */
            hl = CPU_RB (ADDR_HL);
            switch (op)
            {
                case 0x80      ... 0x85:  case 0x87: ADD     break;
                case 0x88      ... 0x8D:  case 0x8F: ADC     break;
                case 0x86: ADHL    break; case 0x8E: ACHL    break;
                case 0x90      ... 0x95:  case 0x97: SUB     break;
                case 0x98      ... 0x9D:  case 0x9F: SBC     break;
                case 0x96: SBHL    break; case 0x9E: SCHL    break;
                case 0xA0      ... 0xA5:  case 0xA7: AND     break;
                case 0xA8      ... 0xAD:  case 0xAF: XOR     break;
                case 0xA6: ANHL    break; case 0xAE: XRHL    break; 
                case 0xB0      ... 0xB5:  case 0xB7: OR      break;
                case 0xB8      ... 0xBD:  case 0xBF: CP      break;
                case 0xB6: ORHL    break; case 0xBE: CPHL    break; 
            }
        break;
    }

    gb->rt += opTicks[op];
    gb->clock_t += gb->rt;

    return gb->rt;
}

void gb_handle_interrupts (struct GB * gb)
{
    /* Get interrupt flags */
    const uint8_t io_IE = CPU_RB (0xFF00 + IntrEnabled);
    const uint8_t io_IF = CPU_RB (0xFF00 + IntrFlags);

    /* Run if CPU ran HALT instruction or IME enabled w/flags */
    if (gb->halted || (gb->ime && (io_IE & io_IF & IF_Any)))
    {
        gb->halted = 0;
        gb->ime = 0;

        /* Check all 5 IE and IF bits for flag confirmations 
           This loop also services interrupts by priority (0 = highest) */
        uint8_t i;
        for (i = 0; i <= 5; i++)
        {
            const uint16_t requestAddress = 0x40 + (i * 8);
            const uint8_t flag = 1 << i;

            if ((io_IE & flag) && (io_IF & flag))
            {
                gb->clock_t += 8;
                gb->sp -= 2;
                //CPU_WB (gb->sp, gb->pc >> 8);
                //gb->sp--;
                //CPU_WB (gb->sp, gb->pc & 0xFF);
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
    TICKS_TRANSFER    = 172,
    TICKS_HBLANK      = 204,
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

	/* Check if background is enabled */
	if (LCDC_(0))
	{
        /* Get minimum starting address depends on LCDC bits 3 and 6 are set.
        Starts at 0x1800 as this is the VRAM index minus 0x8000 */
        const uint16_t BGTileMap  = (gb->io[LCDControl] & 0x08) ? 0x9C00 : 0x9800;
        //const uint16_t winTileMap = (gb->io[IO_LCDControl] & 0x40) ? 0x9C00 : 0x9800;

        /* X position counter */
        uint8_t lineX = 0;
        const uint8_t lineY = gb->io[LY];

        assert (gb->io[LY] < DISPLAY_HEIGHT);

        /* Run at least 20 times (for the 160 pixel length) */
        for (lineX = 0; lineX < DISPLAY_WIDTH; lineX += 8)
        {
            /* BG tile fetcher gets tile ID. Bits 0-4 define X loction, bits 5-9 define Y location
            All related calculations following are found here:
            https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md */

            const uint8_t posX = lineX + gb->io[ScrollX];
            const uint8_t posY = lineY + gb->io[ScrollY];

            uint16_t tileAddr = BGTileMap + 
                ((posY / 8) << 5) +  /* Bits 5-9, Y location */
                (posX / 8);          /* Bits 0-4, X location */

            uint16_t tileID = gb->vram[tileAddr & 0x1FFF];

            /* Tilemap location depends on LCDC 4 set, which are different rules for BG and Window tiles */

            /* Fetcher gets low byte and high byte for tile */
            const uint16_t bit12 = !((gb->io[LCDControl] & 0x10) || (tileID & 0x80)) << 12;
            const uint16_t tileRow = 0x8000 + bit12 + (tileID << 4) + ((posY & 7) << 1);

            /* Finally get the pixel bytes from these addresses */
            const uint8_t byteLo = gb->vram[tileRow & 0x1FFF];
            const uint8_t byteHi = gb->vram[(tileRow + 1) & 0x1FFF];

            /* Produce pixel data from the combined bytes*/
            int x;
            for (x = 0; x < 8; x++)
            {
                const uint8_t bitLo = (byteLo >> (7 - x)) & 1;
                const uint8_t bitHi = (byteHi >> (7 - x)) & 1;
                pixels[lineX + x] = (bitHi << 1) + bitLo;
            }
        }
    }

    return pixels;
}

inline void gb_oam_read (struct GB * gb) { }

inline void gb_hblank (struct GB * gb) { }

inline void gb_vblank (uint8_t * io) { }

/* Evaluate LY=LYC */

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
            gb_vblank (gb->io);
    }
}

void gb_render (struct GB * const gb)
{
    //uint8_t frame = 0;

    /* Todo: continuously fetch pixels clock by clock for LCD data transfer.
       Similarly do clock-based processing for the other actions. */
    if (gb->io[LY] < DISPLAY_HEIGHT)
    {
        /* Visible line, within screen bounds */
        if (gb->lineClock < TICKS_OAM_READ)
        {
            /* Mode 2 - OAM read */
            if (IO_STAT_MODE != Stat_OAM_Search)
            {
                gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_OAM_Search;

                if (gb->io[LCDStatus] & 0x20) /* Mode 2 interrupt */
					gb->io[IntrFlags] |= IF_LCD_STAT;

                /* Fetch OAM data for sprites to be drawn on this line */
                //ppu_OAM_fetch (ppu, io_regs);
            }
        }
        else if (gb->lineClock < TICKS_OAM_READ + TICKS_TRANSFER)
        {
            /* Mode 3 - Transfer to LCD */
            if (IO_STAT_MODE != Stat_Transfer)
            {
                gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_Transfer;
            }
        }
        else if (gb->lineClock < TICKS_OAM_READ + TICKS_TRANSFER + TICKS_HBLANK)
        {
            /* Mode 0 - H-blank */
            if (IO_STAT_MODE != Stat_HBlank)
            {   
                /* Fetch line of pixels for the screen and draw them */
                uint8_t * pixels = gb_pixel_fetch (gb);
                gb->draw_line (gb->extData.ptr, pixels, gb->io[LY]);

                gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_HBlank;

                if (gb->io[LCDStatus] & 0x08) /* Mode 0 interrupt */
					gb->io[IntrFlags] |= IF_LCD_STAT;
            }
        }
        else
        {
            /* Starting new line */
            gb->io[LY] = (gb->io[LY] + 1) % SCAN_LINES;
            gb->lineClock -= (TICKS_OAM_READ + TICKS_TRANSFER + TICKS_HBLANK);

            if (gb->io[LYC] == gb->io[LY])
            {
                /* Set bit 02 flag for comparing lYC and LY */
                gb->io[LCDStatus] |= (1 << 2);

                /* If STAT interrupt is enabled, an interrupt is requested */
                if (gb->io[LCDStatus] & 0x40) /* LYC = LY stat interrupt */
					gb->io[IntrFlags] |= IF_LCD_STAT;
            }
            else
                /* Unset the flag */
                gb->io[LCDStatus] &= ~(1 << 2);

            /* Check if all visible lines are done */
            if (gb->io[LY] == DISPLAY_HEIGHT)
            {
                /* Enter Vblank and indicate that a frame is completed */
                gb->io[LCDStatus] = IO_STAT_CLEAR | Stat_VBlank;

                gb->io[IntrFlags] |= IF_VBlank;

                if (gb->io[LCDStatus] & 0x10) /* Mode 1 interrupt */
					gb->io[IntrFlags] |= IF_LCD_STAT;

                //frame = 1;
            }
        }
    }
    else /* Outside of screen Y bounds */
    {
        /* Mode 1 - V-blank */
        if(gb->lineClock >= TICKS_VBLANK)
        {
            //printf("Read %d lines. Took %d cycles. (%d leftover)\n", gb->io[IO_LineY], ppu->frameTicks, ppu->ticks);

            /* Advance though lines below the screen */
            gb->io[LY] = (gb->io[LY] + 1) % SCAN_LINES;
            gb->lineClock -= TICKS_VBLANK;

            if (gb->io[LYC] == gb->io[LY])
            {
                /* Set bit 02 flag for comparing lYC and LY */
                gb->io[LCDStatus] |= (1 << 2);

                /* If STAT interrupt is enabled, an interrupt is requested */
                if (gb->io[LCDStatus] & 0x40) /* LYC = LY stat interrupt */
					gb->io[IntrFlags] |= IF_LCD_STAT;
            }
            else
                /* Unset the flag */
                gb->io[LCDStatus] &= ~(1 << 2);

            if (gb->io[LY] == 0)
            {
                /* Return to top line and OAM read */
                //ppu->frameTicks -= 70224;
            }
        }
    }

    /* ...and DMA transfer to OMA, if needed */
    //DMA_to_OAM_transfer (ppu);

    //return frame;
}