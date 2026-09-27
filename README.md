# RP6502 Project Template

Scaffolding for a new Picocomputer 6502 software project. It builds with
either 6502 compiler, cc65 or llvm-mos, and switching between them is one
setting. Three "Hello, world!" examples are included to start from:

 * `src/main.c` — C, and builds with either compiler.
 * `src/main-cc65.s` — assembly for cc65, which uses the ca65 syntax.
 * `src/main-llvm-mos.s` — the same program in llvm-mos assembly.

Make sure `CMakeLists.txt` points to the one you want, then delete the others.
The two assembly files pick up where the C runtime leaves off, so they read
alike; only the assembler directives differ.

### Requirements:
 * CMake 3.21 or newer
 * Python 3
 * Make or Ninja
 * [cc65 or llvm-mos](https://github.com/picocomputer?view_as=public).
   Install both if you want to try both.

The install steps for Windows, macOS and Linux are in
[RP6502-SDK](https://picocomputer.github.io/sdk.html#sdk-install), along with
the rest of the SDK documentation.

### Use the template:
Go to the [GitHub template](https://github.com/picocomputer/rp6502-sdk) and
select "Use this template" then "Create a new repository". Don't fork it: a
fork stays linked to this repository and includes its history, while "Use this
template" makes a new repository with a clean history. Then clone the new
repository.

```bash
$ git clone [path_to_github]
$ cd [to_where_it_cloned]
```

### Updating the tools:
`tools/` holds the CMake and Python scripts that the SDK runs. Update them with
the "RP6502: update tools" task (Terminal > Run Task), or with:

```bash
$ cmake -P tools/rp6502.cmake
```

### Updating an older project:
Projects made before this template merged cc65 and llvm-mos have their compiler
wired into the top of `CMakeLists.txt`, and a `tools/` that predates any of
this. Start by copying this template's `tools/rp6502.cmake` over yours. That
name used to be the cc65 toolchain file; it is now the small script that
fetches everything, and the toolchain it replaces comes back as
`tools/cc65-toolchain.cmake` on the first configure.

Then replace everything above `project()` with:

```cmake
cmake_minimum_required(VERSION 3.21)

include(${CMAKE_CURRENT_LIST_DIR}/tools/rp6502.cmake)
```

Delete `tools/CMakeLists.txt` and the `add_subdirectory(tools)` line that
pulled it in — the `include()` above replaces both. Copy `CMakePresets.json`
from this template as well; that is where the compiler is chosen now. Old
projects called `rp6502_executable()` with the address their compiler happened
to use, and `DATA default RESET default` works under both.
