/*
  WinGDI/DirectDraw wrapper
*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include "global.h"
#include "mgfx.h"

#define WINDOW_CLASS_NAME "gzxwin"
#define WINDOW_STYLE (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME)

static HWND hwnd;
static HANDLE hinst;
static HANDLE hmem;
static LPWNDCLASS winclass;
static HBITMAP scrbmp;
static HDC scrdc;

static int inited=0;

static unsigned char *vscr2;

static BITMAPINFO *bmi;
static RGBQUAD *wpal;

void mgfx_close(void);


static int *txkey;
static int txsize;

/* These are missing in my winuser.h */
#ifndef VK_A

#define VK_A 0x41
#define VK_B 0x42
#define VK_C 0x43
#define VK_D 0x44
#define VK_E 0x45
#define VK_F 0x46
#define VK_G 0x47
#define VK_H 0x48
#define VK_I 0x49
#define VK_J 0x4a
#define VK_K 0x4b
#define VK_L 0x4c
#define VK_M 0x4d
#define VK_N 0x4e
#define VK_O 0x4f
#define VK_P 0x50
#define VK_Q 0x51
#define VK_R 0x52
#define VK_S 0x53
#define VK_T 0x54
#define VK_U 0x55
#define VK_V 0x56
#define VK_W 0x57
#define VK_X 0x58
#define VK_Y 0x59
#define VK_Z 0x5a

#define VK_0 0x30
#define VK_1 0x31
#define VK_2 0x32
#define VK_3 0x33
#define VK_4 0x34
#define VK_5 0x35
#define VK_6 0x36
#define VK_7 0x37
#define VK_8 0x38
#define VK_9 0x39

#endif

static int ktabsrc[] = {
  VK_BACK,            WKEY_BS,
  VK_TAB,             WKEY_TAB,
  VK_CLEAR,           WKEY_DEL,
  VK_RETURN,          WKEY_ENTER,
  VK_KANA,            WKEY_,
  VK_SHIFT,           WKEY_LSHIFT,
  VK_CONTROL,         WKEY_LCTRL,
  VK_MENU,            WKEY_LALT,
  VK_PAUSE,           WKEY_BRK,
  VK_CAPITAL,         WKEY_CLOCK,
  VK_ESCAPE,          WKEY_ESC,
  VK_SPACE,           WKEY_SPACE,
  VK_PRIOR,           WKEY_PGUP,
  VK_NEXT,            WKEY_PGDN,
  VK_END,             WKEY_END,
  VK_HOME,            WKEY_HOME,
  VK_LEFT,            WKEY_LEFT,
  VK_UP,              WKEY_UP,
  VK_RIGHT,           WKEY_RIGHT,
  VK_DOWN,            WKEY_DOWN,
  VK_SELECT,          WKEY_,
  VK_EXECUTE,         WKEY_,
  VK_SNAPSHOT,        WKEY_PRNSCR,
  VK_INSERT,          WKEY_INS,
  VK_DELETE,          WKEY_DEL,
  VK_HELP,            WKEY_,
  VK_LWIN,            WKEY_LOS,
  VK_RWIN,            WKEY_ROS,
  VK_APPS,            WKEY_,

  VK_0,               WKEY_0,
  VK_1,               WKEY_1,
  VK_2,               WKEY_2,
  VK_3,               WKEY_3,
  VK_4,               WKEY_4,
  VK_5,               WKEY_5,
  VK_6,               WKEY_6,
  VK_7,               WKEY_7,
  VK_8,               WKEY_8,
  VK_9,               WKEY_9,

  VK_A,               WKEY_A,
  VK_B,               WKEY_B,
  VK_C,               WKEY_C,
  VK_D,               WKEY_D,
  VK_E,               WKEY_E,
  VK_F,               WKEY_F,
  VK_G,               WKEY_G,
  VK_H,               WKEY_H,
  VK_I,               WKEY_I,
  VK_J,               WKEY_J,
  VK_K,               WKEY_K,
  VK_L,               WKEY_L,
  VK_M,               WKEY_M,
  VK_N,               WKEY_N,
  VK_O,               WKEY_O,
  VK_P,               WKEY_P,
  VK_Q,               WKEY_Q,
  VK_R,               WKEY_R,
  VK_S,               WKEY_S,
  VK_T,               WKEY_T,
  VK_U,               WKEY_U,
  VK_V,               WKEY_V,
  VK_W,               WKEY_W,
  VK_X,               WKEY_X,
  VK_Y,               WKEY_Y,
  VK_Z,               WKEY_Z,

  VK_NUMPAD0,         WKEY_N0,
  VK_NUMPAD1,         WKEY_N1,
  VK_NUMPAD2,         WKEY_N2,
  VK_NUMPAD3,         WKEY_N3,
  VK_NUMPAD4,         WKEY_N4,
  VK_NUMPAD5,         WKEY_N5,
  VK_NUMPAD6,         WKEY_N6,
  VK_NUMPAD7,         WKEY_N7,
  VK_NUMPAD8,         WKEY_N8,
  VK_NUMPAD9,         WKEY_N9,

  VK_MULTIPLY,        WKEY_NSTAR,
  VK_ADD,             WKEY_NPLUS,
  VK_SEPARATOR,       WKEY_NPERIOD,
  VK_SUBTRACT,        WKEY_NMINUS,
  VK_DIVIDE,          WKEY_NSLASH,

  VK_F1,              WKEY_F1,
  VK_F2,              WKEY_F2,
  VK_F3,              WKEY_F3,
  VK_F4,              WKEY_F4,
  VK_F5,              WKEY_F5,
  VK_F6,              WKEY_F6,
  VK_F7,              WKEY_F7,
  VK_F8,              WKEY_F8,
  VK_F9,              WKEY_F9,
  VK_F10,             WKEY_F10,
  VK_F11,             WKEY_F11,
  VK_F12,             WKEY_F12,
  VK_F13,             WKEY_,
  VK_F14,             WKEY_,
  VK_F15,             WKEY_,
  VK_F16,             WKEY_,
  VK_F17,             WKEY_,
  VK_F18,             WKEY_,
  VK_F19,             WKEY_,
  VK_F20,             WKEY_,
  VK_F21,             WKEY_,
  VK_F22,             WKEY_,
  VK_F23,             WKEY_,
  VK_F24,             WKEY_,

  VK_NUMLOCK,         WKEY_NLOCK,
  VK_SCROLL,          WKEY_SLOCK,
  VK_LSHIFT,          WKEY_LSHIFT,
  VK_RSHIFT,          WKEY_RSHIFT,
  VK_LCONTROL,        WKEY_LCTRL,
  VK_RCONTROL,        WKEY_RCTRL,
  VK_LMENU,           WKEY_,
  VK_RMENU,           WKEY_,
  VK_PROCESSKEY,      WKEY_,
  VK_ATTN,            WKEY_,
  VK_CRSEL,           WKEY_,
  VK_EXSEL,           WKEY_,
  VK_EREOF,           WKEY_,
  VK_PLAY,            WKEY_,
  VK_ZOOM,            WKEY_,
  VK_NONAME,          WKEY_,
  VK_PA1,             WKEY_,

  0xBC,               WKEY_COMMA,
  0xBD,               WKEY_MINUS,
  0xBE,               WKEY_PERIOD,
  0xBF,               WKEY_SLASH,
  0xC0,               WKEY_GRAVE,
  0xDB,		      WKEY_LBR,
  0xDD,		      WKEY_RBR,
  0xBA,		      WKEY_SCOLON,
  0xDE,		      WKEY_FOOT, 
  0xDC,		      WKEY_BSLASH, /* je to dobre ? */

  -1,			-1
};

