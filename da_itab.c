/*
  Z80 disassembler

  tabulka instrukci
  include do disasm.c
*/

unsigned char d_op[1024]={
  o_NOP, A_T, a__,  a__,    o_LD,  A_DW,a_BC, a_s,  /* 00 */
  o_LD,  A_T, a_iBC,a_A,    o_INC, A_T, a_BC, a__,  /* 02 */
  o_INC, A_T, a_B,  a__,    o_DEC, A_T, a_B,  a__,  /* 04 */
  o_LD,  A_DB,a_B,  a_s,    o_RLCA,A_T, a__,  a__,  /* 06 */
  o_EX,  A_T, a_AF, a_AF_,  o_ADD, A_T, a_HL, a_BC, /* 08 */
  o_LD,  A_T, a_A,  a_iBC,  o_DEC, A_T, a_BC, a__,  /* 0A */
  o_INC, A_T, a_C,  a__,    o_DEC, A_T, a_C,  a__,  /* 0C */
  o_LD,  A_DB,a_C,  a_s,    o_RRCA,A_T, a__,  a__,  /* 0E */

  o_DJNZ,A_RA,a_s,  a__,    o_LD,  A_DW,a_DE, a_s,  /* 10 */
  o_LD,  A_T, a_iDE,a_A,    o_INC, A_T, a_DE, a__,  /* 12 */
  o_INC, A_T, a_D,  a__,    o_DEC, A_T, a_D,  a__,  /* 14 */
  o_LD,  A_DB,a_D,  a_s,    o_RLA, A_T, a__,  a__,  /* 16 */
  o_JR,  A_RA,a_s,  a__,    o_ADD, A_T, a_HL, a_DE, /* 18 */
  o_LD,  A_T, a_A,  a_iDE,  o_DEC, A_T, a_DE, a__,  /* 1A */
  o_INC, A_T, a_E,  a__,    o_DEC, A_T, a_E,  a__,  /* 1C */
  o_LD,  A_DB,a_E,  a_s,    o_RRA, A_T, a__,  a__,  /* 1E */

  o_JR,  A_RA,a_NZ, a_s,    o_LD,  A_DW,a_HL, a_s,  /* 20 */
  o_LD,  A_A, a_is, a_HL,   o_INC, A_T, a_HL, a__,  /* 22 */
  o_INC, A_T, a_H,  a__,    o_DEC, A_T, a_H,  a__,  /* 24 */
  o_LD,  A_DB,a_H,  a_s,    o_DAA, A_T, a__,  a__,  /* 26 */
  o_JR,  A_RA,a_Z,  a_s,    o_ADD, A_T, a_HL, a_HL, /* 28 */
  o_LD,  A_A, a_HL, a_is,   o_DEC, A_T, a_HL, a__,  /* 2A */
  o_INC, A_T, a_L,  a__,    o_DEC, A_T, a_L,  a__,  /* 2C */
  o_LD,  A_DB,a_L,  a_s,    o_CPL, A_T, a__,  a__,  /* 2E */

  o_JR,  A_RA,a_NC, a_s,    o_LD,  A_DW,a_SP, a_s,  /* 30 */
  o_LD,  A_A, a_is, a_A,    o_INC, A_T, a_SP, a__,  /* 32 */
  o_INC, A_T, a_iHL,a__,    o_DEC, A_T, a_iHL,a__,  /* 34 */
  o_LD,  A_DB,a_iHL,a_s,    o_SCF, A_T, a__,  a__,  /* 36 */
  o_JR,  A_RA,a_C,  a_s,    o_ADD, A_T, a_HL, a_SP, /* 38 */
  o_LD,  A_A, a_A,  a_is,   o_DEC, A_T, a_SP, a__,  /* 3A */
  o_INC, A_T, a_A,  a__,    o_DEC, A_T, a_A,  a__,  /* 3C */
  o_LD,  A_DB,a_A,  a_s,    o_CCF, A_T, a__,  a__,  /* 3E */

  o_LD,  A_T, a_B,  a_B,    o_LD,  A_T, a_B,  a_C,  /* 40 */
  o_LD,  A_T, a_B,  a_D,    o_LD,  A_T, a_B,  a_E,  /* 42 */
  o_LD,  A_T, a_B,  a_H,    o_LD,  A_T, a_B,  a_L,  /* 44 */
  o_LD,  A_T, a_B,  a_iHL,  o_LD,  A_T, a_B,  a_A,  /* 46 */
  o_LD,  A_T, a_C,  a_B,    o_LD,  A_T, a_C,  a_C,  /* 48 */
  o_LD,  A_T, a_C,  a_D,    o_LD,  A_T, a_C,  a_E,  /* 4A */
  o_LD,  A_T, a_C,  a_H,    o_LD,  A_T, a_C,  a_L,  /* 4C */
  o_LD,  A_T, a_C,  a_iHL,  o_LD,  A_T, a_C,  a_A,  /* 4E */

  o_LD,  A_T, a_D,  a_B,    o_LD,  A_T, a_D,  a_C,  /* 50 */
  o_LD,  A_T, a_D,  a_D,    o_LD,  A_T, a_D,  a_E,  /* 52 */
  o_LD,  A_T, a_D,  a_H,    o_LD,  A_T, a_D,  a_L,  /* 54 */
  o_LD,  A_T, a_D,  a_iHL,  o_LD,  A_T, a_D,  a_A,  /* 56 */
  o_LD,  A_T, a_E,  a_B,    o_LD,  A_T, a_E,  a_C,  /* 58 */
  o_LD,  A_T, a_E,  a_D,    o_LD,  A_T, a_E,  a_E,  /* 5A */
  o_LD,  A_T, a_E,  a_H,    o_LD,  A_T, a_E,  a_L,  /* 5C */
  o_LD,  A_T, a_E,  a_iHL,  o_LD,  A_T, a_E,  a_A,  /* 5E */

  o_LD,  A_T, a_H,  a_B,    o_LD,  A_T, a_H,  a_C,  /* 60 */
  o_LD,  A_T, a_H,  a_D,    o_LD,  A_T, a_H,  a_E,  /* 62 */
  o_LD,  A_T, a_H,  a_H,    o_LD,  A_T, a_H,  a_L,  /* 64 */
  o_LD,  A_T, a_H,  a_iHL,  o_LD,  A_T, a_H,  a_A,  /* 66 */
  o_LD,  A_T, a_L,  a_B,    o_LD,  A_T, a_L,  a_C,  /* 68 */
  o_LD,  A_T, a_L,  a_D,    o_LD,  A_T, a_L,  a_E,  /* 6A */
  o_LD,  A_T, a_L,  a_H,    o_LD,  A_T, a_L,  a_L,  /* 5C */
  o_LD,  A_T, a_L,  a_iHL,  o_LD,  A_T, a_L,  a_A,  /* 6E */


  o_LD,  A_T, a_iHL,a_B,    o_LD,  A_T, a_iHL,a_C,  /* 70 */
  o_LD,  A_T, a_iHL,a_D,    o_LD,  A_T, a_iHL,a_E,  /* 72 */
  o_LD,  A_T, a_iHL,a_H,    o_LD,  A_T, a_iHL,a_L,  /* 74 */
  o_HALT,A_T, a__,  a__,    o_LD,  A_T, a_iHL,a_A,  /* 76 */
  o_LD,  A_T, a_A,  a_B,    o_LD,  A_T, a_A,  a_C,  /* 78 */
  o_LD,  A_T, a_A,  a_D,    o_LD,  A_T, a_A,  a_E,  /* 7A */
  o_LD,  A_T, a_A,  a_H,    o_LD,  A_T, a_A,  a_L,  /* 7C */
  o_LD,  A_T, a_A,  a_iHL,  o_LD,  A_T, a_A,  a_A,  /* 7E */

  o_ADD, A_T, a_A,  a_B,    o_ADD, A_T, a_A,  a_C,  /* 80 */
  o_ADD, A_T, a_A,  a_D,    o_ADD, A_T, a_A,  a_E,  /* 82 */
  o_ADD, A_T, a_A,  a_H,    o_ADD, A_T, a_A,  a_L,  /* 84 */
  o_ADD, A_T, a_A,  a_iHL,  o_ADD, A_T, a_A,  a_A,  /* 86 */
  o_ADC, A_T, a_A,  a_B,    o_ADC, A_T, a_A,  a_C,  /* 88 */
  o_ADC, A_T, a_A,  a_D,    o_ADC, A_T, a_A,  a_E,  /* 8A */
  o_ADC, A_T, a_A,  a_H,    o_ADC, A_T, a_A,  a_L,  /* 8C */
  o_ADC, A_T, a_A,  a_iHL,  o_ADC, A_T, a_A,  a_A,  /* 8E */

  o_SUB, A_T, a_B,  a__,    o_SUB, A_T, a_C,  a__,  /* 90 */
  o_SUB, A_T, a_D,  a__,    o_SUB, A_T, a_E,  a__,  /* 92 */
  o_SUB, A_T, a_H,  a__,    o_SUB, A_T, a_L,  a__,  /* 94 */
  o_SUB, A_T, a_iHL,a__,    o_SUB, A_T, a_A,  a__,  /* 96 */
  o_SBC, A_T, a_A,  a_B,    o_SBC, A_T, a_A,  a_C,  /* 98 */
  o_SBC, A_T, a_A,  a_D,    o_SBC, A_T, a_A,  a_E,  /* 9A */
  o_SBC, A_T, a_A,  a_H,    o_SBC, A_T, a_A,  a_L,  /* 9C */
  o_SBC, A_T, a_A,  a_iHL,  o_SBC, A_T, a_A,  a_A,  /* 9E */

  o_AND, A_T, a_B,  a__,    o_AND, A_T, a_C,  a__,  /* A0 */
  o_AND, A_T, a_D,  a__,    o_AND, A_T, a_E,  a__,  /* A2 */
  o_AND, A_T, a_H,  a__,    o_AND, A_T, a_L,  a__,  /* A4 */
  o_AND, A_T, a_iHL,a__,    o_AND, A_T, a_A,  a__,  /* A6 */
  o_XOR, A_T, a_B,  a__,    o_XOR, A_T, a_C,  a__,  /* A8 */
  o_XOR, A_T, a_D,  a__,    o_XOR, A_T, a_E,  a__,  /* AA */
  o_XOR, A_T, a_H,  a__,    o_XOR, A_T, a_L,  a__,  /* AC */
  o_XOR, A_T, a_iHL,a__,    o_XOR, A_T, a_A,  a__,  /* AE */

  o_OR,  A_T, a_B,  a__,    o_OR,  A_T, a_C,  a__,  /* A0 */
  o_OR,  A_T, a_D,  a__,    o_OR,  A_T, a_E,  a__,  /* A2 */
  o_OR,  A_T, a_H,  a__,    o_OR,  A_T, a_L,  a__,  /* A4 */
  o_OR,  A_T, a_iHL,a__,    o_OR,  A_T, a_A,  a__,  /* A6 */
  o_CP,  A_T, a_B,  a__,    o_CP,  A_T, a_C,  a__,  /* A8 */
  o_CP,  A_T, a_D,  a__,    o_CP,  A_T, a_E,  a__,  /* AA */
  o_CP,  A_T, a_H,  a__,    o_CP,  A_T, a_L,  a__,  /* AC */
  o_CP,  A_T, a_iHL,a__,    o_CP,  A_T, a_A,  a__,  /* AE */

  o_RET, A_T, a_NZ, a__,    o_POP, A_T, a_BC, a__,  /* C0 */
  o_JP,  A_A, a_NZ, a_s,    o_JP,  A_A, a_s,  a__,  /* C2 */
  o_CALL,A_A, a_NZ, a_s,    o_PUSH,A_T, a_BC, a__,  /* C4 */
  o_ADD, A_DB,a_A,  a_s,    o_RST, A_RS,a_s,  a__,  /* C6 */
  o_RET, A_T, a_Z,  a__,    o_RET, A_T, a__,  a__,  /* C8 */
  o_JP,  A_A, a_Z,  a_s,    0,     0,   0,    0,    /* CA */
  o_CALL,A_A, a_Z,  a_s,    o_CALL,A_A, a_s,  a__,  /* CC */
  o_ADC, A_DB,a_A,  a_s,    o_RST, A_RS,a_s,  a__,  /* CE */

  o_RET, A_T, a_NC, a__,    o_POP, A_T, a_DE, a__,  /* D0 */
  o_JP,  A_A, a_NC, a_s,    o_OUT, A_DB,a_is, a_A,  /* D2 */
  o_CALL,A_A, a_NC, a_s,    o_PUSH,A_T, a_DE, a__,  /* D4 */
  o_SUB, A_DB,a_s,  a__,    o_RST, A_RS,a_s,  a__,  /* D6 */
  o_RET, A_T, a_C,  a__,    o_EXX, A_T, a__,  a__,  /* D8 */
  o_JP,  A_A, a_C,  a_s,    o_IN,  A_DB,a_A,  a_is, /* DA */
  o_CALL,A_A, a_C,  a_s,    0,     0,   0,    0,    /* DC */
  o_SBC, A_DB,a_A,  a_s,    o_RST, A_RS,a_s,  a__,  /* DE */

  o_RET, A_T, a_PO, a__,    o_POP, A_T, a_HL, a__,  /* E0 */
  o_JP,  A_A, a_PO, a_s,    o_EX,  A_T, a_iSP,a_HL, /* E2 */
  o_CALL,A_A, a_PO, a_s,    o_PUSH,A_T, a_HL, a__,  /* E4 */
  o_AND, A_DB,a_s,  a__,    o_RST, A_RS,a_s,  a__,  /* E6 */
  o_RET, A_T, a_PE, a__,    o_JP,  A_T, a_iHL,a__,  /* E8 */
  o_JP,  A_A, a_PE, a_s,    o_EX,  A_T, a_DE, a_HL, /* EA */
  o_CALL,A_A, a_PE, a_s,    0,     0,   0,    0,    /* EC */
  o_XOR, A_DB,a_s,  a__,    o_RST, A_RS,a_s,  a__,  /* EE */

  o_RET, A_T, a_P,  a__,    o_POP, A_T, a_AF, a__,  /* F0 */
  o_JP,  A_A, a_P,  a_s,    o_DI,  A_T, a__,  a__,  /* F2 */
  o_CALL,A_A, a_P,  a_s,    o_PUSH,A_T, a_AF, a__,  /* F4 */
  o_OR,  A_DB,a_s,  a__,    o_RST, A_RS,a_s,  a__,  /* F6 */
  o_RET, A_T, a_M,  a__,    o_LD,  A_T, a_SP, a_HL, /* F8 */
  o_JP,  A_A, a_M,  a_s,    o_EI,  A_T, a__,  a__,  /* FA */
  o_CALL,A_A, a_M,  a_s,    0,     0,   0,    0,    /* FC */
  o_CP,  A_DB,a_s,  a__,    o_RST, A_RS,a_s,  a__,  /* FE */
};

