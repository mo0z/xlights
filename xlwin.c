
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
#include <bulk77i/util.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xlwin.h"

static int error = 0;

/* this was pulled from the libx11 1.6.3 source
 * so we can retreat in a structured way instead of auto_clean
 */
static int xconn_print_error(Display *dpy, XErrorEvent *event, FILE *fh) {
	char buffer[BUFSIZ];
	char mesg[BUFSIZ];
	char number[32];
	const char *mtype = "XlibMessage";
	register _XExtension *ext = NULL;
	_XExtension *bext = NULL;
	XGetErrorText(dpy, event->error_code, buffer, BUFSIZ);
	XGetErrorDatabaseText(dpy, mtype, "XError", "X Error", mesg, BUFSIZ);
	fprintf(fh, "%s:  %s\n  ", mesg, buffer);
	XGetErrorDatabaseText(dpy, mtype, "MajorCode", "Request Major code %d",
	  mesg, BUFSIZ);
	fprintf(fh, mesg, event->request_code);
	if(event->request_code < 128) {
		snprintf(number, sizeof(number), "%d", event->request_code);
		XGetErrorDatabaseText(dpy, "XRequest", number, "", buffer, BUFSIZ);
	} else {
		for(ext = dpy->ext_procs; ext; ext = ext->next)
			if(ext->codes.major_opcode == event->request_code)
				break;
		if(ext) {
			strncpy(buffer, ext->name, BUFSIZ);
			buffer[BUFSIZ - 1] = '\0';
		} else
			buffer[0] = '\0';
	}
	fprintf(fh, " (%s)\n", buffer);
	if(event->request_code >= 128) {
		XGetErrorDatabaseText(dpy, mtype, "MinorCode", "Request Minor code %d",
		mesg, BUFSIZ);
		fputs("  ", fh);
		fprintf(fh, mesg, event->minor_code);
		if(ext) {
			snprintf(mesg, sizeof(mesg), "%s.%d", ext->name, event->minor_code);
			XGetErrorDatabaseText(dpy, "XRequest", mesg, "", buffer, BUFSIZ);
			fprintf(fh, " (%s)", buffer);
		}
		fputs("\n", fh);
	}
	if(event->error_code >= 128) {
		/* kludge, try to find the extension that caused it */
		buffer[0] = '\0';
		for(ext = dpy->ext_procs; ext; ext = ext->next) {
			if(ext->error_string)
				(*ext->error_string)(dpy, event->error_code, &ext->codes,
				  buffer, BUFSIZ);
			if(buffer[0]) {
				bext = ext;
				break;
			}
			if(ext->codes.first_error &&
			ext->codes.first_error < (int)event->error_code &&
			(!bext || ext->codes.first_error > bext->codes.first_error))
				bext = ext;
		}
		if(bext)
			snprintf(buffer, sizeof(buffer), "%s.%d", bext->name,
			  event->error_code - bext->codes.first_error);
		else
			strcpy(buffer, "Value");
		XGetErrorDatabaseText(dpy, mtype, buffer, "", mesg, BUFSIZ);
		if(mesg[0]) {
			fputs("  ", fh);
			fprintf(fh, mesg, event->resourceid);
			fputs("\n", fh);
		}
		/* let extensions try to print the values */
		for(ext = dpy->ext_procs; ext; ext = ext->next)
			if(ext->error_values)
				(*ext->error_values)(dpy, event, fh);
	} else if((event->error_code == BadWindow) ||
	  (event->error_code == BadPixmap) || (event->error_code == BadCursor) ||
	  (event->error_code == BadFont) || (event->error_code == BadDrawable) ||
	  (event->error_code == BadColor) || (event->error_code == BadGC) ||
	  (event->error_code == BadIDChoice) || (event->error_code == BadValue) ||
	  (event->error_code == BadAtom)) {
		if(event->error_code == BadValue)
			XGetErrorDatabaseText(dpy, mtype, "Value", "Value 0x%x",
			  mesg, BUFSIZ);
		else if(event->error_code == BadAtom)
			XGetErrorDatabaseText(dpy, mtype, "AtomID", "AtomID 0x%x",
			  mesg, BUFSIZ);
		else
			XGetErrorDatabaseText(dpy, mtype, "ResourceID", "ResourceID 0x%x",
			  mesg, BUFSIZ);
		fputs("  ", fh);
		fprintf(fh, mesg, event->resourceid);
		fputs("\n", fh);
	}
	XGetErrorDatabaseText(dpy, mtype, "ErrorSerial", "Error Serial #%d",
	  mesg, BUFSIZ);
	fputs("  ", fh);
	fprintf(fh, mesg, event->serial);
	XGetErrorDatabaseText(dpy, mtype, "CurrentSerial", "Current Serial #%d",
	  mesg, BUFSIZ);
	fputs("\n  ", fh);
	fprintf(fh, mesg, dpy->request);
	fputs("\n", fh);
	return 0;
}

