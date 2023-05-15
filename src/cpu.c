
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
#define DECR_HL       cpu->r[L]--; if (cpu->r[L] == 255) cpu->r[H]--;
#define HL_ADDR_BYTE  mmu_rb (mmu, ADDR_HL);

#ifdef DEBUG_OP_PRINT

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

#endif

/* 8-bit load instructions */

#define LD(X,Y)   OPXY(LD, X, Y); cpu->r[X] = cpu->r[Y]; cpu->rm = 1;        
#define LDrn(X)   OPX(LDrn, X);   cpu->r[X] = CPU_RB (cpu->pc); cpu->pc++; cpu->rm = 2;
#define LDrHL(X)  OPX(LDrHL, X);  cpu->r[X] = HL_ADDR_BYTE; cpu->rm = 2;
#define LDHLr(X)  OPX(LDHLr, X);  CPU_WB (ADDR_HL, cpu->r[X]); cpu->rm = 2; /* 70 ... 77 */
#define LDHLn     OP(LDHLn);      CPU_WB ((cpu->r[H] << 8) + cpu->r[L], CPU_RB (cpu->pc)); cpu->pc++; cpu->rm = 3;
#define LDHL      OP(LDHL);       CPU_WB (ADDR_HL, cpu->pc); cpu->pc++; cpu->rm = 3;

#define LDHLIA    OP(LDHLIA); CPU_WB (ADDR_HL, cpu->r[A]); INCR_HL;  cpu->rm=2; //22
#define LDAHLI    OP(LDAHLI); cpu->r[A] = HL_ADDR_BYTE; INCR_HL; cpu->rm=2; //2A
#define LDHLDA    OP(LDHLDA); CPU_WB (ADDR_HL, cpu->r[A]); DECR_HL;  cpu->rm=2; //32
#define LDAHLD    OP(LDAHLD); cpu->r[A] = HL_ADDR_BYTE; DECR_HL; cpu->rm=2; //3A

#define LDArrm(X,Y)  OPXY(LDArrm, X, Y); cpu->r[A] = CPU_RB (ADDR_XY(X,Y)); cpu->rm = 2; //0A, 1A
#define LDrrmA(X,Y)  OPXY(LDrrmA, X, Y); CPU_WB (ADDR_XY(X,Y), cpu->r[A]); cpu->rm = 2;  //02, 12

/* 16-bit load instructions */

#define LDrr(X,Y) OPXY(LDrr, X, Y); cpu->r[Y] = CPU_RB (cpu->pc); cpu->r[X] = CPU_RB (cpu->pc + 1); cpu->pc += 2; cpu->rm = 3;
#define LDimSP    OP(LDimSP); uint16_t val = CPU_RW (cpu->pc); CPU_WW (val, cpu->sp); cpu->pc += 2; cpu->rm = 5;
#define LDSP      OP(LDSP);   cpu->sp = CPU_RW (cpu->pc); cpu->pc += 2; cpu->rm =3;

#define PUSH(X,Y) OPXY(PUSH, X, Y); cpu->sp--; CPU_WB(cpu->sp, cpu->r[X]); cpu->sp--; CPU_WB(cpu->sp, cpu->r[Y]); cpu->rm = 4;
#define POP(X,Y)  OPXY(POP, X, Y);  cpu->r[Y] = CPU_RB(cpu->sp++); cpu->r[X] = CPU_RB(cpu->sp++); cpu->rm = 3;
#define PUSHF     OP(PUSHF); cpu->sp--; CPU_WB(cpu->sp, cpu->r[A]); cpu->sp--; CPU_WB(cpu->sp, cpu->flags); cpu->rm = 4;
#define POPF      OP(POPF);  cpu->flags = CPU_RB(cpu->sp++); cpu->r[A] = CPU_RB(cpu->sp++); cpu->rm = 3;

/* 8-bit arithmetic/logic instructions */

/* Flag setting helpers */

#define SET_ADD_CARRY(X,Y,T)    cpu->flags = __builtin_add_overflow (X, Y, T) << 4;
#define SET_SUB_CARRY(X,Y,T)    cpu->flags = __builtin_sub_overflow (X, Y, T) << 4;
#define SET_ZERO(X)    if (!X) cpu->flags |= FLAG_Z; else cpu->flags &= ~(FLAG_Z);
#define SET_HALF(X)    if ((X) & 0x10) cpu->flags |= FLAG_H; else cpu->flags &= ~(FLAG_H);
#define SET_HALF_S(X)  if (X) cpu->flags |= FLAG_H; else cpu->flags &= ~(FLAG_H);
#define SET_CARRY(X)   if (X) cpu->flags |= FLAG_C; else cpu->flags &= ~(FLAG_C);

