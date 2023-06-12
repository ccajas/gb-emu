/* Instruction helpers */

#define ADDR_HL       ((gb->r[H] << 8) + gb->r[L])
#define ADDR_XY(X,Y)  ((X << 8) + Y)

#define CPU_RB(A)     gb_mem_access (gb, A, 0, 0)
#define CPU_WB(A,X)   gb_mem_access (gb, A, (X), 1)
#define CPU_RB_PC     CPU_RB (gb->pc++)
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

/** Choose between operations based on position */

#define OPR_2_(A, B)  if ((op & 7) != 6) { A } else { B }

#define OP_R8_G(op_) \
    case op_:\
    case op_ + 1:\
    case op_ + 2:\
    case op_ + 3:\
    case op_ + 4:\
    case op_ + 5:\
    case op_ + 7:\

/** 8-bit load instructions **/   

#define LD_r8     OP(LD_r8);    if (*reg1 != *reg2) { *reg1 = *reg2; }
#define LDrm      OP(LDrm);     *reg1 = CPU_RB_PC;
#define LD_rHL    OP(LD_rHL);   *reg1 = CPU_RB (ADDR_HL);

#define LD_HLr    OP(LDHLr);    CPU_WB (ADDR_HL, *reg2);
#define LDHLm     OP(LDHLm);    CPU_WB (ADDR_HL, CPU_RB_PC);

#define LDAmm     OP(LDAmm);    gb->r[A] = CPU_RB (CPU_RW (gb->pc)); gb->pc += 2;
#define LDmmA     OP(LDmmA);    CPU_WB (CPU_RW (gb->pc), gb->r[A]);  gb->pc += 2;

#define LDIOmA    OP(LDIOmA);   CPU_WB (0xFF00 + CPU_RB_PC, gb->r[A]);
#define LDAIOm    OP(LDAIOm);   gb->r[A] = CPU_RB (0xFF00 + CPU_RB_PC);
#define LDIOCA    OP(LDIOCA);   CPU_WB (0xFF00 + gb->r[C], gb->r[A]);
#define LDAIOC    OP(LDAIOC);   gb->r[A] = CPU_RB (0xFF00 + gb->r[C]); 

    /* Increment instruction templates */
    #define INCR_HL   gb->r[L]++; if (!gb->r[L]) gb->r[H]++
    #define DECR_HL   gb->r[L]--; if (gb->r[L] == 255) gb->r[H]--

#define LDrrmA    OP(LDrrmA);   CPU_WB (ADDR_XY (*reg1, *(reg1+1)), gb->r[A]);
#define LDHLIA    OP(LDHLIA);   CPU_WB (ADDR_HL, gb->r[A]); INCR_HL;
#define LDHLDA    OP(LDHLDA);   CPU_WB (ADDR_HL, gb->r[A]); DECR_HL;

#define LDArrm    OP(LDArrm);   gb->r[A] = CPU_RB (ADDR_XY(*(reg1-1), *reg1));
#define LDAHLI    OP(LDAHLI);   gb->r[A] = CPU_RB (ADDR_HL); INCR_HL;
#define LDAHLD    OP(LDAHLD);   gb->r[A] = CPU_RB (ADDR_HL); DECR_HL;

/** 16-bit load instructions **/

#define LDmSP     OP(LDmSP);  CPU_WW (CPU_RW (gb->pc), gb->sp); gb->pc += 2;
#define LDSP      OP(LDSP);   gb->sp = CPU_RW (gb->pc); gb->pc += 2;
#define LDSPHL    OP(LDSPHL); gb->sp = ADDR_HL;

#define LDrr      OP(LDrr)\
    if (opHh == 6) gb->sp = CPU_RW (gb->pc);\
    else { *(reg1+1) = CPU_RB (gb->pc); *reg1 = CPU_RB (gb->pc + 1); }\
    gb->pc += 2;\

#define PUSH_(X, Y)                 gb->sp--; CPU_WB (gb->sp, X); gb->sp--; CPU_WB (gb->sp, Y);
#define PUSHrr_(op_, R16_1, R16_2)  OP(PUSHrr); case op_: PUSH_(R16_1, R16_2); break;

