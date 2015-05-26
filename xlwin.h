
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
	enum {
		XLWIN_CM  = (1 << 0),
		XLWIN_WIN = (1 << 1),
		XLWIN_C   = (1 << 2),
		XLWIN_F   = (1 << 3),
		XLWIN_GC  = (1 << 4),
	} xlwin_init;
	Colormap cm;
	struct rect r;
	Window win;
	XColor c[3];
	XFontStruct *f;
	GC gc;
};

struct xlwin_state {
	int led_mask;
	int pressed[1];
};

int xconn_connect(struct xconn *xc);
int xconn_any_pressed(struct xconn *xc, int num, int *pressed, ...);
struct xlwin *xlwin_new(struct xconn *xc, struct rect *r,
                        struct xlwin_state *s);
void xlwin_draw(struct xconn *xc, struct xlwin *w, struct xlwin_state *s);

#endif // XLWIN_H
