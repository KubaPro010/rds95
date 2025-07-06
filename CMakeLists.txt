cmake_minimum_required(VERSION 3.10)

project(rds95 VERSION 1.5)

add_compile_options(-Wall -Werror -Wextra -pedantic -O2 -std=c18 -march=native -DVERSION=\"${PROJECT_VERSION}\")

file(GLOB INIH_FILES "inih/*.c")
add_library(inih OBJECT ${INIH_FILES})

file(GLOB SOURCES src/*.c)

add_executable(rds95 ${SOURCES})

target_link_libraries(rds95 PRIVATE m pthread pulse pulse-simple inih)

install(TARGETS rds95 DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
