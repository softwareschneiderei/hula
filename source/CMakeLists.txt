add_library(hula_core
  "uncased_name.cpp"
  "uncased_name.hpp"
  "device_server_spec.cpp"
  "device_server_spec.hpp"
  "types.hpp"
  "types.cpp"
  "code_generator.hpp"
  "code_generator.cpp")

target_include_directories(hula_core
  INTERFACE .)

target_link_libraries(hula_core
  PUBLIC CONAN_PKG::toml11
  PUBLIC CONAN_PKG::fmt)
