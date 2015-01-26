project(al_wildmidi_loader C)
cmake_minimum_required(VERSION 2.8)

set(TEST_EXE "al_WM_test")


#set the build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are:
        None Debug Release RelWithDebInfo MinSizeRel"
      FORCE)
endif()

# Collect manually BUT RECURSIVELY sources in directories.
# NOTE: this as well as file globbing is DISCOURAGED by the cmake dev team in
# countless places on the web (!)
# BUT: I can't be arsed to type singularly all source files...sorry!
# This means that when in doubt CMAKE has to be re-run MANUALLY cause the check
# in the makefile for a changed CMakeLists file would fail and something wonky
# may happen during build.
aux_source_directory(${PROJECT_SOURCE_DIR} SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/loader SRC_LOADER_LIST)

set(GLOB_SOURCE ${SRC_LIST} ${SRC_LOADER_LIST})

include_directories(${PROJECT_SOURCE_DIR}/loader)

# find all header files recursively in the project tree to add in the global
# source tree to get all them in the IDE, regardless if used in a target or not
# (VERY handy with QtCreator ^__^")
file(GLOB_RECURSE global_headers "*.h")
set(GLOB_SOURCE ${GLOB_SOURCE} ${global_headers})
add_custom_target(global_fake SOURCES ${GLOB_SOURCE})

# set standard libs (still for WIndows only)
set(STANDARD_WIN_LIBS kernel32 user32 gdi32 winspool shell32 ole32 oleaut32
                      uuid comdlg32 advapi32 winmm psapi shlwapi)

# set Allegro Lib in static-debug form
#set(ALLEGRO_LIBS allegro-debug-static allegro_dialog-debug-static
#                 allegro_audio-debug-static allegro_acodec-debug-static
#                 allegro_primitives-debug-static allegro_font-debug-static
#                 allegro_ttf-debug-static allegro_image-debug-static
#                 allegro_color-debug-static)

# set Allegro Lib in dynamic-debug form
set(ALLEGRO_LIBS allegro-debug allegro_dialog-debug allegro_audio-debug
                 allegro_acodec-debug allegro_primitives-debug
                 allegro_font-debug allegro_ttf-debug allegro_image-debug
                 allegro_color-debug)

# set Allegro Static Lib dependencies
set(DEP_ALLEG_LIBS ogg opengl32)

#set Wildmidi deps
set(WILDMIDI_LIBS WildMidi)


# set c++11 flags
# we use with C++11 standard features when possible and statically link
# the std libs (to ease distribution)
# NOTE: this might still inject a dependency to libwinpthreads on Windows
# to avoid that use a NON-posix threads enabled compiler.
# some handy g++ switches, we are still on windows only
set(CMAKE_CXX_FLAGS "-std=c99 -static-libstdc++ -static-libgcc")

aux_source_directory(. SRC_LIST)
add_executable(${TEST_EXE} ${GLOB_SOURCE})

target_link_libraries(${TEST_EXE} ${ALLEGRO_LIBS})
target_link_libraries(${TEST_EXE} ${DEP_ALLEG_LIBS})
target_link_libraries(${TEST_EXE} ${WILDMIDI_LIBS})

set(dest ${CMAKE_BINARY_DIR})
set(src ${CMAKE_SOURCE_DIR})
set(basename "DEMO.XMI")

add_custom_command(
            OUTPUT  "${dest}/${basename}"
            DEPENDS "${src}/${basename}"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${src}/${basename}" "${dest}/${basename}"
            )
