# CMakeList.txt : CMake project for Skywall, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(Skywall VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

project ("Skywall")

set(SRC_Files
"uci.cpp"
"globals.cpp"
"tunable.cpp"
"eval.cpp"
"magics.cpp"
"move.cpp"
"tests.cpp"
"search.cpp"
"board.cpp"
)

# Add source to this project's executable.
add_executable (Skywall ${SRC_Files} "magics.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Skywall PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

target_compile_options(Skywall PUBLIC -Wall -Wextra -Wpedantic -march=native -O3 -flto)

target_link_options(Skywall PUBLIC -fsanitize=undefined)
target_link_libraries(Skywall -static)
