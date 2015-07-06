
# xlights Makefile

CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDLIBS += -lX11 -lbulk77i
LDFLAGS += $(PROD_LDFLAGS)

all: xlights

xlights: xlights.o xlwin.o $(EXTERN_OBJ)
	$(CC) $(LOADLIBES) $(LDFLAGS) $(LDLIBS) $(LDADD) -o $@ $> $^

clean:
	rm xlights xlights.o xlwin.o || true

include global.mk

.PHONY: all clean
