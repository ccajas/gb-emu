/* Instruction helpers */

#define ADDR_HL       ((gb->r[H] << 8) + gb->r[L])
#define ADDR_XY(X,Y)  ((gb->r[X] << 8) + gb->r[Y])
#define IMM           CPU_RB (gb->pc++)

#define CPU_RB(A)     gb_mem_access (gb, A, 0, 0)
#define CPU_WB(A,X)   gb_mem_access (gb, A, (X), 1)
#define CPU_RW(A)     ( CPU_RB (A) +  (CPU_RB (A + 1) << 8) )
#define CPU_WW(A,X)   { CPU_WB (A, X); CPU_WB (A + 1, (X >> 8)); }

#ifdef GB_OPS_DEBUG

#define NYI(S)           LOG_("Instruction not implemented! (%s)", S); gb->ni++;
#define OP(name)         LOG_("%s", #name);
#define OPX(name, X)     LOG_("%s %c", #name, gb->reg_names[X]);
#define OPXY(name, X, Y) LOG_("%s %c,%c", #name, gb->reg_names[X], gb->reg_names[Y]);
#define OPN(name, N)     LOG_("%s 0x%02x", #name, N);

#else

#define NYI(S)
#define OP(name)
#define OPX(name, X)
#define OPXY(name, X, Y)
#define OPN(name, N)

#endif  

/** 8-bit load instructions **/   

#define LD        OP(LD);       if (r1 != r2) { gb->r[r1] = gb->r[r2]; }
#define LDrm      OP(LDrm);     gb->r[r1] = IMM;
#define LDrHL     OP(LDrHL);    gb->r[r1] = CPU_RB (ADDR_HL);

#define LDHLr     OP(LDHLr);    CPU_WB (ADDR_HL, gb->r[r2]);
#define LDHLm     OP(LDHLm);    CPU_WB (ADDR_HL, IMM);

#define LDArrm    OP(LDArrm);   gb->r[A] = CPU_RB (ADDR_XY(r1 - 1, r1));
#define LDAmm     OP(LDAmm);    gb->r[A] = CPU_RB (CPU_RW (gb->pc)); gb->pc += 2;
#define LDrrmA    OP(LDrrmA);   CPU_WB (ADDR_XY (r1, r1 + 1), gb->r[A]);
#define LDmmA     OP(LDmmA);    CPU_WB (CPU_RW (gb->pc), gb->r[A]);  gb->pc += 2;

#define LDIOmA    OP(LDIOmA);   CPU_WB (0xFF00 + IMM, gb->r[A]);
#define LDAIOm    OP(LDAIOm);   gb->r[A] = CPU_RB (0xFF00 + IMM);
#define LDIOCA    OP(LDIOCA);   CPU_WB (0xFF00 + gb->r[C], gb->r[A]);
#define LDAIOC    OP(LDAIOC);   gb->r[A] = CPU_RB (0xFF00 + gb->r[C]); 

    /* Increment instruction templates */
    #define INCR_HL   gb->r[L]++; if (!gb->r[L]) gb->r[H]++
    #define DECR_HL   gb->r[L]--; if (gb->r[L] == 255) gb->r[H]--

#define LDHLIA    OP(LDHLIA);   CPU_WB  (ADDR_HL, gb->r[A]); INCR_HL;
#define LDHLDA    OP(LDHLDA);   CPU_WB  (ADDR_HL, gb->r[A]); DECR_HL;
#define LDAHLI    OP(LDAHLI);   gb->r[A] = CPU_RB (ADDR_HL); INCR_HL;
#define LDAHLD    OP(LDAHLD);   gb->r[A] = CPU_RB (ADDR_HL); DECR_HL;

/** 16-bit load instructions **/

#define LDrr      OP(LDrr);   \
    gb->r[r1 + 1] = CPU_RB (gb->pc);\
    gb->r[r1] = CPU_RB (gb->pc + 1);\
    gb->pc += 2;\

#define LDmSP     OP(LDmSP);  \
    CPU_WW (CPU_RW (gb->pc), gb->sp);\
    gb->pc += 2;\

#define LDSP      OP(LDSP);   gb->sp = CPU_RW (gb->pc); gb->pc += 2;
#define LDSPHL    OP(LDSPHL); gb->sp = ADDR_HL;   

