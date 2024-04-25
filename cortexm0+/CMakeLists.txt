cmake_minimum_required(VERSION 3.10)
set(PICO_PLATFORM rp2040)
include(pico_sdk_import.cmake)

project(web C CXX ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(web
        server_pico.c
        )

target_link_libraries(web pico_stdlib)
pico_add_extra_outputs(web)
