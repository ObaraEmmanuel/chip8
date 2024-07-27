cmake_minimum_required(VERSION 3.16)
project(chip8 C)

set(CMAKE_C_STANDARD 99)
set(BUILD_DIR build)
IF (WIN32)
    # Change to your SDL lib installation
    set(SDL2_DIR "C:\\dev\\libs\\SDL2-2.30.4\\cmake")
ENDIF ()

find_package(SDL2 REQUIRED)
file(GLOB SRC src/*.c)

include_directories(${SRC} ${SDL2_INCLUDE_DIRS})

add_executable(chip8 ${SRC})
target_link_libraries(chip8 ${SDL2_LIBRARIES})

IF (NOT WIN32)
    target_link_libraries(chip8 m)
ELSE()
    target_link_libraries(chip8 winmm.lib)
ENDIF()

if(WIN32)
    get_target_property(SDL2_DLL SDL2::SDL2 IMPORTED_LOCATION)
    get_filename_component(SDL2_DLL_NAME "${SDL2_DLL}" NAME)
    add_custom_command(TARGET chip8 POST_BUILD
            MAIN_DEPENDENCY "${SDL2_DLL}"
            BYPRODUCTS "${SDL2_DLL_NAME}"
            COMMENT "Copying SDL2 DLL"
            COMMAND "${CMAKE_COMMAND}" -E copy "${SDL2_DLL}" "$<TARGET_FILE_DIR:chip8>/${SDL2_DLL_NAME}"
    )
endif()