unsigned char d_ddop[4*256]={
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 00 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 02 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 04 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 06 */
  o_sp,  0,   0,    0,      o_ADD, A_T, a_IX, a_BC, /* 08 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 0A */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 0C */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 0E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 10 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 12 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 14 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 16 */
  o_sp,  0,   0,    0,      o_ADD, A_T, a_IX, a_DE, /* 18 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 1A */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 1C */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 1E */

  o_sp,  0,   0,    0,      o_LD,  A_DW,a_IX, a_s,  /* 20 */
  o_LD,  A_A, a_is, a_IX,   o_INC, A_T, a_IX, a__,  /* 22 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 24 */
  o_ui,  0,   0,    0,      o_sp,  0,   0,    0,    /* 26 */
  o_sp,  0,   0,    0,      o_ADD, A_T, a_IX, a_IX, /* 28 */
  o_LD,  A_A, a_IX, a_is,   o_DEC, A_T, a_IX, a__,  /* 2A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2C */
  o_ui,  0,   0,    0,      o_sp,  0,   0,    0,    /* 2E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 30 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 32 */
  o_INC, A_I, a_iIX,a__,    o_DEC, A_I, a_iIX,a__,  /* 34 */
  o_LD,  A_IB,a_iIX,a_s,    o_sp,  0,   0,    0,    /* 36 */
  o_sp,  0,   0,    0,      o_ADD, A_T, a_IX, a_SP, /* 38 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 3A */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 3C */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 3E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 40 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 42 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 44 */
  o_LD,  A_I, a_B,  a_iIX,  o_sp,  0,   0,    0,    /* 46 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 48 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 4A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 4C */
  o_LD,  A_I, a_C,  a_iIX,  o_sp,  0,   0,    0,    /* 4E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 50 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 52 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 54 */
  o_LD,  A_I, a_D,  a_iIX,  o_sp,  0,   0,    0,    /* 56 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 58 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 5A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 5C */
  o_LD,  A_I, a_E,  a_iIX,  o_sp,  0,   0,    0,    /* 5E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 60 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 62 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 64 */
  o_LD,  A_I, a_H,  a_iIX,  o_ui,  0,   0,    0,    /* 66 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 68 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6C */
  o_LD,  A_I, a_L,  a_iIX,  o_ui,  0,   0,    0,    /* 6E */

  o_LD,  A_I, a_iIX,a_B,    o_LD,  A_I, a_iIX,a_C,  /* 70 */
  o_LD,  A_I, a_iIX,a_D,    o_LD,  A_I, a_iIX,a_E,  /* 72 */
  o_LD,  A_I, a_iIX,a_H,    o_LD,  A_I, a_iIX,a_L,  /* 74 */
  o_sp,  0,   0,    0,      o_LD,  A_I, a_iIX,a_A,  /* 76 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 78 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 7A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 7C */
  o_LD,  A_I, a_A,  a_iIX,  o_sp,  0,   0,    0,    /* 7E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 80 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 82 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 84 */
  o_ADD, A_I, a_A,  a_iIX,  o_sp,  0,   0,    0,    /* 86 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 88 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 8A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8C */
  o_ADC, A_I, a_A,  a_iIX,  o_sp,  0,   0,    0,    /* 8E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 90 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 92 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 94 */
  o_SUB, A_I, a_iIX,0,      o_sp,  0,   0,    0,    /* 96 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 98 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 9A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9C */
  o_SBC, A_I, a_A,  a_iIX,  o_sp,  0,   0,    0,    /* 9E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* A0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* A2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A4 */
  o_AND, A_I, a_iIX,0,      o_sp,  0,   0,    0,    /* A6 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* A8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* AA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* AC */
  o_XOR, A_I, a_iIX,0,      o_sp,  0,   0,    0,    /* AE */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* B0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* B2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B4 */
  o_OR,  A_I, a_iIX,0,      o_sp,  0,   0,    0,    /* B6 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* B8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* BA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* BC */
  o_CP,  A_I, a_iIX,0,      o_sp,  0,   0,    0,    /* BE */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C2 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C4 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C6 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C8 */
  o_sp,  0,   0,    0,      0,     0,   0,    0,    /* CA */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* CC */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* CE */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D2 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D4 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D6 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* DA */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* DC */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* DE */

  o_sp,  0,   0,    0,      o_POP, A_T, a_IX, 0,    /* E0 */
  o_sp,  0,   0,    0,      o_EX,  A_T, a_iSP,a_IX, /* E2 */
  o_sp,  0,   0,    0,      o_PUSH,A_T, a_IX, 0,    /* E4 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* E6 */
  o_sp,  0,   0,    0,      o_JP,  A_T, a_IX, 0,    /* E8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* EA */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* EC */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* EE */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* F0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* F2 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* F4 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* F6 */
  o_sp,  0,   0,    0,      o_LD,  A_T, a_SP, a_IX, /* F8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* FA */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* FC */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0     /* FE */

};

