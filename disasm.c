/*
 * GZX - George's ZX Spectrum Emulator
 * Z80 disassembler
 *
 * Copyright (c) 1999-2017 Jiri Svoboda
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "disasm.h"
#include "memio.h"

#define MAXARGLEN 20

static char hexc[]={
  '0','1','2','3','4','5','6','7',
  '8','9','A','B','C','D','E','F'
};

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

static char *op_n[]= {
  "ADC", "ADD", "AND", "BIT", "CALL","CCF", "CP",  "CPD",
  "CPDR","CPI", "CPIR","CPL", "DAA", "DEC", "DI",  "DJNZ",
  "EI",  "EX",  "EXX", "HALT","IM",  "IN",  "INC", "IND",
  "INDR","INI", "INIR","JP",  "JR",  "LD",  "LDD", "LDDR",
  "LDI", "LDIR","NEG", "NOP", "OR",  "OUT", "OUTD","OTDR",
  "OUTI","OTIR","POP", "PUSH","RES", "RET", "RETI","RETN",
  "RLA", "RL",  "RLCA","RLC", "RLD", "RRA", "RR",  "RRCA",
  "RRC","RRD",  "RST", "SBC", "SCF", "SET", "SLA", "SRA",
  "SLL", "SRL", "SUB", "XOR"
};

/* operand format strings */
static char *a_n[] = {
  "",  "A", "B", "C",
  "D", "E", "H", "L",
  "I", "R", "$", "($)",
  "AF","BC","DE","HL",

  "IX",  "IY",  "(IX$)","(IY$)",
  "C",   "NC",  "M",    "P",
  "Z",   "NZ",  "PE",   "PO",
  "(BC)","(DE)","(HL)", "(SP)",

  "(C)","0", "1",  "2",
  "3",  "4", "5",  "6",
  "7",  "SP","AF'",
};

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

#include "da_itab.c"

#define BUF_SIZE 64

static unsigned char *d_tab[3][3]={
	     /* --      ED      CB */
  /* -- */ { d_op,   d_edop, d_cbop },
  /* DD */ { d_ddop, d_edop, d_ddcbop },
  /* FD */ { d_fdop, d_edop, d_fdcbop },
};

static unsigned char op,idxop;
static unsigned char *desc;

static uint16_t o_ibegin;
static int ddfd; /* 0=none 1=DD 2=FD */
static int edcb; /* 0=none 1=ED 2=CB */

static int cur_arg;
static uint16_t in_pos;

uint16_t disasm_org;
char disasm_buf[BUF_SIZE];

static int out_pos;

static int da_getc(void) {
  int b;

  b=zx_memget8(in_pos++);
  disasm_org=in_pos;
  
  return b;
}

static void da_seek_org(uint16_t addr) {
  in_pos=addr;
  disasm_org=addr;
}

static void da_putc(char c) {
  if(out_pos<BUF_SIZE-1)
    disasm_buf[out_pos++]=c;
}

/***********************************/

static unsigned da_getw(void) {
  unsigned short r;

  r =da_getc();
  r|=((unsigned)da_getc())<<8;

  return r;
}

static void da_puts(char *s) {
  while(*s) da_putc(*s++);
}

static void da_prnw(uint16_t w) {
  int i;

  da_putc('$');
  for(i=12;i>=0;i-=4) {
    da_putc(hexc[(w>>i)&15]);
  }
}

static void da_prnb(uint8_t b) {
  da_putc('$');
  da_putc(hexc[b>>4]);
  da_putc(hexc[b&15]);
}

static void da_prnbsuf(unsigned char b) {
  da_putc(hexc[b>>4]);
  da_putc(hexc[b&15]);
  da_putc('H');
}

static void da_prnb_s8(uint8_t b) {

  if(b&0x80) {
    da_putc('-');
    b=(b^0xff)+1;
  } else {
    da_putc('+');
  }

  da_putc('$');
  da_putc(hexc[b>>4]);
  da_putc(hexc[b&15]);
}

