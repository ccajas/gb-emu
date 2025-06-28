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

#define USE_INC_MCYCLE

#ifdef USE_INC_MCYCLE
#define INC_MCYCLE  ++gb->rm
#else
#define INC_MCYCLE
#endif

/** CPU bus read/write **/

#define CPU_RB(A)     gb_mem_read (gb, A)
#define CPU_WB(A,X)   gb_mem_write (gb, A, (X))
#define CPU_RB_PC     gb_mem_read (gb, gb->pc)
#define CPU_RW(A)     ( CPU_RB (A) +  (CPU_RB (A + 1) << 8) )
#define CPU_WW(A,X)   { CPU_WB (A, X); CPU_WB (A + 1, (X >> 8)); }

/** Choose between operations based on position **/

#define OPR_2_(op_, name) \
    case op_:     name (REG_B) break;\
    case op_ + 1: name (REG_C) break;\
    case op_ + 2: name (REG_D) break;\
    case op_ + 3: name (REG_E) break;\
    case op_ + 4: name (REG_H) break;\
    case op_ + 5: name (REG_L) break;\
    case op_ + 6: hl = CPU_RB(REG_HL); name (hl) break;\
    case op_ + 7: name (REG_A) break;\

#define OPR_2R_(op_, name) \
    OPR_2_(op_, name)\
    OPR_2_(op_ + 0x8,  name)\
    OPR_2_(op_ + 0x10, name)\
    OPR_2_(op_ + 0x18, name)\
    OPR_2_(op_ + 0x20, name)\
    OPR_2_(op_ + 0x28, name)\
    OPR_2_(op_ + 0x30, name)\
    OPR_2_(op_ + 0x38, name)\

#define OP_r8(op_, name, R)\
    case op_:     name ##_r8(R, REG_B) break;\
    case op_ + 1: name ##_r8(R, REG_C) break;\
    case op_ + 2: name ##_r8(R, REG_D) break;\
    case op_ + 3: name ##_r8(R, REG_E) break;\
    case op_ + 4: name ##_r8(R, REG_H) break;\
    case op_ + 5: name ##_r8(R, REG_L) break;\
    case op_ + 7: name ##_r8(R, REG_A) break;\

#define OP_r8_hl(op_, name, R)\
    OP_r8(op_, name, R);\
    case op_ + 6: name ##_r8(R, CPU_RB(REG_HL)); break;\

#define OP_r8_g(op_, name, R)\
    case op_:        name ##_r8(REG_B, R); break;\
    case op_ + 0x8:  name ##_r8(REG_C, R); break;\
    case op_ + 0x10: name ##_r8(REG_D, R); break;\
    case op_ + 0x18: name ##_r8(REG_E, R); break;\
    case op_ + 0x20: name ##_r8(REG_H, R); break;\
    case op_ + 0x28: name ##_r8(REG_L, R); break;\
    case op_ + 0x30: name ##_hl(REG_A, R); break;\
    case op_ + 0x38: name ##_r8(REG_A, R); break;\

#define OP_r16_g(op_, _r16)\
    case op_: _r16        (REG_BC) break;\
    case op_ + 0x10: _r16 (REG_DE) break;\
    case op_ + 0x20: _r16 (REG_HL) break;\

#define OP_r16_g1(op_, _r16)\
    OP_r16_g(op_, _r16)\
    case op_ + 0x30: _r16 (gb->sp) break;\

#define OP_r16_g2(op_, _r16)\
    OP_r16_g(op_, _r16)\
    case op_ + 0x30: _r16 (REG_HL) break;\

#define OP_r16_g3(op_, _r16)\
    case op_: _r16        (op_,        REG_B, REG_C)       break;\
    case op_ + 0x10: _r16 (op_ + 0x10, REG_D, REG_E)       break;\
    case op_ + 0x20: _r16 (op_ + 0x20, REG_H, REG_L)       break;\
    case op_ + 0x30: _r16 (op_ + 0x30, REG_A, (gb->flags)) break;\

/** 8-bit load instructions **/

