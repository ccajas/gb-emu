
#include <string.h>
#include "gb.h"

#define DEBUG_OP_PRINT

GameBoy GB;

CPU * cpu = &GB.cpu;
MMU * mmu = &GB.mmu;

#define CPU_RB(A)     mmu_rb(mmu, A)
#define CPU_WB(A,X)   mmu_wb(mmu, A, X)
#define CPU_RW(A)     mmu_rw(mmu, A)
#define CPU_WW(A,X)   mmu_ww(mmu, A, X)

/* Instruction helpers */

#define ADDR_HL       ((cpu->r[H] << 8) + cpu->r[L])
#define ADDR_XY(X,Y)  ((cpu->r[X] << 8) + cpu->r[Y])
#define INCR_HL       cpu->r[L]++; if (!cpu->r[L]) cpu->r[H]++
#define DECR_HL       cpu->r[L]--; if (cpu->r[L] == 255) cpu->r[H]--
#define HL_ADDR_BYTE  mmu_rb (mmu, ADDR_HL)

#ifdef GB_DEBUG

#define NYI(S)           LOG_("Instruction not implemented! (%s)", S); cpu->ni++;
#define OP(name)         LOG_("%s", #name);
#define OPX(name, X)     LOG_("%s %c", #name, cpu->reg_names[X]);
#define OPXY(name, X, Y) LOG_("%s %c,%c", #name, cpu->reg_names[X], cpu->reg_names[Y]);
#define OPN(name, N)     LOG_("%s 0x%02x", #name, N);

#else

#define NYI(S)
#define OP(name)
#define OPX(name, X)
#define OPXY(name, X, Y)
#define OPN(name, N)

#endif  

/* 8-bit load instructions */   

/* Load function templates */
/* Used for LDrr, LDrn, LDrHL */
#define LD_X_Y(X,Y) OPXY(LD_X_Y, X, Y); cpu->r[X] = Y; cpu->rm = 1; /* 2 for (HL),n */ 

/* Used for LDHLr, LDHLn */
#define LDHL_X(X) OPX(LDHL_X, X); CPU_WB (ADDR_HL, X); cpu->rm = 2; /* 3 for LDHLn */

/* Used for LDArrm, LDAIOm, LDAIOC */
#define LDA_X(X)  OPX(LDA_X, X);  cpu->r[A] = CPU_RB (X); cpu->rm = 2; /* 3 for LDAIOm */


#define LDAmm        OP(LDAmm);          cpu->r[A] = CPU_RB (CPU_RW (cpu->pc)); /*cpu->pc += 2;*/ cpu->rm = 4;

#define LDrrmA(X,Y)  OPXY(LDrrmA, X, Y); CPU_WB (ADDR_XY(X,Y), cpu->r[A]); cpu->rm = 2;  //02, 12

#define LDmmA     OP(LDmmA);  CPU_WB (CPU_RW (cpu->pc), cpu->r[A]);  cpu->pc += 2; cpu->rm = 4;
#define LDIOmA    OP(LDIOmA); CPU_WB (0xFF00 + CPU_RB (cpu->pc++), cpu->r[A]); cpu->rm = 3;

#define LDHLIA    OP(LDHLIA); CPU_WB (ADDR_HL, cpu->r[A]); INCR_HL; cpu->rm=2; //22
#define LDHLDA    OP(LDHLDA); CPU_WB (ADDR_HL, cpu->r[A]); DECR_HL; cpu->rm=2; //32

#define LDAHLI    OP(LDAHLI); cpu->r[A] = HL_ADDR_BYTE;    INCR_HL; cpu->rm=2; //2A
#define LDAHLD    OP(LDAHLD); cpu->r[A] = HL_ADDR_BYTE;    DECR_HL; cpu->rm=2; //3A

/* 16-bit load instructions */

#define LDrr(X,Y) OPXY(LDrr, X, Y); cpu->r[Y] = CPU_RB (cpu->pc); cpu->r[X] = CPU_RB (cpu->pc + 1); cpu->pc += 2; cpu->rm = 3;
#define LDimSP    OP(LDimSP); uint16_t val = CPU_RW (cpu->pc); CPU_WW (val, cpu->sp); cpu->pc += 2; cpu->rm = 5;
#define LDSP      OP(LDSP);   cpu->sp = CPU_RW (cpu->pc); cpu->pc += 2; cpu->rm =3;

