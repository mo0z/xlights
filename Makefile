
# xlights Makefile

CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += -lX11 -lbulk77i $(PROD_LDFLAGS)

all: xlights

xlights: xlights.o xlwin.o $(EXTERN_OBJ)
	$(LINK)

clean:
	rm xlights xlights.o xlwin.o || true

include global.mk

.PHONY: all clean
