/*
 * GZX - George's ZX Spectrum Emulator
 * Menus & file selector
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

#include <stdio.h>
#include <stdlib.h>
//#include <dirent.h>
#include <string.h>
//#include <sys/stat.h>
//#include <sys/fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "gzx.h"
#include "memio.h"
#include "mgfx.h"
#include "zx_tape.h"
#include "snap.h"
#include "menus.h"
#include "sys_all.h"
#include "sys_win.h"

#define MIN(x,y) ((x)<(y) ? (x) : (y))

/*** text editting line ***/

#define TELINE_MAX 64
typedef struct {
  char buf[TELINE_MAX+1];
  int maxlen,len,pos;
  int x,y;
  int focus;
} teline_t;

static void teline_empty(teline_t *t) {  
  t->len = t->pos = 0;
  memset(t->buf,' ',t->maxlen);
  t->buf[t->maxlen]=0;
}

static void teline_settext(teline_t *t, char *s) {
  int l;
  
  l=strlen(s);
  if(l>t->maxlen) l=t->maxlen;
  strncpy(t->buf,s,l);
  memset(t->buf+l,' ',t->maxlen-l);
  t->pos=t->len=l;
}


static void teline_init(teline_t *t, int x, int y, int maxlen) {
  if(maxlen>TELINE_MAX) maxlen = TELINE_MAX;
  t->maxlen = maxlen;

  t->x = x;
  t->y = y;
  t->focus=0;
  
  teline_empty(t);
}

static void teline_draw(teline_t *t) {
  int p = t->pos;
  
  fgc=7; bgc=0;
  gmovec(t->x,t->y);
  gputs(t->buf);
  if(t->focus) {
    gmovec(t->x+p,t->y);
    fgc=0; bgc=5;
    gputc(t->buf[p]);
  }
}

static void teline_rmchar(teline_t *t, int pos) {
  int i;
  
  for(i=pos;i<t->len-1;i++)
    t->buf[i]=t->buf[i+1];
  t->buf[-- t->len] = ' ';
}

static void teline_key(teline_t *t, wkey_t *k) {
  if(!k->press) return;
  switch(k->key) {
    case WKEY_BS:
      if(t->pos>0)
        teline_rmchar(t,--t->pos);
      break;
    case WKEY_DEL:
      if(t->pos<t->len)
        teline_rmchar(t,t->pos);
      break;
    case WKEY_LEFT:
      if(t->pos>0) t->pos--;
      break;
    case WKEY_RIGHT:
      if(t->pos<t->len) t->pos++;
      break;
      
    default:
      if(k->c>=' ' && k->c<128 && t->len < t->maxlen) {
      int i;
    
      for(i=t->len;i>t->pos;i--)
        t->buf[i] = t->buf[i-1];
      
      t->buf[t->pos++]=k->c;
      t->len++;
    }
    break;
  }
}

/***** file/dir selector *****/

#define F_DIR 0x01
#define F_SEL 0x02

typedef struct {
  unsigned char flags;
  char *name;
} mde;

static mde *flist;
static int nent;
static int fsscrpos, fscurspos;
static int fsscrlines,fscols;
static int flist_cx0;
static char *sf;

#define QSBUF_MAXL 64
static char qsbuf[QSBUF_MAXL+1];
static int qsbuf_l;

static char fs_cwd[SYS_PATH_MAX+1];

static teline_t fn_line;

static char *fsel_caption;
static int end_fsel,ack_fsel;

static int fsel_fnsearch(char *name, int n);
static void fsel_setpos(int pos);

static void fsel_set_fnline(void) {
 teline_settext(&fn_line,flist[fscurspos].name);
}

/*
  Draw a string / width=n (pad/cut)
*/
static void gputs_n(int n, char *s) {
  while(n>0) {
    gputc(*s);
    if(*s) s++;
    n--;
  }
}

static void fsel_draw_files(int x, int y) {
  int i;
  int c,d;
  char *s;
  
  for(i=0;i<fsscrlines;i++) {
    c = fscurspos-fsscrpos==i;
    
    if(i+fsscrpos<nent) {
      s=flist[i+fsscrpos].name;
      d = flist[i+fsscrpos].flags & F_DIR;
    } else {
      d=0;
      s="";
    }
    
    if(fn_line.focus) {
      fgc = c?5:(d?6:7);
      bgc = 0;
    } else {
      fgc = c?0:(d?6:7);
      bgc = c?5:0;
    }
    gmovec(x,y+i);
    gputs_n(fscols,s);
    if(c) {
      gmovec(x,y+i);
      fgc=2; bgc=4;
      gputs(qsbuf);
    }
  }
}

