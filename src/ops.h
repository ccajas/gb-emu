#define CPU_RB(A)     mmu_rb(mmu, A)
#define CPU_WB(A,X)   mmu_wb(mmu, A, X)
#define CPU_RW(A)     mmu_rw(mmu, A)
#define CPU_WW(A,X)   mmu_ww(mmu, A, X)

/* Instruction helpers */

#define ADDR_HL       ((cpu->r[H] << 8) + cpu->r[L])
#define ADDR_XY(X,Y)  ((cpu->r[X] << 8) + cpu->r[Y])
#define IMM           CPU_RB (cpu->pc++)

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

#define LD        OP(LD);     cpu->r[r1] = cpu->r[r2];
#define LDrm      OP(LDrm);   cpu->r[r1] = IMM;
#define LDrHL     OP(LDrHL);  cpu->r[r1] = CPU_RB (ADDR_HL);

#define LDHLr     OP(LDHLr);  CPU_WB (ADDR_HL, cpu->r[r2]);
#define LDHLm     OP(LDHLm);  CPU_WB (ADDR_HL, IMM);

#define LDArrm    OP(LDArrm); cpu->r[A] = CPU_RB (ADDR_XY(r1 - 1, r1));
#define LDAmm     OP(LDAmm);  cpu->r[A] = CPU_RB (CPU_RW (cpu->pc)); cpu->pc += 2;
#define LDrrmA    OP(LDrrmA); CPU_WB (ADDR_XY (r1, r1 + 1), cpu->r[A]);
#define LDmmA     OP(LDmmA);  CPU_WB (CPU_RW (cpu->pc), cpu->r[A]);  cpu->pc += 2;

#define LDIOmA    OP(LDIOmA); CPU_WB (0xFF00 + IMM, cpu->r[A]);
#define LDAIOm    OP(LDAIOm); cpu->r[A] = CPU_RB (0xFF00 + IMM);
#define LDIOCA    OP(LDIOCA); CPU_WB (0xFF00 + cpu->r[C], cpu->r[A]);
#define LDAIOC    OP(LDAIOC); cpu->r[A] = CPU_RB (0xFF00 + cpu->r[C]); 

/* Increment instruction templates */
    #define INCR_HL   cpu->r[L]++; if (!cpu->r[L]) cpu->r[H]++
    #define DECR_HL   cpu->r[L]--; if (cpu->r[L] == 255) cpu->r[H]--

#define LDHLIA    OP(LDHLIA); CPU_WB (ADDR_HL, cpu->r[A]); INCR_HL;
#define LDHLDA    OP(LDHLDA); CPU_WB (ADDR_HL, cpu->r[A]); DECR_HL;
#define LDAHLI    OP(LDAHLI); cpu->r[A] = CPU_RB (ADDR_HL); INCR_HL;
#define LDAHLD    OP(LDAHLD); cpu->r[A] = CPU_RB (ADDR_HL); DECR_HL;

/** 16-bit load instructions **/

#define LDrr      OP(LDrr);   cpu->r[r1 + 1] = CPU_RB (cpu->pc); cpu->r[r1] = CPU_RB (cpu->pc + 1); cpu->pc += 2;
#define LDmSP     OP(LDmSP);  uint16_t val = CPU_RW (cpu->pc); CPU_WW (val, cpu->sp); cpu->pc += 2;
#define LDSP      OP(LDSP);   cpu->sp = CPU_RW (cpu->pc); cpu->pc += 2;
#define LDSPHL    OP(LDSPHL); cpu->sp = ADDR_HL;   

#define PUSH      OP(PUSH);   cpu->sp--; CPU_WB (cpu->sp, cpu->r[r1]); cpu->sp--; CPU_WB (cpu->sp, cpu->r[r1 + 1]);
#define PUSHF     OP(PUSHF);  cpu->sp--; CPU_WB (cpu->sp, cpu->r[A]);  cpu->sp--; CPU_WB (cpu->sp, cpu->flags);
#define POP       OP(POP);    cpu->r[r1 + 1] = CPU_RB (cpu->sp++); cpu->r[r1] = CPU_RB (cpu->sp++);
#define POPF      OP(POPF);   cpu->flags = CPU_RB (cpu->sp++);     cpu->r[A]  = CPU_RB (cpu->sp++);

/* Flag setting helpers */

#define FLAG_Z   0x80
#define FLAG_N   0x40
#define FLAG_H   0x20
#define FLAG_C   0x10

