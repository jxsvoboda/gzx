/*
 * GZX - George's ZX Spectrum Emulator
 * HelenOS graphics
 *
 * Copyright (c) 1999-2025 Jiri Svoboda
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

#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <fibril.h>
#include <io/keycode.h>
#include <io/pixelmap.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include "../../mgfx.h"
#include "../../gzx.h"

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t;

static void window_close_event(ui_window_t *, void *);
static void window_kbd_event(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t window_cb = {
	.close = window_close_event,
	.kbd = window_kbd_event
};

static ui_t *ui;
static ui_window_t *window;
static ui_wdecor_style_t style;
static gfx_context_t *gc;
static gfx_bitmap_t *bitmap;
static pixelmap_t pixelmap;

static color_t color[256];

static int video_w;
static int video_h;

static int xscale;
static int yscale;

static int *txkey;
static int txsize;

static int ktabsrc[] = {
	KC_ESCAPE,		WKEY_ESC,
	KC_1,			WKEY_1,
	KC_2,			WKEY_2,
	KC_3,			WKEY_3,
	KC_4,			WKEY_4,
	KC_5,			WKEY_5,
	KC_6,			WKEY_6,
	KC_7,			WKEY_7,
	KC_8,			WKEY_8,
	KC_9,			WKEY_9,
	KC_0,			WKEY_0,
	KC_MINUS,		WKEY_MINUS,
	KC_EQUALS,		WKEY_EQUAL,
	KC_BACKSPACE,		WKEY_BS,
	KC_TAB,			WKEY_TAB,
	KC_Q,			WKEY_Q,
	KC_W,			WKEY_W,
	KC_E,			WKEY_E,
	KC_R,			WKEY_R,
	KC_T,			WKEY_T,
	KC_Y,			WKEY_Y,
	KC_U,			WKEY_U,
	KC_I,			WKEY_I,
	KC_O,			WKEY_O,
	KC_P,			WKEY_P,
	KC_LBRACKET,		WKEY_LBR,
	KC_RBRACKET,		WKEY_RBR,
	KC_ENTER,		WKEY_ENTER,
	KC_LCTRL,		WKEY_LCTRL,
	KC_A,			WKEY_A,
	KC_S,			WKEY_S,
	KC_D,			WKEY_D,
	KC_F,			WKEY_F,
	KC_G,			WKEY_G,
	KC_H,			WKEY_H,
	KC_J,			WKEY_J,
	KC_K,			WKEY_K,
	KC_L,			WKEY_L,
	KC_SEMICOLON,		WKEY_SCOLON,
	KC_QUOTE,		WKEY_FOOT,
	KC_BACKTICK,		WKEY_GRAVE,
	KC_LSHIFT,		WKEY_LSHIFT,
	KC_BACKSLASH,		WKEY_BSLASH,
	KC_Z,			WKEY_Z,
	KC_X,			WKEY_X,
	KC_C,			WKEY_C,
	KC_V,			WKEY_V,
	KC_B,			WKEY_B,
	KC_N,			WKEY_N,
	KC_M,			WKEY_M,
	KC_PERIOD,		WKEY_PERIOD,
	KC_SLASH,		WKEY_SLASH,
	KC_RSHIFT,		WKEY_RSHIFT,
	KC_NTIMES,		WKEY_NSTAR,
	KC_LALT,		WKEY_LALT,
	KC_SPACE,		WKEY_SPACE,
	KC_CAPS_LOCK,		WKEY_CLOCK,
	KC_F1,			WKEY_F1,
	KC_F2,			WKEY_F2,
	KC_F3,			WKEY_F3,
	KC_F4,			WKEY_F4,
	KC_F5,			WKEY_F5,
	KC_F6,			WKEY_F6,
	KC_F7,			WKEY_F7,
	KC_F8,			WKEY_F8,
	KC_F9,			WKEY_F9,
	KC_F10,			WKEY_F10,
	KC_NUM_LOCK,		WKEY_NLOCK,
	KC_SCROLL_LOCK,		WKEY_SLOCK,
	KC_N7,			WKEY_N7,
	KC_N8,			WKEY_N8,
	KC_N9,			WKEY_N9,
	KC_NMINUS,		WKEY_NMINUS,
	KC_N4,			WKEY_N4,
	KC_N5,			WKEY_N5,
	KC_N6,			WKEY_N6,
	KC_NPLUS,		WKEY_NPLUS,
	KC_N1,			WKEY_N1,
	KC_N2,			WKEY_N2,
	KC_N3,			WKEY_N3,
	KC_N0,			WKEY_N0,
	KC_NPERIOD,		WKEY_NPERIOD,
	KC_COMMA,		WKEY_LESS,
	KC_F11,			WKEY_F11,
	KC_F12,			WKEY_F12,
	KC_NENTER,		WKEY_NENTER,
	KC_RCTRL,		WKEY_RCTRL,
	KC_NSLASH,		WKEY_NSLASH,
	KC_PRTSCR,		WKEY_PRNSCR,
	KC_RALT,		WKEY_RALT,
	KC_BREAK,		WKEY_BRK,
	KC_HOME,		WKEY_HOME,
	KC_UP,			WKEY_UP,
	KC_PAGE_UP,		WKEY_PGUP,
	KC_LEFT,		WKEY_LEFT,
	KC_RIGHT,		WKEY_RIGHT,
	KC_END,			WKEY_END,
	KC_DOWN,		WKEY_DOWN,
	KC_PAGE_DOWN,		WKEY_PGDN,
	KC_INSERT,		WKEY_INS,
	KC_DELETE,		WKEY_DEL,
	-1,			-1
};

static void window_close_event(ui_window_t *window, void *arg)
{
	(void) window;
	(void) arg;

	quit = 1;
}

static void window_kbd_event(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	(void) window;
	(void) arg;

	w_putkey(event->type == KEY_PRESS, txkey[event->key], -1);
}

static int init_video(void)
{
	gfx_rect_t rect;
	gfx_rect_t wrect;
	gfx_coord2_t off;
	ui_wnd_params_t params;
	gfx_bitmap_params_t bparams;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;
	int vw, vh;

	scr_xs = video_w;
	scr_ys = video_h;

	if (dbl_ln) {
		xscale = 2;
		yscale = 1;
	} else {
		xscale = 2;
		yscale = 2;
	}

	vw = scr_xs * xscale;
	vh = scr_ys * yscale;

	if (dbl_ln) {
		vh *= 2;
	}

	rc = ui_create(UI_DISPLAY_DEFAULT, &ui);
	if (rc != EOK) {
		printf("Error initializing UI\n");
		return -1;
	}

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = vw;
	rect.p1.y = vh;

	ui_wnd_params_init(&params);
	params.caption = "GZX";

	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(ui, params.style, &rect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &params.rect);

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return -1;
	}

	style = params.style;
	ui_window_set_cb(window, &window_cb, NULL);

	rc = ui_window_get_app_gc(window, &gc);
	if (rc != EOK) {
		printf("Error creating graphic context.\n");
		return -1;
	}

	gfx_bitmap_params_init(&bparams);
	bparams.rect.p0.x = 0;
	bparams.rect.p0.y = 0;
	bparams.rect.p1.x = vw;
	bparams.rect.p1.y = vh;

	rc = gfx_bitmap_create(gc, &bparams, NULL, &bitmap);
	if (rc != EOK) {
		printf("Error allocating screen bitmap.\n");
		return -1;
	}

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK) {
		printf("Error getting allocation info.'\n");
		return -1;
	}

	pixelmap.width = vw;
	pixelmap.height = vh;
	pixelmap.data = alloc.pixels;

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return -1;
	}

	return 0;
}

static int init_vscr(void)
{
	/* set up virtual frame buffer */

	vscr0 = calloc(scr_xs * scr_ys, sizeof(uint8_t));
	if (!vscr0) {
		printf("malloc failed\n");
		return -1;
	}

	if (dbl_ln) {
		vscr1 = calloc(scr_xs * scr_ys, sizeof(uint8_t));
		if (!vscr1) {
			printf("malloc failed\n");
			return -1;
		}
	}
}

