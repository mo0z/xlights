
// xlwin.h

#ifndef XLWIN_H
#define XLWIN_H

#include <X11/Xlib.h>

#define CAPS (1 << 0)
#define NUM  (1 << 1)
#define SCRL (1 << 2)

struct rect {
	int x, y, w, h;
};

struct xconn {
	Display *display;
	int screen;
	Window root;
};

struct xlwin {
	Colormap cm;
	struct rect r;
	Window win;
	XColor c[2];
	XFontStruct *f;
	GC gc;
};

int xconn_connect(struct xconn *xc);
int xconn_any_pressed(struct xconn *xc, int num, ...);
struct xlwin *xlwin_new(struct xconn *xc, struct rect *r, int led_mask);
int xlwin_draw(struct xconn *xc, struct xlwin *w, int led_mask);
void xlwin_end(struct xconn *xc, struct xlwin *w);
void xconn_close(struct xconn *xc);

#endif // XLWIN_H
