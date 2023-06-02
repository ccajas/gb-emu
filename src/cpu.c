#include <stdio.h>
#include <string.h>
#include <profileapi.h>
#include "cpu.h"
#include "ops.h"

struct CPU cpu;

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

/* CPU related functions */

uint8_t cpu_mem_access (const uint16_t addr, const uint8_t val, const uint8_t write)
{
    /* For debug logging purposes */
    if (addr == 0xFF44 && !write) return 0x90;

    /* Byte to be accessed from memory */
    uint8_t * b;
    #define BYTE_RW(b)  { if (write) *b = val; return *b; }
    switch (addr)
    {
        case 0x0000 ... 0x7FFF: return mbc_rw (addr, val, write);      /* ROM from MBC     */
        case 0x8000 ... 0x9FFF: return ppu_rw (addr, val, write);      /* Video RAM        */
        case 0xA000 ... 0xBFFF: return mbc_rw (addr, val, write);      /* External RAM     */
        case 0xC000 ... 0xDFFF: b = &cpu.ram[addr % 0x2000];           /* Work RAM         */
                                BYTE_RW (b);
        case 0xE000 ... 0xFDFF: return 0xFF;                           /* Echo RAM         */
        case 0xFE00 ... 0xFE9F: return ppu_rw (write, addr, val);      /* OAM              */
        case 0xFEA0 ... 0xFEFF: return 0xFF;                           /* Not usable       */
        case 0xFF00 ... 0xFF7F: b = &cpu.io[addr % 0x80]; BYTE_RW(b);  /* I/O registers    */                        
        case 0xFF80 ... 0xFFFE: b = &cpu.hram[addr % 0x80];            /* High RAM         */  
                                BYTE_RW (b);
        case 0xFFFF:            b = &cpu.io[addr & 0x7F]; BYTE_RW(b);  /* Interrupt enable */
    }
}

inline uint8_t cpu_read(const uint16_t addr)
{
    return cpu_mem_access(addr, 0, 0);
}

void cpu_state ()
{
    const uint16_t pc = cpu.pc;

    printf("%08X ", (uint32_t) cpu.clock_t);
    printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X "
        "SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
        cpu.r[A], cpu.flags, cpu.r[B], cpu.r[C], cpu.r[D], cpu.r[E], cpu.r[H], cpu.r[L], 
        cpu.sp, cpu.pc, cpu_read (pc), cpu_read (pc+1), cpu_read (pc+2), cpu_read (pc+3)
    );
}

void cpu_boot_reset ()
{
    cpu.r[A]	= 0x01;
    cpu.flags	= 0xB0;
    cpu.r[B]	= 0x0;
    cpu.r[C]	= 0x13;
    cpu.r[D]	= 0x0;
    cpu.r[E]	= 0xD8;
    cpu.r[H]	= 0x01;
    cpu.r[L]	= 0x4D;
    cpu.sp     = 0xFFFE;
    cpu.pc     = 0x0100;

    cpu.ime = 1;
    cpu.invalid = 0;
    cpu.halted = 0;
}

inline void cpu_handle_interrupts ()
{
    /* Handle interrupts */
    const uint8_t io_IE = CPU_RB (0xFF00 + IntrEnabled);
    const uint8_t io_IF = CPU_RB (0xFF00 + IntrFlags);

    /* Run if CPU ran HALT instruction or IME enabled w/flags */
    if (cpu.halted || (cpu.ime && (io_IE & io_IF & IF_Any)))
    {
        cpu.halted = 0;
        cpu.ime = 0;

        /* Increment clock and push PC to SP */
        cpu.clock_t += 8;
        CPU_WW (cpu.sp, cpu.pc);

        /* Check all 5 IE and IF bits for flag confirmations 
           This loop also services interrupts by priority (0 = highest) */
        uint8_t i;
        for (i = 0; i <= 5; i++)
        {
            const uint16_t requestAddress = 0x40 + (i * 8);
            const uint8_t flag = 1 << i;

            if ((io_IE & flag) && (io_IF & flag))
            {
                /* Clear flag bit */
                CPU_WB (0xFF00 + IntrFlags, io_IF ^ flag);
                cpu.sp -= 2;

                /* Move PC to request address */
                cpu.clock_t += 8;
                cpu.pc = requestAddress;
                cpu.clock_t += 4;
            }
        }
    }
}

inline uint8_t cpu_step()
{
    /* Interrupt handling and timers */
    cpu_handle_interrupts();

    if (cpu.halted)
    {
        cpu.rt = 4;
        cpu.clock_t += cpu.rt;

        return cpu.rt;
    }

    /* Load next op and execute */
    const uint8_t op = cpu_read (cpu.pc++);
    cpu.frameClock += cpu_exec (op);
    //ppu_step (cpu.io);
    //cpu_state();

    /* Check if a frame is done */
    uint8_t frameDone = 0;

    if (cpu.frameClock >= FRAME_CYCLES)
    {
        cpu.frameClock -= FRAME_CYCLES;
        frameDone = 1;
    }

    return frameDone;
}

void cpu_frame()
{
    /* Returns 1 when frame is completed */
    while (!cpu_step()) { /* Do extra stuff in between steps here */ }
}


uint8_t cpu_exec (const uint8_t op)
{
    cpu.rt = 0;

    const uint8_t  opL = op & 0xf;
    const uint8_t  opHh = op >> 3; /* Octal divisions */
    uint16_t hl = ADDR_HL;

    /* Default values for operands (can be overridden for other opcodes) */
    const uint8_t  r1 = ((opHh & 7) == 7) ? A : B + (opHh & 7);
    const uint8_t  r2 = ((opL & 7)  == 7) ? A : ((opL & 7) < 6) ? B + (opL & 7) : 255;

    uint8_t tmp = cpu.r[A];

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

    cpu.rt += opTicks[op];
    cpu.clock_t += cpu.rt;

    return cpu.rt;
}

void cpu_exec_cb (const uint8_t op)
{   
    const uint8_t opL  = op & 0xf;
    const uint8_t opHh = op >> 3; /* Octal divisions */

    const uint8_t r = ((opL & 7) == 7) ? A : ((opL & 7) < 6) ? B + (opL & 7) : 255;
    const uint8_t r_bit  = opHh & 7;

    /* Fetch value at address (HL) if it's needed */
    uint8_t hl = (opL == 0x6 || opL == 0xE) ? CPU_RB (ADDR_HL) : 0;

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
                
                case 0x38      ... 0x3D:  case 0x3F: SRL     break;
            }
        break;
        case 8 ... 0xF:
            /* Bit test */
            if (opL == 0x6 || opL == 0xE ) { BITHL } else { BIT }
        break;
        case 0x10 ... 0x17:
            /* Bit reset */
            if (opL == 0x6 || opL == 0xE ) { RESHL } else { RES }
        break;
        case 0x18 ... 0x1F:
            /* Bit set */
            if (opL == 0x6 || opL == 0xE ) { SETHL } else { SET }
        break;
    }
}