cmake_minimum_required(VERSION 3.16)

project(tinf C)

include(CheckCCompilerFlag)
include(CTest)

# Check if tinf is the top-level project (standalone), or a subproject
set(_tinf_standalone FALSE)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(_tinf_standalone TRUE)
endif()

# TINF_BUILD_TESTING controls if tinf adds testing support
#
# When built standalone, it defaults to the value of BUILD_TESTING if set.
# An optional prefix for the test names can be set with TINF_TEST_PREFIX.
set(_tinf_testing_default ON)
if(_tinf_standalone)
  if(DEFINED BUILD_TESTING)
    set(_tinf_testing_default ${BUILD_TESTING})
  endif()
else()
  set(_tinf_testing_default OFF)
endif()
option(TINF_BUILD_TESTING "Add testing support" ${_tinf_testing_default})
unset(_tinf_testing_default)

mark_as_advanced(TINF_TEST_PREFIX)

# Take a list of compiler flags and add those which the compiler accepts to
# the COMPILE_OPTIONS directory property
function(add_valid_c_compile_options)
  foreach(flag IN LISTS ARGN)
    string(REGEX REPLACE "[^a-zA-Z0-9]+" "_" flag_var "CFLAG_${flag}")
    check_c_compiler_flag("${flag}" ${flag_var})
    if(${flag_var})
      add_compile_options("${flag}")
    endif()
  endforeach()
endfunction()

if(MSVC)
  add_valid_c_compile_options(/W3)
else()
  add_valid_c_compile_options(-Wall -Wextra -pedantic)
endif()

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
target_include_directories(tinf PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>)

#
# tgunzip
#
add_executable(tgunzip examples/tgunzip/tgunzip.c)
target_link_libraries(tgunzip PRIVATE tinf)
if(MSVC)
  target_compile_definitions(tgunzip PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

#
# Tests
#
if(TINF_BUILD_TESTING)
  add_executable(test_tinf test/test_tinf.c)
  target_link_libraries(test_tinf PRIVATE tinf)
  if(MSVC)
    target_compile_definitions(test_tinf PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()

  add_test("${TINF_TEST_PREFIX}tinf" test_tinf)
endif()
