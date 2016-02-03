
/* xlwin.h
 *
 * Copyright (c) 2015-2016, mar77i <mar77i at mar77i dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef XLWIN_H
#define XLWIN_H

#include <X11/Xlib.h>

#define CAPS  (1 <<  0)
#define NUM   (1 <<  1)
#define SCRL  (1 <<  2)
#define MOUSE (1 << 13)

struct rect {
	int x, y, w, h;
};

struct xconn {
	Display *display;
	int screen;
	Window root;
};

struct xlwin {
	enum {
		XLWIN_START,
		XLWIN_CM,
		XLWIN_WIN,
		XLWIN_C,
		XLWIN_F,
		XLWIN_GC,
	} xlwin_init;
	Colormap cm;
	struct rect r;
	Window win;
	XColor c[4];
	XFontStruct *f;
	GC gc;
};

int xconn_connect(struct xconn *xc);
void xconn_close(struct xconn *xc);
int xconn_any_pressed(struct xconn *xc, int num, int *pressed, ...);
struct xlwin *xlwin_new(struct xconn *xc, struct rect *r, int led_mask,
                        int pressed);
void xlwin_end(struct xconn *xc, struct xlwin *w);
void xlwin_draw(struct xconn *xc, struct xlwin *w, int led_mask, int pressed);

#endif // XLWIN_H
