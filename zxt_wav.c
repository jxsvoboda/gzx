/*
  GZX - George's ZX Spectrum Emulator
  Gerton Lunter's TAP fileformat support
*/

#include "global.h"
#include "zx_tape.h"
#include "zxt_fif.h"

#define ID_RIFF 0x52494646
#define ID_WAVE 0x57415645
#define ID_fmt  0x666d7420
#define ID_data 0x64617461

#define WAV_TAG_PCM 0x0001

static int wav_open_file(char *filename);
static int wav_close_file(void);
static int wav_rewind_file(void);
  
static int wav_block_type(void);
static int wav_get_b_data_info(tb_data_info_t *info);
static int wav_get_b_voice_info(tb_voice_info_t *voice);
  
static int wav_skip_block(void);
static int wav_open_block(void);
static int wav_close_block(void);
  
static int wav_b_data_getbytes(int n, u8 *dst);
static int wav_b_voice_getsmps(int n, unsigned *dst);
static int wav_b_tones_gettone(int *pnum, int *plen);
  
static int wav_b_moredata(void);


tfr_t tfr_wav = {
  wav_open_file,
  wav_close_file,
  wav_rewind_file,
  wav_block_type,
  wav_get_b_data_info,
  wav_get_b_voice_info,
  wav_skip_block,
  wav_open_block,
  wav_close_block,
  wav_b_data_getbytes,
  wav_b_voice_getsmps,
  wav_b_tones_gettone,
  wav_b_moredata
};

typedef struct {
  unsigned long start,len,end,id;
} wchnk;

static FILE *tapf;
static int tapf_len;
static int tapf_fb;

static int block_open;
static int block_start;
static wchnk block_chnk;

static void wav_chunk_start(wchnk *chnk);
static void wav_chunk_end(wchnk *chnk);

static wchnk riff_chnk,fmt_chnk;

static unsigned bytes_smp,bits_smp;
static unsigned smp_sec;

static int wav_open_file(char *filename) {
  u32 rid;
  unsigned fmttag,avgb_sec,blk_align;
  unsigned channels;
 

  tapf=fopen(filename,"rb");
  if(!tapf) return -1;
  tapf_len=fsize(tapf);
  
  block_open=0;
    
  wav_chunk_start(&riff_chnk);
  rid=fgetu32be(tapf);
  
  if(riff_chnk.id!=ID_RIFF || rid!=ID_WAVE) {
    printf("Not a RIFF WAVE file. %04lx!=%04x or %04lx!=%04x\n",
    riff_chnk.id,ID_RIFF,rid,ID_WAVE);
    return -1;
  }
  if(riff_chnk.end>tapf_len) {
    printf("Error in WAVE file (possibly truncated)\n");
    return -1;
  }
  
  /* search for fmt-chunk */
  wav_chunk_start(&fmt_chnk);
  while(fmt_chnk.id!=ID_fmt) {
    wav_chunk_end(&fmt_chnk);
    if(ftell(tapf)>=riff_chnk.end) return -1;
    wav_chunk_start(&fmt_chnk);
  }
  
  /* common entries */
  fmttag=fgetu16le(tapf);
  channels=fgetu16le(tapf);
  smp_sec=fgetu32le(tapf);
  avgb_sec=fgetu32le(tapf);
  blk_align=fgetu16le(tapf);
  
  if(fmttag!=WAV_TAG_PCM) {
    printf("WAV format is not PCM\n");
    return -1;
  }
  
  if(channels!=1) {
    printf("WAV is not mono (1-channel)\n");
    return -1;
  }
  
  /* PCM-specific entries */
  bits_smp=fgetu16le(tapf);
  
  if(bits_smp!=8 && bits_smp!=16) {
    printf("WAV PCM is not 8-bit nor 16-bit\n");
    return -1;
  }
  
  bytes_smp=bits_smp>>3;
  
  tapf_fb=ftell(tapf);
  
  printf(".wav opened\n");
  
  return 0;
}

static int wav_close_file(void) {
  return fclose(tapf);
}

static int wav_rewind_file(void) {
  if(fseek(tapf,tapf_fb,SEEK_SET)<0) return -1;
  block_open=0;
  return 0;
}
  
static int wav_block_type(void) {
  wchnk chnk;
  int pos;
  
  printf("wav_block_type\n");
  
  pos=ftell(tapf);
  if(pos>=riff_chnk.end) return BT_EOT;
  
  wav_chunk_start(&chnk);
  
  fseek(tapf,pos,SEEK_SET);
  
  if(chnk.id==ID_data) return BT_VOICE;
    else return BT_UNKNOWN;
}

static int wav_get_b_data_info(tb_data_info_t *info) {
  return -2;
}

static int wav_get_b_voice_info(tb_voice_info_t *info) {
  int pos;
  wchnk chnk;
  
  printf("wav_get_b_voice_info\n");
  
  if(block_open) return -1;
  
  pos=ftell(tapf);
  wav_chunk_start(&chnk);
  fseek(tapf,pos,SEEK_SET);
  
  fprintf(logfi,"wav_get_b_voice_info: bstart=%d(%ld) blen=%ld\n",
    pos,chnk.start,chnk.len);
  
  info->pause_after_len=ROM_PAUSE_LEN;
  info->samples=chnk.len/bytes_smp;
  info->smp_len=3500000/smp_sec;
  
  return 0;
}
  
static int wav_skip_block(void) {
  wchnk chnk;

  if(block_open) return -1;

  wav_chunk_start(&chnk);
  wav_chunk_end(&chnk);
  
  return 0;
}

static int wav_open_block(void) {
  if(block_open) return -1;

  block_start=ftell(tapf);
  
  if(block_start>=riff_chnk.end) return -1;
  
  wav_chunk_start(&block_chnk);
  block_open =1;
  
  return 0;
}

static int wav_close_block(void) {
  if(!block_open) return -1;
  
  wav_chunk_end(&block_chnk);
  block_open =0;
  return 0;
}
  
static int wav_b_data_getbytes(int n, u8 *dst) {
  return -2;
}

static int wav_b_voice_getsmps(int n, unsigned *dst) {
  int smpleft;
  int i;
  unsigned tmp_u;
  signed tmp_s;

//  printf("wav_b_voice_getsmps\n");

  if(!block_open) return -1;
    
  smpleft=(block_chnk.end-ftell(tapf))/bytes_smp;
  if(smpleft<n) return -1;
    
  if(bits_smp==8) { /* 8-bit unsigned */
    for(i=0;i<n;i++) {
      tmp_u=fgetu8(tapf);
      dst[i]=tmp_u>=128;
    }
  } else {
    for(i=0;i<n;i++) {
      tmp_s=fgets16le(tapf);
      dst[i]=tmp_s>=0;
    }
  }
  
  return 0;
}

static int wav_b_tones_gettone(int *pnum, int *plen) {
  return -2;
}
  
static int wav_b_moredata(void) {
  if(!block_open) return -1;
  return ftell(tapf)<block_chnk.end;
}

static void wav_chunk_start(wchnk *chnk) {
  chnk->id=fgetu32be(tapf);
  chnk->len=fgetu32le(tapf);
  chnk->start=ftell(tapf);
  chnk->end=chnk->start+chnk->len;
}

static void wav_chunk_end(wchnk *chnk) {
  fseek(tapf,chnk->end,SEEK_SET);
}