static void freeflist(mde **flist, int nent) {
  int i;

  fprintf(logfi,"freeing file list..\n"); fflush(logfi);
  if(nent>0) {
    for(i=0;i<nent;i++)
      free((*flist)[i].name);

    free(*flist);
  }

  *flist=NULL;

  fprintf(logfi,"..done\n"); fflush(logfi);
}

static int my_select(char *name) {
  if(!name) return 0;
  if(strcmp(name,"..")==0) return 1;
  if(name[0]=='.') return 0;
  return 1;
}

static int my_sort(const void *va, const void *vb) {
  const mde *a=va,*b=vb;
  int i;
  
  i=((b->flags&F_DIR)!=0)-((a->flags&F_DIR)!=0);
  if(i==0) i=strcmp(a->name,b->name);
  return i;
}

static void add_entry(char *fname, unsigned flags) {
  int nlen;
  
  if(!(flist=realloc(flist,(nent+1)*sizeof(mde)))) {
    printf("malloc failed\n");
  }
  nlen=strlen(fname);
  if(!(flist[nent].name=malloc(nlen+1))) {
    printf("malloc failed\n");
  }
  strcpy(flist[nent].name,fname);
  flist[nent].flags=flags;
  nent++;
}

static void get_dir(void) {
  char *fname;
  int is_dir;

  fprintf(logfi,"open dir\n"); fflush(logfi);
  if(sys_opendir(".")<0) {
    return;
  }

  flist=NULL;

  fprintf(logfi,"reading dir\n"); fflush(logfi);
  nent=0;
  while(sys_readdir(&fname,&is_dir)>=0) {
    if(my_select(fname)) {
      add_entry(fname,
	is_dir ? F_DIR : 0);
    }
  }

  qsort(flist,nent,sizeof(mde),my_sort);
  
#ifdef __MINGW32__
  {
    unsigned long dmask;
    int i;
    char dstr[4];
    
    strcpy(dstr,"X:\\");
    dmask = win_enumdrives();
    for(i=0;i<26;i++)
      if(dmask & (1<<i)) {
        dstr[0]='A'+i;
        add_entry(dstr,F_DIR);
      }
  }
#endif
  
  sys_closedir();
  fprintf(logfi,"finished reading dir\n");
}

static int fs_select(char *name) {

  fprintf(logfi,"selected file '%s'\n",name); fflush(logfi);

  if(!strcmp(name,"..")) {
    char *cur_dir,*p,*q;
    
    fprintf(logfi,"'..' dir! entering\n"); fflush(logfi);
    
    /* extract current directory name */
    cur_dir = sys_getcwd(NULL,0);
    p=strrchr(cur_dir,'/');
    q=strrchr(cur_dir,'\\');
    if(!p) p=q;
      else if(!!q && q>p) p=q;
      
    if(!!p && *p) ++p;

    if(sys_chdir(name)<0) {
      fprintf(logfi,"chdir failed\n");
      free(cur_dir);
      return -1;
    }

    /* find it in the current directory */
    if(p) fprintf(logfi,"jump to entry '%s'\n",p);
    if(!!p && *p) {
      int i;
      freeflist(&flist,nent);
      get_dir();
      fscurspos=0;
      fsscrpos=0;
      
      i = fsel_fnsearch(p,strlen(p)+1);
      if(i>=0) fsel_setpos(i);
    }
    free(cur_dir);

    if(!sys_getcwd(fs_cwd,SYS_PATH_MAX)) fs_cwd[0]=0;
    return 0;
  }
  
  if(sys_isdir(name)) {

    fprintf(logfi,"a directory! entering..\n"); fflush(logfi);
    if(sys_chdir(name)<0) {
      fprintf(logfi,"chdir failed\n");
      return -1;
    }
    freeflist(&flist,nent);
    get_dir();
    fscurspos=0;
    fsscrpos=0;
    if(!sys_getcwd(fs_cwd,SYS_PATH_MAX)) fs_cwd[0]=0;
  } else {
    fprintf(logfi,"a file! selecting..\n"); fflush(logfi);
    sf=name;
    return 1;
  }
  return 0;
}

