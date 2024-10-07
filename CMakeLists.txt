cmake_minimum_required(VERSION 3.13)
project(chasm)
set(APP_NAME "chasm")
set(CXX_STANDARD 23)

set(SOURCES src/cpu/cpu.cpp src/main.cpp src/dev.cpp src/cpu/ops.cpp src/cpu/mem.cpp src/video.cpp)

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

find_package(PkgConfig)

set(ENV{SDL3DIR} ${CMAKE_FIND_ROOT_PATH}/include/SDL3)

find_package(SDL3 REQUIRED COMPONENTS SDL3)
if(NOT SDL3_FOUND)
	message(FATAL_ERROR "SDL3 library not found!")
endif(NOT SDL3_FOUND)

# Additional CFLAGS suggested by the SDL library:
if(PKG_CONFIG_FOUND)
	execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --cflags-only-other sdl3
			OUTPUT_VARIABLE SDL3_OTHER_CFLAGS
			ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(PKG_CONFIG_FOUND)

# Test for large file support:
execute_process(COMMAND getconf LFS_CFLAGS
                OUTPUT_VARIABLE DETECTED_LFS_CFLAGS
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(DETECTED_LFS_CFLAGS)
	add_definitions(${DETECTED_LFS_CFLAGS})
	# message(STATUS "Large filesystem flags: ${DETECTED_LFS_CFLAGS}")
endif(DETECTED_LFS_CFLAGS)

add_definitions(-DSDL_MAIN_HANDLED)

include_directories(${SDL3_INCLUDE_DIRS})

check_symbol_exists(posix_memalign "stdlib.h" HAVE_POSIX_MEMALIGN)
check_symbol_exists(aligned_alloc "stdlib.h" HAVE_ALIGNED_ALLOC)
check_symbol_exists(_aligned_malloc "stdlib.h" HAVE__ALIGNED_MALLOC)

#set(WIN32_MODE 1)

if(WIN32_MODE)
    if(WIN32)
        set(GUIWIN_SOURCES src/gui/win/opencon.cpp)
        set(GUIWIN_RES src/gui/win/chasm-winicon.rc)
        #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
        add_definitions(-DSDL_MAIN_HANDLED)

        if(NOT CMAKE_RC_COMPILER)
            set(CMAKE_RC_COMPILER windres)
        endif(NOT CMAKE_RC_COMPILER)
        ENABLE_LANGUAGE(RC)
        set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -Ocoff -o <OBJECT> <SOURCE>")
        set_source_files_properties(${GUIWIN_RES} PROPERTIES LANGUAGE RC)
        add_executable(${APP_NAME} WIN32 ${GUIWIN_RES} ${SOURCES} ${GUIWIN_SOURCES})
    else()
        add_executable(${APP_NAME} ${SOURCES})
    endif(WIN32)
else()
    if(WIN32)
        add_definitions(-DSDL_MAIN_HANDLED)
        add_executable(${APP_NAME} ${SOURCES})
    else()
        add_executable(${APP_NAME} ${SOURCES})
    endif(WIN32)
endif(WIN32_MODE)

if(SDL3_OTHER_CFLAGS)
	target_compile_definitions(${APP_NAME} PRIVATE ${SDL3_OTHER_CFLAGS})
	# message(STATUS "Additional CFLAGS of SDL: ${SDL3_OTHER_CFLAGS}")
    message("-- SDL: using SDL3 ${SDL3_VERSION} with flags '${SDL3_OTHER_CFLAGS}'")
else()
    message("-- SDL: using SDL3 ${SDL3_VERSION}")
endif(SDL3_OTHER_CFLAGS)

add_compile_options(-Wall -Wextra -Wpedantic -O3 -mavx -march=native)
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 23)

target_link_libraries(${APP_NAME} ${SDL3_LIBRARIES})

