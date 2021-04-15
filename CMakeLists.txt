cmake_minimum_required(VERSION 3.19)
project(chip8 C)

set(CMAKE_C_STANDARD 99)
set(BUILD_DIR build)

find_package(SDL2 REQUIRED)
file(GLOB SRC src/*.c)

include_directories(${SRC} ${SDL2_INCLUDE_DIRS})

add_executable(chip8 ${SRC})
target_link_libraries(chip8 ${SDL2_LIBRARIES})
target_link_libraries(chip8 m)