#define SET_FLAG_Z(X)    cpu->f_z = (X == 0);
#define SET_FLAG_H_S(X)  cpu->f_h = ((X) & 0x10) > 0;
#define SET_FLAG_H(X)    cpu->f_h = (X);
#define SET_FLAG_C(X)    cpu->f_c = (X);

#define KEEP_ZERO       cpu->flags &= FLAG_Z;
#define KEEP_CARRY      cpu->flags &= FLAG_C;
#define KEEP_ZERO_CARRY cpu->flags &= (FLAG_Z | FLAG_C);

#define SET_ADD_FLAGS(X)  SET_FLAG_Z (cpu->r[A]); cpu->f_n = 0; SET_FLAG_H_S (cpu->r[A] ^ X ^ tmp); cpu->f_c = (cpu->r[A] < tmp);  
#define SET_SUB_FLAGS(X)  SET_FLAG_Z (cpu->r[A]); cpu->f_n = 1; SET_FLAG_H_S (cpu->r[A] ^ X ^ tmp); cpu->f_c = (cpu->r[A] > tmp);
#define SET_IMM_FLAGS     SET_FLAG_Z ((tmp & 0xFF)); SET_FLAG_H_S (cpu->r[A] ^ m ^ tmp); cpu->f_c = (tmp & 0xFF00) ? 1 : 0;

/** 8-bit arithmetic/logic instructions **/

    /* Add function templates */
    #define ADD_A_X(X)  cpu->r[A] += X; SET_ADD_FLAGS(X);
    #define ADD_AC_X(X) cpu->r[A] += X; cpu->r[A] += (cpu->f_c) ? 1 : 0; SET_ADD_FLAGS(X);
    #define SUB_A_X(X)  cpu->r[A] -= X; SET_SUB_FLAGS(X);
    #define SUB_AC_X(X) cpu->r[A] -= X; cpu->r[A] -= (cpu->f_c) ? 1 : 0; SET_SUB_FLAGS(X); 

/* Add and subtract */
#define ADD      OP(ADD); {\
    uint8_t tmp = cpu->r[A] + cpu->r[r2];\
	cpu->f_c = (cpu->r[A] > tmp);\
	cpu->f_h = ((cpu->r[A] ^ cpu->r[r2] ^ tmp) & 0x10) > 0;\
	cpu->f_n = 0;\
	cpu->f_z = ((tmp & 0xFF) == 0x00);\
	cpu->r[A] = (tmp & 0xFF);\
}
#define ADHL     OP(ADHL);    ADD_A_X(hl);

#define ADC      OP(ADC); {\
    uint16_t tmp = cpu->r[A] + cpu->r[r2] + cpu->f_c;\
	cpu->f_c = (tmp & 0xFF00) ? 1 : 0;\
	cpu->f_h = ((cpu->r[A] ^ cpu->r[r2] ^ tmp) & 0x10) > 0;\
	cpu->f_n = 0;\
	cpu->f_z = ((tmp & 0xFF) == 0x00);\
	cpu->r[A] = (tmp & 0xFF);\
}

#define ACHL     OP(ACHL);    ADD_AC_X(hl);

#define SUB      OP(SUB); {\
    uint8_t tmp = cpu->r[A] - (cpu->r[r2]);\
    cpu->f_c = (tmp > cpu->r[A]) ? 1 : 0;\
    cpu->f_h = ((cpu->r[A] ^ cpu->r[r2] ^ tmp) & 0x10) > 0;\
    cpu->f_n = 1;\
    cpu->f_z = ((tmp & 0xFF) == 0x00);\
    cpu->r[A] = (tmp & 0xFF);\
}
#define SBHL     OP(SBHL);    SUB_A_X(hl);

#define SBC      OP(SBC); {\
    uint16_t tmp = cpu->r[A] - (cpu->r[r2] + cpu->f_c);\
    cpu->f_c = (tmp & 0xFF00) ? 1 : 0;\
    cpu->f_h = ((cpu->r[A] ^ cpu->r[r2] ^ tmp) & 0x10) > 0;\
    cpu->f_n = 1;\
    cpu->f_z = ((tmp & 0xFF) == 0x00);\
    cpu->r[A] = (tmp & 0xFF);\
}
#define SCHL     OP(SCHL);    SUB_AC_X(hl);

