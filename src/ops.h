#define DEBUG_OP_PRINT

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

#define KEEP_ZERO       cpu->flags &= FLAG_Z;
#define KEEP_CARRY      cpu->flags &= FLAG_C;
#define KEEP_ZERO_CARRY cpu->flags &= (FLAG_Z | FLAG_C);

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

/* Jump function templates */
#define JR_X(X) if (X) { int8_t ci = (int8_t) CPU_RB (cpu->pc++); cpu->pc += ci; cpu->rm++; } else { cpu->pc++; }

#define JPNN    OP(JPNN); cpu->pc = CPU_RW(cpu->pc); cpu->rm = 3;
#define JPHL    OP(JPHL); cpu->pc = ADDR_HL; cpu->rm = 1;
#define JRm     OP(JRm);  cpu->pc += (int8_t) CPU_RB (cpu->pc); cpu->pc++; cpu->rm = 3;

#define JRNZ    OP(JRNZ); JR_X (!(cpu->flags & FLAG_Z));
#define JRZ     OP(JRZ);  JR_X (cpu->flags & FLAG_Z);

#define JRNC {\
    if((cpu->flags & 0x10) == 0) {\
        int8_t ci = (int8_t) CPU_RB (cpu->pc++);\
        cpu->pc += ci;\
        cpu->rm++;\
    }\
    else \
        cpu->pc++;\
    OP(JRNC);\
}

#define JRC {\
    if(cpu->flags & 0x10) {\
        int8_t ci = (int8_t) CPU_RB (cpu->pc++);\
        cpu->pc += ci;\
        cpu->rm++;\
    }\
    else \
        cpu->pc++;\
    OP(JRC);\
}

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