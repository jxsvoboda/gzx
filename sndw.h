#ifndef _SNDW_H
#define _SNDW_H

int sndw_init(int bufs);
void sndw_done(void);
void sndw_write(u8 *buf);

#endif
