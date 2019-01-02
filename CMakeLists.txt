cmake_minimum_required(VERSION 3.0)

project(tinf C)

include(CTest)

# Check if tinf is the top-level project (standalone), or a subproject
set(tinf_standalone FALSE)
get_directory_property(tinf_parent_directory PARENT_DIRECTORY)
if(tinf_parent_directory STREQUAL "")
  set(tinf_standalone TRUE)
endif()
unset(tinf_parent_directory)

#
# tinf
#
add_library(tinf
  src/adler32.c
  src/crc32.c
  src/tinfgzip.c
  src/tinflate.c
  src/tinfzlib.c
  src/tinf.h
)
target_include_directories(tinf PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

#
# tgunzip
#
add_executable(tgunzip examples/tgunzip/tgunzip.c)
target_link_libraries(tgunzip tinf)
if(MSVC)
  target_compile_definitions(tgunzip PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
