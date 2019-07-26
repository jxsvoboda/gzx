/*
 * GZX - George's ZX Spectrum Emulator
 * WinGDI/DirectDraw graphics
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

/*
  This module tries to load(run-time) and initialise DirectDraw (ddraw.dll)
  and use it to setup a fullscreen display mode (320x200 or 640x400).

  If this fails, it reverts to using GDI only and the program runs
  in a window.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include "../../gzx.h"
#include "../../mgfx.h"

#include <ddraw.h>

#define WINDOW_CAPTION "GZX"
#define WINDOW_CLASS_NAME "gzxwin"

/* this style is for the windowed mode */
#define WINDOW_STYLE (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME)

/* run-time ddraw loading */
typedef HRESULT WINAPI (*DDC_PROC)(
  GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);
static HMODULE ddlib;
static DDC_PROC ddc_proc;

/* GDI stuff */
static HWND hwnd;
static HANDLE hinst;
static HBITMAP scrbmp;
static HDC scrdc;
static LPWNDCLASS winclass;
static LPWNDCLASSEX winclassex;

static int inited=0;

static int sxs,sys;
static unsigned char *vscr2;

static BITMAPINFO *bmi;
static RGBQUAD *wpal;

static LPDIRECTDRAW lpdd;

static LPDIRECTDRAWSURFACE lpDDSPrimary; // DirectDraw primary surface
static LPDIRECTDRAWSURFACE lpDDSBack; // DirectDraw back surface

void mgfx_close(void);
static LONG APIENTRY myproc(HWND hwnd, UINT msg, UINT wparam, LONG lparam);


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
  0xDC,		      WKEY_BSLASH, /* Is this correct? */

  -1,			-1
};

/* graphics */

/*
  Link ddraw.dll and get the required
  function adresses. Handles possibility of multiple call.
*/
static int dd_rtload(void) {
  if(ddlib==NULL) {
    ddlib = LoadLibrary("ddraw.dll");
    if(ddlib==NULL) return -1;
  
    ddc_proc = (DDC_PROC)GetProcAddress(ddlib, TEXT("DirectDrawCreate"));
    if(ddc_proc==NULL) return -1;
  }
  return 0;
}

static void dd_close(void) {
  if(lpdd) {
    if(lpDDSPrimary) {
      IDirectDrawSurface_Release(lpDDSPrimary);
      lpDDSPrimary=NULL;
    }

    IDirectDraw_Release(lpdd);
    lpdd=NULL;
  }
}

/* Create primary DirectDraw surface */
static int c_prim_surf(void) {
  DDSURFACEDESC ddsd;
  LPDDSURFACEDESC lpddsd = &ddsd;
  DDSCAPS ddscaps;
  HRESULT ddrval;

  // Create the primary surface with 1 back buffer
  memset( &ddsd, 0, sizeof(ddsd) );
  ddsd.dwSize = sizeof( ddsd );
  ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
  ddsd.dwBackBufferCount = 1;

  ddrval = IDirectDraw_CreateSurface( lpdd, lpddsd, &lpDDSPrimary, NULL );

  if( ddrval != DD_OK )
    return -1;

  // Get the pointer to the back buffer
  ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
  ddrval = IDirectDrawSurface_GetAttachedSurface(lpDDSPrimary,&ddscaps, &lpDDSBack);
  
  if( ddrval != DD_OK )
    return -1;

  return 0;
}