static void fsel_draw(void) {
  int l;

  mgfx_fillrect(flist_cx0*8-8,0,flist_cx0*8+8*(fscols+1),scr_ys-1,1);
  fsel_draw_files(flist_cx0,2);
  teline_draw(&fn_line);
//  mgfx_fillrect(0,8*23,320,199,7);
  gmovec(scr_xs/16-(strlen(fsel_caption)/2),0);
  fgc=0; bgc=7;
  gputs(fsel_caption);
  
  /* Display current directory path */
  gmovec(flist_cx0,1); bgc=2;
  l=strlen(fs_cwd);
  if(l<=fscols) {
    fgc=7;
    gputs_n(fscols,fs_cwd);
  } else {
    fgc=5; gputc('<'); fgc=7;
    gputs_n(fscols-1,fs_cwd+l-(fscols-1));
  }
//  if(sf)
//  gputs(0,8*24,7,0,sf);
}

static void fsel_nameline(void) {
  int end = 0;
  int res;
  wkey_t k;
  
  fn_line.focus = 1;

  while(!end) {
    fsel_draw();
    mgfx_updscr();
    do {
      mgfx_input_update();
      usleep(1000);
    } while(!w_getkey(&k));
    
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
        end=1; end_fsel=1;
	break;
      case WKEY_TAB:
	end=1;
	break;

      case WKEY_ENTER:
        fn_line.buf[fn_line.len]=0;
        res = fs_select(fn_line.buf);
	if(res>0) { end=1; end_fsel=1; }
        if(fn_line.len<fn_line.maxlen)
          fn_line.buf[fn_line.len]=' ';
	if(res>=0)
	  teline_empty(&fn_line);
	break;
        
      default:
        teline_key(&fn_line,&k);
	break;
    }
  }
  
  fn_line.focus = 0;
}

static void fsel_setpos(int pos) {
  if(pos<0) pos=0;
  if(pos>=nent) pos=nent-1;
  fscurspos=pos;

  if(pos<fsscrpos) fsscrpos=pos;
    else if(pos>=fsscrpos+fsscrlines) {
      fsscrpos = pos-fsscrlines+1;
      if(fsscrpos<0) fsscrpos=0;
    }
    
  fsel_set_fnline();
}

/*
  Finds a file in the current directory that matches
  the first n characters of name.

  Returns it's index, or -1 if no match is found.
*/
int fsel_fnsearch(char *name, int n) {
  int i;
  
  /* search from current position */
  i=fscurspos;
  while(i<nent && strncmp(name,flist[i].name,n)) i++;
  
  /* no match -> try searching from the beginning */
  if(i>=nent) {
    i=0;
    while(i<nent && strncmp(name,flist[i].name,n)) i++;
  }
  /* match? */
  if(i<nent) return i;
    else return -1;
}

static void fsel_qs_insert(char c) {
  int i;
  
  if(qsbuf_l>=QSBUF_MAXL) return;
  qsbuf[qsbuf_l++]=c;
  qsbuf[qsbuf_l]=0;

  /* try finding a file that matches this prefix */
  i = fsel_fnsearch(qsbuf,qsbuf_l);
  
  /* match? -> set position */
  if(i>=0) {
    fsel_setpos(i);
  } else {
    qsbuf[--qsbuf_l]=0;
  }
}

static void fsel_qs_empty(void) {
  qsbuf[0]=0;
  qsbuf_l=0;
}


int file_sel(char **fname, char *caption) {
  int res;
  char *s;
  wkey_t k;
    
  sf=NULL; /* umlc warning */

  if(!sys_getcwd(fs_cwd,SYS_PATH_MAX)) fs_cwd[0]=0;
  fsel_qs_empty();

  end_fsel=ack_fsel=0;
  
  fsscrpos=0;
  fscurspos=0;
  fsscrlines=21;
  fscols=20;

  flist_cx0 = scr_xs/16 - fscols/2;
  teline_init(&fn_line,flist_cx0,24,20);
  fn_line.focus=0;
  fsel_caption = caption;

  get_dir();

  fsel_set_fnline();

  mgfx_selln(3); /* Enable rendering odd and even lines */

  while(!end_fsel) {
    fsel_draw();
    mgfx_updscr();
    do {
      mgfx_input_update();
      usleep(1000);
    } while(!w_getkey(&k));
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
        end_fsel=1;
	ack_fsel=0;
	break;
	
      case WKEY_ENTER:
      	fsel_qs_empty();
	fprintf(logfi,"enter pressed\n"); fflush(logfi);
	s=flist[fscurspos].name;
	if(fs_select(s)) {
	  end_fsel=1;
	  ack_fsel=1;
	}
	fsel_set_fnline();
	break;
	
      case WKEY_UP:
        fsel_setpos(fscurspos-1);
	fsel_qs_empty();
	break;

      case WKEY_DOWN:
        fsel_setpos(fscurspos+1);
	fsel_qs_empty();
	break;
	
      case WKEY_PGUP:
        fsel_setpos(fscurspos-fsscrlines+1);
	fsel_qs_empty();
	break;
	
      case WKEY_PGDN:
        fsel_setpos(fscurspos+fsscrlines-1);
	fsel_qs_empty();
	break;
	
      case WKEY_HOME:
        fsel_setpos(0);
	fsel_qs_empty();
	break;

      case WKEY_END:
        fsel_setpos(nent-1);
	fsel_qs_empty();
	break;

      case WKEY_TAB:
	fsel_nameline();
        fsel_set_fnline();
	fsel_qs_empty();
	break;

      case WKEY_BS:
        if(qsbuf_l>0)
	  qsbuf[--qsbuf_l]=0;
	break;

      default:
        if(k.c>=' ' && k.c<128)
	  fsel_qs_insert(k.c);
	break;
    }
  }
  
  if(ack_fsel) {
    *fname=malloc(strlen(sf)+1);
    if(!*fname) {
      fprintf(logfi,"malloc failed\n");
      exit(1);
    }
    strcpy(*fname,sf);
    res=1;
  } else {
    *fname=NULL;
    res=0;
  }

  fprintf(logfi,"freeing file list...\n"); fflush(logfi);
  freeflist(&flist,nent);
  fprintf(logfi,"ok. (%s)\n",*fname); fflush(logfi);

  fsel_caption = NULL;

  return res;
}