/* Bitwise logic */
#define AND      OP(AND)      cpu->r[A] &= cpu->r[r2]; cpu->flags = 0; SET_FLAG_Z (cpu->r[A]); SET_FLAG_H (1);
#define ANHL     OP(ANHL);    cpu->r[A] &= hl;         cpu->flags = 0; SET_FLAG_Z (cpu->r[A]); SET_FLAG_H (1);
#define XOR      OP(XOR)      cpu->r[A] ^= cpu->r[r2]; cpu->flags = 0; SET_FLAG_Z (cpu->r[A]);
#define XRHL     OP(XRHL);    cpu->r[A] ^= hl;         cpu->flags = 0; SET_FLAG_Z (cpu->r[A]);
#define OR       OP(OR);      cpu->r[A] |= cpu->r[r2]; cpu->flags = 0; SET_FLAG_Z (cpu->r[A]);  
#define ORHL     OP(ORHL)     cpu->r[A] |= hl;         cpu->flags = 0; SET_FLAG_Z (cpu->r[A]);

#define ADDm     OP(ADDm);    { uint8_t m = IMM; uint8_t tmp  = cpu->r[A]; ADD_A_X(m); }
#define ADCm     OP(ADCm)     { uint8_t m = IMM; uint16_t tmp = cpu->r[A] + m + cpu->f_c; SET_IMM_FLAGS; cpu->f_n = 0; cpu->r[A] = (tmp & 0xFF); }

#define SUBm     OP(SUBm);    { uint8_t m = IMM; uint8_t tmp  = cpu->r[A]; SUB_A_X(m); }
#define SBCm     OP(SBCm)     { uint8_t m = IMM; uint16_t tmp = cpu->r[A] - (m + cpu->f_c); SET_IMM_FLAGS; cpu->f_n = 1; cpu->r[A] = (tmp & 0xFF); }

#define ANDm     OP(ANDm);    cpu->r[A] &= IMM; cpu->flags = 0; SET_FLAG_Z (cpu->r[A]); cpu->flags |= FLAG_H;
#define XORm     OP(XORm);    cpu->r[A] ^= IMM; cpu->flags = 0; SET_FLAG_Z (cpu->r[A]);
#define ORm      OP(ORm);     cpu->r[A] |= IMM; cpu->flags = 0; SET_FLAG_Z (cpu->r[A]);

    /* Flag template for CP instructions */
    #define FLAGS_CP  cpu->flags = FLAG_N; SET_FLAG_C (tmp > cpu->r[A]); SET_FLAG_Z (tmp); SET_FLAG_H ((tmp & 0xF) > (cpu->r[A] & 0xF));

#define CP       OP(CP);      tmp -= cpu->r[r2]; FLAGS_CP;
#define CPHL     OP(CPHL);    tmp -= hl;         FLAGS_CP;
#define CPm      OP(CPm);     uint8_t m = IMM; tmp -= m; FLAGS_CP;
/*{\
    uint8_t ds = cpu->r[A] - CPU_RB (cpu->pc++);\
    SET_FLAG_H ((ds & 0xF) > (cpu->r[A] & 0xF)); SET_FLAG_C (ds > cpu->r[A]); SET_FLAG_Z (ds); cpu->f_n = 1;\
}*/

#define INC      OP(INC);     cpu->r[r1]++; SET_FLAG_H (!(cpu->r[r1] & 0xF));       SET_FLAG_Z (cpu->r[r1]); cpu->f_n = 0;
#define DEC      OP(DEC);     cpu->r[r1]--; SET_FLAG_H ((cpu->r[r1] & 0xF) == 0xF); SET_FLAG_Z (cpu->r[r1]); cpu->f_n = 1;
#define INCHL    OP(INCHL);   { uint8_t tmp = CPU_RB (ADDR_HL) + 1; CPU_WB (ADDR_HL, tmp); SET_FLAG_Z (tmp); SET_FLAG_H ((tmp & 0xF) == 0);   cpu->f_n = 0; }
#define DECHL    OP(DECHL);   { uint8_t tmp = CPU_RB (ADDR_HL) - 1; CPU_WB (ADDR_HL, tmp); SET_FLAG_Z (tmp); SET_FLAG_H ((tmp & 0xF) == 0xF); cpu->f_n = 1; }
#define CPL      OP(CPL);     cpu->r[A] ^= 0xFF; cpu->flags |= 0x60; 

/** 16-bit arithmetic instructions **/

    /* Flag templates for Add HL instructions */
    #define FLAGS_ADHL  cpu->f_n = 0;   cpu->f_h = ((hl & 0xfff) > (tmp & 0xfff));      cpu->f_c = (hl > tmp) ? 1 : 0;
    #define FLAGS_SPm   cpu->flags = 0; cpu->f_h = ((cpu->sp & 0xF) + (i & 0xF) > 0xF); cpu->f_c = ((cpu->sp & 0xFF) + (i & 0xFF) > 0xFF);

