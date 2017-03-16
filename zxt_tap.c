/*
  GZX - George's ZX Spectrum Emulator
  Gerton Lunter's TAP fileformat support
*/

#include "fileutil.h"
#include "global.h"
#include "intdef.h"
#include "zx_tape.h"
#include "zxt_fif.h"

static int tap_open_file(char *filename);
static int tap_close_file(void);
static int tap_rewind_file(void);
  
static int tap_block_type(void);
static int tap_get_b_data_info(tb_data_info_t *info);
static int tap_get_b_voice_info(tb_voice_info_t *info);
  
static int tap_skip_block(void);
static int tap_open_block(void);
static int tap_close_block(void);
  
static int tap_b_data_getbytes(int n, u8 *dst);
static int tap_b_voice_getsmps(int n, unsigned *dst);
static int tap_b_tones_gettone(int *pnum, int *plen);
  
static int tap_b_moredata(void);


tfr_t tfr_tap = {
  tap_open_file,
  tap_close_file,
  tap_rewind_file,
  tap_block_type,
  tap_get_b_data_info,
  tap_get_b_voice_info,
  tap_skip_block,
  tap_open_block,
  tap_close_block,
  tap_b_data_getbytes,
  tap_b_voice_getsmps,
  tap_b_tones_gettone,
  tap_b_moredata
};

static FILE *tapf;
static int tapf_len;

static int block_open;
static int block_start;
static int block_len;
static int block_end;

static int tapf_fb;

static int tap_open_file(char *filename) {

  tapf=fopen(filename,"rb");
  if(!tapf) return -1;
  tapf_len=fsize(tapf);
  
  block_open=0;
  tapf_fb = ftell(tapf);
  
  return 0;
}

static int tap_close_file(void) {
  return fclose(tapf);
}

static int tap_rewind_file(void) {
  if(fseek(tapf,tapf_fb,SEEK_SET)<0) return -1;
  block_open=0;
  return 0;
}
  
static int tap_block_type(void) {
  if(block_open) return -1;
  
  if(ftell(tapf)>=tapf_len) return BT_EOT;
  return BT_DATA;
}

static int tap_get_b_data_info(tb_data_info_t *info) {
  unsigned bstart,blen;
  u8 bflag;
  
  if(block_open) return -1;

  bstart=ftell(tapf);
  blen=fgetu16le(tapf);
  bflag=fgetu8(tapf);
  fseek(tapf,bstart,SEEK_SET);
  
  fprintf(logfi,"tap_get_b_data_info: bstart=%d blen=%d bflag=$%02x\n",
    bstart,blen,bflag);
  
  info->rom_timing=1;
  info->has_leadin=1;
  info->data_bytes=blen;
  info->used_bits=8;
  info->pause_after_len=ROM_PAUSE_LEN;
  
  info->pilot_len=ROM_PILOT_LEN;
  info->sync1_len=ROM_SYNC1_LEN;
  info->sync2_len=ROM_SYNC2_LEN;
  info->zero_len =ROM_ZERO_LEN;
  info->one_len  =ROM_ONE_LEN;
  info->pilot_pulses=(!bflag) ? ROM_PPULSES_H : ROM_PPULSES_D;
  
  return 0;
}

static int tap_get_b_voice_info(tb_voice_info_t *info) {
  return -2;
}
  
static int tap_skip_block(void) {
  unsigned blen;

  if(block_open) return -1;

  blen=fgetu16le(tapf);
  fseek(tapf,blen,SEEK_CUR);
  return 0;
}

static int tap_open_block(void) {
  if(block_open) return -1;

  block_start=ftell(tapf);
  
  if(block_start>=tapf_len) return -1;
  
  block_len  =fgetu16le(tapf);
  block_end  =block_start+2+block_len;
  block_open =1;
  
  return 0;
}

static int tap_close_block(void) {
  if(!block_open) return -1;
  
  fseek(tapf,block_end,SEEK_SET);
  block_open =0;
  return 0;
}
  
static int tap_b_data_getbytes(int n, u8 *dst) {
  int bleft,r;

  if(!block_open) return -1;
    
  bleft=block_end-ftell(tapf);
  if(bleft<n) return -1;
    
  r=fread(dst,1,n,tapf);
  
  if(r<n) return -1;
  return 0;
}

static int tap_b_voice_getsmps(int n, unsigned *dst) {
  return -2;
}

static int tap_b_tones_gettone(int *pnum, int *plen) {
  return -2;
}
  
static int tap_b_moredata(void) {
  if(!block_open) return -1;
  return ftell(tapf)<block_end;
}
