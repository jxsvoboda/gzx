/* 
  GZX - George's ZX Spectrum Emulator
  sound output
  
  currently plays something like the SPK
  through the sound card
*/

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "sndw.h"
#include "zx_sound.h"

static u8 *snd_buf;
static int snd_bufs,snd_bff;

int zx_sound_init(void) {
  snd_bufs=560*2;

  if(sndw_init(snd_bufs)<0) return -1;
    
  snd_bff=0;
  snd_buf=malloc(snd_bufs);

  if(!snd_buf) {
    fprintf(stderr,"malloc failed\n");
    return -1;
  }
  return 0;
}

void zx_sound_done(void) {
  sndw_done();
  free(snd_buf);
}

void zx_sound_smp(int ay_out) {
  
  /* mixing */
  snd_buf[snd_bff++]=128 + ay_out + (spk?-16:+16)+(mic?-16:+16);
  
  if(snd_bff>=snd_bufs) {
    snd_bff=0;
    
    sndw_write(snd_buf);
  }
}
