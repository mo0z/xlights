
/* xlwin.c
 *
 * Copyright (c) 2015-2016, mar77i <mar77i at mar77i dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <bulk77i/util.h>

#include "xlwin.h"

int xconn_connect(struct xconn *xc) {
	if(xc == NULL)
		return -1;
	xc->display = XOpenDisplay(NULL);
	if(xc->display == NULL) {
		fprintf(stderr, "Error: Display could not be opened.\n");
		return -1;
	}
	xc->screen = DefaultScreen(xc->display);
	xc->root = XRootWindow(xc->display, xc->screen);
	return 0;
}

void xconn_close(struct xconn *xc) {
	if(xc == NULL || xc->display == NULL)
		return;
	XCloseDisplay(xc->display);
}

#define BITS_IN_INT (sizeof pressed * CHAR_BIT)
int xconn_any_pressed(struct xconn *xc, int num, int *pressed, ...) {
	char kr[32];
	KeyCode kc;
	va_list ap;
	int i, ret = 0;
	if(xc == NULL)
		return -1;
	XQueryKeymap(xc->display, kr);
	va_start(ap, pressed);
	if(pressed != NULL)
		memset(pressed, 0, ((num / BITS_IN_INT) +
		       sizeof pressed * ((num % BITS_IN_INT) > 0)));
	for(i = 0; i < num; i++) {
		kc = XKeysymToKeycode(xc->display, va_arg(ap, KeySym));
		if((kr[kc / 8] & (1 << (kc % 8))) != 0) {
			pressed[i / BITS_IN_INT] |= 1 << (i % BITS_IN_INT);
			ret++;
			break;
		}
	}
	va_end(ap);
	return ret;
}

#define SYNC_INIT(x) do { \
	XSync(xc->display, False); \
	w->xlwin_init |= (x); \
} while(0)
struct xlwin *xlwin_new(struct xconn *xc, struct rect *r, int led_mask,
                        int pressed) {
	struct xlwin *w;
	XVisualInfo vinfo;
	if(xc == NULL || r == NULL)
		return NULL;
	w = malloc(sizeof *w);
	if(w == NULL) {
		perror("malloc");
		return NULL;
	}
	w->xlwin_init = 0;
	memcpy(&w->r, r, sizeof w->r);
	XMatchVisualInfo(xc->display, xc->screen, 32, TrueColor, &vinfo);
	w->cm = XCreateColormap(xc->display, xc->root, vinfo.visual, AllocNone);
	SYNC_INIT(XLWIN_CM);
	w->win = XCreateWindow(xc->display, xc->root, r->x, r->y, r->w, r->h, 0,
	  vinfo.depth, InputOutput, vinfo.visual,
	  CWColormap|CWBorderPixel|CWBackPixel|CWOverrideRedirect,
	  &(XSetWindowAttributes){
		.colormap = w->cm,
		.border_pixel = 0,
		.background_pixel = 0,
		.override_redirect = True,
	  });
	SYNC_INIT(XLWIN_WIN);
	XSetNormalHints(xc->display, w->win, &(XSizeHints){
		.flags = PPosition|PSize,
		.x = r->x,
		.y = r->y,
		.width = r->w,
		.height = r->h,
	});
	XParseColor(xc->display, w->cm, "#ff0000", w->c);
	XAllocColor(xc->display, w->cm, w->c);
	XParseColor(xc->display, w->cm, "#00ff00", w->c + 1);
	XAllocColor(xc->display, w->cm, w->c + 1);
	XParseColor(xc->display, w->cm, "#000000", w->c + 2);
	XAllocColor(xc->display, w->cm, w->c + 2);
	XParseColor(xc->display, w->cm, "#ffff00", w->c + 3);
	XAllocColor(xc->display, w->cm, w->c + 3);
	SYNC_INIT(XLWIN_C);
	w->f = XLoadQueryFont(xc->display, "fixed");
	SYNC_INIT(XLWIN_F);
	w->gc = XCreateGC(xc->display, w->win, 0, NULL);
	SYNC_INIT(XLWIN_GC);
	XMapWindow(xc->display, w->win);
	xlwin_draw(xc, w, led_mask, pressed);
	return w;
}

static inline int xlwin_freecolors(Display *display, Colormap *cm,
                                   XColor c[], const size_t nc) {
	size_t i;
	unsigned long p[nc];
	for(i = 0; i < nc; i++)
		p[i] = c[i].pixel;
	return XFreeColors(display, *cm, p, nc, 0);
}

void xlwin_end(struct xconn *xc, struct xlwin *w) {
	if(xc == NULL || xc->display == NULL || w == NULL)
		return;
	if((w->xlwin_init & XLWIN_C) != 0)
		xlwin_freecolors(xc->display, &w->cm, w->c, UTIL_LENGTH(w->c));
	if((w->xlwin_init & XLWIN_CM) != 0)
		XFreeColormap(xc->display, w->cm);
	if((w->xlwin_init & XLWIN_WIN) != 0)
		XDestroyWindow(xc->display, w->win);
	if((w->xlwin_init & XLWIN_F) != 0)
		XFreeFont(xc->display, w->f);
	if((w->xlwin_init & XLWIN_GC) != 0)
		XFreeGC(xc->display, w->gc);
	free(w);
}

void xlwin_draw(struct xconn *xc, struct xlwin *w, int led_mask, int pressed) {
	int h, ha, ol;
	if(xc == NULL || w == NULL)
		return;
	ol = w->r.h - 1;
	XSetForeground(xc->display, w->gc, XBlackPixel(xc->display, xc->screen));
	XFillRectangle(xc->display, w->win, w->gc, 0, 0, w->r.w, w->r.h);
	if((pressed & 1) != 0) {
		XSetForeground(xc->display, w->gc,
		               XWhitePixel(xc->display, xc->screen));
		XFillArc(xc->display, w->win, w->gc, 0, 0, ol, ol, 0, 360 << 6);
		XSetForeground(xc->display, w->gc, w->c[2].pixel);
		XDrawArc(xc->display, w->win, w->gc, 0, 0, ol, ol, 0, 360 << 6);
	} else {
		XSetForeground(xc->display, w->gc,
		 w->c[((led_mask & NUM) != 0) | (3 * ((led_mask & MOUSE) != 0))].pixel);
		XFillArc(xc->display, w->win, w->gc, 0, 0, ol, ol, 0, 360 << 6);
	}
	if((pressed & 2) != 0) {
		XSetForeground(xc->display, w->gc,
		               XWhitePixel(xc->display, xc->screen));
		XFillArc(xc->display, w->win, w->gc, w->r.h, 0, ol, ol, 0, 360 << 6);
		XSetForeground(xc->display, w->gc, w->c[2].pixel);
		XDrawArc(xc->display, w->win, w->gc, w->r.h, 0, ol, ol, 0, 360 << 6);
	} else {
		XSetForeground(xc->display, w->gc,
		               w->c[(led_mask & CAPS) != 0].pixel);
		XFillArc(xc->display, w->win, w->gc, w->r.h, 0, ol, ol, 0, 360 << 6);
	}

	if((pressed & 4) != 0) {
		XSetForeground(xc->display, w->gc,
		               XWhitePixel(xc->display, xc->screen));
		XFillArc(xc->display, w->win, w->gc,w->r.h << 1, 0,
		         ol, ol, 0, 360 << 6);
		XSetForeground(xc->display, w->gc, w->c[2].pixel);
		XDrawArc(xc->display, w->win, w->gc,w->r.h << 1, 0,
		         ol, ol, 0, 360 << 6);
	} else {
		XSetForeground(xc->display, w->gc,
		               w->c[(led_mask & SCRL) != 0].pixel);
		XFillArc(xc->display, w->win, w->gc,w->r.h << 1, 0,
		         ol, ol, 0, 360 << 6);
	}

	XSetForeground(xc->display, w->gc, w->c[2].pixel);
	XSetFont(xc->display, w->gc, w->f->fid);
	h = w->r.h >> 1;
	ha = w->f->ascent >> 1;
	XDrawString(xc->display, w->win, w->gc,
	        h - (XTextWidth(w->f, "NUM", 3) >> 1), h + ha, "NUM", 3);
	XDrawString(xc->display, w->win, w->gc,
	        w->r.h + h - (XTextWidth(w->f, "CAPS", 4) >> 1), h + ha, "CAPS", 4);
	XDrawString(xc->display, w->win, w->gc,
	 (w->r.h << 1) + h - (XTextWidth(w->f, "SCRL", 4) >> 1), h + ha, "SCRL", 4);
	XSync(xc->display, False);
}