#define SET_ADD_FLAGS(T,X)  cpu->flags = (cpu->r[A] < T) ? FLAG_C : 0; SET_ZERO(cpu->r[A]); SET_HALF(cpu->r[A] ^ X ^ T);
#define SET_SUB_FLAGS(T,X)  cpu->flags = (cpu->r[A] > T) ? FLAG_N | FLAG_C : FLAG_N; SET_ZERO(cpu->r[A]); SET_HALF(cpu->r[A] ^ X ^ T); 

#define ADD(X)   OPX(ADD, X); uint8_t tmp = cpu->r[A]; cpu->r[A] += cpu->r[X]; SET_ADD_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define ADC(X)   OPX(ADC, X); uint8_t tmp = cpu->r[A]; cpu->r[A] += cpu->r[X]; cpu->r[A] += (cpu->flags & FLAG_C) ? 1 : 0; SET_ADD_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define ADHL     OP(ADHL);    uint8_t tmp = cpu->r[A]; cpu->r[A] += hl; SET_ADD_FLAGS(tmp, hl); cpu->rm = 2;
#define ACHL     OP(ACHL);    uint8_t tmp = cpu->r[A]; cpu->r[A] += hl; cpu->r[A] += (cpu->flags & FLAG_C) ? 1 : 0; SET_SUB_FLAGS(tmp, hl); cpu->rm = 2; 
#define ADDm     OP(ADDm);    uint8_t m = CPU_RB(cpu->pc++); uint8_t tmp = cpu->r[A]; cpu->r[A] += m; SET_ADD_FLAGS(tmp, m); cpu->rm = 2;

#define SUB(X)   OPX(SUB, X); uint8_t tmp = cpu->r[A]; cpu->r[A] -= cpu->r[X]; SET_SUB_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define SBC(X)   OPX(SBC, X); uint8_t tmp = cpu->r[A]; cpu->r[A] -= cpu->r[X]; cpu->r[A] -= (cpu->flags & FLAG_C) ? 1 : 0; SET_SUB_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define SBHL     OP(SBHL);    uint8_t tmp = cpu->r[A]; cpu->r[A] -= hl; SET_SUB_FLAGS(tmp, hl); cpu->rm = 2;
#define SCHL     OP(SCHL);    uint8_t tmp = cpu->r[A]; cpu->r[A] -= hl; cpu->r[A] -= (cpu->flags & FLAG_C) ? 1 : 0; SET_SUB_FLAGS(tmp, hl); cpu->rm = 2;
#define SUBm     OP(SUBm);    uint8_t m = CPU_RB(cpu->pc++); uint8_t tmp = cpu->r[A]; cpu->r[A] -= m; SET_SUB_FLAGS(tmp, m); cpu->rm = 2;

#define AND(X)   OPX(AND, X)  cpu->r[A] &= cpu->r[X]; cpu->flags = cpu->r[A] ? 0 : FLAG_Z; cpu->flags |= FLAG_H; cpu->rm = 1;
#define XOR(X)   OPX(XOR, X)  cpu->r[A] ^= cpu->r[X]; cpu->flags = cpu->r[A] ? 0 : FLAG_Z; cpu->rm = 1;
#define ANHL     OP(ANHL);    cpu->r[A] &= hl; cpu->flags = cpu->r[A] ? 0 : FLAG_Z; cpu->flags |= FLAG_H; cpu->rm = 2;
#define XRHL     OP(XRHL);    cpu->r[A] ^= hl; cpu->flags = cpu->r[A] ? 0 : FLAG_Z; cpu->rm = 2;
#define OR(X)    OPX(OR, X);  cpu->r[A] |= cpu->r[X]; cpu->flags = cpu->r[A] ? 0 : FLAG_Z; cpu->rm = 1;
#define ORHL     OP(ORHL)     cpu->r[A] |= hl; cpu->flags = cpu->r[A] ? 0 : FLAG_Z; cpu->rm = 2;
#define CP(X)    OPX(CP, X);  uint8_t tmp = cpu->r[A]; tmp -= cpu->r[X]; cpu->flags = (tmp < 0) ? 0x50 : FLAG_N; SET_ZERO(tmp); SET_HALF(cpu->r[A] ^ cpu->r[X] ^ tmp); cpu->rm = 1;
#define CPHL     OP(CPHL);    uint8_t tmp = cpu->r[A]; tmp -= hl; cpu->flags = (tmp < 0) ? 0x50 : FLAG_N; SET_ZERO(tmp); SET_HALF(cpu->r[A] ^ hl ^ tmp); cpu->rm = 2;

