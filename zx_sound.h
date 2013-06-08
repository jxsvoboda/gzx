#ifndef _ZX_SOUND_H
#define _ZX_SOUND_H

int zx_sound_init(void);
void zx_sound_done(void);
void zx_sound_smp(int ay_out);

#endif