static void fini_vscr(void)
{
	free(vscr0);
	vscr0 = NULL;

	if (dbl_ln) {
		free(vscr1);
		vscr1 = NULL;
	}
}

int mgfx_init(int w, int h)
{
	int i;

	video_w = w;
	video_h = h;

	if (init_video() < 0)
		return -1;

	w_initkey();

	/* set up key translation table */
	txsize = 1;
	for (i = 0; ktabsrc[i * 2 + 1] != -1; i++)
		if (ktabsrc[i * 2] >= txsize)
			txsize = ktabsrc[i * 2] + 1;

	txkey = calloc(txsize, sizeof(int));
	if (!txkey) {
		printf("malloc failed\n");
		return -1;
	}

	for (i = 0; ktabsrc[i * 2 + 1] != -1; i++)
		txkey[ktabsrc[i * 2]] = ktabsrc[i * 2 + 1];

	if (init_vscr() < 0)
		return -1;

	mgfx_selln(3);

	clip_x0 = clip_y0 = 0;
	clip_x1 = scr_xs - 1;
	clip_y1 = scr_ys - 1;

	return 0;
}

int mgfx_set_disp_size(int w, int h)
{
	int vw, vh;
	gfx_bitmap_params_t bparams;
	gfx_bitmap_alloc_t alloc;
	gfx_rect_t rect;
	gfx_rect_t nrect;
	gfx_rect_t wrect;
	gfx_coord2_t off;
	errno_t rc;

	fini_vscr();
	gfx_bitmap_destroy(bitmap);
	bitmap = NULL;
	gc = NULL;

	video_w = w;
	video_h = h;

	scr_xs = video_w;
	scr_ys = video_h;

	if (dbl_ln) {
		xscale = 2;
		yscale = 1;
	} else {
		xscale = 2;
		yscale = 2;
	}

	vw = scr_xs * xscale;
	vh = scr_ys * yscale;

	if (dbl_ln) {
		vh *= 2;
	}

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = vw;
	rect.p1.y = vh;

	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(ui, style, &rect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &nrect);

	rc = ui_window_resize(window, &nrect);
	if (rc != EOK) {
		printf("Error resizing window.\n");
		return -1;
	}

	rc = ui_window_get_app_gc(window, &gc);
	if (rc != EOK) {
		printf("Error creating graphic context.\n");
		return -1;
	}

	gfx_bitmap_params_init(&bparams);
	bparams.rect.p0.x = 0;
	bparams.rect.p0.y = 0;
	bparams.rect.p1.x = vw;
	bparams.rect.p1.y = vh;

	rc = gfx_bitmap_create(gc, &bparams, NULL, &bitmap);
	if (rc != EOK) {
		printf("Error allocating screen bitmap.\n");
		return -1;
	}

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK) {
		printf("Error getting allocation info.'\n");
		return -1;
	}

	pixelmap.width = vw;
	pixelmap.height = vh;
	pixelmap.data = alloc.pixels;

	if (init_vscr() < 0)
		return -1;

	clip_x0 = clip_y0 = 0;
	clip_x1 = scr_xs - 1;
	clip_y1 = scr_ys - 1;

	return 0;
}

