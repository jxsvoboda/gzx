/*
 * GZX - George's ZX Spectrum Emulator
 * FILE helper functions
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

#include <stdint.h>
#include <stdio.h>
#include "fileutil.h"

unsigned fgetu8(FILE *f) {
  uint8_t tmp;
  
  fread(&tmp,sizeof(tmp),1,f);
    
  return tmp;
}

void fungetu8(FILE *f, uint8_t c) {
  ungetc(c,f);
}

unsigned fgetu16le(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=tmp | (fgetu8(f)<<8);
  return tmp;
}

unsigned fgetu16be(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=fgetu8(f) | (tmp<<8);
  return tmp;
}

unsigned fgetu24le(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=tmp | (fgetu8(f)<<8);
  tmp=tmp | (fgetu8(f)<<16);
  return tmp;
}

unsigned fgetu32le(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=tmp | (fgetu8(f)<<8);
  tmp=tmp | (fgetu8(f)<<16);
  tmp=tmp | (fgetu8(f)<<24);
  return tmp;
}

unsigned fgetu32be(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=fgetu8(f) | (tmp<<8);
  tmp=fgetu8(f) | (tmp<<8);
  tmp=fgetu8(f) | (tmp<<8);
  return tmp;
}

signed fgets16le(FILE *f) {
  signed tmp;
    
  tmp=fgetu8(f);
  tmp=tmp | (fgetu8(f)<<8);
  if(tmp&0x8000) tmp=(tmp&0x7fff)-0x8000;
  return tmp;
}

void fputu8(FILE *f, uint8_t val) {
  fwrite(&val,sizeof(val),1,f);
}

void fputu16le(FILE *f, uint16_t val) {
  fputu8(f,val&0xff);
  fputu8(f,val>>8);
}

void fputu16be(FILE *f, uint16_t val) {
  fputu8(f,val>>8);
  fputu8(f,val&0xff);
}

unsigned long fsize(FILE *f) {
  unsigned long oldpos,flen;
  
  oldpos=ftell(f);
  fseek(f, 0, SEEK_END);
  flen=ftell(f);
  fseek(f, oldpos, SEEK_SET);
  
  return flen;
}