/* Initialise DirectDraw */
static int dd_init(HWND wnd) {
  HRESULT ddrval;
  int ddxr,ddyr;

  ddrval=ddc_proc(NULL,&lpdd,NULL); /* DirectDrawCreate */
  if(ddrval!=DD_OK) return -1;

  ddrval=IDirectDraw_SetCooperativeLevel(lpdd,wnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
  if(ddrval!=DD_OK) {
    dd_close();
    return -1;
  }

  if(dbl_ln) {
    ddxr=640; ddyr=400;
  } else {
    ddxr=640; ddyr=400;
  }
  ddrval=IDirectDraw_SetDisplayMode(lpdd,ddxr,ddyr,/*8*/16);
  if(ddrval!=DD_OK) {
    dd_close();
    return -1;
  }
  return 0;
}


/* Create a full-screen window with a DirectDraw context,
   set video mode. */
static int mwin_init_ddraw(void) {  
  HANDLE hmem; 
  
  /* load ddraw.dll first */
  if(dd_rtload()<0) return -1;
  
  /* ok, so create the window */

  hinst=GetModuleHandle(NULL);
  hmem=LocalAlloc(LPTR,sizeof(WNDCLASSEX));
  if(!hmem) return -1;

  winclassex=(LPWNDCLASSEX)LocalLock(hmem);

  winclassex->cbSize=sizeof(WNDCLASSEX);
  winclassex->style=CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  winclassex->lpfnWndProc=(WNDPROC)myproc;
  winclassex->cbClsExtra=0;
  winclassex->cbWndExtra=0;
  winclassex->hInstance=hinst;
  winclassex->hIcon=LoadIcon(NULL,IDI_APPLICATION);
  winclassex->hIconSm=LoadIcon(NULL,IDI_APPLICATION);
  winclassex->hCursor=LoadCursor(NULL,IDC_ARROW);
  winclassex->hbrBackground=GetStockObject(BLACK_BRUSH);
  winclassex->lpszMenuName=(LPSTR)"gfxwinmenu";
  winclassex->lpszClassName=(LPSTR)WINDOW_CLASS_NAME;

  if(!RegisterClassEx(winclassex))
    return -1;
  
  hwnd=CreateWindowEx(WS_EX_TOPMOST,WINDOW_CLASS_NAME,WINDOW_CAPTION,
    WS_VISIBLE | WS_POPUP,
    0,0,
    GetSystemMetrics(SM_CXSCREEN),
    GetSystemMetrics(SM_CYSCREEN),
    NULL,NULL,
    hinst,
    NULL);
    
  if(!hwnd) {
    UnregisterClass(WINDOW_CLASS_NAME,hinst);
    return -1;
  }

  if(dd_init(hwnd)<0) {
    DestroyWindow(hwnd); hwnd=0;
    UnregisterClass(WINDOW_CLASS_NAME,hinst);
    return -1;
  }

  if(c_prim_surf()<0) {
    dd_close();
    DestroyWindow(hwnd); hwnd=0;
    UnregisterClass(WINDOW_CLASS_NAME,hinst);
    return -1;
  }
  
  ShowCursor(0);
  
  return 0;
}

/* Create a GDI main window */
static int mwin_init_gdi(void) {
  HANDLE hmem;
  int wxs,wys;
  RECT wrect;
  
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
    return -1;
  }

  /* Determine window size from application area size */
  wrect.left = 0;
  wrect.top = 0;
  wrect.right = sxs;
  wrect.bottom = sys;
  
  AdjustWindowRect(&wrect, WINDOW_STYLE, 0);
  wxs = wrect.right-wrect.left;
  wys = wrect.bottom-wrect.top;
  
  /* Create window */
  hwnd=CreateWindow(WINDOW_CLASS_NAME,WINDOW_CAPTION,
    WINDOW_STYLE,
    CW_USEDEFAULT,CW_USEDEFAULT,
    wxs,wys,
    NULL,
    0,
    hinst,
    NULL);
    
  if(!hwnd) {
    return -1;
  }
  
  return 0;
}

static int mwin_init(int fs) {
  if(!fs || mwin_init_ddraw()<0) {
    if(fs) fprintf(stderr,"DDraw failed, revert to GDI\n");
    if(mwin_init_gdi()<0) return -1;
  }
  return 0;
}

static void mwin_draw(void) {
  HDC dc;
  PAINTSTRUCT ps;
    
  dc=BeginPaint(hwnd,&ps);
  
  bmi->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  bmi->bmiHeader.biWidth=sxs;
  bmi->bmiHeader.biHeight=sys;
  bmi->bmiHeader.biPlanes=1;
  bmi->bmiHeader.biBitCount=8;
  bmi->bmiHeader.biCompression=BI_RGB;
  bmi->bmiHeader.biSizeImage=sxs*sys;
  bmi->bmiHeader.biXPelsPerMeter=0;
  bmi->bmiHeader.biYPelsPerMeter=0;
  bmi->bmiHeader.biClrUsed=0;
  bmi->bmiHeader.biClrImportant=0;
  
  scrbmp=CreateDIBitmap(dc,&bmi->bmiHeader,CBM_INIT,vscr2,bmi,DIB_RGB_COLORS);

  scrdc=CreateCompatibleDC(dc);
  SelectObject(scrdc,scrbmp);
  BitBlt(dc,0,0,sxs,sys,scrdc,0,0,SRCCOPY);
  DeleteDC(scrdc);
  
  DeleteObject(scrbmp);

  EndPaint(hwnd, &ps);
}


