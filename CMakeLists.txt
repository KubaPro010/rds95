cmake_minimum_required(VERSION 3.10)

project(rds95 VERSION 1.4)

add_compile_options(-Wall -Werror -Wextra -pedantic -O2 -std=c18 -march=native -DVERSION=\"${PROJECT_VERSION}\")

file(GLOB SOURCES src/*.c)

add_executable(rds95 ${SOURCES})

target_link_libraries(rds95 PRIVATE m pthread pulse pulse-simple)

install(TARGETS rds95 DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