unsigned char d_fdop[4*256]={
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 00 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 02 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 04 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 06 */
  o_sp,  0,   0,    0,      o_ADD, A_T, a_IY, a_BC, /* 08 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 0A */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 0C */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 0E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 10 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 12 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 14 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 16 */
  o_sp,  0,   0,    0,      o_ADD, A_T, a_IY, a_DE, /* 18 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 1A */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 1C */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 1E */

  o_sp,  0,   0,    0,      o_LD,  A_DW,a_IY, a_s,  /* 20 */
  o_LD,  A_A, a_is, a_IY,   o_INC, A_T, a_IY, a__,  /* 22 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 24 */
  o_ui,  0,   0,    0,      o_sp,  0,   0,    0,    /* 26 */
  o_sp,  0,   0,    0,      o_ADD, A_T, a_IY, a_IY, /* 28 */
  o_LD,  A_A, a_IY, a_is,   o_DEC, A_T, a_IY, a__,  /* 2A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2C */
  o_ui,  0,   0,    0,      o_sp,  0,   0,    0,    /* 2E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 30 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 32 */
  o_INC, A_I, a_iIY,a__,    o_DEC, A_I, a_iIY,a__,  /* 34 */
  o_LD,  A_IB,a_iIY,a_s,    o_sp,  0,   0,    0,    /* 36 */
  o_sp,  0,   0,    0,      o_ADD, A_T, a_IY, a_SP, /* 38 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 3A */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 3C */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 3E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 40 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 42 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 44 */
  o_LD,  A_I, a_B,  a_iIY,  o_sp,  0,   0,    0,    /* 46 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 48 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 4A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 4C */
  o_LD,  A_I, a_C,  a_iIY,  o_sp,  0,   0,    0,    /* 4E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 50 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 52 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 54 */
  o_LD,  A_I, a_D,  a_iIY,  o_sp,  0,   0,    0,    /* 56 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 58 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 5A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 5C */
  o_LD,  A_I, a_E,  a_iIY,  o_sp,  0,   0,    0,    /* 5E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 60 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 62 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 64 */
  o_LD,  A_I, a_H,  a_iIY,  o_ui,  0,   0,    0,    /* 66 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 68 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6C */
  o_LD,  A_I, a_L,  a_iIY,  o_ui,  0,   0,    0,    /* 6E */

  o_LD,  A_I, a_iIY,a_B,    o_LD,  A_I, a_iIY,a_C,  /* 70 */
  o_LD,  A_I, a_iIY,a_D,    o_LD,  A_I, a_iIY,a_E,  /* 72 */
  o_LD,  A_I, a_iIY,a_H,    o_LD,  A_I, a_iIY,a_L,  /* 74 */
  o_sp,  0,   0,    0,      o_LD,  A_I, a_iIY,a_A,  /* 76 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 78 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 7A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 7C */
  o_LD,  A_I, a_A,  a_iIY,  o_sp,  0,   0,    0,    /* 7E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 80 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 82 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 84 */
  o_ADD, A_I, a_A,  a_iIY,  o_sp,  0,   0,    0,    /* 86 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 88 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 8A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8C */
  o_ADC, A_I, a_A,  a_iIY,  o_sp,  0,   0,    0,    /* 8E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 90 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 92 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 94 */
  o_SUB, A_I, a_iIY,0,      o_sp,  0,   0,    0,    /* 96 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 98 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* 9A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9C */
  o_SBC, A_I, a_A,  a_iIY,  o_sp,  0,   0,    0,    /* 9E */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* A0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* A2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A4 */
  o_AND, A_I, a_iIY,0,      o_sp,  0,   0,    0,    /* A6 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* A8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* AA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* AC */
  o_XOR, A_I, a_iIY,0,      o_sp,  0,   0,    0,    /* AE */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* B0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* B2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B4 */
  o_OR,  A_I, a_iIY,0,      o_sp,  0,   0,    0,    /* B6 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* B8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* BA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* BC */
  o_CP,  A_I, a_iIY,0,      o_sp,  0,   0,    0,    /* BE */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C2 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C4 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C6 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* C8 */
  o_sp,  0,   0,    0,      0,     0,   0,    0,    /* CA */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* CC */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* CE */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D2 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D4 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D6 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* D8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* DA */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* DC */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* DE */

  o_sp,  0,   0,    0,      o_POP, A_T, a_IY, 0,    /* E0 */
  o_sp,  0,   0,    0,      o_EX,  A_T, a_iSP,a_IY, /* E2 */
  o_sp,  0,   0,    0,      o_PUSH,A_T, a_IY, 0,    /* E4 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* E6 */
  o_sp,  0,   0,    0,      o_JP,  A_T, a_IY, 0,    /* E8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* EA */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* EC */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* EE */

  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* F0 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* F2 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* F4 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* F6 */
  o_sp,  0,   0,    0,      o_LD,  A_T, a_SP, a_IY, /* F8 */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* FA */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0,    /* FC */
  o_sp,  0,   0,    0,      o_sp,  0,   0,    0     /* FE */

};

