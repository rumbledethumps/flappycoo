cmake_minimum_required(VERSION 3.21)

set(TEST_SOURCES tests/main.c tests/test_game.c src/game.c)

# The unit tests, built with the host C compiler
if(HOST_TESTS)
    project(flappycoo C)
    enable_testing()
    add_executable(host_tests ${TEST_SOURCES})
    target_include_directories(host_tests PRIVATE src tests)
    set_target_properties(host_tests PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
    )
    # Both 6502 compilers make char unsigned.
    target_compile_options(host_tests PRIVATE -funsigned-char -Wall -Wextra)
    add_test(NAME host_tests COMMAND host_tests)
    return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/tools/rp6502.cmake)

project(flappycoo C)

enable_testing()
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# PNG conversion to XRAM data files, one command per image
add_custom_command(
    OUTPUT coo.bin coo_pal.bin
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/img/png2bin.py
        sprite 8 52x36 ${CMAKE_CURRENT_SOURCE_DIR}/img/coo.png
        coo.bin coo_pal.bin
    DEPENDS img/png2bin.py img/coo.png
    VERBATIM
)
add_custom_command(
    OUTPUT logo.bin logo_pal.bin
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/img/png2bin.py
        bitmap 4 ${CMAKE_CURRENT_SOURCE_DIR}/img/logo.png
        logo.bin logo_pal.bin
    DEPENDS img/png2bin.py img/logo.png
    VERBATIM
)
add_custom_command(
    OUTPUT pipe_body.bin pipe_pal.bin
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/img/png2bin.py
        sprite 4 32x64 ${CMAKE_CURRENT_SOURCE_DIR}/img/pipe_body.png
        pipe_body.bin pipe_pal.bin
    DEPENDS img/png2bin.py img/pipe_body.png
    VERBATIM
)
# The cap is drawn with the body palette, so pipe_cap_pal.bin is not loaded.
add_custom_command(
    OUTPUT pipe_cap.bin pipe_cap_pal.bin
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/img/png2bin.py
        sprite 4 36x12 ${CMAKE_CURRENT_SOURCE_DIR}/img/pipe_cap.png
        pipe_cap.bin pipe_cap_pal.bin
    DEPENDS img/png2bin.py img/pipe_cap.png
    VERBATIM
)
add_custom_command(
    OUTPUT sky_tiles.bin sky_map.bin sky_pal.bin
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/img/png2bin.py
        tiles 4 ${CMAKE_CURRENT_SOURCE_DIR}/img/sky.png
        sky_tiles.bin sky_map.bin sky_pal.bin
    DEPENDS img/png2bin.py img/sky.png
    VERBATIM
)
add_custom_command(
    OUTPUT ground_tiles.bin ground_map.bin ground_pal.bin
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/img/png2bin.py
        tiles 4 ${CMAKE_CURRENT_SOURCE_DIR}/img/ground.png
        ground_tiles.bin ground_map.bin ground_pal.bin
    DEPENDS img/png2bin.py img/ground.png
    VERBATIM
)

add_executable(flappycoo)
rp6502_map(flappycoo src/xram.h "XRAM_.*")
rp6502_asset(flappycoo help src/help.txt)
rp6502_asset(flappycoo XRAM(XRAM_COO_PALETTE) ${CMAKE_CURRENT_BINARY_DIR}/coo_pal.bin)
rp6502_asset(flappycoo XRAM(XRAM_SKY_PALETTE) ${CMAKE_CURRENT_BINARY_DIR}/sky_pal.bin)
rp6502_asset(flappycoo XRAM(XRAM_GROUND_PALETTE) ${CMAKE_CURRENT_BINARY_DIR}/ground_pal.bin)
rp6502_asset(flappycoo XRAM(XRAM_PIPE_PALETTE) ${CMAKE_CURRENT_BINARY_DIR}/pipe_pal.bin)
rp6502_asset(flappycoo XRAM(XRAM_LOGO_PALETTE) ${CMAKE_CURRENT_BINARY_DIR}/logo_pal.bin)
rp6502_asset(flappycoo XRAM(XRAM_SKY_MAP) ${CMAKE_CURRENT_BINARY_DIR}/sky_map.bin)
rp6502_asset(flappycoo XRAM(XRAM_GROUND_MAP) ${CMAKE_CURRENT_BINARY_DIR}/ground_map.bin)
rp6502_asset(flappycoo XRAM(XRAM_SKY_TILES) ${CMAKE_CURRENT_BINARY_DIR}/sky_tiles.bin)
rp6502_asset(flappycoo XRAM(XRAM_GROUND_TILES) ${CMAKE_CURRENT_BINARY_DIR}/ground_tiles.bin)
rp6502_asset(flappycoo XRAM(XRAM_COO) ${CMAKE_CURRENT_BINARY_DIR}/coo.bin)
rp6502_asset(flappycoo XRAM(XRAM_LOGO) ${CMAKE_CURRENT_BINARY_DIR}/logo.bin)
rp6502_asset(flappycoo XRAM(XRAM_PIPE_BODY) ${CMAKE_CURRENT_BINARY_DIR}/pipe_body.bin)
rp6502_asset(flappycoo XRAM(XRAM_PIPE_CAP) ${CMAKE_CURRENT_BINARY_DIR}/pipe_cap.bin)
rp6502_executable(flappycoo DATA default RESET default)
rp6502_web(flappycoo PAGE web/index.html)
target_sources(flappycoo PRIVATE
    src/game.c
    src/input.c
    src/main.c
    src/sound.c
    src/video.c
)

# Emulator scripts that play the game ROM
# WSL uses the Windows build of the emulator, as the rest of the SDK does.
# The Linux build needs GL, X11 and ALSA libraries that the WSL install
# steps leave out.
if(CMAKE_HOST_SYSTEM_VERSION MATCHES "[Mm]icrosoft")
    set(emulator ${RP6502_TOOLS_DIR}/rp6502-emu.exe)
else()
    set(emulator ${RP6502_TOOLS_DIR}/rp6502-emu)
endif()
foreach(script keyboard tablet gamepad screenshots)
    add_test(NAME ${script}
        COMMAND ${emulator}
            --script ${CMAKE_CURRENT_SOURCE_DIR}/tests/${script}.script
            --mute --seed 1 --save-dir ${CMAKE_CURRENT_BINARY_DIR}/save
            ${CMAKE_CURRENT_BINARY_DIR}/flappycoo.rp6502
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${script} PROPERTIES TIMEOUT 60)
endforeach()
