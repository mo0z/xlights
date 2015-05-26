
# xlights

On-screen keyboard LEDs using Xlib.

## Installation

The xlights binary is called with a numeric duration in seconds and the X11 mode
is set with `--x11`, like in the example below:
```
$ xlights 3 --x11
```

No requirement on the order of arguments is given.
Text output (on stdout) is assumed the default, and for `--x11` a default
duration of 3 seconds is set.

With DWM, it's possible to pass key descriptions that would be used with
`XGrabKey()` without specifying a modifier using `0`.<br>
The changes in `config.h` would look as follows:

```c
static const char *xlightscmd[] = { "/home/mar77i/bin/xlights", "1", "--x11" };

static Key keys[] = {
	...
	{ 0,                            XK_Caps_Lock, spawn,       { .v = xlightscmd } },
	{ 0,                            XK_Num_Lock, spawn,        { .v = xlightscmd } },
	{ 0,                            XK_Scroll_Lock, spawn,     { .v = xlightscmd } },
};
```

## License

Do what the fuck you want with it.
