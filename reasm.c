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

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "da_itab.h"
#include "reasm.h"

/* List of opcode tables and prefixes. */
static op_table_desc_t tdesc[N_TABLES] = {
	{
		d_op,
		{ },
		0
	},
	{
		d_ddop,
		{ 0xdd },
		1
	},
	{
		d_fdop,
		{ 0xfd },
		1
	},
	{
		d_edop,
		{ 0xed },
		1
	},
	{
		d_cbop,
		{ 0xcb },
		1
	},
	{
		d_ddcbop,
		{ 0xdd, 0xcb },
		2
	},
	{
		d_fdcbop,
		{ 0xfd, 0xcb },
		2
	}
};

/** Determine if character is a hexadecimal digit.
 *
 * @param c Character
 * @return @c true iff @a c is a hexadecimal digit
 */
static bool is_hexdigit(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F');
}

/** Return the value of a hexadecimal digit.
 *
 * @param c Character (must be a hexadecimal digit)
 * @return Value of hexadecimal digit
 */
static uint8_t hexval(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return 10 + (c - 'a');
	} else if (c >= 'A' && c <= 'F') {
		return 10 + (c - 'A');
	} else {
		return 0;
	}
}

/** Skip whitespace characters.
 *
 * @param sp Address of pointer to next character.
 */
static void reasm_skipws(const char **sp)
{
	while (**sp == ' ')
		++*sp;
}

/** Match pattern.
 *
 * The pattern can contain an identifier (e.g. 'NOP', 'AF'), number
 * ('$42') and punctuation ('(', ')'). When matching we allow
 * whitespace before and after punctuation. The '$' metacharacter
 * is considered as a terminating character for the pattern. This
 * is so that a pattern containing '$', e.g., '(IX$)' can be matched
 * in parts, i.e., the prefix '(IX', then the parameter can be matched,
 * then the suffix ')'.
 *
 * @param sp Address of pointer to current character.
 * @param pattern Pattern
 */
static int reasm_match_pattern(const char **sp, const char *pattern)
{
	const char *p;
	bool ends_punct = false;

	p = *sp;
	while (*pattern != '\0' && *pattern != '$') {
		/* Skip spaces before and after parentheses. */
		if (*pattern == '(' || *pattern == ')') {
			ends_punct = true;
			reasm_skipws(&p);
			if (*p != *pattern)
				return -1;
			reasm_skipws(&p);
		}

		/* Match same characters. */
		while (*pattern != '\0' && *pattern != '$' &&
		    toupper(*p) == *pattern) {
			ends_punct = false;
			++p;
			++pattern;
		}

		if (toupper(*p) != *pattern)
			break;
	}

	if (*pattern != '\0' && *pattern != '$')
		return -1;

	if ((isalnum(*p) || *p == '_') && !ends_punct)
		return -1;

	*sp = p;
	return 0;
}

/** Match numeric parameter (i.e. literal value, [+-]$<hex>).
 *
 * @param sp Address of pointer to current character
 * @parma val Place to store value on success
 * @return Zero on success, non-zero on failure
 */
static int reasm_match_param(const char **sp, int32_t *val)
{
	const char *p;
	bool sign;
	int32_t v;
	int32_t w;

	p = *sp;
	sign = false;

	if (*p == '+' || *p == '-') {
		sign = (*p == '-');
		++p;
	}

	if (*p != '$')
		return -1;
	++p;
	if (!is_hexdigit(*p))
		return -1;

	v = 0;
	do {
		w = v << 4;
		/* Check for overflow. */
		if (w >> 4 != v)
			return -1;
		v = w + hexval(*p);
		++p;
	} while (is_hexdigit(*p));

	*sp = p;
	*val = sign ? -v : v;
	return 0;
}

/** Match operand.
 *
 * @param op Operand type
 * @param sp Address of pointer to string
 * @param val Place to store operand value
 * @return Zero on success, non-zero on failure
 */
static int reasm_match_oper(int op, const char **sp, int32_t *val)
{
	const char *p;

	/* Match the pattern, or the prefix, if there is a '$' sign. */
	if (a_n[op][0] != '$' && reasm_match_pattern(sp, a_n[op]) != 0)
		return -1;

	/* Is there a '$' metachar? */
	p = strchr(a_n[op], '$');
	if (p != NULL) {
		/* Match the parameter. */
		if (reasm_match_param(sp, val) != 0)
			return -1;

		/* Match the rest. */
		if (reasm_match_pattern(sp, &p[1]) != 0)
			return -1;

		return 0;
	}

	return 0;
}

/** Match instruction descritpion..
 *
 * @param org Address of start of instruction
 * @param desc Opcode table description
 * @param opcode Opcode
 * @param desc Table description
 * @param icode Instruction name code
 * @param str String containing instruction operands
 * @param buf Destination buffer
 * @param bsize Destination buffer size
 * @return Positive number of bytes written on success
 */
