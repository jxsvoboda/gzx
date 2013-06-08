/* 
  GZX - George's ZX Spectrum Emulator
  pcm playback through ALSA
  
  (very bad at the moment..)
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/asoundlib.h>
#include <errno.h>
#include "global.h"
#include "sndw.h"

static snd_pcm_t *pcm_handle = NULL;
static snd_pcm_info_t pcm_info;

static FILE *sndf;

static int alsa_pcm_open(int card, int device) {
  int res;
  
//  printf("open pcm %d.%d..\n",card,device);
  res = snd_pcm_open(&pcm_handle, card, device, SND_PCM_OPEN_PLAYBACK);
  if(res<0) {
    fprintf(stderr,"error opening pcm\n");
    return -1;
  }
  
//  printf("get pcm info..\n");
  res = snd_pcm_info(pcm_handle, &pcm_info);
  if(res<0) {
    fprintf(stderr,"error getting pcm info\n");
    return -1;
  }
  
  if(!(pcm_info.flags & SND_PCM_INFO_PLAYBACK)) {
    fprintf(stderr,"desired device does not support playback\n");
    return -1;
  }
  
  
//  printf("set non-block mode..\n");
  res = snd_pcm_nonblock_mode(pcm_handle,1);
  if(res<0) {
    fprintf(stderr,"error setting non-blocking mode\n");
    return -1;
  }
  
  sndf=fopen("snddump","wb");
  
  return 0;
}

static int alsa_pcm_set_params(void) {
  snd_pcm_channel_params_t params;
  int res;
  
  params.channel = SND_PCM_CHANNEL_PLAYBACK;
  params.mode    = SND_PCM_MODE_STREAM;
  params.format.interleave = 1;
  params.format.format     = SND_PCM_SFMT_U8;
  params.format.rate       = 28000/*28000*/;
  params.format.voices     = 1;
  /* params.digital = */
  params.start_mode        = SND_PCM_START_FULL;
  params.stop_mode         = SND_PCM_STOP_STOP;/*ROLLOVER*/
  params.time              = 0;
  params.ust_time          = 0;
  /* params.sync = */
  params.buf.stream.queue_size    = 560*2*2;
  params.buf.stream.fill          = SND_PCM_FILL_SILENCE_WHOLE/*NONE*/;
  params.buf.stream.max_fill      = 0;
  
  
//  printf("set pcm params..\n");
  res = snd_pcm_channel_params(pcm_handle, &params);
  if(res<0) {
    fprintf(stderr,"error setting pcm params\n");
    return -1;
  }
  
  return 0;
}

static int alsa_pcm_prepare_playback(void) {
  int res;
  
//  printf("start playback..\n");
  res = snd_pcm_playback_prepare(pcm_handle);
  if(res<0) {
    fprintf(stderr,"error preparing playback\n");
    return -1;
  }
  return 0;
}

static int alsa_pcm_write(void *data, int count) {
  int res;
  
//  fwrite(data,1,count,sndf);
  
  res=snd_pcm_write(pcm_handle,data,count);
  //printf("res=%d\n",res);
  if(res<0 && res!=-EAGAIN) {
    printf("res=%d\n",res);
    fprintf(stderr,"error writing data\n");
    return -1;
  }
  if(res==-EAGAIN) {
    printf("busy\n");
  }
  return res;
}

static void alsa_pcm_close(void) {
//  printf("close pcm..\n");
  snd_pcm_close(pcm_handle);
  fclose(sndf);
}

/**********************************************/

static int buf_size;

int sndw_init(int bufs) {
  buf_size=bufs;
 
  if(alsa_pcm_open(0,0)<0) return -1;
  if(alsa_pcm_set_params()<0) return -1;
  if(alsa_pcm_prepare_playback()<0) return -1;
    
  return 0;
}

void sndw_done(void) {
  alsa_pcm_close();
}

void sndw_write(u8 *buf) {
  snd_pcm_channel_status_t status;
  int res;
    
  status.channel=SND_PCM_CHANNEL_PLAYBACK;
  res= snd_pcm_channel_status(pcm_handle, &status);
  if(res) {
    fprintf(stderr,"error getting channel status\n");
  } else {
    if(status.underrun>0) printf("sound underruns:%d\n",status.underrun);
/*    printf("status= ");
    switch(status.status) {
      case SND_PCM_STATUS_NOTREADY: printf("NOTREADY\n"); break;
      case SND_PCM_STATUS_READY:    printf("READY\n"); break;
      case SND_PCM_STATUS_PREPARED: printf("PREPARED\n"); break;
      case SND_PCM_STATUS_RUNNING:  printf("RUNNING\n"); break;
      case SND_PCM_STATUS_UNDERRUN: printf("UNDERRUN\n"); break;
      default: printf("%d (unknown value)\n",status.status); break;
    }*/
  }

  if(status.status==SND_PCM_STATUS_UNDERRUN) {
    printf("underrun\n");
    snd_pcm_playback_prepare(pcm_handle);
  }

  alsa_pcm_write(buf,buf_size);
}
