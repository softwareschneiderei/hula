cmake_minimum_required(VERSION 3.11)
project(hula_demo)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  message(FATAL_ERROR "Please run 'conan install' first!")
endif()
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(hula_demo
  hula_cool_camera.cpp
  hula_cool_camera.hpp
  main.cpp)

target_link_libraries(hula_demo
  CONAN_PKG::cpptango)