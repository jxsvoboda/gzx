GZX - George's ZX Spectrum Emulator
===================================
Copyright (c) 1999-2020 Jiri Svoboda

Introduction
------------
GZX is a ZX Spectrum emulator. I wrote it at a time where Linux had no
good emulators and it's been the only emulator I used ever since.

Some of the features of this emulator:

  * Run in Windows (DirectDraw or GDI) and Linux (SDL 1.x)
  * ZX Spectrum 48K, 128K
  * Joystick (Kempston) emulation
  * AY-3-8192
  * Read and write TZX, TAP, WAV tape formats
  * Z80, SNA, AY snapshots
  * Double line mode (i.e. interlaced image emulation)
  * ULAplus emulation
  * Spec256 (not 100% done)
  * Emulate built-in MIDI port of 128K Spectrum
  * Record audio to WAV file
  * Record data sent to I/O ports
  * Built-in debugger


Running
-------

In Linux:

    $ ./gzx [options]

In Windows:

    > gzx.exe [options]

Supported command-line options:

  Option           | Description
  ---------------  | -----------
  -midi <device>   | Output to specified MIDI device
  <snapshot-file>  | Load snapshot file at startup

Controls
--------
Note that most functions can be accessed via menus as well as via shortcuts.

  Key         | Function
  ----------- | --------
  Esc         | Open main menu
  F2          | Save snapshot
  F3          | Load snapshot
  F4          | Display options
  F5          | Tape menu
  F6          | Hardware options
  F7          | Reset to 48K mode
  F8          | Reset to 128K mode
  F9          | Select tapefile
  F10         | Quit
  F11         | Toggle fullscreen mode
  F12         | Enter debugger
  Alt-Shift-L | Lock down UI (disable emulator control keys)
  Alt-Shift-U | Unlock UI (reenable emulator control keys)

These might change or be removed in the future:

  Key         | Function
  ----------- | --------
  Alt-W       | Open Record Audio dialog to start recording audio to WAV
  Alt-E       | Stop recording audio
  Alt-R       | Start recording I/O port output to `out.ior`
  Alt-T       | Stop recording I/O port output
  Alt-N       | Select previous/none Spec256 background
  Alt-M       | Select next Spec256 background

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

Key functions while in the debugger
-----------------------------------

  Key          | Function
  ------------ | --------
  Esc or Enter | Exit debugger, return to normal emulation
  Up Arrow     | Move cursor to previous instruction
  Down Arrow   | Move cursor to next instruction
  Page Up      | Move cursor up one page
  Page Down    | Move cursor down one page
  Home         | Move cursor up by 256 bytes
  End          | Move cursor down by 256 bytes
  F7           | Trace into
  F8           | Step over
  F9           | Go to cursor (run until PC = cursor position)
  F11          | View Spectrum screen (while paused)

Joystick emulation
------------------
Cursor block keys are mapped to Kempston joystick, as follows:

  Key          | Spectrum Key
  ------------ | ------------
  Arrow keys   | Direction
  Insert       | Button 1
  Delete       | Button 2
  Home         | Button 3

About the double scanline mode
------------------------------
Some demos make use of the interlaced nature of PAL image by displaying
different images on alternate fields. This allows to either display
an image with higher vertical resolution (384 lines) or to display
more than two colors in a single cell.

These effects won't display correctly unless you enable double scanline mode
by enabling setting Double Line to Yes in the main menu. This can, however,
cause display artifacts so it should not be enabled all the time.

About Spec256 emulation
-----------------------
Spec256 was a DOS-based emulator that allowed playing spectrum games that
have been enhanced to 256 colors. GZX can play enhanced Spec256 games
(albeit not perfectly). To load a Spec256 enhanced game, select Load Snapshot
from the main menu and load the `.sna` file from the Spec256 game folder
(it should also contain a `.gfx` file). You can also use Alt-N, Alt-M keys
to cycle between Spec256 backgrounds (as automatic switching is not
implemented).

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

You may need to have some development packages installed. For details,
see http://www.helenos.org/wiki/UsersGuide/CompilingFromSource

Next you need to setup XCW tools which we use for the cross-compilation:
    $ PATH=$PATH:$PWD/tools/xcw/bin

Now go to your gzx workspace and off we go:
    $ cd ../gzx
    $ make test-hos

This will build the HelenOS binaries, install then to the HelenOS workspace
and start emulation. Once in HelenOS start GZX by typing

    # cd gzx
    # ./gzx

If you want to only build the binaries without installing, type
    $ make hos

If you want to only build and install the binaries without starting emulation,
type
    $ make install-hos

Now you need to go to root of your HelenOS workspace and type 'make' to re-build
the OS image.

Maintainance notes
------------------
To check ccstyle type

    $ make ccheck

This requires the `ccheck` tool from the [Sycek project][1]

[1]: https://github.com/jxsvoboda/sycek
