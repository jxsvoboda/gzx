/*
 * GZX - George's ZX Spectrum Emulator
 * TZX file format support
 *
 * Copyright (c) 1999-2017 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include "fileutil.h"
#include "gzx.h"
#include "intdef.h"
#include "zx_tape.h"
#include "zxt_fif.h"

static int tzx_open_file(char *filename);
static int tzx_close_file(void);
static int tzx_rewind_file(void);
  
static int tzx_block_type(void);
static int tzx_get_b_data_info(tb_data_info_t *info);
static int tzx_get_b_voice_info(tb_voice_info_t *info);
  
static int tzx_skip_block(void);
static int tzx_open_block(void);
static int tzx_close_block(void);
  
static int tzx_b_data_getbytes(int n, u8 *dst);
static int tzx_b_voice_getsmps(int n, unsigned *dst);
static int tzx_b_tones_gettone(int *pnum, int *plen);
  
static int tzx_b_moredata(void);


tfr_t tfr_tzx = {
  tzx_open_file,
  tzx_close_file,
  tzx_rewind_file,
  tzx_block_type,
  tzx_get_b_data_info,
  tzx_get_b_voice_info,
  tzx_skip_block,
  tzx_open_block,
  tzx_close_block,
  tzx_b_data_getbytes,
  tzx_b_voice_getsmps,
  tzx_b_tones_gettone,
  tzx_b_moredata
};

static FILE *tapf;
static int tapf_len;
static int tapf_fb;

static int block_open;
static int block_start;
static int block_dstart;
static u8  block_tzx_type;
static int block_type;
static int block_dlen;
static int block_end;

static u8 voice_byte;
static int voice_bbits;

static char *tzx_id="ZXTape!\032";

static int maj_ver,min_ver;

static int tzx_open_file(char *filename) {
  char tmpid[8];

  tapf=fopen(filename,"rb");
  if(!tapf) return -1;
  tapf_len=fsize(tapf);
  
  block_open=0;
  
  if(fread(tmpid,1,8,tapf)<8) return -1;
  if(memcmp(tmpid,tzx_id,8)) {
    printf("not a TZX file\n");
    return -1;
  }
  
  maj_ver=fgetu8(tapf);
  min_ver=fgetu8(tapf);
  
  tapf_fb=ftell(tapf);
  
  return 0;
}

static int tzx_close_file(void) {
  return fclose(tapf);
}

static int tzx_rewind_file(void) {
  if(fseek(tapf,tapf_fb,SEEK_SET)<0) return -1;
  block_open=0;
  return 0;
}
  
static int tzx_block_type(void) {
  int pos;
  u8 type;
  
  if(block_open) return -1;
  
  pos=ftell(tapf);
  if(pos>=tapf_len) return BT_EOT;
  
  type=fgetu8(tapf);
  
  fseek(tapf,pos,SEEK_SET);
  
  fprintf(logfi,"tzx_block_type (0x%02x)\n",type);
  switch(type) {
    case 0x10:  /* standard speed data block */
    case 0x11:  /* turbo loading data block */
    case 0x14:  /* pure data block */
      return BT_DATA;
      
    case 0x12:  /* pure tone */
    case 0x13:  /* sequence of pulses */
      return BT_TONES;
      
    case 0x15:  /* direct recording */
      return BT_VOICE;
      
    default:
      return BT_UNKNOWN;
  }
}