#define PUSH(X,Y) OPXY(PUSH, X, Y); cpu->sp--; CPU_WB(cpu->sp, cpu->r[X]); cpu->sp--; CPU_WB(cpu->sp, cpu->r[Y]); cpu->rm = 4;
#define POP(X,Y)  OPXY(POP, X, Y);  cpu->r[Y] = CPU_RB(cpu->sp++); cpu->r[X] = CPU_RB(cpu->sp++); cpu->rm = 3;
#define PUSHF     OP(PUSHF); cpu->sp--; CPU_WB(cpu->sp, cpu->r[A]); cpu->sp--; CPU_WB(cpu->sp, cpu->flags); cpu->rm = 4;
#define POPF      OP(POPF);  cpu->flags = CPU_RB(cpu->sp++); cpu->r[A] = CPU_RB(cpu->sp++); cpu->rm = 3;

/* Flag setting helpers */

#define SET_ADD_CARRY(X,Y,T)    cpu->flags = __builtin_add_overflow (X, Y, T) << 4;
#define SET_SUB_CARRY(X,Y,T)    cpu->flags = __builtin_sub_overflow (X, Y, T) << 4;
#define SET_ZERO(X)    if (!X) cpu->flags |= FLAG_Z; else cpu->flags &= ~(FLAG_Z);
#define SET_HALF(X)    if ((X) & 0x10) cpu->flags |= FLAG_H; else cpu->flags &= ~(FLAG_H);
#define SET_HALF_S(X)  if (X) cpu->flags |= FLAG_H; else cpu->flags &= ~(FLAG_H);
#define SET_CARRY(X)   if (X) cpu->flags |= FLAG_C; else cpu->flags &= ~(FLAG_C);

#define KEEP_ZERO       cpu->flags &= 0x80;
#define KEEP_CARRY      cpu->flags &= 0x10;
#define KEEP_ZERO_CARRY cpu->flags &= 0x90;

#define SET_ADD_FLAGS(T,X)  cpu->flags = (cpu->r[A] < T) ? FLAG_C : 0; SET_ZERO(cpu->r[A]); SET_HALF(cpu->r[A] ^ X ^ T);
#define SET_SUB_FLAGS(T,X)  cpu->flags = (cpu->r[A] > T) ? FLAG_N | FLAG_C : FLAG_N; SET_ZERO(cpu->r[A]); SET_HALF(cpu->r[A] ^ X ^ T); 

/* 8-bit arithmetic/logic instructions */

/* Add function templates */
#define ADD_A_X(X)  cpu->r[A] += X; SET_ADD_FLAGS(tmp, X);
#define ADD_AC_X(X) cpu->r[A] += X; cpu->r[A] += (cpu->flags & FLAG_C) ? 1 : 0; SET_ADD_FLAGS(tmp, X);

#define ADD(X)   OPX(ADD, X); ADD_A_X(cpu->r[X]);
#define ADHL     OP(ADHL);    ADD_A_X(hl);
#define ADDm     OP(ADDm);    uint8_t m = CPU_RB(cpu->pc++); uint8_t tmp = cpu->r[A]; ADD_A_X(m);

#define ADC(X)   OPX(ADC, X); ADD_AC_X(cpu->r[X]);
#define ACHL     OP(ACHL);    ADD_AC_X(hl);

#define SUB(X)   OPX(SUB, X); cpu->r[A] -= cpu->r[X]; SET_SUB_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define SBC(X)   OPX(SBC, X); cpu->r[A] -= cpu->r[X]; cpu->r[A] -= (cpu->flags & FLAG_C) ? 1 : 0; SET_SUB_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define SBHL     OP(SBHL);    cpu->r[A] -= hl; SET_SUB_FLAGS(tmp, hl); cpu->rm = 2;
#define SCHL     OP(SCHL);    cpu->r[A] -= hl; cpu->r[A] -= (cpu->flags & FLAG_C) ? 1 : 0; SET_SUB_FLAGS(tmp, hl); cpu->rm = 2;
#define SUBm     OP(SUBm);    uint8_t m = CPU_RB(cpu->pc++); uint8_t tmp = cpu->r[A]; cpu->r[A] -= m; SET_SUB_FLAGS(tmp, m); cpu->rm = 2;
    
#define AND(X)   OPX(AND, X)  cpu->r[A] &= cpu->r[X]; cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->flags |= FLAG_H; cpu->rm = 1;
#define XOR(X)   OPX(XOR, X)  cpu->r[A] ^= cpu->r[X]; cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->rm = 1;
#define ANHL     OP(ANHL);    cpu->r[A] &= hl;        cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->flags |= FLAG_H; cpu->rm = 2;
#define XRHL     OP(XRHL);    cpu->r[A] ^= hl;        cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->rm = 2;
#define OR(X)    OPX(OR, X);  cpu->r[A] |= cpu->r[X]; cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->rm = 1;
#define ORHL     OP(ORHL)     cpu->r[A] |= hl;        cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->rm = 2;
#define CP(X)    OPX(CP, X);  tmp -= cpu->r[X]; cpu->flags = (tmp < 0) ? 0x50 : FLAG_N; SET_ZERO(tmp); SET_HALF(cpu->r[A] ^ cpu->r[X] ^ tmp); cpu->rm = 1;
#define CPHL     OP(CPHL);    tmp -= hl; cpu->flags = (tmp < 0) ? 0x50 : FLAG_N; SET_ZERO(tmp); SET_HALF(cpu->r[A] ^ hl ^ tmp); cpu->rm = 2;


