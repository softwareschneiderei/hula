cmake_minimum_required(VERSION 3.11)
project(hula)

set(CMAKE_CXX_STANDARD 17)

find_package(toml11 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

add_subdirectory(source)
add_subdirectory(tests)

add_executable(hula
  main.cpp)

target_include_directories(hula
  PRIVATE .)

target_link_libraries(hula
  PUBLIC hula_core)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  message("Adding extra std::filesystem library for g++ 8.x series")
  target_link_libraries(hula
    PUBLIC stdc++fs)
endif()
