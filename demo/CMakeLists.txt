cmake_minimum_required(VERSION 3.11)
project(cool_camera)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  message(FATAL_ERROR "Please run 'conan install' first!")
endif()
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(cool_camera
  hula_cool_camera.cpp
  hula_cool_camera.hpp
  main.cpp)

target_link_libraries(cool_camera
  CONAN_PKG::cpptango)