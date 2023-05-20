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

/** 8-bit load instructions **/   

/* Load function templates */
/* Used for LDrr, LDrn, LDrHL */
#define LD_X_Y(X,Y) OPXY(LD_X_Y, X, Y); cpu->r[X] = Y;

/* Used for LDHLr, LDHLn */
#define LDHL_X(X) OPX(LDHL_X, X); CPU_WB (ADDR_HL, X);

/* Used for LDArrm, LDAIOm, LDAIOC */
#define LDA_X(X)  OPX(LDA_X, X);  cpu->r[A] = CPU_RB (X);

#define LDrHL(X)     OPX(LDrHL, X);      cpu->r[X] = CPU_RB (ADDR_HL);
#define LDAmm        OP(LDAmm);          cpu->r[A] = CPU_RB (CPU_RW (cpu->pc)); cpu->pc += 2;
#define LDArrm(X,Y)  OPXY(LDArrm, X, Y); cpu->r[A] = CPU_RB (ADDR_XY(X,Y));
#define LDrrmA(X,Y)  OPXY(LDrrmA, X, Y); CPU_WB (ADDR_XY (X,Y), cpu->r[A]);

#define LDmmA     OP(LDmmA);  CPU_WB (CPU_RW (cpu->pc), cpu->r[A]);  cpu->pc += 2;
#define LDIOmA    OP(LDIOmA); CPU_WB (0xFF00 + CPU_RB (cpu->pc++), cpu->r[A]);

#define LDHLIA    OP(LDHLIA); CPU_WB (ADDR_HL, cpu->r[A]); INCR_HL;
#define LDHLDA    OP(LDHLDA); CPU_WB (ADDR_HL, cpu->r[A]); DECR_HL;

#define LDAHLI    OP(LDAHLI); cpu->r[A] = HL_ADDR_BYTE;    INCR_HL;
#define LDAHLD    OP(LDAHLD); cpu->r[A] = HL_ADDR_BYTE;    DECR_HL;

/** 16-bit load instructions **/

#define LDrr(X,Y) OPXY(LDrr, X, Y); cpu->r[Y] = CPU_RB (cpu->pc); cpu->r[X] = CPU_RB (cpu->pc + 1); cpu->pc += 2;
#define LDimSP    OP(LDimSP); uint16_t val = CPU_RW (cpu->pc); CPU_WW (val, cpu->sp); cpu->pc += 2;
#define LDSP      OP(LDSP);   cpu->sp = CPU_RW (cpu->pc); cpu->pc += 2;

#define PUSH(X,Y) OPXY(PUSH, X, Y); cpu->sp--;   CPU_WB(cpu->sp, cpu->r[X]); cpu->sp--; CPU_WB(cpu->sp, cpu->r[Y]);
#define PUSHF     OP(PUSHF);        cpu->sp--;   CPU_WB(cpu->sp, cpu->r[A]); cpu->sp--; CPU_WB(cpu->sp, cpu->flags);
#define POP(X,Y)  OPXY(POP, X, Y);  cpu->r[Y]  = CPU_RB(cpu->sp++); cpu->r[X] = CPU_RB(cpu->sp++);
#define POPF      OP(POPF);         cpu->flags = CPU_RB(cpu->sp++); cpu->r[A] = CPU_RB(cpu->sp++);

/* Flag setting helpers */

#define SET_ZERO(X)    if (!X) cpu->flags |= FLAG_Z; else cpu->flags &= ~(FLAG_Z);
#define SET_HALF(X)    if ((X) & 0x10) cpu->flags |= FLAG_H; else cpu->flags &= ~(FLAG_H);
#define SET_HALF_S(X)  if (X) cpu->flags |= FLAG_H; else cpu->flags &= ~(FLAG_H);
#define SET_CARRY(X)   if (X) cpu->flags |= FLAG_C; else cpu->flags &= ~(FLAG_C);

#define KEEP_ZERO       cpu->flags &= FLAG_Z;
#define KEEP_CARRY      cpu->flags &= FLAG_C;
#define KEEP_ZERO_CARRY cpu->flags &= (FLAG_Z | FLAG_C);

#define SET_ADD_FLAGS(T,X)  cpu->flags = (cpu->r[A] < T) ? FLAG_C : 0; SET_ZERO(cpu->r[A]); SET_HALF(cpu->r[A] ^ X ^ T);
#define SET_SUB_FLAGS(T,X)  cpu->flags = (cpu->r[A] > T) ? FLAG_N | FLAG_C : FLAG_N; SET_ZERO(cpu->r[A]); SET_HALF(cpu->r[A] ^ X ^ T); 

/** 8-bit arithmetic/logic instructions **/

/* Add function templates */
#define ADD_A_X(X)  cpu->r[A] += X; SET_ADD_FLAGS(tmp, X);
#define ADD_AC_X(X) cpu->r[A] += X; cpu->r[A] += (cpu->flags & FLAG_C) ? 1 : 0; SET_ADD_FLAGS(tmp, X);
#define SUB_A_X(X)  cpu->r[A] -= X; SET_SUB_FLAGS(tmp, X);
#define SUB_AC_X(X) cpu->r[A] -= X; cpu->r[A] -= (cpu->flags & FLAG_C) ? 1 : 0; SET_SUB_FLAGS(tmp, X); 

