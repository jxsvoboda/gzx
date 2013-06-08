#ifndef _DISASM_H
#define _DISASM_H

extern unsigned disasm_org;
extern char disasm_buf[];

int disasm_instr(void);

#endif