#define POPrr_(op_, R16_1, R16_2)   OP(POPrr); case op_:\
    R16_2 = CPU_RB (gb->sp++) & ((op_ == 0xF1) ? 0xF0 : 0xFF); R16_1 = CPU_RB (gb->sp++); break;\

#define R16_OPS_3(OP_, START) \
    OP_ (START, (gb->r[B]), (gb->r[C]))\
    OP_ (START + 0x10, (gb->r[D]), (gb->r[E]))\
    OP_ (START + 0x20, (gb->r[H]), (gb->r[L]))\
    OP_ (START + 0x30, (gb->r[A]), (gb->flags))\

/* Flag setting helpers */

#define FLAG_Z   0x80
#define FLAG_N   0x40
#define FLAG_H   0x20
#define FLAG_C   0x10

#define SET_FLAG_Z(X)    gb->f_z = ((X) == 0);
#define SET_FLAG_H_S(X)  gb->f_h = ((X) & 0x10) > 0;
#define SET_FLAG_H(X)    gb->f_h = (X);
#define SET_FLAG_C(X)    gb->f_c = (X);

#define SET_FLAGS(mask, z,n,h,c)\
    gb->flags = (gb->flags & mask) | ((!z << 7) + (n << 6) + (h << 5) + (c << 4));

#define KEEP_CARRY      gb->flags &= FLAG_C;

/** 8-bit arithmetic/logic instructions **/

    /* Add function templates */
    #define FLAGS_ADD_(X)  SET_FLAGS (0,\
        (tmp & 0xFF), 0,(((gb->r[A] ^ X ^ tmp) & 0x10) > 0),(tmp >= 0x100));\

    #define FLAGS_SUB_(X)  SET_FLAGS (0,\
        (tmp & 0xFF), 1,(((gb->r[A] ^ X ^ tmp) & 0x10) > 0),((tmp & 0xFF00) ? 1 : 0));\

    #define FLAGS_CP       SET_FLAGS (0, tmp, 1, ((tmp & 0xF) > (gb->r[A] & 0xF)), (tmp > gb->r[A]));

    #define ADC_A_(X, C)   uint16_t tmp = gb->r[A] + X + C; FLAGS_ADD_(X); gb->r[A] = tmp & 0xFF;
    #define SBC_A_(X, C)   uint16_t tmp = gb->r[A] - X - C; FLAGS_SUB_(X); gb->r[A] = tmp & 0xFF;

    /* Add and subtract */

#define ADD_A_r8    OP(ADD_r8)  ADC_A_(*reg2, 0)
#define ADD_A_HL    OP(ADD_HL)  uint8_t hl = CPU_RB (ADDR_HL); ADC_A_(hl, 0)
#define ADC_A_r8    OP(ADC_r8)  ADC_A_(*reg2, gb->f_c)
#define ADC_A_HL    OP(ADC_HL)  uint8_t hl = CPU_RB (ADDR_HL); ADC_A_(hl, gb->f_c)

#define SUB_A_r8    OP(SUB_r8)  SBC_A_(*reg2, 0)
#define SUB_A_HL    OP(SUB_HL)  uint8_t hl = CPU_RB (ADDR_HL); SBC_A_(hl, 0)
#define SBC_A_r8    OP(SBC_r8)  SBC_A_(*reg2, gb->f_c)
#define SBC_A_HL    OP(SBC_HL)  uint8_t hl = CPU_RB (ADDR_HL); SBC_A_(hl, gb->f_c)

/* Bitwise logic */

    /* Bitwise logic templates */
    #define FLAGS_AND_(X)    gb->flags = 0; SET_FLAG_Z (gb->r[A] &= X); SET_FLAG_H (1);
    #define FLAGS_XOR_(X)    gb->flags = 0; SET_FLAG_Z (gb->r[A] ^= X);
    #define FLAGS_OR_(X)     gb->flags = 0; SET_FLAG_Z (gb->r[A] |= X);

