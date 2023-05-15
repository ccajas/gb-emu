
#include <string.h>
#include "gb.h"

GameBoy GB;

CPU * cpu = &GB.cpu;
MMU * mmu = &GB.mmu;

#define CPU_RB(A)     mmu_rb(mmu, A)
#define CPU_WB(A,X)   mmu_wb(mmu, A, X)
#define CPU_RW(A)     mmu_rw(mmu, A)
#define CPU_WW(A,X)   mmu_ww(mmu, A, X)

/* Instruction helpers */

#define ADDR_HL       ((cpu->r[H] << 8) + cpu->r[L])
#define INCR_HL       cpu->r[L]++; if (!cpu->r[L]) cpu->r[H]++
#define DECR_HL       cpu->r[L]--; if (cpu->r[L] == 255) cpu->r[H]--;
#define HL_ADDR_BYTE  mmu_rb (mmu, ADDR_HL);

#define NYI(S)           LOG_("Instruction not implemented! (%s)\n", S); cpu->ni++;
#define OP(name)         LOG_("%s\n", #name);
#define OPX(name, X)     LOG_("%s (%c)\n", #name, cpu->reg_names[X]);
#define OPXY(name, X, Y) LOG_("%s (%c,%c)\n", #name, cpu->reg_names[X], cpu->reg_names[Y]);

/* 8-bit load instructions */

#define LD(X,Y)   OPXY(LD, X, Y); cpu->r[X] = cpu->r[Y]; cpu->rm = 1;        
#define LDrn(X)   OPX(LDrn, X);   cpu->r[X] = CPU_RB (cpu->pc); cpu->pc++; cpu->rm = 2;
#define LDrHL(X)  OPX(LDrHL, X);  cpu->r[X] = HL_ADDR_BYTE; cpu->rm = 2;
#define LDHLr(X)  OPX(LDHLr, X);  CPU_WB (ADDR_HL, cpu->r[X]); cpu->rm = 2;
#define LDHLn()   OP(LDHLn);      CPU_WB ((cpu->r[H] << 8) + cpu->r[L], CPU_RB (cpu->pc)); cpu->pc++; cpu->rm = 3;
#define LDHL()    OP(LDHL);       CPU_WB (ADDR_HL, cpu->pc); cpu->pc++; cpu->rm = 3;

#define LDHLIA()  OP(LDHLIA); CPU_WB (ADDR_HL, cpu->r[A]); INCR_HL;  cpu->rm=2; //22
#define LDAHLI()  OP(LDAHLI); cpu->r[A] = HL_ADDR_BYTE; INCR_HL; cpu->rm=2; //2A
#define LDHLDA()  OP(LDHLDA); CPU_WB (ADDR_HL, cpu->r[A]); DECR_HL;  cpu->rm=2; //32
#define LDAHLD()  OP(LDAHLD); cpu->r[A] = HL_ADDR_BYTE; DECR_HL; cpu->rm=2; //3A

#define LDrrmA(X,Y)  OPXY(LDrrmA, X, Y); CPU_WB((cpu->r[X] << 8) + cpu->r[Y], cpu->r[A]); cpu->rm = 2;

/* 16-bit load instructions */

#define LDrr(X,Y) OPXY(LDrr, X, Y); cpu->r[Y] = CPU_RB (cpu->pc); cpu->r[X] = CPU_RB (cpu->pc + 1); cpu->pc += 2; cpu->rm =3;
#define LDSP()    OP(LDSP);         cpu->sp = CPU_RW (cpu->pc); cpu->pc += 2; cpu->rm =3;

/* 8-bit arithmetic/logic instructions */

#define SET_ADD_CARRY(X,Y,T)    cpu->flags = __builtin_add_overflow (X, Y, T) << 4;
#define SET_SUB_CARRY(X,Y,T)    cpu->flags = __builtin_sub_overflow (X, Y, T) << 4;