unsigned char d_edop[4*256]={
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 00 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 02 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 04 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 06 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 08 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 0A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 0C */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 0E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 10 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 12 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 14 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 16 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 18 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 1A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 1C */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 1E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 20 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 22 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 24 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 26 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 28 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2C */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 30 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 32 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 34 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 36 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 38 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 3A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 3C */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 3E */

  o_IN,  A_T, a_B,  a_iC,   o_OUT, A_T, a_iC, a_B,  /* 40 */
  o_SBC, A_T, a_HL, a_BC,   o_LD,  A_A, a_is, a_BC, /* 42 */
  o_NEG, A_T, a__,  a__,    o_RETN,A_T, a__,  a__,  /* 44 */
  o_IM,  A_T, a_0,  a__,    o_LD,  A_T, a_I,  a_A,  /* 46 */
  o_IN,  A_T, a_C,  a_iC,   o_OUT, A_T, a_iC, a_C,  /* 48 */
  o_ADC, A_T, a_HL, a_BC,   o_LD,  A_A, a_BC, a_is, /* 4A */
  o_ui,  0,   0,    0,      o_RETI,A_T, a__,  a__,  /* 4C */
  o_ui,  0,   0,    0,      o_LD,  A_T, a_R,  a_A,  /* 4E */

  o_IN,  A_T, a_D,  a_iC,   o_OUT, A_T, a_iC, a_D,  /* 50 */
  o_SBC, A_T, a_HL, a_DE,   o_LD,  A_A, a_is, a_DE, /* 52 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 54 */
  o_IM,  A_T, a_1,  a__,    o_LD,  A_T, a_A,  a_I,  /* 56 */
  o_IN,  A_T, a_E,  a_iC,   o_OUT, A_T, a_iC, a_E,  /* 58 */
  o_ADC, A_T, a_HL, a_DE,   o_LD,  A_A, a_DE, a_is, /* 5A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 5C */
  o_IM,  A_T, a_2,  a__,    o_LD,  A_T, a_A,  a_R,  /* 5E */

  o_IN,  A_T, a_H,  a_iC,   o_OUT, A_T, a_iC, a_H,  /* 60 */
  o_SBC, A_T, a_HL, a_HL,   o_LD,  A_A, a_is, a_HL, /* 62 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 64 */
  o_ui,  0,   0,    0,      o_RRD, A_T, a__,  a__,  /* 66 */
  o_IN,  A_T, a_L,  a_iC,   o_OUT, A_T, a_iC, a_L,  /* 68 */
  o_ADC, A_T, a_HL, a_HL,   o_LD,  A_A, a_HL, a_is, /* 6A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6C */
  o_ui,  0,   0,    0,      o_RLD, 0,   0,    0,    /* 6E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 70 */
  o_SBC, A_T, a_HL, a_SP,   o_LD,  A_A, a_is, a_SP, /* 72 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 74 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 76 */
  o_IN,  A_T, a_A,  a_iC,   o_OUT, A_T, a_iC, a_A,  /* 78 */
  o_ADC, A_T, a_HL, a_SP,   o_LD,  A_A, a_SP, a_is, /* 7A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 7C */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 7E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 80 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 82 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 84 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 86 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 88 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8C */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 90 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 92 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 94 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 96 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 98 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9C */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9E */

  o_LDI, A_T, a__,  a__,    o_CPI, A_T, a__,  a__,  /* A0 */
  o_INI, A_T, a__,  a__,    o_OUTI,A_T, a__,  a__,  /* A2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A4 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A6 */
  o_LDD, A_T, a__,  a__,    o_CPD, A_T, a__,  a__,  /* A8 */
  o_IND, A_T, a__,  a__,    o_OUTD,A_T, a__,  a__,  /* AA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* AC */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* AE */

  o_LDIR,A_T, a__,  a__,    o_CPIR,A_T, a__,  a__,  /* B0 */
  o_INIR,A_T, a__,  a__,    o_OTIR,A_T, a__,  a__,  /* B2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B4 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B6 */
  o_LDDR,A_T, a__,  a__,    o_CPDR,A_T, a__,  a__,  /* B8 */
  o_INDR,A_T, a__,  a__,    o_OTDR,A_T, a__,  a__,  /* BA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* BC */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* BE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C4 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* CA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* CC */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* CE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D4 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* DA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* DC */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* DE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E4 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* EA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* EC */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* EE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F4 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* FA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* FC */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0     /* FE */

};


