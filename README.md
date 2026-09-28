# Flappy Coo

Flappy Coo is a Flappy Bird game for the
[Picocomputer 6502](https://picocomputer.github.io), with a highland cow, a
"coo", in place of the bird. The game is small, but each part of a full
game is in it: screens, a saved best score, animated sprites, scrolling
tiles, a bitmap, text, sound and three kinds of input. Copy it to start a
game of your own. It builds with either 6502 compiler, cc65 or llvm-mos.

[![Play Flappy Coo](https://rumbledethumps.github.io/flappycoo/title.png)](https://rumbledethumps.github.io/flappycoo/)

[Play it in your browser](https://rumbledethumps.github.io/flappycoo/).

## Playing

Flap through the gaps between the pipes. Each pipe passed scores a point.

| Action | Keyboard | Mouse, pen or touchscreen | Gamepad |
|---|---|---|---|
| Flap, start | Space or Enter | click or tap | A, B, X or Y |
| Pause | P | right click | Start |

The screen and the help text name only Space, click, P and right click.

Flappy Coo needs version 0.34 or newer of the Picocomputer firmware or the
emulator. Custom-size sprites and the `SAVE:` drive are new in 0.34.

## Requirements

 * CMake 3.21 or newer
 * Python 3
 * Make or Ninja
 * [cc65 or llvm-mos](https://github.com/picocomputer?view_as=public).
 * gcc for your computer, to run the unit tests there. On Windows, install
   it with `winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT` and
   open a new terminal. The `host` preset names gcc, because the llvm-mos
   `bin` folder contains a clang that builds only 6502 programs.

The install steps for Windows, macOS and Linux are in
[RP6502-SDK](https://picocomputer.github.io/sdk.html#sdk-install), along with
the rest of the SDK documentation.

## Building and running

In VS Code, choose a preset and press F5, as
[Getting Started](https://picocomputer.github.io/sdk.html#getting-started)
describes.

On the command line:

```bash
$ cmake --preset cc65/Release
$ cmake --build --preset cc65/Release
$ tools/rp6502-emu build/cc65/release/flappycoo.rp6502
```

On Windows and WSL the emulator is `tools/rp6502-emu.exe`. The first
configure downloads the emulator into `tools/`, so a fresh clone needs a
network connection once.

The two ROMs hold the same image data and help text and differ only in the
program. Both pass the same emulator scripts and produce identical
screenshots, so CI releases the smaller ROM. The sizes change with each
code edit and compiler update, so they are not listed here. The job
summary of each CI run lists both.

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
| Input | `src/input.c` | The keyboard, the tablet and up to four gamepads are read into two buttons: flap and pause. A mouse, a pen and each finger on a touchscreen are all contacts of the tablet device. |
| Help | `src/help.txt` | Shown by HELP and INFO on a Picocomputer and in the ROM Help window of the emulator. |
| Web player | `index.html` | The page on GitHub Pages, with the click-to-play overlay, the footer, and the title screenshot as the picture. See [RP6502-WEB](https://picocomputer.github.io/web.html). |

The device docs are [Keyboard](https://picocomputer.github.io/ria.html#ria-keyboard),
[Tablet](https://picocomputer.github.io/ria.html#tablet),
[Gamepads](https://picocomputer.github.io/ria.html#ria-gamepads) and
[PSG](https://picocomputer.github.io/ria.html#programmable-sound-generator).

## Planes

The canvas is 320x240. Plane 0 is the back and plane 2 is the front, and
in each plane the sprite layer is drawn over the fill layer.

| Plane | Fill layer | Sprite layer |
|---|---|---|
| 0 | Sky, [mode 2](https://picocomputer.github.io/vga.html#vga-mode-2) tiles at 4 bpp, scanlines 0-215 | |
| 1 | Logo, [mode 3](https://picocomputer.github.io/vga.html#vga-mode-3) bitmap at 4 bpp | Pipes, [mode 5](https://picocomputer.github.io/vga.html#vga-mode-5) sprites at 4 bpp, scanlines 0-215 |
| 2 | Text, [mode 1](https://picocomputer.github.io/vga.html#vga-mode-1) at 1 bpp, scanlines 0-215; ground, mode 2 tiles at 4 bpp, scanlines 216-239 | Coo, one mode 5 sprite at 8 bpp |

## Images

The build converts the PNGs in the table below with `img/png2bin.py`,
which needs only Python 3. `CMakeLists.txt` holds one
`add_custom_command()` per PNG, so a new PNG in `img/` is converted only
after a command is added for it. `rp6502_asset()` adds each converted file
that the game uses to the ROM with an XRAM address, and the files are
loaded into XRAM before the 6502 starts. The program has no loading code,
and no image passes through 6502 RAM.
[Adding Assets](https://picocomputer.github.io/sdk.html#sdk-assets)
explains `rp6502_asset()`.

| Image | png2bin.py command | XRAM data | Sizes in the code |
|---|---|---|---|
| `coo.png` | `sprite 8 52x36` | 18 sprite images, 256 colors | `COO_W`, `COO_H` and `COO_FRAME_COUNT` in `src/game.h` |
| `logo.png` | `bitmap 4` | a 160x82 bitmap, 16 colors | `LOGO_W` and `LOGO_H` in `src/xram.h` |
| `pipe_body.png` | `sprite 4 32x64` | one sprite image, 16 colors | `PIPE_BODY_W` and `PIPE_BODY_H` in `src/xram.h` |
| `pipe_cap.png` | `sprite 4 36x12` | one sprite image, drawn with the body palette. The palette in `pipe_cap.png` must match the palette in `pipe_body.png`, because the cap palette is not loaded. | `PIPE_W` in `src/game.h` and `PIPE_CAP_H` in `src/xram.h` |
| `sky.png` | `tiles 4` | 237 tiles, a 64x27 map, 16 colors | `SKY_MAP_W` and `SKY_MAP_H` in `src/xram.h` |
| `ground.png` | `tiles 4` | 18 tiles, an 8x3 map, 16 colors | `GROUND_MAP_W` and `GROUND_MAP_H` in `src/xram.h` |

`concept.png` is the concept art that the coo frames and the logo were cut
from. The build does not use it.

Each converted PNG must be:

 * indexed color (a palette image), not interlaced, at 1, 2, 4 or 8 bits
   per pixel;
 * saved with at most 2^bpp palette entries, 16 at 4 bpp or 256 at 8 bpp,
   which is the size of the matching palette in `src/xram.h`;
 * transparent at index 0, with alpha 0. An entry with alpha below 128
   becomes the color 0x0000, which the VGA draws as transparent;
 * a whole number of frames for `sprite`, each frame 4 to 64 pixels wide
   and high in steps of 4, or a multiple of 8 pixels with at most 256
   different tiles for `tiles`;
 * for `sky.png` and `ground.png`, a power-of-two number of tiles wide (64
   and 8 now), because the scroll positions wrap with a mask.

The converter stops the build with a message when an image is not
indexed, is interlaced, has more colors or tiles than fit, or is not a
whole number of frames, or when a sprite frame size is not 4 to 64 pixels
in steps of 4. The compiler stops the build when `SKY_MAP_W` or
`GROUND_MAP_W` in `src/xram.h` is not a power of two. Colors are RGB555,
so the lowest 3 bits of each red, green and blue value are dropped. See
[Colors, Palettes and Fonts](https://picocomputer.github.io/vga.html#colors-palettes-and-fonts).

The sizes in the last column of the table above are not read from the
PNGs, and no check compares the two. When a size does not match the PNG,
the game draws garbage and the tests still pass. Data larger than the
space for it in `xram_layout_t` usually stops the build with "ROM data
already exists at $...", where the address is that of the data after it.

## XRAM layout

`src/xram.h` holds the structures of the devices the game uses, copied from
the RIA and VGA docs, and `xram_layout_t`, which places all the XRAM data.
Each `XRAM_` name is an `offsetof()` in that structure. `rp6502_map()`
reads the names, so `CMakeLists.txt` and the C code use the same addresses
and no address is written twice. See
[XRAM Memory Map](https://picocomputer.github.io/sdk.html#sdk-xram-memory-map).

| Data, in order | Bytes | Why |
|---|---|---|
| PSG | 64 | First, because the PSG must not cross a 256-byte page. |
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
| `host_tests` | The rules in `src/game.c`, tested by `tests/test_game.c` with [utest.h](https://github.com/sheredom/utest.h), including a bot that must pass 50 pipes. | `host` |
| `keyboard`, `tablet`, `gamepad` | Each device starts a game, pauses it, resumes it and pauses it again, and the canvas stays still while paused. A tablet pointer that only hovers does not start a game. | cc65, llvm-mos |
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

The `tests/*.script` files are emulator scripts. They press keys and
buttons, run frames, and compare the canvas. They run with `--seed 1`, so
every run is the same. [Scripting](https://picocomputer.github.io/emu.html#scripting)
lists the commands.

## CI

`.github/workflows/ci.yml` runs on each push and pull request to `main`. It
runs the host tests, builds and tests cc65 Release and llvm-mos Release,
and uploads the smaller `flappycoo.rp6502` with the screenshots from both
builds as a workflow artifact. On `main`, it also publishes the ROM as a
release tagged `build-<commit>`, which becomes the Latest release only when
`main` is still at that commit. The job summary lists both ROM sizes and
which ROM was released, and the notes of each release give both sizes.

On `main`, CI also publishes the web player to GitHub Pages: `index.html`,
the released ROM, `title.png` from the `screenshots` test, and
`rp6502.js` and `rp6502.wasm` from the latest rp6502 release. Like the
Latest release, the page is replaced only when `main` is still at that
commit.

## Starting your own game

 * Replace `flappycoo` and `Flappy Coo` with the name of the new game
   everywhere in `CMakeLists.txt`, `.github/workflows/ci.yml` and
   `index.html`, and set `db` in `index.html` to your user name and the
   name of the game.
 * Turn on GitHub Pages with Settings > Pages > Source: GitHub Actions,
   or delete the `pages` job from `.github/workflows/ci.yml`. Change the
   play link at the top of this README to the new Pages address.
 * Rename `SAVE:flappycoo.hiscore` in `src/main.c`, so the scores of two
   games are in separate files.
 * Replace `src/help.txt`.
 * Replace the PNGs, then change each png2bin command in `CMakeLists.txt`
   and the sizes that the Images table lists for that PNG. Fit the
   `COO_FRAME_` frame numbers and the `COO_HIT_` hitbox in `src/game.h` to
   the new coo. There must be three rise frames, so `COO_FRAME_DIVE` is
   `COO_FRAME_RISE + 3`.

## Updating the tools

`tools/` holds the CMake and Python scripts that the SDK runs. Update them
with the "RP6502: update tools" task (Terminal > Run Task), or with:

```bash
$ cmake -P tools/rp6502.cmake
```

A configure downloads the emulator only when `tools/` has none. An update
replaces it with the latest release. Commit the updated files in `tools/`.
