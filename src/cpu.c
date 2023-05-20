#include <string.h>
#include <profileapi.h>
#include "gb.h"
#include "ops.h"

GameBoy GB;

CPU * cpu = &GB.cpu;
MMU * mmu = &GB.mmu;

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

void cpu_init()
{
    strcpy(cpu->reg_names, "ABCDEHL");
    cpu->ni = 0;
}

void cpu_print_regs()
{
#ifdef GB_DEBUG
    int i;
    for (i = A; i < L; i++)
        LOG_("%d ", cpu->r[i]);
#endif
}

void cpu_state()
{
#ifndef GB_DEBUG
    const uint16_t pc = cpu->pc;

    printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
        cpu->r[A], cpu->flags, cpu->r[B], cpu->r[C], cpu->r[D], cpu->r[E], cpu->r[H], cpu->r[L], 
        cpu->sp, cpu->pc, 
        CPU_RB (pc), CPU_RB (pc+1), CPU_RB (pc+2), CPU_RB (pc+3)
    );
#endif
}

void cpu_boot_reset()
{
    cpu->r[A]	= 0x01;
    cpu->flags	= 0xB0; /* (Z-HC) */
    cpu->r[B]	= 0x0;
    cpu->r[C]	= 0x13;
    cpu->r[D]	= 0x0;
    cpu->r[E]	= 0xD8;
    cpu->r[H]	= 0x01;
    cpu->r[L]	= 0x4D;
    cpu->sp     = 0xFFFE;
    cpu->pc     = 0x0100;

    cpu->ime = 0;
    cpu->invalid = 0;
}

#ifdef EXEC_TEST

void cpu_exec(uint8_t const op) { }

#else