#define ADHLrr   OP(ADHLrr);  {\
    uint16_t tmp = (r1 == L) ? hl << 1 : hl + ADDR_XY(r1 - 1, r1); FLAGS_ADHL;\
    cpu->r[H] = (tmp >> 8); cpu->r[L] = tmp & 0xFF;\
}

#define ADHLSP   OP(ADHLSP);  {\
    uint16_t tmp = hl + cpu->sp; FLAGS_ADHL;\
    cpu->r[H] = (tmp >> 8); cpu->r[L] = tmp & 0xFF;\
}

#define ADDSPm   OP(ADDSPm);  { int8_t i = (int8_t) IMM; FLAGS_SPm; cpu->sp += i; }
#define LDHLSP   OP(LDHLSP);  { int8_t i = (int8_t) IMM;\
    cpu->r[H] = ((cpu->sp + i) >> 8); cpu->r[L] = (cpu->sp + i) & 0xFF;\
    cpu->flags = 0;\
    cpu->f_h = ((cpu->sp & 0xF) + (i & 0xF) > 0xF);\
    cpu->f_c = ((cpu->sp & 0xFF) + (i & 0xFF) > 0xFF);\
}

#define INCrr    OP(INCrr);   cpu->r[r1 + 1]++; if (!cpu->r[r1 + 1]) cpu->r[r1]++;
#define INCSP    OP(INCSP);   cpu->sp++;
#define DECrr    OP(DECrr);   cpu->r[r1]--; if (cpu->r[r1] == 0xff) cpu->r[r1 - 1]--;
#define DECSP    OP(DECSP);   cpu->sp--;

/** CPU control instructions **/

#define CCF     OP(CCF);  cpu->f_c = !cpu->f_c; cpu->f_n = cpu->f_h = 0;
#define SCF     OP(SCF);  cpu->f_c = 1; cpu->f_n = cpu->f_h = 0;
#define HALT    OP(HALT); cpu->halt = 1; 
#define STOP    OP(STOP); cpu->stop = 1; 
#define NOP     OP(NOP);
#define DI      OP(DI);   cpu->ime = 0; 
#define EI      OP(EI);   cpu->ime = 1; 

/** Jump and call instructions **/

    /* Jump and call function templates */
    #define JP_IF(X)   if (X) { cpu->pc = CPU_RW (cpu->pc); cpu->rt += 4; } else cpu->pc += 2;
    #define JR_IF(X)   if (X) { cpu->pc += (int8_t) CPU_RB (cpu->pc); cpu->rt += 4; } cpu->pc++;   
    #define CALL_IF(X) if (X) { cpu->sp -= 2; CPU_WW (cpu->sp, cpu->pc + 2); cpu->pc = CPU_RW (cpu->pc); cpu->rt += 12; } else cpu->pc += 2;

/* Jump to | relative jump */
#define JPNN    OP(JPNN); cpu->pc = CPU_RW (cpu->pc);
#define JPHL    OP(JPHL); cpu->pc = ADDR_HL; 
#define JRm     OP(JRm);  cpu->pc += (int8_t) CPU_RB (cpu->pc); cpu->pc++;

#define JPZ     OP(JPZ);  JP_IF (cpu->f_z);
#define JPNZ    OP(JPNZ); JP_IF (!cpu->f_z);
#define JPC     OP(JPC);  JP_IF (cpu->f_c);
#define JPNC    OP(JPNC); JP_IF (!cpu->f_c);

/* Conditional relative jump */
#define JRZ     OP(JRZ);  JR_IF (cpu->f_z);
#define JRNZ    OP(JRNZ); JR_IF (!cpu->f_z);
#define JRC     OP(JRC);  JR_IF (cpu->f_c);
#define JRNC    OP(JRNC); JR_IF (!cpu->f_c);

/* Calls */
#define CALLm   OP(CALLm);  { uint16_t tmp = CPU_RW (cpu->pc); cpu->pc += 2; cpu->sp -= 2; CPU_WW(cpu->sp, cpu->pc); cpu->pc = tmp; }
#define CALLZ   OP(CALLZ);  CALL_IF (cpu->f_z);
#define CALLNZ  OP(CALLNZ); CALL_IF (!(cpu->f_z));
#define CALLC   OP(CALLC);  CALL_IF (cpu->f_c);
#define CALLNC  OP(CALLNC); CALL_IF (!(cpu->f_c));

    /* Return function templates */
    #define RET__     cpu->pc = CPU_RW (cpu->sp); cpu->sp += 2;
    #define RET_IF(X) if (X) { RET__; cpu->rt += 12; }

