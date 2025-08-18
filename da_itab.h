/*
 * GZX - George's ZX Spectrum Emulator
 * Integrated debugger
 *
 * Copyright (c) 1999-2025 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef DA_ITAB_H
#define DA_ITAB_H

/* instruction names */

#define o_ADC	0
#define o_ADD   1
#define o_AND	2
#define o_BIT	3
#define o_CALL	4
#define o_CCF	5
#define o_CP	6
#define o_CPD	7
#define o_CPDR	8
#define o_CPI	9
#define o_CPIR	10
#define o_CPL	11
#define o_DAA	12
#define o_DEC   13
#define o_DI    14
#define o_DJNZ  15
#define o_EI    16
#define o_EX    17
#define o_EXX   18
#define o_HALT	19
#define o_IM	20
#define o_IN	21
#define o_INC	22
#define o_IND	23
#define o_INDR	24
#define o_INI	25
#define o_INIR	26
#define o_JP	27
#define o_JR	28
#define o_LD	29
#define o_LDD	30
#define o_LDDR	31
#define o_LDI	32
#define o_LDIR  33
#define o_NEG	34
#define o_NOP	35
#define o_OR	36
#define o_OUT	37
#define o_OUTD	38
#define o_OTDR	39
#define o_OUTI	40
#define o_OTIR	41
#define o_POP	42
#define o_PUSH	43
#define o_RES	44
#define o_RET	45
#define o_RETI	46
#define o_RETN	47
#define o_RLA	48
#define o_RL	49
#define o_RLCA	50
#define o_RLC	51
#define o_RLD   52
#define o_RRA	53
#define o_RR	54
#define o_RRCA	55
#define o_RRC	56
#define o_RRD   57
#define o_RST	58
#define o_SBC	59
#define o_SCF	60
#define o_SET	61
#define o_SLA	62
#define o_SRA	63
#define o_SLL	64
#define o_SRL	65
#define o_SUB	66
#define o_XOR	67

#define o_sp	68
#define o_ui   128

#define o_x    128
/* when ORed with operation number, it means that the assembler does not
 * understand this and it needs to be written out as DB.
 * Used for some undocumented opcodes. 
 */

#define A_T	0	/* trivial */
#define A_DB	1       /* 8-bit direct */
#define A_DW    2       /* 16-bit direct */
#define A_A     3       /* address */
#define A_I	4       /* indirect */
#define A_RA    5       /* PC-relative address */
#define A_RS    6	/* restart address */
#define A_IB    7	/* indirect with 8-bit displacement */
#define A_XI    8       /* DDCB/FDCB indirect */

#define a__	0
#define a_A	1
#define a_B	2
#define a_C	3

#define a_D	4
#define a_E	5
#define a_H	6
#define a_L	7

#define a_I	8
#define a_R	9
#define a_s	10
#define a_is	11

#define a_AF	12
#define a_BC	13
#define a_DE	14
#define a_HL	15

#define a_IX	16
#define a_IY	17
#define a_iIX	18
#define a_iIY	19

//#define a_C	20
#define a_NC	21
#define a_M	22
#define a_P	23

#define a_Z	24
#define a_NZ	25
#define a_PE	26
#define a_PO	27

#define a_iBC	28
#define a_iDE	29
#define a_iHL	30
#define a_iSP	31

#define a_iC	32
#define a_0	33
#define a_1	34
#define a_2	35
#define a_3	36
#define a_4	37
#define a_5	38
#define a_6	39
#define a_7	40

#define a_SP	41
#define a_AF_	42

extern unsigned char d_op[1024];
extern unsigned char d_ddop[1024];
extern unsigned char d_fdop[1024];
extern unsigned char d_edop[1024];
extern unsigned char d_cbop[1024];
extern unsigned char d_ddcbop[1024];
extern unsigned char d_fdcbop[1024];

#endif