#define LD_r8(dest, src)     OP(LD_r8) dest = src;
#define LD_HL_r8(_, src)     OP(LD_r8) CPU_WB(REG_HL, src);

#define LD_OPS\
    OP_r8_hl(0x40, LD, REG_B)\
    OP_r8_hl(0x48, LD, REG_C)\
    OP_r8_hl(0x50, LD, REG_D)\
    OP_r8_hl(0x58, LD, REG_E)\
    OP_r8_hl(0x60, LD, REG_H)\
    OP_r8_hl(0x68, LD, REG_L)\
    OP_r8_hl(0x78, LD, REG_A)\

#define LDrm_r8(r8, _)   OP(LD m)  r8 = CPU_RB_PC; ++gb->pc;
#define LDrm_hl(r8, _)   OP(LD m)  CPU_WB (REG_HL, CPU_RB_PC); ++gb->pc;

#define LDAmm   OP(LDAmm)   REG_A = CPU_RB (CPU_RW (gb->pc)); ++gb->pc; ++gb->pc;
#define LDmmA   OP(LDmmA)   CPU_WB (CPU_RW (gb->pc), REG_A);  ++gb->pc; ++gb->pc;

#define LDHmA   OP(LDH mA)  CPU_WB (0xFF00 + CPU_RB_PC, REG_A); ++gb->pc;
#define LDHCA   OP(LDH CA)  CPU_WB (0xFF00 + REG_C, REG_A);
#define LDHAm   OP(LDH Am)  REG_A = CPU_RB (0xFF00 + CPU_RB_PC); ++gb->pc;
#define LDHAC   OP(LDH AC)  REG_A = CPU_RB (0xFF00 + REG_C); 

#define LDrrmA(r16)  OP(LDrrmA)\
    CPU_WB (r16, REG_A);  REG_HL += (op > 0x30) ? -1 : (op > 0x20) ? 1 : 0;

#define LDArrm(r16)  OP(LDArrm)\
    REG_A = CPU_RB (r16); REG_HL += (op > 0x30) ? -1 : (op > 0x20) ? 1 : 0;

/** 16-bit load instructions **/

#define LDmSP           OP(LDmSP)   { gb->nn = CPU_RW (gb->pc); CPU_WW (gb->nn, gb->sp); ++gb->pc; ++gb->pc; }
#define LDSP            OP(LDSP)    gb->sp = CPU_RW (gb->pc); ++gb->pc; ++gb->pc;
#define LDSPHL          OP(LDSPHL)  gb->sp = REG_HL; INC_MCYCLE;
#define LDrr(r16)       OP(LDrr)    r16 = CPU_RW (gb->pc); ++gb->pc; ++gb->pc;

#define PUSH_(X, Y)     gb->sp--; INC_MCYCLE; CPU_WB (gb->sp, X); gb->sp--; CPU_WB (gb->sp, Y);

#define PUSHrr(op_, R16_1, R16_2)  OP(PUSHrr) PUSH_(R16_1, R16_2);
#define POPrr(op_, R16_1, R16_2)   OP(POPrr)\
    R16_2 = CPU_RB (gb->sp++) & ((op_ == 0xF1) ? 0xF0 : 0xFF); R16_1 = CPU_RB (gb->sp++);

/* Flag setting helpers */

#define SET_FLAG_Z(X)    gb->f_z = ((X) == 0)
#define SET_FLAG_H_S(X)  gb->f_h = ((X) & 0x10) > 0
#define SET_FLAG_H(X)    gb->f_h = (X)
#define SET_FLAG_C(X)    gb->f_c = (X)

#define SET_FLAGS(mask, z,n,h,c)\
    gb->flags = (gb->flags & mask) | ((!z << 7) | (n << 6) | (h << 5) | (c << 4))