#define RET     OP(RET);   RET__;
#define RETI    OP(RETI);  RET__; cpu->ime = 1;
#define RETZ    OP(RETZ);  RET_IF (cpu->f_z);
#define RETNZ   OP(RETNZ); RET_IF (!(cpu->f_z));
#define RETC    OP(RETC);  RET_IF (cpu->f_c);
#define RETNC   OP(RETNC); RET_IF (!(cpu->f_c));

#define RST     OP(RST);   uint16_t n = (opHh - 0x18) << 3; cpu->sp -= 2; mmu_ww (mmu, cpu->sp, cpu->pc); cpu->pc = n;

/* Rotate and shift instructions */

#define RLA   	OP(RLA);   { uint8_t tmp = cpu->r[A]; cpu->r[A] = (cpu->r[A] << 1) | (cpu->f_c);      cpu->flags = ((tmp >> 7) & 1) * FLAG_C; }
#define RRA     OP(RRA);   { uint8_t tmp = cpu->r[A]; cpu->r[A] = (cpu->r[A] >> 1) | (cpu->f_c << 7); cpu->flags = (tmp & 1) * FLAG_C; }
#define RLCA    OP(RLCA);  cpu->r[A] = (cpu->r[A] << 1) | (cpu->r[A] >> 7); cpu->flags = (cpu->r[A] & 1) * FLAG_C; 
#define RRCA    OP(RRCA);  cpu->flags = (cpu->r[A] & 1) * FLAG_C; cpu->r[A] = (cpu->r[A] >> 1) | (cpu->r[A] << 7); 

#define RLC     NYI("RLC");
#define RLCHL   NYI("RLCHL");

#define RL      OP(RR); {\
    uint8_t tmp = cpu->r[r];\
    cpu->r[r] <<= 1; cpu->r[r] |= cpu->f_c;\
    cpu->flags = (!cpu->r[r]) * FLAG_Z;\
    cpu->f_c = tmp >> 7;\
}

#define RLHL    NYI("RLHL");

#define RRC     OP(RRC); {\
    uint8_t tmp = cpu->r[r];\
    cpu->r[r] >>= 1; cpu->r[r] |= (tmp << 7);\
    cpu->flags = (!cpu->r[r]) * FLAG_Z;\
    cpu->f_c = tmp & 1;\
}

#define RRCHL   NYI("RRCHL");

#define RR      OP(RR); {\
    uint8_t tmp = cpu->r[r];\
    cpu->r[r] >>= 1; cpu->r[r] |= cpu->f_c << 7;\
    cpu->flags = (!cpu->r[r]) * FLAG_Z;\
    cpu->f_c = tmp & 1;\
}

#define RRHL    NYI("RRHL");

#define SRL     OP(SRL);   { uint8_t co = (cpu->r[r] & 1) ? 0x10 : 0; cpu->r[r] >>= 1; cpu->flags = (cpu->r[r]) ? 0 : FLAG_Z; cpu->flags = (cpu->flags & 0xEF) + co; }

#define BIT     OP(BIT);   KEEP_CARRY; cpu->flags |= FLAG_H; cpu->flags |= (cpu->r[r] & (1 << r_bit)) ? 0 : FLAG_Z;
#define BITHL   OP(BITHL); KEEP_CARRY; cpu->flags |= FLAG_H; cpu->flags |= (hl & (1 << r_bit)) ? 0 : FLAG_Z;

#define RES     OP(RES);   cpu->r[r] &= (0xFE << r_bit) | (0xFF >> (8 - r_bit));
#define RESHL   OP(RESHL); hl &= (0xFE << r_bit) | (0xFF >> (8 - r_bit)); CPU_WB (ADDR_HL ,hl);

#define SET     OP(SET);   cpu->r[r] |= (1 << r_bit); 
#define SETHL   OP(SET);   hl |= (1 << r_bit); CPU_WB (ADDR_HL ,hl);

/* Misc instructions */

#define PREFIX    OP(PREFIX)   { uint8_t cb = CPU_RB (cpu->pc++); cpu_exec_cb (cb); }
#define INVALID   OP(INVALID); cpu->invalid = 1;