static LONG APIENTRY myproc(HWND hwnd, UINT msg, UINT wparam, LONG lparam) {
  LONG r;
  
  switch(msg) {
    case WM_PAINT:
      //DefWindowProc(hwnd,msg,wparam,lparam);
      mwin_draw();
      break;

    case WM_CLOSE:
    case WM_DESTROY:
      /* don't terminate program if we closed the window ourselves
         (e.g. when DDraw init fails) */
      if(inited) {
        exit(0);
        PostQuitMessage(0);
      }
      return 0;

    case WM_KEYDOWN:
    case WM_KEYUP:
      w_putkey(msg==WM_KEYDOWN,txkey[wparam],-1);
      fflush(logfi);
      break;

    default:
      r = DefWindowProc(hwnd,msg,wparam,lparam);
//      printf("[D");
//      printf("]");
      return r;
  }
  return 0;
}

extern FILE *logfi;

void mgfx_close(void) {
  if(!inited) return;
  dd_close();
  if(hwnd) { DestroyWindow(hwnd); hwnd=0; }
  UnregisterClass(WINDOW_CLASS_NAME,hinst);
  inited=0;
}

int mgfx_init(void) {
  int i;
  
  if(inited) return 0;
  
  scr_xs=320;
  scr_ys=200;
  
  mgfx_selln(3);
  
  sxs = scr_xs << 1;
  sys = scr_ys << 1;
  
  /* Create window, set graph. mode, create surface */
  if(mwin_init(0)<0) return -1;
  
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
  
  vscr0=calloc(scr_xs*scr_ys, sizeof(uint8_t));
  if(!vscr0) return 1;
  
  vscr1=calloc(scr_xs*scr_ys, sizeof(uint8_t));
  if(!vscr1) return 1;
 

  vscr2=calloc(sxs*sys, sizeof(uint8_t));
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

int mgfx_toggle_fs(void) {
  int want_fs;
  
  want_fs = !lpdd;
  if(want_fs && dd_rtload()<0) return -1;
  dd_close();
  inited=0; /* prevent program termination */
  if(hwnd) { DestroyWindow(hwnd); hwnd=0; }
  UnregisterClass(WINDOW_CLASS_NAME,hinst);
  if(!want_fs) ShowCursor(1);
  mgfx_input_update(); /* catch WM_DESTROY */
  if(mwin_init(want_fs)<0) exit(1);
  inited=1;
  ShowWindow(hwnd,SW_SHOWNORMAL);
  UpdateWindow(hwnd);
  return 0;
}

int mgfx_toggle_dbl_ln(void) {
  dbl_ln = !dbl_ln;
  mgfx_selln(3);
  mgfx_updscr();
  return 0;
}

int mgfx_is_fs(void) {
  return lpdd != NULL;
}

void mgfx_updscr(void) {
  unsigned char *sp,*dp;
  int y,x;

  if(!dbl_ln) {
    dp=vscr2+(scr_ys-1)*2*scr_xs*2;
    sp=vscr0;
  
    for(y=0;y<scr_ys;y++) {
      for(x=0;x<scr_xs;x++)
        dp[2*x]=dp[2*x+1]=sp[x];
      dp-=scr_xs << 1;
      for(x=0;x<scr_xs;x++)
        dp[2*x]=dp[2*x+1]=sp[x];
      dp-=scr_xs << 1;
      sp+=scr_xs;
    }
  } else {
    dp=vscr2+(2*(scr_ys-1))*scr_xs*2;
    sp=vscr0;
  
    for(y=0;y<scr_ys;y++) {
      for(x=0;x<scr_xs;x++)
        dp[2*x]=dp[2*x+1]=sp[x];
      dp-=scr_xs<<2;
      sp+=scr_xs;
    }
    
    dp=vscr2+(2*(scr_ys-1)+1)*scr_xs*2;
    sp=vscr1;
  
    for(y=0;y<scr_ys;y++) {
      for(x=0;x<scr_xs;x++)
        dp[2*x]=dp[2*x+1]=sp[x];
      dp-=scr_xs<<2;
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
//    printf("[%d]",msg.message);
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}
