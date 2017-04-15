#
# GZX makefile
#
# We can build either natively in generic GNU/Unix-like environment or
# cross compile for Win32 using MinGW cross-compiler
#

CC		= gcc
CC_w32		= i686-w64-mingw32-gcc

CFLAGS		= -O2 -Wall -Werror -Wmissing-prototypes -I/usr/include/SDL
CFLAGS_g	= $(CFLAGS) -DUSE_GPU
CFLAGS_w32	= -O2 -Wall -Werror -Wmissing-prototypes
CFLAGS_w32_g	= $(CFLAGS_w32) -DUSE_GPU

LIBS		= -lSDL
LIBS_w32	= -lgdi32 -lwinmm

bkqual = $$(date '+%Y-%m-%d')

sources_generic = \
    fileutil.c \
    gzx.c \
    memio.c \
    z80.c \
    z80dep.c \
    snap.c \
    snap_ay.c \
    strutil.c \
    zx.c \
    zx_kbd.c \
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

sources = \
    $(sources_generic) \
    gfx_sdl.c \
    snd_sdl.c \
    sys_unix.c

sources_g = \
    $(sources) \
    z80g.c

sources_w32 = \
    $(sources_generic) \
    gfx_win.c \
    snd_win.c \
    sys_win.c

sources_w32_g = \
    $(sources_w32) \
    z80g.c

binary = gzx
binary_g = gzx-g
binary_w32 = gzx.exe
binary_w32_g = gzx-g.exe

objects = $(sources:.c=.o)
objects_g = $(sources_g:.c=.g.o)
objects_w32 = $(sources_w32:.c=.w32.o)
objects_w32_g = $(sources_w32_g:.c=.g.w32.o)
headers = $(wildcard *.h)

# Default target
default: $(binary)

all: $(binary) $(binary_g) $(binary_w32) $(binary_w32_g)

$(binary): $(objects)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(binary_g): $(objects_g)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(binary_w32): $(objects_w32)
	$(CC_w32) $(CFLAGS) -o $@ $^ $(LIBS_w32)

$(binary_w32_g): $(objects_w32_g)
	$(CC_w32) $(CFLAGS) -o $@ $^ $(LIBS_w32)

$(objects): $(headers)

%.g.o: %.c
	$(CC) -c $(CFLAGS_g) -o $@ $<

%.w32.o: %.c
	$(CC_w32) -c $(CFLAGS_w32) -o $@ $<

%.g.w32.o: %.c
	$(CC_w32) -c $(CFLAGS_w32_g) -o $@ $<

clean:
	rm -f *.o $(binary) $(binary_g) $(binary_w32) $(binary_w32_g)

backup: clean
	cd .. && tar czf gzx-$(bkqual).tar.gz trunk
	cd .. && rm -f gzx-latest.tar.gz && ln -s gzx-$(bkqual).tar.gz gzx-latest.tar.gz