/** 8-bit arithmetic/logic instructions **/

    /* Add function templates */
    #define FLAGS_ALU_(X, N)  SET_FLAGS (0,\
        (gb->nn & 0xFF), N, (((REG_A ^ X ^ gb->nn) & 0x10) > 0), (gb->nn >= 0x100))

    #define ADC_A_(X, C)   const uint8_t val = X; gb->nn = REG_A + val + C; FLAGS_ALU_(val, 0); REG_A = gb->nn & 0xFF;
    #define SBC_A_(X, C)   const uint8_t val = X; gb->nn = REG_A - val - C; FLAGS_ALU_(val, 1); REG_A = gb->nn & 0xFF;

    /* Add and subtract */

#define ADD_r8(_, r8)  OP(ADD)  { ADC_A_(r8, 0) }
#define ADC_r8(_, r8)  OP(ADC)  { ADC_A_(r8, gb->f_c) }

#define SUB_r8(_, r8)  OP(SUB)  { SBC_A_(r8, 0) }
#define SBC_r8(_, r8)  OP(SBC)  { SBC_A_(r8, gb->f_c) }

/* Bitwise logic */

    /* Bitwise logic templates */
    #define FLAGS_AND_(X)    gb->flags = 0; SET_FLAG_Z (REG_A &= X); SET_FLAG_H (1);
    #define FLAGS_XOR_(X)    gb->flags = 0; SET_FLAG_Z (REG_A ^= X);
    #define FLAGS_OR_(X)     gb->flags = 0; SET_FLAG_Z (REG_A |= X);
    #define FLAGS_CP         SET_FLAGS (0, tmp, 1, ((tmp & 0xF) > (REG_A & 0xF)), (tmp > REG_A));

#define AND_r8(_, r8)   OP(AND)  FLAGS_AND_(r8);
#define XOR_r8(_, r8)   OP(XOR)  FLAGS_XOR_(r8);
#define OR_r8(_, r8)    OP(OR)   FLAGS_OR_(r8);
#define CP_r8(_, r8)    OP(CP)   tmp -= r8; FLAGS_CP;

#define ADDm        OP(ADDm)     { uint8_t m = CPU_RB_PC; ++gb->pc; ADC_A_(m, 0); }
#define ADCm        OP(ADCm)     { uint8_t m = CPU_RB_PC; ++gb->pc; ADC_A_(m, gb->f_c); }

#define SUBm        OP(SUBm)     { uint8_t m = CPU_RB_PC; ++gb->pc; SBC_A_(m, 0); }
#define SBCm        OP(SBCm)     { uint8_t m = CPU_RB_PC; ++gb->pc; SBC_A_(m, gb->f_c); }

#define ANDm        OP(ANDm)     FLAGS_AND_(CPU_RB_PC); ++gb->pc;
#define XORm        OP(XORm)     FLAGS_XOR_(CPU_RB_PC); ++gb->pc;
#define ORm         OP(ORm)      FLAGS_OR_(CPU_RB_PC); ++gb->pc;
#define CPm         OP(CPm)      tmp -= CPU_RB_PC; ++gb->pc; FLAGS_CP;

#define INC(X)      OP(INC)      SET_FLAGS (16, X, 0, ((X & 0xF) == 0),   0);
#define DEC(X)      OP(DEC)      SET_FLAGS (16, X, 1, ((X & 0xF) == 0xF), 0);

#define CCF         OP(CCF)      gb->f_c = !gb->f_c; gb->f_n = gb->f_h = 0;
#define SCF         OP(SCF)      gb->f_c = 1;        gb->f_n = gb->f_h = 0;
#define CPL         OP(CPL)      REG_A ^= 0xFF; gb->flags |= 0x60;

#define INC_r8(reg, _)  OP(INC)  reg++; INC(reg)
#define DEC_r8(reg, _)  OP(DEC)  reg--; DEC(reg)
#define INC_hl(reg, _)  OP(INC)  CPU_WB (REG_HL, tmp = CPU_RB (REG_HL) + 1); INC(tmp)
#define DEC_hl(reg, _)  OP(DEC)  CPU_WB (REG_HL, tmp = CPU_RB (REG_HL) - 1); DEC(tmp)

