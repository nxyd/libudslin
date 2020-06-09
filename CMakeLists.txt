cmake_minimum_required(VERSION 3.13.0)
project(udslin)

# Specify build paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(COLOR_OUTPUT "Use color ouput in terminal" ON)

if(COLOR_OUTPUT)
    add_definitions(-DCOLOR_OUTPUT=1)
endif(COLOR_OUTPUT)

# Add a _d to debug binaries
set(CMAKE_DEBUG_POSTFIX "_d")

set(SRC_FILES
    src/LinDrv.cpp
    src/LinTp.cpp
    src/LUDS.cpp
)

set(HEADER_FILES
    src/RingBuffer.h
    src/LinDrv.h
    src/LinTp.h
    src/LUDS.h
)

if(MSVC)
    set(SRC_FILES ${SRC_FILES} UDSLIN.def)
endif()

if(MINGW)
    set(CMAKE_CXX_FLAGS "-std=c++11")
    add_definitions(-DUSE_UDSLIN_EXPORTS)
endif()

add_library(${PROJECT_NAME} SHARED
    ${SRC_FILES}
    ${HEADER_FILES}
    resources/winres.rc
)

target_link_libraries(${PROJECT_NAME} Winmm)
