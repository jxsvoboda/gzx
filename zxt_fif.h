#ifndef ZXT_FIF_H
#define ZXT_FIF_H

#include "intdef.h"
#include "zx_tape.h"

typedef struct {
  int (*open_file)(char *filename);
  int (*close_file)(void);
  int (*rewind_file)(void);
  
  int (*block_type)(void);
  int (*get_b_data_info)(tb_data_info_t *info);
  int (*get_b_voice_info)(tb_voice_info_t *voice);
  
  int (*skip_block)(void);
  int (*open_block)(void);
  int (*close_block)(void);
  
  int (*b_data_getbytes)(int n, u8 *dst);
  int (*b_voice_getsmps)(int n, unsigned *dst);
  int (*b_tones_gettone)(int *pnum, int *plen);
  
  int (*b_moredata)(void);
} tfr_t;

#endif
