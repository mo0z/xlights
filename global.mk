
# global.mk

# useful compiler flags; thanks xavier roche @ httrack blog

CFLAGS += -std=c99 -pipe -fvisibility=hidden -Wall -Wextra -Wformat \
	-Wformat-security -Wreturn-type -Wpointer-arith -Winit-self \
	-Wsign-compare -Wmultichar -Wuninitialized -Werror -funroll-loops \
	-funswitch-loops -pedantic

LDFLAGS += -Wl,-z,relro,-z,now,-O1,--as-needed,--no-undefined \
	-Wl,--build-id=sha1,--no-allow-shlib-undefined -rdynamic

VALGRIND_FLAGS = --trace-children=yes --leak-check=full --track-origins=yes \
	--show-leak-kinds=all

vg:
	valgrind $(VALGRIND_FLAGS) $(ARGS)

DEV_CFLAGS = -Og -g
PROD_CFLAGS = -O3
PROD_LDFLAGS = -Wl,--discard-all

.PHONY: vg
