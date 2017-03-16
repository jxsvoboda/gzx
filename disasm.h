#ifndef DISASM_H
#define DISASM_H

extern unsigned disasm_org;
extern char disasm_buf[];

int disasm_instr(void);

#endif
