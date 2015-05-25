
# xlights Makefile

CFLAGS += -D_DEFAULT_SOURCE
LDLIBS += -lX11

all: xlights

xlights: xlights.o xlwin.o
	$(CC) $(LOADLIBES) $(LDFLAGS) $(LDLIBS) $(LDADD) -o $@ $>

clean:
	rm xlights xlights.o xlwin.o || true

include global.mk

.PHONY: all clean
