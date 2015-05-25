
# global.mk

# useful compiler flags; thanks xavier roche @ httrack blog

CFLAGS += -std=c99 -pipe -fvisibility=hidden -Wall -Wextra -Wformat \
	-Wformat-security -Wreturn-type -Wpointer-arith -Winit-self \
	-Wsign-compare -Wmultichar -Wuninitialized -Werror -funroll-loops \
	-funswitch-loops -pedantic

LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-O1 -Wl,--no-undefined \
	-Wl,--build-id=sha1 -rdynamic

VALGRIND_FLAGS = --trace-children=yes --leak-check=full --track-origins=yes \
	--show-leak-kinds=all

vg:
	valgrind $(VALGRIND_FLAGS) $(ARGS)

DEV_CFLAGS = -O1 -g
PROD_CFLAGS = -O3
PROD_LDFLAGS = -Wl,--discard-all

.PHONY: vg
