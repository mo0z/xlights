
// xlights.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bulk77i/term.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xlwin.h"

static void xlights_text(int led_mask) {
	printf("NUM: %s\nCAPS: %s\nSCRL: %s\n",
		   led_mask & NUM ? "ON" : "OFF", led_mask & CAPS ? "ON" : "OFF",
		   led_mask & SCRL ? "ON" : "OFF");
}

int main(int argc, char *argv[]) {
	int x11 = 0, wait = 3, pressed = 0;
	struct xconn xc;
	struct xlwin *w = NULL;
	XWindowAttributes xwa;
	XKeyboardState ks;
	while(argc > 1) {
		if(strcmp(argv[argc - 1], "--x11") == 0) {
			argc--;
			x11 = 1;
		} else if(argv[argc - 1][strspn(argv[argc - 1], "0123456789")] ==
		  '\0') {
			argc--;
			wait = strtol(argv[1], NULL, 10);
		} else {
			fprintf(stderr, "Error: invalid arguments\n");
			return EXIT_FAILURE;
		}
	}
	if(xconn_connect(&xc) < 0)
		return EXIT_FAILURE;
	XGetWindowAttributes(xc.display, xc.root, &xwa);
	XGetKeyboardControl(xc.display, &ks);
	if(x11 != 0) {
		xconn_any_pressed(&xc, 3, &pressed,
		                  XK_Num_Lock, XK_Caps_Lock, XK_Scroll_Lock);
		w = xlwin_new(&xc, &(struct rect){
			xwa.width - 32 * 5, xwa.height - 48, 32 * 3, 32
		}, ks.led_mask, pressed);
		if(w == NULL)
			return EXIT_FAILURE;
	}
	while(xconn_any_pressed(&xc, 3, &pressed,
	  XK_Num_Lock, XK_Caps_Lock, XK_Scroll_Lock) != 0)
		usleep(100000);
	XGetKeyboardControl(xc.display, &ks);
	if(x11 == 0)
		xlights_text(ks.led_mask);
	else {
		xlwin_draw(&xc, w, ks.led_mask, pressed);
		sleep(wait);
	}
	xlwin_end(&xc, w);
	xconn_close(&xc);
	return EXIT_SUCCESS;
}
