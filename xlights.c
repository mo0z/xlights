
// xlights.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xlwin.h"

int main(int argc, char *argv[]) {
	int x11 = 0, wait = 3;
	struct xconn xc;
	struct xlwin *w = NULL;
	XWindowAttributes xwa;
	XKeyboardState ks;
	struct xlwin_state st;
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
	if(x11 != 0) {
		XGetWindowAttributes(xc.display, xc.root, &xwa);
		XGetKeyboardControl(xc.display, &ks);
		xconn_any_pressed(&xc, 3, st.pressed,
		                  XK_Num_Lock, XK_Caps_Lock, XK_Scroll_Lock);
		st.led_mask = ks.led_mask;
		w = xlwin_new(&xc, &(struct rect){
			xwa.width - 32 * 5, xwa.height - 48, 32 * 3, 32
		}, &st);
		if(w == NULL)
			return EXIT_FAILURE;
	}
	while(xconn_any_pressed(&xc, 3, st.pressed,
	  XK_Num_Lock, XK_Caps_Lock, XK_Scroll_Lock) != 0)
		usleep(100000);
	XGetKeyboardControl(xc.display, &ks);
	st.led_mask = ks.led_mask;
	xlwin_draw(&xc, w, &st);
	sleep(wait);
	return EXIT_SUCCESS;
}
