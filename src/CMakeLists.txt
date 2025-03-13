cmake_minimum_required(VERSION 3.10)

# Project name and version
project(rds95 VERSION 1.0)

# Set compiler and flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -pedantic -O2 -std=c18 -march=native -DVERSION=\"${PROJECT_VERSION}\"")

# Define sources
set(SOURCES
    rds95.c
    waveforms.c
    rds.c
    control_pipe.c
    modulator.c
    lib.c
    ascii_cmd.c
)

# Define the executable
add_executable(rds95 ${SOURCES})

# Link additional libraries
target_link_libraries(rds95 PRIVATE m pthread pulse pulse-simple)

# Install target
install(TARGETS rds95 DESTINATION /usr/local/bin)