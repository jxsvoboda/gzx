/*
 * GZX - George's ZX Spectrum Emulator
 * Z80 disassembler
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "da_itab.h"
#include "disasm.h"
#include "memio.h"

#define MAXARGLEN 20

static char hexc[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static char *op_n[] = {
	"ADC",  "ADD",  "AND",  "BIT",  "CALL", "CCF", "CP",   "CPD",
	"CPDR", "CPI",  "CPIR", "CPL",  "DAA",  "DEC", "DI",   "DJNZ",
	"EI",   "EX",   "EXX",  "HALT", "IM",   "IN",  "INC",  "IND",
	"INDR", "INI",  "INIR", "JP",   "JR",   "LD",  "LDD",  "LDDR",
	"LDI",  "LDIR", "NEG",  "NOP",  "OR",   "OUT", "OUTD", "OTDR",
	"OUTI", "OTIR", "POP",  "PUSH", "RES",  "RET", "RETI", "RETN",
	"RLA",  "RL",   "RLCA", "RLC",  "RLD",  "RRA", "RR",   "RRCA",
	"RRC",  "RRD",  "RST",  "SBC",  "SCF",  "SET", "SLA",  "SRA",
	"SLL",  "SRL",  "SUB",  "XOR"
};

/* operand format strings */
static char *a_n[] = {
	"",  "A", "B", "C",
	"D", "E", "H", "L",
	"I", "R", "$", "($)",
	"AF", "BC", "DE", "HL",

	"IX",   "IY",   "(IX$)", "(IY$)",
	"C",    "NC",   "M",     "P",
	"Z",    "NZ",   "PE",    "PO",
	"(BC)", "(DE)", "(HL)",  "(SP)",

	"(C)", "0",  "1",  "2",
	"3",   "4",  "5",  "6",
	"7",   "SP", "AF'",
};

#define BUF_SIZE 64

static unsigned char *d_tab[3][3] = {
	/*         --      ED      CB */
	/* -- */ { d_op,   d_edop, d_cbop },
	/* DD */ { d_ddop, d_edop, d_ddcbop },
	/* FD */ { d_fdop, d_edop, d_fdcbop },
};

static unsigned char op, idxop;
static unsigned char *desc;

static uint16_t o_ibegin;
static int ddfd; /* 0=none 1=DD 2=FD */
static int edcb; /* 0=none 1=ED 2=CB */

static int cur_arg;
static uint16_t in_pos;

uint16_t disasm_org;
char disasm_buf[BUF_SIZE];

static int out_pos;

static int da_getc(void)
{
	int b;

	b = zx_memget8(in_pos++);
	disasm_org = in_pos;

	return b;
}

static void da_seek_org(uint16_t addr)
{
	in_pos = addr;
	disasm_org = addr;
}

static void da_putc(char c)
{
	if (out_pos < BUF_SIZE - 1)
		disasm_buf[out_pos++] = c;
}

/** ******************************** */

static unsigned da_getw(void)
{
	unsigned short r;

	r = da_getc();
	r |= ((unsigned)da_getc()) << 8;

	return r;
}

static void da_puts(char *s)
{
	while (*s)
		da_putc(*s++);
}

static void da_prnw(uint16_t w)
{
	int i;

	da_putc('$');
	for (i = 12; i >= 0; i -= 4) {
		da_putc(hexc[(w >> i) & 15]);
	}
}

static void da_prnb(uint8_t b)
{
	da_putc('$');
	da_putc(hexc[b >> 4]);
	da_putc(hexc[b & 15]);
}

static void da_prnbsuf(unsigned char b)
{
	da_putc(hexc[b >> 4]);
	da_putc(hexc[b & 15]);
	da_putc('H');
}

static void da_prnb_s8(uint8_t b)
{

	if (b & 0x80) {
		da_putc('-');
		b = (b ^ 0xff) + 1;
	} else {
		da_putc('+');
	}

	da_putc('$');
	da_putc(hexc[b >> 4]);
	da_putc(hexc[b & 15]);
}

static void da_t(void)
{
}

static void da_db(void)
{
	da_prnb(da_getc());
}