#define SET_ADD_FLAGS(T,X)  cpu->flags = (cpu->r[A] < T) ? 0x10 : 0;    if (!cpu->r[A]) cpu->flags |=0x80; if ((cpu->r[A] ^ X ^ T) & 0x10) cpu->flags |=0x20;
#define SET_SUB_FLAGS(T,X)  cpu->flags = (cpu->r[A] > T) ? 0x50 : 0x40; if (!cpu->r[A]) cpu->flags |=0x80; if ((cpu->r[A] ^ X ^ T) & 0x10) cpu->flags |=0x20; 
/*
#define ADD(X)   OPX(ADD, X); uint8_t total; SET_ADD_CARRY (cpu->r[A], cpu->r[X], &total); SET_ADD_FLAGS(total, cpu->r[X]); cpu->rm = 1;
#define ADC(X)   OPX(ADC, X); uint8_t total; cpu->r[A] += (cpu->flags & 0x10) ? 1 : 0; SET_ADD_CARRY (cpu->r[A], cpu->r[X], &total); SET_ADD_FLAGS(total, cpu->r[X]); cpu->rm = 1;
#define ADHL()   OP(ADHL);    uint8_t total = cpu->r[A]; uint8_t hl = HL_ADDR_BYTE; SET_ADD_CARRY(cpu->r[A], hl, &total); SET_ADD_FLAGS(total, hl); cpu->rm = 2;
#define ACHL()   OP(ACHL);    uint8_t total; cpu->r[A] += (cpu->flags & 0x10) ? 1 : 0; SET_ADD_CARRY (cpu->r[A], ADDR_HL, &total); SET_ADD_FLAGS(total, ADDR_HL); cpu->rm = 2;
*/
#define ADD(X)   OPX(ADD, X); uint8_t tmp = cpu->r[A]; cpu->r[A] += cpu->r[X]; SET_ADD_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define ADC(X)   OPX(ADC, X); uint8_t tmp = cpu->r[A]; cpu->r[A] += cpu->r[X]; cpu->r[A] += (cpu->flags & 0x10) ? 1 : 0; SET_ADD_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define ADHL()   OP(ADHL);    uint8_t tmp = cpu->r[A]; uint8_t hl = HL_ADDR_BYTE; cpu->r[A] -= hl; SET_ADD_FLAGS(tmp, hl); cpu->rm = 2;
#define ACHL()   OP(ACHL);    uint8_t tmp = cpu->r[A]; uint8_t hl = HL_ADDR_BYTE; cpu->r[A] -= hl; cpu->r[A] -= (cpu->flags & 0x10) ? 1 : 0; SET_SUB_FLAGS(tmp, hl); cpu->rm = 2; 

#define SUB(X)   OPX(SUB, X); uint8_t tmp = cpu->r[A]; cpu->r[A] -= cpu->r[X]; SET_SUB_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define SBC(X)   OPX(SBC, X); uint8_t tmp = cpu->r[A]; cpu->r[A] -= cpu->r[X]; cpu->r[A] -= (cpu->flags & 0x10) ? 1 : 0; SET_SUB_FLAGS(tmp, cpu->r[X]); cpu->rm = 1;
#define SBHL()   OP(SBHL);    uint8_t tmp = cpu->r[A]; uint8_t hl = HL_ADDR_BYTE; cpu->r[A] -= hl; SET_SUB_FLAGS(tmp, hl); cpu->rm = 2;
#define SCHL()   OP(SCHL);    uint8_t tmp = cpu->r[A]; uint8_t hl = HL_ADDR_BYTE; cpu->r[A] -= hl; cpu->r[A] -= (cpu->flags & 0x10) ? 1 : 0; SET_SUB_FLAGS(tmp, hl); cpu->rm = 2; 
#define AND(X)   NYI("AND"); cpu->rm = 1;
#define XOR(X)   NYI("XOR"); cpu->rm = 1;
#define ANHL(X)  NYI("ANHL"); cpu->rm = 1;
#define XRHL(X)  NYI("XRHL"); cpu->rm = 1;
#define OR(X)    NYI("OR"); cpu->rm = 1;
#define CP(X)    NYI("CP"); cpu->rm = 1;
#define ORHL(X)  NYI("ORHL"); cpu->rm = 1;
#define CPHL(X)  NYI("CPHL"); cpu->rm = 1;

#define INC(X)  OPX(INC, X); cpu->r[X]++; cpu->flags = cpu->r[X] ? 0 : 0x80; cpu->rm = 1;
#define DEC(X)  OPX(DEC, X); cpu->r[X]--; cpu->flags = cpu->r[X] ? 0 : 0x80; cpu->rm = 1;

/* 16-bit arithmetic instructions */

#define ADHLrr(X,Y) NYI("ADHLrr");     cpu->rm = 1;
#define INCrr(X,Y)  OPXY(INCrr, X, Y); cpu->r[Y]++; if (!cpu->r[Y]) cpu->r[X]++; cpu->rm = 1;
#define INCSP()     OP(INCSP);         cpu->sp++; cpu->rm = 1;
#define DECrr(X,Y)  OPXY(DECrr, X, Y); cpu->r[Y]--; if (cpu->r[Y] == 0xff) cpu->r[X]--; cpu->rm = 1;
#define DECSP()     OP(DECSP);         cpu->sp++; cpu->rm = 1;

/* CPU control instructions */

#define CCF()   OP(CCF);  uint8_t ci = cpu->flags & 0x10 ? 0 : 0x10; cpu->flags &= 0xEF; cpu->flags += ci; cpu->rm = 1;
#define SCF()   OP(SCF);  cpu->flags |= 0x10; cpu->rm = 1;
#define HALT()  OP(HALT); cpu->halt = 1; cpu->rm = 1;
#define STOP()  OP(STOP); cpu->stop = 1; cpu->rm = 1;
#define NOP()   OP(NOP);  cpu->rm = 1