static void render_display_line(int dy, uint8_t *spix)
{
	int i, j, k;
	pixel_t pval;
	color_t *col;

	for (j = 0; j < yscale; j++) {
		for (i = 0; i < scr_xs; i++) {
			for (k = 0; k < xscale; k++) {
				col = &color[spix[i]];
				pval = PIXEL(255, col->r, col->g, col->b);
				pixelmap_put_pixel(&pixelmap, xscale * i + k,
				    yscale * dy + j, pval);
			}
		}
	}
}

void mgfx_updscr(void)
{
	unsigned y;

	if (dbl_ln) {
		for (y = 0; y < scr_ys; y++) {
			render_display_line(2 * y, vscr0 + scr_xs * y);
			render_display_line(2 * y + 1, vscr1 + scr_xs * y);
		}
	} else {
		for (y = 0; y < scr_ys; y++) {
			render_display_line(y, vscr0 + scr_xs * y);
		}
	}
	(void) gfx_bitmap_render(bitmap, NULL, NULL);
}

static unsigned b6to8(unsigned cval)
{
	return 255 * cval / 63;
}

void mgfx_setpal(int base, int cnt, int *p)
{
	int i;

	for (i = 0; i < cnt; i++) {
		color[i].r = b6to8(p[3 * i]);
		color[i].g = b6to8(p[3 * i + 1]);
		color[i].b = b6to8(p[3 * i + 2]);
	}
}

/* input */

void mgfx_input_update(void)
{
	fibril_usleep(10);
}

int mgfx_toggle_fs(void)
{
	/* Not implemented */
	return 0;
}

int mgfx_toggle_dbl_ln(void)
{
	fini_vscr();
	dbl_ln = !dbl_ln;
	if (dbl_ln) {
		xscale = 2;
		yscale = 1;
	} else {
		xscale = 2;
		yscale = 2;
	}
	init_vscr();
	mgfx_selln(3);
	return 0;
}

int mgfx_is_fs(void)
{
	return 0;
}