#define INC(X)   OPX(INC, X); cpu->r[X]++; cpu->flags = cpu->r[X] ? 0 : 0x80; cpu->rm = 1;
#define DEC(X)   OPX(DEC, X); cpu->r[X]--; cpu->flags = cpu->r[X] ? 0 : 0x80; cpu->rm = 1;
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

#define CCF     OP(CCF);  uint8_t ci = (cpu->flags & 0x10) ^ 0x10; cpu->flags &= 0x80; cpu->flags |= ci; cpu->rm = 1;
#define SCF     OP(SCF);  cpu->flags &= 0x80; cpu->flags |= 0x10; cpu->rm = 1;
#define HALT    OP(HALT); cpu->halt = 1; cpu->rm = 1;
#define STOP    OP(STOP); cpu->stop = 1; cpu->rm = 1;
#define NOP     OP(NOP);  cpu->rm = 1;
#define DI      OP(DI);   cpu->ime = 0; cpu->rm = 1;
#define EI      OP(EI);   cpu->ime = 1; cpu->rm = 1;

/* Jump and call instructions */

#define JRNC    NYI("JRNC"); cpu->ni++; cpu->rm = 1;

#define RST(N)  OPN(RST, N); cpu->sp -= 2; mmu_ww (mmu, cpu->sp, cpu->pc); cpu->pc = N; cpu->rm = 3;

/* Rotate and shift instructions */

#define RLA   	OP(RLA);   uint8_t ci = cpu->r[A]; cpu->r[A] = (cpu->r[A] << 1) | (cpu->flags & 0x10 ? 1 : 0); cpu->flags = ((ci >> 7) & 1) * 0x10; cpu->rm = 1;
#define RRA     OP(RRA);   uint8_t ci = cpu->r[A]; cpu->r[A] = (cpu->r[A] << 1) | ((cpu->flags & 0x10 ? 1 : 0) << 7); cpu->flags = (ci & 1) * 0x10; cpu->rm = 1;
#define RLCA    OP(RLCA);  cpu->r[A] = (cpu->r[A] << 1) | (cpu->r[A] >> 7); cpu->flags = (cpu->r[A] & 1) * 0x10; cpu->rm = 1;
#define RRCA    OP(RRCA);  cpu->flags = (cpu->r[A] & 1) * 0x10; cpu->r[A] = (cpu->r[A] >> 1) | (cpu->r[A] << 7); cpu->rm = 1;

/* CPU related functions */

inline void cpu_init()
{
    strcpy(cpu->reg_names, "ABCDEHL");
    cpu->ni = 0;
    cpu->ime = 0;
}

void cpu_print_regs()
{
    int i;
    for (i = A; i < L; i++)
        LOG_("%d ", cpu->r[i]);

    /*LOG_(" . %llu\n", cpu->clock_m);*/
}

void cpu_state()
{
    const uint16_t pc = cpu->pc;

    LOG_("A:%02x F:%02x B:%02x C:%02x D:%02x E:%02x H:%02x L:%02x SP:%04x PC:%04x PCMEM:%02x,%02x,%02x,%02x\n",
        cpu->r[A], cpu->flags, cpu->r[B], cpu->r[C], cpu->r[D], cpu->r[E], cpu->r[H], cpu->r[L], 
        cpu->sp, cpu->pc, 
        CPU_RB (pc), CPU_RB (pc+1), CPU_RB (pc+2), CPU_RB (pc+3)
    );
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
}

#ifdef EXEC_TEST

void cpu_exec(uint8_t const op) { }

#else

