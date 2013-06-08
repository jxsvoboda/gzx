#ifndef _SNAP_H
#define _SNAP_H

#include "global.h"
#include "z80.h"

int zx_load_snap_z80(char *name);
int zx_load_snap_sna(char *name);
int zx_load_snap(char *name);
int zx_save_snap(char *name);

#endif