/* graphics */

static void mwin_draw(void) {
  HDC dc;
  PAINTSTRUCT ps;
  int sys;
  
  sys=scr_ys*(dbl_ln?2:1);
  
  dc=BeginPaint(hwnd,&ps);
  
  bmi->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  bmi->bmiHeader.biWidth=scr_xs;
  bmi->bmiHeader.biHeight=sys;
  bmi->bmiHeader.biPlanes=1;
  bmi->bmiHeader.biBitCount=8;
  bmi->bmiHeader.biCompression=BI_RGB;
  bmi->bmiHeader.biSizeImage=scr_xs*sys;
  bmi->bmiHeader.biXPelsPerMeter=0;
  bmi->bmiHeader.biYPelsPerMeter=0;
  bmi->bmiHeader.biClrUsed=0;
  bmi->bmiHeader.biClrImportant=0;
  
  scrbmp=CreateDIBitmap(dc,&bmi->bmiHeader,CBM_INIT,vscr2,bmi,DIB_RGB_COLORS);

  scrdc=CreateCompatibleDC(dc);
  SelectObject(scrdc,scrbmp);
  BitBlt(dc,0,0,scr_xs,sys,scrdc,0,0,SRCCOPY);
  DeleteDC(scrdc);
  
  DeleteObject(scrbmp);

  EndPaint(hwnd, &ps);
}


static LONG APIENTRY myproc(HWND hwnd, UINT msg, UINT wparam, LONG lparam) {

  switch(msg) {
    case WM_PAINT:
      //DefWindowProc(hwnd,msg,wparam,lparam);
      mwin_draw();
      break;

    case WM_CLOSE:
    case WM_DESTROY:
      exit(0);
      PostQuitMessage(0);
      return 0;

    case WM_KEYDOWN:
    case WM_KEYUP:
      printf("win VK=0x%02x\n",wparam);
      w_putkey(msg==WM_KEYDOWN,txkey[wparam],-1);
      if(wparam==VK_SHIFT) fprintf(logfi,"shift %d\n",msg==WM_KEYDOWN);
      if(wparam==VK_LSHIFT) fprintf(logfi,"lshift %d\n",msg==WM_KEYDOWN);
      if(wparam==VK_RSHIFT) fprintf(logfi,"rshift %d\n",msg==WM_KEYDOWN);
      if(wparam==VK_P) fprintf(logfi,"'p' %d\n",msg==WM_KEYDOWN);
      fflush(logfi);
      return 0;

    default:
      return DefWindowProc(hwnd,msg,wparam,lparam);
  }
  return 0;
}