static int tzx_get_b_data_info(tb_data_info_t *info) {
  unsigned bstart;
  u8 bflag;
  u8 btype;
  unsigned datalen;
  unsigned pause;
  
  if(block_open) return -1;

  bstart=ftell(tapf);
  btype=fgetu8(tapf);
  
  switch(btype) {
    case 0x10: /* standard speed data block */
      pause=fgetu16le(tapf);
      datalen=fgetu16le(tapf);
      bflag=fgetu8(tapf);
      
      fprintf(logfi,"tzx_get_b_data_info: (standard) bstart=%d type=0x%02x datalen=$%02x\n flag=%02x",
        bstart,btype,datalen,bflag);
    
      info->rom_timing=1;
      info->has_leadin=1;
      info->data_bytes=datalen;
      info->used_bits=8;
      info->pause_after_len=pause*3500;
  
      info->pilot_len=ROM_PILOT_LEN;
      info->sync1_len=ROM_SYNC1_LEN;
      info->sync2_len=ROM_SYNC2_LEN;
      info->zero_len =ROM_ZERO_LEN;
      info->one_len  =ROM_ONE_LEN;
      info->pilot_pulses=(!bflag) ? ROM_PPULSES_H : ROM_PPULSES_D;
      break;
    
    case 0x11:  /* turbo loading data block */
      info->pilot_len=fgetu16le(tapf);
      info->sync1_len=fgetu16le(tapf);
      info->sync2_len=fgetu16le(tapf);
      info->zero_len =fgetu16le(tapf);
      info->one_len  =fgetu16le(tapf);
      info->pilot_pulses=fgetu16le(tapf);
      info->used_bits=fgetu8(tapf);
      
      pause=fgetu16le(tapf);
      datalen=fgetu24le(tapf);
      
      fprintf(logfi,"tzx_get_b_data_info: (turbo) bstart=%d type=0x%02x datalen=$%02x\n",
        bstart,btype,datalen);
    
      info->rom_timing=0;
      info->has_leadin=1;
      info->data_bytes=datalen;
      info->pause_after_len=pause*3500;
      break;
      
    case 0x14:  /* pure data block */
      info->pilot_len=0;
      info->sync1_len=0;
      info->sync2_len=0;
      info->zero_len =fgetu16le(tapf);
      info->one_len  =fgetu16le(tapf);
      info->pilot_pulses=0;
      info->used_bits=fgetu8(tapf);
      
      pause=fgetu16le(tapf);
      datalen=fgetu24le(tapf);
      
      fprintf(logfi,"tzx_get_b_data_info: (pure data) bstart=%d type=0x%02x datalen=$%02x\n",
        bstart,btype,datalen);
    
      info->rom_timing=0;
      info->has_leadin=0;
      info->data_bytes=datalen;
      info->pause_after_len=pause*3500;
      break;
      
    default:
      fseek(tapf,bstart,SEEK_SET);
      return -1;
  }
      
  fseek(tapf,bstart,SEEK_SET);
    
  return 0;
}

static int tzx_get_b_voice_info(tb_voice_info_t *info) {
  unsigned bstart,btype;
  int used_bits,data_len;

  if(block_open) return -1;

  bstart=ftell(tapf);
  btype=fgetu8(tapf);
  
  if(btype!=0x15) {
    fseek(tapf,bstart,SEEK_SET);
    return -1;
  }
  
  fprintf(logfi,"tzx_get_b_voice_info: bstart=%d btype=0x%02x\n",
    bstart,btype);
  
  info->smp_len=fgetu16le(tapf);
  info->pause_after_len=fgetu16le(tapf);
  used_bits=fgetu8(tapf);
  data_len=fgetu24le(tapf);
  info->samples=data_len*8 + (used_bits-8);
  
  fseek(tapf,bstart,SEEK_SET);
  return 0;
}
  