#define INC(X)   OPX(INC, X); cpu->r[X]++; KEEP_CARRY; cpu->flags |= (cpu->r[X] & 0xF) ? 0 : 0x20; cpu->flags |= cpu->r[X] ? 0 : 0x80; cpu->rm = 1;
#define DEC(X)   OPX(DEC, X); cpu->r[X]--; KEEP_CARRY; cpu->flags |= ((cpu->r[X] & 0xF) == 0xF) ? 0x20 : 0; cpu->flags |= cpu->r[X] ? 0 : 0x80; cpu->flags |= 0x40; cpu->rm = 1;
#define INCHL    OP(INCHL);   uint8_t tmp = CPU_RB(ADDR_HL) + 1; CPU_WB(ADDR_HL, tmp); SET_ZERO(tmp); SET_HALF_S((tmp & 0x0F) == 0); cpu->flags &= ~(0x40); cpu->rm = 3;
#define DECHL    OP(DECHL);   uint8_t tmp = CPU_RB(ADDR_HL) - 1; CPU_WB(ADDR_HL, tmp); SET_ZERO(tmp); SET_HALF_S((tmp & 0x0F) == 0xF); cpu->flags |= 0x40; cpu->rm = 3;
#define CPL      OP(CPL);     cpu->r[A] ^= 0xFF; cpu->flags |= 0x60; cpu->rm = 1;

/* 16-bit arithmetic instructions */
    
#define ADHLrr(X,Y) OPXY(ADHLrr, X, Y); uint16_t tmp = hl; hl += ADDR_XY(X,Y); if (hl < tmp) cpu->flags |= FLAG_C; else cpu->flags &= 0xEF; cpu->r[H] = (hl >> 8); cpu->r[L] = hl & 0xFF; cpu->rm = 2;
#define ADHLSP      OP(ADHLSP);         uint16_t tmp = hl; hl += cpu->sp;      if (hl < tmp) cpu->flags |= FLAG_C; else cpu->flags &= 0xEF; cpu->r[H] = (hl >> 8); cpu->r[L] = hl & 0xFF; cpu->rm = 2;
#define ADDSPm      OP(ADDSPm);         int8_t i = (int8_t) CPU_RB(cpu->pc++); cpu->flags = 0; SET_HALF_S ((cpu->sp & 0xF) + (i & 0xF) > 0xF); SET_CARRY ((cpu->sp & 0xFF) + (i & 0xFF) > 0xFF); cpu->sp += i; cpu->rm = 4;
#define INCrr(X,Y)  OPXY(INCrr, X, Y);  cpu->r[Y]++; if (!cpu->r[Y]) cpu->r[X]++; cpu->rm = 2;
#define INCSP       OP(INCSP);          cpu->sp++; cpu->rm = 2;
#define DECrr(X,Y)  OPXY(DECrr, X, Y);  cpu->r[Y]--; if (cpu->r[Y] == 0xff) cpu->r[X]--; cpu->rm = 2;
#define DECSP       OP(DECSP);          cpu->sp++; cpu->rm = 2;

/* CPU control instructions */

#define CCF     OP(CCF);  KEEP_ZERO; uint8_t ci = (cpu->flags & 0x10) ^ 0x10; cpu->flags |= ci; cpu->rm = 1;
#define SCF     OP(SCF);  KEEP_ZERO; cpu->flags |= 0x10; cpu->rm = 1;
#define HALT    OP(HALT); cpu->halt = 1; cpu->rm = 1;
#define STOP    OP(STOP); cpu->stop = 1; cpu->rm = 1;
#define NOP     OP(NOP);  cpu->rm = 1;
#define DI      OP(DI);   cpu->ime = 0; cpu->rm = 1;
#define EI      OP(EI);   cpu->ime = 1; cpu->rm = 1;

/* Jump and call instructions */

#define JPNN    OP(JPNN); cpu->pc = CPU_RW(cpu->pc); cpu->rm = 3;
#define JPHL    OP(JPHL); cpu->pc = ADDR_HL; cpu->rm = 1;
#define JRm     OP(JRm);  cpu->pc += (int8_t) CPU_RB (cpu->pc); cpu->pc++; cpu->rm = 3;
#define JRZm    NYI("JRZm"); cpu->rm = 1;
#define JRCm    NYI("JRCm"); cpu->rm = 1;