void cpu_exec(uint8_t const op)
{
    cpu->rm = 0;

    /* LOG_("Running op %02x:\n", op); */
    uint8_t opL = op & 0xf;
    /* uint8_t opH  = op >> 4; */
    uint8_t opHh = op >> 3; /* Octal divisions */
    uint8_t arg1 = 0, arg2 = 0;
    uint16_t hl = ADDR_HL;

    /* Default values for opcode arguments (can be overridden for other opcodes) */
    arg2 = ((opL & 7) == 7) ? A : ((opL & 7) < 6) ? B + (opL & 7) : 255;

    switch (opHh)
    {
        case 0 ... 5: case 0x7:

            arg1 = (opHh == 0x7) ? A : B + opHh; 
            switch (opL)
            {
                case 0: /* JMP and CPU operations */
                    if (op == 0x0)  { NOP; }
                    if (op == 0x10) { STOP; }
                break;
                case 1: /* 16-bit load, LDrr */
                    LDrr (arg1, arg1 + 1); break;
                case 2: /* 8-bit load, LDrrmA or LDHLIA */
                    if (op == 0x02 || op == 0x12) { LDrrmA (arg1, arg1 + 1); }
                    if (op == 0x22) { LDHLIA; }
                break;
                case 3: /* 16-bit increment */
                    INCrr (arg1, arg1 + 1); break;
                case 4: 
                case 0xC: /* 8-bit increment */
                    INC (arg1); break;
                case 5:
                case 0xD: /* 8-bit decrement */
                    DEC (arg1); break;
                case 6:
                case 0xE: /* Load 8-bit immediate */
                    LDrn (arg1); break;
                case 7: /* Bit and flag operations */
                    if (op == 0x07) { RLCA; }
                    if (op == 0x17) { RLA; }
                break;
                case 8: /* JMP and load operations */
                    if (op == 0x08) { LDimSP; }
                break;  
                case 9: /* 16-bit add */
                    if (opHh <= 5) { ADHLrr (arg1 - 1, arg1); } else { ADHLSP; } break;
                break;
                case 0xA:
                    if (op < 0x2A)  { LDArrm (arg1 - 1, arg1); }
                    if (op == 0x2A) { LDAHLI }
                    if (op == 0x3A) { LDAHLD }
                break;
                case 0xB: /* 16-bit decrement */
                    if (opHh <= 5) { DECrr (arg1 - 1, arg1); } else { DECSP; } break;
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
            if (op == 0x36) { LDHLn; }
            if (op == 0x37) { SCF; }
        break;
        case 8 ... 0xD: case 0xF:
            /* 8-bit load, LD, LDrHL */
            arg1 = (opHh == 0xF) ? A : B + (opHh - 8);
            if (arg2 != 255) { LD (arg1, arg2); } else { LDrHL (arg1); }
        break;
        case 0xE:
            /* 8-bit load, LDHLr or HALT */
            if (arg2 != 255) { LDHLr (arg2); } else { HALT; }
        break;
        /* 8-bit arithmetic */
        case 0x10 ... 0x17:
            if (arg2 != 255)
            {
                switch (opHh - 0x10)
                {
                    case 0: { ADD (arg2); } break;
                    case 1: { ADC (arg2); } break;
                    case 2: { SUB (arg2); } break;
                    case 3: { SBC (arg2); } break;
                    case 4: { AND (arg2); } break;
                    case 5: { XOR (arg2); } break;
                    case 6: { OR (arg2); } break;
                    case 7: { CP (arg2); } break;
                }
            }
            else {
                uint8_t hl = HL_ADDR_BYTE;

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
            arg1 = (opHh - 0x18 >= 0x6) ? A : B + (opHh - 0x18);

            if (opL == 1) { if (arg1 != A) { POP  (arg1, arg1 + 1) } else { POPF; }}
            if (opL == 5) { if (arg1 != A) { PUSH (arg1, arg1 + 1) } else { PUSHF; }}

            if (opL == 6) 
            {
                if (op == 0xC6) { ADDm; }
                if (op == 0xD6) { SUBm; }
            }
            if (opL == 0x07 || opL == 0x0f)
            {
                uint16_t n = (opHh - 0x18) * 0x08; RST(n);
            }
            if (op == 0xE8) { ADDSPm; }
            if (op == 0xF3) { DI; }
            if (op == 0xFB) { EI; }
        break;
        default:
            cpu->rm = 0;
        break;
    }

    if (cpu->rm == 0) { LOG_("*N/A (%02x)*\n", op); cpu->ni++; } else { LOG_("\n"); }
}

#endif

void cpu_clock()
{   
/*    cpu->r[A] = 1; cpu->r[B] = 2;

    cpu_print_regs();*/
    cpu_boot_reset();
    cpu_state();

    /*Load next op and execute */
    uint16_t i;   
    for (i = 0; i < 256; i++)
    {
        cpu->clock_m += cpu->rm;
        cpu->clock_t += (cpu->rm << 2);

        uint8_t op = i;// mmu_rb(mmu, cpu->pc++);
        LOG_("Test op %02x... ", op);
        /*LOG_("Read op %02x at PC %04x\n", op, cpu->pc);*/

        cpu_exec(op);
        //cpu_state();
    }
    
    /*LOG_("Ran CPU. (%lld clocks)\n", cpu->clock_m);*/
    LOG_("%d%% of 256 instructions done.\n", ((256 - cpu->ni) * 100) / 256);
}
