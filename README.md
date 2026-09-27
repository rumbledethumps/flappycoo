# Flappy Coo

Flappy Coo is a Flappy Bird game for the
[Picocomputer 6502](https://picocomputer.github.io), with a highland cow, a
"coo", in place of the bird. The game is small, but each part of a full
game is in it: screens, a saved best score, animated sprites, scrolling
tiles, a bitmap, text, sound and three kinds of input. Copy it to start a
game of your own. It builds with either 6502 compiler, cc65 or llvm-mos.

## Playing

Flap through the gaps between the pipes. Each pipe passed scores a point.

| Action | Keyboard | Mouse, pen or touchscreen | Gamepad |
|---|---|---|---|
| Flap, start | Space, Up or W | click or tap | A, B, X, Y or up |
| Pause | P or Enter | right click | Start |
| Quit | Escape | | |

Flappy Coo needs version 0.34 or newer of the Picocomputer firmware or the
emulator. Custom-size sprites and the `SAVE:` drive are new in 0.34.

## Requirements

 * CMake 3.21 or newer
 * Python 3
 * Make or Ninja
 * [cc65 or llvm-mos](https://github.com/picocomputer?view_as=public).
   The unit-test ROM needs llvm-mos, so install both to run every test.
 * gcc or clang for your computer, to run the unit tests there.

The install steps for Windows, macOS and Linux are in
[RP6502-SDK](https://picocomputer.github.io/sdk.html#sdk-install), along with
the rest of the SDK documentation.

## Building and running

In VS Code, choose a preset and press F5, as
[Getting Started](https://picocomputer.github.io/sdk.html#getting-started)
describes. The llvm-mos presets also build the unit-test ROM, `tests`, so
choose `flappycoo` as the launch target in the CMake side panel.

On the command line:

```bash
$ cmake --preset cc65/Release
$ cmake --build --preset cc65/Release
$ tools/rp6502-emu build/cc65/release/flappycoo.rp6502
```

On Windows and WSL the emulator is `tools/rp6502-emu.exe`. The first
configure downloads the emulator into `tools/`, so a fresh clone needs a
network connection once.

| Release build | Program (bytes) | ROM (bytes) |
|---|---|---|
| cc65 | 6051 | 59713 |
| llvm-mos | 10376 | 64144 |

The rest of each ROM is the same image data and help text. Both builds
pass the same tests and produce identical screenshots, so CI releases the
smaller ROM, which today is the cc65 ROM. llvm-mos Release builds with `-O3`,
which optimizes for speed rather than size.

## Where each part is

| Part | Files | How |
|---|---|---|
| Frame loop | `src/main.c` | Each frame: wait for VSYNC, draw, read the input, run the rules, start sounds, save a new best score. Drawing is first, in the blanking time after VSYNC, so that no sprite moves while the canvas is drawn. |
| Rules and states | `src/game.c`, `src/game.h` | `game.state` is title, play, fall or game over, and `game.paused` is pause. The files include no Picocomputer headers, so the unit tests build on any computer. |
| Score and best score | `src/main.c` | The best score is 2 bytes in `SAVE:flappycoo.hiscore`, read at start and written on each new best. See [Saves](https://picocomputer.github.io/port.html#port-save). |
| Text layer | `src/video.c` | The words for each screen, in the built-in font, redrawn only when the screen or the score changes. |
| Sprite animation | `img/coo.png`, `src/video.c` | 18 frames of 52x36 pixels at 8 bits per pixel, with a separate 256-color palette for the coo. Each frame, the image address in the coo sprite is set to the frame in `game.coo_frame`. |
| Pipes | `img/pipe_body.png`, `img/pipe_cap.png` | 12 sprites, 4 per pipe. The 64-row body image is drawn at double height to cover the longest pipe. Every row of the body is the same, so the doubling does not show. |
| Tile backgrounds | `img/sky.png`, `img/ground.png` | Tile maps that wrap. The sky scrolls at a quarter of the ground speed. |
| Bitmap logo | `img/logo.png` | On the title screen. Otherwise it is moved off the canvas. |
| Sound | `src/sound.c` | One PSG channel each for the flap, the score, a hit and the "moo" at game over. |
| Input | `src/input.c` | The keyboard, the tablet and up to four gamepads are read into three buttons: flap, pause and quit. A mouse, a pen and each finger on a touchscreen are all contacts of the tablet device. |
| Help | `src/help.txt` | Shown by HELP and INFO on a Picocomputer and in the ROM Help window of the emulator. |

The device docs are [Keyboard](https://picocomputer.github.io/ria.html#ria-keyboard),
[Tablet](https://picocomputer.github.io/ria.html#tablet),
[Gamepads](https://picocomputer.github.io/ria.html#ria-gamepads) and
[PSG](https://picocomputer.github.io/ria.html#programmable-sound-generator).

## Planes

The canvas is 320x240. Plane 0 is the back and plane 2 is the front, and
each plane's sprite layer is drawn over its fill layer.

| Plane | Fill layer | Sprite layer |
|---|---|---|
| 0 | Sky, [mode 2](https://picocomputer.github.io/vga.html#vga-mode-2) tiles at 4 bpp, scanlines 0-215 | |
| 1 | Logo, [mode 3](https://picocomputer.github.io/vga.html#vga-mode-3) bitmap at 4 bpp | Pipes, [mode 5](https://picocomputer.github.io/vga.html#vga-mode-5) sprites at 4 bpp, scanlines 0-215 |
| 2 | Text, [mode 1](https://picocomputer.github.io/vga.html#vga-mode-1) at 1 bpp, scanlines 0-215; ground, mode 2 tiles at 4 bpp, scanlines 216-239 | Coo, one mode 5 sprite at 8 bpp |

## Images

The build converts each PNG in `img/` with `img/png2bin.py`, which needs
only Python 3. `rp6502_asset()` in `CMakeLists.txt` adds each output file
to the ROM with an XRAM address, and the files are loaded into XRAM before
the 6502 starts. The program has no loading code, and no image passes
through 6502 RAM. [Adding Assets](https://picocomputer.github.io/sdk.html#sdk-assets)
explains `rp6502_asset()`.

| Image | png2bin.py command | XRAM data |
|---|---|---|
| `coo.png` | `sprite 8 52x36` | 18 sprite images, 256 colors |
| `logo.png` | `bitmap 4` | a 160x82 bitmap, 16 colors |
| `pipe_body.png` | `sprite 4 32x64` | one sprite image, 16 colors |
| `pipe_cap.png` | `sprite 4 36x12` | one sprite image; the palette is the same as the body palette, so it is not loaded |
| `sky.png` | `tiles 4` | 237 tiles, a 64x27 map, 16 colors |
| `ground.png` | `tiles 4` | 18 tiles, an 8x3 map, 16 colors |

`concept.png` is the concept art that the coo frames and the logo were cut
from. The build does not use it.

Each PNG must be:

 * indexed color (a palette image), not interlaced, at 1, 2, 4 or 8 bits
   per pixel;
 * saved with exactly 2^bpp palette entries, 16 at 4 bpp or 256 at 8 bpp,
   which is the size of its palette in `src/xram.h`;
 * transparent at index 0, with alpha 0. An entry with alpha below 128
   becomes the color 0x0000, which the VGA draws as transparent;
 * a whole number of frames for `sprite`, or a multiple of 8 pixels with
   at most 256 different tiles for `tiles`.

The converter stops the build with a message when an image is not
indexed, is interlaced, has more colors or tiles than fit, or is not a
whole number of frames. Colors are RGB555, so the lowest 3 bits of each
red, green and blue value are dropped. See
[Colors, Palettes and Fonts](https://picocomputer.github.io/vga.html#colors-palettes-and-fonts).

## XRAM layout

`src/xram.h` holds the structures of the devices the game uses, copied from
the RIA and VGA docs, and `xram_layout_t`, which places all the XRAM data.
Each `XRAM_` name is an `offsetof()` in that structure. `rp6502_map()`
reads the names, so `CMakeLists.txt` and the C code use the same addresses
and no address is written twice. See
[XRAM Memory Map](https://picocomputer.github.io/sdk.html#sdk-xram-memory-map).

| Data, in order | Bytes | Why |
|---|---|---|
| PSG | 64 | First, because its 64 bytes must not cross a 256-byte page. |
| Palettes | 644 | Together, because sprites read colors through a 1 KB direct-mapped cache, and colors within 1 KB of each other never collide in it. |
| Mode configurations and sprites | 192 | |
| Keyboard, tablet, gamepads | 124 | |
| Text | 1080 | 40x27 characters. |
| Sky and ground maps | 1752 | |
| Sky and ground tiles | 8960 | Room for 256 sky tiles, so new art does not move the addresses after it. |
| Coo frames | 33696 | |
| Logo | 6560 | |
| Pipe body and cap | 1240 | |

The layout uses 54312 bytes and leaves 11224 free.

## Tests

| Test | What it checks | Presets |
|---|---|---|
| `host_tests` | The rules in `src/game.c`: 20 cases in `tests/test_game.c`, with [utest.h](https://github.com/sheredom/utest.h), including a bot that must pass 50 pipes. | `host` |
| `unit_tests` | The same cases as a ROM, `tests.rp6502`, where `int` is 16 bits as it is in the game. | llvm-mos |
| `boot` | The ROM starts, and Escape quits with exit code 0. | cc65, llvm-mos |
| `keyboard`, `tablet`, `gamepad` | Each device starts a game, then pauses it, and the canvas stays still while paused. | cc65, llvm-mos |
| `screenshots` | Writes `title.png`, `play.png`, `paused.png` and `over.png` into the build directory. | cc65, llvm-mos |

The host tests:

```bash
$ cmake --preset host
$ cmake --build --preset host
$ ctest --preset host
```

Each compiler preset has a test preset of the same name, to run after a
build with that preset:

```bash
$ ctest --preset llvm-mos/Release
```

cc65 cannot compile utest.h, which needs 64-bit integers, so the cc65 ROM
is tested by the emulator scripts only.

The `tests/*.script` files are emulator scripts. They press keys and
buttons, run frames, and compare the canvas. They run with `--seed 1`, so
every run is the same. [Scripting](https://picocomputer.github.io/emu.html#scripting)
lists the commands.

The llvm-mos linker does not check that the heap stays clear of the C
stack. `tests/heap-check.ld` makes the link fail with "test ROM heap
overlaps the soft stack" when the test ROM grows too large for RAM.

## CI

`.github/workflows/ci.yml` runs on each push and pull request to `main`. It
runs the host tests, builds and tests cc65 Release and llvm-mos Release,
and uploads the smaller `flappycoo.rp6502` with the screenshots from both
builds as a workflow artifact. On `main`, it also publishes the ROM as a
release tagged `build-<commit>`. The job summary lists both ROM sizes.

## Starting your own game

 * Rename the target `flappycoo` in `CMakeLists.txt`, and the ROM paths in
   `.github/workflows/ci.yml`.
 * Rename `SAVE:flappycoo.hiscore` in `src/main.c`, so the scores of two
   games are in separate files.
 * Replace `src/help.txt`.
 * Replace the PNGs, then change the sizes in `src/xram.h` and
   `src/game.h` to match.

## Updating the tools

`tools/` holds the CMake and Python scripts that the SDK runs. Update them
with the "RP6502: update tools" task (Terminal > Run Task), or with:

```bash
$ cmake -P tools/rp6502.cmake
```

A configure downloads the emulator only when `tools/` has none. An update
replaces it with the latest release. Commit the updated files in `tools/`.