#define JRNZ {\
    cpu->rm = 2;\
    if((cpu->flags & 0x80) == 0) {\
        int8_t ci = (int8_t) CPU_RB (cpu->pc++);\
        cpu->pc += ci;\
        cpu->rm++;\
    }\
    else \
        cpu->pc++;\
    OP(JRNZ);\
}
#define JRNC    NYI("JRNC"); cpu->ni++; cpu->rm = 1;

#define CALLm   OP(CALLm); uint16_t tmp = CPU_RW (cpu->pc); cpu->pc += 2; cpu->sp -= 2; CPU_WW(cpu->sp, cpu->pc); cpu->pc = tmp; cpu->rm = 6;

#define RET     OP(RET);   cpu->pc = CPU_RW (cpu->sp); cpu->sp += 2; cpu->rm = 4;
#define RETI    OP(RETI);  cpu->pc = CPU_RW (cpu->sp); cpu->sp += 2; cpu->ime = 1; cpu->rm = 4;

#define RST(N)  OPN(RST, N); cpu->sp -= 2; mmu_ww (mmu, cpu->sp, cpu->pc); cpu->pc = N; cpu->rm = 3;

/* Rotate and shift instructions */

#define RLA   	OP(RLA);   uint8_t ci = cpu->r[A]; cpu->r[A] = (cpu->r[A] << 1) | (cpu->flags & 0x10 ? 1 : 0); cpu->flags = ((ci >> 7) & 1) * 0x10; cpu->rm = 1;
#define RRA     OP(RRA);   uint8_t ci = cpu->r[A]; cpu->r[A] = (cpu->r[A] << 1) | ((cpu->flags & 0x10 ? 1 : 0) << 7); cpu->flags = (ci & 1) * 0x10; cpu->rm = 1;
#define RLCA    OP(RLCA);  cpu->r[A] = (cpu->r[A] << 1) | (cpu->r[A] >> 7); cpu->flags = (cpu->r[A] & 1) * 0x10; cpu->rm = 1;
#define RRCA    OP(RRCA);  cpu->flags = (cpu->r[A] & 1) * 0x10; cpu->r[A] = (cpu->r[A] >> 1) | (cpu->r[A] << 7); cpu->rm = 1;

/* Misc instructions */

#define PREFIX    OP(PREFIX)   uint8_t cb = CPU_RB (cpu->pc++); cpu_exec_cb (cb);
#define INVALID   OP(INVALID); cpu->invalid = 1; cpu->rm = 1;

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

inline void cpu_init()
{
    strcpy(cpu->reg_names, "ABCDEHL");
    cpu->ni = 0;
    cpu->ime = 0;
    cpu->invalid = 0;
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
                    if (op == 0x28) { JRZm; }
                    if (op == 0x38) { JRCm; }
                break;  
                case 9: /* 16-bit add */
                    if (opHh <= 5) { ADHLrr (r1 - 1, r1); } else { ADHLSP; } break;
                break;
                case 0xA:
                    if (op < 0x2A)  { LDA_X (ADDR_XY(cpu->r[r1 - 1], cpu->r[r1])); /* LDArrm */ }
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
            if (r2 != 255) { LD_X_Y (r1, cpu->r[r2]); } else {  LD_X_Y (r1, HL_ADDR_BYTE); }
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
                break;
                case 5: /* PUSH operations */
                    if (r1 != A) { PUSH (r1, r1 + 1) } else { PUSHF; }
                break;
                case 6:
                    if (op == 0xC6) { ADDm; }
                    if (op == 0xD6) { SUBm; }
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
            }
        break;
        default:
            cpu->rm = 0;
        break;
    }
#endif

    cpu->rt = opTicks[op];
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
/*    cpu->r[A] = 1; cpu->r[B] = 2;

    cpu_print_regs();*/
    cpu_boot_reset();
    cpu_state();

    /*Load next op and execute */
    uint16_t i;   
    for (i = 0; i < 50000; i++)
    {
        cpu->clock_m += cpu->rm;
        cpu->clock_t += (cpu->rm << 2);

        uint8_t op = mmu_rb(mmu, cpu->pc++);
        //LOG_("Test op %02x... ", op);
        /*LOG_("Read op %02x at PC %04x\n", op, cpu->pc);*/

        cpu_exec(op, i+1);
        cpu_state();
    }
    
    /*LOG_("Ran CPU. (%lld clocks)\n", cpu->clock_m);*/
    LOG_("%d%% of 256 instructions done.\n", ((256 - cpu->ni) * 100) / 256);
}