/* The following is from SameBoy. MIT License. */
#define DAA      OP(DAA);     {\
    int16_t a = REG_A;\
    if (gb->f_n) {\
        if (gb->f_h) a = (a - 0x06) & 0xFF;\
        if (gb->f_c) a -= 0x60;\
    } else {\
        if (gb->f_h || (a & 0x0F) > 9) a += 0x06;\
        if (gb->f_c || a > 0x9F) a += 0x60;\
    }\
    REG_A = a;\
    if ((a & 0x100) == 0x100) SET_FLAG_C (1);\
    SET_FLAG_Z (REG_A);\
    SET_FLAG_H (0);\
}

/** 16-bit arithmetic instructions **/

    /* Flag templates for Add HL instructions */
    #define FLAGS_ADHL  gb->f_n = 0;   gb->f_h = ((REG_HL & 0xfff) > (gb->nn & 0xfff)); gb->f_c = (REG_HL > gb->nn) ? 1 : 0;
    #define FLAGS_SPm   gb->flags = 0; gb->f_h = ((gb->sp & 0xF) + (i & 0xF) > 0xF); gb->f_c = ((gb->sp & 0xFF) + (i & 0xFF) > 0xFF);

#define ADHLrr(r16)   OP(ADHLrr); {\
    gb->nn = REG_HL + r16; INC_MCYCLE; FLAGS_ADHL; REG_HL = gb->nn;\
}

#define ADDSPm   OP(ADDSPm)   {\
    const int8_t i = (int8_t) CPU_RB_PC; ++gb->pc; INC_MCYCLE; FLAGS_SPm; INC_MCYCLE; gb->sp += i; }

#define LDHLSP   OP(LDHLSP)   { const int8_t i = (int8_t) CPU_RB_PC;\
    ++gb->pc; INC_MCYCLE; gb->flags = 0;\
    gb->f_h = ((gb->sp & 0xF) + (i & 0xF) > 0xF);\
    gb->f_c = ((gb->sp & 0xFF) + (i & 0xFF) > 0xFF);\
    REG_H = ((gb->sp + i) >> 8); REG_L = (gb->sp + i) & 0xFF;\
}

#define INCrr(r16)    OP(INCrr); INC_MCYCLE; r16++;
#define DECrr(r16)    OP(DECrr); INC_MCYCLE; r16--;

/** CPU control instructions **/

#define STOP    OP(STOP)  gb->stopped = 1; /* STOP is handled after switch/case */
#define NOP     OP(NOP)
#define DI      OP(DI)    gb->imePending = 0; gb->ime = 0; 
#define EI      OP(EI)    gb->ime = 1;//gb->imePending = 1; 

/** Jump and call instructions **/

/* Jump to | relative jump */
#define JPNN    OP(JPNN)  gb->pc = CPU_RW (gb->pc); INC_MCYCLE;
#define JPHL    OP(JPHL)  gb->pc = REG_HL; 
#define JRm     OP(JRm)   const int8_t inc = (int8_t) CPU_RB (gb->pc);\
    gb->pc += inc; if(!gb->pcInc) { gb->pc--; gb->pcInc = 1; } ++gb->pc; INC_MCYCLE;\

/* Calls */
#define CALLm   OP(CALLm);  {\
    gb->nn = CPU_RW (gb->pc); ++gb->pc; ++gb->pc; gb->sp -= 2;\
    CPU_WW(gb->sp, gb->pc); gb->pc = gb->nn; INC_MCYCLE; }\

    /* Return function template */
    #define RET__     gb->pc = CPU_RW (gb->sp); INC_MCYCLE; gb->sp += 2; 

#define RET     OP(RET)    RET__
#define RETI    OP(RETI)   RET__ gb->ime = 1;

    /* Conditional function templates */
    #define JP_IF(X) \
        gb->nn = CPU_RW (gb->pc); ++gb->pc; ++gb->pc;\
        if (X) { gb->pc = gb->nn ; INC_MCYCLE; }\

    #define JR_IF(X) \
        int8_t e = (int8_t) CPU_RB (gb->pc); ++gb->pc;\
        if (X) { if (!gb->pcInc) { e--; gb->pcInc = 1; } gb->pc += e; INC_MCYCLE; }\

    #define CALL_IF(X) \
        gb->nn = CPU_RW (gb->pc); ++gb->pc; ++gb->pc;\
        if (X) { PUSH_(gb->pc >> 8, gb->pc & 0xFF);\
            gb->pc = gb->nn; }\

    #define RET_IF(X) INC_MCYCLE; if (X) { RET__ }

