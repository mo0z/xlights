
// xlwin.c

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>

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

int xconn_any_pressed(struct xconn *xc, int num, ...) {
	char kr[32];
	KeyCode kc;
	va_list ap;
	int i, ret = 0;
	XQueryKeymap(xc->display, kr);
	va_start(ap, num);
	for(i = 0; i < num; i++) {
		kc = XKeysymToKeycode(xc->display, va_arg(ap, KeySym));
		if((kr[kc / 8] & (1 << (kc % 8))) != 0) {
			ret++;
			break;
		}
	}
	va_end(ap);
	return ret;
}

struct xlwin *xlwin_new(struct xconn *xc, struct rect *r, int led_mask) {
	struct xlwin *w;
	XVisualInfo vinfo;
	if(XMatchVisualInfo(xc->display, xc->screen, 32, TrueColor, &vinfo) == 0) {
		fprintf(stderr, "Error: XMatchVisualInfo\n");
		return NULL;
	}
	w = malloc(sizeof *w);
	if(w == NULL) {
		perror("malloc");
		return NULL;
	}
	memset(w, 0, sizeof  *w);
	memcpy(&w->r, r, sizeof w->r);
	w->cm = XCreateColormap(xc->display, xc->root, vinfo.visual, AllocNone);
	w->win = XCreateWindow(xc->display, xc->root, r->x, r->y, r->w, r->h, 0,
	  vinfo.depth, InputOutput, vinfo.visual,
	  CWColormap|CWBorderPixel|CWBackPixel|CWOverrideRedirect,
	  &(XSetWindowAttributes){
		.colormap = w->cm,
		.border_pixel = 0,
		.background_pixel = 0,
		.override_redirect = True,
	  });
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
	w->f = XLoadQueryFont(xc->display, "fixed");
	w->gc = XCreateGC(xc->display, w->win, 0, NULL);
	XMapWindow(xc->display, w->win);
	if(xlwin_draw(xc, w, led_mask) < 0)
		goto error;
	return w;
error:
	xlwin_end(xc, w);
	return NULL;
}

int xlwin_draw(struct xconn *xc, struct xlwin *w, int led_mask) {
	int h, ha;
	if(xc == NULL || w == NULL)
		return -1;
	XSetForeground(xc->display, w->gc, w->c[(led_mask & NUM) != 0].pixel);
	XFillArc(xc->display, w->win, w->gc, 0, 0, w->r.h, w->r.h, 0, 360 << 6);
	XSetForeground(xc->display, w->gc, w->c[(led_mask & CAPS) != 0].pixel);
	XFillArc(xc->display, w->win, w->gc, w->r.h, 0, w->r.h, w->r.h, 0, 360 << 6);
	XSetForeground(xc->display, w->gc, w->c[(led_mask & SCRL) != 0].pixel);
	XFillArc(xc->display, w->win, w->gc, w->r.h << 1, 0, w->r.h, w->r.h, 0, 360 << 6);

	XSetForeground(xc->display, w->gc, BlackPixel(xc->display, xc->screen));
	XSetFont(xc->display, w->gc, w->f->fid);
	h = w->r.h >> 1;
	ha = w->f->ascent >> 1;
	XDrawString(xc->display, w->win, w->gc,
	        h - (XTextWidth(w->f, "NUM", 3) >> 1), h + ha, "NUM", 3);
	XDrawString(xc->display, w->win, w->gc,
	        w->r.h + h - (XTextWidth(w->f, "CAPS", 4) >> 1), h + ha, "CAPS", 4);
	XDrawString(xc->display, w->win, w->gc,
	 (w->r.h << 1) + h - (XTextWidth(w->f, "SCRL", 4) >> 1), h + ha, "SCRL", 4);
	XFlush(xc->display);
	return 0;
}

void xlwin_end(struct xconn *xc, struct xlwin *w) {
	if(xc == NULL || w == NULL)
		return;
	XFreeFont(xc->display, w->f);
	XFreeGC(xc->display, w->gc);
	XFreeColors(xc->display, w->cm, (unsigned long[]){
		w->c[0].pixel, w->c[1].pixel
	}, 2, 0);
	XFreeColormap(xc->display, w->cm);
	XDestroyWindow(xc->display, w->win);
	free(w);
}

void xconn_close(struct xconn *xc) {
	if(xc == NULL)
		return;
	XCloseDisplay(xc->display);
}