unsigned char d_cbop[]={
  o_RLC, A_T, a_B,  a__,    o_RLC, A_T, a_C,  a__,  /* 00 */
  o_RLC, A_T, a_D,  a__,    o_RLC, A_T, a_E,  a__,  /* 02 */
  o_RLC, A_T, a_H,  a__,    o_RLC, A_T, a_L,  a__,  /* 04 */
  o_RLC, A_T, a_iHL,a__,    o_RLC, A_T, a_A,  a__,  /* 06 */
  o_RRC, A_T, a_B,  a__,    o_RRC, A_T, a_C,  a__,  /* 08 */
  o_RRC, A_T, a_D,  a__,    o_RRC, A_T, a_E,  a__,  /* 0A */
  o_RRC, A_T, a_H,  a__,    o_RRC, A_T, a_L,  a__,  /* 0C */
  o_RRC, A_T, a_iHL,a__,    o_RRC, A_T, a_A,  a__,  /* 0E */

  o_RL,  A_T, a_B,  a__,    o_RL,  A_T, a_C,  a__,  /* 10 */
  o_RL,  A_T, a_D,  a__,    o_RL,  A_T, a_E,  a__,  /* 12 */
  o_RL,  A_T, a_H,  a__,    o_RL,  A_T, a_L,  a__,  /* 14 */
  o_RL,  A_T, a_iHL,a__,    o_RL,  A_T, a_A,  a__,  /* 16 */
  o_RR,  A_T, a_B,  a__,    o_RR,  A_T, a_C,  a__,  /* 18 */
  o_RR,  A_T, a_D,  a__,    o_RR,  A_T, a_E,  a__,  /* 1A */
  o_RR,  A_T, a_H,  a__,    o_RR,  A_T, a_L,  a__,  /* 1C */
  o_RR,  A_T, a_iHL,a__,    o_RR,  A_T, a_A,  a__,  /* 1E */

  o_SLA, A_T, a_B,  a__,    o_SLA, A_T, a_C,  a__,  /* 20 */
  o_SLA, A_T, a_D,  a__,    o_SLA, A_T, a_E,  a__,  /* 22 */
  o_SLA, A_T, a_H,  a__,    o_SLA, A_T, a_L,  a__,  /* 24 */
  o_SLA, A_T, a_iHL,a__,    o_SLA, A_T, a_A,  a__,  /* 26 */
  o_SRA, A_T, a_B,  a__,    o_SRA, A_T, a_C,  a__,  /* 28 */
  o_SRA, A_T, a_D,  a__,    o_SRA, A_T, a_E,  a__,  /* 2A */
  o_SRA, A_T, a_H,  a__,    o_SRA, A_T, a_L,  a__,  /* 2C */
  o_SRA, A_T, a_iHL,a__,    o_SRA, A_T, a_A,  a__,  /* 2E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 30 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 32 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 34 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 36 */
  o_SRL, A_T, a_B,  a__,    o_SRL, A_T, a_C,  a__,  /* 38 */
  o_SRL, A_T, a_D,  a__,    o_SRL, A_T, a_E,  a__,  /* 3A */
  o_SRL, A_T, a_H,  a__,    o_SRL, A_T, a_L,  a__,  /* 3C */
  o_SRL, A_T, a_iHL,a__,    o_SRL, A_T, a_A,  a__,  /* 3E */

  o_BIT, A_T, a_0,  a_B,    o_BIT, A_T, a_0,  a_C,  /* 40 */
  o_BIT, A_T, a_0,  a_D,    o_BIT, A_T, a_0,  a_E,  /* 42 */
  o_BIT, A_T, a_0,  a_H,    o_BIT, A_T, a_0,  a_L,  /* 44 */
  o_BIT, A_T, a_0,  a_iHL,  o_BIT, A_T, a_0,  a_A,  /* 46 */
  o_BIT, A_T, a_1,  a_B,    o_BIT, A_T, a_1,  a_C,  /* 48 */
  o_BIT, A_T, a_1,  a_D,    o_BIT, A_T, a_1,  a_E,  /* 4A */
  o_BIT, A_T, a_1,  a_H,    o_BIT, A_T, a_1,  a_L,  /* 4C */
  o_BIT, A_T, a_1,  a_iHL,  o_BIT, A_T, a_1,  a_A,  /* 4E */

  o_BIT, A_T, a_2,  a_B,    o_BIT, A_T, a_2,  a_C,  /* 50 */
  o_BIT, A_T, a_2,  a_D,    o_BIT, A_T, a_2,  a_E,  /* 52 */
  o_BIT, A_T, a_2,  a_H,    o_BIT, A_T, a_2,  a_L,  /* 54 */
  o_BIT, A_T, a_2,  a_iHL,  o_BIT, A_T, a_2,  a_A,  /* 56 */
  o_BIT, A_T, a_3,  a_B,    o_BIT, A_T, a_3,  a_C,  /* 58 */
  o_BIT, A_T, a_3,  a_D,    o_BIT, A_T, a_3,  a_E,  /* 5A */
  o_BIT, A_T, a_3,  a_H,    o_BIT, A_T, a_3,  a_L,  /* 5C */
  o_BIT, A_T, a_3,  a_iHL,  o_BIT, A_T, a_3,  a_A,  /* 5E */

  o_BIT, A_T, a_4,  a_B,    o_BIT, A_T, a_4,  a_C,  /* 60 */
  o_BIT, A_T, a_4,  a_D,    o_BIT, A_T, a_4,  a_E,  /* 62 */
  o_BIT, A_T, a_4,  a_H,    o_BIT, A_T, a_4,  a_L,  /* 64 */
  o_BIT, A_T, a_4,  a_iHL,  o_BIT, A_T, a_4,  a_A,  /* 66 */
  o_BIT, A_T, a_5,  a_B,    o_BIT, A_T, a_5,  a_C,  /* 68 */
  o_BIT, A_T, a_5,  a_D,    o_BIT, A_T, a_5,  a_E,  /* 6A */
  o_BIT, A_T, a_5,  a_H,    o_BIT, A_T, a_5,  a_L,  /* 6C */
  o_BIT, A_T, a_5,  a_iHL,  o_BIT, A_T, a_5,  a_A,  /* 6E */

  o_BIT, A_T, a_6,  a_B,    o_BIT, A_T, a_6,  a_C,  /* 70 */
  o_BIT, A_T, a_6,  a_D,    o_BIT, A_T, a_6,  a_E,  /* 72 */
  o_BIT, A_T, a_6,  a_H,    o_BIT, A_T, a_6,  a_L,  /* 74 */
  o_BIT, A_T, a_6,  a_iHL,  o_BIT, A_T, a_6,  a_A,  /* 76 */
  o_BIT, A_T, a_7,  a_B,    o_BIT, A_T, a_7,  a_C,  /* 78 */
  o_BIT, A_T, a_7,  a_D,    o_BIT, A_T, a_7,  a_E,  /* 7A */
  o_BIT, A_T, a_7,  a_H,    o_BIT, A_T, a_7,  a_L,  /* 7C */
  o_BIT, A_T, a_7,  a_iHL,  o_BIT, A_T, a_7,  a_A,  /* 7E */

  o_RES, A_T, a_0,  a_B,    o_RES, A_T, a_0,  a_C,  /* 80 */
  o_RES, A_T, a_0,  a_D,    o_RES, A_T, a_0,  a_E,  /* 82 */
  o_RES, A_T, a_0,  a_H,    o_RES, A_T, a_0,  a_L,  /* 84 */
  o_RES, A_T, a_0,  a_iHL,  o_RES, A_T, a_0,  a_A,  /* 86 */
  o_RES, A_T, a_1,  a_B,    o_RES, A_T, a_1,  a_C,  /* 88 */
  o_RES, A_T, a_1,  a_D,    o_RES, A_T, a_1,  a_E,  /* 8A */
  o_RES, A_T, a_1,  a_H,    o_RES, A_T, a_1,  a_L,  /* 8C */
  o_RES, A_T, a_1,  a_iHL,  o_RES, A_T, a_1,  a_A,  /* 8E */

  o_RES, A_T, a_2,  a_B,    o_RES, A_T, a_2,  a_C,  /* 90 */
  o_RES, A_T, a_2,  a_D,    o_RES, A_T, a_2,  a_E,  /* 92 */
  o_RES, A_T, a_2,  a_H,    o_RES, A_T, a_2,  a_L,  /* 94 */
  o_RES, A_T, a_2,  a_iHL,  o_RES, A_T, a_2,  a_A,  /* 96 */
  o_RES, A_T, a_3,  a_B,    o_RES, A_T, a_3,  a_C,  /* 98 */
  o_RES, A_T, a_3,  a_D,    o_RES, A_T, a_3,  a_E,  /* 9A */
  o_RES, A_T, a_3,  a_H,    o_RES, A_T, a_3,  a_L,  /* 9C */
  o_RES, A_T, a_3,  a_iHL,  o_RES, A_T, a_3,  a_A,  /* 9E */

  o_RES, A_T, a_4,  a_B,    o_RES, A_T, a_4,  a_C,  /* A0 */
  o_RES, A_T, a_4,  a_D,    o_RES, A_T, a_4,  a_E,  /* A2 */
  o_RES, A_T, a_4,  a_H,    o_RES, A_T, a_4,  a_L,  /* A4 */
  o_RES, A_T, a_4,  a_iHL,  o_RES, A_T, a_4,  a_A,  /* A6 */
  o_RES, A_T, a_5,  a_B,    o_RES, A_T, a_5,  a_C,  /* A8 */
  o_RES, A_T, a_5,  a_D,    o_RES, A_T, a_5,  a_E,  /* AA */
  o_RES, A_T, a_5,  a_H,    o_RES, A_T, a_5,  a_L,  /* AC */
  o_RES, A_T, a_5,  a_iHL,  o_RES, A_T, a_5,  a_A,  /* AE */

  o_RES, A_T, a_6,  a_B,    o_RES, A_T, a_6,  a_C,  /* B0 */
  o_RES, A_T, a_6,  a_D,    o_RES, A_T, a_6,  a_E,  /* B2 */
  o_RES, A_T, a_6,  a_H,    o_RES, A_T, a_6,  a_L,  /* B4 */
  o_RES, A_T, a_6,  a_iHL,  o_RES, A_T, a_6,  a_A,  /* B6 */
  o_RES, A_T, a_7,  a_B,    o_RES, A_T, a_7,  a_C,  /* B8 */
  o_RES, A_T, a_7,  a_D,    o_RES, A_T, a_7,  a_E,  /* BA */
  o_RES, A_T, a_7,  a_H,    o_RES, A_T, a_7,  a_L,  /* BC */
  o_RES, A_T, a_7,  a_iHL,  o_RES, A_T, a_7,  a_A,  /* BE */

  o_SET, A_T, a_0,  a_B,    o_SET, A_T, a_0,  a_C,  /* C0 */
  o_SET, A_T, a_0,  a_D,    o_SET, A_T, a_0,  a_E,  /* C2 */
  o_SET, A_T, a_0,  a_H,    o_SET, A_T, a_0,  a_L,  /* C4 */
  o_SET, A_T, a_0,  a_iHL,  o_SET, A_T, a_0,  a_A,  /* C6 */
  o_SET, A_T, a_1,  a_B,    o_SET, A_T, a_1,  a_C,  /* C8 */
  o_SET, A_T, a_1,  a_D,    o_SET, A_T, a_1,  a_E,  /* CA */
  o_SET, A_T, a_1,  a_H,    o_SET, A_T, a_1,  a_L,  /* CC */
  o_SET, A_T, a_1,  a_iHL,  o_SET, A_T, a_1,  a_A,  /* CE */

  o_SET, A_T, a_2,  a_B,    o_SET, A_T, a_2,  a_C,  /* D0 */
  o_SET, A_T, a_2,  a_D,    o_SET, A_T, a_2,  a_E,  /* D2 */
  o_SET, A_T, a_2,  a_H,    o_SET, A_T, a_2,  a_L,  /* D4 */
  o_SET, A_T, a_2,  a_iHL,  o_SET, A_T, a_2,  a_A,  /* D6 */
  o_SET, A_T, a_3,  a_B,    o_SET, A_T, a_3,  a_C,  /* D8 */
  o_SET, A_T, a_3,  a_D,    o_SET, A_T, a_3,  a_E,  /* DA */
  o_SET, A_T, a_3,  a_H,    o_SET, A_T, a_3,  a_L,  /* DC */
  o_SET, A_T, a_3,  a_iHL,  o_SET, A_T, a_3,  a_A,  /* DE */

  o_SET, A_T, a_4,  a_B,    o_SET, A_T, a_4,  a_C,  /* E0 */
  o_SET, A_T, a_4,  a_D,    o_SET, A_T, a_4,  a_E,  /* E2 */
  o_SET, A_T, a_4,  a_H,    o_SET, A_T, a_4,  a_L,  /* E4 */
  o_SET, A_T, a_4,  a_iHL,  o_SET, A_T, a_4,  a_A,  /* E6 */
  o_SET, A_T, a_5,  a_B,    o_SET, A_T, a_5,  a_C,  /* E8 */
  o_SET, A_T, a_5,  a_D,    o_SET, A_T, a_5,  a_E,  /* EA */
  o_SET, A_T, a_5,  a_H,    o_SET, A_T, a_5,  a_L,  /* EC */
  o_SET, A_T, a_5,  a_iHL,  o_SET, A_T, a_5,  a_A,  /* EE */

  o_SET, A_T, a_6,  a_B,    o_SET, A_T, a_6,  a_C,  /* F0 */
  o_SET, A_T, a_6,  a_D,    o_SET, A_T, a_6,  a_E,  /* F2 */
  o_SET, A_T, a_6,  a_H,    o_SET, A_T, a_6,  a_L,  /* F4 */
  o_SET, A_T, a_6,  a_iHL,  o_SET, A_T, a_6,  a_A,  /* F6 */
  o_SET, A_T, a_7,  a_B,    o_SET, A_T, a_7,  a_C,  /* F8 */
  o_SET, A_T, a_7,  a_D,    o_SET, A_T, a_7,  a_E,  /* FA */
  o_SET, A_T, a_7,  a_H,    o_SET, A_T, a_7,  a_L,  /* FC */
  o_SET, A_T, a_7,  a_iHL,  o_SET, A_T, a_7,  a_A,  /* FE */

};