#define PUSH      OP(PUSH);   gb->sp--; CPU_WB (gb->sp, gb->r[r1]); gb->sp--; CPU_WB (gb->sp, gb->r[r1 + 1]);
#define PUSHF     OP(PUSHF);  gb->sp--; CPU_WB (gb->sp, gb->r[A]);  gb->sp--; CPU_WB (gb->sp, gb->flags);
#define POP       OP(POP);    gb->r[r1 + 1] = CPU_RB (gb->sp++); gb->r[r1] = CPU_RB (gb->sp++);
#define POPF      OP(POPF);   {\
    uint8_t sp = CPU_RB(gb->sp++);\
    gb->r[A]  = CPU_RB(gb->sp++);\
    gb->flags = 0; uint8_t fl = 7;\
    while (fl >= 4) {\
        gb->flags |= (((sp >> fl) & 1) << fl); fl--; }\
}\

/* Flag setting helpers */

#define FLAG_Z   0x80
#define FLAG_N   0x40
#define FLAG_H   0x20
#define FLAG_C   0x10

#define SET_FLAG_Z(X)    gb->f_z = (X == 0);
#define SET_FLAG_H_S(X)  gb->f_h = ((X) & 0x10) > 0;
#define SET_FLAG_H(X)    gb->f_h = (X);
#define SET_FLAG_C(X)    gb->f_c = (X);

#define KEEP_ZERO       gb->flags &= FLAG_Z;
#define KEEP_CARRY      gb->flags &= FLAG_C;
#define KEEP_ZERO_CARRY gb->flags &= (FLAG_Z | FLAG_C);

#define SET_ADD_FLAGS(X)  \
    SET_FLAG_Z (gb->r[A]);\
    gb->f_n = 0;\
    SET_FLAG_H_S (gb->r[A] ^ X ^ tmp);\
    gb->f_c = (gb->r[A] < tmp);

#define SET_SUB_FLAGS(X)  \
    SET_FLAG_Z (gb->r[A]);\
    gb->f_n = 1;\
    SET_FLAG_H_S (gb->r[A] ^ X ^ tmp);\
    gb->f_c = (gb->r[A] > tmp);

#define SET_IMM_FLAGS     SET_FLAG_Z ((tmp & 0xFF)); SET_FLAG_H_S (gb->r[A] ^ m ^ tmp); gb->f_c = (tmp & 0xFF00) ? 1 : 0;

/** 8-bit arithmetic/logic instructions **/

    /* Add function templates */
    #define ADD_A_X(X)  gb->r[A] += X; SET_ADD_FLAGS(X);
    #define ADD_AC_X(X) gb->r[A] += X; gb->r[A] += (gb->f_c) ? 1 : 0; SET_ADD_FLAGS(X);
    #define SUB_A_X(X)  gb->r[A] -= X; SET_SUB_FLAGS(X);
    #define SUB_AC_X(X) gb->r[A] -= X; gb->r[A] -= (gb->f_c) ? 1 : 0; SET_SUB_FLAGS(X); 

/* Add and subtract */

#define ADD      OP(ADD); {\
    uint8_t tmp = gb->r[A] + gb->r[r2];\
	SET_FLAG_Z ((tmp & 0xFF));\
	gb->f_n = 0;\
	SET_FLAG_H_S (gb->r[A] ^ gb->r[r2] ^ tmp);\
	gb->f_c = (gb->r[A] > tmp);\
	gb->r[A] = tmp;\
}
#define ADHL     OP(ADHL); {\
    uint8_t tmp = gb->r[A] + hl;\
    SET_FLAG_Z ((tmp & 0xFF));\
    gb->f_n = 0;\
    SET_FLAG_H_S (gb->r[A] ^ hl ^ tmp);\
    gb->f_c = (gb->r[A] > tmp);\
    gb->r[A] = tmp;\
}

#define ADC      OP(ADC); {\
    uint16_t tmp = gb->r[A] + gb->r[r2] + gb->f_c;\
	SET_FLAG_Z ((tmp & 0xFF));\
	gb->f_n = 0;\
	SET_FLAG_H_S (gb->r[A] ^ gb->r[r2] ^ tmp);\
	gb->f_c = (tmp >= 0x100);\
	gb->r[A] = tmp & 0xFF;\
}

#define ACHL     OP(ACHL); {\
    uint16_t tmp = gb->r[A] + hl + gb->f_c;\
	SET_FLAG_Z ((tmp & 0xFF));\
	gb->f_n = 0;\
	SET_FLAG_H_S (gb->r[A] ^ hl ^ tmp);\
	gb->f_c = (tmp >= 0x100);\
	gb->r[A] = tmp & 0xFF;\
}
//ADD_AC_X(hl);

