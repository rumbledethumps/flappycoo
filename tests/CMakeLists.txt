cmake_minimum_required(VERSION 3.21)

# The unit tests, built with the host C compiler
project(flappycoo_tests C)

enable_testing()
add_executable(host_tests main.c test_game.c ../src/game.c)
target_include_directories(host_tests PRIVATE ../src)
set_target_properties(host_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)
# Both 6502 compilers make char unsigned.
target_compile_options(host_tests PRIVATE -funsigned-char -Wall -Wextra)
add_test(NAME host_tests COMMAND host_tests)