/* Add and subtract */
#define ADD(X)   OPX(ADD, X); ADD_A_X(cpu->r[X]);
#define ADHL     OP(ADHL);    ADD_A_X(hl);
#define ADC(X)   OPX(ADC, X); ADD_AC_X(cpu->r[X]);
#define ACHL     OP(ACHL);    ADD_AC_X(hl);

#define SUB(X)   OPX(SUB, X); SUB_A_X(cpu->r[X]);
#define SBHL     OP(SBHL);    SUB_A_X(hl);
#define SBC(X)   OPX(SBC, X); SUB_AC_X(cpu->r[X]); 
#define SCHL     OP(SCHL);    SUB_AC_X(hl);

/* Bitwise logic */
#define AND(X)   OPX(AND, X)  cpu->r[A] &= cpu->r[X]; cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->flags |= FLAG_H; 
#define ANHL     OP(ANHL);    cpu->r[A] &= hl;        cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->flags |= FLAG_H;
#define XOR(X)   OPX(XOR, X)  cpu->r[A] ^= cpu->r[X]; cpu->flags = 0; SET_ZERO(cpu->r[A]);
#define XRHL     OP(XRHL);    cpu->r[A] ^= hl;        cpu->flags = 0; SET_ZERO(cpu->r[A]);
#define OR(X)    OPX(OR, X);  cpu->r[A] |= cpu->r[X]; cpu->flags = 0; SET_ZERO(cpu->r[A]);  
#define ORHL     OP(ORHL)     cpu->r[A] |= hl;        cpu->flags = 0; SET_ZERO(cpu->r[A]);

#define ADDm     OP(ADDm);    uint8_t m = CPU_RB(cpu->pc++); uint8_t tmp = cpu->r[A]; ADD_A_X(m);
#define SUBm     OP(SUBm);    uint8_t m = CPU_RB(cpu->pc++); uint8_t tmp = cpu->r[A]; SUB_A_X(m);
#define ANDm     OP(ANDm);    cpu->r[A] &= CPU_RB(cpu->pc++); cpu->flags = 0; SET_ZERO(cpu->r[A]); cpu->flags |= FLAG_H;
#define XORm     OP(XORm);    cpu->r[A] ^= CPU_RB(cpu->pc++); cpu->flags = 0; SET_ZERO(cpu->r[A]);
#define ORm      OP(ORm);     cpu->r[A] |= CPU_RB(cpu->pc++); cpu->flags = 0; SET_ZERO(cpu->r[A]);

#define CP(X)    OPX(CP, X);  tmp -= cpu->r[X]; cpu->flags = (tmp < 0) ? 0x50 : FLAG_N; SET_ZERO(tmp); SET_HALF(cpu->r[A] ^ cpu->r[X] ^ tmp);
#define CPHL     OP(CPHL);    tmp -= hl; cpu->flags = (tmp < 0) ? 0x50 : FLAG_N; SET_ZERO(tmp); SET_HALF(cpu->r[A] ^ hl ^ tmp);
#define CPm      OP(CPm);     uint8_t tmp = cpu->r[A]; uint8_t m = CPU_RB(cpu->pc++); tmp -= m; cpu->flags = (tmp < 0) ? 0x50 : FLAG_N; SET_ZERO(tmp); SET_HALF(cpu->r[A] ^ tmp ^ m);

#define INC(X)   OPX(INC, X); cpu->r[X]++; KEEP_CARRY; cpu->flags |= (cpu->r[X] & 0xF) ? 0 : 0x20; cpu->flags |= cpu->r[X] ? 0 : 0x80; 
#define DEC(X)   OPX(DEC, X); cpu->r[X]--; KEEP_CARRY; cpu->flags |= ((cpu->r[X] & 0xF) == 0xF) ? 0x20 : 0; cpu->flags |= cpu->r[X] ? 0 : 0x80; cpu->flags |= 0x40; 
#define INCHL    OP(INCHL);   uint8_t tmp = CPU_RB(ADDR_HL) + 1; CPU_WB(ADDR_HL, tmp); SET_ZERO(tmp); SET_HALF_S((tmp & 0x0F) == 0); cpu->flags &= ~(0x40);
#define DECHL    OP(DECHL);   uint8_t tmp = CPU_RB(ADDR_HL) - 1; CPU_WB(ADDR_HL, tmp); SET_ZERO(tmp); SET_HALF_S((tmp & 0x0F) == 0xF); cpu->flags |= 0x40;
#define CPL      OP(CPL);     cpu->r[A] ^= 0xFF; cpu->flags |= 0x60; 

/** 16-bit arithmetic instructions **/
    
