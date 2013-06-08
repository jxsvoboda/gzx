/*
  svgalib wrapper
*/

#include <stdio.h>
#include <stdlib.h>
#include <vga.h>
#include <vgakeyboard.h>
//#include <string.h>
//#include <stdarg.h>
#include "mgfx.h"

void my_handler(int scancode, int press);

int *txkey;
int txsize;
int ktabsrc[]= {
  SCANCODE_ESCAPE,		WKEY_ESC,
  SCANCODE_1,			WKEY_1,
  SCANCODE_2,			WKEY_2,
  SCANCODE_3,			WKEY_3,
  SCANCODE_4,			WKEY_4,
  SCANCODE_5,			WKEY_5,
  SCANCODE_6,			WKEY_6,
  SCANCODE_7,			WKEY_7,
  SCANCODE_8,			WKEY_8,
  SCANCODE_9,			WKEY_9,
  SCANCODE_0,			WKEY_0,
  SCANCODE_MINUS,		WKEY_MINUS,
  SCANCODE_EQUAL,		WKEY_EQUAL,
  SCANCODE_BACKSPACE,		WKEY_BS,
  SCANCODE_TAB,			WKEY_TAB,
  SCANCODE_Q,			WKEY_Q,
  SCANCODE_W,			WKEY_W,
  SCANCODE_E,			WKEY_E,
  SCANCODE_R,			WKEY_R,
  SCANCODE_T,			WKEY_T,
  SCANCODE_Y,			WKEY_Y,
  SCANCODE_U,			WKEY_U,
  SCANCODE_I,			WKEY_I,
  SCANCODE_O,			WKEY_O,
  SCANCODE_P,			WKEY_P,
  SCANCODE_BRACKET_LEFT,	WKEY_LBR,
  SCANCODE_BRACKET_RIGHT,	WKEY_RBR,
  SCANCODE_ENTER,		WKEY_ENTER,
  SCANCODE_LEFTCONTROL,		WKEY_LCTRL,
  SCANCODE_CONTROL,		WKEY_LCTRL,
  SCANCODE_A,			WKEY_A,
  SCANCODE_S,			WKEY_S,
  SCANCODE_D,			WKEY_D,
  SCANCODE_F,			WKEY_F,
  SCANCODE_G,			WKEY_G,
  SCANCODE_H,			WKEY_H,
  SCANCODE_J,			WKEY_J,
  SCANCODE_K,			WKEY_K,
  SCANCODE_L,			WKEY_L,
  SCANCODE_SEMICOLON,		WKEY_SCOLON,
  SCANCODE_APOSTROPHE,		WKEY_FOOT,
  SCANCODE_GRAVE,		WKEY_GRAVE,
  SCANCODE_LEFTSHIFT,		WKEY_LSHIFT,
  SCANCODE_BACKSLASH,		WKEY_BSLASH,
  SCANCODE_Z,			WKEY_Z,
  SCANCODE_X,			WKEY_X,
  SCANCODE_C,			WKEY_C,
  SCANCODE_V,			WKEY_V,
  SCANCODE_B,			WKEY_B,
  SCANCODE_N,			WKEY_N,
  SCANCODE_M,			WKEY_M,
  SCANCODE_COMMA,		WKEY_COMMA,
  SCANCODE_PERIOD,		WKEY_PERIOD,
  SCANCODE_SLASH,		WKEY_SLASH,
  SCANCODE_RIGHTSHIFT,		WKEY_RSHIFT,
  SCANCODE_KEYPADMULTIPLY,	WKEY_NSTAR,
  SCANCODE_LEFTALT,		WKEY_LALT,
  SCANCODE_SPACE,		WKEY_SPACE,
  SCANCODE_CAPSLOCK,		WKEY_CLOCK,
  SCANCODE_F1,			WKEY_F1,
  SCANCODE_F2,			WKEY_F2,
  SCANCODE_F3,			WKEY_F3,
  SCANCODE_F4,			WKEY_F4,
  SCANCODE_F5,			WKEY_F5,
  SCANCODE_F6,			WKEY_F6,
  SCANCODE_F7,			WKEY_F7,
  SCANCODE_F8,			WKEY_F8,
  SCANCODE_F9,			WKEY_F9,
  SCANCODE_F10,			WKEY_F10,
  SCANCODE_NUMLOCK,		WKEY_NLOCK,
  SCANCODE_SCROLLLOCK,		WKEY_SLOCK,
  SCANCODE_KEYPAD7,		WKEY_N7,
  SCANCODE_CURSORUPLEFT,	WKEY_N7,
  SCANCODE_KEYPAD8,		WKEY_N8,
  SCANCODE_CURSORUP,		WKEY_N8,
  SCANCODE_KEYPAD9,		WKEY_N9,
  SCANCODE_CURSORUPRIGHT,	WKEY_N9,
  SCANCODE_KEYPADMINUS,		WKEY_NMINUS,
  SCANCODE_KEYPAD4,		WKEY_N4,
  SCANCODE_CURSORLEFT,		WKEY_N4,
  SCANCODE_KEYPAD5,		WKEY_N5,
  SCANCODE_KEYPAD6,		WKEY_N6,
  SCANCODE_CURSORRIGHT,		WKEY_N6,
  SCANCODE_KEYPADPLUS,		WKEY_NPLUS,
  SCANCODE_KEYPAD1,		WKEY_N1,
  SCANCODE_CURSORDOWNLEFT,	WKEY_N1,
  SCANCODE_KEYPAD2,		WKEY_N2,
  SCANCODE_CURSORDOWN,		WKEY_N2,
  SCANCODE_KEYPAD3,		WKEY_N3,
  SCANCODE_CURSORDOWNRIGHT,	WKEY_N3,
  SCANCODE_KEYPAD0,		WKEY_N0,
  SCANCODE_KEYPADPERIOD,	WKEY_NPERIOD,
  SCANCODE_LESS,		WKEY_LESS,
  SCANCODE_F11,			WKEY_F11,
  SCANCODE_F12,			WKEY_F12,
  SCANCODE_KEYPADENTER,		WKEY_NENTER,
  SCANCODE_RIGHTCONTROL,	WKEY_RCTRL,
  SCANCODE_KEYPADDIVIDE,	WKEY_NSLASH,
  SCANCODE_PRINTSCREEN,		WKEY_PRNSCR,
  SCANCODE_RIGHTALT,		WKEY_RALT,
  SCANCODE_BREAK,		WKEY_BRK,
  SCANCODE_BREAK_ALTERNATIVE,	WKEY_BRK,
  SCANCODE_HOME,		WKEY_HOME,
  SCANCODE_CURSORBLOCKUP,	WKEY_UP,
  SCANCODE_PAGEUP,		WKEY_PGUP,
  SCANCODE_CURSORBLOCKLEFT,	WKEY_LEFT,
  SCANCODE_CURSORBLOCKRIGHT,	WKEY_RIGHT,
  SCANCODE_END,			WKEY_END,
  SCANCODE_CURSORBLOCKDOWN,	WKEY_DOWN,
  SCANCODE_PAGEDOWN,		WKEY_PGDN,
  SCANCODE_INSERT,		WKEY_INS,
  SCANCODE_REMOVE,		WKEY_DEL,
  SCANCODE_RIGHTWIN,		WKEY_LOS,
  SCANCODE_LEFTWIN,		WKEY_ROS,
  -1,				-1
};

