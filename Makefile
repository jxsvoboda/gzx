#
# GZX makefile
#
# We can build either natively in generic GNU/Unix-like environment or
# cross compile for Win32 using MinGW cross-compiler
#

CC		= gcc
CC_w32		= i686-w64-mingw32-gcc
CC_helenos	= helenos-cc
LD_helenos	= helenos-ld

CFLAGS		= -O2 -Wall -Werror -Wmissing-prototypes -I/usr/include/SDL -DWITH_MIDI
CFLAGS_g	= $(CFLAGS) -DUSE_GPU
CFLAGS_w32	= -O2 -Wall -Werror -Wmissing-prototypes
CFLAGS_w32_g	= $(CFLAGS_w32) -DUSE_GPU
CFLAGS_helenos	= -O2 -Wall -Wno-error -DHELENOS_BUILD -D_HELENOS_SOURCE \
    -D_REALLY_WANT_STRING_H \
    `helenos-pkg-config --cflags libgui libdraw libmath libhound libpcm`
CFLAGS_helenos_g = $(CFLAGS_helenos) -DUSE_GPU
PREFIX_hos	= `helenos-bld-config --install-dir`
INSTALL		= install

LIBS		= -lSDL -lasound
LIBS_w32	= -lgdi32 -lwinmm
LIBS_helenos	=  `helenos-pkg-config --libs libgui libdraw libmath libpcm libhound`

bkqual = $$(date '+%Y-%m-%d')

sources_generic = \
    adt/list.c \
    tape/tape.c \
    tape/tzx.c \
    wav/chunk.c \
    wav/rwave.c \
    fileutil.c \
    gzx.c \
    memio.c \
    midi.c \
    z80.c \
    z80dep.c \
    rs232.c \
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
    platform/sdl/byteorder.c \
    platform/sdl/gfx_sdl.c \
    platform/sdl/snd_sdl.c \
    platform/sdl/sys_unix.c \
    platform/sdl/sysmidi_alsa.c

sources_g = \
    $(sources) \
    z80g.c

sources_w32 = \
    $(sources_generic) \
    platform/win/byteorder.c \
    platform/win/gfx_win.c \
    platform/win/snd_win.c \
    platform/win/sys_win.c \
    platform/win/sysmidi_win.c

sources_w32_g = \
    $(sources_w32) \
    z80g.c

sources_helenos = \
    $(sources_generic) \
    platform/helenos/gfx.c \
    platform/helenos/snd.c \
    platform/helenos/sys.c

sources_helenos_g = \
    $(sources_helenos) \
    z80g.c

binary = gzx
binary_g = gzx-g
binary_w32 = gzx.exe
binary_w32_g = gzx-g.exe
binary_helenos = gzx-hos
binary_helenos_g = gzx-g-hos

objects = $(sources:.c=.o)
objects_g = $(sources_g:.c=.g.o)
objects_w32 = $(sources_w32:.c=.w32.o)
objects_w32_g = $(sources_w32_g:.c=.g.w32.o)
objects_helenos = $(sources_helenos:.c=.hos.o)
objects_helenos_g = $(sources_helenos_g:.c=.g.hos.o)
headers = $(wildcard *.h */*.h */*/*.h)

# Default target
default: $(binary)

all: $(binary) $(binary_g) $(binary_w32) $(binary_w32_g) $(binary_helenos) \
    $(binary_helenos_g)

w32: $(binary_w32) $(binary_w32_g)
hos: $(binary_helenos) $(binary_helenos_g)

install-hos: hos
	$(INSTALL) -d $(PREFIX_hos)/gzx
	$(INSTALL) -T $(binary_helenos) $(PREFIX_hos)/gzx/gzx
	$(INSTALL) -T $(binary_helenos_g) $(PREFIX_hos)/gzx/gzx_g
	$(INSTALL) -T font.bin $(PREFIX_hos)/gzx/font.bin
	$(INSTALL) -d $(PREFIX_hos)/gzx/roms
	$(INSTALL) -T roms/zx48.rom $(PREFIX_hos)/gzx/roms/zx48.rom
	$(INSTALL) -T roms/zx128_0.rom $(PREFIX_hos)/gzx/roms/zx128_0.rom
	$(INSTALL) -T roms/zx128_1.rom $(PREFIX_hos)/gzx/roms/zx128_1.rom

uninstall-hos:
	rm -f $(PREFIX_hos)/gzx/roms/zx48.rom
	rm -f $(PREFIX_hos)/gzx/roms/zx128_0.rom
	rm -f $(PREFIX_hos)/gzx/roms/zx128_1.rom
	rmdir $(PREFIX_hos)/gzx/roms
	rm -f $(PREFIX_hos)/gzx/gzx
	rm -f $(PREFIX_hos)/gzx/gzx_g
	rm -f $(PREFIX_hos)/gzx/font.bin
	rmdir $(PREFIX_hos)/gzx

test-hos: install-hos
	helenos-test

$(binary): $(objects)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(binary_g): $(objects_g)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(binary_w32): $(objects_w32)
	$(CC_w32) $(CFLAGS) -o $@ $^ $(LIBS_w32)

$(binary_w32_g): $(objects_w32_g)
	$(CC_w32) $(CFLAGS) -o $@ $^ $(LIBS_w32)

$(binary_helenos): $(objects_helenos)
	$(LD_helenos) -o $@ $^ $(LIBS_helenos)

$(binary_helenos_g): $(objects_helenos_g)
	$(LD_helenos) -o $@ $^ $(LIBS_helenos)

$(objects): $(headers)

%.g.o: %.c
	$(CC) -c $(CFLAGS_g) -o $@ $<

%.w32.o: %.c
	$(CC_w32) -c $(CFLAGS_w32) -o $@ $<

%.g.w32.o: %.c
	$(CC_w32) -c $(CFLAGS_w32_g) -o $@ $<

%.hos.o: %.c
	$(CC_helenos) -c $(CFLAGS_helenos) -o $@ $<

%.g.hos.o: %.c
	$(CC_helenos) -c $(CFLAGS_helenos_g) -o $@ $<

clean:
	rm -f *.o */*.o $(binary) $(binary_g) $(binary_w32) $(binary_w32_g)

backup: clean
	cd .. && tar czf gzx-$(bkqual).tar.gz trunk
	cd .. && rm -f gzx-latest.tar.gz && ln -s gzx-$(bkqual).tar.gz gzx-latest.tar.gz
