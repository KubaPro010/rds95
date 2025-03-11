cmake_minimum_required(VERSION 3.10)

# Project name and version
project(minirds VERSION 1.0)

# Define options
option(ODA_RTP "Enable ODA (RT+)" ON)

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
    control_pipe.c
    modulator.c
    lib.c
    ascii_cmd.c
)

# Define the executable
add_executable(minirds ${SOURCES})

# Link additional libraries
target_link_libraries(minirds PRIVATE m pthread pulse pulse-simple)

# Install target
install(TARGETS minirds DESTINATION /usr/local/bin)