#define SUB      OP(SUB); {\
    uint8_t tmp = gb->r[A] - gb->r[r2];\
    gb->f_z = ((tmp & 0xFF) == 0x00);\
    gb->f_n = 1;\
    gb->f_h = ((gb->r[A] ^ gb->r[r2] ^ tmp) & 0x10) > 0;\
    gb->f_c = (tmp > gb->r[A]) ? 1 : 0;\
	gb->r[A] = (tmp & 0xFF);\
}
#define SBHL     OP(SBHL);    SUB_A_X(hl);

#define SBC      OP(SBC); {\
    uint16_t tmp = gb->r[A] - gb->f_c - gb->r[r2];\
    gb->f_z = ((tmp & 0xFF) == 0x00);\
    gb->f_n = 1;\
    gb->f_h = ((gb->r[A] ^ gb->r[r2] ^ tmp) & 0x10) > 0;\
    gb->f_c = (tmp & 0xFF00) ? 1 : 0;\
    gb->r[A] = (tmp & 0xFF);\
}
#define SCHL     OP(SCHL);    SUB_AC_X(hl);

/* Bitwise logic */

#define AND      OP(AND)      gb->r[A] &= gb->r[r2];\
    gb->flags = 0;\
    SET_FLAG_Z (gb->r[A]);\
    SET_FLAG_H (1);

#define ANHL     OP(ANHL);    gb->r[A] &= hl;\
    gb->flags = 0;\
    SET_FLAG_Z (gb->r[A]);\
    SET_FLAG_H (1);

#define XOR      OP(XOR)      gb->r[A] ^= gb->r[r2];\
    gb->flags = 0;\
    SET_FLAG_Z (gb->r[A]);\

#define XRHL     OP(XRHL);    gb->r[A] ^= hl;\
    gb->flags = 0;\
    SET_FLAG_Z (gb->r[A]);\

#define OR       OP(OR);      gb->r[A] |= gb->r[r2];\
    gb->flags = 0;\
    SET_FLAG_Z (gb->r[A]);\

#define ORHL     OP(ORHL)     gb->r[A] |= hl;\
    gb->flags = 0;\
    SET_FLAG_Z (gb->r[A]);\

#define ADDm     OP(ADDm);    { uint8_t m = IMM; uint8_t tmp  = gb->r[A]; ADD_A_X(m); }
#define ADCm     OP(ADCm)     { uint8_t m = IMM; uint16_t tmp = gb->r[A] + m + gb->f_c; SET_IMM_FLAGS; gb->f_n = 0; gb->r[A] = (tmp & 0xFF); }

#define SUBm     OP(SUBm);    { uint8_t m = IMM; uint8_t tmp  = gb->r[A]; SUB_A_X(m); }
#define SBCm     OP(SBCm)     { uint8_t m = IMM; uint16_t tmp = gb->r[A] - (m + gb->f_c); SET_IMM_FLAGS; gb->f_n = 1; gb->r[A] = (tmp & 0xFF); }

#define ANDm     OP(ANDm);    gb->r[A] &= IMM; gb->flags = 0; SET_FLAG_Z (gb->r[A]); gb->flags |= FLAG_H;
#define XORm     OP(XORm);    gb->r[A] ^= IMM; gb->flags = 0; SET_FLAG_Z (gb->r[A]);
#define ORm      OP(ORm);     gb->r[A] |= IMM; gb->flags = 0; SET_FLAG_Z (gb->r[A]);

    /* Flag template for CP instructions */
    #define FLAGS_CP  gb->flags = FLAG_N; SET_FLAG_C (tmp > gb->r[A]); SET_FLAG_Z (tmp); SET_FLAG_H ((tmp & 0xF) > (gb->r[A] & 0xF));

#define CP       OP(CP);      tmp -= gb->r[r2];  FLAGS_CP;
#define CPHL     OP(CPHL);    tmp -= hl;         FLAGS_CP;
#define CPm      OP(CPm);     uint8_t m = IMM; tmp -= m; FLAGS_CP;
/*{\
    uint8_t ds = gb->r[A] - CPU_RB (gb->pc++);\
    SET_FLAG_H ((ds & 0xF) > (gb->r[A] & 0xF)); SET_FLAG_C (ds > gb->r[A]); SET_FLAG_Z (ds); gb->f_n = 1;\
}*/

