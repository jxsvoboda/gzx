/*
 * FILE helper functions
 */

#include <stdio.h>
#include "fileutil.h"
#include "global.h"

unsigned fgetu8(FILE *f) {
  u8 tmp;
  
  fread(&tmp,sizeof(tmp),1,f);
    
  return tmp;
}

void fungetu8(FILE *f, u8 c) {
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

void fputu8(FILE *f, u8 val) {
  fwrite(&val,sizeof(val),1,f);
}

void fputu16le(FILE *f, u16 val) {
  fputu8(f,val&0xff);
  fputu8(f,val>>8);
}

void fputu16be(FILE *f, u16 val) {
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
