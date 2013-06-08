CC	= gcc
CFLAGS	= -O2 -mno-cygwin -march=i386 -ffast-math -Wall -I../../cygtst/mydirx/include
# CFLAGS	= -O6 -mcpu=pentium -ffast-math -Wall
LXOBJ	= gfx_vga.o snd_alsa.o
WINOBJ	= gfx_win.o snd_win.o sys_win.o
OBJ	= gzx.o z80.o snap.o zx_sound.o zx_tape.o zxt_tap.o zxt_wav.o\
 zxt_tzx.o zx_scr.o ay.o menus.o mgfx.o debug.o disasm.o

all: gzxwin.exe

gzx: $(OBJ) $(LXOBJ)
	gcc -o gzx -s $(OBJ) $(LXOBJ) -lefence -lvga -lasound

gzxwin.exe: $(OBJ) $(WINOBJ)
	gcc -mno-cygwin -o gzxwin.exe -s $(OBJ) $(WINOBJ) -lgdi32 -lwinmm

gzx.o: gzx.c

z80.o: z80.c

snap.o: snap.c

zx_sound.o: zx_sound.c

zx_tape.o: zx_tape.c

zxt_tap.o: zxt_tap.c

zxt_wav.o: zxt_wav.c

zxt_tzx.o: zxt_tzx.c

ay.o: ay.c

mgfx.o: mgfx.c

vgaw.o: vgaw.c

winw.o: winw.c

snd_alsa.o: snd_alsa.c

snd_win.o: snd_win.c

sys_win.o: sys_win.c

debug.o: debug.c

disasm.o: disasm.c

clean:
	rm *.o gzx

ay.c: Makefile.in global.h ay.h
	touch ay.c
da_itab.c: Makefile.in
	touch da_itab.c
debug.c: Makefile.in mgfx.h zx_scr.h z80.h global.h
	touch debug.c
disasm.c: Makefile.in da_itab.c
	touch disasm.c
gfx_gdi.c: Makefile.in global.h mgfx.h
	touch gfx_gdi.c
gfx_vga.c: Makefile.in mgfx.h
	touch gfx_vga.c
gfx_wdirx.c: Makefile.in global.h mgfx.h
	touch gfx_wdirx.c
gfx_win.c: Makefile.in global.h mgfx.h
	touch gfx_win.c
gzx.bk.c: Makefile.in mgfx.h global.h z80.h zx_keys.h zx_scr.h snap.h zx_sound.h zx_tape.h ay.h menus.h z80g.h
	touch gzx.bk.c
gzx.c: Makefile.in mgfx.h global.h z80.h zx_keys.h zx_scr.h snap.h zx_sound.h zx_tape.h ay.h menus.h debug.h z80g.h sys_all.h
	touch gzx.c
menus.c: Makefile.in global.h mgfx.h zx_scr.h zx_tape.h snap.h menus.h sys_all.h
	touch menus.c
mgfx.c: Makefile.in mgfx.h
	touch mgfx.c
snap.c: Makefile.in z80.h global.h ay.h
	touch snap.c
snd_alsa.c: Makefile.in global.h sndw.h
	touch snd_alsa.c
snd_win.c: Makefile.in global.h mgfx.h sndw.h
	touch snd_win.c
sys_win.c: Makefile.in sys_win.h
	touch sys_win.c
z80.c: Makefile.in global.h z80.h z80itab.c z80g.c
	touch z80.c
z80g.c: Makefile.in global.h z80g.h
	touch z80g.c
z80itab.c: Makefile.in
	touch z80itab.c
zx_scr.c: Makefile.in mgfx.h global.h zx_scr.h z80.h z80g.h
	touch zx_scr.c
zx_sound.c: Makefile.in global.h sndw.h
	touch zx_sound.c
zx_tape.c: Makefile.in global.h zx_tape.h z80.h zxt_fif.h
	touch zx_tape.c
zxt_tap.c: Makefile.in global.h zx_tape.h zxt_fif.h
	touch zxt_tap.c
zxt_tzx.c: Makefile.in global.h zx_tape.h zxt_fif.h
	touch zxt_tzx.c
zxt_wav.c: Makefile.in global.h zx_tape.h zxt_fif.h
	touch zxt_wav.c
ay.h: Makefile.in global.h
	touch ay.h
debug.h: Makefile.in
	touch debug.h
global.h: Makefile.in sys_win.h
	touch global.h
menus.h: Makefile.in
	touch menus.h
mgfx.h: Makefile.in
	touch mgfx.h
snap.h: Makefile.in global.h z80.h
	touch snap.h
sndw.h: Makefile.in
	touch sndw.h
sys_all.h: Makefile.in
	touch sys_all.h
sys_win.h: Makefile.in sys_all.h
	touch sys_win.h
z80.h: Makefile.in global.h
	touch z80.h
z80g.h: Makefile.in global.h
	touch z80g.h
zx_keys.h: Makefile.in
	touch zx_keys.h
zx_scr.h: Makefile.in global.h
	touch zx_scr.h
zx_sound.h: Makefile.in
	touch zx_sound.h
zx_tape.h: Makefile.in
	touch zx_tape.h
zxt_fif.h: Makefile.in zx_tape.h
	touch zxt_fif.h