static void da_dw(void)
{
	da_prnw(da_getw());
}

static void da_a(void)
{
	da_prnw(da_getw());
}

static void da_i(void)
{
	da_prnb_s8(da_getc());
}

static void da_ra(void)
{
	unsigned adr;

	adr = da_getc();
	if (adr & 0x80)
		adr |= ~0xff; /* znamenk. rozsireni */
	adr += disasm_org;
	da_prnw(adr);
}

static void da_rs(void)
{
	da_prnbsuf(op & 0x38);
}

static void da_ib(void)
{
	if (cur_arg == 0)
		da_prnb_s8(da_getc());
	else
		da_prnb(da_getc());
}

static void da_xi(void)
{
	da_prnb_s8(idxop);
}

static void (*da_arg[])(void) = {
	da_t, da_db, da_dw, da_a,
	da_i, da_ra, da_rs, da_ib,
	da_xi
};

/* print left, variable and right part of argument */
static void p_arg(int i)
{
	char *p, *an;

	cur_arg = i;

	an = a_n[desc[2 + i]];
	p = strchr(an, '$');

	if (!p)
		da_puts(an);
	else {
		p = an;
		while (*p != '$')
			da_putc(*p++);
		da_arg[desc[1]]();
		p++;
		da_puts(p);
	}
}

static int dec_instr(void)
{
	int c;

	o_ibegin = disasm_org;
	ddfd = 0;

	/* read DD/FD prefixes */
	c = da_getc();
	if (c < 0)
		return -1;
	op = c;

	if (op == 0xDD)
		ddfd = 1;
	else if (op == 0xFD)
		ddfd = 2;

	if (ddfd)
		op = da_getc();

	switch (op) {
	case 0xED:
		edcb = 1;
		break;
	case 0xCB:
		edcb = 2;
		break;
	default:
		edcb = 0;
		break;
	}

	/* try reading opcode if we don't have it yet */
	if (edcb)
		op = da_getc();

	/* For DDCB/FDCB that was actually the displacement, opcode is the following byte */
	if (ddfd && (edcb == 2)) {
		idxop = op;
		op = da_getc();
	}

	/* Read instruction description from the correct table */
	desc = d_tab[ddfd][edcb] + 4 * op;

	return 0;
}

int disasm_instr(void)
{
	char *ops;
	int l;

	in_pos = disasm_org;

	out_pos = 0;
	disasm_buf[BUF_SIZE - 1] = 0;

	if (dec_instr() < 0)
		return -1;

	/* Stray prefix? */
	if (desc[0] == o_sp) {
		switch (ddfd) {
		case 1:
			da_puts("DEFB $DD");
			break;
		case 2:
			da_puts("DEFB $FD");
			break;
		default:
			break;
		}
		da_seek_org(o_ibegin + 1);
		da_putc(0);

		return 0;
	}

	if (desc[0] & o_x) { /* Undocumented instruction */
		da_puts("DEFB ");
		switch (ddfd) {
		case 1:
			da_puts("$DD,");
			break;
		case 2:
			da_puts("$FD,");
			break;
		default:
			break;
		}
		switch (edcb) {
		case 1:
			da_puts("$ED,");
			break;
		case 2:
			da_puts("$CB,");
			break;
		default:
			break;
		}
		if (ddfd && (edcb == 2)) {
			da_prnb(idxop);
			da_putc(',');
		}
		da_prnb(op);
		/* Here we could add code to decode some addressing types */
		if (desc[0] == o_ui) {
			/* If there is no description available */
			da_putc(0);
			return 0;
		}
		da_puts("\t; "); /* Comment the rest of the line */
	}

	/* Print opcode name */
	ops = op_n[desc[0] & (~o_x)];
	da_puts(ops);

	if (desc[2]) { /* Does it have operands? */
		/* Pad with spaces */
		l = strlen(ops);
		while (l++ < 5)
			da_putc(' ');

		/* Print the operands */
		p_arg(0);
		if (desc[3]) { /* Second opcode? */
			da_putc(',');
			p_arg(1);
		}

	}
	da_putc(0);

	return 0;
}