#define INC      OP(INC);     gb->r[r1]++; SET_FLAG_H (!(gb->r[r1] & 0xF));       SET_FLAG_Z (gb->r[r1]); gb->f_n = 0;
#define DEC      OP(DEC);     gb->r[r1]--; SET_FLAG_H ((gb->r[r1] & 0xF) == 0xF); SET_FLAG_Z (gb->r[r1]); gb->f_n = 1;
#define INCHL    OP(INCHL);   { uint8_t tmp = CPU_RB (ADDR_HL) + 1; CPU_WB (ADDR_HL, tmp); SET_FLAG_Z (tmp); SET_FLAG_H ((tmp & 0xF) == 0);   gb->f_n = 0; }
#define DECHL    OP(DECHL);   { uint8_t tmp = CPU_RB (ADDR_HL) - 1; CPU_WB (ADDR_HL, tmp); SET_FLAG_Z (tmp); SET_FLAG_H ((tmp & 0xF) == 0xF); gb->f_n = 1; }

/* The following is from SameBoy. MIT License. */
#define DAA      OP(DAA);     {\
		int16_t a = gb->r[A];\
		if (gb->f_n) {\
			if (gb->f_h) a = (a - 0x06) & 0xFF;\
			if (gb->f_c) a -= 0x60;\
		} else {\
			if (gb->f_h || (a & 0x0F) > 9) a += 0x06;\
			if (gb->f_c || a > 0x9F) a += 0x60;\
		}\
		gb->r[A] = a;\
		if ((a & 0x100) == 0x100) SET_FLAG_C (1);\
		SET_FLAG_Z (gb->r[A]);\
		SET_FLAG_H (0);\
}

#define CPL      OP(CPL);     gb->r[A] ^= 0xFF; gb->flags |= 0x60; 

/** 16-bit arithmetic instructions **/

    /* Flag templates for Add HL instructions */
    #define FLAGS_ADHL  gb->f_n = 0;   gb->f_h = ((hl & 0xfff) > (tmp & 0xfff));      gb->f_c = (hl > tmp) ? 1 : 0;
    #define FLAGS_SPm   gb->flags = 0; gb->f_h = ((gb->sp & 0xF) + (i & 0xF) > 0xF); gb->f_c = ((gb->sp & 0xFF) + (i & 0xFF) > 0xFF);

#define ADHLrr   OP(ADHLrr);  {\
    uint16_t tmp = (r1 == L) ? hl << 1 : hl + ADDR_XY(r1 - 1, r1); FLAGS_ADHL;\
    gb->r[H] = (tmp >> 8); gb->r[L] = tmp & 0xFF;\
}

#define ADHLSP   OP(ADHLSP);  {\
    uint16_t tmp = hl + gb->sp; FLAGS_ADHL;\
    gb->r[H] = (tmp >> 8); gb->r[L] = tmp & 0xFF;\
}

#define ADDSPm   OP(ADDSPm);  { int8_t i = (int8_t) IMM; FLAGS_SPm; gb->sp += i; }
#define LDHLSP   OP(LDHLSP);  { int8_t i = (int8_t) IMM;\
    gb->r[H] = ((gb->sp + i) >> 8); gb->r[L] = (gb->sp + i) & 0xFF;\
    gb->flags = 0;\
    gb->f_h = ((gb->sp & 0xF) + (i & 0xF) > 0xF);\
    gb->f_c = ((gb->sp & 0xFF) + (i & 0xFF) > 0xFF);\
}

#define INCrr    OP(INCrr);   gb->r[r1 + 1]++; if (!gb->r[r1 + 1]) gb->r[r1]++;
#define INCSP    OP(INCSP);   gb->sp++;
#define DECrr    OP(DECrr);   gb->r[r1]--; if (gb->r[r1] == 0xff) gb->r[r1 - 1]--;
#define DECSP    OP(DECSP);   gb->sp--;

/** CPU control instructions **/

#define CCF     OP(CCF);  gb->f_c = !gb->f_c; gb->f_n = gb->f_h = 0;
#define SCF     OP(SCF);  gb->f_c = 1; gb->f_n = gb->f_h = 0;
#define HALT    OP(HALT); gb->halted = 1; 
#define STOP    OP(STOP); gb->stop = 1; /* STOP is handled after switch/case */
#define NOP     OP(NOP);
#define DI      OP(DI);   gb->ime = 0; 
#define EI      OP(EI);   gb->ime = 1; 

