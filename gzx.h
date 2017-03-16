#ifndef GZX_H
#define GZX_H

#include <stdio.h>

void zx_reset(void);

void zx_debug_mstep(void);
void zx_debug_key(int press, int key);

extern char *start_dir;
extern FILE *logfi;
extern int quit;

extern int slow_load;

#endif
