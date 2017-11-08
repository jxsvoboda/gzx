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

Running
-------

In Linux:

    $ ./gzx [options]

In Windows:

    > gzx.exe [options]

Supported command-line options:

  Option          | Description
  --------------- | -----------
  -dl             | Enable double-scanline mode
  -midi <device>  | Output to specified MIDI device
  <snapshot-file> | Load snapshot file at startup

`gzx-g` and `gzx-g.exe` are variants with GPU/Spec256 support enabled

Controls
--------
Note that most functions can be accessed via menus as well as via shortcuts.

  Key | Function
  --- | --------
  Esc | Open main menu
  F2  | Save snapshot
  F3  | Load snapshot
  F5  | Tape menu
  F7  | Reset to 48K mode
  F8  | Reset to 128K mode
  F10 | Quit
  F12 | Enter debugger

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