static void da_t(void) {
}

static void da_db(void) {
  da_prnb(da_getc());
}

static void da_dw(void) {
  da_prnw(da_getw());
}

static void da_a(void) {
  da_prnw(da_getw());
}

static void da_i(void) {
  da_prnb_s8(da_getc());
}

static void da_ra(void) {
  unsigned adr;

  adr=da_getc();
  if(adr&0x80) adr|=~0xff; /* znamenk. rozsireni */
  adr+=disasm_org;
  da_prnw(adr);
}

static void da_rs(void) {
  da_prnbsuf(op&0x38);
}

static void da_ib(void) {
  if(cur_arg==0) da_prnb_s8(da_getc());
    else da_prnb(da_getc());
}

static void da_xi(void) {
  da_prnb_s8(idxop);
}


static void (*da_arg[])(void)={
  da_t, da_db,da_dw,da_a,
  da_i, da_ra,da_rs,da_ib,
  da_xi
};

/* print left, variable and right part of argument */
static void p_arg(int i) {
  char *p,*an;

  cur_arg=i;

  an=a_n[desc[2+i]];
  p=strchr(an,'$');

  if(!p) da_puts(an);
  else {
    p=an;
    while(*p != '$') da_putc(*p++);
    da_arg[desc[1]]();
    p++;
    da_puts(p);
  }
}

static int dec_instr(void) {
  int c;

  o_ibegin = disasm_org;
  ddfd=0;

  /* read DD/FD prefixes */
  c=da_getc();
  if(c<0) return -1;
  op=c;

  if(op==0xDD) ddfd=1;
    else if(op==0xFD) ddfd=2;

  if(ddfd) op=da_getc();

  switch(op) {
    case 0xED: edcb=1; break;
    case 0xCB: edcb=2; break;
    default:   edcb=0; break;
  }

  /* try reading opcode if we don't have it yet */
  if(edcb) op=da_getc();

  /* For DDCB/FDCB that was actually the displacement, opcode is the following byte */
  if(ddfd && (edcb==2)) {
    idxop=op;
    op=da_getc();
  }

  /* Read instruction description from the correct table */
  desc=d_tab[ddfd][edcb]+4*op;

  return 0;
}

int disasm_instr(void) {
  char *ops;
  int l;

  in_pos = disasm_org;
  
  out_pos=0;  
  disasm_buf[BUF_SIZE-1]=0;

  if(dec_instr()<0) return -1;

  /* Stray prefix? */
  if(desc[0]==o_sp) {
    switch(ddfd) {
      case 1: da_puts("DEFB $DD"); break;
      case 2: da_puts("DEFB $FD"); break;
      default: break;
    }
    da_seek_org(o_ibegin+1);
    da_putc(0);

    return 0;
  }

  if(desc[0]&o_x) { /* Undocumented instruction */
    da_puts("DEFB ");
    switch(ddfd) {
      case 1: da_puts("$DD,"); break;
      case 2: da_puts("$FD,"); break;
      default: break;
    }
    switch(edcb) {
      case 1: da_puts("$ED,"); break;
      case 2: da_puts("$CB,"); break;
      default: break;
    }
    if(ddfd && (edcb==2)) {
      da_prnb(idxop);
      da_putc(',');
    }
    da_prnb(op);
    /* Here we could add code to decode some addressing types */
    if(desc[0]==o_ui) {
      /* If there is no description available */
      da_putc(0);
      return 0;
    }
    da_puts("\t; "); /* Comment the rest of the line */
  }

  /* Print opcode name */
  ops=op_n[desc[0]&(~o_x)];
  da_puts(ops);

  if(desc[2]) { /* Does it have operands? */
    /* Pad with spaces */
    l=strlen(ops);
    while(l++<5) da_putc(' ');

    /* Print the operands */
    p_arg(0);
    if(desc[3]) { /* Second opcode? */
      da_putc(',');
      p_arg(1);
    }

  }
  da_putc(0);

  return 0;
}
