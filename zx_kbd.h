#ifndef ZX_KBD_H
#define ZX_KBD_H

#include "intdef.h"

u8 zx_key_in(u8 pwr);
void zx_key_state_set(int key, int press);
int zx_keys_init(void);

#endif
