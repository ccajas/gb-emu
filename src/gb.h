#ifndef GB_H
#define GB_H

#include "cart.h"
#include "io.h"

#define FRAME_CYCLES      70224
#define DISPLAY_WIDTH     160
#define DISPLAY_HEIGHT    144
#define SCAN_LINES        154

#define VRAM_SIZE     0x2000
#define WRAM_SIZE     0x2000
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
    enum { A = 0, B, C, D, E, H, L, F = 10 } registers;

    uint8_t r[7];     /* A-E, H, L - 8-bit registers */
    
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
    
    uint16_t pc, sp;
    uint64_t clock_t;
    uint32_t frameClock;
    uint8_t  rt; /* Tracks individual step clocks */

    uint8_t  stop, halted;
    uint8_t  invalid;

    /* Memory and I/O registers */
    uint8_t vram[VRAM_SIZE];
    uint8_t ram [WRAM_SIZE];   /* Work RAM  */
    uint8_t hram[HRAM_SIZE];   /* High RAM  */
    uint8_t io  [IO_SIZE];

    /* Interrupt master enable */
    uint8_t ime;

    /* Directly accessible data by frontend */
    struct gb_data_s
    {
        /* Joypad button inputs */
        uint8_t joypad;

        /* Used for drawing the display and tilemap */
#ifdef GB_APP_DRAW
        struct Texture tileMap;
        struct Texture frameBuffer;
#endif
    }
    gbData_;
};

static uint8_t gb_mem_access (struct GB * gb, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    /* For debug logging purposes */
    if (addr == 0xFF44 && !write) return 0x90;

    /* Byte to be accessed from memory */
    uint8_t * b;
    switch (addr)
    {
        case 0x0000 ... 0x7FFF:  return 0;//mbc_rw (addr, val, write);       /* ROM from MBC     */
        case 0x8000 ... 0x9FFF:  return 0;//ppu_rw (addr, val, write);       /* Video RAM        */
        case 0xA000 ... 0xBFFF:  return 0;//mbc_rw (addr, val, write);       /* External RAM     */
                                                                         /* Work RAM         */
        case 0xC000 ... 0xDFFF:  b = &gb->ram[addr % 0x2000]; if (write) *b = val; return *b;
        case 0xE000 ... 0xFDFF:  return 0xFF;                            /* Echo RAM         */
        case 0xFE00 ... 0xFE9F:  return 0;// ppu_rw (write, addr, val);       /* OAM              */
        case 0xFEA0 ... 0xFEFF:  return 0xFF;                            /* Not usable       */
                                                                         /* I/O registers    */
        case 0xFF00 ... 0xFF7F:  b = &gb->io[addr % 0x80];   if (write) *b = val; return *b;
                                                                         /* High RAM         */                          
        case 0xFF80 ... 0xFFFE:  b = &gb->hram[addr % 0x80]; if (write) *b = val; return *b;  
                                                                         /* Interrupt enable */
        case 0xFFFF:             b = &gb->io[addr & 0x7F];   if (write) *b = val; return *b;  
    }
}

#include "ops.h"

static const int8_t opTicks[256] = {
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

static uint8_t gb_exec_cb (struct GB * gb, const uint8_t op)
{
    return 0;
}

static uint8_t gb_cpu_exec (struct GB * gb, const uint8_t op)
{
    gb->rt = 0;

    const uint8_t  opL = op & 0xf;
    const uint8_t  opHh = op >> 3; /* Octal divisions */
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

static void gb_handle_interrupts (struct GB * gb)
{

}

static void gb_handle_timings (struct GB * gb)
{

}

static void gb_render (struct GB * gb)
{

}

static uint8_t gb_step (struct GB * gb)
{
    gb_handle_interrupts (gb);

    /* Load next op and execute */
    const uint8_t op = gb_mem_access (gb, gb->pc++, 0, 0);
    gb->frameClock += gb_cpu_exec (gb, op);

    gb_handle_timings (gb);
    gb_render (gb);

    /* Check if a frame is done */
    const uint32_t lastClock = gb->frameClock;
    uint8_t frameDone = 0;
    gb->frameClock %= FRAME_CYCLES;

    if (lastClock > gb->frameClock) frameDone = 1;

    return frameDone;
}

#endif