#define ADHLrr(X,Y) OPXY(ADHLrr, X, Y); uint16_t tmp = hl; hl += ADDR_XY(X,Y); if (hl < tmp) cpu->flags |= FLAG_C; else cpu->flags &= 0xEF; cpu->r[H] = (hl >> 8); cpu->r[L] = hl & 0xFF;
#define ADHLSP      OP(ADHLSP);         uint16_t tmp = hl; hl += cpu->sp;      if (hl < tmp) cpu->flags |= FLAG_C; else cpu->flags &= 0xEF; cpu->r[H] = (hl >> 8); cpu->r[L] = hl & 0xFF;
#define ADDSPm      OP(ADDSPm);         int8_t i = (int8_t) CPU_RB(cpu->pc++); cpu->flags = 0; SET_HALF_S ((cpu->sp & 0xF) + (i & 0xF) > 0xF); SET_CARRY ((cpu->sp & 0xFF) + (i & 0xFF) > 0xFF); cpu->sp += i;
#define INCrr(X,Y)  OPXY(INCrr, X, Y);  cpu->r[Y]++; if (!cpu->r[Y]) cpu->r[X]++;
#define INCSP       OP(INCSP);          cpu->sp++;
#define DECrr(X,Y)  OPXY(DECrr, X, Y);  cpu->r[Y]--; if (cpu->r[Y] == 0xff) cpu->r[X]--;
#define DECSP       OP(DECSP);          cpu->sp++;

/** CPU control instructions **/

#define CCF     OP(CCF);  KEEP_ZERO; cpu->flags |= (cpu->flags ^ FLAG_C); 
#define SCF     OP(SCF);  KEEP_ZERO; cpu->flags |= FLAG_C; 
#define HALT    OP(HALT); cpu->halt = 1; 
#define STOP    OP(STOP); cpu->stop = 1; 
#define NOP     OP(NOP);  
#define DI      OP(DI);   cpu->ime = 0; 
#define EI      OP(EI);   cpu->ime = 1; 

/** Jump and call instructions **/

/* Jump and call function templates */
#define JR_X(X)   if (X) { cpu->pc += (int8_t) CPU_RB (cpu->pc); cpu->rt += 4; } cpu->pc++;
#define CALL_X(X) if (X) { cpu->sp -= 2; CPU_WW (cpu->sp, cpu->pc + 2); cpu->pc = CPU_RW (cpu->pc); cpu->rt += 12; } else cpu->pc += 2;

/* Jump to | relative jump */
#define JPNN    OP(JPNN); cpu->pc = CPU_RW(cpu->pc);
#define JPHL    OP(JPHL); cpu->pc = ADDR_HL; 
#define JRm     OP(JRm);  cpu->pc += (int8_t) CPU_RB (cpu->pc); cpu->pc++;

/* Conditional relative jump */
#define JRZ     OP(JRZ);  JR_X (cpu->flags & FLAG_Z);
#define JRNZ    OP(JRNZ); JR_X (!(cpu->flags & FLAG_Z));
#define JRC     OP(JRC);  JR_X (cpu->flags & FLAG_C);
#define JRNC    OP(JRNC); JR_X (!(cpu->flags & FLAG_C));

/* Calls */
#define CALLm   OP(CALLm); uint16_t tmp = CPU_RW (cpu->pc); cpu->pc += 2; cpu->sp -= 2; CPU_WW(cpu->sp, cpu->pc); cpu->pc = tmp;
#define CALLZ   OP(CALLZ);  CALL_X (cpu->flags & FLAG_Z);
#define CALLNZ  OP(CALLNZ); CALL_X (!(cpu->flags & FLAG_Z));
#define CALLC   OP(CALLC);  CALL_X (cpu->flags & FLAG_C);
#define CALLNC  OP(CALLNC); CALL_X (!(cpu->flags & FLAG_C));

#define RET     OP(RET);   cpu->pc = CPU_RW (cpu->sp); cpu->sp += 2;
#define RETI    OP(RETI);  cpu->pc = CPU_RW (cpu->sp); cpu->sp += 2; cpu->ime = 1;

#define RST(N)  OPN(RST, N); cpu->sp -= 2; mmu_ww (mmu, cpu->sp, cpu->pc); cpu->pc = N;

/* Rotate and shift instructions */

#define RLA   	OP(RLA);   uint8_t ci = cpu->r[A]; cpu->r[A] = (cpu->r[A] << 1) | (cpu->flags & 0x10 ? 1 : 0); cpu->flags = ((ci >> 7) & 1) * 0x10; 
#define RRA     OP(RRA);   uint8_t ci = cpu->r[A]; cpu->r[A] = (cpu->r[A] << 1) | ((cpu->flags & 0x10 ? 1 : 0) << 7); cpu->flags = (ci & 1) * 0x10; 
#define RLCA    OP(RLCA);  cpu->r[A] = (cpu->r[A] << 1) | (cpu->r[A] >> 7); cpu->flags = (cpu->r[A] & 1) * 0x10; 
#define RRCA    OP(RRCA);  cpu->flags = (cpu->r[A] & 1) * 0x10; cpu->r[A] = (cpu->r[A] >> 1) | (cpu->r[A] << 7); 

/* Misc instructions */

#define PREFIX    OP(PREFIX)   uint8_t cb = CPU_RB (cpu->pc++); cpu_exec_cb (cb);
#define INVALID   OP(INVALID); cpu->invalid = 1;