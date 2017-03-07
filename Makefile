#
# GZX makefile
#
# We can build either in generic GNU/Unix-like environment or
# in Cygwin/Win32
#

bld_target = gnu

CC		= gcc

CFLAGS_gnu	= -O2 -Wall -Werror -Wmissing-prototypes -I/usr/include/SDL
CFLAGS_w32	= -O2 -mno-cygwin -march=i386 -Wall -I../../cygtst/mydirx/include

LIBS_gnu	= -lSDL
LIBS_w32	= -lgdi32 -lwinmm

bkqual = $$(date '+%Y-%m-%d')

sources_generic = \
    gzx.c \
    z80.c \
    snap.c \
    snap_ay.c \
    zx_sound.c \
    zx_tape.c \
    zxt_tap.c \
    zxt_wav.c \
    zxt_tzx.c \
    zx_scr.c \
    ay.c \
    menus.c \
    mgfx.c \
    debug.c \
    disasm.c \
    iorec.c

sources_gnu = \
    gfx_sdl.c \
    snd_sdl.c \
    sys_unix.c

sources_w32 = \
    gfx_win.c \
    snd_win.c \
    sys_win.c

ifeq ($(bld_target), w32)
	sources = $(sources_generic) $(sources_w32)
	binary = gzxwin.exe
	CFLAGS = $(CFLAGS_w32)
	LIBS = $(LIBS_w32)
else
	sources = $(sources_generic) $(sources_gnu)
	binary = gzx
	CFLAGS = $(CFLAGS_gnu)
	LIBS = $(LIBS_gnu)
endif

objects = $(sources:.c=.o)
headers = $(wildcard *.h)

all: $(binary)

$(binary): $(objects)
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

$(objects): $(headers)

clean:
	rm -f *.o $(binary)

backup: clean
	cd .. && tar czf gzx-$(bkqual).tar.gz trunk
	cd .. && rm -f gzx-latest.tar.gz && ln -s gzx-$(bkqual).tar.gz gzx-latest.tar.gz