static int xconn_error(Display *dpy, XErrorEvent *event) {
	error = 1;
    return xconn_print_error(dpy, event, stderr);
}

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
	XSetErrorHandler(xconn_error);
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

#define SYNC_INIT do { \
	XSync(xc->display, False); \
	if(error != 0) { \
		xlwin_end(xc, w); \
		return NULL; \
	} \
	w->xlwin_init++; \
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
	SYNC_INIT;
	w->win = XCreateWindow(xc->display, xc->root, r->x, r->y, r->w, r->h, 0,
	  vinfo.depth, InputOutput, vinfo.visual,
	  CWColormap|CWBorderPixel|CWBackPixel|CWOverrideRedirect,
	  &(XSetWindowAttributes){
		.colormap = w->cm,
		.border_pixel = 0,
		.background_pixel = 0,
		.override_redirect = True,
	  });
	SYNC_INIT;
	XSetNormalHints(xc->display, w->win, &(XSizeHints){
		.flags = PPosition|PSize,
		.x = r->x,
		.y = r->y,
		.width = r->w,
		.height = r->h,
	});
	XParseColor(xc->display, w->cm, "#ff0000", w->c);
	XAllocColor(xc->display, w->cm, w->c);
	SYNC_INIT;
	XParseColor(xc->display, w->cm, "#00ff00", w->c + 1);
	XAllocColor(xc->display, w->cm, w->c + 1);
	SYNC_INIT;
	XParseColor(xc->display, w->cm, "#000000", w->c + 2);
	XAllocColor(xc->display, w->cm, w->c + 2);
	SYNC_INIT;
	XParseColor(xc->display, w->cm, "#ffff00", w->c + 3);
	XAllocColor(xc->display, w->cm, w->c + 3);
	SYNC_INIT;
	w->f = XLoadQueryFont(xc->display, "fixed");
	SYNC_INIT;
	w->gc = XCreateGC(xc->display, w->win, 0, NULL);
	SYNC_INIT;
	XMapWindow(xc->display, w->win);
	xlwin_draw(xc, w, led_mask, pressed);
	return w;
}

static inline int xlwin_freecolors(Display *display, struct xlwin *w) {
	size_t i;
	unsigned long p[UTIL_LENGTH(w->c)];
	for(i = 0; i < UTIL_LENGTH(w->c) && i + 1 < w->xlwin_init; i++)
		p[i] = w->c[i].pixel;
	return XFreeColors(display, w->cm, p, i, 0);
}

void xlwin_end(struct xconn *xc, struct xlwin *w) {
	if(xc == NULL || xc->display == NULL || w == NULL)
		return;
	if(w->xlwin_init >= 2)
		xlwin_freecolors(xc->display, w);
	if(w->xlwin_init >= 1)
		XFreeColormap(xc->display, w->cm);
	if(w->xlwin_init > UTIL_LENGTH(w->c) + 1) {
		w->xlwin_init -= UTIL_LENGTH(w->c) + 1;
		if(w->xlwin_init-- >= 1)
			XFreeFont(xc->display, w->f);
		if(w->xlwin_init >= 1)
			XFreeGC(xc->display, w->gc);
	}
	free(w);
}

void xlwin_draw(struct xconn *xc, struct xlwin *w, int led_mask, int pressed) {
	int h, ha, ol;
	if(xc == NULL || w == NULL || error != 0)
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