/** Jump and call instructions **/

/* Jump to | relative jump */
#define JPNN    OP(JPNN); gb->pc = CPU_RW (gb->pc);
#define JPHL    OP(JPHL); gb->pc = ADDR_HL; 
#define JRm     OP(JRm);  gb->pc += (int8_t) CPU_RB (gb->pc); gb->pc++;

    /* Jump and call function templates */
    #define JP_IF(X)   if (X) { gb->pc = CPU_RW (gb->pc); gb->rt += 4; } else gb->pc += 2;
    #define JR_IF(X)   if (X) { gb->pc += (int8_t) CPU_RB (gb->pc); gb->rt += 4; } gb->pc++;   
    #define CALL_IF(X) if (X) { gb->sp -= 2; CPU_WW (gb->sp, (gb->pc + 2)); gb->pc = CPU_RW (gb->pc); gb->rt += 12; } else gb->pc += 2;

    /* Return function templates */
    #define RET__     gb->pc = CPU_RW (gb->sp); gb->sp += 2;
    #define RET_IF(X) if (X) { RET__; gb->rt += 12; }

/* Conditional jump */

#define JPZ     OP(JPZ);  JP_IF (gb->f_z);
#define JPNZ    OP(JPNZ); JP_IF (!gb->f_z);
#define JPC     OP(JPC);  JP_IF (gb->f_c);
#define JPNC    OP(JPNC); JP_IF (!gb->f_c);

/* Conditional relative jump */
#define JRZ     OP(JRZ);  JR_IF (gb->f_z);
#define JRNZ    OP(JRNZ); JR_IF (!gb->f_z);
#define JRC     OP(JRC);  JR_IF (gb->f_c);
#define JRNC    OP(JRNC); JR_IF (!gb->f_c);

/* Calls */
#define CALLm   OP(CALLm);  { uint16_t tmp = CPU_RW (gb->pc); gb->pc += 2; gb->sp -= 2; CPU_WW(gb->sp, gb->pc); gb->pc = tmp; }
#define CALLZ   OP(CALLZ);  CALL_IF (gb->f_z);
#define CALLNZ  OP(CALLNZ); CALL_IF (!(gb->f_z));
#define CALLC   OP(CALLC);  CALL_IF (gb->f_c);
#define CALLNC  OP(CALLNC); CALL_IF (!(gb->f_c));

#define RET     OP(RET);   RET__;
#define RETI    OP(RETI);  RET__; gb->ime = 1;
#define RETZ    OP(RETZ);  RET_IF (gb->f_z);
#define RETNZ   OP(RETNZ); RET_IF (!(gb->f_z));
#define RETC    OP(RETC);  RET_IF (gb->f_c);
#define RETNC   OP(RETNC); RET_IF (!(gb->f_c));

#define RST     OP(RST);   uint16_t n = (opHh - 0x18) << 3; gb->sp -= 2; CPU_WW (gb->sp, gb->pc); gb->pc = n;

/* Rotate and shift instructions */

#define RLA   	OP(RLA);   { uint8_t tmp = gb->r[A]; gb->r[A] = (gb->r[A] << 1) | (gb->f_c);      gb->flags = ((tmp >> 7) & 1) * FLAG_C; }
#define RRA     OP(RRA);   { uint8_t tmp = gb->r[A]; gb->r[A] = (gb->r[A] >> 1) | (gb->f_c << 7); gb->flags = (tmp & 1) * FLAG_C; }
#define RLCA    OP(RLCA);  gb->r[A] = (gb->r[A] << 1) | (gb->r[A] >> 7); gb->flags = (gb->r[A] & 1) * FLAG_C; 
#define RRCA    OP(RRCA);  gb->flags = (gb->r[A] & 1) * FLAG_C; gb->r[A] = (gb->r[A] >> 1) | (gb->r[A] << 7); 

#define RLC     OP(RLC); {\
    uint8_t tmp = gb->r[r];\
    gb->r[r] <<= 1; gb->r[r] |= (tmp >> 7);\
    gb->flags = (!gb->r[r]) * FLAG_Z;\
    gb->f_c = tmp >> 7;\
}

#define RLCHL   OP(RLCHL); {\
    uint8_t tmp = hl;\
    hl <<= 1; hl |= (tmp >> 7);\
    gb->flags = (!hl) * FLAG_Z;\
    gb->f_c = tmp >> 7;\
}

