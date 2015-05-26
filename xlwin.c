
// xlwin.c

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>

#include "auto_clean.h"
#include "xlwin.h"

static void xconn_close(void *display) {
	if(display == NULL)
		return;
	XCloseDisplay(display);
}

int xconn_connect(struct xconn *xc) {
	if(xc == NULL)
		return -1;
	auto_clean_init();
	xc->display = XOpenDisplay(NULL);
	if(xc->display == NULL) {
		fprintf(stderr, "Error: Display could not be opened.\n");
		return -1;
	}
	if(auto_clean_add(xc->display, xconn_close) < 0) {
		XCloseDisplay(xc->display);
		return -1;
	}
	xc->screen = DefaultScreen(xc->display);
	xc->root = XRootWindow(xc->display, xc->screen);
	return 0;
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
	printf("pressed %d\n", *pressed);
	for(i = 0; i < num; i++) {
		kc = XKeysymToKeycode(xc->display, va_arg(ap, KeySym));
		if((kr[kc / 8] & (1 << (kc % 8))) != 0) {
			pressed[i / BITS_IN_INT] |= 1 << (i % BITS_IN_INT);
			ret++;
			break;
		}
	}
	printf("pressed after %d\n", *pressed);
	va_end(ap);
	return ret;
}

struct xlwin_end_data {
	Display *display;
	struct xlwin *w;
};

static void xlwin_end(void *d) {
	struct xlwin_end_data *x = d;
	if(x->display == NULL || x->w == NULL)
		return;
	if((x->w->xlwin_init & XLWIN_CM) != 0)
		XFreeColormap(x->display, x->w->cm);
	if((x->w->xlwin_init & XLWIN_WIN) != 0)
		XDestroyWindow(x->display, x->w->win);
	if((x->w->xlwin_init & XLWIN_C) != 0)
		XFreeColors(x->display, x->w->cm, (unsigned long[]){
			x->w->c[0].pixel, x->w->c[1].pixel
		}, 2, 0);
	if((x->w->xlwin_init & XLWIN_F) != 0)
		XFreeFont(x->display, x->w->f);
	if((x->w->xlwin_init & XLWIN_GC) != 0)
		XFreeGC(x->display, x->w->gc);
	free(x->w);
}

#define SYNC_INIT(x) do { \
	XSync(xc->display, False); \
	w->xlwin_init |= (x); \
} while(0)
struct xlwin *xlwin_new(struct xconn *xc, struct rect *r,
                        struct xlwin_state *s) {
	struct xlwin *w;
	XVisualInfo vinfo;
	if(xc == NULL || r == NULL || s == NULL)
		return NULL;
	w = malloc(sizeof *w);
	if(w == NULL) {
		perror("malloc");
		return NULL;
	}
	if(auto_clean_add(&(struct xlwin_end_data){
		  .display = xc->display,
		  .w = w,
	  }, xlwin_end) < 0) {
		free(w);
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
	SYNC_INIT(XLWIN_C);
	w->f = XLoadQueryFont(xc->display, "fixed");
	SYNC_INIT(XLWIN_F);
	w->gc = XCreateGC(xc->display, w->win, 0, NULL);
	SYNC_INIT(XLWIN_GC);
	XMapWindow(xc->display, w->win);
	xlwin_draw(xc, w, s);
	return w;
}

void xlwin_draw(struct xconn *xc, struct xlwin *w, struct xlwin_state *s) {
	int h, ha;
	unsigned long x;
	if(xc == NULL || w == NULL || s == NULL)
		return;
	x = XWhitePixel(xc->display, xc->screen);
	printf("pressed draw %d\n", *s->pressed);
	XSetForeground(xc->display, w->gc, (s->pressed[0] & 1) != 0 ? x :
	               w->c[(s->led_mask & NUM) != 0].pixel);
	XFillArc(xc->display, w->win, w->gc, 0, 0, w->r.h, w->r.h, 0, 360 << 6);
	XSetForeground(xc->display, w->gc, (s->pressed[0] & 2) != 0 ? x :
	               w->c[(s->led_mask & CAPS) != 0].pixel);
	XFillArc(xc->display, w->win, w->gc,
	         w->r.h, 0, w->r.h, w->r.h, 0, 360 << 6);
	XSetForeground(xc->display, w->gc, (s->pressed[0] & 4) != 0 ? x :
	         w->c[(s->led_mask & SCRL) != 0].pixel);
	XFillArc(xc->display, w->win, w->gc,
	         w->r.h << 1, 0, w->r.h, w->r.h, 0, 360 << 6);

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
	XSync(xc->display, False);
}
