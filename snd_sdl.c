/* 
  GZX - George's ZX Spectrum Emulator
  pcm playback through SDL
*/

#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "mgfx.h"
#include "sndw.h"

static u8 *audio_ring;
static int audio_bufsize;
static int audio_ringsize;
static int bin, bout, bcnt;
static int paused;

static void sdl_audio_cb(void *userdata, Uint8 *stream, int len)
{
  int xfer;
  
  if (bcnt < len) {
    printf("Audio buffer ring empty, filling silence and pausing.\n");
    memset(stream, 128, len);
    if (!paused) {
      paused = 1;
      SDL_PauseAudio(1);
    }
    return;
  }
  
  while (len > 0) {
    xfer = bout + len > audio_ringsize ? audio_ringsize - bout : len;
    memcpy(stream, audio_ring + bout, xfer);
    stream += xfer;
    bout += xfer;
    bcnt -= xfer;
    len -= xfer;
    if (bout == audio_ringsize)
      bout = 0;
  }

}

int sndw_init(int bufs) {
  SDL_AudioSpec desired;
  
  audio_ringsize = bufs * 3;
  audio_ring = calloc(1, audio_ringsize);
  if (audio_ring == NULL)
    goto error;
  
  audio_bufsize = bufs;
  
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    goto error;
  
  desired.freq = 28000;
  desired.format = AUDIO_U8;
  desired.channels = 1;
  desired.samples = bufs * 3;
  desired.callback = sdl_audio_cb;
  desired.userdata = NULL;
  
  bin = bout = 0;
  paused = 1;
  
  if (SDL_OpenAudio(&desired, NULL) < 0)
    goto error;
  
  return 0;
  
error:
  free(audio_ring);
  return -1;
}

void sndw_done(void) {
  SDL_CloseAudio();
}

void sndw_write(u8 *buf) {
  SDL_LockAudio();
  while (bcnt + audio_bufsize > audio_ringsize) {
    SDL_UnlockAudio();
    if (paused) {
      printf("Starting playback.\n");
      SDL_PauseAudio(0);
      paused = 0;
    }
    usleep(10000);
    SDL_LockAudio();
  }
  
  memcpy(audio_ring + bin, buf, audio_bufsize);
  bcnt += audio_bufsize;
  bin += audio_bufsize;
  if (bin >= audio_ringsize)
    bin = 0;
  SDL_UnlockAudio();
}