#define AND_A_r8    OP(AND_r8)    FLAGS_AND_(*reg2)
#define AND_A_HL    OP(AND_HL)    FLAGS_AND_(CPU_RB (ADDR_HL))
#define XOR_A_r8    OP(AND_r8)    FLAGS_XOR_(*reg2)
#define XOR_A_HL    OP(AND_HL)    FLAGS_XOR_(CPU_RB (ADDR_HL))
#define OR_A_r8     OP(AND_r8)    FLAGS_OR_(*reg2)
#define OR_A_HL     OP(AND_HL)    FLAGS_OR_(CPU_RB (ADDR_HL))
#define CP_A_r8     OP(CP_r8)     tmp -= *reg2;  FLAGS_CP;
#define CP_A_HL     OP(CP_HL)     tmp -= CPU_RB (ADDR_HL); FLAGS_CP;

#define ADDm     OP(ADDm)     { uint8_t m = CPU_RB_PC; ADC_A_(m, 0); }
#define ADCm     OP(ADCm)     { uint8_t m = CPU_RB_PC; ADC_A_(m, gb->f_c); }

#define SUBm     OP(SUBm)     { uint8_t m = CPU_RB_PC; SBC_A_(m, 0); }
#define SBCm     OP(SBCm)     { uint8_t m = CPU_RB_PC; SBC_A_(m, gb->f_c); }

#define ANDm     OP(ANDm)     FLAGS_AND_(CPU_RB_PC);
#define XORm     OP(XORm)     FLAGS_XOR_(CPU_RB_PC);
#define ORm      OP(ORm)      FLAGS_OR_(CPU_RB_PC);
#define CPm      OP(CPm)      tmp -= CPU_RB_PC; FLAGS_CP;

#define INC(X)   OP(INC)      SET_FLAGS (16, X, 0, ((X & 0xF) == 0),   0);
#define DEC(X)   OP(DEC)      SET_FLAGS (16, X, 1, ((X & 0xF) == 0xF), 0);
#define CPL      OP(CPL)      gb->r[A] ^= 0xFF; gb->flags |= 0x60;

#define INC_     OP(INC)  if (opHh == 0x6) {\
    uint8_t tmp = CPU_RB (ADDR_HL); CPU_WB (ADDR_HL, ++tmp); INC(tmp) } else { (*reg1)++; INC(*reg1); }

#define DEC_     OP(DEC)  if (opHh == 0x6) {\
    uint8_t tmp = CPU_RB (ADDR_HL); CPU_WB (ADDR_HL, --tmp); DEC(tmp) } else { (*reg1)--; DEC(*reg1); }

#define LDrm_    if (op == 0x36) { LDHLm } else { LDrm }

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

/** 16-bit arithmetic instructions **/

    /* Flag templates for Add HL instructions */
    #define FLAGS_ADHL  gb->f_n = 0;   gb->f_h = ((hl & 0xfff) > (tmp & 0xfff));      gb->f_c = (hl > tmp) ? 1 : 0;
    #define FLAGS_SPm   gb->flags = 0; gb->f_h = ((gb->sp & 0xF) + (i & 0xF) > 0xF); gb->f_c = ((gb->sp & 0xFF) + (i & 0xFF) > 0xFF);

#define ADHLrr   OP(ADHLrr);  {\
    uint16_t r16 = (opHh == 7) ? gb->sp : ADDR_XY(*(reg1-1), *reg1);\
    uint16_t hl = ADDR_HL; uint16_t tmp = hl + r16; FLAGS_ADHL;\
    gb->r[H] = (tmp >> 8); gb->r[L] = tmp & 0xFF;\
}

#define ADDSPm   OP(ADDSPm);  { int8_t i = (int8_t) CPU_RB_PC; FLAGS_SPm; gb->sp += i; }
#define LDHLSP   OP(LDHLSP);  { int8_t i = (int8_t) CPU_RB_PC;\
    gb->r[H] = ((gb->sp + i) >> 8); gb->r[L] = (gb->sp + i) & 0xFF;\
    gb->flags = 0;\
    gb->f_h = ((gb->sp & 0xF) + (i & 0xF) > 0xF);\
    gb->f_c = ((gb->sp & 0xFF) + (i & 0xFF) > 0xFF);\
}