/***** main menu *****/

#define MENU_NENT 7

static int menu_cx0;

static char *mentry_text[MENU_NENT]= {
  "~Load Snapshot",
  "~Save Snapshot",
  "Select ~Tapefile",
  "Reset ~48",
  "Reset ~128",
  "Toggle ~Windowed",
  "~Quit",
};

static int mkeys[MENU_NENT]={
  WKEY_L,WKEY_S,WKEY_T,WKEY_4,WKEY_1,WKEY_W,WKEY_Q
};


/*
  Draw a string at fixed width, with ~=highlight
*/
static void gputs_hl(int n, int fgc_, int hlc, int bgc_, char *s) {
  bgc=bgc_;
  while(n>0) {
    if(*s=='~') { fgc=hlc; s++; }
      else fgc=fgc_;
      
    gputc(*s);
    if(*s) s++;
    n--;
  }
}

//#include <alloc.h>
static void menu_draw(int mpos) {
  int i;
  int fgc_,bgc_,hlc_;
//  char buf[32];

  mgfx_fillrect(menu_cx0*8,0,menu_cx0*8+8*20,scr_ys-1,1);
  gmovec(scr_xs/16-(strlen("Main Menu")/2),0);
  fgc=7; bgc=1; gputs("Main Menu");

  for(i=0;i<MENU_NENT;i++) {
    if(i==mpos) { fgc_=hlc_=1; bgc_=5; }
      else { fgc_=7; hlc_=14; bgc_=1; }
    gmovec(menu_cx0+1,2+i);
    gputs_hl(18,fgc_,hlc_,bgc_,mentry_text[i]);
  }

//  sprintf(buf,"%lu",coreleft());
//  gputs(scr_xs/2-8*4,scr_ys-8,10,0,buf);
}

static void menu_run_line(int l) {
  switch(l) {
    case 0: load_snap_dialog(); break;
    case 1: save_snap_dialog(); break;
    case 2: select_tapefile_dialog(); break;
    case 3: zx_select_memmodel(ZXM_48K); zx_reset(); break;
    case 4: zx_select_memmodel(ZXM_128K); zx_reset(); break;
    case 5: mgfx_toggle_fs(); break;
    case 6: quit=1; break;
  }
}

void main_menu(void) {
  wkey_t k;
  int end_menu = 0;
  int j;
  int mpos;
  
  mpos=0;
  menu_cx0=scr_xs/16 - 10;
  
  mgfx_selln(3); /* Enable rendering odd and even lines */

  while(!end_menu) {
    menu_draw(mpos);
    mgfx_updscr();
    do {
      mgfx_input_update();
      usleep(1000);
    } while(!w_getkey(&k));
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
        end_menu=1;
	break;

      case WKEY_ENTER:
	menu_run_line(mpos);
	end_menu=1;
	break;
	
      case WKEY_UP:
        if(mpos>0) --mpos;
	  else mpos=MENU_NENT-1;
	break;
	
      case WKEY_DOWN:
        if(mpos<MENU_NENT-1) ++mpos;
	  else mpos=0;
	break;

      case WKEY_PGUP:
        mpos=0;
	break;
	
      case WKEY_PGDN:
        mpos=MENU_NENT-1;
	break;

      default:
	for(j=0;j<MENU_NENT;j++)
	  if(k.key==mkeys[j]) {
	    menu_run_line(j);
	    end_menu=1;
	  }
	break;
    }
  }
}

