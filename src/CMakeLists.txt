# This is from kuba's version of MiniRDS

cmake_minimum_required(VERSION 3.10)

# Project name and version
project(minirds VERSION 1.0)

# Define options
option(ODA_RTP "Enable ODA (RT+)" ON)

option(STATIC_LIBSAMPLERATE "Use a static libsamplerate library (.a)" OFF)

set(LIBSAMPLERATE_DIR "./libsamplerate" CACHE STRING "Directory for static libsamplerate")

# Set compiler and flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -pedantic -O2 -std=c18 -DVERSION=\"${PROJECT_VERSION}\"")

# Define sources
set(SOURCES
    minirds.c
    waveforms.c
    rds.c
)

if(ODA_RTP)
    add_definitions(-DODA)
    add_definitions(-DODA_RTP)
endif()

set(SOURCES
    ${SOURCES}
    fm_mpx.c
    control_pipe.c
    resampler.c
    modulator.c
    lib.c
    ascii_cmd.c
)

# Define the executable
add_executable(minirds ${SOURCES})

# Handle libsamplerate linkage
if(STATIC_LIBSAMPLERATE)
    target_include_directories(minirds PRIVATE ${LIBSAMPLERATE_DIR}/include)
    target_link_libraries(minirds PRIVATE ${LIBSAMPLERATE_DIR}/lib/libsamplerate.a)
else()
    find_library(SAMPLERATE_LIBRARY samplerate)
    target_link_libraries(minirds PRIVATE ${SAMPLERATE_LIBRARY})
endif()

# Link additional libraries
target_link_libraries(minirds PRIVATE m pthread pulse pulse-simple)

# Install target
install(TARGETS minirds DESTINATION /usr/local/bin)
# Add profiling flags globally
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