static int tzx_skip_block(void) {
  unsigned btype,len;

  if(block_open) return -1;

  btype=fgetu8(tapf);
  switch(btype) {
    case 0x10:
      fseek(tapf,2,SEEK_CUR);
      len=fgetu16le(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x11:
      fseek(tapf,15,SEEK_CUR);
      len=fgetu24le(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;  
      
    case 0x12:
      fseek(tapf,4,SEEK_CUR);
      return 0;
      
    case 0x13:
      len=fgetu8(tapf);
      fseek(tapf,len*2,SEEK_CUR);
      return 0;
      
    case 0x14:
      fseek(tapf,7,SEEK_CUR);
      len=fgetu24le(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x15:
      fseek(tapf,5,SEEK_CUR);
      len=fgetu24le(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x20:
      fseek(tapf,2,SEEK_CUR);
      return 0;
      
    case 0x21:
      len=fgetu8(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x22:
      return 0;
      
    case 0x23:
      fseek(tapf,2,SEEK_CUR);
      return 0;
      
    case 0x24:
      fseek(tapf,2,SEEK_CUR);
      return 0;
      
    case 0x25:
      return 0;
      
    case 0x26:
      len=fgetu16le(tapf);
      fseek(tapf,2*len,SEEK_CUR);
      return 0;
      
    case 0x27:
      return 0;
      
    case 0x28:
      len=fgetu16le(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x30:
      len=fgetu8(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x31:
      fseek(tapf,1,SEEK_CUR);
      len=fgetu8(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x32:
      len=fgetu16le(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x33:
      len=fgetu8(tapf);
      fseek(tapf,3*len,SEEK_CUR);
      return 0;
      
    case 0x34:
      fseek(tapf,8,SEEK_CUR);
      return 0;
      
    case 0x35:
      fseek(tapf,10,SEEK_CUR);
      len=fgetu32le(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
      
    case 0x40:
      fseek(tapf,9,SEEK_CUR);
      return 0;
      
    default:
      len=fgetu32le(tapf);
      fseek(tapf,len,SEEK_CUR);
      return 0;
  }
}

static int tzx_open_block(void) {
  if(block_open) return -1;

  block_start=ftell(tapf);
  if(block_start>=tapf_len) return -1;
  
  block_tzx_type=fgetu8(tapf);
  
  switch(block_tzx_type) {
    case 0x10: /* standard speed data block */
      block_dstart=block_start+5;
      block_type=BT_DATA;
      
      fseek(tapf,2,SEEK_CUR);
      block_dlen =fgetu16le(tapf);
      break;
      
    case 0x11: /* turbo loading data block */
      block_dstart=block_start+19;
      block_type=BT_DATA;
      
      fseek(tapf,15,SEEK_CUR);
      block_dlen =fgetu24le(tapf);
      break;
      
    case 0x12: /* pure tone */
      block_dstart=block_start+1;
      block_type=BT_TONES;
      block_dlen =4;
      break;
      
    case 0x13: /* sequence of pulses */
      block_dstart=block_start+2;
      block_type=BT_TONES;
      
      block_dlen =2*fgetu8(tapf);
      break;
      
    case 0x14: /* pure data block */
      block_dstart=block_start+11;
      block_type=BT_DATA;
      
      fseek(tapf,7,SEEK_CUR);
      block_dlen =fgetu24le(tapf);
      break;
      
    case 0x15: /* direct recording */
      block_dstart=block_start+9;
      block_type=BT_VOICE;
      
      fseek(tapf,5,SEEK_CUR);
      block_dlen =fgetu24le(tapf);
      
      voice_bbits=0;
      break;
    
    default: /* unknown */
      fseek(tapf,block_start,SEEK_SET);
      return -1;
  }
  
  fseek(tapf,block_dstart,SEEK_SET);
  block_end  =block_dstart+block_dlen;
  block_open =1;
  
  return 0;
}

static int tzx_close_block(void) {
  if(!block_open) return -1;
  
  fseek(tapf,block_end,SEEK_SET);
  block_open =0;
  return 0;
}
  
static int tzx_b_data_getbytes(int n, u8 *dst) {
  int bleft,r;

  if(!block_open) return -1;
  if(block_type!=BT_DATA) return -1;
    
  bleft=block_end-ftell(tapf);
  if(bleft<n) return -1;
    
  r=fread(dst,1,n,tapf);
  
  if(r<n) return -1;
  return 0;
}

static int tzx_b_voice_getsmps(int n, unsigned *dst) {
  int smpleft;
  int i;

//  printf("tzx_b_voice_getsmps\n");

  if(!block_open) return -1;
  if(block_type!=BT_VOICE) return -1;
    
  /* zatim ignorujeme used_bits */
  smpleft=(block_end-ftell(tapf))*8 + voice_bbits;
  if(smpleft<n) return -1;
  
  for(i=0;i<n;i++) {
    if(!voice_bbits) {
      voice_byte=fgetu8(tapf);
      voice_bbits=8;
    }
    dst[i]=(voice_byte & 0x80) ? 1 : 0;
    voice_byte<<=1;
  }
  
  return 0;
}

static int tzx_b_tones_gettone(int *pnum, int *plen) {
  unsigned pos;
  
  if(block_type!=BT_TONES) return -1;
  
  pos=ftell(tapf);
  if(pos>=block_end) return -1;
  
  switch(block_tzx_type) {
     case 0x12:
       *plen=fgetu16le(tapf);
       *pnum=fgetu16le(tapf);
       break;
       
     case 0x13:
       *plen=fgetu16le(tapf);
       *pnum=1;
       break;
  }
  
  return 0;
}
  
static int tzx_b_moredata(void) {
  if(!block_open) return -1;
  return ftell(tapf)<block_end;
}