#define INCrr    OP(INCrr);   if (opHh == 6) gb->sp++; else { *(reg1 + 1) += 1; if (!*(reg1 + 1)) (*reg1)++; }
#define DECrr    OP(DECrr);   if (opHh == 7) gb->sp--; else { (*reg1)--; if (*reg1 == 0xff) *(reg1 - 1) -= 1; }

/** CPU control instructions **/

#define CCF     OP(CCF);  gb->f_c = !gb->f_c; gb->f_n = gb->f_h = 0;
#define SCF     OP(SCF);  gb->f_c = 1; gb->f_n = gb->f_h = 0;
#define HALT    OP(HALT); if (gb->ime) gb->halted = 1; 
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
    #define JP_IF(X) \
    uint16_t mm = CPU_RW (gb->pc); if (X) {\
        gb->pc = mm; gb->rm++;\
    } else gb->pc += 2;

    #define JR_IF(X) \
    int8_t e = (int8_t) CPU_RB (gb->pc); if (X) {\
        gb->pc += e; gb->rm++;\
    } gb->pc++;

    #define CALL_IF(X) \
    uint16_t mm = CPU_RW (gb->pc); if (X) {\
        PUSH_((gb->pc + 2) >> 8, (gb->pc + 2) & 0xFF);\
        gb->pc = mm; gb->rm += 3;\
    } else gb->pc += 2;

    /* Return function templates */
    #define RET__     gb->pc = CPU_RW (gb->sp); gb->sp += 2;
    #define RET_IF(X) if (X) { RET__; gb->rm += 3; }

/* Conditional jump */

/* Conditional relative jump */

/* Calls */
#define CALLm   OP(CALLm);  {\
    uint16_t tmp = CPU_RW (gb->pc); gb->pc += 2; gb->sp -= 2; CPU_WW(gb->sp, gb->pc); gb->pc = tmp; }\

#define RET     OP(RET);   RET__;
#define RETI    OP(RETI);  RET__; gb->ime = 1;

#define COND_(_)\
    (_ == 0) ? (!gb->f_z) :\
    (_ == 1) ?   gb->f_z  :\
    (_ == 2) ? (!gb->f_c) : gb->f_c\

#define JR_(C)    OP(JR_)     { JR_IF   (COND_(C)); }
#define RET_(C)   OP(RET_)    { RET_IF  (COND_(C)); }
#define JP_(C)    OP(JP_)     { JP_IF   (COND_(C)); }
#define CALL_(C)  OP(CALL_)   { CALL_IF (COND_(C)); }

#define RST     OP(RST);   uint16_t n = (opHh - 0x18) << 3; gb->sp -= 2; CPU_WW (gb->sp, gb->pc); gb->pc = n;

/* Rotate and shift instructions */

#define RLA   	OP(RLA);   { uint8_t tmp = gb->r[A]; gb->r[A] = (gb->r[A] << 1) | (gb->f_c);      gb->flags = ((tmp >> 7) & 1) * FLAG_C; }
#define RRA     OP(RRA);   { uint8_t tmp = gb->r[A]; gb->r[A] = (gb->r[A] >> 1) | (gb->f_c << 7); gb->flags = (tmp & 1) * FLAG_C; }
#define RLCA    OP(RLCA);  gb->r[A] = (gb->r[A] << 1) | (gb->r[A] >> 7); gb->flags = (gb->r[A] & 1) * FLAG_C; 
#define RRCA    OP(RRCA);  gb->flags = (gb->r[A] & 1) * FLAG_C; gb->r[A] = (gb->r[A] >> 1) | (gb->r[A] << 7); 

#define RLC     OP(RLC); {\
    uint8_t tmp = *reg1;\
    *reg1 <<= 1; *reg1 |= (tmp >> 7);\
    SET_FLAGS(0, *reg1, 0, 0, (tmp >> 7));\
}

#define RLCHL   OP(RLCHL); {\
    uint8_t tmp = hl; hl <<= 1; hl |= (tmp >> 7);\
    SET_FLAGS(0, hl, 0, 0, (tmp >> 7));\
}