static int reasm_match_instr(uint16_t org, op_table_desc_t *desc,
    uint8_t opcode, unsigned char *idesc, int icode, const char *str,
    uint8_t *buf, size_t bsize)
{
	unsigned i;
	unsigned char op_class;
	unsigned char op0;
	unsigned char op1;
	unsigned int off;
	const char *sp;
	uint16_t base;
	int32_t val;
	int32_t val2;
	int8_t ra;
	int rc;

	op_class = idesc[1];
	op0 = idesc[2];
	op1 = idesc[3];

	val = 0;
	val2 = 0;

	sp = str;
	rc = reasm_match_oper(op0, &sp, &val);
	if (rc != 0)
		return 0;

	reasm_skipws(&sp);
	if (op0 != a__ && op1 != a__) {
		/* two operands are separated by comma */
		if (*sp != ',')
			return 0;
		++sp;
		reasm_skipws(&sp);
	}

	if (op_class != A_IB) {
		rc = reasm_match_oper(op1, &sp, &val);
		if (rc != 0)
			return 0;
	} else {
		/* Instruction has two numeric parameters. */
		rc = reasm_match_oper(op1, &sp, &val2);
		if (rc != 0)
			return 0;
	}

	reasm_skipws(&sp);

	/* at the end? */
	if (*sp != '\0') {
		/* not matched */
		return 0;
	}

	/* store prefixes */
	off = 0;
	for (i = 0; i < desc->nprefix; i++) {
		if (off < bsize)
			buf[off] = desc->prefix[i];
		++off;
	}

	if (op_class != A_XI) {
		/* store opcode */
		if (off < bsize)
			buf[off] = opcode;
		++off;
	}

	switch (op_class) {
	case A_T:
		break;
	case A_DB:
		/* Check that val is in range of unsigned byte. */
		if (val < 0 || val > 0xff)
			return 0;
		if (off < bsize)
			buf[off] = val;
		++off;
		break;
	case A_DW:
	case A_A:
		/* Check that val is in range of unsigned word. */
		if (val < 0 || val > 0xffff)
			return 0;
		if (off < bsize)
			buf[off] = val & 0xff;
		++off;
		if (off < bsize)
			buf[off] = val >> 8;
		++off;
		break;
	case A_RA:
		base = org + off + 1;
		ra = val - base;
		/* Check that val is in range. */
		if (base + ra != val)
			return 0;
		if (off < bsize)
			buf[off] = val - base;
		++off;
		break;
	case A_RS:
		/* Check that val is a valid restart address. */
		if (val & ~0x38)
			return 0;
		if (off - 1 < bsize)
			buf[off - 1] |= val & 0x38;
		break;
	case A_IB:
		/* Check that val is in range of signed byte. */
		if (val < -0x80 || val > 0x7f)
			return 0;
		if (off < bsize)
			buf[off] = val & 0xff;
		++off;

		/* Check that val is in range of unsigned byte. */
		if (val2 < 0 || val2 > 0xff)
			return 0;
		if (off < bsize)
			buf[off] = val2;
		++off;
		break;
	case A_I:
	case A_XI:
		/* Check that val is in range of signed byte. */
		if (val < -0x80 || val > 0x7f)
			return 0;
		if (off < bsize)
			buf[off] = val & 0xff;
		++off;
		break;
	}

	if (op_class == A_XI) {
		/* store opcode */
		if (off < bsize)
			buf[off] = opcode;
		++off;
	}

	return off;
}

/** Find matching instruction in a partiular table.
 *
 * @param org Address of start of instruction
 * @param desc Table description
 * @param icode Instruction name code
 * @param str String containing instruction operands
 * @param buf Destination buffer
 * @param bsize Destination buffer size
 * @return Positive number of bytes written on success
 */
static int reasm_table_lookup(uint16_t org, op_table_desc_t *desc, int icode,
    const char *str, uint8_t *buf, size_t bsize)
{
	unsigned i;
	unsigned char *idesc;
	unsigned char dicode;
	int rc;

	for (i = 0; i < 256; i++) {
		idesc = &desc->table[i << 2];
		dicode = idesc[0];
		if (icode == dicode) {
			rc = reasm_match_instr(org, desc, i, idesc, icode, str,
			    buf, bsize);
			if (rc > 0)
				return rc;
		}
	}

	return 0;
}

int reasm_instr(uint16_t org, const char *str, uint8_t *buf, size_t bsize)
{
	const char *sp;
	unsigned i;
	unsigned icode;
	int nb;

	sp = str;
	reasm_skipws(&sp);

	/*
	 * Determine instruction name code.
	 */
	for (i = 0; i < o_max; i++) {
		if (reasm_match_pattern(&sp, op_n[i]) == 0)
			break;
	}

	if (i >= o_max) {
		/* no match */
		return -1;
	}

	icode = i;

	reasm_skipws(&sp);

	for (i = 0; i < N_TABLES; i++) {
		nb = reasm_table_lookup(org, &tdesc[i], icode, sp, buf, bsize);
		if (nb > 0)
			break;
	}

	return nb;
}
