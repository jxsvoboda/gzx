GZX - George's ZX Spectrum Emulator
===================================
Copyright (c) 1999-2017 Jiri Svoboda

Introduction
------------
GZX is a ZX Spectrum emulator. I wrote it at a time where Linux had no
good emulators and it's been the only emulator I used ever since.
It still needs a lot of polishing - some of the more advanced features
are difficult to use by a non-developer and the source code is a bit
messy. But it is nevertheless usable.

Software compatibility is a bit lacking. (This is due to bugs that need
fixing, we do support some rather advanced/undocumented CPU features).

Some of the features of this emulator:

  * Run in Windows (DirectDraw or GDI) and Linux (SDL 1.x)
  * ZX Spectrum 48K, 128K
  * AY-3-8192
  * Z80, SNA, AY snapshots
  * Read TAP, TZX, WAV tape formats
  * Write TAP
  * High vertical resolution, Spec256 (experimental)
  * Emulate built-in MIDI port of 128K Spectrum
  * Record data sent to I/O ports
  * Built-in debugger

To get the latest source
------------------------

    $ hg clone https://bitbucket.org/jiri_svoboda/gzx/ gzx/trunk
    $ cd gzx/trunk

Compiling
---------

You need a working GNU toolchain (GCC, Binutils, Make) (Linux or similar OS)
and SDL 1.2 development package. To build Windows binaries, you need MingW64
cross-compiler.

To build just the 'gzx' binary

    $ make

To build all binaries, including Windows binaries:

    $ make all

Cross-compiling for HelenOS
---------------------------

You need a built HelenOS workspace and a working cross-compiler toolchain.
If you don't have one, you need to do something like

    $ git clone https://github.com/HelenOS/helenos.git helenos
    $ cd helenos
    $ sudo tools/toolchain.sh amd64
    $ make PROFILE=amd64

For details, see http://www.helenos.org/wiki/UsersGuide/CompilingFromSource

Next you need to setup XCW tools which we use for the cross-compilation:
    $ PATH=$PATH:$PWD/tools/xcw/bin

Now go to your gzx workspace and off we go:
    $ cd ../gzx
    $ make hos-test

This will build the HelenOS binaries, install then to the HelenOS workspace
and start emulation. Once in HelenOS start GZX by typing

    # cd gzx
    # ./gzx

If you want to only build the binaries without installing, type
    $ make hos

If you want to only build and install the binaries without starting emulation,
type
    $ make hos-install

Now you need to go to root of your HelenOS workspace and type 'make' to re-build
the OS image.

Running
-------

In Linux:

    $ ./gzx [options]

In Windows:

    > gzx.exe [options]

Supported command-line options:

  Option           | Description
  ---------------  | -----------
  -acap <file.wav> | Capture audio output to WAVE file
  -dl              | Enable double-scanline mode
  -midi <device>   | Output to specified MIDI device
  <snapshot-file>  | Load snapshot file at startup

`gzx-g` and `gzx-g.exe` are variants with GPU/Spec256 support enabled

Controls
--------
Note that most functions can be accessed via menus as well as via shortcuts.

  Key         | Function
  ----------- | --------
  Esc         | Open main menu
  F2          | Save snapshot
  F3          | Load snapshot
  F5          | Tape menu
  F7          | Reset to 48K mode
  F8          | Reset to 128K mode
  F10         | Quit
  F12         | Enter debugger
  Alt-Shift-L | Lock down UI (disable emulator control keys)
  Alt-Shift-U | Unlock UI (reenable emulator control keys)

On the numerical keypad:

  Key | Function
  --- | --------
  +   | Start the tape
  -   | Stop the tape
  *   | Rewind the tape
  /   | Toggle quick load

Spectrum Key Mappings
---------------------

  Key          | Spectrum Key
  ------------ | ------------
  Shift        | Caps Shift
  Ctrl         | Symbol Shift
  Shift+Arrows | Cursor Keys
  Backspace    | Delete
