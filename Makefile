
# xlights Makefile

CFLAGS += -D_DEFAULT_SOURCE -iquote ../bulk77i
LDLIBS += -lX11

EXTERN_OBJ = ../bulk77i/auto_clean.o ../bulk77i/bulk.o

all: xlights

$(EXTERN_OBJ):
	$(MAKE) -C $(@D) $(MAKEFLAGS) $(@F)

xlights: xlights.o xlwin.o $(EXTERN_OBJ)
	$(CC) $(LOADLIBES) $(LDFLAGS) $(LDLIBS) $(LDADD) -o $@ $>

clean:
	rm xlights xlights.o xlwin.o || true

include global.mk

.PHONY: all clean