#define RL      OP(RL); {\
    uint8_t tmp = gb->r[r];\
    gb->r[r] <<= 1; gb->r[r] |= gb->f_c;\
    gb->flags = (!gb->r[r]) * FLAG_Z;\
    gb->f_c = tmp >> 7;\
}

#define RLHL    OP(RLHL); {\
    uint8_t tmp = hl;\
    hl <<= 1; hl |= gb->f_c;\
    gb->flags = (!hl) * FLAG_Z;\
    gb->f_c = tmp >> 7;\
}

#define RRC     OP(RRC); {\
    uint8_t tmp = gb->r[r];\
    gb->r[r] >>= 1; gb->r[r] |= (tmp << 7);\
    gb->flags = (!gb->r[r]) * FLAG_Z;\
    gb->f_c = tmp & 1;\
}

#define RRCHL   OP(RRCHL); {\
    uint8_t tmp = hl;\
    hl >>= 1; hl |= (tmp << 7);\
    gb->flags = (!hl) * FLAG_Z;\
    gb->f_c = tmp & 1;\
}

#define RR      OP(RR); {\
    uint8_t tmp = gb->r[r];\
    gb->r[r] >>= 1; gb->r[r] |= gb->f_c << 7;\
    gb->flags = (!gb->r[r]) * FLAG_Z;\
    gb->f_c = tmp & 1;\
}

#define RRHL    OP(RRHL); {\
    uint8_t tmp = hl;\
    hl >>= 1; hl |= gb->f_c << 7;\
    gb->flags = (!hl) * FLAG_Z;\
    gb->f_c = tmp & 1;\
}

#define SLA     OP(SLA);   gb->flags = (gb->r[r] >> 7) * FLAG_C; gb->r[r] <<= 1; SET_FLAG_Z(gb->r[r]);
#define SRA     OP(SRA);   gb->flags = (gb->r[r] & 1)  * FLAG_C; gb->r[r] = (gb->r[r] >> 1) | (gb->r[r] & 0X80); SET_FLAG_Z(gb->r[r]);
#define SLAHL   OP(SLAHL); gb->flags = (hl >> 7) * FLAG_C; hl <<= 1; SET_FLAG_Z(hl);
#define SRAHL   OP(SRAHL); gb->flags = (hl & 1)  * FLAG_C; hl = (hl >> 1) | (hl & 0X80); SET_FLAG_Z(hl);

#define SWAP    OP(SWAP)   { uint8_t tmp = gb->r[r] << 4; gb->r[r] >>= 4; gb->r[r] |= tmp; gb->flags = 0; SET_FLAG_Z (gb->r[r]); }
#define SWAPHL  OP(SWAPHL) { uint8_t tmp = hl << 4; hl >>= 4; hl |= tmp; gb->flags = 0; SET_FLAG_Z(hl); }

#define SRL     OP(SRL);   { uint8_t fc = (gb->r[r] & 1) ? 1 : 0; gb->r[r] >>= 1; gb->flags = 0; SET_FLAG_Z (gb->r[r]); gb->f_c = fc; }
#define SRLHL   OP(SRLHL); { uint8_t fc = (hl & 1)       ? 1 : 0; hl >>= 1;       gb->flags = 0; SET_FLAG_Z (hl);       gb->f_c = fc; }

#define BIT     OP(BIT);   KEEP_CARRY; gb->flags |= FLAG_H; gb->flags |= (gb->r[r] & (1 << r_bit)) ? 0 : FLAG_Z;
#define BITHL   OP(BITHL); KEEP_CARRY; gb->flags |= FLAG_H; gb->flags |= (hl & (1 << r_bit)) ? 0 : FLAG_Z;

#define RES     OP(RES);   gb->r[r] &= (0xFE << r_bit) | (0xFF >> (8 - r_bit));
#define RESHL   OP(RESHL); hl &= (0xFE << r_bit) | (0xFF >> (8 - r_bit)); CPU_WB (ADDR_HL ,hl);

#define SET     OP(SET);   gb->r[r] |= (1 << r_bit); 
#define SETHL   OP(SET);   hl |= (1 << r_bit); CPU_WB (ADDR_HL ,hl);

/* Misc instructions */

#define PREFIX    OP(PREFIX)   { uint8_t cb = CPU_RB (gb->pc++); gb_exec_cb (gb, cb); }
#define INVALID   OP(INVALID); gb->invalid = 1;