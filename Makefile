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
version = git
distname = gzx-$(version)
distbase = distrib
distdir = $(distbase)/$(distname)

sources_generic = \
    adt/list.c \
    tape/deck.c \
    tape/player.c \
    tape/quick.c \
    tape/tap.c \
    tape/tape.c \
    tape/tonegen.c \
    tape/tzx.c \
    tape/wav.c \
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
    zx_scr.c \
    ay.c \
    menus.c \
    mgfx.c \
    debug.c \
    disasm.c \
    iorec.c

sources_gtap_generic = \
    adt/list.c \
    strutil.c \
    tape/deck.c \
    tape/player.c \
    tape/romblock.c \
    tape/tap.c \
    tape/tape.c \
    tape/tonegen.c \
    tape/tzx.c \
    tape/wav.c \
    gtap.c \
    wav/chunk.c \
    wav/rwave.c

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

sources_gtap = \
    $(sources_gtap_generic) \
    platform/sdl/byteorder.c

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

sources_w32_gtap = \
    $(sources_gtap_generic) \
    platform/win/byteorder.c

sources_helenos = \
    $(sources_generic) \
    platform/helenos/gfx.c \
    platform/helenos/snd.c \
    platform/helenos/sys.c

sources_helenos_g = \
    $(sources_helenos) \
    z80g.c

sources_helenos_gtap = \
    $(sources_gtap)

sources_test = \
    adt/list.c \
    tape/player.c \
    tape/tape.c \
    tape/tonegen.c \
    test/main.c \
    test/tape/player.c \
    test/tape/tonegen.c

binary = gzx
binary_g = gzx-g
binary_gtap = gtap
binary_w32 = gzx.exe
binary_w32_g = gzx-g.exe
binary_w32_gtap = gtap.exe
binary_helenos = gzx-hos
binary_helenos_g = gzx-g-hos
binary_helenos_gtap = gtap-hos
binary_test = test-gzx

objects = $(sources:.c=.o)
objects_g = $(sources_g:.c=.g.o)
objects_gtap = $(sources_gtap:.c=.o)
objects_w32 = $(sources_w32:.c=.w32.o)
objects_w32_g = $(sources_w32_g:.c=.g.w32.o)
objects_w32_gtap = $(sources_w32_gtap:.c=.g.w32.o)
objects_helenos = $(sources_helenos:.c=.hos.o)
objects_helenos_g = $(sources_helenos_g:.c=.g.hos.o)
objects_helenos_gtap = $(sources_helenos_gtap:.c=.g.hos.o)
objects_test = $(sources_test:.c=.o)

headers = $(wildcard *.h */*.h */*/*.h)

ccheck_list = $(shell find . -name '*.[ch'] | grep -vxFf .ccheck_not)

# Default target
default: $(binary) $(binary_gtap)

all: $(binary) $(binary_g) $(binary_gtap) $(binary_w32) $(binary_w32_g) \
    $(binary_w32_gtap) $(binary_helenos) $(binary_helenos_g) \
    $(binary_helenos_gtap) $(binary_test)

w32: $(binary_w32) $(binary_w32_g) $(binary_w32_gtap)
hos: $(binary_helenos) $(binary_helenos_g) $(binary_helenos_gtap)

install-hos: hos
	$(INSTALL) -d $(PREFIX_hos)/gzx
	$(INSTALL) -T $(binary_helenos) $(PREFIX_hos)/gzx/gzx
	$(INSTALL) -T $(binary_helenos_g) $(PREFIX_hos)/gzx/gzx-g
	$(INSTALL) -T $(binary_helenos_gtap) $(PREFIX_hos)/gzx/gtap
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
	rm -f $(PREFIX_hos)/gzx/gzx-g
	rm -f $(PREFIX_hos)/gzx/gtap
	rm -f $(PREFIX_hos)/gzx/font.bin
	rmdir $(PREFIX_hos)/gzx

test-hos: install-hos
	helenos-test

test: $(binary_test)
	./$(binary_test)

dist: $(binary) $(binary_g) $(binary_gtap) $(binary_w32) $(binary_w32_g) \
     $(binary_w32_gtap)
	mkdir -p $(distdir)
	cp -t $(distdir) $^
	cp -r -t $(distdir) roms
	cp -r -t $(distdir) font.bin
	cp -r -t $(distdir) README.md
	echo $(version) > $(distdir)/VERSION
	rm -rf $(distbase)/$(distname)/gzx.zip
	cd $(distbase) && zip -r $(distname).zip $(distname)

ccheck:
	ccheck-run.sh $(ccheck_list)

$(binary): $(objects)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(binary_g): $(objects_g)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(binary_gtap): $(objects_gtap)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(binary_w32): $(objects_w32)
	$(CC_w32) $(CFLAGS_w32) -o $@ $^ $(LIBS_w32)

$(binary_w32_g): $(objects_w32_g)
	$(CC_w32) $(CFLAGS_w32) -o $@ $^ $(LIBS_w32)

$(binary_w32_gtap): $(objects_w32_gtap)
	$(CC_w32) $(CFLAGS_w32) -o $@ $^ $(LIBS_w32)

$(binary_helenos): $(objects_helenos)
	$(LD_helenos) -o $@ $^ $(LIBS_helenos)

$(binary_helenos_g): $(objects_helenos_g)
	$(LD_helenos) -o $@ $^ $(LIBS_helenos)

$(binary_helenos_gtap): $(objects_helenos_gtap)
	$(LD_helenos) -o $@ $^ $(LIBS_helenos)

$(binary_test): $(objects_test)
	$(CC) $(CFLAGS) -o $@ $^

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
	rm -f *.o */*.o */*/*.o $(binary) $(binary_g) $(binary_gtap) \
	    $(binary_w32) $(binary_w32_g) $(binary_w32_gtap) $(binary_helenos) \
	    $(binary_helenos_g) $(binary_helenos_gtap) $(binary_test)
	rm -rf distrib

backup: clean
	cd .. && tar czf gzx-$(bkqual).tar.gz trunk
	cd .. && rm -f gzx-latest.tar.gz && ln -s gzx-$(bkqual).tar.gz gzx-latest.tar.gz