/***** tape menu *****/

#define TMENU_NENT 4

static char *tmentry_text[TMENU_NENT]= {
  "~Play",
  "~Stop",
  "~Rewind",
  "~Quick Load Toggle"
};

static int tmkeys[TMENU_NENT]={
  WKEY_P,WKEY_S,WKEY_R,WKEY_Q
};

static void tmenu_draw(int mpos) {
  int i;
  int fgc,bgc,hlc;
  
  mgfx_fillrect(menu_cx0*8,0,menu_cx0*8+8*20,scr_ys-1,1);
  gmovec(scr_xs/16-(strlen("Tape Menu")/2),0);
  fgc=7; bgc=1; gputs("Tape Menu");
 
  for(i=0;i<TMENU_NENT;i++) {
    if(i==mpos) { fgc=hlc=1; bgc=5; }
      else { fgc=7; hlc=14; bgc=1; }
    gmovec(menu_cx0+1,2+i);
    gputs_hl(18,fgc,hlc,bgc,tmentry_text[i]);
  }
}

static void tmenu_run_line(int l) {
  switch(l) {
    case 0: zx_tape_play(); break;
    case 1: zx_tape_stop(); break;
    case 2: zx_tape_rewind(); break;
    case 3: slow_load=!slow_load; break;
  }
}

void tape_menu(void) {
  wkey_t k;
  int end_menu = 0;
  int j;
  int mpos;

  mpos=0;
  menu_cx0=scr_xs/16 - 10;
  
  mgfx_selln(3); /* Enable rendering odd and even lines */
  
  while(!end_menu) {
    tmenu_draw(mpos);
    mgfx_updscr();
    do {
      mgfx_input_update();
      usleep(1000);
    } while(!w_getkey(&k));
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
        end_menu=1;
	break;
	
      case WKEY_ENTER:
        tmenu_run_line(mpos);
	end_menu=1;
	break;
	
      case WKEY_UP:
        if(mpos>0) --mpos;
	  else mpos=TMENU_NENT-1;
	break;

      case WKEY_DOWN:
        if(mpos<TMENU_NENT-1) ++mpos;
	  else mpos=0;
	break;
	
      case WKEY_PGUP:
        mpos=0;
	break;
	
      case WKEY_PGDN:
	mpos=MENU_NENT-1;
	break;

      default:
	for(j=0;j<TMENU_NENT;j++)
	  if(k.key==tmkeys[j]) {
	    tmenu_run_line(j);
	    end_menu=1;
	  }
	break;
    }
  }
}


/**** select tapefile dialog *****/

void select_tapefile_dialog(void) {
  char *fname;
  
  if(file_sel(&fname,"Select Tapefile")>0) {
    fprintf(logfi,"selecting tape file\n"); fflush(logfi);
    zx_tape_selectfile(fname);
    fprintf(logfi,"freeing filename\n"); fflush(logfi);
    free(fname);
  }
}

void load_snap_dialog(void) {
  char *fname;
  
  if(file_sel(&fname,"Load Snapshot")>0) {
    zx_load_snap(fname);
    free(fname);
  }
}

void save_snap_dialog(void) {
/*  char *fname;

  if(file_sel(&fname,"Load Snapshot")>0) {
    zx_load_snap(fname);
    free(fname);
  }*/
  wkey_t k;
  
  fscols=20;
  flist_cx0 = scr_xs/16 - fscols/2;
  teline_init(&fn_line,flist_cx0,12,20);
  fn_line.focus = 1;
    
  while(1) {
    mgfx_fillrect(flist_cx0*8-8,0,flist_cx0*8+8*(fscols+1),scr_ys-1,1);
    teline_draw(&fn_line);
    gmovec(scr_xs/16-(strlen("Save Snapshot")/2),0);
    fgc=7; bgc=1; gputs("Save Snapshot");

    mgfx_updscr();
    do {
      mgfx_input_update();
      usleep(1000);
    } while(!w_getkey(&k));
    
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
	return;

      case WKEY_ENTER:
        fn_line.buf[fn_line.len]=0;
	zx_save_snap(fn_line.buf);
	return;
        
      default:
        teline_key(&fn_line,&k);
	break;
    }
  }
  
//  zx_save_snap("testx.z80");
}