/* Jump instructions */

#define JRNC()  NYI("JRNC"); cpu->rm = 1;

/* Rotate and shift instructions */

#define RLA   	OP(RLA);   uint8_t ci = cpu->r[A]; cpu->r[A] = cpu->r[A] << 1 | (cpu->flags & 0x10); cpu->flags = 0; cpu->flags = ((ci >> 7) & 1) * 0x10;
#define RLCA    OP(RLCA);  cpu->r[A] = ( cpu->r[A] << 1) | ( cpu->r[A] >> 7); cpu->flags = 0; cpu->flags = (cpu->r[A] & 1) * 0x10;
#define RLAa    OP(RLAa);  uint8_t ci = cpu->flags & 0x10 ? 1 : 0; uint8_t co=cpu->r[A] & 0x80 ? 0x10 : 0; cpu->r[A]=(cpu->r[A]<<1)+ci; (cpu->flags &= 0xEF) + co; cpu->rm = 1;
#define RLCAa   OP(RLCAa); uint8_t ci = cpu->r[A]  & 0x80 ? 1 : 0; uint8_t co=cpu->r[A] & 0x80 ? 0x10 : 0; cpu->r[A]=(cpu->r[A]<<1)+ci; (cpu->flags &= 0xEF) + co; cpu->rm = 1;

/* CPU related functions */

inline void cpu_init()
{
    strcpy(cpu->reg_names, "ABCDEHL");
    cpu->ni = 0;
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
    uint8_t opHh = op >> 3;
    uint8_t arg1 = 0, arg2 = 0;

    /* Default values for opcode arguments (can be overridden for other opcodes) */
    arg2 = ((opL & 7) == 7) ? A : ((opL & 7) < 6) ? B + (opL & 7) : 255;

    switch (opHh)
    {
        case 0 ... 5: case 0x7:

            arg1 = (opHh == 0x7) ? A : B + opHh; 
            switch (opL)
            {
                case 0: /* JMP and CPU operations */
                    if (op == 0x0)  { NOP(); }
                    if (op == 0x10) { STOP(); }
                break;
                case 1: /* 16-bit load, LDrr */
                    LDrr (arg1, arg1 + 1); break;
                case 2: /* 8-bit load, LDrrmA or LDHLIA */
                    if (op == 0x02 || op == 0x12) { LDrrmA (arg1, arg1 + 1); }
                    if (op == 0x22) { LDHLIA (); }
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
                break;
                case 8: /* JMP and load operations */
                break;  
                case 9: /* 16-bit add */
                break;
                case 0xA:
                break;
                case 0xB: /* 16-bit decrement */
                    if (opHh <= 5) { DECrr (arg1 - 1, arg1); } break;
                case 0xF: /* Other CPU/etc ops */
                    if (op == 0x3F) { CCF(); } 
                break;
            }
        break;
        case 6:
            if (op == 0x30) { JRNC (); }
            if (op == 0x31) { LDSP (); }
            if (op == 0x32) { LDHLDA (); }
            if (op == 0x33) { INCSP (); }
            if (op == 0x34) { }
            if (op == 0x35) { }
            if (op == 0x36) { LDHLn (); }
            if (op == 0x37) { SCF (); }
        break;
        case 8 ... 0xD: case 0xF:
            /* 8-bit load, LD, LDrHL */
            arg1 = (opHh == 0xF) ? A : B + (opHh - 8);
            if (arg2 != 255) { LD (arg1, arg2); } else { LDrHL (arg1); }
        break;
        case 0xE:
            /* 8-bit load, LDHLr or HALT */
            if (arg2 != 255) { LDHLr (arg2); } else { HALT(); }
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
                    case 3: SBC (arg2); break;
                    case 4: AND (arg2); break;
                    case 5: XOR (arg2); break;
                    case 6: OR (arg2); break;
                    case 7: CP (arg2); break;
                }
            }
            else {
                switch (opHh - 0x10)
                {
                    case 0: { ADHL (); } break;
                    case 1: { ACHL (); } break;
                    case 2: { SBHL (); } break;
                    case 3: SCHL (); break;
                    case 4: ANHL (); break;
                    case 5: XRHL (); break;
                    case 6: ORHL (); break;
                    case 7: CPHL (); break;
                }
            }
        break;
        case 0x18 ... 0x1F:
            cpu->rm = 0;
        break;
        default:
            cpu->rm = 0;
        break;
    }

    if (cpu->rm == 0) { LOG_("*N/A (%02x)*\n", op); cpu->ni++; }
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
