/*
 * GZX - George's ZX Spectrum Emulator
 * PCM playback through SDL
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

#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "../../mgfx.h"
#include "../../sndw.h"

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