unsigned char d_ddcbop[]={
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 00 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 02 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 04 */
  o_RLC, A_XI,a_iIX,a__,    o_ui,  0,   0,    0,    /* 06 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 08 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 0A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 0C */
  o_RRC, A_XI,a_iIX,a__,    o_ui,  0,   0,    0,    /* 0E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 10 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 12 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 14 */
  o_RL,  A_XI,a_iIX,a__,    o_ui,  0,   0,    0,    /* 16 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 18 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 1A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 1C */
  o_RR,  A_XI,a_iIX,a__,    o_ui,  0,   0,    0,    /* 1E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 20 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 22 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 24 */
  o_SLA, A_XI,a_iIX,a__,    o_ui,  0,   0,    0,    /* 26 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 28 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2C */
  o_SRA, A_XI,a_iIX,a__,    o_ui,  0,   0,    0,    /* 2E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 30 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 32 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 34 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 36 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 38 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 3A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 3C */
  o_SRL, A_XI,a_iIX,a__,    o_ui,  0,   0,    0,    /* 3E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 40 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 42 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 44 */
  o_BIT, A_XI,a_0,  a_iIX,  o_ui,  0,   0,    0,    /* 46 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 48 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 4A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 4C */
  o_BIT, A_XI,a_1,  a_iIX,  o_ui,  0,   0,    0,    /* 4E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 50 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 52 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 54 */
  o_BIT, A_XI,a_2,  a_iIX,  o_ui,  0,   0,    0,    /* 56 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 58 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 5A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 5C */
  o_BIT, A_XI,a_3,  a_iIX,  o_ui,  0,   0,    0,    /* 5E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 60 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 62 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 64 */
  o_BIT, A_XI,a_4,  a_iIX,  o_ui,  0,   0,    0,    /* 66 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 68 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6C */
  o_BIT, A_XI,a_5,  a_iIX,  o_ui,  0,   0,    0,    /* 6E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 70 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 72 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 74 */
  o_BIT, A_XI,a_6,  a_iIX,  o_ui,  0,   0,    0,    /* 76 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 78 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 7A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 7C */
  o_BIT, A_XI,a_7,  a_iIX,  o_ui,  0,   0,    0,    /* 7E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 80 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 82 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 84 */
  o_RES, A_XI,a_0,  a_iIX,  o_ui,  0,   0,    0,    /* 86 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 88 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8C */
  o_RES, A_XI,a_1,  a_iIX,  o_ui,  0,   0,    0,    /* 8E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 90 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 92 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 94 */
  o_RES, A_XI,a_2,  a_iIX,  o_ui,  0,   0,    0,    /* 96 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 98 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9C */
  o_RES, A_XI,a_3,  a_iIX,  o_ui,  0,   0,    0,    /* 9E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A4 */
  o_RES, A_XI,a_4,  a_iIX,  o_ui,  0,   0,    0,    /* A6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* AA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* AC */
  o_RES, A_XI,a_5,  a_iIX,  o_ui,  0,   0,    0,    /* AE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B4 */
  o_RES, A_XI,a_6,  a_iIX,  o_ui,  0,   0,    0,    /* B6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* BA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* BC */
  o_RES, A_XI,a_7,  a_iIX,  o_ui,  0,   0,    0,    /* BE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C4 */
  o_SET, A_XI,a_0,  a_iIX,  o_ui,  0,   0,    0,    /* C6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* CA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* CC */
  o_SET, A_XI,a_1,  a_iIX,  o_ui,  0,   0,    0,    /* CE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D4 */
  o_SET, A_XI,a_2,  a_iIX,  o_ui,  0,   0,    0,    /* D6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* DA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* DC */
  o_SET, A_XI,a_3,  a_iIX,  o_ui,  0,   0,    0,    /* DE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E4 */
  o_SET, A_XI,a_4,  a_iIX,  o_ui,  0,   0,    0,    /* E6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* EA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* EC */
  o_SET, A_XI,a_5,  a_iIX,  o_ui,  0,   0,    0,    /* EE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F4 */
  o_SET, A_XI,a_6,  a_iIX,  o_ui,  0,   0,    0,    /* F6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* FA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* FC */
  o_SET, A_XI,a_7,  a_iIX,  o_ui,  0,   0,    0,    /* FE */

};