void cpu_exec (uint8_t const op, uint32_t const cl)
{
    cpu->rm = 0;
    cpu->rt = 0;

    /* LOG_("Running op %02x:\n", op); */
    uint8_t opL = op & 0xf;
    /* uint8_t opH  = op >> 4; */
    uint8_t opHh = op >> 3; /* Octal divisions */
    uint8_t r1 = 0, r2 = 0;
    uint16_t hl = ADDR_HL;

    /* Default values for opcode arguments (can be overridden for other opcodes) */
    r1 = (opHh == 0x7) ? A : B + (opHh & 7); 
    r2 = ((opL & 7) == 7) ? A : ((opL & 7) < 6) ? B + (opL & 7) : 255;

#ifdef TABLE
    switch (op)
    {
        case 0x0:                        NOP; break;
        case 0x01: case 0x11: case 0x21: LDrr   (r1, r1 + 1); break;
        case 0x02: case 0x12:            LDrrmA (r1, r1 + 1); break;
        case 0x03: case 0x13: case 0x23: INCrr  (r1, r1 + 1); break; /* 16-bit increment */
        case 0x04: case 0x14: case 0x24:
        case 0x0C: case 0x1C: case 0x2C: case 0x3C: INC (r1) break;
        case 0x05: case 0x15: case 0x25:
        case 0x0D: case 0x1D: case 0x2D: case 0x3D: DEC (r1) break;
        case 0x06: case 0x16: case 0x26:
        case 0x0E: case 0x1E: case 0x2E: case 0x3E: LDrn (r1) break;
        case 0x07: RLCA;   break;        case 0x08: LDimSP; break;
        case 0x09: case 0x19: case 0x29: { ADHLrr (r1 - 1, r1); break; }
        case 0x39: ADHLSP; break;        /* 16-bit add */
        case 0x0A: case 0x1A:            LDArrm (r1 - 1, r1); break;
        case 0x0B: case 0x1B: case 0x2B: DECrr  (r1 - 1, r1); break; /* 16-bit decrement */
        case 0x10: STOP;   break;        case 0x20: JRNZ; break;

        case 0x2A: LDAHLI; break;
        case 0x3A: LDAHLD; break;

        case 0x40 ...   0x45: case 0x47: case 0x50 ...   0x55: case 0x57:  
        case 0x60 ...   0x65: case 0x67: case 0x70 ...   0x75: case 0x77:
        case 0x48 ...   0x4D: case 0x4F: case 0x58 ...   0x5D: case 0x5F:  
        case 0x68 ...   0x6D: case 0x6F:  
        case 0x78 ...   0x7D: case 0x7F: LD (r1, r2); break;
        case 0x46: case 0x56: case 0x66: 
        case 0x4E: case 0x5E: case 0x6E: case 0x7E: LDrHL (r1); break;
        case 0x76: HALT; break;
                 
        break;
    }

#else

    switch (opHh)
    {
        case 0 ... 5: case 0x7:

            r1 = (opHh == 0x7) ? A : B + opHh; 
            switch (opL)
            {
                case 0: /* JMP and CPU operations */
                    if (op == 0x0)  { NOP; }
                    if (op == 0x10) { STOP; }
                    if (op == 0x20) { JRNZ; }
                break;
                case 1: /* 16-bit load, LDrr */
                    LDrr (r1, r1 + 1); break;
                case 2: /* 8-bit load, LDrrmA or LDHLIA */
                    if (op == 0x02 || op == 0x12) { LDrrmA (r1, r1 + 1); }
                    if (op == 0x22) { LDHLIA; }
                break;
                case 3: /* 16-bit increment */
                    INCrr (r1, r1 + 1); break;
                case 4: 
                case 0xC: /* 8-bit increment */
                    INC (r1); break;
                case 5:
                case 0xD: /* 8-bit decrement */
                    DEC (r1); break;
                case 6:
                case 0xE: /* Load 8-bit immediate, LDrm */
                    LD_X_Y (r1, CPU_RB (cpu->pc++)); break;
                case 7: /* Bit and flag operations */
                    if (op == 0x07) { RLCA; }
                    if (op == 0x17) { RLA; }
                break;
                case 8: /* JMP and load operations */
                    if (op == 0x08) { LDimSP; }
                    if (op == 0x18) { JRm; }
                    if (op == 0x28) { JRZ; }
                    if (op == 0x38) { JRC; }
                break;  
                case 9: /* 16-bit add */
                    if (opHh <= 5) { ADHLrr (r1 - 1, r1); } else { ADHLSP; } break;
                break;
                case 0xA:
                    if (op < 0x2A)  { LDArrm (r1 - 1, r1); }
                    if (op == 0x2A) { LDAHLI }
                    if (op == 0x3A) { LDAHLD }
                break;
                case 0xB: /* 16-bit decrement */
                    if (opHh <= 5) { DECrr (r1 - 1, r1); } else { DECSP; } break;
                case 0xF: /* Other CPU/etc ops */
                    if (op == 0x0F) { RRCA; }
                    if (op == 0x1F) { RRA; }
                    if (op == 0x2F) { CPL; }
                    if (op == 0x3F) { CCF; } 
                break;
            }
        break;
        case 6:
            if (op == 0x30) { JRNC; }
            if (op == 0x31) { LDSP; }
            if (op == 0x32) { LDHLDA; }
            if (op == 0x33) { INCSP; }
            if (op == 0x34) { INCHL; }
            if (op == 0x35) { DECHL; }
            if (op == 0x36) { LDHL_X (CPU_RB (cpu->pc++)); /* LDHLm */ }
            if (op == 0x37) { SCF; }
        break;
        case 8 ... 0xD: case 0xF:
            /* 8-bit load, LD or LDrHL */
            r1 = (opHh == 0xF) ? A : B + (opHh - 8);
            if (r2 != 255) { LD_X_Y (r1, cpu->r[r2]); } else { LD_X_Y (r1, HL_ADDR_BYTE); }
        break;
        case 0xE:
            /* 8-bit load, LDHLr or HALT */
            if (r2 != 255) { LDHL_X (cpu->r[r2]); } else { HALT; }
        break;
        /*case 0x10:
            if (r2 != 255) { ADD (r2); } else { ADHL; }
        break;*/
        case 0x11 ... 0x17:
            /* 8-bit arithmetic */
            if (r2 != 255)
            {
                uint8_t tmp = cpu->r[A]; 
                switch (opHh - 0x10)
                {
                    case 0: { ADD (r2); } break;
                    case 1: { ADC (r2); } break;
                    case 2: { SUB (r2); } break;
                    case 3: { SBC (r2); } break;
                    case 4: { AND (r2); } break;
                    case 5: { XOR (r2); } break;
                    case 6: { OR (r2); } break;
                    case 7: { CP (r2); } break;
                }
            }
            else {
                uint8_t hl = HL_ADDR_BYTE;
                uint8_t tmp = cpu->r[A]; 

                switch (opHh - 0x10)
                {
                    case 0: { ADHL; } break;
                    case 1: { ACHL; } break;
                    case 2: { SBHL; } break;
                    case 3: { SCHL; } break;
                    case 4: { ANHL; } break;
                    case 5: { XRHL; } break;
                    case 6: { ORHL; } break;
                    case 7: { CPHL; } break;
                }
            }
        break;
        case 0x18 ... 0x1F:

            /* Used for PUSH and POP instructions */
            r1 = (opHh - 0x18 >= 0x6) ? A : B + (opHh - 0x18);
            switch (opL)
            {
                case 0: /* RET and LD IO operations */
                    if (op == 0xE0) { LDIOmA; }
                    if (op == 0xF0) { LDA_X (0xFF00 + CPU_RB (cpu->pc++)); /* LDAIOm */ }
                break;
                case 1: /* POP operations */
                    if (r1 != A) { POP  (r1, r1 + 1) } else { POPF; }
                break;  
                case 2: /* JP and LD IO operations */
                    if (op == 0xF2) { LDA_X (0xFF00 + cpu->r[C]); /* LDAIOC */ }
                break;
                case 3:
                    if      (op == 0xC3) { JPNN; }
                    else if (op == 0xF3) { DI; }
                    else    { INVALID; }
                break;
                case 4:
                case 0xC:
                    if (op == 0xC4) { CALLNZ; }
                    else if (op == 0xCC) { CALLZ; }
                    else if (op == 0xD4) { CALLNC; }
                    else if (op == 0xDC) { CALLC;  }
                    else    { INVALID; }
                break;
                case 5: /* PUSH operations */
                    if (r1 != A) { PUSH (r1, r1 + 1) } else { PUSHF; }
                break;
                case 6: /* 8-bit operations, immediate */
                    if (op == 0xC6) { ADDm; }
                    if (op == 0xD6) { SUBm; }
                    if (op == 0xE6) { ANDm; }
                    if (op == 0xF6) { ORm;  }                    
                break;
                case 0x07 : case 0x0f: /* Call to address xx */
                    { uint16_t n = (opHh - 0x18) * 0x08; RST(n); }
                break;
                case 8:
                    if (op == 0xE8) { ADDSPm; }
                break;
                case 9:
                    if (op == 0xC9) { RET;  }
                    if (op == 0xD9) { RETI; }
                    if (op == 0xE9) { JPHL; }
                break;
                case 0xA:
                    if (op == 0xEA) { LDmmA; }
                    if (op == 0xFA) { LDAmm; }
                case 0xB:
                    if      (op == 0xCB) { PREFIX; }
                    else if (op == 0xFB) { EI; }
                    else    { INVALID; }
                break;
                case 0xD:
                    if   (op == 0xCD) { CALLm; }
                    else { INVALID; }
                break;
                case 0xE:
                    if (op == 0xEE) { XORm; }
                    if (op == 0xFE) { CPm; }
                break;
            }
        break;
        default:
            cpu->rm = 0;
        break;
    }
#endif

    cpu->rt += opTicks[op];
    cpu->clock_t += cpu->rt;
    cpu->clock_m += (cpu->rt >> 2);

    if (cpu->rt == 0) { LOG_("*N/A (%02x)*\n", op); cpu->ni++; } else { LOG_("\n"); }
}

#endif

void cpu_exec_cb (uint8_t const op)
{
    cpu->rm = 2;
}

void cpu_clock()
{   
    /*Load next op and execute */
    uint16_t i;   
    for (i = 0; i < 50000; i++)
    {
        uint8_t op = mmu_rb(mmu, cpu->pc++);
        LOG_("Test op %02x... ", op);

        cpu_exec(op, i+1);
        cpu_state();
    }
}
 