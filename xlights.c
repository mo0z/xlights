
// xlights.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xlwin.h"

int main(int argc, char *argv[]) {
	int x11 = 0, ret = EXIT_FAILURE, wait = 3;
	struct xconn xc;
	struct xlwin *w = NULL;
	XWindowAttributes xwa;
	XKeyboardState ks;
	while(argc > 1) {
		if(strcmp(argv[argc - 1], "--x11") == 0) {
			argc--;
			x11 = 1;
		} else if(strspn(argv[argc - 1], "0123456789") ==
		  strlen(argv[argc - 1])) {
			argc--;
			wait = strtol(argv[1], NULL, 10);
		} else {
			fprintf(stderr, "Error: invalid arguments\n");
			return EXIT_FAILURE;
		}
	}
	if(xconn_connect(&xc) < 0)
		return EXIT_FAILURE;
	if(x11 != 0) {
		if(XGetWindowAttributes(xc.display, xc.root, &xwa) == 0) {
			fprintf(stderr, "Error: XGetWindowAttributes\n");
			goto error;
		}
		if(XGetKeyboardControl(xc.display, &ks) == 0) {
			fprintf(stderr, "Error: XGetKeyboardControl\n");
			goto error;
		}
		w = xlwin_new(&xc, &(struct rect){
			xwa.width - 32 * 5, xwa.height - 48, 32 * 3, 32
		}, ks.led_mask);
	}
	while(xconn_any_pressed(&xc, 3,
	  XK_Caps_Lock, XK_Num_Lock, XK_Scroll_Lock) != 0)
		usleep(100000);
	if(XGetKeyboardControl(xc.display, &ks) == 0) {
		fprintf(stderr, "Error: XGetKeyboardControl\n");
		goto error;
	}
	xlwin_draw(&xc, w, ks.led_mask);
	sleep(wait);
error:
	if(w != NULL)
		xlwin_end(&xc, w);
	xconn_close(&xc);
	return ret;
}
