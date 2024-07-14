# CMakeList.txt : CMake project for Skywall, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Skywall")

# Add source to this project's executable.
add_executable (Skywall "uci.cpp"  "globals.h" "globals.cpp" "tunable.cpp" "eval.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Skywall PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
if(MSVC)
  target_compile_options(Skywall PUBLIC "/O2")
  target_link_options(Skywall PUBLIC "/INCREMENTAL")
endif()