unsigned char d_fdcbop[]={
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 00 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 02 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 04 */
  o_RLC, A_XI,a_iIY,a__,    o_ui,  0,   0,    0,    /* 06 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 08 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 0A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 0C */
  o_RRC, A_XI,a_iIY,a__,    o_ui,  0,   0,    0,    /* 0E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 10 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 12 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 14 */
  o_RL,  A_XI,a_iIY,a__,    o_ui,  0,   0,    0,    /* 16 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 18 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 1A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 1C */
  o_RR,  A_XI,a_iIY,a__,    o_ui,  0,   0,    0,    /* 1E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 20 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 22 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 24 */
  o_SLA, A_XI,a_iIY,a__,    o_ui,  0,   0,    0,    /* 26 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 28 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 2C */
  o_SRA, A_XI,a_iIY,a__,    o_ui,  0,   0,    0,    /* 2E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 30 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 32 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 34 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 36 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 38 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 3A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 3C */
  o_SRL, A_XI,a_iIY,a__,    o_ui,  0,   0,    0,    /* 3E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 40 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 42 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 44 */
  o_BIT, A_XI,a_0,  a_iIY,  o_ui,  0,   0,    0,    /* 46 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 48 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 4A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 4C */
  o_BIT, A_XI,a_1,  a_iIY,  o_ui,  0,   0,    0,    /* 4E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 50 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 52 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 54 */
  o_BIT, A_XI,a_2,  a_iIY,  o_ui,  0,   0,    0,    /* 56 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 58 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 5A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 5C */
  o_BIT, A_XI,a_3,  a_iIY,  o_ui,  0,   0,    0,    /* 5E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 60 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 62 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 64 */
  o_BIT, A_XI,a_4,  a_iIY,  o_ui,  0,   0,    0,    /* 66 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 68 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 6C */
  o_BIT, A_XI,a_5,  a_iIY,  o_ui,  0,   0,    0,    /* 6E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 70 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 72 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 74 */
  o_BIT, A_XI,a_6,  a_iIY,  o_ui,  0,   0,    0,    /* 76 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 78 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 7A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 7C */
  o_BIT, A_XI,a_7,  a_iIY,  o_ui,  0,   0,    0,    /* 7E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 80 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 82 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 84 */
  o_RES, A_XI,a_0,  a_iIY,  o_ui,  0,   0,    0,    /* 86 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 88 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 8C */
  o_RES, A_XI,a_1,  a_iIY,  o_ui,  0,   0,    0,    /* 8E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 90 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 92 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 94 */
  o_RES, A_XI,a_2,  a_iIY,  o_ui,  0,   0,    0,    /* 96 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 98 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9A */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* 9C */
  o_RES, A_XI,a_3,  a_iIY,  o_ui,  0,   0,    0,    /* 9E */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A4 */
  o_RES, A_XI,a_4,  a_iIY,  o_ui,  0,   0,    0,    /* A6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* A8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* AA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* AC */
  o_RES, A_XI,a_5,  a_iIY,  o_ui,  0,   0,    0,    /* AE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B4 */
  o_RES, A_XI,a_6,  a_iIY,  o_ui,  0,   0,    0,    /* B6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* B8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* BA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* BC */
  o_RES, A_XI,a_7,  a_iIY,  o_ui,  0,   0,    0,    /* BE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C4 */
  o_SET, A_XI,a_0,  a_iIY,  o_ui,  0,   0,    0,    /* C6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* C8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* CA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* CC */
  o_SET, A_XI,a_1,  a_iIY,  o_ui,  0,   0,    0,    /* CE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D4 */
  o_SET, A_XI,a_2,  a_iIY,  o_ui,  0,   0,    0,    /* D6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* D8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* DA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* DC */
  o_SET, A_XI,a_3,  a_iIY,  o_ui,  0,   0,    0,    /* DE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E4 */
  o_SET, A_XI,a_4,  a_iIY,  o_ui,  0,   0,    0,    /* E6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* E8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* EA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* EC */
  o_SET, A_XI,a_5,  a_iIY,  o_ui,  0,   0,    0,    /* EE */

  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F0 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F2 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F4 */
  o_SET, A_XI,a_6,  a_iIY,  o_ui,  0,   0,    0,    /* F6 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* F8 */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* FA */
  o_ui,  0,   0,    0,      o_ui,  0,   0,    0,    /* FC */
  o_SET, A_XI,a_7,  a_iIY,  o_ui,  0,   0,    0,    /* FE */

};
