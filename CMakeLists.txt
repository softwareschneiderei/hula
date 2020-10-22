cmake_minimum_required(VERSION 3.11)
project(hula)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  message(FATAL_ERROR "Please run 'conan install' first!")
endif()
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_subdirectory(source)
add_subdirectory(tests)

add_executable(hula
  main.cpp)

target_include_directories(hula
  PRIVATE .)

target_link_libraries(hula
  PUBLIC hula_core
  PUBLIC CONAN_PKG::toml11
  PUBLIC CONAN_PKG::fmt)