void mgfx_problem(void) {
  mgfx_close();
  fprintf(stderr, "Problem!\n");
  exit(1);
}

extern FILE *logfi;

void mgfx_close(void) {
  if(!inited) return;
  inited=0;
}

int mgfx_init(void) {
  int i;
  int wxs,wys;
  int sxs,sys;
  RECT wrect;
  
  if(inited) return 0;
  
  scr_xs=320;
  scr_ys=200;
  
  mgfx_selln(3);
  
  sxs=scr_xs;
  sys=dbl_ln ? (scr_ys<<1) : scr_ys;

  /* Setup main window */
  hinst=GetModuleHandle(NULL);

  hmem=LocalAlloc(LPTR,sizeof(WNDCLASS));
  if(!hmem) exit(1);

  winclass=(LPWNDCLASS)LocalLock(hmem);

  winclass->style=0;
  winclass->lpfnWndProc=(WNDPROC)myproc;
  winclass->hInstance=hinst;
  winclass->hIcon=LoadIcon(NULL,IDI_APPLICATION);
//  winclass->hIconSm=LoadIcon(NULL,IDI_APPLICATION);
  winclass->hCursor=LoadCursor(NULL,IDC_ARROW);
  winclass->hbrBackground=GetStockObject(LTGRAY_BRUSH);
  winclass->lpszMenuName=(LPSTR)"";
  winclass->lpszClassName=(LPSTR)WINDOW_CLASS_NAME;

  if(!RegisterClass(winclass)) {
    exit(1);
  }

  /* urci velikost okna z velikosti vnitrku */
  wrect.left = 0;
  wrect.top = 0;
  wrect.right = sxs;
  wrect.bottom = sys;
  
  AdjustWindowRect(&wrect, WINDOW_STYLE, 0);
  wxs = wrect.right-wrect.left;
  wys = wrect.bottom-wrect.top;
  
  /* vytvori okno */
  
  if(!(hwnd=CreateWindow(WINDOW_CLASS_NAME,"GZX",
                 WINDOW_STYLE,
                 CW_USEDEFAULT,CW_USEDEFAULT,
		 wxs,wys,
                 NULL,
                 0,
                 hinst,
                 NULL))) {
                   exit(1);
                 }

  bmi=malloc(sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD));
  if(!bmi) exit(0);
  wpal=(RGBQUAD *) &bmi->bmiColors;

  /* setup keyboard translation table */
  
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
  
  w_initkey();
    
  /* setup virtual framebuffer */
  
  vscr0=malloc(scr_xs*scr_ys*sizeof(_U8));
  if(!vscr0) return 1;
  
  vscr1=malloc(scr_xs*scr_ys*sizeof(_U8));
  if(!vscr1) return 1;
 

  vscr2=malloc(sxs*sys*sizeof(_U8));
  if(!vscr2) return 1;
  

  clip_x0=clip_y0=0;
  clip_x1=scr_xs-1;
  clip_y1=scr_ys-1;
  
  ShowWindow(hwnd,SW_SHOWNORMAL);
  UpdateWindow(hwnd);
  
  inited=1;
  
  atexit(mgfx_close);
    
  return 0;
}


void mgfx_updscr(void) {
  unsigned char *sp,*dp;
  int y;

  if(!dbl_ln) {
    dp=vscr2+(scr_ys-1)*scr_xs;
    sp=vscr0;
  
    for(y=0;y<scr_ys;y++) {
      memcpy(dp,sp,scr_xs);
      dp-=scr_xs;
      sp+=scr_xs;
    }
  } else {
    dp=vscr2+(2*(scr_ys-1))*scr_xs;
    sp=vscr0;
  
    for(y=0;y<scr_ys;y++) {
      memcpy(dp,sp,scr_xs);
      dp-=scr_xs<<1;
      sp+=scr_xs;
    }
    
    dp=vscr2+(2*(scr_ys-1)+1)*scr_xs;
    sp=vscr1;
  
    for(y=0;y<scr_ys;y++) {
      memcpy(dp,sp,scr_xs);
      dp-=scr_xs<<1;
      sp+=scr_xs;
    }
  }
  RedrawWindow(hwnd,0,0,RDW_INVALIDATE);
}

void mgfx_setpal(int base, int cnt, int *p) {
  int i;
  
  for(i=base;i<base+cnt;i++) {
    wpal[i].rgbRed=(*p++)<<2;
    wpal[i].rgbGreen=(*p++)<<2;
    wpal[i].rgbBlue=(*p++)<<2;
  }
}

/* input */

void mgfx_input_update(void) {
  MSG msg;
  
  while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
    if(msg.message==WM_QUIT) exit(0);
    TranslateMessage(&msg);
    DispatchMessage(&msg);    
  }
}
