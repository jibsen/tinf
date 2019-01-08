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

# TINF_BUILD_TESTING controls if tinf adds testing support
#
# When built standalone, it defaults to the value of BUILD_TESTING if set.
# An optional prefix for the test names can be set with TINF_TEST_PREFIX.
set(tinf_testing_default ON)
if(tinf_standalone)
  if(DEFINED BUILD_TESTING)
    set(tinf_testing_default ${BUILD_TESTING})
  endif()
else()
  set(tinf_testing_default OFF)
endif()
option(TINF_BUILD_TESTING "Add testing support" ${tinf_testing_default})
unset(tinf_testing_default)

mark_as_advanced(TINF_TEST_PREFIX)

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

#
# Tests
#
if(TINF_BUILD_TESTING)
  add_executable(test_tinf test/test_tinf.c)
  target_link_libraries(test_tinf tinf)
  if(MSVC)
    target_compile_definitions(test_tinf PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()

  add_test("${TINF_TEST_PREFIX}tinf" test_tinf)
endif()
