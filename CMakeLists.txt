cmake_minimum_required(VERSION 2.6)

project(tinf C)

option(BUILD_TEST "Build unit test" OFF)

include_directories(${PROJECT_SOURCE_DIR})

add_library(tinf src/tinflate.c src/tinfgzip.c src/tinfzlib.c src/adler32.c src/crc32.c src/tinf.h)

if(BUILD_TEST)
	add_executable(test_tinf test/test_tinf.c test/greatest.h src/tinf.h)
	target_link_libraries(test_tinf tinf)

	enable_testing()
	add_test(test_tinf test_tinf)
endif()
