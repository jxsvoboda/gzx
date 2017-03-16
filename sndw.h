#ifndef SNDW_H
#define SNDW_H

#include "intdef.h"

int sndw_init(int bufs);
void sndw_done(void);
void sndw_write(u8 *buf);

#endif
