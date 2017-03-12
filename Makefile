
# xlights Makefile
#
# Copyright (c) 2015-2016, mar77i <mysatyre at gmail dot com>
#
# This software may be modified and distributed under the terms
# of the ISC license.  See the LICENSE file for details.

CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += -lX11 -lbulk77i $(PROD_LDFLAGS)

all: xlights

xlights: xlights.o xlwin.o
	$(LINK)

install: xlights
	@mkdir -p "$(DESTDIR)$(PREFIX)$(BIN_PATH)"
	@echo installing $> $^ to \"$(DESTDIR)$(PREFIX)$(BIN_PATH)\"
	@cp -f $> $^ "$(DESTDIR)$(PREFIX)$(BIN_PATH)"
	@for file in $> $^; do \
		chmod 755 "$(DESTDIR)$(PREFIX)$(BIN_PATH)/$$file"; \
	done

clean:
	rm xlights xlights.o xlwin.o || true

include global.mk

.PHONY: all clean
