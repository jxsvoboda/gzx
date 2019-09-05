/*
 * GZX - George's ZX Spectrum Emulator
 * Execution trace
 *
 * Copyright (c) 1999-2019 Jiri Svoboda
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

#include <stdio.h>
#include "disasm.h"
#include "gzx.h"
#include "memio.h"
#include "xtrace.h"
#include "z80.h"

static void xtrace_fprintregs(FILE *f)
{
	fprintf(f, "AF %04x BC %04x DE %04x HL %04x IX %04x PC %04x R %02d (HL)%02x Pg%02x\n",
	    z80_getAF() & 0xffd7, z80_getBC(), z80_getDE(), z80_getHL(),
	    cpus.IX, cpus.PC, cpus.R, zx_memget8(z80_getHL()), page_reg);
	fprintf(f, "AF'%04x BC'%04x DE'%04x HL'%04x IY %04x SP'%04x I%02d IFF%d%d IM%d\n",
          z80_getAF_() & 0xffd7, z80_getBC_(), z80_getDE_(), z80_getHL_(), cpus.IY,
	  cpus.SP, cpus.I, cpus.IFF1, cpus.IFF2, cpus.int_mode);
}

static void xtrace_fprintinstr(FILE *f)
{
	disasm_org = cpus.PC;
	if (disasm_instr() == 0)
		fprintf(f, "%04x: %s\n", cpus.PC, disasm_buf);
}

/** Log an instruction that is about to be executed. */
void xtrace_instr(void)
{
	xtrace_fprintregs(logfi);
	xtrace_fprintinstr(logfi);
}

/** Log that the emulator is resetting the machine. */
void xtrace_reset(void)
{
	fprintf(logfi, "** Resetting the machine **\n");
}

/** Log that an interrupt has been raised. */
void xtrace_int(void)
{
	fprintf(logfi, "** Raising interrupt **\n");
}
