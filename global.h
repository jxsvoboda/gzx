#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>

#define Z80_CLOCK 3500000L

void zx_reset(void);

void zx_debug_mstep(void);
void zx_debug_key(int press, int key);

extern char *start_dir;
extern FILE *logfi;
extern int quit;

extern int slow_load;

#endif