#define RL      OP(RL); {\
    uint8_t tmp = *reg1;\
    *reg1 <<= 1; *reg1 |= gb->f_c;\
    SET_FLAGS(0, *reg1, 0, 0, (tmp >> 7));\
}

#define RLHL    OP(RLHL); {\
    uint8_t tmp = hl; hl <<= 1; hl |= gb->f_c;\
    SET_FLAGS(0, hl, 0, 0, (tmp >> 7));\
}

#define RRC     OP(RRC); {\
    uint8_t tmp = *reg1;\
    *reg1 >>= 1; *reg1 |= (tmp << 7);\
    SET_FLAGS(0, *reg1, 0, 0, (tmp & 1));\
}

#define RRCHL   OP(RRCHL); {\
    uint8_t tmp = hl; hl >>= 1; hl |= (tmp << 7);\
    SET_FLAGS(0, hl, 0, 0, (tmp & 1));\
}

#define RR      OP(RR); {\
    uint8_t tmp = *reg1;\
    *reg1 >>= 1; *reg1 |= gb->f_c << 7;\
    SET_FLAGS(0, *reg1, 0, 0, (tmp & 1));\
}

#define RRHL    OP(RRHL); {\
    uint8_t tmp = hl; hl >>= 1; hl |= gb->f_c << 7;\
    SET_FLAGS(0, hl, 0, 0, (tmp & 1));\
}

#define SLA     OP(SLA);   gb->flags = 0; SET_FLAG_C (*reg1 >> 7); *reg1 <<= 1; SET_FLAG_Z (*reg1);
#define SRA     OP(SRA);   gb->flags = 0; SET_FLAG_C (*reg1 & 1); *reg1 = (*reg1 >> 1) | (*reg1 & 0X80); SET_FLAG_Z(*reg1);
#define SLAHL   OP(SLAHL); gb->flags = 0; SET_FLAG_C (hl >> 7); hl <<= 1; SET_FLAG_Z(hl);
#define SRAHL   OP(SRAHL); gb->flags = 0; SET_FLAG_C (hl & 1); hl = (hl >> 1) | (hl & 0X80); SET_FLAG_Z(hl);

#define SWAP    OP(SWAP) {\
    uint8_t tmp = *reg1 << 4;\
    *reg1 >>= 4; *reg1 |= tmp; SET_FLAGS(0, *reg1, 0, 0, 0);\
}

#define SWAPHL  OP(SWAPHL) {\
    uint8_t tmp = hl << 4;\
    hl >>= 4; hl |= tmp; SET_FLAGS(0, hl, 0, 0, 0);\
}

#define SRL     OP(SRL); {\
    uint8_t fc = (*reg1 & 1) ? 1 : 0;\
    *reg1 >>= 1; SET_FLAGS(0, *reg1, 0, 0, fc);\
}

#define SRLHL   OP(SRLHL); {\
    uint8_t fc = (hl & 1) ? 1 : 0;\
    hl >>= 1; SET_FLAGS(0, hl, 0, 0, fc);\
}

#define BIT     OP(BIT);   SET_FLAGS(16, (*reg1 & (1 << r_bit)), 0, 1, 0);
#define BITHL   OP(BITHL); SET_FLAGS(16, (hl & (1 << r_bit)), 0, 1, 0); gb->rm += 1;

#define RES     OP(RES);   *reg1 &= (0xFE << r_bit) | (0xFF >> (8 - r_bit));
#define RESHL   OP(RESHL); hl &= (0xFE << r_bit) | (0xFF >> (8 - r_bit)); gb->rm += 2;

#define SET     OP(SET);   *reg1 |= (1 << r_bit); 
#define SETHL   OP(SET);   hl |= (1 << r_bit); gb->rm += 2;

/* Misc instructions */

#define PREFIX    OP(PREFIX)   /*{ uint8_t cb = CPU_RB (gb->pc++); gb_exec_cb (gb, cb); }*/
#define INVALID   OP(INVALID); gb->invalid = 1;