/* graphics */

void w_vga_problem(void) {
  fprintf(stderr, "vga Problem!");
  exit(1);
}

void w_restoremode(void) {
  vga_setmode(TEXT);
}

void w_restorekeyboard(void) {
}

int mgfx_init(void) {
  int i;
  
  /* Initialize svgalib */
  if (vga_init()) {
    w_vga_problem();
  }
  
  scr_xs=320;
  scr_ys=200;
  
  if(vga_setmode(dbl_ln ? G320x400x256 : G320x200x256))
    w_vga_problem();
  
  atexit(w_restoremode);
  
  if(keyboard_init())
    w_vga_problem();
    
  atexit(w_restorekeyboard);  
  
  keyboard_seteventhandler(my_handler);
  w_initkey();
  
  /* set up key translation table */
  txsize=1;
  for(i=0;ktabsrc[i*2+1]!=-1;i++)
    if(ktabsrc[i*2]>=txsize) txsize=ktabsrc[i*2]+1;
    
  txkey=calloc(txsize,sizeof(int));
  if(!txkey) {
    printf("malloc failed\n");
    exit(1);
  }
  
  for(i=0;ktabsrc[i*2+1]!=-1;i++)
    txkey[ktabsrc[i*2]]=ktabsrc[i*2+1];
    
  /* set up virtual frame buffer */
  
  vscr0=malloc(scr_xs*scr_ys*sizeof(_U8));
  if(!vscr0) {
    printf("malloc failed\n");
    exit(1);
  }
  
  if(dbl_ln) {
    vscr1=malloc(scr_xs*scr_ys*sizeof(_U8));
    if(!vscr1) {
      printf("malloc failed\n");
      exit(1);
    }
  }
  
  mgfx_selln(3);

  clip_x0=clip_y0=0;
  clip_x1=scr_xs-1;
  clip_y1=scr_ys-1;
  
  return 0;
}

void mgfx_updscr(void) {
  unsigned u,pln,y;
  unsigned char *sp,*dp;
  unsigned char *src_scr;
  unsigned w_mask[2];
  
  w_mask[0]=write_l0;
  w_mask[1]=write_l1;
  
  if(dbl_ln) {
    for(y=0;y<scr_ys<<1;y++) {
      src_scr=(y&1) ? vscr1 : vscr0;
      if(w_mask[y&1])
        for(pln=0;pln<4;pln++) {
          vga_drawpixel(pln,y);
          dp=graph_mem+(scr_xs>>2)*y;
          sp=src_scr+scr_xs*(y>>1)+pln;
          for(u=0;u<scr_xs;u+=4)
          *dp++=sp[u];
        }
    }
  } else {
    vga_drawscansegment(vscr0,0,0,scr_xs*scr_ys*sizeof(_U8));
  }
}

void mgfx_setpal(int base, int cnt, int *p) {
  int pal[256*3];
  int i;
  
  for(i=0;i<cnt;i++) {
    pal[3*i]  =(*p++)/*<<2*/;
    pal[3*i+1]=(*p++)/*<<2*/;
    pal[3*i+2]=(*p++)/*<<2*/;
  }
  vga_setpalvec(base,cnt,pal);
}

/* input */

void my_handler(int scancode, int press) {
  w_putkey(press==KEY_EVENTPRESS,txkey[scancode],-1);
}

void mgfx_input_update(void) {
  keyboard_update();
}