/* Conditional jump, relative jump, return, call */

#define COND_(_)\
    (_ == 0) ? (!gb->f_z) :\
    (_ == 1) ?   gb->f_z  :\
    (_ == 2) ? (!gb->f_c) : gb->f_c\

#define JR_(C)    OP(JR_)     { JR_IF   (COND_(C)) }
#define RET_(C)   OP(RET_)    { RET_IF  (COND_(C)) }
#define JP_(C)    OP(JP_)     { JP_IF   (COND_(C)) }
#define CALL_(C)  OP(CALL_)   { CALL_IF (COND_(C)) }

#define RST       OP(RST)    gb->sp -= 2; INC_MCYCLE; CPU_WW (gb->sp, gb->pc); gb->pc = op & 0x38;

/* Rotate and shift instructions */

#define RLA   	OP(RLA)   { gb->nn = REG_A; REG_A = (REG_A << 1) | (gb->f_c);      gb->flags = 0; gb->f_c = ((gb->nn >> 7) & 1); }
#define RRA     OP(RRA)   { gb->nn = REG_A; REG_A = (REG_A >> 1) | (gb->f_c << 7); gb->flags = 0; gb->f_c = (gb->nn & 1); }
#define RLCA    OP(RLCA)  REG_A = (REG_A << 1) | (REG_A >> 7); gb->flags = 0; gb->f_c = (REG_A & 1); 
#define RRCA    OP(RRCA)  gb->flags = 0; gb->f_c = (REG_A & 1); REG_A = (REG_A >> 1) | (REG_A << 7); 

#define RLC(X)  OP(RLC) {\
    gb->nn = X;\
    X <<= 1; X |= (gb->nn >> 7);\
    SET_FLAGS(0, X, 0, 0, (gb->nn >> 7));\
}

#define RL(X)   OP(RL) {\
    gb->nn = X;\
    X <<= 1; X |= gb->f_c;\
    SET_FLAGS(0, X, 0, 0, (gb->nn >> 7));\
}

#define RRC(X)  OP(RRC) {\
    gb->nn = X;\
    X >>= 1; X |= ((gb->nn << 7) & 0xff);\
    SET_FLAGS(0, X, 0, 0, (gb->nn & 1));\
}

#define RR(X)   OP(RR) {\
    gb->nn = X;\
    X >>= 1; X |= gb->f_c << 7;\
    SET_FLAGS(0, X, 0, 0, (gb->nn & 1));\
}

#define SLA(X)  OP(SLA)   gb->flags = 0; SET_FLAG_C (X >> 7); X <<= 1; SET_FLAG_Z (X);
#define SRA(X)  OP(SRA)   gb->flags = 0; SET_FLAG_C (X & 1); X = (X >> 1) | (X & 0X80); SET_FLAG_Z(X);

#define SWAP(X) OP(SWAP) {\
    X = ((0xF0 & (X << 4)) | (0x0F & (X >> 4)));\
    SET_FLAGS(0, X, 0, 0, 0);\
}

#define SRL(X)  OP(SRL) {\
    uint8_t fc = (X & 1) ? 1 : 0;\
    X >>= 1; SET_FLAGS(0, X, 0, 0, fc);\
}

/* Bit instructions */

#define BIT(X)  OP(BIT)   SET_FLAGS(16, (X & (1 << r_bit)), 0, 1, 0);
#define RES(X)  OP(RES)   X &= (0xFE << r_bit) | (0xFF >> (8 - r_bit));
#define SET(X)  OP(SET)   X |= (1 << r_bit); 

/* Misc instructions */

#define PREFIX    OP(PREFIX)
#define INVALID   OP(INVALID)